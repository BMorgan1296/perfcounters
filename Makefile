CC=gcc
CFLAGS= -O2 #-g -fsanitize=address
.DEFAULT_GOAL := lib
BUILD_DIR = $(shell pwd)

TYPE= INTEL_CORE #INTEL_XEON
GEN= GEN6

perf_counters_util.o: perf_counters_util.c perf_counters_util.h 
	$(CC) $(CFLAGS) -c $< -D $(TYPE) -D $(GEN) 

pmu_perfmon.o: pmu_perfmon.c pmu_perfmon.h
	$(CC) $(CFLAGS) -c $< -D $(TYPE) -D $(GEN) 

uncore_perfmon.o: uncore_perfmon.c uncore_perfmon.h
	$(CC) $(CFLAGS) -c $< -D $(TYPE) -D $(GEN) 

perf_counters.a: perf_counters_util.o pmu_perfmon.o uncore_perfmon.o
	ar rcs perf_counters.a perf_counters_util.o pmu_perfmon.o uncore_perfmon.o 

lib: perf_counters.a 
	#cp perf_counters.h /usr/local/include/perf_counters.h

#TESTS#
test_uncore: test_uncore.c perf_counters.a
	$(CC) $(CFLAGS) -o $@ $^

test_pmu: test_pmu.c perf_counters.a
	$(CC) $(CFLAGS) -o $@ $^

test: test_uncore test_pmu uncore_address_map

all: lib test 

clean:
	rm -rf *.o *.a test_uncore test_pmu
