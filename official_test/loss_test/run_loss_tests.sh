#!/bin/bash
# runs every *_loss.satl in this folder -- each one prints 547311173 when no data was lost
cd "$(dirname "$0")"
for test in *_loss.satl; do
    answer=$(/home/madness/.satl/satl "$test" 2> /dev/null)
    echo "$test: $answer (exit $?)"
done
