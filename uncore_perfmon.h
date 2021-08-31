#include "perf_counters.h"
#include "perf_counters_util.h"

#include <sched.h>

#ifndef UNCORE_COUNTERS_H
#define UNCORE_COUNTERS_H

#ifdef INTEL_CORE
	//Enabling registers (to allow reading and writing in the first place)
	#if defined(GEN4) || defined(GEN5)
		#define MSR_UNC_PERF_GLOBAL_CTRL   0x391
		#define MSR_UNC_PERF_GLOBAL_STATUS 0x392
	#elif defined(GEN6) || defined(GEN7) || defined(GEN8) 
		#define MSR_UNC_PERF_GLOBAL_CTRL   0xE01
		#define MSR_UNC_PERF_GLOBAL_STATUS 0xE02
	#endif

	//MSR_UNC_PERF_GLOBAL_CTRL
	#define GLOBAL_CTRL_EN (1 << 29)
	#define GLOBAL_CTRL_DISABLE (uint64_t)(0)
	#define GLOBAL_CTRL_CLEAR (uint64_t)(0)
	#define MSR_UNC_CTR_DISABLE (uint64_t)(0)
	#define GLOBAL_CTRL_PMI_CORE_SEL(n) (1 << n) //Can select multiple cores to have a forwarded PMI by using this

	//CBo Perf Monitoring//
	#define MSR_UNC_CBO_CONFIG 0x396 //Reading returns the number of CBos
	#define CBO_MAX_CTR 2 //Max amount of counters per CBo
	#if defined(GEN4) || defined(GEN5) || defined(GEN6) || defined(GEN7) || defined(GEN8)
		#define MSR_UNC_CBO_PERFEVTSEL(CBo,n) (0x700 + (CBo*0x10) + n)
		#define MSR_UNC_CBO_PERFCTR(CBo,n) (0x700 + (CBo*0x10) + (n+6))
	#elif defined(GEN9) || defined(GEN10) || defined(GEN11) //Not 100% sure about 9. Will need to test on a 9th Gen CPU to see if it belongs in above mappings or down here.
		#define MSR_UNC_CBO_PERFEVTSEL(CBo,n) (0x700 + (CBo*0x8) + n)
		#define MSR_UNC_CBO_PERFCTR(CBo,n) (0x700 + (CBo*0x8) + (n+2))
	#endif

	//ARB Perf Monitoring//
	#define ARB_MAX_CTR 2
	#define MSR_UNC_ARB_PERFEVTSEL(ARB) (0x3B2 + ARB)
	#define MSR_UNC_ARB_PERFCTR(ARB) (0x3B0 + ARB)

	//CBo and ARB CTR Mask. Bitwise & with this to clear bits
	#define MSR_CBO_ARB_MASK (uint64_t)(0x0FFFFFFFFFFF)
	#define MSR_FIXED_MASK   (uint64_t)(0xFFFFFFFFFFFF)

	//Fixed uncore counter MSRs//
	#define FIXED_MAX_CTR 1
	#define MSR_UNC_PERF_FIXED_CTRL 0x394
	#define MSR_UNC_PERF_FIXED_CTR 0x395 //Uncore clock cycles. Reading from this will give that event value.


	//IMC Perf Monitoring//
	#define IMC_FIXED_CTR_NUM 5
	
	//DEFINE NEW COUNTERS HERE//
	#define UNCORE_MAX_COUNTER 1024
	COUNTER_T uncore_counters[UNCORE_MAX_COUNTER] = 
	{
		//CBo events
		{0x22, 0x41, 0, "UNC_CBO_XSNP_RESPONSE.MISS_XCORE"},
		{0x22, 0x81, 0, "UNC_CBO_XSNP_RESPONSE.MISS_EVICTION"},
		{0x22, 0x44, 0, "UNC_CBO_XSNP_RESPONSE.HIT_XCORE"},
		{0x22, 0x48, 0, "UNC_CBO_XSNP_RESPONSE.HITM_XCORE"},

		{0x34, 0x21, 0, "UNC_CBO_CACHE_LOOKUP.WRITE_M"},
		{0x34, 0x81, 0, "UNC_CBO_CACHE_LOOKUP.ANY_M"},
		{0x34, 0x18, 0, "UNC_CBO_CACHE_LOOKUP.READ_I"},
		{0x34, 0x88, 0, "UNC_CBO_CACHE_LOOKUP.ANY_I"},
		{0x34, 0x1F, 0, "UNC_CBO_CACHE_LOOKUP.READ_MESI"},
		{0x34, 0x2F, 0, "UNC_CBO_CACHE_LOOKUP.WRITE_MESI"},
		{0x34, 0x8F, 0, "UNC_CBO_CACHE_LOOKUP.ANY_MESI"},
		{0x34, 0x86, 0, "UNC_CBO_CACHE_LOOKUP.ANY_ES"},
		{0x34, 0x16, 0, "UNC_CBO_CACHE_LOOKUP.READ_ES"},
		{0x34, 0x26, 0, "UNC_CBO_CACHE_LOOKUP.WRITE_ES"},

		//ARB events
		{0x80, 0x01, 0, "UNC_ARB_TRK_OCCUPANCY.ALL"},
		{0x81, 0x01, 0, "UNC_ARB_TRK_REQUESTS.ALL"},
		{0x81, 0x20, 0, "UNC_ARB_TRK_REQUESTS.WRITES"},
		{0x84, 0x01, 0, "UNC_ARB_COH_TRK_REQUESTS.ALL"},
		{0x80, 0x01, 0, "UNC_ARB_TRK_OCCUPANCY.CYCLES_WITH_ANY_REQUEST"}, //same as first ARB event. Really Intel?

		//Fixed event (technically an ARB event according to the manual)
		{0x00, 0x01, 0, "UNC_CLOCK.SOCKET"},
	};

#elif defined INTEL_XEON
	//Not implemented
#endif

#endif //UNCORE_PERFMON_H