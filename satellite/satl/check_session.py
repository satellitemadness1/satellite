#!/usr/bin/env python3
# `satl --repl` at a real terminal (PLAN M0.6): the session, the directory words
# and the table, driven through pty.fork() so the pty is satl's CONTROLLING
# terminal -- the only way Ctrl-C is the key it is, and not a byte nobody reads.
#
#     make && python3 satellite/satl/check_session.py     (check.sh runs it)
#
# THE TERMINAL AND ITS SCREEN ARE satellite/prompt/check_prompt.py's, exec'd here
# rather than copied: one VT100 to fix, and a session is the same screen with satl
# on the far end of it instead of the reader alone.

import datetime, os, shutil, signal, subprocess, sys, tempfile, termios, time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
with open(os.path.join(ROOT, 'satellite', 'prompt', 'check_prompt.py')) as harness:
    text = harness.read()
exec(text[:text.index('\nt = Terminal()')])       # Screen, Terminal, check(), failed

SATL = os.environ.get('SATL', os.path.join(ROOT, 'build', 'satl'))
READER_ARGS = [SATL, '--repl']

# THE SESSION'S PROMPT (the author, 2026-09-22): [satellite][user][folder]>>, in
# bold bright white with black brackets. The folder is wherever the session is.
import pwd
USER = pwd.getpwuid(os.geteuid()).pw_name
def prompt_in(folder):
    return '[satellite][%s][%s]>> ' % (USER, folder)
Terminal.is_a_prompt = staticmethod(lambda row: row.startswith('[satellite][' + USER + '][') and row.endswith(']>>'))

# A HOME OF ITS OWN, BEFORE THE FIRST TERMINAL (KEY_PLAN.md KEY-1): a line typed at a console makes
# ~/.satl/console/<today>/ in the HOME satl is given, and this script run by hand would hand it the
# person's. Made as check.sh makes build/check-home -- .satl, then satl --rebuild -- and put in
# os.environ, which every Terminal and every satl below starts from.
def fresh_home():
    home = tempfile.mkdtemp(prefix='session_home_')
    os.mkdir(os.path.join(home, '.satl'))
    subprocess.run([SATL, '--rebuild'], env=dict(os.environ, HOME=home),
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return home
homes = [fresh_home()]
os.environ['HOME'] = homes[0]

room = tempfile.mkdtemp(prefix='session_check_')
open(os.path.join(room, 'alpha'), 'w').close()
open(os.path.join(room, '.hidden'), 'w').close()
os.mkdir(os.path.join(room, 'nested'))

t = Terminal(width=100)
# THE AUTHOR'S NOTE OPENS THE PROMPT (2026-10-06), in place of the line that said how to leave it.
check(t.wait_for('FOR DETAILED INFO SEE "satellite.help()"'), 'the session opens with the author\'s note')
check(t.wait_for(prompt_in(os.getcwd()).rstrip()), 'the prompt is [satellite][user][folder]>>')
row = t.screen.row
drawn = t.screen.text()[row]
BLACK, WHITE = (False, 30), (True, 97)
brackets = [i for i, ch in enumerate(drawn) if ch in '[]']
letters = [i for i, ch in enumerate(drawn) if ch not in '[] ']
check(len(brackets) == 6 and all(t.screen.look(row, i) == BLACK for i in brackets),
      '... its six brackets are black')
check(letters and all(t.screen.look(row, i) == WHITE for i in letters),
      '... and every letter of it is bold white, the >> too')
t.type(b'x')
check(t.wait_for(lambda t: t.screen.text()[row].endswith('>> x')) and
      t.screen.look(row, len(drawn) + 1) == (False, None),
      '... and what a person types after it is in the terminal\'s own colour')
t.type(b'\x7f')
t.at_prompt()

t.type(b'satellite.console.display("typed and ran")\r')
# The needle must be the OUTPUT row and not the echo of the line that typed it,
# which holds the same words.
check(t.wait_for(lambda t: 'typed and ran' in t.screen.text()) and
      screen_order(t, prompt_in(os.getcwd()) + 'satellite.console.display("typed and ran")', 'typed and ran'),
      'a typed line runs, below the line that typed it')

t.at_prompt()
t.type(b'\x1b[A\r')
check(t.wait_for(lambda t: t.raw.count(b'typed and ran') >= 3), 'Up recalls the last line and runs it again')

t.at_prompt()
t.type(('satellite.directory.change("%s")\r' % room).encode())
t.at_prompt()
t.type(b'satellite.directory.list()\r')
check(t.wait_for(lambda t: t.screen.shows('name') and t.screen.shows('permissions')), 'list() draws the table')
# ABOVE IT, THE FREE SPACE (the author, 2026-09-25: "write it in white color"), the
# prompt's own bold bright white.
free_row = next((i for i, text in enumerate(t.screen.text()) if text.startswith('FREE SPACE IN DIRECTORY: ')), None)
check(free_row is not None and t.screen.text()[free_row].endswith(' mb') and
      all(t.screen.look(free_row, i) == WHITE for i, ch in enumerate(t.screen.text()[free_row]) if ch != ' '),
      '... under a line of the free space in mb, every letter of it bold white')
check(t.wait_for(lambda t: t.screen.shows('alpha') and t.screen.shows('.hidden') and t.screen.shows('nested')),
      '... of the directory change() moved to, dotfiles kept')
check(t.at_prompt() and t.screen.text()[t.screen.row] == prompt_in(os.path.realpath(room)).rstrip(),
      'the prompt names the folder change() moved to')
check(not t.screen.shows(' .  ') and not t.screen.shows('..'), '... without . and ..')

t.at_prompt()
t.type(b'satellite.directory.change("/")\r')
t.at_prompt()
t.type(b'satellite.directory.list("%s")\r' % room.encode())
check(t.wait_for(lambda t: t.raw.count(b'alpha') >= 2), 'list(d) lists somewhere else, after change moved away')

# CTRL-C WHILE THE TABLE IS COUNTED (the author, 2026-09-25: a directory's row walks
# everything under it, for its size, files and sub). /usr is hundreds of thousands of
# names on any Linux -- 2.3 s here -- so the key lands mid-walk. It is pressed once
# the line has been handed off (bracketed paste goes off, ESC[?2004l), when the
# terminal is cooked and Ctrl-C is SIGINT; the answer is the library's own for its
# read: 130 interrupted, and no table.
t.at_prompt()
tables, handed_off = t.raw.count(b'permissions'), t.raw.count(b'\x1b[?2004l')
t.type(b'satellite.directory.list("/usr")\r')
t.wait_for(lambda t: t.raw.count(b'\x1b[?2004l') > handed_off)
t.type(b'\x03')
check(t.wait_for(lambda t: b'/usr (machine_code: 130 interrupted)' in t.raw) and t.at_prompt() and
      t.raw.count(b'permissions') == tables,
      'Ctrl-C while the table is counted stops it: 130 interrupted, no table, the prompt back')

t.at_prompt()
t.type(b'satellite.console.display("half a line')
t.type(b'\x03')
# WAIT FOR THE PROMPT THE INTERRUPT DREW, exactly as every other step here waits.
# Typing straight after the Ctrl-C raced D0.6.5, which DROPS keys that arrive
# while a line is still being dealt with: the next line was eaten and the check
# failed 3 times in 5 under load (2026-09-17). at_prompt() cannot pass early --
# the row still holds the half-typed line until the interrupt abandons it.
t.at_prompt()
t.type(b'satellite.console.display("after the interrupt")\r')
check(t.wait_for('after the interrupt') and not t.screen.shows('half a line"satellite'),
      'Ctrl-C while typing abandons the line, and the next line is whole')

t.at_prompt()
t.type(b'satellite.directory.list("/nowhere_at_all")\r')
check(t.wait_for(lambda t: b'directory_not_found' in t.raw),   # the message is longer than the screen is wide
      'a directory that is not there is refused by name')
t.at_prompt()
t.type(b'satellite.console.display("the session goes on")\r')
check(t.wait_for('the session goes on'), '... and the session goes on')

# THE PROMPT'S SHORT SPELLINGS (the author, 2026-10-10): directory("d") and dir("d") are
# satellite.directory.change("d"), and list() is satellite.directory.list() -- typed at the
# prompt only (satl/prompt_shorts.hpp; a program is told the word, capsule_reach.cpp).
t.at_prompt()
t.type(('dir("%s")\r' % room).encode())
check(t.at_prompt() and t.screen.text()[t.screen.row] == prompt_in(os.path.realpath(room)).rstrip(),
      'dir("d") moves the session, as satellite.directory.change("d") does')
t.type(b'directory("/")\r')
check(t.at_prompt() and t.screen.text()[t.screen.row] == prompt_in('/').rstrip(),
      '... and directory("d") moves it too')
t.type(('directory ("%s")\r' % room).encode())
check(t.at_prompt() and t.screen.text()[t.screen.row] == prompt_in(os.path.realpath(room)).rstrip(),
      '... with a space before its bracket as well')
seen = t.raw.count(b'nested')
t.type(b'list()\r')
check(t.wait_for(lambda t: t.raw.count(b'nested') > seen and t.screen.shows('permissions')),
      'list() draws the table satellite.directory.list() draws, of the folder directory() moved to')
t.at_prompt()
t.type(b'directory("/")\r')
t.at_prompt()
seen = t.raw.count(b'alpha')
t.type(('list("%s")\r' % room).encode())
check(t.wait_for(lambda t: t.raw.count(b'alpha') > seen), 'list("d") lists somewhere else, as satellite.directory.list("d") does')
t.at_prompt()
t.type(b'\x1b[A')
check(t.wait_for(lambda t: t.screen.text()[t.screen.row].endswith('list("%s")' % room)),
      'Up recalls the line as it was typed, short spelling and all')
t.type(b'\x03')

# BLOCKS AT THE PROMPT (the author, 2026-09-25): a line that opens a block has its { written
# for it and the next line starts one level in; a spacesuit gets its constructor, and then its
# protected and public sections as each } closes the one before; a } typed on a line that is
# only indentation steps back first; what the block declares is kept, what it does runs.
t.at_prompt()
t.type(b'satellite.spacesuit dog()\r')
check(t.wait_for('...     satellite.constructor()') and screen_order(t, '... {', '...     satellite.constructor()', '...     {'),
      'satellite.spacesuit dog() + Enter writes { and satellite.constructor() with its {')
t.type(b'satellite.console.display("made")\r')
t.type(b'}\r')
check(t.wait_for('...     satellite.protected') and screen_order(t, '...     }', '...     satellite.protected', '...     {'),
      '... the constructor\'s } steps back one level, and satellite.protected is written with its {')
t.type(b'satellite.variable.string name = "Rex"\r')
t.type(b'}\r')
check(t.wait_for('...     satellite.public') and screen_order(t, '...     satellite.protected', '...     satellite.public'),
      '... and after protected\'s }, satellite.public with its {')
t.type(b'satellite.capsule call_speak()\r')
check(t.wait_for(lambda t: t.screen.text().count('...         {') >= 1),
      '... a capsule inside it has its { written too')
t.type(b'satellite.console.display(name + " says woof")\r')
t.type(b'}\r')
t.type(b'}\r')
t.type(b'}\r')
t.at_prompt()
t.type(b'dog d()\r')
check(t.wait_for('made'), '... and the spacesuit was kept: an object of it is made at the prompt')
t.at_prompt()
t.type(b'd.call_speak()\r')
check(t.wait_for('Rex says woof'), '... and its capsule runs')
t.at_prompt()
t.type(b'satellite.capsule twice(satellite.variable.number n)\r')
check(t.wait_for(lambda t: t.screen.text()[t.screen.row].startswith('...') and t.screen.col == 8 and
                 t.screen.text()[t.screen.row - 1] == '... {'),
      'satellite.capsule + Enter writes { and starts the next line one level in')
t.type(b'satellite.return(n * 2)\r')
t.type(b'}\r')
t.at_prompt()
t.type(b'satellite.variable.number big = twice(21)\r')
t.at_prompt()
# EACH LINE OF THE BLOCK WAITS FOR THE READER TO BE BACK -- an empty "..." -- as at_prompt() waits for an empty
# prompt: the session writes the { for a line before the reader is reading again, and keys that arrive in
# between were typed while a line ran, so D0.6.5 drops them. Typed without waiting, this check raced the reader
# and lost a line now and then (2026-10-06: 1 run in 6 on build 0029, 3 in 4 on 0031).
in_the_block = lambda: t.wait_for(lambda t: t.screen.text()[t.screen.row].strip() == '...')
t.type(b'satellite.statement.if(big == 42)\r'); in_the_block()
t.type(b'satellite.console.display("forty two")\r'); in_the_block()
t.type(b'}\r'); in_the_block()
t.type(b'satellite.statement.else\r'); in_the_block()
t.type(b'satellite.console.display("not it")\r'); in_the_block()
t.type(b'}\r')
check(t.wait_for('forty two') and 'not it' not in [r.strip() for r in t.screen.text()],
      'an if written at the prompt waits for its else, then runs, with a capsule the session declared')

# ONLY A STATEMENT ON A LINE OF ITS OWN: inside a block typed at the prompt, list() is a name like any other,
# refused and told the word (the review of 2026-10-10: a capsule named list calling itself had its call rewritten).
t.at_prompt()
t.type(b'satellite.statement.if(1 == 1)\r'); in_the_block()
t.type(b'list()\r'); in_the_block()
t.type(b'}\r'); in_the_block()
t.type(b'\r')
check(t.wait_for(lambda t: b'no capsule named list' in t.raw and b'short spelling' in t.raw),
      'inside a block typed at the prompt, list() is not rewritten: refused, and told the word')

# A CAPSULE THE SESSION DECLARED UNDER A SHORT SPELLING'S NAME IS THE ONE CALLED (prompt_shorts.hpp).
t.at_prompt()
t.type(b'satellite.capsule list()\r'); in_the_block()
t.type(b'satellite.console.display("the session\'s own list")\r'); in_the_block()
t.type(b'}\r')
t.at_prompt()
t.type(b'list()\r')
check(t.wait_for("the session's own list"), 'list() calls a capsule the session declared named list, not the folder word')

# satellite.access AT THE PROMPT (2026-09-26, access_calls.hpp): a map written whole is kept,
# and a line of nothing but satellite.access(m) prints what m is and how to reach into it.
t.at_prompt()
t.type(b'satellite.container.map<satellite.variable.string, satellite.container.list<satellite.variable.number>> m = {"a": {1, 2}}\r')
t.at_prompt()
t.type(b'satellite.access(m)\r')
check(t.wait_for(lambda t: any(r.startswith('m is a map (string -> list of numbers), 1 key') for r in t.screen.text())) and
      t.wait_for(lambda t: any(r.startswith('  m["key"][n]') for r in t.screen.text())),
      'satellite.access(m) on a line of its own prints what m is and how to reach every level of it')

# CTRL-C STOPS A LOOP TYPED AT THE PROMPT (the author's testers, 2026-09-26, ERRORS3 7): it
# kept a core at 100% and the prompt never came back. Now the loop stops, the prompt returns,
# and the loop's name keeps what it reached.
# EACH LINE OF THE LOOP WAITS FOR THE "..." (open-work tools, 2026-10-25), as the if's lines above do:
# spin > 0 now and then came back false, and the output cannot say why. Typed straight after the header,
# `spin = spin + 1` could reach a session still busy with it, and D0.6.5 would drop it (never caught in
# the act here); or Ctrl-C, pressed after a fixed second, could land before the loop ran. AND Ctrl-C
# NOW WAITS FOR THE } TO BE HANDED OFF, and a moment more, as the sleep below does.
t.at_prompt()
t.type(b'satellite.variable.number spin = 0\r')
t.at_prompt()
t.type(b'satellite.statement.while (spin > -1)\r'); in_the_block()
t.type(b'spin = spin + 1\r'); in_the_block()
handed_off = t.raw.count(b'\x1b[?2004l')
t.type(b'}\r')
t.wait_for(lambda t: t.raw.count(b'\x1b[?2004l') > handed_off)
time.sleep(0.3)   # the loop has run, and the session's flag is cleared
t.type(b'\x03')
check(t.wait_for('Ctrl-C stopped satellite.statement.while'), 'Ctrl-C stops a while typed at the prompt')
t.at_prompt()
t.type(b'satellite.console.display(spin > 0)\r')
check(t.wait_for(lambda t: any(r.strip() == 'true' for r in t.screen.text()[-6:])),
      '... and the prompt comes back, runs the next line, and the loop\'s name kept what it reached')

# CTRL-C STOPS A SLEEP TYPED AT THE PROMPT (satellite.time.sleep, 2026-10-10): an hour asked for, and
# the prompt back a moment after the key -- a sleep looks up every tenth of a second
# (bytecode/time_calls.hpp), as a join() does. Pressed once the line has been handed off (ESC[?2004l,
# as for /usr above) AND a moment more: the session clears its flag just before it runs a line, and a
# key landing before that would be wiped, and the hour slept out (the review, 2026-10-10).
t.at_prompt()
handed_off = t.raw.count(b'\x1b[?2004l')
t.type(b'satellite.time.sleep(3600s)\r')
t.wait_for(lambda t: t.raw.count(b'\x1b[?2004l') > handed_off)
time.sleep(0.3)
pressed = time.time()
t.type(b'\x03')
check(t.wait_for('satellite.time.sleep(3600s) was stopped with Ctrl-C') and time.time() - pressed < 5,
      'Ctrl-C stops satellite.time.sleep(3600s) typed at the prompt')

t.at_prompt()
t.type(b'exit\r')
check(t.finish() == 0, 'exit leaves with 0')

modes = __import__('termios').tcgetattr(t.fd)[3]
check(modes & __import__('termios').ICANON and modes & __import__('termios').ECHO,
      'the terminal is cooked again after the session')

piped = subprocess.run([SATL, '--repl'], input=b'satellite.console.display("from a pipe")\n', capture_output=True)
check(piped.returncode == 0 and b'from a pipe\n' in piped.stdout and b'[satellite][' not in piped.stdout,
      'from a pipe: the line runs, with no prompt text and no banner')
piped = subprocess.run([SATL, '--repl'], input=b'nonsense\nsatellite.console.display("still here")\n', capture_output=True)
check(piped.returncode == 25 and b'still here\n' in piped.stdout,
      "a refused line does not stop the session, and its code is the session's status")

# THE DAY'S FOLDER (KEY_PLAN.md KEY-1), EACH CHECK IN A HOME OF ITS OWN. A session that is looked in after
# ends with Ctrl-D on an empty line, which answers no line (editor.cpp, session.cpp), so `exit` is never
# among what it typed. The day is taken before the run and after it, and either is accepted.
def console_of(home):
    return os.path.join(home, '.satl', 'console')

def days_in(home):
    console = console_of(home)
    return sorted(os.listdir(console)) if os.path.isdir(console) and not os.path.islink(console) else []

# A TERMINAL IN `home`, running `command` -- satl --repl unless another is given.
def session_in(home, command=None):
    global READER_ARGS
    session = READER_ARGS
    os.environ['HOME'], READER_ARGS = home, command or session
    try:
        return Terminal(width=100)
    finally:
        os.environ['HOME'], READER_ARGS = homes[0], session

def mode_of(path):
    return os.lstat(path).st_mode & 0o7777

# THE CONSOLES' OWN FILES (KEY_PLAN.md KEY-2): console-NNN.c4 in the day's folder, claimed at a console's
# first Enter, one more than the highest there. Each is named here 'day/console-NNN.c4'.
def files_in(home):
    console = console_of(home)
    return sorted(day + '/' + name for day in days_in(home) for name in os.listdir(os.path.join(console, day)))

def numbered(day, count):
    return ['%s/console-%03d.c4' % (day, number) for number in range(1, count + 1)]

# WHAT TWO CONSOLES OF ONE RUN LEAVE: console-001.c4 and console-002.c4 in the folder of the day taken
# before the run, or of the day taken after it -- or, had midnight fallen between their starts,
# console-001.c4 in each.
def two_consoles_left(files, before, after):
    return files in (numbered(before, 2), numbered(after, 2), sorted(numbered(before, 1) + numbered(after, 1)))

homes.append(fresh_home())
before = time.strftime('%Y-%m-%d')
t = session_in(homes[-1])
t.at_prompt()
t.type(b'satellite.console.display("one typed line")\r')
t.wait_for(lambda t: 'one typed line' in t.screen.text())
t.at_prompt()
t.type(b'\x04')
status = t.finish()
after = time.strftime('%Y-%m-%d')
console, days = console_of(homes[-1]), days_in(homes[-1])
check(status == 0 and len(days) == 1 and days[0] in (before, after) and mode_of(console) == 0o700 and
      os.path.isdir(os.path.join(console, days[0])) and mode_of(os.path.join(console, days[0])) == 0o700 and
      files_in(homes[-1]) == numbered(days[0], 1) and mode_of(os.path.join(console, numbered(days[0], 1)[0])) == 0o600,
      'one typed line makes ~/.satl/console/<today>/console-001.c4, both folders 0700 and the file 0600, '
      'and Ctrl-D leaves with 0')

# A FILE RUN'S input() ANSWERED AT A TERMINAL IS AN ENTER TOO: its getline tells the store, with no reader
# in between (console_calls.cpp).
asks = os.path.join(room, 'asks.satl')
with open(asks, 'w') as program:
    program.write('satellite.include(satellite)\n\n'
                  'satellite.capsule satellite.main(satellite.variable.arguments arguments)\n{\n'
                  '    satellite.variable.string name = satellite.console.input("name? ")\n'
                  '    satellite.console.display("got " + name)\n'
                  '    satellite.return(satellite)\n}\n')
homes.append(fresh_home())
before = time.strftime('%Y-%m-%d')
t = session_in(homes[-1], [SATL, asks])
asked = t.wait_for('name?')
t.type(b'Ada\r')
answered = t.wait_for('got Ada')
status = t.finish()
after = time.strftime('%Y-%m-%d')
days = days_in(homes[-1])
check(asked and answered and status == 0 and len(days) == 1 and days[0] in (before, after) and
      files_in(homes[-1]) == numbered(days[0], 1),
      "a file run's input() answered at a terminal makes ~/.satl/console/<today>/console-001.c4")

homes.append(fresh_home())
t = session_in(homes[-1])
t.at_prompt()
t.type(b'\x04')
check(t.finish() == 0 and not os.path.lexists(console_of(homes[-1])),
      'a session with nothing typed makes nothing')

# A LINE STOPPED WITH CTRL-C WAS NEVER ENTERED (rule 3), so it makes nothing either.
homes.append(fresh_home())
t = session_in(homes[-1])
t.at_prompt()
t.type(b'satellite.console.display("stopped")')
t.type(b'\x03')
t.at_prompt()
t.type(b'\x04')
check(t.finish() == 0 and not os.path.lexists(console_of(homes[-1])),
      'a line stopped with Ctrl-C makes nothing')

# A CONSOLE THAT CANNOT SAVE KEEPS WORKING (rule 8), as a real satl says it: ~/.satl/console a link to a
# folder beside it. S017 is on the screen once, by print_notice, both lines run, and nothing is written
# through the link.
homes.append(fresh_home())
elsewhere = os.path.join(homes[-1], 'elsewhere')
os.mkdir(elsewhere)
os.symlink(elsewhere, console_of(homes[-1]))
t = session_in(homes[-1])
t.at_prompt()
t.type(b'satellite.console.display("first line")\r')
first = t.wait_for(lambda t: 'first line' in t.screen.text())
t.at_prompt()
t.type(b'satellite.console.display("second line")\r')
second = t.wait_for(lambda t: 'second line' in t.screen.text())
t.at_prompt()
t.type(b'\x04')
check(t.finish() == 0 and first and second and t.raw.count(b'S017 CONSOLE_NOT_SAVED') == 1 and
      os.path.islink(console_of(homes[-1])) and os.listdir(elsewhere) == [],
      'a console folder that is a link: S017 once on the screen, both lines run, nothing written through it')

# PIPED IN, AND SHOWN ON A TERMINAL: stdout and stderr on a pty, stdin a pipe. SATL_NO_WINDOW keeps the pipe
# satl's stdin.
def piped_to_a_terminal(command, lines, env):
    master, slave = pty.openpty()
    run = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=slave, stderr=slave, env=env)
    os.close(slave)
    run.stdin.write(lines)
    run.stdin.close()
    seen = b''
    while select.select([master], [], [], 10)[0]:
        try:
            chunk = os.read(master, 65536)
        except OSError:
            break
        if not chunk:
            break
        seen += chunk
    os.close(master)
    return run.wait(timeout=10), seen

# PIPED, ITS OUTPUT TO A PIPE AND THEN TO A TERMINAL: lines from a pipe are nobody's typing (rule 1), and a
# terminal on stdout alone is not a console.
homes.append(fresh_home())
alone = dict(os.environ, HOME=homes[-1], SATL_NO_WINDOW='1')
piped = subprocess.run([SATL, '--repl'], input=b'satellite.console.display("from a pipe")\n', capture_output=True,
                       env=alone)
status, seen = piped_to_a_terminal([SATL, '--repl'], b'satellite.console.display("piped, shown on a terminal")\n',
                                   alone)
check(piped.returncode == 0 and b'from a pipe\n' in piped.stdout and status == 0 and
      b'piped, shown on a terminal' in seen and not os.path.lexists(console_of(homes[-1])),
      'a piped --repl makes nothing, its output on a pipe or on a terminal')

# THE SAME FOR A FILE RUN'S input(), and only this run shows that satl makes no store for a terminal on
# stdout alone (structured-library.cpp): the reader tells the store of no piped line, but input()'s getline
# tells it of every line whenever there is a store.
homes.append(fresh_home())
status, seen = piped_to_a_terminal([SATL, asks], b'Ada\n', dict(os.environ, HOME=homes[-1], SATL_NO_WINDOW='1'))
check(status == 0 and b'got Ada' in seen and not os.path.lexists(console_of(homes[-1])),
      "a piped file run's input() line makes nothing, its output on a terminal")

# AND A TERMINAL ON STDIN ALONE: `satl asks.satl > a file`, answered at the terminal. A console is stdin
# and stdout both terminals (rule 1), so there is no store, and the line, typed as it was, makes nothing.
homes.append(fresh_home())
master, slave = pty.openpty()
with open(os.path.join(room, 'written'), 'w+b') as written:
    run = subprocess.Popen([SATL, asks], stdin=slave, stdout=written, stderr=written,
                           env=dict(os.environ, HOME=homes[-1], SATL_NO_WINDOW='1'))
    os.close(slave)
    os.write(master, b'Ada\n')
    status = run.wait(timeout=10)
    written.seek(0)
    seen = written.read()
os.close(master)
check(status == 0 and b'got Ada' in seen and not os.path.lexists(console_of(homes[-1])),
      "a file run's input() typed at a terminal makes nothing, its output to a file")

# ONE FILE A CONSOLE (KEY_PLAN.md KEY-2), EACH CHECK IN A HOME OF ITS OWN, its sessions ended with Ctrl-D on
# an empty line. Which satl holds which file open, and on which descriptor, is read from /proc: the file
# stays open for the run, never on 0, 1 or 2, and close-on-exec, so no satl started from another holds it.
def held_by(pid, home):
    inside = os.path.realpath(console_of(home)) + os.sep
    held = []
    for fd in os.listdir('/proc/%d/fd' % pid):
        try:
            target = os.readlink('/proc/%d/fd/%s' % (pid, fd))
        except OSError:
            continue
        if target.startswith(inside):
            held.append((int(fd), target[len(inside):]))
    return sorted(held, key=lambda one: one[1])

# THE satls A satl STARTED, from whichever of its threads started them.
def children_of(pid):
    children = []
    for task in os.listdir('/proc/%d/task' % pid):
        try:
            with open('/proc/%d/task/%s/children' % (pid, task)) as listing:
                children += [int(child) for child in listing.read().split()]
        except OSError:
            pass
    return children

def held_above_two(held):
    return len(held) == 1 and held[0][0] > 2

homes.append(fresh_home())
before = time.strftime('%Y-%m-%d')
statuses = []
for line in ('the first session', 'the second session'):
    t = session_in(homes[-1])
    t.at_prompt()
    t.type(('satellite.console.display("%s")\r' % line).encode())
    t.wait_for(lambda t: line in t.screen.text())
    t.at_prompt()
    t.type(b'\x04')
    statuses.append(t.finish())
after = time.strftime('%Y-%m-%d')
files = files_in(homes[-1])
check(statuses == [0, 0] and two_consoles_left(files, before, after) and
      all(mode_of(os.path.join(console_of(homes[-1]), name)) == 0o600 for name in files),
      'two sessions in one day make console-001.c4 and console-002.c4, each 0600')

# TWO AT THE SAME TIME: both at their prompts, each line typed, and the two Enters sent together, so the
# two satls read the day's folder and claim in it at once.
homes.append(fresh_home())
before = time.strftime('%Y-%m-%d')
one, two = session_in(homes[-1]), session_in(homes[-1])
ready = one.at_prompt() and two.at_prompt()
one.type(b'satellite.console.display("the first of two")')
two.type(b'satellite.console.display("the second of two")')
os.write(one.fd, b'\r')
os.write(two.fd, b'\r')
ran = (one.wait_for(lambda t: 'the first of two' in t.screen.text()) and
       two.wait_for(lambda t: 'the second of two' in t.screen.text()))
one.at_prompt()
two.at_prompt()
held = [held_by(one.pid, homes[-1]), held_by(two.pid, homes[-1])]
one.type(b'\x04')
two.type(b'\x04')
statuses = [one.finish(), two.finish()]
after = time.strftime('%Y-%m-%d')
files = files_in(homes[-1])
check(ready and ran and statuses == [0, 0] and two_consoles_left(files, before, after) and
      all(held_above_two(each) for each in held) and sorted(each[0][1] for each in held) == files,
      'two sessions at the same time make two files, each held open by its own satl')

# THE PROMPT'S LINES AND input()'s ANSWER AT THE PROMPT are one console's, so one satl claims one file.
homes.append(fresh_home())
before = time.strftime('%Y-%m-%d')
t = session_in(homes[-1])
t.at_prompt()
t.type(b'satellite.console.display("a line before the question")\r')
t.wait_for(lambda t: 'a line before the question' in t.screen.text())
t.at_prompt()
t.type(b'satellite.variable.string answer = satellite.console.input("answer? ")\r')
# input()'S OWN PROMPT, ALONE ON THE CURSOR'S ROW, as at_prompt() waits for the session's: the typed line
# already shows "answer? ", and an answer typed before input() reads is dropped (D0.6.5).
asked = t.wait_for(lambda t: t.screen.text()[t.screen.row] == 'answer?')
t.type(b'forty two\r')
t.at_prompt()
t.type(b'satellite.console.display("answered " + answer)\r')
answered = t.wait_for(lambda t: 'answered forty two' in t.screen.text())
t.at_prompt()
held = held_by(t.pid, homes[-1])
t.type(b'\x04')
status = t.finish()
after = time.strftime('%Y-%m-%d')
files = files_in(homes[-1])
check(asked and answered and status == 0 and files in (numbered(before, 1), numbered(after, 1)) and
      held_above_two(held) and [held[0][1]] == files,
      "the prompt's lines and an input() answer at the prompt claim one file, held open above 0, 1 and 2")

# A satl THAT `run` STARTS IS A CONSOLE OF ITS OWN (prompt_run.cpp): the prompt's file is claimed at the run
# line's Enter, and the program's at its first answer -- each held by its own satl, and neither by the other.
asks_twice = os.path.join(room, 'asks_twice.satl')
with open(asks_twice, 'w') as program:
    program.write('satellite.include(satellite)\n\n'
                  'satellite.capsule satellite.main(satellite.variable.arguments arguments)\n{\n'
                  '    satellite.variable.string name = satellite.console.input("name? ")\n'
                  '    satellite.variable.string again = satellite.console.input("again? ")\n'
                  '    satellite.console.display("got " + name + " and " + again)\n'
                  '    satellite.return(satellite)\n}\n')
homes.append(fresh_home())
before = time.strftime('%Y-%m-%d')
t = session_in(homes[-1])
t.at_prompt()
t.type(('run %s\r' % asks_twice).encode())
asked = t.wait_for('name?')
at_the_question = held_by(t.pid, homes[-1])
before_its_answer = [held_by(child, homes[-1]) for child in children_of(t.pid)]
t.type(b'Ada\r')
again = t.wait_for('again?')
started = children_of(t.pid)
programs = held_by(started[0], homes[-1]) if len(started) == 1 else []
prompts = held_by(t.pid, homes[-1])
t.type(b'Bea\r')
got = t.wait_for('got Ada and Bea')
t.at_prompt()
t.type(b'\x04')
status = t.finish()
after = time.strftime('%Y-%m-%d')
files = files_in(homes[-1])
check(asked and again and got and status == 0 and two_consoles_left(files, before, after) and
      held_above_two(at_the_question) and at_the_question[0][1].endswith('/console-001.c4') and
      before_its_answer == [[]] and prompts == at_the_question and held_above_two(programs) and
      sorted([prompts[0][1], programs[0][1]]) == files,
      'a satl that run starts is a console of its own: a file of its own at its first answer, '
      "and neither satl holds the other's")

# AND ONE THAT `interpret` STARTS, the same way.
homes.append(fresh_home())
before = time.strftime('%Y-%m-%d')
t = session_in(homes[-1])
t.at_prompt()
t.type(('interpret %s\r' % asks).encode())
asked = t.wait_for('name?')
t.type(b'Ada\r')
got = t.wait_for('got Ada')
t.at_prompt()
t.type(b'\x04')
status = t.finish()
after = time.strftime('%Y-%m-%d')
check(asked and got and status == 0 and two_consoles_left(files_in(homes[-1]), before, after),
      "... and so is a satl that interpret starts: two files, the prompt's and the program's")

# EVERY ENTER KEPT (KEY_PLAN.md KEY-3): a line of the console's file, as Enter sent it, less the control
# characters and any byte that is not UTF-8. Each check in a HOME of its own, its session ended with Ctrl-D on
# an empty line, so no `exit` is among its lines; what is checked is the file's bytes, whole.
def file_of(home):
    files = files_in(home)
    if len(files) != 1:
        return None
    with open(os.path.join(console_of(home), files[0]), 'rb') as kept:
        return kept.read()

def shows_row(text):
    return lambda t: text in [r.strip() for r in t.screen.text()]

# TYPED, ONE LINE MENDED WITH BACKSPACE (rule 3: pritn, two Backspaces and nt keeps print) AND ONE HOLDING AN
# ACCENT AND AN EMOJI, kept byte for byte.
homes.append(fresh_home())
t = session_in(homes[-1])
t.at_prompt()
t.type(b'satellite.console.display("pritn')
t.type(b'\x7f\x7f')
t.type(b'nt")\r')
mended = t.wait_for(shows_row('print'))
t.at_prompt()
t.type('satellite.console.display("caf\u00e9 \U0001F600")\r'.encode())
accented = t.wait_for(shows_row('caf\u00e9 \U0001F600'))
t.at_prompt()
t.type(b'satellite.variable.number three = 3\r')
t.at_prompt()
t.type(b'\x04')
status = t.finish()
kept = file_of(homes[-1])
check(status == 0 and mended and accented and
      kept == ('satellite.console.display("print")\n'
               'satellite.console.display("caf\u00e9 \U0001F600")\n'
               'satellite.variable.number three = 3\n').encode(),
      'three typed lines, one mended with Backspace and one holding an accent and an emoji, are the three lines '
      'of console-001.c4 as they were entered (%r)' % kept)

# A PASTED BLOCK OF THREE LINES is three lines, each kept once, when it is answered -- and the control
# character pasted inside one is dropped from it.
homes.append(fresh_home())
t = session_in(homes[-1])
t.at_prompt()
t.type(b'\x1b[200~satellite.console.display("pasted one")\rsatellite.console.display("pasted\x01two")\r'
       b'satellite.console.display("pasted three")\r\x1b[201~')
pasted = t.wait_for(shows_row('pasted three'))
t.at_prompt()
t.type(b'\x04')
status = t.finish()
kept = file_of(homes[-1])
check(status == 0 and pasted and
      kept == b'satellite.console.display("pasted one")\nsatellite.console.display("pastedtwo")\n'
              b'satellite.console.display("pasted three")\n',
      'a pasted block of three lines is three lines of the file, the control character pasted into one dropped (%r)' % kept)

# A LINE STOPPED WITH CTRL-C WAS NEVER ENTERED (rule 3), and AN EMPTY ENTER IS AN EMPTY LINE.
homes.append(fresh_home())
t = session_in(homes[-1])
t.at_prompt()
t.type(b'satellite.console.display("before the stop")\r')
t.wait_for(shows_row('before the stop'))
t.at_prompt()
t.type(b'\r')
t.at_prompt()
t.type(b'satellite.console.display("stopped')
t.type(b'\x03')
t.at_prompt()
t.type(b'satellite.console.display("after the stop")\r')
after_it = t.wait_for(shows_row('after the stop'))
t.at_prompt()
t.type(b'\x04')
status = t.finish()
kept = file_of(homes[-1])
check(status == 0 and after_it and
      kept == b'satellite.console.display("before the stop")\n\nsatellite.console.display("after the stop")\n',
      'a line stopped with Ctrl-C is not in the file, and an empty Enter is an empty line of it (%r)' % kept)

# AN input() ANSWER AT THE PROMPT, read through the session's own reader (read_for_the_program), after the
# line that asked.
homes.append(fresh_home())
t = session_in(homes[-1])
t.at_prompt()
t.type(b'satellite.variable.string answer = satellite.console.input("answer? ")\r')
asked = t.wait_for(lambda t: t.screen.text()[t.screen.row] == 'answer?')
t.type(b'forty two\r')
t.at_prompt()
t.type(b'\x04')
status = t.finish()
kept = file_of(homes[-1])
check(asked and status == 0 and
      kept == b'satellite.variable.string answer = satellite.console.input("answer? ")\nforty two\n',
      "an input() answer at the prompt is a line of the console's file, after the line that asked (%r)" % kept)

# WHAT THE PROMPT WRITES FOR A PERSON IS NOT ENTERED (write_for_them, session.cpp): the { after a header typed by
# hand never reaches the reader, so it is no line of the file. The indentation the prompt starts a block's line
# with is part of what Enter sends, and a } typed on it steps back first (editor.cpp).
homes.append(fresh_home())
t = session_in(homes[-1])
t.at_prompt()
t.type(b'satellite.statement.if (1 == 1)\r'); in_the_block()
written = t.screen.text()[t.screen.row - 1] == '... {'
t.type(b'satellite.console.display("inside the block")\r'); in_the_block()
t.type(b'}\r'); in_the_block()
t.type(b'\x04')   # the if waits for an else, and the end of the input runs it
ran = t.wait_for(shows_row('inside the block'))
status = t.finish()
kept = file_of(homes[-1])
check(written and ran and status == 0 and
      kept == b'satellite.statement.if (1 == 1)\n    satellite.console.display("inside the block")\n}\n',
      'the { the prompt writes for a person is no line of the file: only the three lines typed and Entered (%r)' % kept)

# A satl KILLED WITH SIGKILL, which runs nothing on its way out, after two Enters -- the second still running --
# has both lines in its file: each is written before it runs, and nothing waits in a buffer.
homes.append(fresh_home())
t = session_in(homes[-1])
t.at_prompt()
t.type(b'satellite.console.display("before the kill")\r')
t.wait_for(shows_row('before the kill'))
t.at_prompt()
handed_off = t.raw.count(b'\x1b[?2004l')
t.type(b'satellite.time.sleep(3600s)\r')
running = t.wait_for(lambda t: t.raw.count(b'\x1b[?2004l') > handed_off)
time.sleep(0.3)
os.kill(t.pid, signal.SIGKILL)
status = t.finish()
kept = file_of(homes[-1])
check(running and status == -signal.SIGKILL and
      kept == b'satellite.console.display("before the kill")\nsatellite.time.sleep(3600s)\n',
      'a satl killed with SIGKILL after two Enters, the second still running, has both lines in its file (%r)' % kept)

# A FILE RUN ON A PTY, its program's two input() calls answered: getline's lines (console_calls.cpp), kept by the
# store itself, in a file of the run's own.
homes.append(fresh_home())
t = session_in(homes[-1], [SATL, asks_twice])
asked = t.wait_for('name?')
t.type(b'Ada\r')
again = t.wait_for('again?')
t.type(b'Bea\r')
got = t.wait_for('got Ada and Bea')
status = t.finish()
kept = file_of(homes[-1])
check(asked and again and got and status == 0 and kept == b'Ada\nBea\n',
      "a satl file.satl on a pty keeps both answers to its program's two input() calls in its own file (%r)" % kept)

# THE WALK OF A FULL STORE (KEY_PLAN.md KEY-5): 16,425 console files of 2 KB, 45 days of 365 -- Section 3's
# measure of a 32 MB store -- are listed before the first prompt is drawn, and the prompt answers a line. Each
# file is 2 KB of holes, so the room takes their names and not their 32 MB; the walk reads only their sizes.
homes.append(fresh_home())
os.mkdir(console_of(homes[-1]), 0o700)
for day in range(45):
    folder = os.path.join(console_of(homes[-1]), str(datetime.date(2025, 1, 1) + datetime.timedelta(days=day)))
    os.mkdir(folder, 0o700)
    for number in range(1, 366):
        with open(os.path.join(folder, 'console-%03d.c4' % number), 'wb') as made:
            made.truncate(2048)
started = time.time()
t = session_in(homes[-1])
drawn = t.at_prompt()
first_prompt = time.time() - started
t.type(b'satellite.console.display("after the walk")\r')
answered = t.wait_for(shows_row('after the walk'))
t.at_prompt()
t.type(b'\x04')
status = t.finish()
check(drawn and answered and status == 0 and len(files_in(homes[-1])) == 16426,
      'with a store of 16,425 console files in its HOME, the first prompt is drawn (%.2f s) and answers a line, '
      'its own the 16,426th file' % first_prompt)
shutil.rmtree(homes[-1], ignore_errors=True)

# LEFT AND RIGHT FROM CONSOLE TO CONSOLE (KEY_PLAN.md KEY-6), FOUR SESSIONS ONE AFTER ANOTHER IN ONE HOME. The
# first three each type a line of their own and end with Ctrl-D. In the fourth, Left on the empty line shows the
# third's line, and Left past its start the second's; Enter on that line runs it and puts it into the fourth's
# file. The fourth's file is the newest name: a later day, or a higher number in the same one.
homes.append(fresh_home())
statuses = []
for word in ('one', 'two', 'three'):
    t = session_in(homes[-1])
    t.at_prompt()
    t.type(('satellite.console.display("%s")\r' % word).encode())
    t.wait_for(shows_row(word))
    t.at_prompt()
    t.type(b'\x04')
    statuses.append(t.finish())
third, second = 'satellite.console.display("three")', 'satellite.console.display("two")'
t = session_in(homes[-1])
t.at_prompt()
row = t.screen.row
t.type(b'\x1b[D')
shows_third = t.wait_for(lambda t: t.screen.text()[row].endswith(']>> ' + third))
t.type(b'\x1b[D' * (len(third) + 1))
shows_second = t.wait_for(lambda t: t.screen.text()[row].endswith(']>> ' + second))
t.type(b'\r')
ran = t.wait_for(shows_row('two'))
t.at_prompt()
t.type(b'\x04')
statuses.append(t.finish())
files = files_in(homes[-1])
fourth = open(os.path.join(console_of(homes[-1]), files[-1]), 'rb').read() if len(files) == 4 else None
check(statuses == [0, 0, 0, 0] and shows_third and shows_second,
      "in the fourth of four sessions, Left on the empty line shows the third's line, and Left past its start the "
      "second's (statuses %r)" % statuses)
check(ran and fourth == (second + '\n').encode(),
      "... and Enter on that line runs it and puts it into the fourth's file (%r)" % fourth)

# A DELETE THE SYSTEM WILL NOT ALLOW ASKS FOR A PASSWORD ON THE TERMINAL (FILE_AND_DRIVE_PLAN.md FD-2): a file in a
# folder of the person's own that they made read-only asks for their sudo password, draws nothing of what is typed,
# keeps nothing of it in the console's file, and Ctrl-C at the question leaves the file. The question is never
# answered, so no su or sudo is ever started. As root every folder is writable, so nothing is asked.
if os.geteuid() != 0:
    homes.append(fresh_home())
    locked = tempfile.mkdtemp(prefix='session_delete_')
    open(os.path.join(locked, 'kept'), 'w').close()
    os.chmod(locked, 0o555)
    t = session_in(homes[-1])
    t.at_prompt()
    typed_line = 'satellite.file.delete("%s/kept")' % locked
    t.type(typed_line.encode() + b'\r')
    asked = t.wait_for(lambda t: ('[sudo] password for %s: ' % USER).encode() in t.raw)
    t.type(b's3cret')
    t.pump(0.3)
    drawn = b's3cret' in t.raw
    t.type(b'\x03')
    said = t.wait_for(lambda t: b'no password was typed' in t.raw)
    t.at_prompt()
    t.type(b'\x04')
    status = t.finish()
    kept = file_of(homes[-1])
    log = os.path.join(homes[-1], '.satl', 'satellite.log')
    logged = open(log, 'rb').read() if os.path.exists(log) else b''
    check(asked and not drawn and said and os.path.exists(os.path.join(locked, 'kept')) and
          kept == (typed_line + '\n').encode() and b's3cret' not in logged,
          'delete in a read-only folder of their own asks for the sudo password, draws nothing typed, keeps nothing '
          'of it in the console\'s file or satellite.log, and Ctrl-C leaves the file (%r)' % kept)

    # IN A FILE RUN Ctrl-C ENDS satl, as it always has -- and at the question, the terminal gets its ECHO back first.
    asks_to_delete = os.path.join(room, 'asks_to_delete.satl')
    with open(asks_to_delete, 'w') as program:
        program.write('satellite.include(satellite)\nsatellite.capsule satellite.main()\n{\n'
                      '    satellite.file.delete("%s/kept")\n    satellite.return(satellite)\n}\n' % locked)
    t = session_in(homes[-1], [SATL, asks_to_delete])
    asked = t.wait_for(lambda t: ('[sudo] password for %s: ' % USER).encode() in t.raw)
    t.type(b'\x03')
    status = t.finish()
    echo_back = bool(termios.tcgetattr(t.fd)[3] & termios.ECHO)
    os.chmod(locked, 0o755)
    check(asked and status == -signal.SIGINT and echo_back and os.path.exists(os.path.join(locked, 'kept')),
          'in a file run Ctrl-C at the password question ends satl by SIGINT, with ECHO put back (status %r)' % status)
    shutil.rmtree(locked, ignore_errors=True)

# A PASTE ONTO A NAME ALREADY THERE ASKS FIRST, AND ONLY y REPLACES IT (FILE_AND_DRIVE_PLAN.md FD-3, rule 6): n keeps
# the file and says so, y puts the copy in its place, and neither answer is a line of the console's file. The
# clipboard is $TMPDIR/.satl/tmp, so this session's TMPDIR is a folder of its own -- never the person's /tmp/.satl.
homes.append(fresh_home())
clip = tempfile.mkdtemp(prefix='session_clip_')
source = tempfile.mkdtemp(prefix='session_source_')
pasting = tempfile.mkdtemp(prefix='session_paste_')
with open(os.path.join(source, 'note.txt'), 'w') as note:
    note.write('new\n')
with open(os.path.join(pasting, 'note.txt'), 'w') as note:
    note.write('old\n')
def note_says():
    with open(os.path.join(pasting, 'note.txt')) as note:
        return note.read()
tmpdir_before = os.environ.get('TMPDIR')
os.environ['TMPDIR'] = clip
t = session_in(homes[-1])
if tmpdir_before is None:
    del os.environ['TMPDIR']
else:
    os.environ['TMPDIR'] = tmpdir_before
typed_lines = ['satellite.file.copy("%s/note.txt")' % source, 'satellite.file.paste("%s")' % pasting,
               'satellite.file.paste("%s")' % pasting]
t.at_prompt()
t.type(typed_lines[0].encode() + b'\r')
t.at_prompt()
t.type(typed_lines[1].encode() + b'\r')
asked_once = t.wait_for(lambda t: b'-- replace it? [y/N]' in t.raw)
t.type(b'n\r')
kept_said = t.wait_for(lambda t: b'is already there, and it was kept' in t.raw)
after_no = note_says()
t.at_prompt()
t.type(typed_lines[2].encode() + b'\r')
asked_again = t.wait_for(lambda t: t.raw.count(b'-- replace it? [y/N]') == 2)
t.type(b'y\r')
t.pump(0.5)
t.at_prompt()
after_yes = note_says()
t.type(b'\x04')
status = t.finish()
kept = file_of(homes[-1])
check(status == 0 and asked_once and kept_said and after_no == 'old\n' and asked_again and after_yes == 'new\n' and
      kept == ('\n'.join(typed_lines) + '\n').encode(),
      'a paste onto a name already there asks; n keeps the file and says so, y replaces it, and the console\'s file '
      'holds the three typed lines and neither answer (%r)' % kept)
for folder in (clip, source, pasting):
    shutil.rmtree(folder, ignore_errors=True)

# satellite.console.bash ON THE CONSOLE ITSELF (the author, 2026-10-23: "so I don't have to use a regular console
# anymore"): what a command asks is answered by typing at satl's terminal -- its input is the terminal, not a pipe --
# and Ctrl-C stops a command and gives the prompt back, with satl there to run the next line.
homes.append(fresh_home())
t = session_in(homes[-1])
t.at_prompt()
t.type(b'satellite.console.bash("read -p \'name? \' n; echo hi $n")\r')
asked = t.wait_for(lambda t: t.raw.count(b'name? ') >= 2)   # the line's own echo is the first
t.type(b'bob\r')
answered = t.wait_for(lambda t: b'hi bob' in t.raw)
t.at_prompt()
t.type(b'satellite.console.bash("sleep 30; echo never")\r')
t.pump(1.0)
t.type(b'\x03')
t.at_prompt()
t.type(b'satellite.console.display("after the stop")\r')
after = t.wait_for(shows_row('after the stop'))
# A JOB OF ITS OWN (the review, 2026-10-23): two Ctrl-C at a command that catches the first are the command's, and
# satl -- which had ended at the second -- is still there; an inner shell killed with kill -9 leaves the terminal
# satl's again; and a last line with no newline is not drawn over by the prompt after it.
t.at_prompt()
t.type(b'satellite.console.bash("trap \'echo caught\' INT; sleep 2; sleep 2")\r')
t.pump(0.7)
t.type(b'\x03')
t.pump(0.5)
t.type(b'\x03')
t.at_prompt()
t.type(b'satellite.console.display("after two")\r')
after_two = t.wait_for(shows_row('after two'))
t.at_prompt()
t.type(b'satellite.console.bash("bash --norc --noprofile -i")\r')
t.pump(1.0)
t.type(b'kill -9 $$\r')
t.at_prompt()
t.type(b'satellite.console.display("after the kill")\r')
after_kill = t.wait_for(shows_row('after the kill'))
t.at_prompt()
t.type(b'satellite.console.bash("printf no-newline-here")\r')
kept = t.wait_for(shows_row('no-newline-here'))
t.at_prompt()
kept = kept and shows_row('no-newline-here')(t)
t.type(b'\x04')
status = t.finish()
check(status == 0 and asked and answered and after and b'never' not in t.raw.replace(b'echo never', b''),
      'console.bash: a question the command asks is answered at the terminal, and Ctrl-C stops it and gives the '
      'prompt back (status %r)' % status)
check(status == 0 and after_two and after_kill and kept,
      'console.bash runs as a job of its own: two Ctrl-C at a command that catches one leave satl running, an inner '
      'shell killed with kill -9 gives the terminal back, and a last line with no newline stays on the screen')

for home in homes:
    shutil.rmtree(home, ignore_errors=True)
shutil.rmtree(room, ignore_errors=True)
print('every session check passed' if failed == 0 else 'a session check FAILED', flush=True)
sys.exit(1 if failed else 0)
