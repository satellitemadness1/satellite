#!/bin/bash
# Seven runs in each order; the fastest time for each side is compared.
cd "$(dirname "$0")/../.."
best_satellite=999999999999999 best_cout=999999999999999
for order in satellite-first cout-first; do
    for run in 1 2 3 4 5 6 7; do
        read satellite cout < <(build/race $order 2>&1 > build/race.out)
        [ "$satellite" -lt "$best_satellite" ] && best_satellite=$satellite
        [ "$cout" -lt "$best_cout" ] && best_cout=$cout
    done
done
rm -f build/race.out
python3 -c "
s, c = $best_satellite, $best_cout
print(f'10,000,000 displays, fastest of 14 runs')
print(f'  satellite-004 (number index -> library): {s:>14,} ns  {s/1e7:6.2f} ns per line')
print(f'  std::cout directly:                      {c:>14,} ns  {c/1e7:6.2f} ns per line')
print(f'  satellite-004 is x{s/c:.3f} of C++  (the bar is x1.05)')
"
