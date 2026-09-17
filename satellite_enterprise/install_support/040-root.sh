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

# \$HOME/.satl IS 003's, AND SO IS THE ONE IN THIS ACCOUNT'S passwd ENTRY: a HOME
# changed by sudo or env would otherwise let a root inside the real one through.
[ -n "${HOME:-}" ] || die "HOME is not set, so \$HOME/.satl, satellite 003's install, cannot be told apart from --root"
passwd_home=$(getent passwd "$(id -u)" 2>/dev/null | cut -d: -f6) || passwd_home=
for _home in "$HOME" ${passwd_home:+"$passwd_home"}; do
    _home_satl=$(real_path "$_home/.satl" && echo .)
    _home_satl=${_home_satl%.}
    case $root_real in
        "$_home_satl" | "$_home_satl"/*)
            refuse_root "$_home/.satl is satellite 003's install, and satellite 004 does not install over it or inside it" ;;
    esac
done
case $root_real in
    /usr/local | /usr/local/bin | /usr/local/share | /usr/local/share/*)
        refuse_root "/usr/local, its bin/ and its share/ are where satellite 003's --system install goes, and where 004 installs is not decided yet (PLAN D0.5.1)" ;;
esac

repo_real=$(real_path "$repo" && echo .)
repo_real=${repo_real%.}
[ "$root_real" != "$repo_real" ] ||
    refuse_root "it is the repository's top folder, which is on the author's PATH -- a file named satl there would become the word satl"

# ANY FOLDER ON PATH: a satl there would change what `satl` runs -- which is 003's
# today (D0.5.1). An empty entry means the current folder, and the ':' added here
# keeps a TRAILING empty entry, which field splitting would otherwise drop.
_saved_ifs=$IFS
IFS=:
set -f
_path=${PATH-}:
for _entry in $_path; do
    [ -n "$_entry" ] || _entry=.
    _entry_real=$(real_path "$_entry" && echo .)
    _entry_real=${_entry_real%.}
    if [ "$_entry_real" = "$root_real" ]; then
        IFS=$_saved_ifs
        set +f
        refuse_root "it is on PATH ($_entry), where a file named satl would change what the word satl runs -- today satellite 003's (PLAN D0.5.1)"
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
# FILE: a satl or satl-term there is ours only when the record holds its exact
# sha256, and satellite-numbers/ only when there is a record at all. A symlink is
# never ours, because this installer writes plain files. Asked here, before make,
# and again in 060 just before anything is written, because make takes minutes.
record=$root/$record_name

ours() {
    _sum=$(sha256sum < "$root/$1") || return 1
    grep -qxF -- "${_sum%% *}  $1" "$record"
}

check_the_root_is_ours() {
    if [ -e "$record" ] || [ -L "$record" ]; then
        [ -f "$record" ] && [ ! -L "$record" ] || refuse_root "its $record_name is not a plain file, so it is not this installer's record"
    fi
    for _name in satl satl-term; do
        [ -e "$root/$_name" ] || [ -L "$root/$_name" ] || continue
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
