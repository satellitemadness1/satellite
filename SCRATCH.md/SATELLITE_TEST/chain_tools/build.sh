#!/bin/bash
# build.sh [loss-test folder] -- rebuilds loss_chain.satl, satellite.test.loss()'s one program, from the 16 loss tests
# (default: ../fixed16, the tests as they are in official_test/loss_test/; give that folder itself after a loss test
# changes -- it is only read). Each make_NN_*_part.py writes one part from its test by checked replacements, and stops
# the build when its test no longer reads as it expects; hand/ holds the chain's own head and main. Then the parts are
# put together in chain order, the contract is checked, and each part's diff against its test is written to diffs/.
set -e
D=$(cd "$(dirname "$0")" && pwd)
export LOSS_TESTS=$(cd "${1:-$D/../fixed16}" && pwd)
cd "$D"
mkdir -p parts diffs
for g in make_00_helpers.py make_01_map.py make_02_file.py make_03_list.py make_04_string.py make_05_multiple.py \
         make_06_infinity.py make_07_object.py make_08_number.py make_09_fraction.py make_10_float.py \
         make_11_percent.py make_12_13_bits.py make_14_hex.py make_15_color.py make_16_bool.py; do
    python3 "$g"
done
cp hand/head.satl hand/main.satl parts/
ORDER="01_map:map 02_file:file 03_list:list 04_string:string 05_multiple:multiple 06_infinity:infinity 07_object:object
       08_number:number 09_fraction:fraction 10_float:float 11_percent:percent 12_binary:binary
       13_hex_operations:hex_operations 14_hex:hex 15_color:color 16_bool:bool"
{
    cat parts/head.satl; echo
    cat parts/00_helpers.satl; echo
    for p in $ORDER; do cat "parts/${p%%:*}.satl"; echo; done
    cat parts/main.satl
} > loss_chain.satl
python3 check_contract.py loss_chain.satl
for p in $ORDER; do
    diff -u "$LOSS_TESTS/${p##*:}_loss.satl" "parts/${p%%:*}.satl" > "diffs/${p%%:*}.diff" || true
done
echo "diffs/: $(ls diffs | wc -l) parts against their tests"
