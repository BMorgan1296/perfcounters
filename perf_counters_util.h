//https://stackoverflow.com/questions/42088515/perf-event-open-how-to-monitoring-multiple-events
//https://github.com/wcohen/libpfm4
//http://perfmon2.sourceforge.net/docs_v4.html
#define _GNU_SOURCE
#include <sched.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <fcntl.h>

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

inline void cpuid(uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
	__asm__ __volatile__ ("cpuid " :
                      "=a" (*eax),
                      "=b" (*ebx),
                      "=c" (*ecx),
                      "=d" (*edx)
                      : "a" (*eax), "c" (*ecx));
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
    int fp;
    uint64_t msr_data = 0;
    char file[128] = {0};

    snprintf(file, 128, "/dev/cpu/%d/msr", affinity);

    fp = open(file, O_RDONLY);
    if(fp < 0)
    {
        fprintf(stderr, "rdmsr(): could not open /dev/cpu/%d/msr, out of range of available processors?\n", affinity);
        exit(1);
    }

    if(pread(fp, &msr_data, sizeof(msr_data), reg) != sizeof(msr_data))
    {
        fprintf(stderr, "rdmsr(): fgets() could not read MSR 0x%x from /dev/cpu/%d/msr, check msr kernel module is inserted\n", reg, affinity);
        exit(1);
    }
    close(fp);
    return msr_data;
}

#endif //PERF_COUNTERS_UTIL_H
