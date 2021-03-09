#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
$dir_script/../../etc/scripts/test_panda.py --tool=bambu \
   --args="--configuration-name=GCC7_O2 -O2 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_GCC7 --discrepancy" \
   -ldiscrepancy_list -o output_discrepancy_test1 \
   -b$dir_script --table=output_discrepancy1.tex --name="Discrepancy" $@
exit $?
