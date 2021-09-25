# x86 Performance Counter Interface

## Introduction

Provides a C interface for measuring the performance of code. 

Includes a per-core Performance Monitor Unit (PMU) interface as well as an uncore interface.

Built to be highly accurate when measuring a given function.

## Code Samples

Please see `test_pmu.c` and `test_uncore.c` for code samples.

## Installation

Pretty simple. If it doesn't work, please make an issue or send a message. Secure Boot must be turned off to allow a custom kernel module to be installed from my [user_rdpmc](https://github.com/Bmorgan1296/user_rdpmc) repo.

`sudo ./build.sh`

The above installs some dependencies and a custom kernel module, then parses your Intel processor model and type (Core or Xeon) to compile the project properly. This is due to some of the Model Specific Registers (MSRs) changing between processor models, so it accounts for this.