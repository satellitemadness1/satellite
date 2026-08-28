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

# OFF BY DEFAULT, and this is the one decision in this file that was made
# twice.
#
# Both of these write outside the root: a symlink in ~/.local/bin, and symlinks
# under ~/.local/share. The first draft had them on, on the grounds that
# satellite's rule is to do everything for the user and an install that leaves
# `satl` unfindable by name has copied files rather than installed a language.
#
# Then the machine was looked at. MEASURED 2026-08-26: this box already has
# ~/.local/bin/satl -- 968536 bytes, the FIRST satellite's interpreter -- plus
# satl-term, the nine hicolor sizes, an index.theme and the mime packet, all
# installed there by that satellite's own installer. Defaulting these on would
# have replaced a working interpreter that the user still runs, on the first
# plain ./install.sh, with a build that cannot yet interpret anything.
#
# So they are off, and the other half of satellite's rule is the one that
# decides: doing everything for the user is never doing something behind their
# back. A plain run writes under $root and nowhere else, which is exactly what
# was asked for. 070-desktop.sh performs these when asked and REFUSES to
# overwrite anything it does not own even then, and 080-report.sh prints what
# turning them on would do.
link_bin=no
desktop=no

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
