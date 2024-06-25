#include <string.h>
#include "uncore_perfmon.h"

void print_map(uncore_perfmon_t *u)
{
	uint8_t n_cbo = uncore_get_num_cbo(u->affinity);
	for (uint8_t cbo = 0; cbo < n_cbo; cbo++)
	{
		for (int ctr = 0; ctr < CBO_MAX_CTR; ++ctr)
		{
			printf("CBo %d CTR %d Val %d\t", cbo, ctr, u->cbo_ctrs_map[(cbo*CBO_MAX_CTR)+ctr]);
		}
		putchar('\n');
	}
	putchar('\n');
}

void uncore_enable_cbo_counter(uint8_t affinity, uint32_t msr, CBO_COUNTER_INFO_T counter_info)
{
    uint64_t value = 0;
    value = (uint64_t)( (counter_info.counter.cmask << 24) | (counter_info.flags) | (counter_info.counter.umask << 8) | counter_info.counter.event);
    if(counter_info.counter.cmask > 0xF)
    {
    	fprintf(stderr, "uncore_enable_cbo_counter(): CMASK too large (Max: 0xF)\n");
    	exit(1);
    }
    wrmsr(affinity, msr, value);
}

void enable_arb_counter(uint8_t affinity, uint32_t msr, COUNTER_INFO_T counter_info)
{
    uint64_t value = 0;
    value = (uint64_t)( (counter_info.counter.cmask << 24) | (counter_info.flags) | (counter_info.counter.umask << 8) | counter_info.counter.event);
    if(counter_info.counter.cmask > 0xF)
    {
    	fprintf(stderr, "enable_arb_counter(): CMASK too large (Max: 0xF)\n");
    	exit(1);
    }
    wrmsr(affinity, msr, value);
}

void uncore_enable_fixed_counter(uint8_t affinity, COUNTER_INFO_T counter_info)
{
    uint64_t value = 0;
    value = (uint64_t) (counter_info.flags);
    wrmsr(affinity, MSR_UNC_PERF_FIXED_CTRL, value);
}

void uncore_enable_counters(uncore_perfmon_t *u)
{
	//Disable usage of uncore counters, to clear it all
	wrmsr(u->affinity, MSR_UNC_PERF_GLOBAL_CTRL, GLOBAL_CTRL_DISABLE);
	wrmsr(u->affinity, MSR_UNC_PERF_GLOBAL_STATUS, GLOBAL_CTRL_CLEAR);

	//Disable CBos to clear
	uint8_t n_cbo = uncore_get_num_cbo(u->affinity);
	for (int cbo = 0; cbo < n_cbo; ++cbo)
	{
		for (int ctr = 0; ctr < CBO_MAX_CTR; ++ctr)
			wrmsr(u->affinity, MSR_UNC_CBO_PERFEVTSEL(cbo, ctr), MSR_UNC_CTR_DISABLE);
	}

	//Disable ARB to clear
	for (int arb = 0; arb < CBO_MAX_CTR; ++arb)
		wrmsr(u->affinity, MSR_UNC_ARB_PERFEVTSEL(arb), MSR_UNC_CTR_DISABLE);
	//Disable Fixed to clear
	wrmsr(u->affinity, MSR_UNC_PERF_FIXED_CTRL, MSR_UNC_CTR_DISABLE);

	//Enable usage of uncore counters
	wrmsr(u->affinity, MSR_UNC_PERF_GLOBAL_CTRL, GLOBAL_CTRL_EN);
}

//Initialises counters using wrmsr
void uncore_enable_cbo_counters(uncore_perfmon_t *u)
{
	uint8_t n_cbo = uncore_get_num_cbo(u->affinity);
	for (uint8_t cbo = 0; cbo < n_cbo; cbo++)
	{
		for (int ctr = 0; ctr < CBO_MAX_CTR; ++ctr)
		{
			if(u->cbo_ctrs_map[(cbo*CBO_MAX_CTR)+ctr] == 1)
			{
				uncore_enable_cbo_counter(u->affinity, MSR_UNC_CBO_PERFEVTSEL(cbo, ctr), u->cbo_ctrs_info[(cbo*CBO_MAX_CTR)+ctr]);
			}
		}
	}
}

void uncore_enable_arb_counters(uncore_perfmon_t *u)
{
	for (int arb = 0; arb < u->num_arb_ctrs; ++arb)
	{
		enable_arb_counter(u->affinity, MSR_UNC_ARB_PERFEVTSEL(arb), u->arb_ctrs_info[arb]);
	}
}

void uncore_enable_fixed_counters(uncore_perfmon_t *u)
{
	for (int i = 0; i < u->num_fixed_ctrs; ++i)
	{
		uncore_enable_fixed_counter(u->affinity, u->fixed_ctrs_info[i]);
	}
}


//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// INTERFACE ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

char *uncore_perfmon_get_string_from_ctr(COUNTER_T counter)
{
	char *str;
	int found = 0;
	for (int i = 0; i < UNCORE_MAX_COUNTER; ++i)
	{
		if (uncore_counters[i].event == counter.event && \
			uncore_counters[i].umask == counter.umask && \
			uncore_counters[i].cmask == counter.cmask && \
			uncore_counters[i].name[0] != 0)
		{
			str = uncore_counters[i].name;
			found = 1;
			break;
		}
	}

	if(found)
		return str;
	else
		return 0;
}

void uncore_print_string_from_ctr(COUNTER_T counter, int cbo)
{
	char temp[128] = {0};
	char *str;
	if((str = uncore_perfmon_get_string_from_ctr(counter)) == 0)
	{
		snprintf(temp, 13, "RAW_CTR_%04X", (counter.umask << 8 | counter.event));
		str = temp;
	}
	if(cbo >= 0)
		printf("CBo_%d_%s\t", cbo, str);
	else
		printf("%s\t", str);		
	//printf("%s\tmin_%d\tmax_%d\t", str, i, i);
}

void uncore_perfmon_print_headers_csv(uncore_perfmon_t *u)
{
	uint8_t n_cbo = uncore_get_num_cbo(u->affinity);
	if(u->num_cbo_ctrs >= 1)
	{
		for (int i = 0; i < n_cbo * CBO_MAX_CTR; ++i)
		{
			if(u->cbo_ctrs_map[i] == 1)
				uncore_print_string_from_ctr(u->cbo_ctrs_info[i].counter, u->cbo_ctrs_info[i].cbo);
		}	
	}
	if(u->num_arb_ctrs >= 1)
	{
		for (int i = 0; i < u->num_arb_ctrs; ++i)
		{	
			uncore_print_string_from_ctr(u->arb_ctrs_info[i].counter, -1);
		}
	}
	if(u->num_fixed_ctrs >= 1)
	{
		for (int i = 0; i < u->num_fixed_ctrs; ++i)
		{
			uncore_print_string_from_ctr(u->fixed_ctrs_info[i].counter, -1);
		}
	}	
	putchar('\n');	
}

void uncore_perfmon_print_results_csv(uncore_perfmon_t *u)
{
	//Print results
	for (int i = 0; i < u->num_cbo_ctrs + u->num_arb_ctrs + u->num_fixed_ctrs; ++i)
	{
		printf("%f\t", (double)u->results[i].total/(double)u->samples);
		//printf("%f\t%lu\t%lu\t", (double)u->results[i].total/(double)u->samples, u->results[i].min, u->results[i].max);		
	}
	putchar('\n');
}

uint8_t uncore_get_num_cbo(uint8_t affinity)
{
	int res = 0; 
	#if defined(GEN2) || defined(GEN3) || defined(GEN4) || defined(GEN5) || defined(GEN6) || defined(GEN7) || defined(GEN8)
		res = (uint8_t)rdmsr(affinity, MSR_UNC_CBO_CONFIG) - 1;
	#elif defined(GEN9) || defined(GEN10) || defined(GEN11) || defined (GEN12) || defined (GEN13) || defined (GEN14)
		res = (uint8_t)rdmsr(affinity, MSR_UNC_CBO_CONFIG);
		
		//i9 10900K has 10 slices but the MSRs do not allow accessing slices 7,8,9 >:(
		#if defined(GEN10)
			if(res == 0xa)
			{
				res = 7;
			}
		#endif
	#endif

	return res;
}

void uncore_perfmon_init(uncore_perfmon_t *u, 
						 uint8_t affinity, 
						 int64_t samples,
						 uint8_t num_cbo_ctrs,
						 uint8_t num_arb_ctrs,
						 uint8_t num_fixed_ctrs,
						 CBO_COUNTER_INFO_T *cbo_ctrs_info,
						 COUNTER_INFO_T *arb_ctrs_info,
						 COUNTER_INFO_T *fixed_ctrs_info)
{
	//Init the static values
	u->affinity = affinity;
	u->samples = samples;
	u->num_cbo_ctrs = num_cbo_ctrs;
	u->num_arb_ctrs = num_arb_ctrs;
	u->num_fixed_ctrs = num_fixed_ctrs;
	uint32_t total_ctrs = (u->num_cbo_ctrs + u->num_arb_ctrs + u->num_fixed_ctrs);

	if(total_ctrs >= 1)
	{
		if(u->num_cbo_ctrs >= 1)
		{
			uint8_t n_cbo = uncore_get_num_cbo(u->affinity);
			u->cbo_ctrs_map = calloc(n_cbo * CBO_MAX_CTR, sizeof(uint8_t));			
			u->cbo_ctrs_info = calloc(n_cbo * CBO_MAX_CTR, sizeof(CBO_COUNTER_INFO_T));
			//Iterate through user specified counters (uc)
			for (uint8_t uc = 0; uc < u->num_cbo_ctrs; uc++)
			{
				int success = 0;
				//Get the CBo for this counter
				uint8_t uc_cbo = cbo_ctrs_info[uc].cbo;
				//If the CBo specified for this counter is out of range of available CBos
				if(uc_cbo >= n_cbo)
				{
					fprintf(stderr, "uncore_perfmon_init(): could not assign counter to CBo, specified CBo out of range (CBo count: %d)\n", n_cbo);
					exit(1);
				}
				//Now go through each ctr in this CBo
				for (int ctr = 0; ctr < CBO_MAX_CTR; ctr++)
				{
					if(u->cbo_ctrs_map[(uc_cbo*CBO_MAX_CTR)+ctr] == 0)
					{
						//Copy counter to counter map and set map lookup value
						u->cbo_ctrs_info[(uc_cbo*CBO_MAX_CTR)+ctr] = cbo_ctrs_info[uc];
						u->cbo_ctrs_map[(uc_cbo*CBO_MAX_CTR)+ctr] = 1;
						success = 1;
						ctr = CBO_MAX_CTR;
					}
				}
				if(!success)
				{
					fprintf(stderr, "uncore_perfmon_init(): could not assign counter to CBo %d, not enough CBo counter slots (Max: %d)\n", uc_cbo, CBO_MAX_CTR);
					exit(1);
				}
			}
		}		
		if(u->num_arb_ctrs >= 1)
		{
			if(u->num_arb_ctrs > ARB_MAX_CTR)
			{
				fprintf(stderr, "uncore_perfmon_init(): could not assign counter to ARB, not enough ARB counter slots (Max: %d)\n", ARB_MAX_CTR);
				exit(1);
			}
			u->arb_ctrs_info = calloc(u->num_arb_ctrs, sizeof(COUNTER_INFO_T));
			for (int ctr = 0; ctr < u->num_arb_ctrs; ++ctr)
				u->arb_ctrs_info[ctr] = arb_ctrs_info[ctr];
		}
		if(u->num_fixed_ctrs >= 1)
		{
			if(u->num_fixed_ctrs > FIXED_MAX_CTR)
			{
				fprintf(stderr, "uncore_perfmon_init(): could not assign Fixed counter, not enough Fixed counters (Max: %d)\n", FIXED_MAX_CTR);
				exit(1);
			}
			u->fixed_ctrs_info = calloc(u->num_fixed_ctrs, sizeof(COUNTER_INFO_T));
			for (int ctr = 0; ctr < u->num_fixed_ctrs; ++ctr)
				u->fixed_ctrs_info[ctr] = fixed_ctrs_info[ctr];
		}

		//Measurement vars for averaging each counter
		u->results = calloc(total_ctrs, sizeof(COUNTER_RESULTS_T));
		//init min to max value
		for (int i = 0; i < total_ctrs; ++i)
		{
			u->results[i].min = -1;
		}
	}
	else
	{
		fprintf(stderr, "uncore_perfmon_init(): num_ctrs should be greater than 0\n");
		exit(1);
	}
}

void uncore_enable_all_counters(uncore_perfmon_t *u)
{
	uint32_t total_ctrs = (u->num_cbo_ctrs + u->num_arb_ctrs + u->num_fixed_ctrs);

	if(total_ctrs >= 1)
	{
		uncore_enable_counters(u);
		if(u->num_cbo_ctrs >= 1)
			uncore_enable_cbo_counters(u);
		if(u->num_arb_ctrs >= 1)
			uncore_enable_arb_counters(u);
		if(u->num_fixed_ctrs >= 1)
			uncore_enable_fixed_counters(u);
	}
}

void uncore_perfmon_change_samples(uncore_perfmon_t *u, int64_t samples)
{
	if(u->samples > 0)
		u->samples = samples;
	else
	{
		fprintf(stderr, "uncore_perfmon error: samples should be greater than 0");
		exit(1);
	}
}

void uncore_perfmon_destroy(uncore_perfmon_t *u)
{
	if(u->num_cbo_ctrs >= 1)
	{
		free(u->cbo_ctrs_info);
		free(u->cbo_ctrs_map);
	}
	if(u->num_arb_ctrs >= 1)
		free(u->arb_ctrs_info);
	if(u->num_fixed_ctrs >= 1)
		free(u->fixed_ctrs_info);

	free(u->results);
}

#define REG_TOTAL_CTRS (*reg_num_cbo_ctrs + *reg_num_arb_ctrs + *reg_num_fixed_ctrs)

void uncore_perfmon_read_ctrs(uncore_perfmon_t *u)
{
	set_cpu(u->affinity);

	//We want samples to be in a register so that the execution for loop is quick
	uint8_t *reg_affinity = &(u->affinity);
	int64_t *reg_samples = &(u->samples);
	uint8_t *reg_num_cbo_ctrs = &(u->num_cbo_ctrs);
	uint8_t *reg_num_arb_ctrs = &(u->num_arb_ctrs);
	uint8_t *reg_num_fixed_ctrs = &(u->num_fixed_ctrs);

	//Reset values
	for (int s = 0; s < REG_TOTAL_CTRS; ++s)
	{
		u->results[s].val_before = 0;
		u->results[s].val_after = 0;
		u->results[s].total = 0;
		u->results[s].min = -1;
		u->results[s].max = 0;
	}

	for (int s = 0; s < *reg_num_cbo_ctrs; ++s)
		u->results[s].total = rdmsr(*reg_affinity, MSR_UNC_CBO_PERFCTR(u->cbo_ctrs_info->cbo, s % CBO_MAX_CTR));
	for (int s = 0; s < *reg_num_arb_ctrs; ++s)
		u->results[s + *reg_num_cbo_ctrs].total = rdmsr(*reg_affinity, MSR_UNC_ARB_PERFCTR(s));
	for (int s = 0; s < *reg_num_fixed_ctrs; ++s)
		u->results[s + *reg_num_cbo_ctrs + *reg_num_arb_ctrs].total = rdmsr(*reg_affinity, MSR_UNC_PERF_FIXED_CTR);
}

void uncore_perfmon_monitor(uncore_perfmon_t *u, void (*exe)(void *, void *), void* arg1, void* arg2)
{
	uint64_t t1, t2;
	
	set_cpu(u->affinity);

	uint8_t fail = 1;
	uint8_t n_cbo = uncore_get_num_cbo(u->affinity);
	uint8_t *map = u->cbo_ctrs_map;

	//We want samples to be in a register so that the execution for loop is quick
	uint8_t *reg_affinity = &(u->affinity);
	int64_t *reg_samples = &(u->samples);
	uint8_t *reg_num_cbo_ctrs = &(u->num_cbo_ctrs);
	uint8_t *reg_num_arb_ctrs = &(u->num_arb_ctrs);
	uint8_t *reg_num_fixed_ctrs = &(u->num_fixed_ctrs);

	//Incorporated some error checking due to weird values sometimes reported when reading from /dev/cpu/X/msr
	while(fail)
	{
		fail = 0;

		//Reset values
		for (int s = 0; s < REG_TOTAL_CTRS; ++s)
		{
			u->results[s].val_before = 0;
			u->results[s].val_after = 0;
			u->results[s].total = 0;
			u->results[s].min = -1;
			u->results[s].max = 0;
		}
		
		//Read Counters
		for (int s = 0, c = 0; s < n_cbo * CBO_MAX_CTR; ++s)
		{
			if(*reg_num_cbo_ctrs && map[s])
			{
				u->results[c].val_before = rdmsr(*reg_affinity, MSR_UNC_CBO_PERFCTR(u->cbo_ctrs_info[s].cbo, s % CBO_MAX_CTR));
				c++;
			}
		}

		for (int s = 0; s < *reg_num_arb_ctrs; ++s)
		{
			u->results[s + *reg_num_cbo_ctrs].val_before = rdmsr(*reg_affinity, MSR_UNC_ARB_PERFCTR(s));
		}

		for (int s = 0; s < *reg_num_fixed_ctrs; ++s)
			u->results[s + *reg_num_cbo_ctrs + *reg_num_arb_ctrs].val_before = rdmsr(*reg_affinity, MSR_UNC_PERF_FIXED_CTR);

		//Execute the provided function code
		for (int s = 0; s < *reg_samples; ++s)
		{
			exe(arg1, arg2);
		}

		//Read after execution		
		for (int s = 0, c = 0; s < n_cbo * CBO_MAX_CTR; ++s)
		{
			if(*reg_num_cbo_ctrs && map[s])
			{
				u->results[c].val_after = rdmsr(*reg_affinity, MSR_UNC_CBO_PERFCTR(u->cbo_ctrs_info[s].cbo, s % CBO_MAX_CTR));
				c++;
			}
		}

		for (int s = 0; s < *reg_num_arb_ctrs; ++s)
		{
			u->results[s + *reg_num_cbo_ctrs].val_after = rdmsr(*reg_affinity, MSR_UNC_ARB_PERFCTR(s));
		}

		for (int s = 0; s < *reg_num_fixed_ctrs; ++s)
		{
			u->results[s + *reg_num_cbo_ctrs + *reg_num_arb_ctrs].val_after = rdmsr(*reg_affinity, MSR_UNC_PERF_FIXED_CTR);
		}

		//Collect results
		for (int s = 0; s < REG_TOTAL_CTRS; ++s)
		{
			//Error checking. If counter overflowed (current less than initial measurement) or misread (bits greater than 48 are set) then fail.
			u->results[s].total = u->results[s].val_after - u->results[s].val_before;
			if((u->results[s].val_after <= u->results[s].val_before) || ((u->results[s].total >> 48) > 0))
			{
				fail = 1;
			}
		}
	}	
}

