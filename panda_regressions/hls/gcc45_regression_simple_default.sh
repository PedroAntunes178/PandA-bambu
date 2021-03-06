#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py gcc_regression_simple --tool=bambu \
   --args="--configuration-name=GCC45_O0 -O0 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_GCC45" \
   --args="--configuration-name=GCC45_O1 -O1 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_GCC45" \
   --args="--configuration-name=GCC45_O2 -O2 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_GCC45" \
   --args="--configuration-name=GCC45_O3 -O3 -lm --simulate --experimental-setup=BAMBU --expose-globals --compiler=I386_GCC45" \
   -o output_gcc45_simple -b$(dirname $0) --table=output_gcc45_simple.tex --name="Gcc45RegressionSimple" $@
exit $?
