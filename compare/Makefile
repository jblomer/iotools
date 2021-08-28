CXXFLAGS = -std=c++17 -Wall -pthread -g -O3
CXXFLAGS_ROOT = $(shell root-config --cflags)
LDFLAGS =
LDFLAGS_ROOT = $(shell root-config --libs)
LDFLAGS_HDF5 = -lhdf5 -lhdf5_hl

.PHONY = clean

all: gen_lhcb_h5 lhcb_h5

lhcb_event.cxx: lhcb_event.h lhcb_event_linkdef.h
	rootcling -f $@ $^

libLhcbEvent.so: lhcb_event.cxx
	g++ -shared -fPIC -o$@ $(CXXFLAGS) $(CXXFLAGS_ROOT) $< $(LDFLAGS) $(LDFLAGS_ROOT)

util.o: util.cc util.h lhcb_event.h
	g++ $(CXXFLAGS) $(CXXFLAGS_ROOT) -c -o $@ $<

util_h5.o: util_h5.cc util_h5.h
	g++ $(CXXFLAGS) -c -o $@ $<

gen_lhcb_h5: gen_lhcb_h5.cc util.o util_h5.o
	g++ $(CXXFLAGS) $(CXXFLAGS_ROOT) -o $@ $^ $(LDFLAGS_ROOT) $(LDFLAGS_HDF5) $(LDFLAGS)

lhcb_h5: lhcb_h5.cc util_h5.o
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDFLAGS_HDF5)

clean:
	rm -f gen_lhcb_h5 lhcb_h5 \
		util.o util_h5.o \
		libLhcbEvent.so lhcb_event.cxx lhcb_event_rdict.pcm
