#!/bin/bash
export LD_LIBRARY_PATH=~/pin-2.14-71313-gcc.4.4.7-linux/parsec-3.0/pkgs/libs/hooks/inst/amd64-linux.gcc-serial/lib/

## Modify the following paths appropriately
PARSEC_PATH=/home/user/pin-2.14-71313-gcc.4.4.7-linux/parsec-3.0/
PIN_EXE=/home/user/pin-2.14-71313-gcc.4.4.7-linux/pin.sh
PIN_TOOL=/home/user/pin-2.14-71313-gcc.4.4.7-linux/advcomparch-2015-16-ex2-helpcode/pintool/obj-intel64/cslab_branch_stats.so
CMDS_FILE=/home/user/pin-2.14-71313-gcc.4.4.7-linux/advcomparch-2015-16-ex2-helpcode/cmds_simlarge.txt
outDir="./outputs_stats/"

for BENCH in "$@"; do
	cmd=$(cat ${CMDS_FILE} | grep "$BENCH")
	outFile="${BENCH}.cslab_branch_predictors.out"
	outFile="$outDir/$outFile"
	
	pin_cmd="$PIN_EXE -t $PIN_TOOL -o $outFile -- $cmd"
	time $pin_cmd
done
