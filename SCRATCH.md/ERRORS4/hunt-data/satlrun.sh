#!/bin/bash
# Safe runner for satellite probes. Runs a private copy of BUILD 0099 (so a rebuild of the tree
# cannot change it mid-hunt), with a private HOME, no window or display, stdin from /dev/null,
# a timeout, and a 16 GB REAL-memory limit (a cgroup; a virtual-memory cap made satl's
# stack segments fail and looked like crashes).
# usage: satlrun.sh file.satl [args...]
#   SATL_PROBE_HOME=<dir>  the HOME to use (default: the shared scratch home)
#   SATL_TIMEOUT=<secs>    wall-clock limit (default 20)
#   SATL_STDIN=<file>      what the program reads as input (default /dev/null)
# Prints "[exit N]" on stderr last. 124 = timed out; 134/139 = abort/segfault.
H=${SATL_PROBE_HOME:-/tmp/claude-1000/-home-madness-code-satl/b70f905b-dca1-4f92-bbad-f66d8ce80b78/scratchpad/home}
mkdir -p "$H/.satl"
[ -f "$H/.satl/config.ini" ] || cp /tmp/claude-1000/-home-madness-code-satl/b70f905b-dca1-4f92-bbad-f66d8ce80b78/scratchpad/home/.satl/config.ini "$H/.satl/config.ini" 2>/dev/null
X=$(mktemp -d); chmod 700 "$X"
ulimit -f 4194304   # no file bigger than 4 GB: a program writing past it is stopped (exit 153)
systemd-run --user --scope -q -p MemoryMax=16G -p MemorySwapMax=0 \
  env -u DISPLAY -u WAYLAND_DISPLAY -u XAUTHORITY -u GDK_BACKEND -u DBUS_SESSION_BUS_ADDRESS \
    HOME="$H" SATL_NO_WINDOW=1 XDG_RUNTIME_DIR="$X" \
    timeout -k 2 ${SATL_TIMEOUT:-20} /tmp/claude-1000/-home-madness-code-satl/b70f905b-dca1-4f92-bbad-f66d8ce80b78/scratchpad/satl-0099/satl "$@" < "${SATL_STDIN:-/dev/null}"
code=$?
rm -rf "$X"
echo "[exit $code]" >&2
exit $code
