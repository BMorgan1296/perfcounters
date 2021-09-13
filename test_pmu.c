#include "perf_counters.h"

#define _GNU_SOURCE
#include <sched.h>

#include <sys/mman.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#define NUM_SAMPLES 10000000
#define NUM_CTRS 1
#define NUM_FIXED_CTRS 3
#define FIXED_CTR_FLAGS IA32_FIXED_BOTH

#ifndef MAP_HUGETLB
	#define MAP_HUGETLB 0x40000 /* arch specific */
#endif

#define MMAP_FLAGS (MAP_PRIVATE | MAP_ANONYMOUS)
#define HUGEPAGEBITS 21
#define PAGE_SIZE (1<<HUGEPAGEBITS)

//Change these values to suit your L1D cache.
#define L1D 32768
#define L1_SETS 64
#define L1_ASSOCIATIVITY 8 //we are trying to find this
#define L1_CACHELINE 64
#define L1_STRIDE (L1_CACHELINE * L1_SETS)

#define L1_CACHE_SET 0 //Set 0 will be accessed

#define AFFINITY 0

//This will access the same set of the L1 cache - Set 0.
#define MEM_ACCESS_OFFSET ((L1_STRIDE) + (L1_CACHE_SET * L1_CACHELINE))

//Access set 0 in l1 num_ways times and then look at perf counters
void cache_way_finder(uint64_t *buf, uint64_t *num_ways)
{
	//put num_ways and i into register
	register uint64_t *p = num_ways; 
	for (register uint64_t i = 0; i < *p; ++i)
	{
		buf[MEM_ACCESS_OFFSET*i] += 1; //read value and write to it
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
	uint8_t *mem = mmap(NULL, sizeof(uint8_t) * len, PROT_READ | PROT_WRITE | PROT_EXEC, MMAP_FLAGS, -1, 0);

	/////////////////////////////////////////////////////////
	//////////////// Pay Attention To Below /////////////////
	/////////////////////////////////////////////////////////

	COUNTER_INFO_T counters[NUM_CTRS] = 
	{ 
		{
			{0xF1, 0x1F, 0, "L2_LINES_IN.ALL"},
			(IA32_PERFEVT_OS | IA32_PERFEVT_USR | IA32_PERFEVT_EN)
		},
	};

	pmu_perfmon_init(&m, NUM_SAMPLES, NUM_FIXED_CTRS, FIXED_CTR_FLAGS, NUM_CTRS, counters);
	printf("L1_ACCESSES\t");
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