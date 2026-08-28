# satellite -- where it goes, and what a run can be told.
#
# Sourced first. Sets the variables every fragment after this one reads; runs
# nothing.

# THE ROOT IS FIXED AND IT IS NOT A PREFIX. Everything satellite owns lives
# under one directory named after the language, which is why the binary is
# $HOME/.satl/satl rather than $HOME/.satl/bin/satl: a prefix layout exists so
# that many packages can share bin/, lib/ and share/, and nothing shares this
# directory. One name to remember, one directory to delete.
#
# share/ underneath it is not a contradiction -- the icons and the mime packet
# keep their XDG-relative paths so that publishing them to the desktop in
# 070-desktop.sh is a link with the same relative path on both sides, rather
# than a translation table between two layouts.
#
# --root is offered because a fixed destination is untestable otherwise: every
# rehearsal of this script would have to write into the home directory it is
# rehearsing for. It is not advertised as a way to install somewhere else.
root=${HOME:?HOME is not set, so there is no home directory to install into}/.satl

action=install
dry_run=no

# THE DECISION IN THIS FILE THAT HAS NOW BEEN MADE THREE TIMES, and the two
# halves of it end up in different places, so they are no longer one comment.
#
# --link IS ON. The first draft had it on, because satellite's rule is to do
# everything for the user and an install that leaves `satl` unfindable by name
# has copied files rather than installed a language. It was then turned OFF on
# the strength of a MEASUREMENT -- 2026-08-26: "this box already has
# ~/.local/bin/satl, 968536 bytes, the FIRST satellite's interpreter" -- because
# defaulting on would have replaced a working interpreter with a build that
# cannot yet interpret anything.
#
# THAT MEASUREMENT HAS EXPIRED. The first satellite was uninstalled from
# $HOME/.local on 2026-08-28 and its /usr/local binary deleted by the author the
# same day. ~/.local/bin/satl is now a 24-byte symlink THIS SCRIPT created,
# pointing into $root. The file the default was protecting does not exist.
#
# It cost the author an evening. With --link off, a plain ./install.sh installs
# a complete language that the word `satl` does not reach, so they hand-edited
# ~/.bashrc to put $HOME/.satl on PATH -- and then told this script, correctly,
# that it must make that unnecessary. An install that only works after the user
# edits a login file is the exact thing install.sh's header refuses to ship.
#
# AND THE SAFETY WAS NEVER THE DEFAULT. This is the part worth writing down,
# because it is what makes the flip safe rather than merely convenient: nothing
# was protecting ~/.local/bin except ours() and the occupied-refusal in
# 070-desktop.sh, and BOTH RUN UNCONDITIONALLY. A path holding anything this
# script did not create is refused, collected, and printed by 080-report.sh --
# whether linking was asked for or assumed. So on-by-default cannot overwrite
# anything; it can only fail loudly, in a named list. Off-by-default was never
# the guard. It was just a quiet way to not find out.
link_bin=yes

# Whether the command line said so, as opposed to this file. 030-arguments.sh
# withdraws the default above for a --root rehearsal and must be able to tell
# "the user asked for links" from "nobody mentioned links".
link_explicit=no
root_given=no

# --desktop STAYS OFF, and it is not an oversight that these two now differ.
# --link creates two symlinks in a bin directory. --desktop writes into
# ~/.local/share AND runs update-mime-database, gtk-update-icon-cache and
# update-desktop-database, which rebuild indexes covering EVERY application on
# the machine. That is a reasonable thing to ask for and an unreasonable thing
# to assume from `./install.sh` with no arguments, and it is not needed for the
# word `satl` to work, which was the whole complaint. The launcher no longer
# depends on it either -- 060-install-tree.sh now writes an absolute Exec, so
# --desktop and --link are independent where they used to be coupled.
#
# --system is unaffected by both: a prefix install writes bin/ and share/ inside
# the prefix, which is already where PATH and the desktop look, so neither flag
# has anything to add there. 070-desktop.sh gates on the layout for that reason.
desktop=no

# WHERE THIS INSTALL LIVES, which is now two questions rather than one.
#
#   layout=root    $HOME/.satl, everything at the top of one directory named
#                  after the language. The default, and everything the header
#                  of install.sh says about a fixed root describes this.
#   layout=prefix  a normal Unix prefix -- bin/ and share/ under $root -- for
#                  installing INTO AN OPERATING SYSTEM rather than beside it.
#
# THE SECOND ONE BREAKS "NO ROOT, EVER", knowingly, on 2026-08-28, at the
# author's instruction, and the reason is a machine this script could not fix
# from inside a home directory. The first satellite is installed at /usr/local,
# which precedes ~/.local/bin on PATH -- so `satl` typed at a prompt ran the
# 2026-08-23 build no matter what this installer did under $HOME, and the .satl
# file type and both icons in the OS were that install's too. A per-user
# installer can report that (080-report.sh does) and can never repair it: the
# files are root-owned and live outside every directory this script was allowed
# to touch. Reporting a problem you have decided in advance never to fix is not
# a doctrine, it is an excuse.
#
# What survives from the old rule is the part that was actually load-bearing:
# THIS SCRIPT STILL NEVER CALLS sudo. System mode requires that the prefix be
# writable by whoever ran it and says so otherwise -- 040-machine.sh -- so the
# escalation is visible in the command the user typed, which is the only place
# it can be audited. Inherited from the first satellite, which put it plainly:
# "a program that silently escalates is a program you cannot audit by reading
# the command you typed."
layout=root

# Prepended to the three program names in 060-install-tree.sh, and empty in the
# default layout. That is the WHOLE difference between the two trees: share/
# already has its XDG-relative shape under $root, deliberately, so publishing it
# to a prefix is the same relative path on both sides and needs no translation
# table. 010's note where the root is declared has said so since it was written.
bin_rel=

# --system with no --prefix. /usr/local and not /usr: /usr belongs to the
# package manager, and a file this script writes there is a file rpm does not
# know about sitting where rpm believes it is authoritative.
system_prefix=/usr/local

# Set by 075-system.sh, read by 080-report.sh, and empty in the home layout --
# declared here because `set -u` is on and 080 reports unconditionally, so a
# variable that only one layout assigns is an unbound-variable exit in the
# other. The same reason have_term starts as `unknown` below rather than unset.
#
#   superseded         first-satellite files removed because this install
#                      replaces the program they described, one path per line
#   theme_index_state  present | installed | missing -- whether the prefix has
#                      the index.theme without which no icon in it is ever
#                      drawn. 075-system.sh explains why that file decides it.
superseded=
theme_index_state=present

# WHETHER TO LINK THE C++ RUNTIME IN. auto | yes | no, resolved by
# 050-building.sh into a STATIC= for make.
#
#   auto  off for a home install, and the best this machine can link for a
#         --system one. Not the same answer twice because it is not the same
#         question: $HOME/.satl is run by the one account that owns the home
#         directory its libraries came out of, and /usr/local/bin/satl is on
#         every account's PATH. See 048-static.mk for what LD_RUN_PATH does to
#         a binary built in this environment.
#   yes   STATIC=full, and make stops with a package name if it cannot.
#   no    link as before.
static=auto

: "${MAKE:=make}"

# Which satl was installed. Set by 050-building.sh, read by 080-report.sh, and
# empty on an uninstall, which builds nothing and chooses nothing.
variant=

# WHETHER THE WINDOW WAS BUILT, which is a question about this machine's
# libraries and not about its instruction set. 047-window.mk asks pkg-config for
# gtk4 and vte-2.91-gtk4 and drops satl-term from `all` when they are missing,
# with a note rather than an error -- so an install on a headless box is a
# correct install of three programs rather than a failure. Set by
# 050-building.sh; read by 060-install-tree.sh, 070-desktop.sh and 080-report.sh.
#
# It starts as `unknown` rather than `no` so that the uninstall can tell the two
# apart: an uninstall builds nothing, so it never learns the answer, and it
# removes satl-term and the launcher by name regardless. Removing a file that
# was never installed costs an rm -f that finds nothing; leaving one behind
# because this run could not prove it was there is how a tree rots.
have_term=unknown
