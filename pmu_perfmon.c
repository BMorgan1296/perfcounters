#include "pmu_perfmon.h"

void enable_general_counter(uint8_t affinity, int num, COUNTER_INFO_T counter_info)
{
    uint64_t value = 0;
    value = (uint64_t)( (counter_info.counter.cmask << 24) | (counter_info.flags) | (counter_info.counter.umask << 8) | counter_info.counter.event);
    wrmsr(affinity, (IA32_PERFEVTSEL0 + num), value);
}

void enable_fixed_counters(uint8_t affinity, int num, uint8_t flags)
{
    uint64_t value = 0;
    for (int i = 0; i < num; ++i)
    {
        value |= (flags << (i * 4));
    }
    wrmsr(affinity, IA32_FIXED_CTR_CTRL, value);
}

//Initialises counters using wrmsr
void enable_fixed_and_general_counters(pmu_perfmon_t *p)
{
    //Enable usage of the counters    
    wrmsr(p->affinity, IA32_PERF_GLOBAL_CTRL, DISABLE_FIXED_AND_GENERAL);
    wrmsr(p->affinity, IA32_PMC0, CLEAR_FIXED_OR_GENERAL);
    wrmsr(p->affinity, IA32_PMC1, CLEAR_FIXED_OR_GENERAL);
    wrmsr(p->affinity, IA32_PMC2, CLEAR_FIXED_OR_GENERAL);
    wrmsr(p->affinity, IA32_PMC3, CLEAR_FIXED_OR_GENERAL);

    wrmsr(p->affinity, IA32_FIXED_CTR0, CLEAR_FIXED_OR_GENERAL);
    wrmsr(p->affinity, IA32_FIXED_CTR1, CLEAR_FIXED_OR_GENERAL);
    wrmsr(p->affinity, IA32_FIXED_CTR2, CLEAR_FIXED_OR_GENERAL);

    for (int i = 0; i < p->num_ctrs; ++i)
    {        
        enable_general_counter(p->affinity, i, p->counters_info[i]);
    }

    enable_fixed_counters(p->affinity, p->num_fixed_ctrs, p->fixed_ctr_flags);

    wrmsr(p->affinity, IA32_PERF_GLOBAL_CTRL, ENABLE_FIXED_AND_GENERAL);
}


//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// INTERFACE ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

char *pmu_perfmon_get_string_from_ctr(COUNTER_T counter)
{
	char *str;
	int found = 0;
	for (int i = 0; i < PMU_MAX_COUNTER; ++i)
	{
		if (pmu_counters[i].event == counter.event && \
			pmu_counters[i].umask == counter.umask && \
			pmu_counters[i].cmask == counter.cmask && \
			pmu_counters[i].name[0] != 0)
		{
			str = pmu_counters[i].name;
			found = 1;
			break;
		}
	}

	if(found)
		return str;
	else
		return 0;
}

void pmu_perfmon_print_headers_csv(pmu_perfmon_t *p)
{
	for (int i = 0; i < p->num_fixed_ctrs; ++i)
	{
		printf("%s\t", pmu_counters[i].name);
		//printf("%s\tmin\tmax\t", pmu_counters[i].name);
	}
	for (int i = 0; i < p->num_ctrs; ++i)
	{
		char temp[128] = {0};
		char *str;
		if((str = pmu_perfmon_get_string_from_ctr(p->counters_info[i].counter)) == 0)
		{
			snprintf(temp, 13, "RAW_CTR_%04X", (p->counters_info[i].counter.umask << 8 | p->counters_info[i].counter.event));
			str = temp;
		}
		printf("%s\t", str);
		//printf("%s\tmin_%d\tmax_%d\t", str, i, i);
	}
	putchar('\n');	
}

void pmu_perfmon_print_results_csv(pmu_perfmon_t *p)
{
	//Print results
	for (int i = 0; i < p->num_fixed_ctrs + p->num_ctrs; ++i)
		printf("%f\t", (double)p->results[i].total/(double)p->samples);
		//printf("%f\t%lu\t%lu\t", (double)p->results[i].total/(double)p->samples, p->results[i].min, p->results[i].max);
	putchar('\n');
}


void pmu_perfmon_init(pmu_perfmon_t *p, uint8_t affinity, int64_t samples, uint8_t num_fixed_ctrs, uint8_t fixed_ctr_flags, uint8_t num_ctrs, COUNTER_INFO_T *counters_info)
{
	//Init the values
	p->affinity = affinity;
	p->samples = samples;
	p->num_fixed_ctrs = num_fixed_ctrs;
	p->fixed_ctr_flags = fixed_ctr_flags;
	p->num_ctrs = num_ctrs;

	if(p->num_ctrs >= 1)
	{
		//Set the counters	
		p->counters_info = calloc(p->num_ctrs, sizeof(COUNTER_INFO_T));
		for (int i = 0; i < p->num_ctrs; i++)
			p->counters_info[i] = counters_info[i];

		//Measurement vars for averaging each counter
		p->results = calloc((p->num_fixed_ctrs + p->num_ctrs), sizeof(COUNTER_RESULTS_T));
		//init min to max value
		for (int i = 0; i < (p->num_fixed_ctrs + p->num_ctrs); ++i)
			p->results[i].min = -1;

		//Setup perf monitoring
		enable_fixed_and_general_counters(p);
	}
	else
	{
		fprintf(stderr, "pmu_perfmon error: num_ctrs should be greater than 0\n");
		exit(0);
	}
}

void pmu_perfmon_change_samples(pmu_perfmon_t *p, int64_t samples)
{
	if(p->samples > 0)
		p->samples = samples;
	else
	{
		fprintf(stderr, "pmu_perfmon error: samples should be greater than 0");
		exit(1);
	}
}

void pmu_perfmon_destroy(pmu_perfmon_t *p)
{
	free(p->counters_info);
	free(p->results);
}

void pmu_perfmon_monitor(pmu_perfmon_t *p, void (*exe)(uint64_t *, uint64_t *), uint64_t* arg1, uint64_t* arg2)
{
	set_cpu(p->affinity);
	//We want samples to be in a register so that the execution for loop is quick
	register int64_t *reg_samples = &(p->samples);
	register uint8_t *reg_num_fixed_ctrs = &(p->num_fixed_ctrs);
	register uint8_t *reg_num_ctrs = &(p->num_ctrs);

	//Reset values
	for (register int s = 0; s < (*reg_num_fixed_ctrs + *reg_num_ctrs); ++s)
	{
		p->results[s].val_before = 0;
		p->results[s].val_after = 0;
		p->results[s].total = 0;
		p->results[s].min = -1;
		p->results[s].max = 0;
	}

	//warmup of 10 loops to get the counters fresh
	#pragma GCC unroll 4096
	for (int s = 0; s < 1000; ++s)
	{
		exe(arg1, arg2);
	}

	//Read Counters
	#pragma GCC unroll 4096
	for (register int s = 0; s < *reg_num_fixed_ctrs; ++s)
		rdpmc((PCTR_FIXED | s), &p->results[s].val_before);
	#pragma GCC unroll 4096
	for (register int s = 0; s < *reg_num_ctrs; ++s)
		rdpmc((PCTR_GENERAL | s), &p->results[s+*reg_num_fixed_ctrs].val_before);

	//Execute the assembled code
	#pragma GCC unroll 4096
	for (register int s = 0; s < *reg_samples; ++s)
	{
		exe(arg1, arg2);
	}

	//Read after execution	
	#pragma GCC unroll 4096
	for (register int s = 0; s < *reg_num_fixed_ctrs; ++s)
		rdpmc((PCTR_FIXED | s), &p->results[s].val_after);
	#pragma GCC unroll 4096
	for (register int s = 0; s < *reg_num_ctrs; ++s)
		rdpmc((PCTR_GENERAL | s), &p->results[s+*reg_num_fixed_ctrs].val_after);

	//Collect results
	for (register int s = 0; s < (*reg_num_fixed_ctrs + *reg_num_ctrs); ++s)
	{
		p->results[s].total = p->results[s].val_after - p->results[s].val_before;
	}
}

//For if we want to run two different functions back to back e.g. accessing a list forwards then backwards to minimise cache thrashing
void pmu_perfmon_monitor2(pmu_perfmon_t *p, void (*exe1)(uint64_t *, uint64_t *), void (*exe2)(uint64_t *, uint64_t *), uint64_t* arg1, uint64_t* arg2)
{
	set_cpu(p->affinity);
	//We want samples to be in a register so that the execution for loop is quick
	register int64_t *reg_samples = &(p->samples);
	register uint8_t *reg_num_fixed_ctrs = &(p->num_fixed_ctrs);
	register uint8_t *reg_num_ctrs = &(p->num_ctrs);

	//Reset values
	for (register int s = 0; s < (*reg_num_fixed_ctrs + *reg_num_ctrs); ++s)
	{
		p->results[s].val_before = 0;
		p->results[s].val_after = 0;
		p->results[s].total = 0;
		p->results[s].min = -1;
		p->results[s].max = 0;
	}

	//warmup of 10 loops to get the counters fresh
	#pragma GCC unroll 4096
	for (int s = 0; s < 1000; ++s)
	{
		exe1(arg1, arg2);
		exe2(arg1, arg2);
	}

	//Read Counters
	#pragma GCC unroll 4096
	for (register int s = 0; s < *reg_num_fixed_ctrs; ++s)
		rdpmc((PCTR_FIXED | s), &p->results[s].val_before);
	#pragma GCC unroll 4096
	for (register int s = 0; s < *reg_num_ctrs; ++s)
		rdpmc((PCTR_GENERAL | s), &p->results[s+*reg_num_fixed_ctrs].val_before);

	//Execute the assembled code
	#pragma GCC unroll 4096
	for (register int s = 0; s < *reg_samples/2; ++s)
	{
		exe1(arg1, arg2);
		exe2(arg1, arg2);
	}

	//Read after execution	
	#pragma GCC unroll 4096
	for (register int s = 0; s < *reg_num_fixed_ctrs; ++s)
		rdpmc((PCTR_FIXED | s), &p->results[s].val_after);
	#pragma GCC unroll 4096
	for (register int s = 0; s < *reg_num_ctrs; ++s)
		rdpmc((PCTR_GENERAL | s), &p->results[s+*reg_num_fixed_ctrs].val_after);

	//Collect results
	for (register int s = 0; s < (*reg_num_fixed_ctrs + *reg_num_ctrs); ++s)
	{
		p->results[s].total = p->results[s].val_after - p->results[s].val_before;
	}
}