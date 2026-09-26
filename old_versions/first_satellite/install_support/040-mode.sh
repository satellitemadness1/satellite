# satellite -- source tree or download folder, and may we write there.
#
# $repo is measured in install.sh itself, above the sourcing, because it is
# also where install_support/ was found. The paragraph that explains how it is
# measured moved up there with it.
#
# Moved out of the 634-line install.sh on 2026-08-24, byte for byte.

# TWO KINDS OF TARBALL REACH THIS SCRIPT, and which one this is decides
# everything below.
#
#   source   the repository, or a tarball of it. There is a Makefile beside
#            this script, so the install tree is declared next door and this
#            script's whole job is to choose a prefix and call `make install`.
#
#   bundle   the download folder from the website: two prebuilt binaries, this
#            script, and install_tree/ -- which is that same `make install`
#            tree, already staged prefix-relative by `make bundle`. No
#            Makefile, no sources, nothing to compile. That is the point of a
#            prebuilt download: the machine receiving it needs no toolchain,
#            and requiring one here would have made the whole bundle pointless.
#
# Until bundle mode existed this script died on the spot in the second case --
# "no Makefile beside ./install.sh" -- so a user who downloaded the folder,
# unpacked it and ran the installer inside it installed nothing at all.
#
# Source mode is tested FIRST, because one directory can be both: `make bundle`
# leaves enterprise_download/ inside the repository, and a developer who runs
# the repository's own install.sh means the Makefile, which is the newer and
# more complete answer of the two.
tree=$repo/install_tree
if [ -f "$repo/Makefile" ]; then
    mode=source
elif [ -d "$tree" ]; then
    mode=bundle
else
    die "no Makefile and no install_tree beside $self.
       This installs one of two things: an unpacked satellite source tarball,
       which has a Makefile beside it, or the prebuilt download folder, which
       has an install_tree/ beside it. This directory is neither, so there is
       nothing here to install."
fi

# The nearest existing ancestor is what actually decides the answer:
# --prefix "$HOME/.local" is perfectly installable when .local does not exist
# yet, as long as $HOME does, and testing -w on a directory that is about to be
# created would refuse every first install.
existing_ancestor() {
    _dir=$1
    while [ ! -d "$_dir" ]; do
        _parent=$(dirname -- "$_dir")
        if [ "$_parent" = "$_dir" ]; then
            break
        fi
        _dir=$_parent
    done
    printf '%s\n' "$_dir"
}

target=$destdir$prefix
ancestor=$(existing_ancestor "$target")

if [ ! -w "$ancestor" ]; then
    user=$(id -un 2>/dev/null || printf '%s' "${USER:-you}")
    message="$ancestor is not writable by $user.

  Either install it as root:
      sudo sh $(quoted "$self") --prefix $(quoted "$prefix")
  or install it into your home directory, which needs no privileges:
      sh $(quoted "$self") --prefix \"\$HOME/.local\"

  This script will not run sudo for you. A program that silently escalates is a
  program you cannot audit by reading the command you typed."

    # A dry run is asking what WOULD happen, so it answers that question and
    # says the permissions would stop it, rather than refusing to answer.
    if [ "$dry_run" = yes ]; then
        printf 'install.sh: note — %s\n\n' "$message"
    else
        die "$message"
    fi
fi
