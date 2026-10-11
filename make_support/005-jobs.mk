# satellite 004 -- how many recipes run at once.
#
# TWENTY-FOUR, 003's number (the author, 2026-09-13: this machine's thread count).
# A -j on the command line wins: GNU make 4.4 has already put it in MAKEFLAGS as
# a word beginning -j when this file is read, so the filter is the whole test.
#
# -j4 IS THE ANSWER ON A BUSY MACHINE. A -j8 died with `posix_spawn failed: Cannot
# allocate memory` under a load near 117 on 2026-09-12; `uptime` first.
ifeq ($(filter -j%,$(MAKEFLAGS)),)
MAKEFLAGS += -j24
endif
