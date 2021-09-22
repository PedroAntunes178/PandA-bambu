#!/bin/bash
COMMONARGS="--simulator=MODELSIM --compiler=I386_GCC49 --std=c99 --experimental-setup=BAMBU -O3 -fno-delete-null-pointer-checks -fopenmp --pragma-parse --mem-delay-read=20 --mem-delay-write=20 --channels-type=MEM_ACC_11 --memory-allocation-policy=NO_BRAM --no-iob --device-name=xc7vx690t-3ffg1930-VVD --clock-period=10 -DMAX_VERTEX_NUMBER=1309073 -DMAX_EDGE_NUMBER=5309056 --max-sim-cycles=2000000000 --simulate"
 NAME=`basename $0 .sh`
`dirname $0`/../../etc/scripts/test_panda.py --spider-style="../lib/latex_format_bambu_results_xilinx.xml" --tool=bambu -l`dirname $0`/list_40DB -ooutput_$NAME --commonargs="$COMMONARGS" \
      --args="--configuration-name=02W-04CH-2C-01CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=4  --channels-number=2 --context_switch=1"\
      --args="--configuration-name=02W-04CH-2C-02CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=4  --channels-number=2 --context_switch=2"\
      --args="--configuration-name=02W-04CH-2C-04CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=4  --channels-number=2 --context_switch=4"\
      --args="--configuration-name=02W-04CH-2C-08CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=4  --channels-number=2 --context_switch=8"\
      --args="--configuration-name=02W-04CH-2C-16CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=4  --channels-number=2 --context_switch=16"\
      --args="--configuration-name=02W-08CH-2C-01CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=8  --channels-number=2 --context_switch=1"\
      --args="--configuration-name=02W-08CH-2C-02CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=8  --channels-number=2 --context_switch=2"\
      --args="--configuration-name=02W-08CH-2C-04CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=8  --channels-number=2 --context_switch=4"\
      --args="--configuration-name=02W-08CH-2C-08CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=8  --channels-number=2 --context_switch=8"\
      --args="--configuration-name=02W-08CH-2C-16CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=8  --channels-number=2 --context_switch=16"\
      --args="--configuration-name=02W-16CH-2C-01CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=16 --channels-number=2 --context_switch=1"\
      --args="--configuration-name=02W-16CH-2C-02CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=16 --channels-number=2 --context_switch=2"\
      --args="--configuration-name=02W-16CH-2C-04CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=16 --channels-number=2 --context_switch=4"\
      --args="--configuration-name=02W-16CH-2C-08CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=16 --channels-number=2 --context_switch=8"\
      --args="--configuration-name=02W-16CH-2C-16CS -DN_THREADS=2  --num-accelerators=2  --memory-banks-number=16 --channels-number=2 --context_switch=16"\
      --args="--configuration-name=04W-04CH-2C-02CS -DN_THREADS=4  --num-accelerators=4  --memory-banks-number=4  --channels-number=2 --context_switch=2"\
      --args="--configuration-name=04W-04CH-2C-04CS -DN_THREADS=4  --num-accelerators=4  --memory-banks-number=4  --channels-number=2 --context_switch=4"\
      --args="--configuration-name=04W-04CH-2C-08CS -DN_THREADS=4  --num-accelerators=4  --memory-banks-number=4  --channels-number=2 --context_switch=8"\
      --args="--configuration-name=04W-04CH-2C-16CS -DN_THREADS=4  --num-accelerators=4  --memory-banks-number=4  --channels-number=2 --context_switch=16"\
      --args="--configuration-name=08W-04CH-2C-02CS -DN_THREADS=8  --num-accelerators=8  --memory-banks-number=4  --channels-number=2 --context_switch=2"\
      --args="--configuration-name=08W-04CH-2C-04CS -DN_THREADS=8  --num-accelerators=8  --memory-banks-number=4  --channels-number=2 --context_switch=4"\
      --args="--configuration-name=08W-04CH-2C-08CS -DN_THREADS=8  --num-accelerators=8  --memory-banks-number=4  --channels-number=2 --context_switch=8"\
      --args="--configuration-name=08W-04CH-2C-16CS -DN_THREADS=8  --num-accelerators=8  --memory-banks-number=4  --channels-number=2 --context_switch=16"\
      --args="--configuration-name=16W-04CH-2C-02CS -DN_THREADS=16 --num-accelerators=16 --memory-banks-number=4  --channels-number=2 --context_switch=2"\
      --args="--configuration-name=16W-04CH-2C-04CS -DN_THREADS=16 --num-accelerators=16 --memory-banks-number=4  --channels-number=2 --context_switch=4"\
      --args="--configuration-name=16W-04CH-2C-08CS -DN_THREADS=16 --num-accelerators=16 --memory-banks-number=4  --channels-number=2 --context_switch=8"\
      --args="--configuration-name=16W-04CH-2C-16CS -DN_THREADS=16 --num-accelerators=16 --memory-banks-number=4  --channels-number=2 --context_switch=16"\
      --args="--configuration-name=32W-04CH-2C-02CS -DN_THREADS=32  --num-accelerators=32  --memory-banks-number=4  --channels-number=2 --context_switch=2"\
      --args="--configuration-name=32W-04CH-2C-04CS -DN_THREADS=32  --num-accelerators=32  --memory-banks-number=4  --channels-number=2 --context_switch=4"\
      --args="--configuration-name=32W-04CH-2C-08CS -DN_THREADS=32  --num-accelerators=32  --memory-banks-number=4  --channels-number=2 --context_switch=8"\
      --args="--configuration-name=32W-04CH-2C-16CS -DN_THREADS=32  --num-accelerators=32  --memory-banks-number=4  --channels-number=2 --context_switch=16"\
      -t1440m --table=$NAME.tex --csv=$NAME.csv --benchmarks_root=`dirname $0` --name="$NAME" "$@"
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
exit 0

