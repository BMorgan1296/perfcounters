#include <sys/mman.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "perf_counters.h"

#define NUM_SAMPLES 20000  //10000000

#ifndef MAP_HUGETLB
	#define MAP_HUGETLB 0x40000 /* arch specific */
#endif

#define MMAP_FLAGS (MAP_PRIVATE | MAP_ANONYMOUS)

#define PAGE_SIZE 4096
#define HUGEPAGEBITS 21
#define HUGEPAGESIZE (1<<HUGEPAGEBITS)
#define HUGEPAGEMASK (HUGEPAGESIZE - 1)

#define PAGE_BITS 12
#define SET_BITS 10

#define L1D 32768
#define L1_SETS 64
#define L1_ASSOCIATIVITY 8
#define L1_CACHELINE 64
#define L1_STRIDE (L1_CACHELINE * L1_SETS)
#define L1_CACHE_SET 0 

#define L2 262144
#define L2_SETS 1024
#define L2_ASSOCIATIVITY 4
#define L2_CACHELINE 64
#define L2_STRIDE (L2_CACHELINE * L2_SETS)
#define L2_CACHE_SET 0

#define L3 2097152 // times 4 slices
#define L3_SLICES 4
#define L3_SETS 2048
#define L3_ASSOCIATIVITY 16
#define L3_CACHELINE 64
#define L3_STRIDE (L3_CACHELINE * L3_SETS)
#define L3_CACHE_SET 0

#define MEM_ACCESS_OFFSET ((L3_STRIDE) + (L3_CACHE_SET * L3_CACHELINE))

unsigned int count_bits(uint64_t n)
{
	unsigned int count = 0;
	while (n)
	{

		n &= (n-1) ;
		count++;
	}
	return count;
}

uint64_t vtop(unsigned pid, uint64_t vaddr)
{
	char path[1024];
	sprintf (path, "/proc/%u/pagemap", pid);
	int fd = open (path, O_RDONLY);
	if (fd < 0)
	{
		return -1;
	}

	uint64_t paddr = -1;
	uint64_t index = (vaddr / PAGE_SIZE) * sizeof(paddr);
	if (pread (fd, &paddr, sizeof(paddr), index) != sizeof(paddr))
	{
		return -1;
	}
	close (fd);
	paddr &= 0x7fffffffffffff;
	return (paddr << PAGE_BITS) | (vaddr & (PAGE_SIZE-1));
}

uint64_t ptos(uint64_t paddr, uint64_t bits)
{
	uint64_t ret = 0;
	uint64_t mask[3] = {0x1b5f575440ULL, 0x2eb5faa880ULL, 0x3cccc93100ULL}; // according to Maurice et al.
	switch (bits)
	{
		case 3:
			ret = (ret << 1) | (uint64_t)(count_bits(mask[2] & paddr) % 2);
		case 2:
			ret = (ret << 1) | (uint64_t)(count_bits(mask[1] & paddr) % 2);
		case 1:
			ret = (ret << 1) | (uint64_t)(count_bits(mask[0] & paddr) % 2);
		default:
		break;
	}
	return ret;
}

static inline void clflush(void *v, void *v1) 
{
	asm volatile ("clflush 0(%0)\n": : "r" (v):);
}

static inline void addr_read(void *v, void *v1) 
{
	#pragma gcc unroll 4096
	for (int i = 0; i < 1; ++i)
	{
		asm ("movl %0, %%eax;" : "=r" ( v ));
	}
}

void get_slice_values(int n_addr, int len, uint8_t *mem, uint8_t *slice_map)
{
	uncore_perfmon_t u;
	uint8_t num_cbos = uncore_get_num_cbo();

	CBO_COUNTER_INFO_T cbo_ctrs[4] =
	{		
		{
			{0x34, 0x8F, 0, "UNC_CBO_CACHE_LOOKUP.ANY_MESI"},
			0, (MSR_UNC_CBO_PERFEVT_EN)
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

	uncore_perfmon_init(&u, 0, NUM_SAMPLES, 4, 0, 0, cbo_ctrs, NULL, NULL);

	//printf("addr\tslice\n");
	//printf("addr\t");
	unsigned int pid = (unsigned int)getpid();
	for (uint64_t i = 0; i < n_addr; ++i)
	{
		mem[((i*L3_CACHELINE))] = pid;
		uint64_t pa = vtop(pid, (uint64_t)&mem[((i*L3_CACHELINE))]);

		int fail = 1;
		while(fail)
		{
			fail = 0;
			uncore_perfmon_monitor(&u, clflush, (void *)(mem+((i*L3_CACHELINE))), NULL);
			int found_slice_count = 0;
			for (int s = 0; s < u.num_cbo_ctrs; ++s)
			{
				if(((double)u.results[s].total/(double)NUM_SAMPLES) >= 1.0)
				{
					slice_map[i] = s;
					found_slice_count++;
					//printf("%04ld | 0x%08lx | Px%08lx\t | %d | %lu", i, ((i*L3_CACHELINE)), pa, s, ptos(pa, 2));
					// if(s != ptos(pa, 2))
					// {
					// 	printf(" | WRONG");
					// }
					//putchar('\n');
				}
				if(((double)u.results[s].total/(double)NUM_SAMPLES) >= 2.0)
				{
					fail = 1;
				}
			}
			if(found_slice_count != 1)
			{
				fail = 1;
			}
		}		
		if(fail == 0 && (ptos(pa, 2) != slice_map[i]))
		{
			printf("%ld: %d | %ld | %f | %f | %f | %f | %lu | %lu\n", i, slice_map[i], ptos(pa, 2), ((double)u.results[0].total/(double)NUM_SAMPLES), ((double)u.results[1].total/(double)NUM_SAMPLES), ((double)u.results[2].total/(double)NUM_SAMPLES), ((double)u.results[3].total/(double)NUM_SAMPLES), u.results[slice_map[i]].val_before, u.results[slice_map[i]].val_after);
			exit(1);
		}
	}
	uncore_perfmon_destroy(&u);				//Destroy measurement util
}

void print_slice_values(int n_addr, int seq, int len, uint8_t *mem, uint8_t *slice_map, uint8_t *id_map)
{
	unsigned int pid = (unsigned int)getpid();
	for (uint64_t i = 0; i < n_addr; ++i)
	{
		mem[((i*L3_CACHELINE))] = pid;
		uint64_t pa = vtop(pid, (uint64_t)&mem[((i*L3_CACHELINE))]);

		if(i % seq == 0)
		{
			printf("0x%08lX\t", pa);
		}
		printf("%d", slice_map[i]);
		if(i % 4 == 3)
			putchar(' ');
		if(i % seq == seq-1)
		{
			printf(" | 0x%04X\n", id_map[((i+1)/seq)-1]);
		}
	}
	putchar('\n');
}

#define NUM_ADDRESS 4096
#define SEQ_LEN 64 //length of each sequence
#define NUM_SEQUENCES (NUM_ADDRESS / SEQ_LEN)
#define MAX_ID 512

void generate_ids(uint8_t *slice_map, uint8_t *id_map)
{
	for (int s = 0; s < NUM_SEQUENCES; ++s)
	{		
		//brute force search to find the ID which matches between sequence n and sequence 0
		int fail = 0;
		for (int i = 0; i < MAX_ID; ++i)
		{
			fail = 0;
			for (int a = 0; a < SEQ_LEN; ++a)
			{
				//printf("SEQ %d | ADDR %d | VAL %d | VS | SEQ 0 ADDR %d VAL %d | ID %d\n", s, a, slice_map[((s*SEQ_LEN)+a)], a, slice_map[a^i], i);
				//does the current address in this sequence match the address in seq 0 ^ ID?
				if(slice_map[((s*SEQ_LEN)+a)] != slice_map[a ^ i])
				{
					fail = 1;
					break;
				}
			}
			if(!fail)
			{
				id_map[s] = i;
				break;
			}
		}
	}
}

int does_addr_differ_by_one(void *addr1, void *addr2)
{
	uint64_t c = (uint64_t)addr1 ^ (uint64_t)addr2;

	if(c == 0)
		return 0;
	else if((c & (c-1)) == 0)
		return 1;
	else
		return 0;

}

int main(int argc, char const *argv[])
{
	size_t len = (size_t)2048ULL * PAGE_SIZE * 2;
	uint8_t *mem = mmap(NULL, sizeof(uint8_t) * len, PROT_READ | PROT_WRITE | PROT_EXEC, MMAP_FLAGS, -1, 0);
	uint8_t *slice_map = malloc(len/L3_CACHELINE * sizeof(uint8_t)); //32786 slice to addresses with current len
	uint8_t *id_map = calloc(len/L3_CACHELINE/SEQ_LEN, sizeof(uint8_t)); //512 total sequences


	// //Get slice values from the perf counter library
	get_slice_values(NUM_ADDRESS, len, mem, slice_map);
	// //Print them
	// //Each sequence is made up of the slice value for 64 distinct L3 cache lines.
	// //As in the paper, we need to generate an ID to relate each sequence back to the initial sequence, which will have ID 0x0.
	generate_ids(slice_map, id_map);

	print_slice_values(NUM_ADDRESS, SEQ_LEN, len, mem, slice_map, id_map);	

	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////

	munmap(mem, len * sizeof(uint8_t));
	free(slice_map);
	free(id_map);
	return 0;
}