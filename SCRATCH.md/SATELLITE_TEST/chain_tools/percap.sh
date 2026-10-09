#!/bin/bash
# percap.sh [passes] -- each part of the chain ALONE on 547311173 (default 10000 passes), one after another, timed
# through timed.sh into runs/percap/times.txt; uptime before and after
D=$(cd "$(dirname "$0")" && pwd)
P=${1:-10000}
mkdir -p "$D/runs/percap"
cd "$D/runs/percap"
rm -f times.txt
uptime > uptime_before.txt
for p in "01_map map_loss" "02_file file_loss" "03_list list_loss" "04_string string_loss" "05_multiple multiple_loss" \
         "06_infinity infinity_loss" "07_object object_loss" "08_number number_loss" "09_fraction fraction_loss" \
         "10_float float_loss" "11_percent percent_loss" "12_binary binary_loss" "13_hex_operations hex_operations_loss" \
         "14_hex hex_loss" "15_color color_loss" "16_bool bool_loss"; do
    set -- $p
    {
        sed -n '/^satellite.include(satellite)$/,/^arguments.infinity_display(128)$/p' "$D/parts/head.satl"
        echo; cat "$D/parts/00_helpers.satl"; echo; cat "$D/parts/$1.satl"; echo
        echo 'satellite.capsule satellite.main()'; echo '{'
        echo "    satellite.variable.number passes = $P"
        echo "    satellite.console.display($2(547311173, passes))"
        echo '    satellite.return(satellite)'; echo '}'
    } > "$2.satl"
    "$D/timed.sh" "$D/runs/percap" "$2.satl"
done
uptime > uptime_after.txt
echo DONE >> times.txt
