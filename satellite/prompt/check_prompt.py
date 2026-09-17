#!/usr/bin/env python3
# satellite/prompt's line reader at a real terminal (PLAN M0.6): build/prompt_reader
# started with pty.fork(), so the pty is its CONTROLLING terminal -- a pipe would
# test nothing, because with no terminal there is no raw mode and no key is a byte.
# It is typed at, and checked on the SCREEN a person would see: a redraw's erasing
# is invisible in the bytes, so a small VT100 below keeps the screen.
#
#     make build/prompt_reader && python3 satellite/prompt/check_prompt.py   (check.sh runs it)

import fcntl, os, pty, re, select, signal, struct, subprocess, sys, termios, time, unicodedata

READER = os.environ.get('PROMPT_READER', 'build/prompt_reader')   # another build's reader, to mutate it
failed = 0


def check(right, what):
    global failed
    failed += 0 if right else 1
    print(('ok   ' if right else 'FAIL ') + what, flush=True)


def cells(ch):
    if unicodedata.combining(ch):
        return 0
    return 2 if unicodedata.east_asian_width(ch) in 'WF' else 1


class Screen:
    """Enough of a VT100 for a line editor: text with its cells, the held wrap,
    CR, LF, cursor moves, erase line, erase screen, home, and bracketed paste."""

    def __init__(self, width, height):
        self.width, self.height = width, height
        self.rows = [[' '] * width for _ in range(height)]
        self.row = self.col = 0
        self.held = False
        self.paste_mode = self.paste_was_on = False
        self.not_csi = 0          # an ESC that did not start a CSI reached the terminal
        self.escape = None
        self.decoder = __import__('codecs').getincrementaldecoder('utf-8')('replace')

    def line_feed(self):
        self.row += 1
        if self.row == self.height:
            self.rows.pop(0)
            self.rows.append([' '] * self.width)
            self.row = self.height - 1

    def put(self, ch):
        w = cells(ch)
        if w == 0:
            return
        if self.held or (self.col != 0 and self.col + w > self.width):
            self.col, self.held = 0, False
            self.line_feed()
        self.rows[self.row][self.col] = ch
        if w == 2 and self.col + 1 < self.width:
            self.rows[self.row][self.col + 1] = ''
        self.col += w
        if self.col >= self.width:
            self.col, self.held = self.width - 1, True

    def csi(self, text):
        final, params = text[-1], text[:-1]
        n = int(params) if params.isdigit() and int(params) > 0 else 1
        self.held = False
        if final == 'A':
            self.row = max(0, self.row - n)
        elif final == 'B':
            self.row = min(self.height - 1, self.row + n)
        elif final == 'C':
            self.col = min(self.width - 1, self.col + n)
        elif final == 'D':
            self.col = max(0, self.col - n)
        elif final == 'K':
            self.rows[self.row][self.col:] = [' '] * (self.width - self.col)
        elif final == 'J':
            self.rows = [[' '] * self.width for _ in range(self.height)]
        elif final == 'H':
            self.row = self.col = 0
        elif params == '?2004':
            self.paste_mode = final == 'h'
            self.paste_was_on = self.paste_was_on or self.paste_mode

    def feed(self, data):
        for ch in self.decoder.decode(data):
            if self.escape is not None:
                self.escape += ch
                if self.escape == '[':
                    continue
                if not self.escape.startswith('['):
                    self.not_csi += 1
                    self.escape = None
                elif 0x40 <= ord(ch) <= 0x7e:
                    self.csi(self.escape[1:])
                    self.escape = None
            elif ch == '\x1b':
                self.escape = ''
            elif ch == '\r':
                self.col, self.held = 0, False
            elif ch == '\n':
                self.held = False
                self.line_feed()
            elif ch >= ' ':
                self.put(ch)

    def text(self):
        return [''.join(r).rstrip() for r in self.rows]

    def shows(self, needle):
        return any(needle in r for r in self.text())


class Terminal:
    def __init__(self, width=40, height=30):
        pid, fd = pty.fork()
        if pid == 0:
            fcntl.ioctl(0, termios.TIOCSWINSZ, struct.pack('HHHH', height, width, 0, 0))
            os.execv(READER, [READER])
            os._exit(127)
        self.pid, self.fd, self.raw = pid, fd, bytearray()
        self.screen = Screen(width, height)
        self.status = None

    def pump(self, seconds):
        ready, _, _ = select.select([self.fd], [], [], seconds)
        if not ready:
            return False
        try:
            data = os.read(self.fd, 65536)
        except OSError:
            data = b''
        if not data:
            return False
        self.raw += data
        self.screen.feed(data)
        return True

    def wait_for(self, what, seconds=10):
        deadline = time.time() + seconds
        while time.time() < deadline:
            if (what(self) if callable(what) else self.screen.shows(what)):
                return True
            self.pump(0.05)
        return what(self) if callable(what) else self.screen.shows(what)

    def type(self, data, gap=0.0):
        view = memoryview(data)
        while view:
            _, writable, _ = select.select([], [self.fd], [], 0.05)
            if writable:
                view = view[os.write(self.fd, view[:4096]):]
                time.sleep(gap)
            while self.pump(0):
                pass
        self.pump(0.1)

    # WAIT FOR AN EMPTY PROMPT BEFORE TYPING A NEW LINE after a long answer: keys
    # that arrive before the reader is back at the prompt were typed while a line
    # ran, and D0.6.5 drops them.
    def at_prompt(self):
        return self.wait_for(lambda t: t.screen.text()[t.screen.row] == 'satl>')

    def resize(self, width, height=30):
        fcntl.ioctl(self.fd, termios.TIOCSWINSZ, struct.pack('HHHH', height, width, 0, 0))

    def finish(self, seconds=10):
        deadline = time.time() + seconds
        while time.time() < deadline:
            self.pump(0.05)
            pid, status = os.waitpid(self.pid, os.WNOHANG)
            if pid:
                while self.pump(0.05):
                    pass
                self.status = os.waitstatus_to_exitcode(status)
                return self.status
        os.kill(self.pid, signal.SIGKILL)
        os.waitpid(self.pid, 0)
        return 'killed'


def screen_order(t, *lines):
    text = [r for r in t.screen.text() if r]
    at = 0
    for r in text:
        if at < len(lines) and r == lines[at]:
            at += 1
    return at == len(lines)


t = Terminal()
check(t.wait_for('satl>'), 'the prompt is drawn')
t.type(b'hello\r')
check(t.wait_for('[hello]') and screen_order(t, 'satl> hello', '[hello]'), 'a typed line comes back below its prompt')
t.type(b'ac\x1b[Db\r')
t.type(b'xz\x1b')
t.type(b'[')
t.type(b'Dy\r')
check(t.wait_for('[xyz]') and t.screen.shows('[abc]'), 'Left inserts in the middle, and an arrow split across three writes still moves')
t.type(b'\x1b[A\r')
check(t.wait_for(lambda t: t.raw.count(b'[xyz]') == 2), 'Up brings back the last line')
t.type(b'half\x03')
t.type(b'after\r')
check(t.wait_for('[after]') and t.screen.shows('(interrupted)') and not t.screen.shows('[halfafter]'),
      'Ctrl-C abandons a half-typed line and the next line starts empty')
t.type(b'q\x1b\x03')
check(t.wait_for(lambda t: t.raw.count(b'(interrupted)') == 2), 'a bare ESC does not swallow the Ctrl-C after it')
t.type(b'\x1b[' + b'1' * 100000 + b'~ok\r')
check(t.wait_for('[ok]'), 'a 100,000-byte escape sequence, then a line that still reads')
t.type(b'\xc3x\r')
rows_from = lambda t, first: t.screen.text()[t.screen.text().index(first):] if first in t.screen.text() else []
check(t.wait_for('[x]'), 'a UTF-8 character cut short drops the half and keeps the x')
t.type(b'ab\x01\x04\r')
check(t.wait_for('[b]'), 'Ctrl-D on a line with text deletes forward')
t.type(b'\x1b[200~one\rtwo\rthree\x1b[201~')
check(t.wait_for('satl> three') and screen_order(t, 'satl> one', '[one]', 'satl> two', '[two]', 'satl> three'),
      'a pasted block of three lines: two answered in order, the unfinished third waits at the prompt')
t.type(b'\r')
check(t.wait_for('[three]'), '... and Enter finishes it')
t.at_prompt()
t.type(b'first\r\x1b[200~' + b''.join(b'P%05d\r' % i for i in range(20000)) + b'\x1b[201~')
check(t.wait_for(lambda t: b'[P19999]' in t.raw, 60) and re.findall(rb'\[P(\d+)\]', bytes(t.raw)) == [b'%05d' % i for i in range(20000)],
      'Enter and a paste of 20,000 lines in one burst: the paste arrives whole (003\'s TCSAFLUSH cut it)')
t.at_prompt()
t.type(b'\x1b[200~' + b''.join(b'A%05d\r' % i for i in range(20000)) + b'\x1b[201~\x1b[200~' + b''.join(b'B%05d\r' % i for i in range(3000)) + b'\x1b[201~')
t.wait_for(lambda t: b'[A19999]' in t.raw, 60)
t.at_prompt()
t.type(b'end_of_b\r')
t.wait_for('[end_of_b]', 60)
t.at_prompt()
answered_b = re.findall(rb'\[B(\d+)\]', bytes(t.raw))
check(re.findall(rb'\[A(\d+)\]', bytes(t.raw)) == [b'%05d' % i for i in range(20000)] and answered_b in ([], [b'%05d' % i for i in range(3000)])
      and not re.search(rb'\[[AB]\d{0,4}[^\d\]]', bytes(t.raw)),
      'two pastes back to back: the first whole, the second whole or dropped whole (D0.6.5), never cut or spliced (%d of it answered)' % len(answered_b))
t.type(b'\x1b[200~drop\rx1\rx2\r\x1b[201~')
t.at_prompt()
t.type(b'after_drop\r')
check(t.wait_for('[after_drop]') and b'[x1]' not in t.raw and b'[x2]' not in t.raw, 'discard_pending() drops the rest of a paste')
t.type(b'marker\r')
t.wait_for('[marker]')
t.at_prompt()
t.type(b'y' * 200 + b'\x1b[200~')
t.resize(40, 25)
t.type(b'\x1b[201~')
check(t.wait_for(lambda t: rows_from(t, '[marker]')[1:2] == ['satl> ' + 'y' * 34]),
      'a resize before a line was ever drawn erases nothing above the prompt')
t.type(b'\x15\r')
t.at_prompt()
before = len(t.raw)
t.type(b'w' * 800000 + b'\r', gap=0.002)
check(t.wait_for(lambda t: b'(800000 bytes)' in t.raw, 60) and len(t.raw) - before < 2000000,
      'an 800 KB line typed with no paste brackets, 4 KB every 2 ms: drawn when it stops, not at every gap (%d KB written)' % ((len(t.raw) - before) // 1000))
t.at_prompt()
t.type(b'\x1b[200~x\x1b]2;TITLE\x07y\x1b[201~\r')
check(t.wait_for('[x\\x1b]2;TITLE\\x07y]') and t.screen.not_csi == 0 and b'\x07' not in t.raw,
      'a pasted OSC is text: shown as \\x1b and \\x07 at the prompt and after, and never reaches the terminal')
t.type(b'\x1b[200~a\x1b[201~b\r')
check(t.wait_for('[ab]'), 'a paste holding ESC [ 201 ~ ends there, and what follows is typed')
t.type(b'wait\r')
t.wait_for('[wait]')
t.type(b'junk')
t.wait_for('(waited)')
t.at_prompt()
t.type(b'ok2\r')
check(t.wait_for('[ok2]') and b'[junkok2]' not in t.raw, 'keys typed while a line runs are thrown away (D0.6.5)')
t.at_prompt()
t.type(b'wait\r')
t.wait_for(lambda t: t.raw.count(b'[wait]') == 2)
t.type(b'\x1b[200~junk1\rjunk2\r')
t.wait_for(lambda t: t.raw.count(b'(waited)') == 2)
time.sleep(0.04)   # the reader is back, dropping what came during the run, when the paste's end arrives
t.type(b'junk3\r\x1b[201~')
t.at_prompt()
t.type(b'ok3\r')
check(t.wait_for('[ok3]') and b'[junk' not in t.raw, 'a paste begun while a line ran and ended after it is dropped whole (D0.6.5)')
t.type(b'\x0c')
check(t.wait_for(lambda t: t.screen.text()[0] == 'satl>' and not any(t.screen.text()[1:])), 'Ctrl-L clears the screen and draws the prompt at the top')
t.type(b'\x04')
check(t.finish() == 0 and t.screen.shows('(end) icanon=1 echo=1 isig=1'), 'Ctrl-D on an empty line ends, and the terminal is cooked again')
check(t.screen.paste_was_on and not t.screen.paste_mode, 'bracketed paste was on while reading, and is off after')

t = Terminal(width=20)
t.wait_for('satl>')
t.type('日本語日本語日本'.encode())
check(t.wait_for(lambda t: t.screen.text()[:2] == ['satl> 日本語日本語日', '本']), 'CJK is two cells a character, and wraps where the terminal wraps it')
t.type(b'\r\x1b[200~' + b'a' * 13 + '日'.encode() + b'\x1b[201~')
check(t.wait_for(lambda t: 'satl> ' + 'a' * 13 in t.screen.text() and '日' in t.screen.text()),
      'a two-cell character that does not fit the last cell starts the next row')
t.type(b'\x7f\x15' + b'b' * 14)
t.type(b'c\x7f\x1b[DX\r')
check(t.wait_for('[' + 'b' * 13 + 'Xb]') and rows_from(t, '[日本語日本語日本]')[:4] == ['[日本語日本語日本]', 'satl> ' + 'b' * 13 + 'X', 'b', '[' + 'b' * 13 + 'Xb]'],
      'a line exactly as wide as the screen: the cursor wraps, backs up over the edge, and inserts there')
t.type(b'c' * 14 + b'\r')
check(t.wait_for('[' + 'c' * 14 + ']') and rows_from(t, 'satl> ' + 'c' * 14)[:2] == ['satl> ' + 'c' * 14, '[' + 'c' * 14 + ']'],
      'a line exactly as wide as the screen: its answer on the next row, with no blank row between')
t.type(b'resize me')
t.screen = Screen(10, 30)   # what is on the screen now is the line alone, cursor on its second row at the new width
t.screen.row, t.screen.col = 5, 5
t.resize(10)
check(t.wait_for(lambda t: t.screen.text()[4:6] == ['satl> resi', 'ze me']),
      'a resize while typing redraws at once, counting the rows at the new width')
t.type(b'X')
check(t.wait_for(lambda t: t.screen.text()[4:6] == ['satl> resi', 'ze meX']), '... and typing goes on there')
t.type(b'\r')
check(t.wait_for(lambda t: b'[resize meX]' in t.raw), '... and the line is whole')   # 12 cells: it wraps on this screen
t.at_prompt()
t.type(b'\x1b[200~' + b''.join(b'L%d\r' % i for i in range(100000)) + b'\x1b[201~')
check(t.wait_for(lambda t: b'[L99999]' in t.raw, 60) and re.findall(rb'\[L(\d+)\]', bytes(t.raw)) == [b'%d' % i for i in range(100000)],
      'a paste of 100,000 lines: every line answered, in order')
t.at_prompt()
t.type(b'\x1b[200~' + b'z' * 1000000 + b'\x1b[201~\r')
check(t.wait_for(lambda t: b'(1000000 bytes)' in t.raw, 60), 'a pasted line of 1,000,000 bytes arrives whole')
t.type(b'exit\r')
check(t.finish() == 0, 'exit ends it')

piped = subprocess.run([READER], input=b'a\nb\x00c\n\nlast', capture_output=True)
check(piped.stdout == b'[a]\n[b\\x00c]\n[]\n[last]\n(end)\n', 'from a pipe: no prompt text, a NUL kept, and a last line with no newline')
r, w = os.pipe()
os.set_blocking(r, False)
reader = subprocess.Popen([READER], stdin=r, stdout=subprocess.PIPE)
os.close(r)
try:
    os.write(w, b'first\n')
    time.sleep(0.3)
    os.write(w, b'second\n')   # a broken pipe here: the reader had already ended
except BrokenPipeError:
    pass
os.close(w)
check(reader.communicate(timeout=10)[0] == b'[first]\n[second]\n(end)\n', 'a non-blocking stdin is waited on, not taken as its end')
with open('/dev/null', 'rb') as null:
    check(subprocess.run([READER], stdin=null, capture_output=True).stdout == b'(end)\n', '< /dev/null ends at once')
with open(READER, 'rb') as binary:
    out = subprocess.run([READER], stdin=binary, capture_output=True).stdout
check(out.endswith(b'(end)\n') and b'\x1b' not in out and b'\x00' not in out, 'the reader\'s own binary as input: every byte shown escaped')

print('every prompt check passed' if failed == 0 else 'a prompt check FAILED', flush=True)
sys.exit(1 if failed else 0)
