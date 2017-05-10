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



benchmarks: result_size.png

result_size.txt: bm_events bm_formats bm_size.sh
	./bm_size.sh > result_size.txt

result_size.png: result_size.txt bm_size.C
	root -q -l bm_size.C

clean:
	rm -f libiotrace.so iotrace.o iotrace_capture iotrace.fanout iotrace_test \
	  util.o \
	  lhcb_opendata \
	  result_*
