all: libiotrace.so iotrace_capture iotrace_test

.PHONY = clean

iotrace.o: iotrace.c wire_format.h
	gcc -std=c99 -pthread -fPIC -Wall -O2 -c iotrace.c

libiotrace.so: iotrace.o
	gcc -pthread -shared -Wl,-soname,libiotrace.so -o libiotrace.so iotrace.o -lc -ldl -lrt

iotrace_capture: capture.cc wire_format.h
	g++ -std=c++11 -pthread -fno-exceptions -Wall -O2 -g $(shell root-config --cflags) -o iotrace_capture \
		$(shell root-config --libs) capture.cc

iotrace_test: test.cc
	g++ -std=c++11 -pthread -fno-exceptions -Wall -O2 -g -o iotrace_test test.cc

clean:
	rm -f libiotrace.so iotrace.o iotrace_capture iotrace.fanout
