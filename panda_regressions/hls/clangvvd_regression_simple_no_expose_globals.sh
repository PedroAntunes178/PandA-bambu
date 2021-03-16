#!/bin/bash
$(dirname $0)/../../etc/scripts/test_panda.py gcc_regression_simple --tool=bambu \
   --args="--configuration-name=CLANGVVD_O0 -O0 -lm --simulate --do-not-expose-globals --experimental-setup=BAMBU --compiler=I386_CLANGVVD" \
   --args="--configuration-name=CLANGVVD_O1 -O1 -lm --simulate --do-not-expose-globals --experimental-setup=BAMBU --compiler=I386_CLANGVVD" \
   --args="--configuration-name=CLANGVVD_O2 -O2 -lm --simulate --do-not-expose-globals --experimental-setup=BAMBU --compiler=I386_CLANGVVD" \
   --args="--configuration-name=CLANGVVD_O3 -O3 -lm --simulate --do-not-expose-globals --experimental-setup=BAMBU --compiler=I386_CLANGVVD" \
   -o output_clangvvd_simple_do_not_expose_globals -b$(dirname $0) --table=output_clangvvd_simple_do_not_expose_globals.tex --name="ClangVVDRegressionSimpleDNEG" $@
exit $?
