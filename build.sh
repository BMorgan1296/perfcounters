#!/bin/bash

#Nasty Hardcoding
if [[ $(lscpu | grep -c "Intel(R) Core(TM)") ]]; then
	TYPE="INTEL_CORE"
else
	TYPE="INTEL_XEON"
fi

#even more nasty hardcoding
MODEL=$(lscpu | grep "Intel" | awk -F "Intel" '{print $2}' | awk -F '-' '{print $2}')
MODEL=$(echo $MODEL | head -c 2)
echo $MODEL
GEN="GEN"
if [[ $MODEL -ge 20 ]]; then
	MODEL=$(echo $MODEL | head -c 1)
fi
GEN=$GEN$MODEL

echo $TYPE
echo $GEN
echo "Compiling..."

make -j8 all OPS="-D$TYPE -D$GEN"

echo "Test with 'sudo ./test_pmu' and 'sudo ./test_uncore'"