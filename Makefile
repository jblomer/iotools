CFLAGS_CUSTOM = -std=c99 -Wall -pthread -g -O2
CXXFLAGS_CUSTOM = -std=c++11 -Wall -pthread -fno-exceptions -Wall -g -O2
CXXFLAGS_ROOT = $(shell root-config --cflags)
LDFLAGS_CUSTOM =
LDFLAGS_ROOT = $(shell root-config --libs)
CFLAGS = $(CFLAGS_CUSTOM)
CXXFLAGS = $(CXXFLAGS_CUSTOM) $(CXXFLAGS_ROOT)
LDFLAGS = $(LDFLAGS_CUSTOM) $(LDFLAGS_ROOT)

all: libiotrace.so iotrace_capture iotrace_test \
  lhcb_opendata

.PHONY = clean

iotrace.o: iotrace.c wire_format.h
	gcc $(CFLAGS) -fPIC -c iotrace.c

libiotrace.so: iotrace.o
	gcc -pthread -shared -Wl,-soname,libiotrace.so -o libiotrace.so iotrace.o -lc -ldl -lrt

iotrace_capture: capture.cc wire_format.h
	g++ $(CXXFLAGS) -o iotrace_capture capture.cc $(LDFLAGS)

iotrace_test: test.cc
	g++ $(CXXFLAGS) -o iotrace_test test.cc

lhcb_opendata: lhcb_opendata.cc lhcb_opendata.h util.h util.o
	g++ $(CXXFLAGS) -o lhcb_opendata lhcb_opendata.cc util.o -lsqlite3 $(LDFLAGS)

util.o: util.cc util.h
	g++ $(CXXFLAGS_CUSTOM) -c util.cc

clean:
	rm -f libiotrace.so iotrace.o iotrace_capture iotrace.fanout iotrace_test \
	  util.o \
	  lhcb_opendata
