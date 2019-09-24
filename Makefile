CXXFLAGS_CUSTOM = -std=c++14 -Wall -pthread -Wall -g -O2
CXXFLAGS_ROOT = $(shell root-config --cflags)
LDFLAGS_CUSTOM =
LDFLAGS_ROOT = $(shell root-config --libs) -lROOTNTuple
CXXFLAGS = $(CXXFLAGS_CUSTOM) $(CXXFLAGS_ROOT)
LDFLAGS = $(LDFLAGS_CUSTOM) $(LDFLAGS_ROOT)

DATA_ROOT = /data/calibration
MASTER_LHCB = /data/lhcb/B2HHH.root
COMPRESSION_none = 0
COMPRESSION_lz4 = 404
COMPRESSION_zlib = 101
COMPRESSION_lzma = 207

all: lhcb_opendata cms_dimuon gen_lhcb

data_lhcb: $(DATA_ROOT)/ntuple/B2HHH~none.ntuple \
	$(DATA_ROOT)/ntuple/B2HHH~zlib.ntuple \
	$(DATA_ROOT)/ntuple/B2HHH~lz4.ntuple \
	$(DATA_ROOT)/ntuple/B2HHH~lzma.ntuple \
	$(DATA_ROOT)/tree/B2HHH~none.root \
	$(DATA_ROOT)/tree/B2HHH~zlib.root \
	$(DATA_ROOT)/tree/B2HHH~lz4.root \
	$(DATA_ROOT)/tree/B2HHH~lzma.root

data: data_lhcb


.PHONY = clean data data_lhcb

gen_lhcb: gen_lhcb.cc util.o
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(DATA_ROOT)/ntuple/B2HHH~%.ntuple: gen_lhcb $(MASTER_LHCB)
	./gen_lhcb -i $(MASTER_LHCB) -o $(shell dirname $@) -c $*

$(DATA_ROOT)/tree/B2HHH~%.root: $(MASTER_LHCB)
	hadd -f$(COMPRESSION_$*) $@ $<

cms_dimuon: cms_dimuon.cc
	g++ $(CXXFLAGS) -o cms_dimuon cms_dimuon.cc $(LDFLAGS)

lhcb_opendata: lhcb_opendata.cc lhcb_opendata.h util.h util.o event.h
	g++ $(CXXFLAGS) -o lhcb_opendata lhcb_opendata.cc util.o $(LDFLAGS)

util.o: util.cc util.h
	g++ $(CXXFLAGS) -c $<

clean:
	rm -f util.o lhcb_opendata cms_dimuon gen_lhcb
