#!/usr/bin/env bash
#
# satellite -- `make startup`. What satl costs to start, measured rather than
# remembered.
#
# PLAN §9's fourth rule is "startup is re-measured every milestone" and until
# 2026-08-31 there was nothing to run. MILESTONES/M6.md §6.9 is the cost of
# that: M4, M4.5 and M5 landed with no measurement, and when M6 finally took
# one it found satl's own share of startup had gone from 0.018 ms to 0.620 --
# 34x -- with three milestones of history to search through for the cause. This
# script is the target that rule needed. make_support/startup.rows is its data,
# and adding a command to the measurement is one line in that file.
#
# WHAT IT DOES DIFFERENTLY FROM A SHELL LOOP SOMEBODY TYPES. Three things, and
# each of them is a mistake this project has already made once:
#
#   1. THE FLOOR IS LINKED THE WAY satl IS. 067-startup.mk builds
#      `int main(){return 0;}` through the same $(CXX) $(CXXFLAGS) $(LDFLAGS)
#      $(STATIC_LDFLAGS) $(LINK_ENV) that 050-build.mk links satl through. The
#      mistake recorded in 040-sources.mk -- timing a static satl against a
#      dynamic empty program, which made satl look 0.9 ms FASTER than doing
#      nothing -- is not one this harness can make.
#   2. IT REPORTS THE SHARE, NOT THE ROW. satl's row minus the floor is what a
#      milestone is answerable for; the row itself moves with the link mode, the
#      loader and the machine. startup.rows carries the argument and the three
#      measurements behind it.
#   3. IT RUNS EVERY COMMAND ONCE AND CHECKS THE STATUS BEFORE TIMING IT. A
#      command that fails fast IS fast, and a table of them reads as an
#      improvement. This tree has been bitten by that shape before --
#      030-directories.mk's note about a test binary nobody built printing PASS.
#
# NOT PART OF `make test`, AND THAT IS A DECISION RATHER THAN AN OMISSION. Eight
# rows of five batches of two hundred invocations is about ten seconds, and the
# answer depends on what else the machine is doing -- so a suite that included
# it would be slow and would go red on a busy afternoon. A check that fails for
# reasons the change under test cannot cause is a check people learn to ignore.
#
# AND IT EXITS 0 EVEN WHEN IT FINDS A REGRESSION, for the same reason. What §6.9
# describes is nobody running the measurement, and an exit status does not fix
# that -- running it does. What a non-zero status WOULD do is put this script on
# the list of things to disable the first time the machine is busy.

set -u -o pipefail

SATL=${SATL:-./satl}
FLOOR=${FLOOR:-.startup-floor}
ROWS=${ROWS:-make_support/startup.rows}
RUNS=${RUNS:-200}
BATCHES=${BATCHES:-5}

# The conditions, passed in by 067-startup.mk because they are the build's facts
# and not this script's. A second probe here would be a second decision that can
# disagree with 010-compiler.mk and 048-static.mk -- which is the argument 048's
# own `static-available` target already makes for install.sh.
STARTUP_CXX=${STARTUP_CXX:-unknown}
STARTUP_CXXFLAGS=${STARTUP_CXXFLAGS:-unknown}
STARTUP_LINK=${STARTUP_LINK:-unknown}

for f in "$SATL" "$FLOOR" "$ROWS"; do
    [ -e "$f" ] || { echo "make startup: $f is not there." >&2; exit 1; }
done

# A NAME WITH NO SLASH IN IT IS A PATH SEARCH, and that is how this script first
# failed. 067-startup.mk names the floor `.startup-floor`, which -e finds in the
# working directory and which bash then looks for on $PATH and does not find --
# so the check above passed and the run died one line into the table. `./` in
# the makefile would fix this one call site; doing it here fixes every way the
# script can be invoked, including by hand with SATL=satl.
case $SATL in */*) ;; *) SATL=./$SATL ;; esac
case $FLOOR in */*) ;; *) FLOOR=./$FLOOR ;; esac

trim() {
    local s=$1
    s=${s#"${s%%[![:space:]]*}"}
    s=${s%"${s##*[![:space:]]}"}
    printf '%s' "$s"
}

# --- reading the registry ----------------------------------------------------
#
# Parsed into three parallel arrays. A `-` in a baseline field survives as the
# string and every reader below tests for it, rather than being turned into a 0
# that would print as an enormous improvement.

declare -a KIND=() BASE=() ARGS=()
BASE_DATE= BASE_LINK= BASE_LOAD= BASE_NOTE= BASE_FLOOR=

while IFS= read -r line || [ -n "$line" ]; do
    case $line in ''|'#'*) continue ;; esac
    case $line in
    DATE*)  BASE_DATE=$(trim "${line#DATE}") ;;
    LINK*)  BASE_LINK=$(trim "${line#LINK}") ;;
    LOAD*)  BASE_LOAD=$(trim "${line#LOAD}") ;;
    NOTE*)  BASE_NOTE=$(trim "${line#NOTE}") ;;
    FLOOR*) BASE_FLOOR=$(trim "${line#*|}")
            KIND+=(floor); BASE+=("$BASE_FLOOR"); ARGS+=("") ;;
    ROW*)   rest=${line#*|}
            KIND+=(satl)
            BASE+=("$(trim "${rest%%|*}")")
            ARGS+=("$(trim "${rest#*|}")") ;;
    *)      echo "make startup: $ROWS: cannot read '$line'" >&2; exit 1 ;;
    esac
done < "$ROWS"

[ -n "$BASE_FLOOR" ] || { echo "make startup: $ROWS has no FLOOR row." >&2; exit 1; }

# --- the measurement ---------------------------------------------------------
#
# Best of $BATCHES runs of $RUNS invocations, which is the method every figure in
# 040-sources.mk was taken with, kept so these numbers stay comparable to the
# ones already written down. BEST and not mean: every source of noise on a shared
# machine adds time and none of it subtracts, so the minimum is the closest
# reading to what satl costs and the mean is a reading of the afternoon.
#
# EPOCHREALTIME AND NOT `date +%s%N`. It is a bash builtin, so the clock is read
# without forking -- and a fork per timestamp is about the size of the whole
# quantity being measured.
#
# OUTPUT TO /dev/null AND SATL_NO_WINDOW=1. The second is not tidiness: satl
# hands itself to satl-term when it has no controlling terminal and nothing is
# reading its output (programs/window_handover.cpp), and /dev/null is a
# character device, which that file counts as nothing reading. A `make startup`
# from cron or CI on a machine with a DISPLAY would otherwise open two hundred
# windows. Measured while this was written: setting the variable costs 0.003 ms
# against letting the /dev/tty probe run, which is inside the noise.
measure() {
    local best=999999999 b i s e us
    for ((b = 0; b < BATCHES; b++)); do
        s=${EPOCHREALTIME/./}
        for ((i = 0; i < RUNS; i++)); do "$@" >/dev/null 2>&1; done
        e=${EPOCHREALTIME/./}
        us=$((e - s))
        ((us < best)) && best=$us
    done
    awk -v u="$best" -v n="$RUNS" 'BEGIN { printf "%.3f", u / 1000 / n }'
}

# --- the report --------------------------------------------------------------

case $STARTUP_LINK in
0)    link_says='STATIC=0 -- libstdc++ and libc dynamic' ;;
1)    link_says='STATIC=1 -- static C++ runtime, dynamic libc' ;;
full) link_says='STATIC=full -- no shared libraries at all' ;;
*)    link_says="STATIC=$STARTUP_LINK" ;;
esac

echo
echo "satl startup -- $(date '+%Y-%m-%d %H:%M'), load $(awk '{print $1}' /proc/loadavg)"
echo "  compiler   $STARTUP_CXX"
echo "  flags      $STARTUP_CXXFLAGS"
echo "  linked     $link_says"
echo "  method     best of $BATCHES runs of $RUNS, output to /dev/null, SATL_NO_WINDOW=1"
echo "  baseline   $BASE_DATE, STATIC=$BASE_LINK, load $BASE_LOAD"
echo "             $BASE_NOTE"
echo
echo "  A SHARE IS THE ROW MINUS THE FLOOR -- what satl costs once the machine's"
echo "  own price for starting any program at all is taken off it."
echo
printf '  %-44s %7s %7s %7s %7s\n' 'command' 'ms' 'share' 'was' 'delta'
printf '  %s\n' '---------------------------------------------------------------------------'

floor_ms=
regressed=0
started=${EPOCHREALTIME%%.*}

for n in "${!KIND[@]}"; do
    if [ "${KIND[$n]}" = floor ]; then
        "$FLOOR" >/dev/null 2>&1 ||
            { echo "make startup: $FLOOR will not run." >&2; exit 1; }
        floor_ms=$(measure "$FLOOR")
        printf '  %-44s %7s %7s %7s %7s\n' \
            'int main(){return 0;}' "$floor_ms" '--' '--' '--'
        continue
    fi

    # THE FLOOR ROW COMES FIRST AND THIS IS WHAT SAYS SO. Every share below is
    # measured against it, so a rows file that put a ROW above the FLOOR would
    # otherwise subtract an empty string and report satl's whole run time as its
    # share -- a wrong number that looks like a catastrophic regression.
    [ -n "$floor_ms" ] ||
        { echo "make startup: $ROWS has a ROW before its FLOOR." >&2; exit 1; }

    # Deliberately split on whitespace: the field holds a command line.
    # shellcheck disable=SC2206
    argv=(${ARGS[$n]})
    label="satl ${ARGS[$n]}"

    # ONCE FOR THE STATUS, BEFORE TWO HUNDRED TIMES FOR THE CLOCK -- the third
    # numbered paragraph at the top of this file.
    if ! SATL_NO_WINDOW=1 "$SATL" "${argv[@]}" >/dev/null 2>&1; then
        printf '  %-44s %7s  FAILED -- not timed\n' "$label" '--'
        regressed=1
        continue
    fi

    ms=$(SATL_NO_WINDOW=1 measure "$SATL" "${argv[@]}")

    read -r share was delta mark <<<"$(awk -v ms="$ms" -v fl="$floor_ms" \
        -v base="${BASE[$n]}" -v bfloor="$BASE_FLOOR" 'BEGIN {
            share = ms - fl
            if (base == "-") { printf "%.3f - - .", share; exit }
            was = base - bfloor
            d = share - was
            limit = 0.2 * was
            if (limit < 0.030) limit = 0.030
            printf "%.3f %.3f %+.3f %s", share, was, d, (d > limit ? "<<<" : ".")
        }')"

    [ "$mark" = '<<<' ] && regressed=1
    [ "$mark" = '.' ] && mark=''
    printf '  %-44s %7s %7s %7s %7s %s\n' "$label" "$ms" "$share" "$was" "$delta" "$mark"
done

printf '  %s\n' '---------------------------------------------------------------------------'
echo "  took $((${EPOCHREALTIME%%.*} - started))s"
echo

# THE PARAGRAPH THAT STOPS THE ms COLUMN BEING READ AS A RESULT. Printed every
# time and not only on a mismatch, because the reader who needs it is the one
# who does not already know to check.
if [ "$STARTUP_LINK" != "$BASE_LINK" ]; then
    echo "  THE ms COLUMN IS NOT COMPARABLE TO THE BASELINE'S. This build is"
    echo "  STATIC=$STARTUP_LINK and the baseline was taken at STATIC=$BASE_LINK. The two differ"
    echo "  by about 0.9 ms of dynamic loader on this machine, which is five times"
    echo "  satl's entire share. The share columns ARE comparable -- each is its"
    echo "  own build's row minus its own build's floor -- and startup.rows carries"
    echo "  the measurement that says so."
else
    echo "  This build and the baseline are both STATIC=$STARTUP_LINK, so the ms column is"
    echo "  comparable as well as the share."
fi
echo

if [ "$regressed" = 1 ]; then
    echo "  <<< SOMETHING GOT SLOWER. A row is marked when its share grew by more"
    echo "  than 0.030 ms AND by more than a fifth. Attribute it before the next"
    echo "  milestone lands on top of it -- MILESTONES/M6.md §9.1 is what happens"
    echo "  when four of them stack up behind one unmeasured change."
    echo
    echo "  When the new number is correct rather than a regression, the table in"
    echo "  make_support/startup.rows is what to update, and PLAN §9's second rule"
    echo "  asks for the reason to go beside it."
else
    echo "  Nothing regressed. When these are the numbers to keep, they go into"
    echo "  make_support/startup.rows, and the account of what moved goes into"
    echo "  make_support/040-sources.mk beside the decision it justifies."
fi
echo

# ALWAYS 0. The top of this file says why.
exit 0
