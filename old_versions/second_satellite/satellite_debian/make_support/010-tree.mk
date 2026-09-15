# satellite -- where the source tree is, and the one rule about touching it.
#
# BEFORE 020-inherited.mk, which spells both its include paths and its SRC
# override through $(ROOT). Nothing else in this directory names `..` at all.

# THE PARENT TREE IS READ ONLY TO THIS BUILD. It is where the language lives --
# src/, pcg/, make_support/ -- and this directory reads all three and writes to
# none of them. Every file this build creates is under build/, which is what
# lets a Debian build and a root build coexist in one checkout: the root build
# puts main.o beside main.cpp, and if this one did too the two would be the same
# file compiled with different flags, taking turns to be wrong.
#
# ONE VARIABLE AND NOT A DOZEN `..`s, so that a directory which moves -- this
# one going one level deeper, the tree being vendored somewhere else -- is one
# edit here rather than a hunt through seven fragments. This is the same
# argument 030-directories.mk makes in the parent for SRC, SYSTEM and the rest.
ROOT = ..

# The third-party header satellite.random reaches for. Named here beside ROOT
# rather than in 060-compile.mk where it is used, because it is a fact about
# where the tree is and not about how a .cpp becomes a .o. pcg/README.md says
# which single type is used and why 512-bit was refused.
PCG = $(ROOT)/pcg/include

# A SENTENCE INSTEAD OF `No such file or directory. Stop.`
#
# Every fragment 020 includes lives in the parent tree, and make's own diagnostic
# for a missing include names one file and explains nothing -- which is exactly
# the wrong answer for the commonest way to arrive here, namely copying this
# directory somewhere on its own and expecting it to build. It cannot: it is a
# build OF the tree above it, not a copy of one.
#
# 040-sources.mk is the file tested because it is the one that declares what
# gets compiled. If it is there, the tree is there.
ifeq (,$(wildcard $(ROOT)/make_support/040-sources.mk))
$(error this is a build of the satellite tree above it, and $(ROOT) does not \
look like one -- $(ROOT)/make_support/040-sources.mk is missing. \
satellite_debian/ reads ../src, ../pcg and ../make_support and cannot be \
moved or copied away from them. Run make from inside satellite_debian/ \
in a full checkout)
endif

# SET HERE, SEVEN FRAGMENTS BEFORE `all` EXISTS, and that is deliberate.
#
# make's default goal is the first target of the first rule, skipping targets
# whose names begin with a dot -- UNLESS the name also contains a slash, in
# which case the dot rule does not apply. The parent gets away with declaring
# `.ldflags-stamp` in 048 before `all` in 050 precisely because that name is
# dotted and slashless. Here the same stamp is `build/.ldflags-stamp`, which has
# a slash in it, so it is an ordinary name as far as this rule is concerned and
# it would become the default goal: a bare `make` would write one stamp file and
# report success, having built nothing.
#
# .DEFAULT_GOAL is a variable and not a rule, so setting it wins wherever it is
# set. It is set in the FIRST fragment rather than beside `all` in 050, so that
# no future fragment inserted between here and there can quietly take the
# default goal by declaring a target with a slash in its name.
.DEFAULT_GOAL := all
