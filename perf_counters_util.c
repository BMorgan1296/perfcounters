#include "perf_counters_util.h"

//Writes into the given Model Specific Register (MSR) using the msr-tools library.
void wrmsr(uint8_t affinity, uint32_t reg, uint64_t contents)
{
	int err;
    char cmd[256] = {0};
    snprintf(cmd, 256, "wrmsr -p %d 0x%08x 0x%016lx", affinity, reg, contents);
    if((err = system(cmd)) != 0)
    {
        fprintf(stderr, "wrmsr(): Could not write to MSR %x, check msr kernel module is inserted\n", reg);
    	perror("wrmsr()");
    	exit(1);
    }
}

void set_cpu(uint8_t affinity)
{
    int num_proc = (int)get_nprocs_conf();;
    if(affinity > num_proc)
    {
        fprintf(stderr, "set_cpu(): Affinity %d greater than available CPU cores (%d)\n", affinity, num_proc);
    }
    cpu_set_t mask;
    int cpu = sched_getcpu();
    if(cpu != affinity)
    {

        CPU_ZERO(&mask);
        CPU_SET(affinity, &mask);
        int result = sched_setaffinity(0, sizeof(mask), &mask);
        if(result == -1)
        {
            perror("main()");
            exit(1);
        }
        cpu = sched_getcpu();
        if(cpu != affinity)
        {
            fprintf(stderr, "set_cpu(): could not change CPU from %d to %d\n", cpu, affinity);
            exit(1);
        }
    }
}

extern inline void cpuid();
extern inline uint64_t rdtscp64();
extern inline void mfence();
extern inline void rdpmc(uint32_t counter, uint64_t *result);
extern inline uint64_t rdmsr(uint8_t affinity, uint32_t reg);
