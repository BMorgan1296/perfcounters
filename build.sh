#!/bin/bash

#Install perf 
sudo apt-get update
sudo apt-get install linux-tools-common linux-tools-generic linux-tools-`uname -r`

#Get the user_rdpmc dependency
git submodule init
git submodule update

#build the CR4 bit switching kernel module
cd user_rdpmc/
./build.sh

# #insert the kernel module
./install.sh

cd ..

make