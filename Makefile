CC=gcc
CFLAGS= -O2 $(OPS) -I/usr/local/include #-g -fsanitize=address
.DEFAULT_GOAL := lib
BUILD_DIR = $(shell pwd)
LDFLAGS += -lperf_counters


perf_counters_util.o: perf_counters_util.c perf_counters_util.h 
	$(CC) $(CFLAGS) -c $<

pmu_perfmon.o: pmu_perfmon.c pmu_perfmon.h
	$(CC) $(CFLAGS) -c $<

uncore_perfmon.o: uncore_perfmon.c uncore_perfmon.h
	$(CC) $(CFLAGS) -c $<

perf_counters.a: perf_counters_util.o pmu_perfmon.o uncore_perfmon.o
	ar rcs perf_counters.a perf_counters_util.o pmu_perfmon.o uncore_perfmon.o 

#Move the library files into the local include folder for other projects to use
lib: perf_counters.a 
	sudo cp perf_counters.h /usr/local/include/
	sudo cp perf_counters_util.h /usr/local/include/
	sudo cp perf_counters.a /usr/local/lib/libperf_counters.a

#TESTS#
test_uncore: test_uncore.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_pmu: test_pmu.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test: test_uncore test_pmu

all: clean lib test 

clean:
	rm -rf *.o *.a test_uncore test_pmu
	sudo rm -f /usr/local/include/perf_counters.h /usr/local/include/perf_counters_util.h /usr/local/lib/libperf_counters.a
