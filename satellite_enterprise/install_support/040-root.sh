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

home_satl=$(real_path "$HOME/.satl" && echo .)
home_satl=${home_satl%.}
case $root_real in
    "$home_satl" | "$home_satl"/*)
        refuse_root "\$HOME/.satl is satellite 003's install, and satellite 004 does not install over it or inside it" ;;
    /usr/local | /usr/local/bin)
        refuse_root "/usr/local is where satellite 003's --system install goes, and where 004 installs is not decided yet (PLAN D0.5.1)" ;;
esac

repo_real=$(real_path "$repo" && echo .)
repo_real=${repo_real%.}
[ "$root_real" != "$repo_real" ] ||
    refuse_root "it is the repository's top folder, which is on the author's PATH -- a file named satl there would become the word satl"

# ANY FOLDER ON PATH: a satl there would change what `satl` runs, for every
# script and shell that finds it -- which is 003's today (D0.5.1). An empty PATH
# entry means the current folder.
_saved_ifs=$IFS
IFS=:
set -f
for _entry in $PATH; do
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
fi

# WHAT THIS INSTALLER PUT IN A ROOT IS KNOWN FROM ITS RECORD, NEVER BY RUNNING A
# FILE: a satl or satl-term there is ours only when the record holds its exact
# sha256, and satellite-numbers/ only when there is a record at all. A symlink is
# never ours, because this installer writes plain files.
record=$root/$record_name

ours() {
    [ ! -L "$root/$1" ] && [ -f "$root/$1" ] && [ -f "$record" ] || return 1
    _sum=$(sha256sum < "$root/$1") || return 1
    grep -qxF -- "${_sum%% *}  $1" "$record"
}

for _name in satl satl-term; do
    if [ -e "$root/$_name" ] || [ -L "$root/$_name" ]; then
        ours "$_name" || refuse_root "it holds a $_name this installer did not put there (no line in $record_name has its sha256) -- satellite 003's, or changed since. Move it, or choose another folder"
    fi
done
if { [ -e "$root/satellite-numbers" ] || [ -L "$root/satellite-numbers" ]; } && [ ! -f "$record" ]; then
    refuse_root "it holds a satellite-numbers/ this installer did not put there (there is no $record_name)"
fi
