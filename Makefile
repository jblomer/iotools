CXXFLAGS_CUSTOM = -std=c++14 -Wall -pthread -Wall -g -O2
CXXFLAGS_ROOT = $(shell root-config --cflags)
LDFLAGS_CUSTOM =
LDFLAGS_ROOT = $(shell root-config --libs) -lROOTNTuple
CXXFLAGS = $(CXXFLAGS_CUSTOM) $(CXXFLAGS_ROOT)
LDFLAGS = $(LDFLAGS_CUSTOM) $(LDFLAGS_ROOT)

DATA_ROOT = /data/calibration
SAMPLE_lhcb = B2HHH
SAMPLE_cms = ttjet_13tev_june2019
MASTER_lhcb = /data/lhcb/$(SAMPLE_lhcb).root
MASTER_cms = /data/cms/$(SAMPLE_cms).root
COMPRESSION_none = 0
COMPRESSION_lz4 = 404
COMPRESSION_zlib = 101
COMPRESSION_lzma = 207

.PHONY = all clean data data_lhcb data_cms
all: lhcb cms_dimuon gen_lhcb gen_cms ntuple_info tree_info


### DATA #######################################################################

data: data_lhcb data_cms

data_lhcb: $(DATA_ROOT)/ntuple/B2HHH~none.ntuple \
	$(DATA_ROOT)/ntuple/B2HHH~zlib.ntuple \
	$(DATA_ROOT)/ntuple/B2HHH~lz4.ntuple \
	$(DATA_ROOT)/ntuple/B2HHH~lzma.ntuple \
	$(DATA_ROOT)/tree/B2HHH~none.root \
	$(DATA_ROOT)/tree/B2HHH~zlib.root \
	$(DATA_ROOT)/tree/B2HHH~lz4.root \
	$(DATA_ROOT)/tree/B2HHH~lzma.root

data_cms: $(DATA_ROOT)/tree/ttjet_13tev_june2019~none.root \
	$(DATA_ROOT)/tree/ttjet_13tev_june2019~lz4.root \
	$(DATA_ROOT)/tree/ttjet_13tev_june2019~zlib.root \
	$(DATA_ROOT)/tree/ttjet_13tev_june2019~lzma.root \
	$(DATA_ROOT)/ntuple/ttjet_13tev_june2019~none.ntuple \
	$(DATA_ROOT)/ntuple/ttjet_13tev_june2019~lz4.ntuple \
	$(DATA_ROOT)/ntuple/ttjet_13tev_june2019~zlib.ntuple \
	$(DATA_ROOT)/ntuple/ttjet_13tev_june2019~lzma.ntuple

gen_lhcb: gen_lhcb.cxx util.o
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

gen_cms: gen_cms.cxx util.o
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(DATA_ROOT)/ntuple/B2HHH~%.ntuple: gen_lhcb $(MASTER_lhcb)
	./gen_lhcb -i $(MASTER_lhcb) -o $(shell dirname $@) -c $*

$(DATA_ROOT)/ntuple/ttjet_13tev_june2019~%.ntuple: gen_cms $(MASTER_cms)
	./gen_cms -i $(MASTER_cms) -o $(shell dirname $@) -c $*

$(DATA_ROOT)/tree/B2HHH~%.root: $(MASTER_lhcb)
	hadd -f$(COMPRESSION_$*) $@ $<

$(DATA_ROOT)/tree/ttjet_13tev_june2019~%.root: $(MASTER_cms)
	hadd -f$(COMPRESSION_$*) $@ $<


### BINARIES ###################################################################

include_cms/classes.hxx: gen_cms $(MASTER_cms)
	./gen_cms -i $(MASTER_cms) -H $(shell dirname $@)

include_cms/classes.cxx: include_cms/classes.hxx
	cd $(shell dirname $@) && rootcling -f classes.cxx classes.hxx linkdef.h

include_cms/libClasses.so: include_cms/classes.cxx
	g++ -shared -fPIC $(CXXFLAGS) -o$@ $< $(LDFLAGS)


ntuple_info: ntuple_info.C include_cms/libClasses.so
	g++ $(CXXFLAGS) -o $@ $< $(LDFLAGS)

tree_info: tree_info.C
	g++ $(CXXFLAGS) -o $@ $< $(LDFLAGS)


cms_dimuon: cms_dimuon.cc
	g++ $(CXXFLAGS) -o cms_dimuon cms_dimuon.cc $(LDFLAGS)

lhcb: lhcb.cxx lhcb.h util.o event.h
	g++ $(CXXFLAGS) -o $@ $< util.o $(LDFLAGS)

util.o: util.cc util.h
	g++ $(CXXFLAGS) -c $<


### BENCHMARKS #################################################################

result_size_%.txt: bm_events_% bm_formats bm_size.sh
	./bm_size.sh $(DATA_ROOT) $(SAMPLE_$*) $$(cat bm_events_$*) > $@


### CLEAN ######################################################################

clean:
	rm -f util.o lhcb cms_dimuon gen_lhcb gen_cms ntuple_info tree_info
	rm -rf _make_ttjet_13tev_june2019*
	rm -rf include_cms
