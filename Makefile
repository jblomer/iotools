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
  lhcb_opendata \
  precision_test

.PHONY = clean benchmarks benchmark_clean

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

precision_test: precision_test.cc
	g++ $(CXXFLAGS) -o precision_test precision_test.cc $(LDFLAGS)

util.o: util.cc util.h
	g++ $(CXXFLAGS_CUSTOM) -c util.cc

clear_page_cache: clear_page_cache.c
	gcc -Wall -g -o clear_page_cache clear_page_cache.c
	sudo chown root clear_page_cache
	sudo chmod 4755 clear_page_cache

BM_FORMAT_LIST = root-inflated \
	     root-deflated \
	     protobuf-inflated \
	     protobuf-deflated \
	     sqlite \
	     h5row \
	     h5column \
	     avro-inflated \
	     avro-deflated \
	     parquet-inflated \
	     parquet-deflated
BM_FORMAT =
BM_DATA_PREFIX = data/lhcb/MagnetDown/B2HHH
BM_USBDATA_PREFIX = data/usb-storage/benchmark-root/lhcb/MagnetDown/B2HHH

benchmarks: graph_size.root \
	result_read_mem.graph.root \
	result_read_ssd.graph.root \
	result_read_hdd.graph.root

result_size.txt: bm_events bm_formats bm_size.sh
	./bm_size.sh > result_size.txt

graph_size.root: result_size.txt bm_size.C
	root -q -l bm_size.C


result_read_mem.%.txt: lhcb_opendata
	BM_CACHED=1 ./bm_timing.sh $@ ./lhcb_opendata -i $(BM_DATA_PREFIX).$*

result_read_ssd.%.txt: lhcb_opendata
	BM_CACHED=0 ./bm_timing.sh $@ ./lhcb_opendata -i $(BM_DATA_PREFIX).$*

result_read_hdd.%.txt: lhcb_opendata
	BM_CACHED=0 ./bm_timing.sh $@ ./lhcb_opendata -i $(BM_USBDATA_PREFIX).$*

result_write_ssd.%.txt: lhcb_opendata
	BM_CACHED=1 ./bm_timing.sh $@ ./lhcb_opendata -i $(BM_DATA_PREFIX).root -o $*

result_write_hdd.%.txt: lhcb_opendata
	BM_CACHED=1 ./bm_timing.sh $@ ./lhcb_opendata -i $(BM_USBDATA_PREFIX).root -o $*

graph_read_mem.root: $(wildcard result_read_mem.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_mem ./bm_combine.sh
	root -q -l 'bm_timing.C("result_read_mem", "READ throughput LHCb OpenData, warm cache", "$@")'

graph_read_ssd.root: $(wildcard result_read_ssd.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_ssd ./bm_combine.sh
	root -q -l 'bm_timing.C("result_read_ssd", "READ throughput LHCb OpenData, SSD cold cache", "$@")'

graph_read_hdd.root: $(wildcard result_read_hdd.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_hdd ./bm_combine.sh
	root -q -l 'bm_timing.C("result_read_hdd", "READ throughput LHCb OpenData, HDD cold cache", "$@")'

graph_write_ssd.root: $(wildcard result_write_ssd.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_write_ssd ./bm_combine.sh
	root -q -l 'bm_timing.C("result_write_ssd", "WRITE throughput LHCb OpenData, SSD", "$@")'

#result_read_hdd.txt: bm_formats bm_timing_disk.sh lhcb_opendata clear_page_cache
#	BM_CACHED=0 BM_PREFIX=data/usb-storage/benchmark-root/lhcb/MagnetDown/B2HHH \
#		  ./bm_timing_read.sh result_read_hdd.txt

#result_read_ssd.graph.root: result_read_ssd.txt bm_timing.C
#	root -q -l 'bm_timing.C("result_read_ssd", "cold cache (SSD)")'

#result_read_hdd.graph.root: result_read_hdd.txt bm_timing.C
#	root -q -l 'bm_timing.C("result_read_hdd", "cold cache (HDD)")'

clean:
	rm -f libiotrace.so iotrace.o iotrace_capture iotrace.fanout iotrace_test \
	  util.o \
	  lhcb_opendata \

benchmark_clean:
	rm -f result_* graph_*
