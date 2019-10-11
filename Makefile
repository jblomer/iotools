CXXFLAGS_CUSTOM = -std=c++14 -Wall -pthread -Wall -g -O2
CXXFLAGS_ROOT = $(shell root-config --cflags)
LDFLAGS_CUSTOM =
LDFLAGS_ROOT = $(shell root-config --libs) -lROOTNTuple
CXXFLAGS = $(CXXFLAGS_CUSTOM) $(CXXFLAGS_ROOT)
LDFLAGS = $(LDFLAGS_CUSTOM) $(LDFLAGS_ROOT)

DATA_ROOT = /data/calibration
DATA_HOST = xrootd-io-test
DATA_REMOTE = http://$(DATA_HOST)/data
SAMPLE_lhcb = B2HHH
SAMPLE_cms = ttjet_13tev_june2019
SAMPLE_cmsX10 = ttjet_13tev_june2019X10
SAMPLE_h1 = h1dst
SAMPLE_h1X05 = h1dstX05
SAMPLE_h1X10 = h1dstX10
SAMPLE_h1X15 = h1dstX15
SAMPLE_h1X20 = h1dstX20
MASTER_lhcb = /data/lhcb/$(SAMPLE_lhcb).root
MASTER_cms = /data/cms/$(SAMPLE_cms).root
MASTER_cmsX10 = /data/cms/$(SAMPLE_cms).root
MASTER_h1 = /data/h1/dstarmb.root /data/h1/dstarp1a.root /data/h1/dstarp1b.root /data/h1/dstarp2.root
NAME_lhcb = LHCb OpenData B2HHH
NAME_cms = CMS nanoAOD $(SAMPLE_cms)
NAME_cmsX10 = CMS nanoAOD $(SAMPLE_cms) [x10]
NAME_h1 = H1 micro DST
NAME_h1X10 = H1 micro DST [x10]
NAME_h1X20 = H1 micro DST [x20]

COMPRESSION_none = 0
COMPRESSION_lz4 = 404
COMPRESSION_zlib = 101
COMPRESSION_lzma = 207

HDD_NSTREAMS = 1
SSD_NSTREAMS = 4
HTTP_NSTREAMS = 4

NET_DEV = eth0

.PHONY = all clean data data_lhcb data_cms data_h1
all: lhcb cms h1 gen_lhcb prepare_cms gen_cms gen_h1 ntuple_info tree_info


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

prepare_cms: prepare_cms.cxx
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

gen_cms: gen_cms.cxx util.o
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

gen_cmsraw: gen_cmsraw.cxx util.o
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

$(DATA_ROOT)/$(SAMPLE_h1X05)~%.ntuple: gen_h1 $(MASTER_h1)
	./gen_h1 -b5 -o $(shell dirname $@) -c $* $(MASTER_h1)

$(DATA_ROOT)/$(SAMPLE_h1X10)~%.ntuple: gen_h1 $(MASTER_h1)
	./gen_h1 -b10 -o $(shell dirname $@) -c $* $(MASTER_h1)

$(DATA_ROOT)/$(SAMPLE_h1X15)~%.ntuple: gen_h1 $(MASTER_h1)
	./gen_h1 -b15 -o $(shell dirname $@) -c $* $(MASTER_h1)

$(DATA_ROOT)/$(SAMPLE_h1X20)~%.ntuple: gen_h1 $(MASTER_h1)
	./gen_h1 -b20 -o $(shell dirname $@) -c $* $(MASTER_h1)

$(DATA_ROOT)/$(SAMPLE_lhcb)~%.root: $(MASTER_lhcb)
	hadd -f$(COMPRESSION_$*) $@ $<

$(DATA_ROOT)/$(SAMPLE_cms)@clustered.root: $(MASTER_cms) prepare_cms
	./prepare_cms -i $< -o $@

$(DATA_ROOT)/$(SAMPLE_cms)~%.root: $(DATA_ROOT)/$(SAMPLE_cms)@clustered.root
	hadd -f$(COMPRESSION_$*) -O $@ $<

$(DATA_ROOT)/$(SAMPLE_cmsX10)~%.root: $(DATA_ROOT)/$(SAMPLE_cms)@clustered.root
	hadd -f$(COMPRESSION_$*) $@ $< $< $< $< $< $< $< $< $< $<

$(DATA_ROOT)/$(SAMPLE_h1)~%.root: $(MASTER_h1)
	hadd -f$(COMPRESSION_$*) $@ $^

$(DATA_ROOT)/$(SAMPLE_h1X05)~%.root: $(MASTER_h1)
	hadd -f$(COMPRESSION_$*) $@ $^ $^ $^ $^ $^

$(DATA_ROOT)/$(SAMPLE_h1X10)~%.root: $(MASTER_h1)
	hadd -f$(COMPRESSION_$*) $@ $^ $^ $^ $^ $^ $^ $^ $^ $^ $^

$(DATA_ROOT)/$(SAMPLE_h1X15)~%.root: $(MASTER_h1)
	hadd -f$(COMPRESSION_$*) $@ $^ $^ $^ $^ $^ $^ $^ $^ $^ $^ $^ $^ $^ $^ $^

$(DATA_ROOT)/$(SAMPLE_h1X20)~%.root: $(MASTER_h1)
	hadd -f$(COMPRESSION_$*) $@ $^ $^ $^ $^ $^ $^ $^ $^ $^ $^ $^ $^ $^ $^ $^ $^ $^ $^ $^ $^


### BINARIES ###################################################################

include_cms/classes.hxx: gen_cms $(MASTER_cms)
	./gen_cms -i $(MASTER_cms) -H $(shell dirname $@)

include_cms/classes.cxx: include_cms/classes.hxx
	cd $(shell dirname $@) && rootcling -f classes.cxx classes.hxx linkdef.h

include_cms/libClasses.so: include_cms/classes.cxx
	g++ -shared -fPIC $(CXXFLAGS) -o$@ $< $(LDFLAGS)


ntuple_info: ntuple_info.C include_cms/libClasses.so libH1event.so
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
	gcc -Wall -g -o $@ $<
	sudo chown root $@
	sudo chmod 4755 $@

add_latency: add_latency.c
	gcc -Wall -g -o $@ $<
	sudo chown root $@
	sudo chmod 4755 $@


result_size_%.txt: bm_events_% bm_formats bm_size.sh
	./bm_size.sh $(DATA_ROOT) $(SAMPLE_$*) $$(cat bm_events_$*) > $@


result_read_mem.lhcb~%.txt: lhcb
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./lhcb -c$(SSD_NSTREAMS) -i $(DATA_ROOT)/$(SAMPLE_lhcb)~$*

result_read_mem.lhcb+rdf~%.txt: lhcb
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./lhcb -c$(SSD_NSTREAMS) -r -i $(DATA_ROOT)/$(SAMPLE_lhcb)~$*

result_read_ssd.lhcb~%.txt: lhcb
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./lhcb -c$(SSD_NSTREAMS) -i $(DATA_ROOT)/$(SAMPLE_lhcb)~$*

result_read_ssd.lhcb+rdf~%.txt: lhcb
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./lhcb -c$(SSD_NSTREAMS) -r -i $(DATA_ROOT)/$(SAMPLE_lhcb)~$*

result_read_hdd.lhcb~%.txt: lhcb
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./lhcb -c$(HDD_NSTREAMS) -i $(DATA_ROOT)/$(SAMPLE_lhcb)~$*

result_read_hdd.lhcb+rdf~%.txt: lhcb
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./lhcb -c$(HDD_NSTREAMS) -r -i $(DATA_ROOT)/$(SAMPLE_lhcb)~$*

result_read_http.lhcb~%.txt: lhcb
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./lhcb -c$(HTTP_NSTREAMS) -i $(DATA_REMOTE)/$(SAMPLE_lhcb)~$*

result_read_http.lhcb+%ms~lz4.root.txt: lhcb
	./add_latency $(NET_DEV) $*
	ping -c1 $(DATA_HOST)
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./lhcb -c$(HTTP_NSTREAMS) -i $(DATA_REMOTE)/$(SAMPLE_lhcb)~lz4.root
	./add_latency $(NET_DEV) 0

result_read_http.lhcb+%ms~lz4.ntuple.txt: lhcb
	./add_latency $(NET_DEV) $*
	ping -c1 $(DATA_HOST)
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./lhcb -c$(HTTP_NSTREAMS) -i $(DATA_REMOTE)/$(SAMPLE_lhcb)~lz4.ntuple
	./add_latency $(NET_DEV) 0


result_read_mem.cms~%.txt: cms
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./cms -c$(SSD_NSTREAMS) -i $(DATA_ROOT)/$(SAMPLE_cms)~$*

result_read_mem.cms+rdf~%.txt: cms
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./cms -c$(SSD_NSTREAMS) -r -i $(DATA_ROOT)/$(SAMPLE_cms)~$*

result_read_ssd.cms~%.txt: cms
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./cms -c$(SSD_NSTREAMS) -i $(DATA_ROOT)/$(SAMPLE_cms)~$*

result_read_ssd.cms+rdf~%.txt: cms
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./cms -c$(SSD_NSTREAMS) -r -i $(DATA_ROOT)/$(SAMPLE_cms)~$*

result_read_hdd.cms~%.txt: cms
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./cms -c$(HDD_NSTREAMS) -i $(DATA_ROOT)/$(SAMPLE_cms)~$*

result_read_hdd.cms+rdf~%.txt: cms
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./cms -c$(HDD_NSTREAMS) -r -i $(DATA_ROOT)/$(SAMPLE_cms)~$*

result_read_http.cms~%.txt: cms
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./cms -c$(HTTP_NSTREAMS) -i $(DATA_REMOTE)/$(SAMPLE_cms)~$*

result_read_http.cms+%ms~lz4.root.txt: cms
	./add_latency $(NET_DEV) $*
	ping -c1 $(DATA_HOST)
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./cms -c$(HTTP_NSTREAMS) -i $(DATA_REMOTE)/$(SAMPLE_cms)~lz4.root
	./add_latency $(NET_DEV) 0

result_read_http.cms+%ms~lz4.ntuple.txt: cms
	./add_latency $(NET_DEV) $*
	ping -c1 $(DATA_HOST)
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./cms -c$(HTTP_NSTREAMS) -i $(DATA_REMOTE)/$(SAMPLE_cms)~lz4.ntuple
	./add_latency $(NET_DEV) 0



result_read_mem.h1X10~%.txt: h1
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./h1 -c$(SSD_NSTREAMS) -i $(DATA_ROOT)/$(SAMPLE_h1X10)~$*

result_read_mem.h1X10+rdf~%.txt: h1
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./h1 -c$(SSD_NSTREAMS) -r -i $(DATA_ROOT)/$(SAMPLE_h1X10)~$*

result_read_ssd.h1X10~%.txt: h1
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./h1 -c$(SSD_NSTREAMS) -i $(DATA_ROOT)/$(SAMPLE_h1X10)~$*

result_read_ssd.h1X10+rdf~%.txt: h1
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./h1 -c$(SSD_NSTREAMS) -r -i $(DATA_ROOT)/$(SAMPLE_h1X10)~$*

result_read_hdd.h1X10~%.txt: h1
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./h1 -c$(HDD_NSTREAMS) -i $(DATA_ROOT)/$(SAMPLE_h1X10)~$*

result_read_hdd.h1X10+rdf~%.txt: h1
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./h1 -c$(HDD_NSTREAMS) -r -i $(DATA_ROOT)/$(SAMPLE_h1X10)~$*

result_read_http.h1X10~%.txt: h1
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./h1 -c$(HTTP_NSTREAMS) -i $(DATA_REMOTE)/$(SAMPLE_h1X10)~$*

result_read_http.h1X10+%ms~lz4.root.txt: h1
	./add_latency $(NET_DEV) $*
	ping -c1 $(DATA_HOST)
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./h1 -c$(HTTP_NSTREAMS) -i $(DATA_REMOTE)/$(SAMPLE_h1X10)~lz4.root
	./add_latency $(NET_DEV) 0

result_read_http.h1X10+%ms~lz4.ntuple.txt: h1
	./add_latency $(NET_DEV) $*
	ping -c1 $(DATA_HOST)
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./h1 -c$(HTTP_NSTREAMS) -i $(DATA_REMOTE)/$(SAMPLE_h1X10)~lz4.ntuple
	./add_latency $(NET_DEV) 0


result_read_%.txt: $(wildcard result_read_%*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_$* ./bm_combine.sh


graph_size.%.root: result_size_%.txt
	root -q -l 'bm_size.C("$*", "Data size $(NAME_$*)")'

graph_read_mem.lhcb@evs.root: result_read_mem.lhcb.txt result_size_lhcb.txt bm_events_lhcb
	root -q -l 'bm_timing.C("result_read_mem.lhcb", "result_size_lhcb.txt", "MEMORY READ throughput $(NAME_lhcb)", "$@", $(shell cat bm_events_lhcb), -1, true)'

graph_read_mem.cms@evs.root: result_read_mem.cms.txt result_size_cms.txt bm_events_cms
	root -q -l 'bm_timing.C("result_read_mem.cms", "result_size_cms.txt", "MEMORY READ throughput $(NAME_cms)", "$@", $(shell cat bm_events_cms), -1, true)'

graph_read_mem.h1@evs.root: result_read_mem.h1.txt result_size_h1.txt bm_events_h1
	root -q -l 'bm_timing.C("result_read_mem.h1", "result_size_h1.txt", "MEMORY READ throughput $(NAME_h1)", "$@", $(shell cat bm_events_h1), -1, true)'

graph_read_mem.h1X10@evs.root: result_read_mem.h1X10.txt result_size_h1X10.txt bm_events_h1X10
	root -q -l 'bm_timing.C("result_read_mem.h1X10", "result_size_h1X10.txt", "MEMORY READ throughput $(NAME_h1X10)", "$@", $(shell cat bm_events_h1X10), -1, true)'


graph_read_ssd.lhcb@evs.root: result_read_ssd.lhcb.txt result_size_lhcb.txt bm_events_lhcb
	root -q -l 'bm_timing.C("result_read_ssd.lhcb", "result_size_lhcb.txt", "SSD READ throughput $(NAME_lhcb)", "$@", $(shell cat bm_events_lhcb), -1, true)'

graph_read_ssd.cms@evs.root: result_read_ssd.cms.txt result_size_cms.txt bm_events_cms
	root -q -l 'bm_timing.C("result_read_ssd.cms", "result_size_cms.txt", "SSD READ throughput $(NAME_cms)", "$@", $(shell cat bm_events_cms), -1, true)'

graph_read_ssd.h1X10@evs.root: result_read_ssd.h1X10.txt result_size_h1X10.txt bm_events_h1X10
	root -q -l 'bm_timing.C("result_read_ssd.h1X10", "result_size_h1X10.txt", "SSD READ throughput $(NAME_h1X10)", "$@", $(shell cat bm_events_h1X10), -1, true)'


graph_%.pdf: graph_%.root
	root -q -l 'bm_convert_to_pdf.C("graph_$*")'


### CLEAN ######################################################################

clean:
	rm -f util.o lhcb cms_dimuon gen_lhcb gen_cms ntuple_info tree_info
	rm -rf _make_ttjet_13tev_june2019*
	rm -rf include_cms
	rm -f libH1event.so libH1Dict.cxx
	rm -f AutoDict_*
