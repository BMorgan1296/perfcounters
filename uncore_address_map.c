#include "perf_counters.h"
#include <sys/mman.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

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

static inline void clflush(void *v, void *v1) 
{
  asm volatile ("clflush 0(%0)": : "r" (v):);
}

uint64_t rdmsr1(uint8_t affinity, uint32_t reg)
{
    FILE *fp;
    uint8_t msr[8] = {0};
    uint64_t ret = 0;

    fp = fopen("/dev/cpu/0/msr","r");
    fseek( fp, reg, SEEK_SET );
    if(fgets(msr, 8, fp) == NULL)
    {
        fprintf(stderr, "rdmsr(): fgets() could not read from /dev/cpu/%d/msr, check msr kernel module is inserted\n", affinity);
        exit(1);
    }
    for (int i = 0; i < 8; i++)
    {
        ret |= (uint64_t)msr[i] << (i * 8);
    }
    fclose(fp);
    return ret;
}

int main(int argc, char const *argv[])
{
	uncore_perfmon_t u;
	uint64_t a = 0;
	uint64_t b = 0;
	size_t len = (size_t)512ULL * PAGE_SIZE;
	void *mem = mmap(NULL, sizeof(uint8_t) * len, PROT_READ | PROT_WRITE | PROT_EXEC, MMAP_FLAGS, -1, 0);

	uint8_t num_cbos = uncore_get_num_cbo();

	CBO_COUNTER_INFO_T cbo_ctrs[4] =
	{		
		{
			{0x34, 0x8F, 0, "UNC_CBO_CACHE_LOOKUP.ANY_MESI"},
			1, (MSR_UNC_CBO_PERFEVT_EN)
		},
		{
			{0x34, 0x8F, 0, "UNC_CBO_CACHE_LOOKUP.ANY_MESI"},
			1, (MSR_UNC_CBO_PERFEVT_EN)
		},
		{
			{0x34, 0x8F, 0, "UNC_CBO_CACHE_LOOKUP.ANY_MESI"},
			2, (MSR_UNC_CBO_PERFEVT_EN)
		},
		{
			{0x34, 0x8F, 0, "UNC_CBO_CACHE_LOOKUP.ANY_MESI"},
			3, (MSR_UNC_CBO_PERFEVT_EN)
		},
	};

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

	uncore_perfmon_init(&u, 0, NUM_SAMPLES, 1, 0, 1, cbo_ctrs, arb_ctrs, fixed_ctrs);

	printf("addr\t");
	uncore_perfmon_print_headers_csv(&u);
	for (uint64_t i = 0; i <= 64; ++i)
	{
		printf("0x%04lx\t", i*64);
		uncore_perfmon_monitor(&u, clflush, (void *)(mem+(i*64)), NULL);
		uncore_perfmon_print_results_csv(&u);
	}
	
	uncore_perfmon_destroy(&u);				//Destroy measurement util

	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////

	munmap(mem, len * sizeof(uint8_t));
	return 0;
}