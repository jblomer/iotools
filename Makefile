CXXFLAGS_CUSTOM = -std=c++14 -Wall -pthread -Wall -g -O2
CXXFLAGS_ROOT = $(shell root-config --cflags)
LDFLAGS_CUSTOM =
LDFLAGS_ROOT = $(shell root-config --libs) -lROOTNTuple
CXXFLAGS = $(CXXFLAGS_CUSTOM) $(CXXFLAGS_ROOT)
LDFLAGS = $(LDFLAGS_CUSTOM) $(LDFLAGS_ROOT)

DATA_ROOT = /data/calibration
SAMPLE_lhcb = B2HHH
SAMPLE_cms = ttjet_13tev_june2019
SAMPLE_cmsX10 = ttjet_13tev_june2019X10
SAMPLE_h1 = h1dst
MASTER_lhcb = /data/lhcb/$(SAMPLE_lhcb).root
MASTER_cms = /data/cms/$(SAMPLE_cms).root
MASTER_cmsX10 = /data/cms/$(SAMPLE_cms).root
MASTER_h1 = /data/h1/dstarmb.root /data/h1/dstarp1a.root /data/h1/dstarp1b.root /data/h1/dstarp2.root
NAME_lhcb = LHCb OpenData B2HHH
NAME_cms = CMS nanoAOD $(SAMPLE_cms)
NAME_cmsX10 = CMS nanoAOD $(SAMPLE_cms) [x10]
NAME_h1 = H1 micro DST
COMPRESSION_none = 0
COMPRESSION_lz4 = 404
COMPRESSION_zlib = 101
COMPRESSION_lzma = 207

.PHONY = all clean data data_lhcb data_cms data_h1
all: lhcb cms h1 gen_lhcb gen_cms gen_h1 ntuple_info tree_info


### DATA #######################################################################

data: data_lhcb data_cms data_h1

data_lhcb: $(DATA_ROOT)/B2HHH~none.ntuple \
	$(DATA_ROOT)/B2HHH~zlib.ntuple \
	$(DATA_ROOT)/B2HHH~lz4.ntuple \
	$(DATA_ROOT)/B2HHH~lzma.ntuple \
	$(DATA_ROOT)/B2HHH~none.root \
	$(DATA_ROOT)/B2HHH~zlib.root \
	$(DATA_ROOT)/B2HHH~lz4.root \
	$(DATA_ROOT)/B2HHH~lzma.root

data_cms: $(DATA_ROOT)/ttjet_13tev_june2019~none.root \
	$(DATA_ROOT)/ttjet_13tev_june2019~lz4.root \
	$(DATA_ROOT)/ttjet_13tev_june2019~zlib.root \
	$(DATA_ROOT)/ttjet_13tev_june2019~lzma.root \
	$(DATA_ROOT)/ttjet_13tev_june2019~none.ntuple \
	$(DATA_ROOT)/ttjet_13tev_june2019~lz4.ntuple \
	$(DATA_ROOT)/ttjet_13tev_june2019~zlib.ntuple \
	$(DATA_ROOT)/ttjet_13tev_june2019~lzma.ntuple

data_h1: $(DATA_ROOT)/h1dst~none.root \
	$(DATA_ROOT)/h1dst~lz4.root \
	$(DATA_ROOT)/h1dst~zlib.root \
	$(DATA_ROOT)/h1dst~lzma.root \
	$(DATA_ROOT)/h1dst~none.ntuple \
	$(DATA_ROOT)/h1dst~lz4.ntuple \
	$(DATA_ROOT)/h1dst~zlib.ntuple \
	$(DATA_ROOT)/h1dst~lzma.ntuple

gen_lhcb: gen_lhcb.cxx util.o
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

gen_cms: gen_cms.cxx util.o
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

gen_h1: gen_h1.cxx util.o libH1event.so
	g++ $(CXXFLAGS) -o $@ $< util.o $(LDFLAGS)

libH1event.so: libh1Dict.cxx
	g++ -shared -fPIC -o $@ $(CXXFLAGS) $< $(LDFLAGS)

libh1Dict.cxx: h1event.h h1linkdef.h
	rootcling -f $@ $^

$(DATA_ROOT)/$(SAMPLE_lhcb)~%.ntuple: gen_lhcb $(MASTER_lhcb)
	./gen_lhcb -i $(MASTER_lhcb) -o $(shell dirname $@) -c $*

$(DATA_ROOT)/$(SAMPLE_cms)~%.ntuple: gen_cms $(MASTER_cms)
	./gen_cms -i $(MASTER_cms) -o $(shell dirname $@) -c $*

$(DATA_ROOT)/$(SAMPLE_cmsX10)~%.ntuple: gen_cms $(MASTER_cms)
	./gen_cms -b10 -i $(MASTER_cms) -o $(shell dirname $@) -c $*

$(DATA_ROOT)/$(SAMPLE_h1)~%.ntuple: gen_h1 $(MASTER_h1)
	./gen_h1 -o $(shell dirname $@) -c $* $(MASTER_h1)

$(DATA_ROOT)/$(SAMPLE_lhcb)~%.root: $(MASTER_lhcb)
	hadd -f$(COMPRESSION_$*) $@ $<

$(DATA_ROOT)/$(SAMPLE_cms)~%.root: $(MASTER_cms)
	hadd -f$(COMPRESSION_$*) $@ $<

$(DATA_ROOT)/$(SAMPLE_cmsX10)~%.root: $(MASTER_cms)
	hadd -f$(COMPRESSION_$*) $@ $< $< $< $< $< $< $< $< $< $<

$(DATA_ROOT)/$(SAMPLE_h1)~%.root: $(MASTER_h1)
	hadd -f$(COMPRESSION_$*) $@ $^


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


cms: cms.cxx util.o
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

lhcb: lhcb.cxx util.o
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

h1: h1.cxx util.o
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

util.o: util.cc util.h
	g++ $(CXXFLAGS) -c $<


### BENCHMARKS #################################################################

clear_page_cache: clear_page_cache.c
	gcc -Wall -g -o clear_page_cache clear_page_cache.c
	sudo chown root clear_page_cache
	sudo chmod 4755 clear_page_cache


result_size_%.txt: bm_events_% bm_formats bm_size.sh
	./bm_size.sh $(DATA_ROOT) $(SAMPLE_$*) $$(cat bm_events_$*) > $@


result_read_mem.lhcb~%.txt: lhcb
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ ./lhcb -i $(DATA_ROOT)/$(SAMPLE_lhcb)~$*

result_read_mem.lhcb+rdf~%.txt: lhcb
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ ./lhcb -r -i $(DATA_ROOT)/$(SAMPLE_lhcb)~$*

result_read_ssd.lhcb~%.txt: lhcb
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ ./lhcb -i $(DATA_ROOT)/$(SAMPLE_lhcb)~$*

result_read_ssd.lhcb+rdf~%.txt: lhcb
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ ./lhcb -r -i $(DATA_ROOT)/$(SAMPLE_lhcb)~$*


result_read_mem.cms~%.txt: cms
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ ./cms -i $(DATA_ROOT)/$(SAMPLE_cms)~$*

result_read_mem.cms+rdf~%.txt: cms
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ ./cms -r -i $(DATA_ROOT)/$(SAMPLE_cms)~$*

result_read_ssd.cms~%.txt: cms
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ ./cms -i $(DATA_ROOT)/$(SAMPLE_cms)~$*

result_read_ssd.cms+rdf~%.txt: cms
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ ./cms -r -i $(DATA_ROOT)/$(SAMPLE_cms)~$*


result_read_mem.h1~%.txt: h1
	BM_CACHED=1 ./bm_timing.sh $@ ./h1 -i $(DATA_ROOT)/$(SAMPLE_h1)~$*


result_read_%.txt: $(wildcard result_read_%~*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_$* ./bm_combine.sh


graph_size.%.root: result_size_%.txt
	root -q -l 'bm_size.C("$*", "Data size $(NAME_$*)")'

graph_read_mem.lhcb@evs.root: result_read_mem.lhcb.txt result_size_lhcb.txt bm_events_lhcb
	root -q -l 'bm_timing.C("result_read_mem.lhcb", "result_size_lhcb.txt", "MEMORY READ throughput $(NAME_lhcb)", "$@", $(shell cat bm_events_lhcb), -1, true)'

graph_read_mem.cms@evs.root: result_read_mem.cms.txt result_size_cms.txt bm_events_cms
	root -q -l 'bm_timing.C("result_read_mem.cms", "result_size_cms.txt", "MEMORY READ throughput $(NAME_cms)", "$@", $(shell cat bm_events_cms), -1, true)'

graph_read_ssd.lhcb@evs.root: result_read_ssd.lhcb.txt result_size_lhcb.txt bm_events_lhcb
	root -q -l 'bm_timing.C("result_read_ssd.lhcb", "result_size_lhcb.txt", "SSD READ throughput $(NAME_lhcb)", "$@", $(shell cat bm_events_lhcb), -1, true)'

graph_read_ssd.cms@evs.root: result_read_ssd.cms.txt result_size_cms.txt bm_events_cms
	root -q -l 'bm_timing.C("result_read_ssd.cms", "result_size_cms.txt", "SSD READ throughput $(NAME_cms)", "$@", $(shell cat bm_events_cms), -1, true)'


graph_%.pdf: graph_%.root
	root -q -l 'bm_convert_to_pdf.C("graph_$*")'


### CLEAN ######################################################################

clean:
	rm -f util.o lhcb cms_dimuon gen_lhcb gen_cms ntuple_info tree_info
	rm -rf _make_ttjet_13tev_june2019*
	rm -rf include_cms
	rm -f libH1event.so libH1Dict.cxx
