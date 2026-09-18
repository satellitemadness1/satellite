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

room = tempfile.mkdtemp(prefix='session_check_')
open(os.path.join(room, 'alpha'), 'w').close()
open(os.path.join(room, '.hidden'), 'w').close()
os.mkdir(os.path.join(room, 'nested'))

t = Terminal(width=100)
check(t.wait_for('One statement a line'), 'the session says how to leave it')
check(t.wait_for('satl>'), 'the prompt is drawn')

t.type(b'satellite.console.display("typed and ran")\r')
# The needle must be the OUTPUT row and not the echo of the line that typed it,
# which holds the same words.
check(t.wait_for(lambda t: 'typed and ran' in t.screen.text()) and
      screen_order(t, 'satl> satellite.console.display("typed and ran")', 'typed and ran'),
      'a typed line runs, below the line that typed it')

t.at_prompt()
t.type(b'\x1b[A\r')
check(t.wait_for(lambda t: t.raw.count(b'typed and ran') >= 3), 'Up recalls the last line and runs it again')

t.at_prompt()
t.type(('satellite.directory.change("%s")\r' % room).encode())
t.at_prompt()
t.type(b'satellite.directory.list()\r')
check(t.wait_for(lambda t: t.screen.shows('name') and t.screen.shows('permissions')), 'list() draws the table')
check(t.wait_for(lambda t: t.screen.shows('alpha') and t.screen.shows('.hidden') and t.screen.shows('nested')),
      '... of the directory change() moved to, dotfiles kept')
check(not t.screen.shows(' .  ') and not t.screen.shows('..'), '... without . and ..')

t.at_prompt()
t.type(b'satellite.directory.change("/")\r')
t.at_prompt()
t.type(b'satellite.directory.list("%s")\r' % room.encode())
check(t.wait_for(lambda t: t.raw.count(b'alpha') >= 2), 'list(d) lists somewhere else, after change moved away')

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

t.at_prompt()
t.type(b'exit\r')
check(t.finish() == 0, 'exit leaves with 0')

modes = __import__('termios').tcgetattr(t.fd)[3]
check(modes & __import__('termios').ICANON and modes & __import__('termios').ECHO,
      'the terminal is cooked again after the session')

piped = subprocess.run([SATL, '--repl'], input=b'satellite.console.display("from a pipe")\n', capture_output=True)
check(piped.returncode == 0 and b'from a pipe\n' in piped.stdout and b'satl>' not in piped.stdout,
      'from a pipe: the line runs, with no prompt text and no banner')
piped = subprocess.run([SATL, '--repl'], input=b'nonsense\nsatellite.console.display("still here")\n', capture_output=True)
check(piped.returncode == 25 and b'still here\n' in piped.stdout,
      "a refused line does not stop the session, and its code is the session's status")

shutil.rmtree(room, ignore_errors=True)
print('every session check passed' if failed == 0 else 'a session check FAILED', flush=True)
sys.exit(1 if failed else 0)
