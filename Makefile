all: libiotrace.so

.PHONY = clean

iotrace.o: iotrace.c
	gcc -std=c99 -pthread -fPIC -Wall -O2 -c iotrace.c

libiotrace.so: iotrace.o
	gcc -pthread -shared -Wl,-soname,libiotrace.so -o libiotrace.so iotrace.o -lc -ldl

clean:
	rm -f libiotrace.so iotrace.o
