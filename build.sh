#!/bin/bash

#Install kernel dependencies
echo "Updating kernel tools"
sudo apt-get update
sudo apt-get install linux-tools-common linux-tools-generic linux-tools-`uname -r`

#Get the user_rdpmc dependency
git submodule init
git submodule update

echo "Building RDPMC kernel module"
#build the CR4 bit switching kernel module
cd user_rdpmc/
./build.sh

echo "Installing RDPMC kernel module"
# #insert the kernel module
./install.sh

cd ..

#Nasty Hardcoding
if [[ $(lscpu | grep -c "Core(TM)") ]]; then
	TYPE="INTEL_CORE"
else
	TYPE="INTEL_XEON"
fi

#even more nasty hardcoding
MODEL=$(lscpu | grep "Model name:" | awk '{print $5}' | awk -F '-' '{print $2}')
MODEL=$(echo ${MODEL:0:2})
GEN="GEN"
if [[ $MODEL -lt 20 ]]; then
	GEN=$GEN$MODEL
else
	GEN=$GEN${MODEL:0:1}
fi

echo $TYPE
echo $GEN
echo "Compiling..."

make OPS="-D$TYPE -D$GEN"