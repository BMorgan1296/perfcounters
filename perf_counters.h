#define _GNU_SOURCE
#include <sched.h>
#include <inttypes.h>

#ifndef PERF_COUNTERS_H
#define PERF_COUNTERS_H

typedef struct counter
{
    uint8_t event;
    uint8_t umask;
    uint16_t cmask;
	char name[128];
} COUNTER_T;

typedef struct counter_info
{
    COUNTER_T counter;
    uint64_t flags;
} COUNTER_INFO_T;

typedef struct counter_results
{
	uint64_t val_before; 	//Counts before execution
	uint64_t val_after; 	//Counts after execution
	uint64_t total; 		//counts total for each event
	uint64_t min; 			//min value found for the event.
	uint64_t max; 			//max value found for the event.
} COUNTER_RESULTS_T;


/////////////////////////////////////
/////// PMU LIBRARY INTERFACE ///////
/////////////////////////////////////
//Flags for fixed counters
#define IA32_FIXED_DISABLE 0x0
#define IA32_FIXED_USR     0x2
#define IA32_FIXED_OS      0x1
#define IA32_FIXED_BOTH    0x3
//Flags for general purpose counters
#define IA32_PERFEVT_USR (uint64_t)0x10000
#define IA32_PERFEVT_OS  (uint64_t)0x20000
#define IA32_PERFEVT_E   (uint64_t)0x40000
#define IA32_PERFEVT_PC  (uint64_t)0x80000
#define IA32_PERFEVT_INT (uint64_t)0x100000
#define IA32_PERFEVT_EN  (uint64_t)0x400000
#define IA32_PERFEVT_INV (uint64_t)0x800000

typedef struct pmu_perfmon
{
	////////////////////////////
	uint8_t affinity;		//processor core to measure counters from.
	int64_t samples; 		//number of samples for the monitor_function
	uint8_t num_ctrs; 		//number of counters requested to monitor. Usually max 4 with hyperthreading, 8 without.
	////////////////////////////
	COUNTER_INFO_T *counters_info; 	//changed by user, are the actual counters for perf to monitor
	uint8_t num_fixed_ctrs;
	uint8_t fixed_ctr_flags;
	////////////////////////////
	COUNTER_RESULTS_T *results; //first in the array are len(num_fixed_ctrs), then general purpose for len(num_ctrs)
} pmu_perfmon_t;

//Raw counter value. See Intel Software Manual, input as hex. Not implemented with library yet.
//To define a counter above, follow the formula below, where event takes up first byte, and umask takes the second.
//Also put the newly defined counter in COUNTER_INFO_T counters[MAX_COUNTER].
#define PMU_RAW_CTR(umask, event) (uint64_t)((umask << 8) | event)

//Flags for controlling events which are counted under which contexts
#define PMU_PERFMON_EXCLUDE_USER 	(1) //excludes user event counting
#define PMU_PERFMON_EXCLUDE_KERNEL 	(2) //excludes kernel event counting
#define PMU_PERFMON_EXCLUDE_HV 		(4) //excludes hypervisor event counting
#define PMU_PERFMON_EXCLUDE_IDLE 	(8) //excludes cpu idle event counting

//OFFCORE LATENCY 10th Gen//
#define MSR_OFFCORE_RSP0 0x1A6
#define MSR_OFFCORE_RSP1 0x1A7
//Request type
#define MSR_OFFCORE_RSP_RQST_TYPE_DEMAND_DATA_RD    (1)
#define MSR_OFFCORE_RSP_RQST_TYPE_DEMAND_RFO        (1<<1)
#define MSR_OFFCORE_RSP_RQST_TYPE_DEMAND_CODE_RD    (1<<2)
#define MSR_OFFCORE_RSP_RQST_TYPE_HWPF_L2_DATA_RD   (1<<4)
#define MSR_OFFCORE_RSP_RQST_TYPE_HWPF_L2_RFO       (1<<5)
#define MSR_OFFCORE_RSP_RQST_TYPE_HWPF_L3           (0x2380)
#define MSR_OFFCORE_RSP_RQST_TYPE_HWPF_L1D_AND_SWPF (1<<10)
#define MSR_OFFCORE_RSP_RQST_TYPE_STREAMING_WR      (1<<11)
#define MSR_OFFCORE_RSP_RQST_TYPE_OTHER             (1<<15)
//Supplier/Response type for the above request types
#define MSR_OFFCORE_RSP_SUPL_TYPE_ANY         (1<<16)
#define MSR_OFFCORE_RSP_SUPL_TYPE_DRAM        (0x184000000)
#define MSR_OFFCORE_RSP_SUPL_TYPE_NON_DRAM    (0x2004000000)
#define MSR_OFFCORE_RSP_SUPL_TYPE_L3_MISS     (0x3FFFC00000)
#define MSR_OFFCORE_RSP_SUPL_TYPE_L3_HIT      (0x1C0000)
//Snoop info fields
#define MSR_OFFCORE_RSP_SNOOP_TYPE_SNOOP_NOT_NEEDED (0x100000000)
#define MSR_OFFCORE_RSP_SNOOP_TYPE_SNOOP_MISS       (0x200000000)
#define MSR_OFFCORE_RSP_SNOOP_TYPE_SNOOP_HIT_NO_FWD (0x400000000)
#define MSR_OFFCORE_RSP_SNOOP_TYPE_SNOOP_HITM       (0x1000000000)
//Outstanding requests (to get cycles). Functionality not implemented on 10-12th Gen.
#define MSR_OFFCORE_RSP_OUTSTANDING_REQUESTS (0x4000000000)

//CREATE MEASUREMENT INSTANCE//
//Init the pmu_perfmon_t once, destroy once.
void pmu_perfmon_init(pmu_perfmon_t *m,
					  uint8_t affinity, 					  
					  int64_t samples, 
					  uint8_t num_fixed_ctrs,
					  uint8_t fixed_ctr_flags,
					  uint8_t num_ctrs,
					  COUNTER_INFO_T *counters_info);
void pmu_perfmon_destroy(pmu_perfmon_t *m);

//Enable all counters as programmed within init()
void pmu_enable_fixed_and_general_counters(pmu_perfmon_t *p);

//Offcore response initialisation
void pmu_msr_offcore_rspx_set(pmu_perfmon_t *p, uint32_t msr_offcore_rspx, uint64_t msr_options);

//ALTER MEASUREMENT//
//Change number of samples to record use when measuring
void pmu_perfmon_change_samples(pmu_perfmon_t *m, int64_t samples);

//UTIL//
//Get the string name for the counter
char *pmu_perfmon_get_string_from_ctr(COUNTER_T counter);

//PRINTING FUNCTIONS//
//Print headers for each counter. Searches and prints human readable name
void pmu_perfmon_print_headers_csv(pmu_perfmon_t *m);
//Print the average results
void pmu_perfmon_print_results_csv(pmu_perfmon_t *m);

//PERF MONITORING FUNCTIONS//
//Measures executions, chained back to back in a for loop
void pmu_perfmon_monitor(pmu_perfmon_t *p, 
	                     void (*exe)(void *, void *), 
	                     void* arg1, 
	                     void* arg2);

void pmu_perfmon_monitor2(pmu_perfmon_t *m,
						 void (*exe1)(uint64_t *, uint64_t *),
					 	 void (*exe2)(uint64_t *,
						 uint64_t *),
					 	 uint64_t* arg1,
					 	 uint64_t* arg2);


/////////////////////////////////////
///// UNCORE LIBRARY INTERFACE //////
/////////////////////////////////////
//Flags for fixed counters
#define MSR_UNC_FIXED_CTRL_CNT_EN 		(uint64_t)(1<<22) //enable
#define MSR_UNC_FIXED_CTRL_CNT_OVF_EN 	(uint64_t)(1<<20) //enable overflow PMI
//Flags for general purpose counters
#define MSR_UNC_CBO_PERFEVT_E 		(uint64_t)(1<<18) //event edge condition/state counting
#define MSR_UNC_CBO_PERFEVT_OVF_EN 	(uint64_t)(1<<20) //enable overflow PMI
#define MSR_UNC_CBO_PERFEVT_EN 		(uint64_t)(1<<22) //enable
#define MSR_UNC_CBO_PERFEVT_INV 	(uint64_t)(1<<23) //invert CMASK/THR counting

#define MSR_UNC_ARB_PERFEVT_E 		(uint64_t)(1<<18) //event edge condition/state counting
#define MSR_UNC_ARB_PERFEVT_OVF_EN 	(uint64_t)(1<<20) //enable overflow PMI
#define MSR_UNC_ARB_PERFEVT_EN 		(uint64_t)(1<<22) //enable
#define MSR_UNC_ARB_PERFEVT_INV 	(uint64_t)(1<<23) //invert CMASK/THR counting

typedef struct cbo_counter_info
{
    COUNTER_T counter;
    uint8_t cbo;
    uint64_t flags;
} CBO_COUNTER_INFO_T;

typedef struct uncore_perfmon
{
	////////////////////////////
	uint8_t affinity;					//processor core to measure uncore counters from. Is agnostic to hyperthreading, i.e. processor 5 will be physical processor 0.
	int64_t samples; 					//number of samples for the monitor_function	 			
	uint8_t num_cbo_ctrs; 				//number of counters requested to monitor. Max 4 CBo. Usually 2 counters per CBo.
	uint8_t num_arb_ctrs; 				//number of counters requested to monitor. Max 1 ARB. Usually 2 counters per ARB.
	uint8_t num_fixed_ctrs;				//number of counters requested to monitor. Usually 1 fixed counter.
	////////////////////////////
	uint8_t *cbo_ctrs_map;
	CBO_COUNTER_INFO_T *cbo_ctrs_info; 	//changed by user, are the actual counters for perf to monitor. Is a map, due to multiple CBos having up to 2 counters each.
	COUNTER_INFO_T *arb_ctrs_info; 		//changed by user, are the actual counters for perf to monitor
	COUNTER_INFO_T *fixed_ctrs_info;	//changed by user, are the actual counters for perf to monitor. Usually only one, but for consistencyt it is an array.
	////////////////////////////
	COUNTER_RESULTS_T *results;
} uncore_perfmon_t;

uint8_t uncore_get_num_cbo(uint8_t affinity); //Returns the number of CBos reported by MSR_UNC_CBO_CONFIG (0x396)

void uncore_perfmon_init(uncore_perfmon_t *u, 
						 uint8_t affinity, 
						 int64_t samples,
						 uint8_t num_cbo_ctrs,
						 uint8_t num_arb_ctrs,
						 uint8_t num_fixed_ctrs,
						 CBO_COUNTER_INFO_T *cbo_ctrs_info,
						 COUNTER_INFO_T *arb_ctrs_info,
						 COUNTER_INFO_T *fixed_ctrs_info);

void uncore_perfmon_destroy(uncore_perfmon_t *u);

void uncore_enable_all_counters(uncore_perfmon_t *u);

void uncore_perfmon_change_samples(uncore_perfmon_t *u, int64_t samples);

char *uncore_perfmon_get_string_from_ctr(COUNTER_T counter);

void uncore_perfmon_print_headers_csv(uncore_perfmon_t *u);

void uncore_perfmon_print_results_csv(uncore_perfmon_t *u);

void uncore_perfmon_monitor(uncore_perfmon_t *u, 
							void (*exe)(void *, void *), 
							void* arg1, 
							void* arg2);

#endif //PERF_COUNTERS_H