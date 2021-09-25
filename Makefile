CC=gcc
CFLAGS= -O2 $(OPS) #-g -fsanitize=address
.DEFAULT_GOAL := lib
BUILD_DIR = $(shell pwd)



perf_counters_util.o: perf_counters_util.c perf_counters_util.h 
	$(CC) $(CFLAGS) -c $<

pmu_perfmon.o: pmu_perfmon.c pmu_perfmon.h
	$(CC) $(CFLAGS) -c $<

uncore_perfmon.o: uncore_perfmon.c uncore_perfmon.h
	$(CC) $(CFLAGS) -c $<

perf_counters.a: perf_counters_util.o pmu_perfmon.o uncore_perfmon.o
	ar rcs perf_counters.a perf_counters_util.o pmu_perfmon.o uncore_perfmon.o 

lib: perf_counters.a 
	sudo cp perf_counters.h /usr/local/include/
	sudo cp perf_counters_util.h /usr/local/include/
	sudo cp perf_counters.a /usr/local/lib/libperf_counters.a

#TESTS#
test_uncore: test_uncore.c perf_counters.a
	$(CC) $(CFLAGS) -o $@ $^

test_pmu: test_pmu.c perf_counters.a
	$(CC) $(CFLAGS) -o $@ $^

test: clean test_uncore test_pmu

all: lib test 

clean:
	rm -rf *.o *.a test_uncore test_pmu
	sudo rm /usr/local/include/perf_counters.h /usr/local/lib/perf_counters.a
