CXXFLAGS_CUSTOM = -std=c++14 -Wall -pthread -Wall -g -O2
CXXFLAGS_ROOT = $(shell root-config --cflags)
LDFLAGS_CUSTOM =
LDFLAGS_ROOT = $(shell root-config --libs)
CXXFLAGS = $(CXXFLAGS_CUSTOM) $(CXXFLAGS_ROOT)
LDFLAGS = $(LDFLAGS_CUSTOM) $(LDFLAGS_ROOT)

all: lhcb_opendata cms_dimuon

.PHONY = clean

cms_dimuon: cms_dimuon.cc
	g++ $(CXXFLAGS) -o cms_dimuon cms_dimuon.cc $(LDFLAGS)

lhcb_opendata: lhcb_opendata.cc lhcb_opendata.h util.h util.o event.h
	g++ $(CXXFLAGS) -o lhcb_opendata lhcb_opendata.cc util.o $(LDFLAGS)

util.o: util.cc util.h
	g++ $(CXXFLAGS) -c util.cc

clean:
	rm -f util.o lhcb_opendata cms_dimuon
