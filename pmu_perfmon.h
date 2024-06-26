#include "perf_counters.h"
#include "perf_counters_util.h"

#ifndef PMU_PERFMON_H
#define PMU_PERFMON_H

#define PCTR_FIXED (1 << 30)
#define PCTR_GENERAL (0 << 30)

//Enabling registers (to allow reading and writing in the first place)
#define IA32_FIXED_CTR_CTRL   0x38D
#define IA32_PERF_GLOBAL_CTRL 0x38F

//General purpose counters (to read from)
#define IA32_PMC0 0xC1
#define IA32_PMC1 0xC2
#define IA32_PMC2 0xC3
#define IA32_PMC3 0xC4

//Fixed counters (to read from)
#define IA32_FIXED_CTR0 0x309
#define IA32_FIXED_CTR1 0x30a
#define IA32_FIXED_CTR2 0x30b

//Event selector registers
/*64-bit register Layout (bit inclusive either side) - see Intel Soft Dev Manual 19.13.1.1 Architectural Performance Monitoring Version 1 Facilities
0-7: Event Select - see Intel Soft Dev Manual performance counter table
8-15: Unit Mask (UMASK) - see Intel Soft Dev Manual performance counter table
16: USR - User Mode - Only counts when operating at privilege 1 2 or 3, can be used with OS flag
17: OS - Operating System Mode - Only counts when privilege 0
18: E - Edge Detect - edge detection of micro arch condition, i.e. count fraction of time spent in a certain state such as waiting for interrupt to be serviced.
19: PC - Pin Control - For detecting overflows through a PMi (i is counter num) being deasserted
20: INT - APIC Interrupt Enable - Exception through APIC when counter overflows
21: RESERVED
22: EN - Enable Counter - Enable the counter, but this must be 0 before writing to IA32_PMCx
23: INV - Invert Counter Mask: inverts the result of applying the CMASK. For instance, if CMASK counts an event when a buffer has greater than 6 items, then you can invert and get when less than 6.
24-31: CMASK - Counter Mask: If the number of events counted are greater than or equal to the CMASK, then the counter is incremented by one. Therefore, can handle multipel occurences of the same event per cycle.
32-63: RESERVED
*/
#define IA32_PERFEVTSEL0 0x186
#define IA32_PERFEVTSEL1 0x187
#define IA32_PERFEVTSEL2 0x188
#define IA32_PERFEVTSEL3 0x189

#define ENABLE_FIXED_AND_GENERAL  (uint64_t)0x000000070000000f
#define DISABLE_FIXED_AND_GENERAL (uint64_t)0x0000000000000000
#define CLEAR_FIXED_OR_GENERAL    (uint64_t)0x0000000000000000

//DEFINE NEW COUNTERS HERE//
#define PMU_MAX_COUNTER 1024
#ifdef INTEL_CORE
	#if defined(GEN10) || defined(GEN11) || defined (GEN12) || defined (GEN13) || defined (GEN14)
		COUNTER_T pmu_counters[PMU_MAX_COUNTER] = 
		{
			{0, 0, 0, "INST_RETIRED.ANY"}, //IA32_PERF_FIXED_CTR0
			{0, 0, 0, "CPU_CLK_UNHALTED.THREAD"}, //IA32_PERF_FIXED_CTR1: CPU_CLK_UNHALTED.THREAD or CPU_CLK_UNHALTED.CORE or CPU_CLK_UNHALTED.THREAD_ANY, depending on AnyThread=0|1
			{0, 0, 0, "CPU_CLK_UNHALTED.REF_TSC"}, //IA32_PERF_FIXED_CTR2
			{0x48, 0x01, 0, "L1D_PEND_MISS.PENDING"},
			{0x48, 0x01, 1, "L1D_PEND_MISS.PENDING_CYCLES"},
			{0x48, 0x02, 0, "L1D_PEND_MISS.FB_FULL"},
			{0x51, 0x01, 0, "L1D.REPLACEMENT"},
			{0x51, 0x20, 0, "L1D.HWPF_MISS"},
			{0x24, 0x21, 0, "L2_RQSTS.DEMAND_DATA_RD_MISS"},
			{0x24, 0xC1, 0, "L2_RQSTS.DEMAND_DATA_RD_HIT"},
			{0x24, 0xE1, 0, "L2_RQSTS.ALL_DEMAND_DATA_RD"},
			{0x24, 0x22, 0, "L2_RQSTS.RFO_MISS"},
			{0x24, 0xC2, 0, "L2_RQSTS.RFO_HIT"},
			{0x24, 0xE2, 0, "L2_RQSTS.ALL_RFO"},
			{0x24, 0x24, 0, "L2_RQSTS.CODE_RD_MISS"},
			{0x24, 0xC4, 0, "L2_RQSTS.CODE_RD_HIT"},
			{0x24, 0x38, 0, "L2_RQSTS.PF_MISS"},
			{0x24, 0x3F, 0, "L2_RQSTS.MISS"},
			{0x24, 0x27, 0, "L2_RQSTS.ALL_DEMAND_MISS"},
			{0x24, 0xE4, 0, "L2_RQSTS.ALL_CODE_RD"},
			{0x24, 0xC1, 0, "L2_RQSTS.ALL_CODE_RD_HIT"},
			{0x24, 0x21, 0, "L2_RQSTS.ALL_CODE_RD_MISS"},
			{0x24, 0xF0, 0, "L2_RQSTS.ALL_HWPF"},
			{0x24, 0x30, 0, "L2_RQSTS.HWPF_MISS"},
			{0x24, 0xFF, 0, "L2_RQSTS.REFERENCES"},
			{0x24, 0xC8, 0, "L2_RQSTS.SWPF_HIT"},
			{0x24, 0x28, 0, "L2_RQSTS.SWPF_MISS"},
			{0x25, 0x1F, 0, "L2_LINES_IN.ALL"},
			{0x26, 0x04, 0, "L2_LINES_OUT.USELESS_HWPF"},
			{0x20, 0x08, 0, "OFFCORE_REQUESTS_OUTSTANDING.DATA_RD"},
			{0x20, 0x08, 1, "OFFCORE_REQUESTS_OUTSTANDING.CYCLES_WITH_DATA_RD"},
			{0x20, 0x04, 0, "OFFCORE_REQUESTS_OUTSTANDING.DEMAND_RFO"},
			{0x20, 0x04, 1, "OFFCORE_REQUESTS_OUTSTANDING.CYCLES_WITH_DEMAND_RFO"},
			{0x21, 0x01, 0, "OFFCORE_REQUESTS.DEMAND_DATA_RD"},
			{0x21, 0x10, 0, "OFFCORE_REQUESTS.L3_MISS_DEMAND_DATA_RD"},
			{0x21, 0x10, 0, "OFFCORE_REQUESTS.DATA_RD"},
			{0x21, 0x08, 0, "OFFCORE_REQUESTS.ALL_REQUESTS"},
			{0xA3, 0x08, 8, "CYCLE_ACTIVITY.CYCLES_L1D_MISS"},
			{0xA3, 0x01, 1, "CYCLE_ACTIVITY.CYCLES_L2_MISS"},
			{0xA3, 0x10, 0x10, "CYCLE_ACTIVITY.CYCLES_MEM_ANY"},
			{0xA3, 0x0C, 0, "CYCLE_ACTIVITY.STALLS_L1D_MISS"},
			{0xA3, 0x05, 5, "CYCLE_ACTIVITY.STALLS_L2_MISS"},
			{0xA3, 0x04, 4, "CYCLE_ACTIVITY.STALLS_TOTAL"},
			{0xA3, 0x06, 6, "CYCLE_ACTIVITY.STALLS_L3_MISS"},
			{0xA2, 0x08, 0, "RESOURCE_STALLS.SB"},

			// {0xA6, 0x01, 0, "EXE_ACTIVITY.EXE_BOUND_0_PORTS"},
			// {0xA6, 0x02, 0, "EXE_ACTIVITY.1_PORTS_UTIL"},
			// {0xA6, 0x04, 0, "EXE_ACTIVITY.2_PORTS_UTIL"},
			// {0xA6, 0x08, 0, "EXE_ACTIVITY.3_PORTS_UTIL"},
			// {0xA6, 0x10, 0, "EXE_ACTIVITY.4_PORTS_UTIL"},
			// {0xA6, 0x40, 0, "EXE_ACTIVITY.BOUND_ON_STORES"},
			// {0x28, 0x40, 0, "CORE_POWER.THROTTLE"},
			// {0xC6, 0x01, 0, "FRONTEND_RETIRED_MISS"}, //multiple of these with same event and umask but there is no CMASK?
			// {0xCB, 0x01, 0, "HW_INTERRUPTS_RECEIVED"},
			// {0xC3, 0x01, 0, "MACHINE_CLEARS.COUNT"},
			// {0xC5, 0x00, 0, "BR_MISP_RETIRED.ALL_BRANCHES"},
			// {0xD0, 0x11, 0, "MEM_INST_RETIRED.STLB_MISS_LOADS"},
			// {0xD0, 0x12, 0, "MEM_INST_RETIRED.STLB_MISS_STORES"},
			// {0xD0, 0x21, 0, "MEM_INST_RETIRED.LOCK_LOADS"},
			// {0xD0, 0x81, 0, "MEM_INST_RETIRED.ALL_LOAD"},
			// {0xD0, 0x82, 0, "MEM_INST_RETIRED.ALL_STORES"},
			{0xD1, 0x01, 0, "MEM_LOAD_RETIRED.L1_HIT"},
			{0xD1, 0x02, 0, "MEM_LOAD_RETIRED.L2_HIT"},
			{0xD1, 0x04, 0, "MEM_LOAD_RETIRED.L3_HIT"},
			{0xD1, 0x08, 0, "MEM_LOAD_RETIRED.L1_MISS"},
			{0xD1, 0x10, 0, "MEM_LOAD_RETIRED.L2_MISS"},
			{0xD1, 0x20, 0, "MEM_LOAD_RETIRED.L3_MISS"},
			{0xD1, 0x40, 0, "MEM_LOAD_RETIRED.FB_HIT"},
			{0xD2, 0x01, 0, "MEM_LOAD_L3_HIT_RETIRED.XSNP_MISS"},
			{0xD2, 0x02, 0, "MEM_LOAD_L3_HIT_RETIRED.XSNP_HIT"},
			{0xD2, 0x04, 0, "MEM_LOAD_L3_HIT_RETIRED.XSNP_HITM"},
			{0xD2, 0x08, 0, "MEM_LOAD_L3_HIT_RETIRED.XSNP_NONE"},
			{0xD2, 0x04, 0, "MEM_LOAD_L3_HIT_RETIRED.XSNP_FWD"},
			{0xD3, 0x01, 0, "MEM_LOAD_L3_MISS_RETIRED.LOCAL_DRAM"},
			// {0xD3, 0x02, 0, "MEM_LOAD_L3_MISS_RETIRED.REMOTE_DRAM"},
			// {0xD3, 0x04, 0, "MEM_LOAD_L3_MISS_RETIRED.REMOTE_HITM"},
			// {0xD3, 0x08, 0, "MEM_LOAD_L3_MISS_RETIRED.REMOTE_FWD"},
			// {0xE6, 0x01, 0, "BACLEARS.ANY"},
			// {0xFE, 0x02, 0, "IDI_MISC.WB_UPGRADE"},
			// {0xFE, 0x04, 0, "IDI_MISC.WB_DOWNGRADE"},
			{0xB7, 0x01, 0, "OCR.COREWB_M.ANY_RESPONSE"},
		};
	#else
		COUNTER_T pmu_counters[PMU_MAX_COUNTER] = 
		{
			{0, 0, 0, "INST_RETIRED.ANY"}, //IA32_PERF_FIXED_CTR0
			{0, 0, 0, "CPU_CLK_UNHALTED.THREAD"}, //IA32_PERF_FIXED_CTR1: CPU_CLK_UNHALTED.THREAD or CPU_CLK_UNHALTED.CORE or CPU_CLK_UNHALTED.THREAD_ANY, depending on AnyThread=0|1
			{0, 0, 0, "CPU_CLK_UNHALTED.REF_TSC"}, //IA32_PERF_FIXED_CTR2
			{0x00, 0x01, 0, "INST_RETIRED.ANY"},
			{0x3C, 0x00, 0, "CPU_CLK_UNHALTED.THREAD"},
			{0x48, 0x01, 0, "L1D_PEND_MISS.PENDING"},
			{0x48, 0x01, 1, "L1D_PEND_MISS.PENDING_CYCLES"},
			{0x48, 0x02, 0, "L1D_PEND_MISS.FB_FULL"},
			{0x51, 0x01, 0, "L1D.REPLACEMENT"},
			{0x63, 0x02, 0, "LOCK_CYCLES_CACHE_LOCK_DURATION"},
			{0x24, 0x21, 0, "L2_RQSTS.DEMAND_DATA_RD_MISS"},
			{0x24, 0xC1, 0, "L2_RQSTS.DEMAND_DATA_RD_HIT"},
			{0x24, 0xE1, 0, "L2_RQSTS.ALL_DEMAND_DATA_RD"},
			{0x24, 0x22, 0, "L2_RQSTS.RFO_MISS"},
			{0x24, 0xC2, 0, "L2_RQSTS.RFO_HIT"},
			{0x24, 0xE2, 0, "L2_RQSTS.ALL_RFO"},
			{0x24, 0x24, 0, "L2_RQSTS.CODE_RD_MISS"},
			{0x24, 0xC4, 0, "L2_RQSTS.CODE_RD_HIT"},
			{0x24, 0x38, 0, "L2_RQSTS.PF_MISS"},
			{0x24, 0x3F, 0, "L2_RQSTS.MISS"},
			{0x24, 0xD8, 0, "L2_RQSTS.PF_HIT"},
			{0x24, 0x27, 0, "L2_RQSTS.ALL_DEMAND_MISS"},
			{0x24, 0xE4, 0, "L2_RQSTS.ALL_CODE_RD"},
			{0x24, 0xF8, 0, "L2_RQSTS.ALL_PF"},
			{0x24, 0xFF, 0, "L2_RQSTS.REFERENCES"},
			{0xF1, 0x1F, 0, "L2_LINES_IN.ALL"},
			{0xF2, 0x01, 0, "L2_LINES_OUT.SILENT"},
			{0xF2, 0x02, 0, "L2_LINES_OUT.NON_SILENT"},
			{0xF2, 0x04, 0, "L2_LINES_OUT.USELESS_PREF"},
			{0xF0, 0x40, 0, "L2_TRANS.L2_WB"},
			{0xAE, 0x01, 0, "ITLB.ITLB_FLUSH"},
			{0xBD, 0x01, 0, "TLB_FLUSH.DTLB_THREAD"},
			{0xBD, 0x20, 0, "TLB_FLUSH.STLB_ANY"},
			{0x60, 0x01, 0, "OFFCORE_REQUESTS_OUTSTANDING.DEMAND_DATA_RD"},
			{0x60, 0x01, 1, "OFFCORE_REQUESTS_OUTSTANDING.CYCLES_WITH_DEMAND_DATA_RD"},
			{0x60, 0x01, 6, "OFFCORE_REQUESTS_OUTSTANDING.DEMAND_DATA_RD_GE_6"},
			{0x60, 0x02, 0, "OFFCORE_REQUESTS_OUTSTANDING.DEMAND_CODE_RD"},
			{0x60, 0x02, 1, "OFFCORE_REQUESTS_OUTSTANDING.CYCLES_WITH_DEMAND_CODE_RD"},
			{0x60, 0x04, 0, "OFFCORE_REQUESTS_OUTSTANDING.DEMAND_RFO"},
			{0x60, 0x04, 1, "OFFCORE_REQUESTS_OUTSTANDING.CYCLES_WITH_DEMAND_RFO"},
			{0x60, 0x08, 0, "OFFCORE_REQUESTS_OUTSTANDING.ALL_DATA_RD"},
			{0x60, 0x08, 1, "OFFCORE_REQUESTS_OUTSTANDING.CYCLES_WITH_DATA_RD"},
			{0x60, 0x10, 0, "OFFCORE_REQUESTS_OUTSTANDING.L3_MISS_DEMAND_DATA_RD"},
			{0x60, 0x10, 1, "OFFCORE_REQUESTS_OUTSTANDING.CYCLES_WITH_L3_MISS_DEMAND_DATA_RD"},
			{0x60, 0x10, 6, "OFFCORE_REQUESTS_OUTSTANDING.L3_MISS_DEMAND_DATA_RD_GE_6"},
			{0xB0, 0x01, 0, "OFFCORE_REQUESTS.DEMAND_DATA_RD"},
			{0xB0, 0x02, 0, "OFFCORE_REQUESTS.DEMAND_CODE_RD"},
			{0xB0, 0x04, 0, "OFFCORE_REQUESTS.DEMAND_RFO"},
			{0xB0, 0x08, 0, "OFFCORE_REQUESTS.ANY.READ"},
			{0xB0, 0x10, 0, "OFFCORE_REQUESTS.ANY.RFO"},
			{0xB0, 0x08, 0, "OFFCORE_REQUESTS.ALL_DATA_RD"},
			{0xB0, 0x20, 0, "OFFCORE_REQUESTS.UNCACHED_MEM"},
			{0xB0, 0x40, 0, "OFFCORE_REQUESTS.L1D_WRITEBACK"},
			{0xB0, 0x80, 0, "OFFCORE_REQUESTS.ALL_REQUESTS"},
			{0xB2, 0x01, 0, "OFFCORE_REQUESTS.BUFFER_SQ_FULL"},

			{0xB7, 0x01, 0, "OFFCORE_RESPONSE0"}, //Can only be assigned to PMC0 or 1. Bit 38 can be used to measure average latency. 3572
			{0xB7, 0x02, 0, "OFFCORE_RESPONSE1"}, //Can only be assigned to PMC0 or 1.
			
			{0xF4, 0x10, 0, "SQ_MISC.SPLIT_LOCK"}, //deprecated I believe.
			{0xF6, 0x01, 0, "SQ_FULL_STALL_CYCLES"},
			{0x83, 0x01, 0, "ICACHE_64B.IFTAG_HIT"},
			{0x83, 0x02, 0, "ICACHE_64B.IFTAG_MISS"},
			{0x83, 0x04, 0, "ICACHE_64B.IFTAG_STALL"},
			{0xA3, 0x01, 0, "CYCLE_ACTIVITY.CYCLES_L2_MISS"},
			{0xA3, 0x05, 0, "CYCLE_ACTIVITY.STALLS_L2_MISS"},
			{0xA3, 0x04, 0, "CYCLE_ACTIVITY.STALLS_TOTAL"},
			{0xA3, 0x02, 0, "CYCLE_ACTIVITY.CYCLES_L3_MISS"},
			{0xA3, 0x06, 0, "CYCLE_ACTIVITY.STALLS_L3_MISS"},
			{0xA3, 0x08, 0, "CYCLE_ACTIVITY.CYCLES_L1D_MISS"},
			{0xA3, 0x0C, 0, "CYCLE_ACTIVITY.STALLS_L1D_MISS"},
			{0xA3, 0x10, 0, "CYCLE_ACTIVITY.CYCLES_MEM_ANY"},
			{0xA3, 0x14, 0, "CYCLE_ACTIVITY.STALLS_MEM_ANY"},
			{0xA2, 0x01, 0, "RESOURCE_STALLS.ANY"},
			{0xA2, 0x02, 0, "RESOURCE_STALLS.LB"},
			{0xA2, 0x08, 0, "RESOURCE_STALLS.SB"},
			{0xA2, 0x10, 0, "RESOURCE_STALLS.ROB"},
			{0xA6, 0x01, 0, "EXE_ACTIVITY.EXE_BOUND_0_PORTS"},
			{0xA6, 0x02, 0, "EXE_ACTIVITY.1_PORTS_UTIL"},
			{0xA6, 0x04, 0, "EXE_ACTIVITY.2_PORTS_UTIL"},
			{0xA6, 0x08, 0, "EXE_ACTIVITY.3_PORTS_UTIL"},
			{0xA6, 0x10, 0, "EXE_ACTIVITY.4_PORTS_UTIL"},
			{0xA6, 0x40, 0, "EXE_ACTIVITY.BOUND_ON_STORES"},
			{0x28, 0x40, 0, "CORE_POWER.THROTTLE"},
			{0xC6, 0x01, 0, "FRONTEND_RETIRED_MISS"}, //multiple of these with same event and umask but there is no CMASK?
			{0xCB, 0x01, 0, "HW_INTERRUPTS_RECEIVED"},
			{0xC3, 0x01, 0, "MACHINE_CLEARS.COUNT"},
			{0xC5, 0x00, 0, "BR_MISP_RETIRED.ALL_BRANCHES"},
			{0xD0, 0x11, 0, "MEM_INST_RETIRED.STLB_MISS_LOADS"},
			{0xD0, 0x12, 0, "MEM_INST_RETIRED.STLB_MISS_STORES"},
			{0xD0, 0x21, 0, "MEM_INST_RETIRED.LOCK_LOADS"},
			{0xD0, 0x81, 0, "MEM_INST_RETIRED.ALL_LOAD"},
			{0xD0, 0x82, 0, "MEM_INST_RETIRED.ALL_STORES"},
			{0xD1, 0x01, 0, "MEM_LOAD_RETIRED.L1_HIT"},
			{0xD1, 0x02, 0, "MEM_LOAD_RETIRED.L2_HIT"},
			{0xD1, 0x04, 0, "MEM_LOAD_RETIRED.L3_HIT"},
			{0xD1, 0x08, 0, "MEM_LOAD_RETIRED.L1_MISS"},
			{0xD1, 0x10, 0, "MEM_LOAD_RETIRED.L2_MISS"},
			{0xD1, 0x20, 0, "MEM_LOAD_RETIRED.L3_MISS"},
			{0xD1, 0x40, 0, "MEM_LOAD_RETIRED.FB_HIT"},
			{0xD2, 0x01, 0, "MEM_LOAD_L3_HIT_RETIRED.XSNP_MISS"},
			{0xD2, 0x02, 0, "MEM_LOAD_L3_HIT_RETIRED.XSNP_HIT"},
			{0xD2, 0x04, 0, "MEM_LOAD_L3_HIT_RETIRED.XSNP_HITM"},
			{0xD2, 0x08, 0, "MEM_LOAD_L3_HIT_RETIRED.XSNP_NONE"},
			{0xD3, 0x01, 0, "MEM_LOAD_L3_MISS_RETIRED.LOCAL_DRAM"},
			{0xD3, 0x02, 0, "MEM_LOAD_L3_MISS_RETIRED.REMOTE_DRAM"},
			{0xD3, 0x04, 0, "MEM_LOAD_L3_MISS_RETIRED.REMOTE_HITM"},
			{0xD3, 0x08, 0, "MEM_LOAD_L3_MISS_RETIRED.REMOTE_FWD"},
			{0xE6, 0x01, 0, "BACLEARS.ANY"},
			{0xFE, 0x02, 0, "IDI_MISC.WB_UPGRADE"},
			{0xFE, 0x04, 0, "IDI_MISC.WB_DOWNGRADE"},

			//Broadwell
			{0xF2, 0x05, 0, "L2_LINES_OUT.DEMAND_CLEAN"},
			{0x2E, 0x41, 0, "LONGEST_LAT_CACHE.MISS"},
			{0x4F, 0x10, 0, "EPT.WALK_CYCLES"},
		};
	#endif
#endif

#endif //PMU_PERFMON_H