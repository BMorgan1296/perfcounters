# x86_64 Performance Counter Interface

## Introduction

Provides a C interface for measuring the performance of code on Intel processors.

Includes a per-core Performance Monitor Unit (PMU) interface as well as an uncore interface.

Built to be highly accurate when measuring a given function.

## Code Samples

Please see `test_pmu.c` and `test_uncore.c` for code samples.

Performance counters are defined in `pmu_perfmon.h` and `uncore_perfmon.h`

## Installation and Building

Pretty simple. If it doesn't work, please make an issue or send a message. Secure Boot must be turned off to allow a custom kernel module to be installed from my forked [user_rdpmc](https://github.com/Bmorgan1296/user_rdpmc) repo.

`sudo ./install.sh`

The above installs some dependencies and a custom kernel module, then parses your Intel processor model and type (Core or Xeon) to compile the project properly. This is due to some of the Model Specific Registers (MSRs) changing between processor models, so it accounts for this.

If you only want to compile the interface after all the dependencies have been installed, then:

`sudo ./build.sh`

## To Do
Xeon processors haven't been implemented as I don't have one. TBA!