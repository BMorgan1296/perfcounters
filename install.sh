#!/bin/bash

#Install kernel dependencies
echo "Updating kernel tools"
sudo apt-get update
sudo apt-get install build-essential linux-tools-common linux-tools-generic linux-tools-`uname -r` git msr-tools -y
sudo modprobe msr

#Get the user_rdpmc dependency
git submodule init
git submodule update --recursive --remote

echo "Building RDPMC kernel module"
#build the CR4 bit switching kernel module
cd user_rdpmc/
./build.sh

echo "Installing RDPMC kernel module"
# #insert the kernel module
./install.sh

cd ..

./build.sh
