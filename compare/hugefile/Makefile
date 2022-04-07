CXXFLAGS = -std=c++17 -Wall -pthread -g -O3 -I../h5hep -I.. -DNDEBUG
CXXFLAGS_ROOT = $(shell root-config --cflags)
LDFLAGS =
LDFLAGS_ROOT = $(shell root-config --libs) -lROOTNTuple
LDFLAGS_HDF5 = -lhdf5
LDFLAGS_PARQUET = -larrow -lparquet
BIN = test_ttree test_ntuple

.PHONY = clean h5hep

all: h5hep $(BIN)

clean:
	make -C ../ -f Makefile.h5hep clean
	rm -f $(BIN) *.o

h5hep:
	make -C ../ -f Makefile.h5hep h5hep

%.o: %.cc
	g++ $(CXXFLAGS) -c -o $@ $^

test_ttree: test_ttree.cc main.o
	g++ $(CXXFLAGS) $(CXXFLAGS_ROOT) -o $@ $^ $(LDFLAGS_ROOT) $(LDFLAGS)

test_ntuple: test_ntuple.cc main.o
	g++ $(CXXFLAGS) $(CXXFLAGS_ROOT) -o $@ $^ $(LDFLAGS_ROOT) $(LDFLAGS)
