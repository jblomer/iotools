all: libiosample.so

.PHONY = clean

iosample.o: iosample.c
	gcc -fPIC -Wall -O2 -c iosample.c

libiosample.so: iosample.o
	gcc -shared -Wl,-soname,libiosample.so -o libiosample.so iosample.o -lc -ldl

clean:
	rm -f libiosample.so iosample.o
