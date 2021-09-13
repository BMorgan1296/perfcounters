#include "perf_counters.h"

#include <sys/mman.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define NUM_SAMPLES 10000000

#define MMAP_FLAGS (MAP_PRIVATE | MAP_ANONYMOUS)
#define PAGE_SIZE 4096

#define L1D 32768
#define L1_SETS 64
#define L1_ASSOCIATIVITY 8
#define L1_CACHELINE 64
#define L1_STRIDE (L1_CACHELINE * L1_SETS)
#define L1_CACHE_SET 0 
#define MEM_ACCESS_OFFSET ((L1_STRIDE) + (L1_CACHE_SET * L1_CACHELINE))

void cache_way_finder(void *buf, void *num_ways)
{
	register uint64_t *p = num_ways; 
	for (register uint64_t i = 0; i < *p; ++i)
	{
		((uint64_t*)buf)[MEM_ACCESS_OFFSET*i] += 1;
	}
	return;
}

void clflush(void *v, void *v1) 
{
  asm volatile ("clflush 0(%0)": : "r" (v):);
}

int main(int argc, char const *argv[])
{
	uncore_perfmon_t u;
	uint64_t a = 0;
	uint64_t b = 0;
	size_t len = (size_t)512ULL * PAGE_SIZE;
	uint8_t *mem = mmap(NULL, sizeof(uint8_t) * len, PROT_READ | PROT_WRITE | PROT_EXEC, MMAP_FLAGS, -1, 0);


	COUNTER_INFO_T arb_ctrs[2] = 
	{
		{
			{0x80, 0x01, 0, "UNC_ARB_TRK_OCCUPANCY.ALL"},
			(MSR_UNC_CBO_PERFEVT_EN)
		},
		{
			{0x80, 0x01, 0, "UNC_ARB_TRK_OCCUPANCY.ALL"},
			(MSR_UNC_CBO_PERFEVT_EN)
		},
	};

	COUNTER_INFO_T fixed_ctrs[1] = 
	{
		{
			{0x00, 0x01, 0, "UNC_CLOCK_SOCKET"},
			(MSR_UNC_FIXED_CTRL_CNT_EN)
		},
	};
	
	//Get number of processors to run the following code on.
	int num_proc = (int)sysconf(_SC_NPROCESSORS_ONLN);

	for (int c = 0; c < num_proc; ++c)
	{
		printf("Trying on processor %d\n", c);

		uint8_t num_cbos = uncore_get_num_cbo(c);

		CBO_COUNTER_INFO_T *cbo_ctrs = malloc(num_cbos * sizeof(CBO_COUNTER_INFO_T));

		for (int i = 0; i < num_cbos; ++i)
		{
			COUNTER_T temp = {0x34, 0x8F, 0, "UNC_CBO_CACHE_LOOKUP.ANY_MESI"};
			cbo_ctrs[i].counter = temp;
			cbo_ctrs[i].cbo = i;
			cbo_ctrs[i].flags = (MSR_UNC_CBO_PERFEVT_EN);
		}

		uncore_perfmon_init(&u, c, NUM_SAMPLES, num_cbos, 0, 1, cbo_ctrs, arb_ctrs, fixed_ctrs);

		printf("ADDR\t");
		uncore_perfmon_print_headers_csv(&u);
		for (uint64_t i = 0; i <= 16; ++i)
		{
			//uncore_perfmon_monitor(&u, cache_way_finder, (uint64_t*)mem, &i);
			uncore_perfmon_monitor(&u, clflush, mem+(i*64), NULL);
			printf("%04lx\t", i*64);
			uncore_perfmon_print_results_csv(&u);
		}
		
		uncore_perfmon_destroy(&u);				//Destroy measurement util
		free(cbo_ctrs);
	}

	munmap(mem, len * sizeof(uint8_t));
	return 0;
}
