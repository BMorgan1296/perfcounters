//https://stackoverflow.com/questions/42088515/perf-event-open-how-to-monitoring-multiple-events
//https://github.com/wcohen/libpfm4
//http://perfmon2.sourceforge.net/docs_v4.html
#define _GNU_SOURCE
#include <sched.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#ifndef PERF_COUNTERS_UTIL_H
#define PERF_COUNTERS_UTIL_H

#ifndef LAST
	#define LAST(k,n) ((k) & ((1<<(n))-1))
	#define MID(k,m,n) LAST((k)>>(m),((n)-(m)))
#endif

//Helper functions
void wrmsr(uint8_t affinity, uint32_t reg, uint64_t contents);
void set_cpu(uint8_t affinity);

void delayloop(uint32_t cycles);

inline void cpuid() 
{
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	asm volatile ("cpuid": "+a" (eax), "+b" (ebx), "+c" (ecx), "+d" (edx));
	return;
}

inline uint64_t rdtscp64() {
  uint32_t low, high;
  asm volatile ("rdtscp": "=a" (low), "=d" (high) :: "ecx");
  return (((uint64_t)high) << 32) | low;
}

inline void mfence()
{
	__asm volatile 
    (
    	"mfence\n"
    );
}

//needs to be serialised...
inline void rdpmc(uint32_t counter, uint64_t *result)
{
    uint32_t lo, hi;
    __asm volatile 
    (
    	"mfence\n"
        "rdpmc\n"
        : "=a" (lo), "=d" (hi)  // outputs
        : "c" (counter)        // inputs
    );
    *result = ((uint64_t)hi << 32) | (uint64_t)lo;
}

inline uint64_t rdmsr(uint8_t affinity, uint32_t reg)
{
    FILE *fp;
    uint8_t msr[8] = {0};
    uint64_t ret = 0;
    char file[128] = {0};

    snprintf(file, 128, "/dev/cpu/%d/msr", affinity);

    fp = fopen(file, "r");
    if(fp == 0)
    {
        fprintf(stderr, "rdmsr(): could not open /dev/cpu/%d/msr, out of range of available processors?\n", affinity);
        exit(1);
    }
    fseek( fp, reg, SEEK_SET );
    if(fgets(msr, 8, fp) == NULL)
    {
        fprintf(stderr, "rdmsr(): fgets() could not read MSR 0x%x from /dev/cpu/%d/msr, check msr kernel module is inserted\n", reg, affinity);
        exit(1);
    }
    for (int i = 0; i < 8; i++)
    {
        ret |= (uint64_t)msr[i] << (i * 8);
    }
    fclose(fp);
    return ret;
}

#endif //PERF_COUNTERS_UTIL_H
