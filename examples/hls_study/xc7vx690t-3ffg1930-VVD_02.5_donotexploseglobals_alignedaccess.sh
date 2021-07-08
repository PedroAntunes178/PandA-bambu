#!/bin/bash
script=$(readlink -e $0)
root_dir=$(dirname $script)
ARGS="--simulator=MODELSIM --compiler=I386_GCC49 --evaluation --no-iob --device=xc7vx690t,-3,ffg1930,VVD --print-dot --do-not-expose-globals --aligned-access --clock-period=2.5  --speculative-sdc-scheduling -fno-delete-null-pointer-checks  --panda-parameter=simple-benchmark-name=1"
NAME=$(basename $0 .sh)
DIRNAME=${root_dir##*/}
$root_dir/../../etc/scripts/test_panda.py --spider-style="$root_dir/latex_format_bambu_results_xilinx_synth.xml" --tool=bambu -l$root_dir/list-VVD-def --args="$ARGS" -t180m --benchmarks_root=$root_dir -ooutput_${DIRNAME}_$NAME --name=${DIRNAME}_$NAME --table=${DIRNAME}_$NAME.tex "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0

