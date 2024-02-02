#include "perf_counters.h"

#define _GNU_SOURCE
#include <sched.h>

#include <sys/mman.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#define NUM_SAMPLES 100000
#define NUM_CTRS 4
#define NUM_FIXED_CTRS 0
#define FIXED_CTR_FLAGS IA32_FIXED_BOTH

#ifndef MAP_HUGETLB
	#define MAP_HUGETLB 0x40000 /* arch specific */
#endif

#define MMAP_FLAGS (MAP_PRIVATE | MAP_ANONYMOUS)
#define HUGEPAGEBITS 21
#define PAGE_SIZE (1<<HUGEPAGEBITS)

//Change these values to suit your L1D cache.
#define L1D 49152
#define L1D_SETS 64
#define L1D_ASSOCIATIVITY 12 //we are trying to find this
#define CACHELINE 64
#define L1D_STRIDE (CACHELINE * L1D_SETS)

#define L2D 1310720
#define L2_SETS 2048
#define L2_ASSOCIATIVITY 10 //we are trying to find this
#define L2_CACHELINE 64
#define L2_STRIDE (L2_CACHELINE * L2_SETS)

#define CACHE_SET 0 //Set 0 will be accessed

#define AFFINITY 0

inline int memaccess(void *v)
{
    int rv = 0;
    asm volatile("mov (%1), %0": "+r" (rv): "r" (v):);
    return rv;
}

inline void clflush(void *p)
{
    asm volatile("clflush 0(%0)" : : "c"(p) : "rax");
}

inline void serializing_cpuid()
{
    asm volatile (
    "xor %%eax, %%eax\n"
    "cpuid"
    : : : "eax", "ebx", "ecx", "edx");
}

//Access set 0 in l1 num_ways times and then look at perf counters
void cache_way_finder(void *buf, void *num_ways)
{
	//put num_ways and i into register
	register uint64_t *p = num_ways; 
	for (register uint64_t i = 0; i < *p; ++i)
	{
		*(uint64_t *)(buf+(L2_STRIDE*i)) += 1; //read value and write to it

		//memaccess((void *)(buf + (L2_STRIDE*i)));
		asm volatile("mfence\nlfence\n");
	}
	return;
}

int main(int argc, char const *argv[])
{
	//Setting scheduling to only run on a single core.
	cpu_set_t mask;			
	CPU_ZERO(&mask);
	CPU_SET(AFFINITY, &mask);
	int result = sched_setaffinity(AFFINITY, sizeof(mask), &mask);
	if(result == -1)
	{
		perror("get_slice_values_adj()");
		exit(1);
	}
	
	pmu_perfmon_t m;
	uint64_t a = 0;
	uint64_t b = 0;
	size_t len = (size_t)512ULL * PAGE_SIZE; //512 normal 4kb pages
	uint8_t *mem = mmap(NULL, sizeof(uint8_t) * len, PROT_READ | PROT_WRITE, MMAP_FLAGS, -1, 0);

	//Creating a linked list for L1 evictions
	void *curr_address = 0;
	void *next_address = 0;
	for (int addr_offset = 0; addr_offset < L1D_STRIDE; addr_offset += CACHELINE)
	{
	    curr_address = mem + (addr_offset % L1D_STRIDE);            
	    for (int i = 0; i < L1D_ASSOCIATIVITY * 100; ++i)
	    {
	       next_address = (void *)((uintptr_t)mem + ((i+1) * L1D_STRIDE) + (addr_offset % L1D_STRIDE));
	       *((void **)curr_address) = next_address;

	       curr_address = next_address;
	    }
	}
	*((void **)curr_address) = NULL;

	/////////////////////////////////////////////////////////
	//////////////// Pay Attention To Below /////////////////
	/////////////////////////////////////////////////////////

	COUNTER_INFO_T counters[NUM_CTRS] = 
	{
		{
			{0xD1, 0x01, 0, "MEM_LOAD_RETIRED.L1_HIT"},
			(IA32_PERFEVT_OS | IA32_PERFEVT_USR | IA32_PERFEVT_EN)
		},
		{
			{0xD1, 0x02, 0, "MEM_LOAD_RETIRED.L2_HIT"},
			(IA32_PERFEVT_OS | IA32_PERFEVT_USR | IA32_PERFEVT_EN)
		},
		{
			{0x25, 0x1F, 0, "L2_LINES_IN.ALL"},
			(IA32_PERFEVT_OS | IA32_PERFEVT_USR | IA32_PERFEVT_EN)
		},
		{
			{0x24, 0xFF, 0, "L2_RQSTS.REFERENCES"},
			(IA32_PERFEVT_OS | IA32_PERFEVT_USR | IA32_PERFEVT_EN)
		},
	};

	pmu_perfmon_init(&m, AFFINITY, NUM_SAMPLES, NUM_FIXED_CTRS, FIXED_CTR_FLAGS, NUM_CTRS, counters);
	pmu_enable_fixed_and_general_counters(&m);
	printf("L2_STRIDE_ACCESSES\t");
	pmu_perfmon_print_headers_csv(&m); //Print the headers as csv (but with tabs)
	for (uint64_t i = 0; i <= 64; ++i)
	{
		pmu_perfmon_monitor(&m, cache_way_finder, (uint64_t*)mem, &i);
		printf("%lu\t", i);
		pmu_perfmon_print_results_csv(&m);	
	}
	
	pmu_perfmon_destroy(&m);				//Destroy measurement util

	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////

	munmap(mem, len * sizeof(uint8_t));
	return 0;
}