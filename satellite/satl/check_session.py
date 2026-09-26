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

import os, shutil, subprocess, sys, tempfile, time

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

room = tempfile.mkdtemp(prefix='session_check_')
open(os.path.join(room, 'alpha'), 'w').close()
open(os.path.join(room, '.hidden'), 'w').close()
os.mkdir(os.path.join(room, 'nested'))

t = Terminal(width=100)
check(t.wait_for('A statement a line'), 'the session says how to leave it')
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
t.type(b'satellite.statement.if(big == 42)\r')
t.type(b'satellite.console.display("forty two")\r')
t.type(b'}\r')
t.type(b'satellite.statement.else\r')
t.type(b'satellite.console.display("not it")\r')
t.type(b'}\r')
check(t.wait_for('forty two') and 'not it' not in [r.strip() for r in t.screen.text()],
      'an if written at the prompt waits for its else, then runs, with a capsule the session declared')

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
t.at_prompt()
t.type(b'satellite.variable.number spin = 0\r')
t.at_prompt()
t.type(b'satellite.statement.while (spin > -1)\r')
t.type(b'spin = spin + 1\r')
t.type(b'}\r')
time.sleep(1.0)
t.type(b'\x03')
check(t.wait_for('Ctrl-C stopped satellite.statement.while'), 'Ctrl-C stops a while typed at the prompt')
t.at_prompt()
t.type(b'satellite.console.display(spin > 0)\r')
check(t.wait_for(lambda t: any(r.strip() == 'true' for r in t.screen.text()[-6:])),
      '... and the prompt comes back, runs the next line, and the loop\'s name kept what it reached')

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

shutil.rmtree(room, ignore_errors=True)
print('every session check passed' if failed == 0 else 'a session check FAILED', flush=True)
sys.exit(1 if failed else 0)
