# satellite -- install the packages satl-term needs, with dnf.
#
# THIS FRAGMENT CALLS sudo, WHICH THE OTHER TWO INSTALLERS IN THIS TREE REFUSE
# TO DO. That refusal was a good rule -- "a program that silently escalates is a
# program you cannot audit by reading the command you typed" -- and it is
# overridden here at the author's instruction, for this package and its
# audience. The rule it is replaced by is weaker but is still a rule: NOTHING IS
# ESCALATED SILENTLY. Every dnf command is printed in full before it runs, the
# reason is printed above it, sudo asks for the password itself so the prompt is
# visibly the system's and not this script's, and --no-deps declines the whole
# thing and installs everything else anyway.
#
# IT RUNS BEFORE 040-machine.sh ON PURPOSE. That fragment decides whether
# satl-term can run by asking ldd whether every library it names resolves. Doing
# the install first means that check sees the libraries this one just put there,
# so a machine that was missing vte gets the window rather than a report
# explaining how to get it.
#
# WHY A LIST OF PACKAGES AND NOT JUST vte291-gtk4. Installing vte291-gtk4 does
# pull in gtk4, pango, cairo and the rest as dependencies, so the short version
# works. The long version is written out because dnf answers "already installed"
# for free, and because this list is then a readable statement of what satl-term
# actually links against -- derived, on 2026-09-08, by asking rpm which package
# owns each library in `readelf -d satl-term`. glibc is deliberately NOT in it:
# it is on every machine by definition, and a script asking to install libc is a
# script that looks like it is doing something else.

# THE FOUR AT THE END ARE NOT satl-term's LIBRARIES, they are the tools that
# make an installed icon visible, and they are here because their absence has
# exactly the same symptom -- nothing appears and nothing reports an error.
#   hicolor-icon-theme       ships index.theme, without which GTK does not read
#                            an icon directory AT ALL. See 070-desktop.sh.
#   shared-mime-info         update-mime-database, which registers .satl
#   desktop-file-utils       update-desktop-database, which is what makes the
#                            launcher the handler for a .satl file
#   gtk-update-icon-cache    the icon cache builder
satl_term_packages='vte291-gtk4 gtk4 pango cairo cairo-gobject harfbuzz
gdk-pixbuf2 graphene vulkan-loader glib2
hicolor-icon-theme shared-mime-info desktop-file-utils gtk-update-icon-cache'

dependencies_state=skipped
dependencies_missing=
dependencies_failed=

if [ "$install_deps" = yes ] && [ "$action" = install ]; then
    if ! command -v dnf >/dev/null 2>&1; then
        # NOT AN ERROR. A machine without dnf is a machine this package was not
        # built for, and 040-machine.sh says so in its own words. The libraries
        # may well be there anyway, which is what the ldd check is for.
        dependencies_state=no-dnf
    elif ! command -v rpm >/dev/null 2>&1; then
        dependencies_state=no-rpm
    else
        # ASKED PACKAGE BY PACKAGE, so that dnf is only invoked when there is
        # something to do. `dnf install` on a fully satisfied list is not
        # harmful, but it is a network round trip and a password prompt for
        # nothing, and on a machine that is already complete this script should
        # not ask for a password at all.
        for _pkg in $satl_term_packages; do
            rpm -q "$_pkg" >/dev/null 2>&1 ||
                dependencies_missing="$dependencies_missing $_pkg"
        done
        # Strip the leading space so the list prints cleanly.
        dependencies_missing=${dependencies_missing# }

        if [ -z "$dependencies_missing" ]; then
            dependencies_state=complete
        else
            # WHO RUNS IT. Already root -- which is the --system case, where the
            # person has run this whole script under sudo -- means dnf directly;
            # sudo would be a second escalation of an escalation. Otherwise sudo,
            # if it is there.
            if [ "$(id -u)" = 0 ]; then
                _dnf='dnf'
            elif command -v sudo >/dev/null 2>&1; then
                _dnf='sudo dnf'
            else
                _dnf=
            fi

            if [ -z "$_dnf" ]; then
                dependencies_state=no-sudo
            else
                say ""
                say "$me: satl-term needs some system packages that are not installed:"
                say ""
                printf '        %s\n' $dependencies_missing
                say ""
                say "    Installing them now. This is the ONE step that needs the"
                say "    administrator password, and the prompt below comes from sudo"
                say "    itself, not from this script. The command being run is:"
                say ""
                say "        $_dnf install -y $dependencies_missing"
                say ""

                # `|| :` AND A STATE VARIABLE RATHER THAN set -e TAKING THE
                # SCRIPT DOWN. A machine with no network, a disabled repository
                # or a declined password should still get a complete satellite
                # install: the interpreter needs none of this, and the window is
                # reported as unavailable by 040-machine.sh in the same words it
                # would use on any other machine that lacks the libraries.
                if run $_dnf install -y $dependencies_missing; then
                    dependencies_state=installed
                else
                    dependencies_state=failed
                    dependencies_failed=$dependencies_missing
                fi
                say ""
            fi
        fi
    fi
fi
