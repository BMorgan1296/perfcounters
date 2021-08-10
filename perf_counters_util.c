#include "perf_counters_util.h"

//Writes into the given Model Specific Register (MSR) using the msr-tools library.
void wrmsr(uint32_t reg, uint64_t contents)
{
	int err;
    char cmd[256] = {0};
    snprintf(cmd, 256, "wrmsr -a 0x%08x 0x%016lx", reg, contents);
    if((err = system(cmd)) != 0)
    {
    	perror("wrmsr()");
    	exit(1);
    }
}

extern inline void cpuid();
extern inline uint64_t rdtscp64();
extern inline void mfence();
extern inline void rdpmc(uint32_t counter, uint64_t *result);
extern inline uint64_t rdmsr(uint8_t affinity, uint32_t reg);