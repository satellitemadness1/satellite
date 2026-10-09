# satellite 004 -- the root, and every root that is refused (PLAN M0.5, D0.5.1).
#
# COMPARED AS REAL PATHS, so `--root ~/.satl/.`, a symlink to /usr/local and a
# relative path from somewhere else are all seen for what they are.
# `realpath -m` resolves a path that does not exist yet.

# The real path of $1, exactly: $(...) strips every trailing newline, and a
# folder name may end in one, so a dot rides behind realpath's own newline.
real_path() {
    _real=$(realpath -m -- "$1" && echo .) || die "cannot read the path $1"
    _real=${_real%.}
    printf '%s' "${_real%?}"
}

root_real=$(real_path "$root" && echo .)
root_real=${root_real%.}

refuse_root() {
    die "will not install into $root: $1"
}

# \$HOME/.satl IS WHERE 004 INSTALLS (D0.5.1, ruled 2026-09-22), and so is the one
# in this account's passwd entry -- a HOME changed by sudo or env still names the
# same folder. That root takes over 003's satl there (060 keeps it as satl.bak-)
# and may be on PATH, because becoming the word satl is the point of it. A root
# INSIDE it is refused: a second satl under the first.
[ -n "${HOME:-}" ] || die "HOME is not set, so \$HOME/.satl cannot be told apart from --root"
passwd_home=$(getent passwd "$(id -u)" 2>/dev/null | cut -d: -f6) || passwd_home=
home_root=no
for _home in "$HOME" ${passwd_home:+"$passwd_home"}; do
    _home_satl=$(real_path "$_home/.satl" && echo .)
    _home_satl=${_home_satl%.}
    case $root_real in
        "$_home_satl") home_root=yes ;;
        "$_home_satl"/*)
            refuse_root "it is inside $_home/.satl, where satellite 004 itself installs" ;;
    esac
done
case $root_real in
    /usr/local | /usr/local/bin | /usr/local/share | /usr/local/share/*)
        refuse_root "/usr/local, its bin/ and its share/ are where satellite 003's --system install goes, and a system install of 004 is not decided (PLAN D0.5.1)" ;;
esac

repo_real=$(real_path "$repo" && echo .)
repo_real=${repo_real%.}
[ "$root_real" != "$repo_real" ] ||
    refuse_root "it is the repository's top folder, which is on the author's PATH -- a file named satl there would become the word satl"

# ANY OTHER FOLDER ON PATH: a satl there would compete with \$HOME/.satl's for the
# word satl. An empty entry means the current folder, and the ':' added here keeps
# a TRAILING empty entry, which field splitting would otherwise drop.
_saved_ifs=$IFS
IFS=:
set -f
_path=${PATH-}:
for _entry in $_path; do
    [ -n "$_entry" ] || _entry=.
    _entry_real=$(real_path "$_entry" && echo .)
    _entry_real=${_entry_real%.}
    if [ "$_entry_real" = "$root_real" ] && [ "$home_root" = no ]; then
        IFS=$_saved_ifs
        set +f
        refuse_root "it is on PATH ($_entry), where a file named satl would compete with \$HOME/.satl's for the word satl"
    fi
done
IFS=$_saved_ifs
set +f

if [ -e "$root" ] || [ -L "$root" ]; then
    [ -d "$root" ] || refuse_root "it exists and is not a folder"
    [ -w "$root" ] || refuse_root "it is not writable"
else
    # The nearest folder that exists must let the root be made -- asked before make runs.
    _parent=$root_real
    while [ ! -e "$_parent" ]; do _parent=$(dirname -- "$_parent"); done
    [ -d "$_parent" ] && [ -w "$_parent" ] || refuse_root "it does not exist, and $_parent cannot hold it"
fi

# WHAT THIS INSTALLER PUT IN A ROOT IS KNOWN FROM ITS RECORD, NEVER BY RUNNING A
# FILE: a satl there -- or a satl-term an earlier install left, which 060
# removes -- is ours only when the record holds its exact
# sha256, and satellite-numbers/ only when there is a record at all. A symlink is
# never ours, because this installer writes plain files. Asked here, before make,
# and again in 060 just before anything is written, because make takes minutes.
record=$root/$record_name

ours() {
    _sum=$(sha256sum < "$root/$1") || return 1
    grep -qxF -- "${_sum%% *}  $1" "$record"
}

# IN \$HOME/.satl, A PLAIN satl THAT IS NOT OURS IS 003's, AND IS KEPT: 060 links it
# to satl.bak-<date> before the new one takes the name (set_aside_satl). A
# satl-term that is not ours is 003's too, and is not this installer's to judge or
# to remove, so it is passed over.
check_the_root_is_ours() {
    set_aside_satl=no
    if [ -e "$record" ] || [ -L "$record" ]; then
        [ -f "$record" ] && [ ! -L "$record" ] || refuse_root "its $record_name is not a plain file, so it is not this installer's record"
    fi
    for _name in satl satl-term; do
        [ -e "$root/$_name" ] || [ -L "$root/$_name" ] || continue
        if [ "$home_root" = yes ] && [ -f "$root/$_name" ] && [ ! -L "$root/$_name" ] &&
            ! { [ -f "$record" ] && ours "$_name"; }; then
            [ "$_name" = satl-term ] || set_aside_satl=yes
            continue
        fi
        [ ! -L "$root/$_name" ] || refuse_root "its $_name is a symlink, and this installer writes only plain files"
        [ -f "$root/$_name" ] || refuse_root "its $_name is not a plain file"
        [ -f "$record" ] || refuse_root "it holds a $_name this installer did not put there (there is no $record_name) -- satellite 003's, or someone else's. Move it, or choose another folder"
        ours "$_name" || refuse_root "it holds a $_name this installer did not put there (no line in $record_name has its sha256) -- changed since, or not this installer's. Move it, or choose another folder"
    done
    if [ -e "$root/satellite-numbers" ] || [ -L "$root/satellite-numbers" ]; then
        [ -f "$record" ] || refuse_root "it holds a satellite-numbers/ this installer did not put there (there is no $record_name)"
        [ -d "$root/satellite-numbers" ] && [ ! -L "$root/satellite-numbers" ] ||
            refuse_root "its satellite-numbers is not a folder this installer made"
    fi
}

check_the_root_is_ours
