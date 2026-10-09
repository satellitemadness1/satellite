#!/bin/bash
# satellite/satellite_variable_number/number_race.sh -- the M4 race, i = i + 1.
# Seven runs in each order for each count; the fastest time of each side is
# compared. A run that fails, or whose answers differ, stops the race with its
# exit status instead of counting (the fault race.sh has, ERROR.md 9).
#
# Two C++ sides: the author's plain signed long long loop (which both compilers
# turn into one multiplication), and the same loop refusing to wrap, which
# performs every addition. Both ratios are printed; neither loop is changed.
#
#     satellite/satellite_variable_number/number_race.sh [build/number_race] [increment]
cd "$(dirname "$0")/../.."
race=${1:-build/number_race}
increment=${2:-1}
for count in 1000000 100000000; do
    best_satellite="" best_cxx="" best_checked=""
    for order in satellite-first cxx-first; do
        for run in 1 2 3 4 5 6 7; do
            output=$("$race" $order $count $increment 2>&1)
            status=$?
            read satellite cxx checked satellite_i cxx_i <<<"$output"
            if [ $status -ne 0 ] || ! [[ "$satellite" =~ ^[0-9]+$ && "$cxx" =~ ^[0-9]+$ && "$checked" =~ ^[0-9]+$ ]]; then
                echo "run failed (exit $status): $output"
                exit 1
            fi
            if [ -z "$best_satellite" ] || [ "$satellite" -lt "$best_satellite" ]; then best_satellite=$satellite; fi
            if [ -z "$best_cxx" ] || [ "$cxx" -lt "$best_cxx" ]; then best_cxx=$cxx; fi
            if [ -z "$best_checked" ] || [ "$checked" -lt "$best_checked" ]; then best_checked=$checked; fi
        done
    done
    python3 -c "
s, c, k, n = $best_satellite, $best_cxx, $best_checked, $count
print(f'i = i + $increment, {n:,} times, fastest of 14 runs (7 in each order); every i = $cxx_i')
print(f'  satellite_number:                    {s:>13,} ns  {s/n:7.3f} ns each')
print(f'  signed long long int (the race):     {c:>13,} ns  {c/n:7.3f} ns each')
print(f'  signed long long int, never wraps:   {k:>13,} ns  {k/n:7.3f} ns each')
print(f'  satellite_number is x{s/c:.3f} of the race C++ (the bar is x1.05), x{s/k:.3f} of C++ that never wraps')
"
done
