# satellite -- how many recipes run at once.
#
# TWENTY-FOUR, set at the author's instruction on 2026-09-13. It is this
# machine's thread count (`nproc`), written as a number rather than asked of
# nproc because twenty-four is what was asked for. A bare `make` used to run
# one recipe at a time.
#
# A -j ON THE COMMAND LINE WINS. `make -j4` builds with four, `make -j1` with
# one and a bare `make -j` without a limit; this line speaks only when nobody
# else did. The filter is the whole test, because GNU make 4.4 has already
# normalised the command line into MAKEFLAGS when this file is read -- MEASURED
# on 4.4.1: -j4, --jobs=4 and -sj4 all arrive as a word beginning -j, and
# `make X=1` with no -j becomes " -j24 -- X=1", the variable still a variable.
#
# -j4 IS THE FALLBACK FOR A BUSY MACHINE. On 2026-09-12 a -j8 died with
# `posix_spawn failed: Cannot allocate memory` under a load average near 117
# from something else, and -j4 built and tested cleanly for the rest of that
# day. `uptime` before a build says which case this is.
#
# A SUB-MAKE SHARES THESE TWENTY-FOUR rather than starting twenty-four more: it
# inherits the jobserver through MAKEFLAGS. That is what keeps the build inside
# 080-install.mk's installer on the same budget as the build that started it.
ifeq ($(filter -j%,$(MAKEFLAGS)),)
MAKEFLAGS += -j24
endif
