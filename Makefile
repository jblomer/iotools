CFLAGS_CUSTOM = -std=c99 -Wall -pthread -g -O2
CXXFLAGS_CUSTOM = -std=c++11 -Wall -pthread -Wall -g -O2 \
		  -I/opt/avro-c-1.8.1/include \
		  -I/opt/parquet-cpp-1.0.0/include
CXXFLAGS_ROOT = $(shell root-config --cflags)
LDFLAGS_CUSTOM = -L/opt/avro-c-1.8.1/lib \
		 -L/opt/parquet-cpp-1.0.0/lib
LDFLAGS_ROOT = $(shell root-config --libs) -lTreePlayer
CFLAGS = $(CFLAGS_CUSTOM)
CXXFLAGS = $(CXXFLAGS_CUSTOM) $(CXXFLAGS_ROOT)
LDFLAGS = $(LDFLAGS_CUSTOM) $(LDFLAGS_ROOT)

all: libiotrace.so iotrace_capture iotrace_test \
  lhcb_opendata

.PHONY = clean benchmarks

iotrace.o: iotrace.c wire_format.h
	gcc $(CFLAGS) -fPIC -c iotrace.c

libiotrace.so: iotrace.o
	gcc -pthread -shared -Wl,-soname,libiotrace.so -o libiotrace.so iotrace.o -lc -ldl -lrt

iotrace_capture: capture.cc wire_format.h
	g++ $(CXXFLAGS) -o iotrace_capture capture.cc $(LDFLAGS)

iotrace_test: test.cc
	g++ $(CXXFLAGS) -o iotrace_test test.cc

lhcb_opendata.pb.cc: lhcb_opendata.proto
	protoc --cpp_out=. lhcb_opendata.proto

lhcb_opendata: lhcb_opendata.cc lhcb_opendata.h util.h util.o lhcb_opendata.pb.cc
	g++ $(CXXFLAGS) -o lhcb_opendata lhcb_opendata.cc lhcb_opendata.pb.cc util.o \
		-lhdf5 -lhdf5_hl -lsqlite3 -lavro -lprotobuf $(LDFLAGS) -lz -lparquet

util.o: util.cc util.h
	g++ $(CXXFLAGS_CUSTOM) -c util.cc

clear_page_cache: clear_page_cache.c
	gcc -Wall -g -o clear_page_cache clear_page_cache.c
	sudo chown root clear_page_cache
	sudo chmod 4755 clear_page_cache

benchmarks: result_size.graph.root \
	result_timing_mem.graph.root \
	result_timing_ssd.graph.root \
	result_timing_hdd.graph.root

result_size.txt: bm_events bm_formats bm_size.sh
	./bm_size.sh > result_size.txt

result_timing_mem.txt: bm_formats bm_timing.sh lhcb_opendata
	./bm_timing.sh result_timing_mem.txt

result_timing_ssd.txt: bm_formats bm_timing_disk.sh lhcb_opendata clear_page_cache
	./bm_timing_disk.sh result_timing_ssd.txt

result_timing_hdd.txt: bm_formats bm_timing_disk.sh lhcb_opendata clear_page_cache
	PREFIX=data/usb-storage/benchmark-root/lhcb/MagnetDown/B2HHH ./bm_timing_disk.sh result_timing_hdd.txt

result_size.graph.root: result_size.txt bm_size.C
	root -q -l bm_size.C

result_timing_mem.graph.root: result_timing_mem.txt bm_timing.C
	root -q -l bm_timing.C

result_timing_ssd.graph.root: result_timing_ssd.txt bm_timing.C
	root -q -l 'bm_timing.C("result_timing_ssd", "cold cache (SSD)")'

result_timing_hdd.graph.root: result_timing_hdd.txt bm_timing.C
	root -q -l 'bm_timing.C("result_timing_hdd", "cold cache (HDD)")'

clean:
	rm -f libiotrace.so iotrace.o iotrace_capture iotrace.fanout iotrace_test \
	  util.o \
	  lhcb_opendata \
	  result_*
