#!/bin/sh
# SCRATCH.md/FAST_PRINTING/build_race.sh -- builds vte_print_race into build/vte_race/,
# with satl's own compiler and its vendored GTK 4.24 + VTE (the flags come from make,
# which builds nothing here), and satellite's number, float and string sources.
#     sh SCRATCH.md/FAST_PRINTING/build_race.sh
set -eu
root=$(cd "$(dirname "$0")/../.." && pwd)
out="$root/build/vte_race"
mkdir -p "$out/obj"
cd "$root"
cxx=$(make --no-print-directory --eval 'p: ; @echo $(CXX)' p)
gtk_cflags=$(make --no-print-directory --eval 'p: ; @echo $(GTK_CFLAGS)' p)
gtk_libs=$(make --no-print-directory --eval 'p: ; @echo $(GTK_LIBS)' p)
s=satellite
for f in $s/satellite_object/satellite_object.cpp $s/satellite_object/str_add_str.cpp \
         $s/satellite_object/str_minus_str.cpp $s/satellite_object/str_find_str.cpp \
         $s/satellite_object/num_add_num.cpp $s/satellite_object/num_sub_num.cpp \
         $s/satellite_object/num_div_num.cpp $s/satellite_object/object_convert.cpp \
         $s/satellite_object/object_percentage.cpp $s/satellite_object/object_float.cpp \
         $s/satellite_variable_float/float_scaled.cpp $s/satellite_object/object_hexadecimal.cpp \
         $s/satellite_object/object_color.cpp $s/satellite_object/object_fraction.cpp \
         $s/satellite_object/object_lock.cpp $s/satellite_variable_number/satellite_number.cpp \
         $s/satellite_variable_number/satellite_number_divide.cpp \
         $s/satellite_variable_number/satellite_number_text.cpp \
         $s/satellite_variable_number/satellite_number_power.cpp \
         $s/satellite_variable_string/satellite_string.cpp \
         $s/satellite_variable_infinity/satellite_infinity.cpp; do
    "$cxx" -std=c++20 -O2 -I"$root" -c "$f" -o "$out/obj/$(basename "$f" .cpp).o"
done
# $gtk_cflags and $gtk_libs are word lists on purpose: unquoted.
"$cxx" -std=c++20 -O2 -I"$root" $gtk_cflags -c SCRATCH.md/FAST_PRINTING/vte_print_race.cpp -o "$out/obj/race.o"
env -u LD_RUN_PATH "$cxx" -fuse-ld=lld -O2 "$out"/obj/*.o $gtk_libs -o "$out/vte_print_race"
echo "built $out/vte_print_race"
