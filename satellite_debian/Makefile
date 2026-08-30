# satellite -- the Debian family build, out of tree.
#
# THIS FILE IS AN INDEX, like the Makefile at the root of the tree and for the
# same reason: the build is the fragments under make_support/, included below in
# the order they are numbered. The root file says a ceiling changes a file's
# shape while splitting to one preserves it; this directory starts split.
#
# WHAT THIS DIRECTORY IS FOR. The root build compiles satellite for whatever
# machine it is run on and leaves its objects beside the sources. This one
# builds the same four binaries for the Debian family and puts every file it
# makes under build/. Nothing it writes lands outside this directory, so the two
# builds sit in one tree without either standing on the other's objects.
#
#     $ cd satellite_debian && make
#     $ ls build/
#     objects  satl  satl-cpu-level  satl-term  satl.haswell
#
# -- the four binaries, plus the object tree they were made from. The three
# flag stamps are dotted and so are not listed; `ls -a` shows them.
#
# ONE DIRECTORY FOR DEBIAN AND UBUNTU, and that is measured rather than assumed.
# Every name this build depends on is spelled identically on both: the packages
# are build-essential, pkg-config, libgtk-4-dev and libvte-2.91-gtk4-dev; the
# pkg-config module is vte-2.91-gtk4; `c++` is g++; libstdc++.a comes from g++
# and libc.a from libc6-dev. Measured on Debian 13 (trixie), 2026-08-28, and
# checked against packages.ubuntu.com for noble, questing and resolute. A second
# directory would be a second copy of one thing, and copies drift.
#
# THE FLOOR IS DEBIAN 13 AND UBUNTU 24.04, and it is satl-term that sets it.
# vte gained its GTK4 build at 0.70; Ubuntu 22.04 ships 0.68 and has no
# libvte-2.91-gtk4-dev at all, so on jammy the other three binaries build and
# the window cannot. That is not a special case in this build -- it is the
# ordinary no-gtk4 path 050-build.mk already describes.
#
# WHERE TO LOOK, by what you want to change:
#
#     where the source tree is ............ 010-tree.mk
#     which parent fragments are reused ... 020-inherited.mk
#     where objects and binaries land ..... 030-output.mk
#     static linking and its apt advice ... 040-static.mk
#     a new target ........................ 050-build.mk
#     how a .cpp becomes a .o ............. 060-compile.mk
#     what a clean removes ................ 070-clean.mk
#
# ORDER IS LOAD-BEARING three times, and each fragment says so at its top:
# 010 before 020, because the include paths and the SRC override are spelled
# through $(ROOT); 020 before 030, because 030 derives every object path from
# the source lists 020 inherits; and 030 and 040 before 050, because 050 tests
# MICROARCH_VARIANTS and HAVE_WINDOW with an ifeq, which make evaluates as it
# reads rather than afterwards.
#
# NAMED ONE BY ONE rather than $(wildcard make_support/*.mk), for the reason the
# root Makefile gives: a wildcard sorts asciibetically and silently includes
# whatever else lands in the directory.
#
# The paths are RELATIVE -- to the parent tree as well as to this directory --
# so make must be run with this directory as its working directory.

include make_support/010-tree.mk
include make_support/020-inherited.mk
include make_support/030-output.mk
include make_support/040-static.mk
include make_support/050-build.mk
include make_support/060-compile.mk
include make_support/070-clean.mk
