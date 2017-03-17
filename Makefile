all: libiotrace.so iotrace_capture

.PHONY = clean

iotrace.o: iotrace.c wire_format.h
	gcc -std=c99 -pthread -fPIC -Wall -O2 -c iotrace.c

libiotrace.so: iotrace.o
	gcc -pthread -shared -Wl,-soname,libiotrace.so -o libiotrace.so iotrace.o -lc -ldl -lrt

iotrace_capture: capture.cc wire_format.h
	g++ -std=c++11 -fno-exceptions -Wall -O2 -g -o iotrace_capture capture.cc

clean:
	rm -f libiotrace.so iotrace.o iotrace_capture
