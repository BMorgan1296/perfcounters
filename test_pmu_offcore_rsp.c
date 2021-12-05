#include "perf_counters.h"

#include <sys/mman.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/sysinfo.h>

#define NUM_SAMPLES 10000000

#define MMAP_FLAGS (MAP_PRIVATE | MAP_ANONYMOUS)
#define PAGE_SIZE 4096

// #define L1D 32768
// #define L1_SETS 64
// #define L1_ASSOCIATIVITY 8
// #define L1_CACHELINE 64
// #define L1_STRIDE (L1_CACHELINE * L1_SETS)
// #define L1_CACHE_SET 0 
// #define MEM_ACCESS_OFFSET ((L1_STRIDE) + (L1_CACHE_SET * L1_CACHELINE))

void clflush(void *v, void *v1) 
{
  asm volatile ("clflush 0(%0)": : "r" (v):);
}

/*Average latency for offcore transactions can be determined by using both MSR_OFFCORE_RSP registers. Using two
performance monitoring counters, program the two OFFCORE_RESPONSE event encodings into the corresponding
IA32_PERFEVTSELx MSRs. Count the weighted cycles via MSR_OFFCORE_RSP0 by programming a request type in
MSR_OFFCORE_RSP0.[15:0] and setting MSR_OFFCORE_RSP0.OUTSTANDING[38] to 1, white setting the
remaining bits to 0. Count the number of requests via MSR_OFFCORE_RSP1 by programming the same request
type from MSR_OFFCORE_RSP0 into MSR_OFFCORE_RSP1[bit 15:0], and setting
MSR_OFFCORE_RSP1.ANY_RESPONSE[16] = 1, while setting the remaining bits to 0. The average latency can be
obtained by dividing the value of the IA32_PMCx register that counted weight cycles by the register that counted
requests.*/

int main(int argc, char const *argv[])
{	
	pmu_perfmon_t m;
	uint64_t a = 0;
	uint64_t b = 0;

	size_t len = (size_t)512ULL * PAGE_SIZE;
	uint8_t *mem = mmap(NULL, sizeof(uint8_t) * len, PROT_READ | PROT_WRITE, MMAP_FLAGS, -1, 0);

	#define NUM_CTRS 1
	COUNTER_INFO_T counters[NUM_CTRS] = 
	{ 
		{
			{0xB7, 0x01, 0, "OFFCORE_RESPONSE0"},
			(IA32_PERFEVT_OS | IA32_PERFEVT_USR | IA32_PERFEVT_EN)
		},
	};
	
	//Get number of processors to run the following code on.
	int num_proc = (int)get_nprocs_conf();

	for (int c = 0; c < num_proc; ++c)
	{
		printf("Trying on processor %d\n", c);
		pmu_perfmon_init(&m, c, NUM_SAMPLES, 0, 0, NUM_CTRS, counters);
		pmu_msr_offcore_rspx_set(&m, MSR_OFFCORE_RSP0, 0xAFB7 | MSR_OFFCORE_RSP_SUPL_TYPE_ANY);
		pmu_enable_fixed_and_general_counters(&m);

		printf("ADDR\t");
		pmu_perfmon_print_headers_csv(&m);
		for (uint64_t i = 0; i <= 16; ++i)
		{
			pmu_perfmon_monitor(&m, clflush, mem+(i*64), NULL);
			printf("%04lx\t", i*64);
			pmu_perfmon_print_results_csv(&m);
		}
		pmu_perfmon_destroy(&m);

	}

	munmap(mem, len * sizeof(uint8_t));
	return 0;
}
