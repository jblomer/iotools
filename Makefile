CXXFLAGS_CUSTOM = -std=c++14 -Wall -pthread -Wall -g -O2
ifeq ($(shell root-config --cflags),)
  $(error Cannot find root-config. Please source thisroot.sh)
endif
CXXFLAGS_ROOT = $(shell root-config --cflags)
LDFLAGS_CUSTOM =
LDFLAGS_ROOT = $(shell root-config --libs) -lROOTNTuple -lROOTNTupleUtil
CXXFLAGS = $(CXXFLAGS_CUSTOM) $(CXXFLAGS_ROOT)
LDFLAGS = $(LDFLAGS_CUSTOM) $(LDFLAGS_ROOT)

MASTER_ROOT = /data/calibration/master
DATA_ROOT = /data/calibration
DATA_HOST = xrootd-io-test
DATA_REMOTE = http://$(DATA_HOST)/data
SAMPLE_lhcb = B2HHH
SAMPLE_cms = ttjet_13tev_june2019
SAMPLE_cmsX10 = ttjet_13tev_june2019X10
SAMPLE_h1 = h1dst
SAMPLE_h1X10 = h1dstX10
SAMPLE_atlas = gg_data
MASTER_lhcb = $(MASTER_ROOT)/$(SAMPLE_lhcb).root
MASTER_cms = $(MASTER_ROOT)/$(SAMPLE_cms).root
MASTER_h1 = $(MASTER_ROOT)/dstarmb.root $(MASTER_ROOT)/dstarp1a.root $(MASTER_ROOT)/dstarp1b.root $(MASTER_ROOT)/dstarp2.root
MASTER_atlas = $(MASTER_ROOT)/data_A.GamGam.root $(MASTER_ROOT)/data_B.GamGam.root $(MASTER_ROOT)/data_C.GamGam.root $(MASTER_ROOT)/data_D.GamGam.root
NAME_lhcb = LHCb Run 1 Open Data B2HHH
NAME_cms = CMS nanoAOD TTJet 13TeV June 2019
NAME_cmsX10 = CMS nanoAOD TTJet 13TeV June 2019 [x10]
NAME_h1 = H1 micro DST
NAME_h1X10 = H1 micro DST [x10]
NAME_atlas = ATLAS 2020 OpenData Hgg

COMPRESSION_none = 0
COMPRESSION_lz4 = 404
COMPRESSION_zlib = 101
COMPRESSION_lzma = 207
COMPRESSION_zstd = 505

NET_DEV = eth0

.PHONY = all benchmarks clean data data_lhcb data_cms data_h1
all: lhcb cms h1 gen_lhcb prepare_cms gen_cms gen_h1 ntuple_info tree_info \
	fuse_forward clock check-uring

benchmarks: lhcb h1 cms atlas
generators: gen_lhcb gen_h1 gen_cms gen_atlas

### DATA #######################################################################

data: data_lhcb data_cms data_h1 data_atlas

data_lhcb: $(DATA_ROOT)/$(SAMPLE_lhcb)~none.root \
	$(DATA_ROOT)/$(SAMPLE_lhcb)~lz4.root \
	$(DATA_ROOT)/$(SAMPLE_lhcb)~zstd.root \
	$(DATA_ROOT)/$(SAMPLE_lhcb)~lzma.root \
	$(DATA_ROOT)/$(SAMPLE_lhcb)~none.ntuple \
	$(DATA_ROOT)/$(SAMPLE_lhcb)~lz4.ntuple \
	$(DATA_ROOT)/$(SAMPLE_lhcb)~zstd.ntuple \
	$(DATA_ROOT)/$(SAMPLE_lhcb)~lzma.ntuple

data_cms: $(DATA_ROOT)/$(SAMPLE_cms)~none.root \
	$(DATA_ROOT)/$(SAMPLE_cms)~lz4.root \
	$(DATA_ROOT)/$(SAMPLE_cms)~zstd.root \
	$(DATA_ROOT)/$(SAMPLE_cms)~lzma.root \
	$(DATA_ROOT)/$(SAMPLE_cms)~none.ntuple \
	$(DATA_ROOT)/$(SAMPLE_cms)~lz4.ntuple \
	$(DATA_ROOT)/$(SAMPLE_cms)~zstd.ntuple \
	$(DATA_ROOT)/$(SAMPLE_cms)~lzma.ntuple

data_h1: $(DATA_ROOT)/$(SAMPLE_h1X10)~none.root \
	$(DATA_ROOT)/$(SAMPLE_h1X10)~lz4.root \
	$(DATA_ROOT)/$(SAMPLE_h1X10)~zstd.root \
	$(DATA_ROOT)/$(SAMPLE_h1X10)~lzma.root \
	$(DATA_ROOT)/$(SAMPLE_h1X10)~none.ntuple \
	$(DATA_ROOT)/$(SAMPLE_h1X10)~lz4.ntuple \
	$(DATA_ROOT)/$(SAMPLE_h1X10)~zstd.ntuple \
	$(DATA_ROOT)/$(SAMPLE_h1X10)~lzma.ntuple

data_atlas: $(DATA_ROOT)/$(SAMPLE_atlas)~none.root \
	$(DATA_ROOT)/$(SAMPLE_atlas)~lz4.root \
	$(DATA_ROOT)/$(SAMPLE_atlas)~zstd.root \
	$(DATA_ROOT)/$(SAMPLE_atlas)~lzma.root \
	$(DATA_ROOT)/$(SAMPLE_atlas)~none.ntuple \
	$(DATA_ROOT)/$(SAMPLE_atlas)~lz4.ntuple \
	$(DATA_ROOT)/$(SAMPLE_atlas)~zstd.ntuple \
	$(DATA_ROOT)/$(SAMPLE_atlas)~lzma.ntuple

gen_lhcb: gen_lhcb.cxx util.o
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

prepare_cms: prepare_cms.cxx
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

gen_cms: gen_cms.cxx util.o
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

gen_cmsraw: gen_cmsraw.cxx util.o
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

gen_h1: gen_h1.cxx util.o
	g++ $(CXXFLAGS) -o $@ $< util.o $(LDFLAGS)

gen_atlas: gen_atlas.cxx util.o
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

inspect: inspect.cc
	g++ $(CXXFLAS) -o $@ $^ $(LDFLAGS)


$(DATA_ROOT)/$(SAMPLE_lhcb)~none.root: $(MASTER_lhcb)
	hadd -O -f0 $@ $<

$(DATA_ROOT)/$(SAMPLE_lhcb)~%.root: $(DATA_ROOT)/$(SAMPLE_lhcb)~none.root
	hadd -O -f$(COMPRESSION_$*) $@ $<

$(DATA_ROOT)/$(SAMPLE_lhcb)~%.ntuple: $(DATA_ROOT)/$(SAMPLE_lhcb)~none.root gen_lhcb
	./gen_lhcb -i $< -o $(shell dirname $@) -c $*


$(DATA_ROOT)/$(SAMPLE_atlas)~none.root: $(MASTER_atlas)
	hadd -O -f0 $@ $^

$(DATA_ROOT)/$(SAMPLE_atlas)~%.root: $(DATA_ROOT)/$(SAMPLE_atlas)~none.root
	hadd -O -f$(COMPRESSION_$*) $@ $<

$(DATA_ROOT)/$(SAMPLE_atlas)~%.ntuple: $(DATA_ROOT)/$(SAMPLE_atlas)~none.root gen_atlas
	./gen_atlas -i $< -o $(shell dirname $@) -c $*


$(DATA_ROOT)/$(SAMPLE_h1)~none.root: $(MASTER_h1)
	hadd -O -f0 $@ $^

$(DATA_ROOT)/$(SAMPLE_h1X10)~%.root: $(DATA_ROOT)/$(SAMPLE_h1)~none.root
	hadd -O -f$(COMPRESSION_$*) $@ $< $< $< $< $< $< $< $< $< $<

$(DATA_ROOT)/$(SAMPLE_h1X10)~%.ntuple: $(DATA_ROOT)/$(SAMPLE_h1)~none.root gen_h1
	./gen_h1 -b10 -o $(shell dirname $@) -c $* $<


$(DATA_ROOT)/$(SAMPLE_cms)~none.root: $(MASTER_cms)
	hadd -O -f0 $@ $<

$(DATA_ROOT)/$(SAMPLE_cms)~%.root: $(DATA_ROOT)/$(SAMPLE_cms)~none.root
	hadd -O -f$(COMPRESSION_$*) $@ $<

$(DATA_ROOT)/$(SAMPLE_cms)~%.ntuple: $(DATA_ROOT)/$(SAMPLE_cms)~none.root gen_cms
	./gen_cms -i $< -o $(shell dirname $@) -c $*


### BINARIES ###################################################################

ntuple_info: ntuple_info.C libH1event.so
	g++ $(CXXFLAGS) -o $@ $< $(LDFLAGS)

ntuple_dump: ntuple_dump.C
	g++ $(CXXFLAGS) -o $@ $< $(LDFLAGS)

tree_info: tree_info.C
	g++ $(CXXFLAGS) -o $@ $< $(LDFLAGS)


cms: cms.cxx util.o
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

lhcb: lhcb.cxx util.o
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

h1: h1.cxx util.o
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

atlas: atlas.cxx util.o
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

util.o: util.cc util.h
	g++ $(CXXFLAGS) -c $<

clock: clock.cxx
	g++ $(CXXFLAGS) -o $@ $< $(LDFLAGS)

check-uring: check-uring.c
	gcc -o $@ $<


fuse_forward: fuse_forward.cxx
	g++ $(CXXFLAGS_CUSTOM) -o $@ $< $(LDFLAGS_CUSTOM) -lfuse


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
		./lhcb -i $(DATA_ROOT)/$(SAMPLE_lhcb)~$*

result_read_mem.lhcb+rdf~%.txt: lhcb
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./lhcb -r -i $(DATA_ROOT)/$(SAMPLE_lhcb)~$*

result_read_mem.lhcb+mmap~%.txt: lhcb
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./lhcb -m -i $(DATA_ROOT)/$(SAMPLE_lhcb)~$*

result_read_optane.lhcb~%.txt: lhcb
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./lhcb -i $(DATA_ROOT)/$(SAMPLE_lhcb)~$*

result_read_optane.lhcb+mmap~%.txt: lhcb
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./lhcb -m -i $(DATA_ROOT)/$(SAMPLE_lhcb)~$*

result_read_ssd.atlas~%.txt: atlas
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		  ./atlas -i $(DATA_ROOT)/$(SAMPLE_atlas)~$*

result_read_ssd.lhcb~%.txt: lhcb
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./lhcb -i $(DATA_ROOT)/$(SAMPLE_lhcb)~$*

result_read_ssd.lhcb+rdf~%.txt: lhcb
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./lhcb -r -i $(DATA_ROOT)/$(SAMPLE_lhcb)~$*

result_read_ssd.lhcb+mmap~%.txt: lhcb
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./lhcb -m -i $(DATA_ROOT)/$(SAMPLE_lhcb)~$*

result_read_ssd.lhcb+N%~none.ntuple.txt: lhcb
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./lhcb -c $* -i $(DATA_ROOT)/$(SAMPLE_lhcb)~none.ntuple

result_read_ssd.lhcb+N%~zstd.ntuple.txt: lhcb
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./lhcb -c $* -i $(DATA_ROOT)/$(SAMPLE_lhcb)~zstd.ntuple

result_read_hdd.lhcb~%.txt: lhcb
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./lhcb -i $(DATA_ROOT)/$(SAMPLE_lhcb)~$*

result_read_hdd.lhcb+rdf~%.txt: lhcb
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./lhcb -r -i $(DATA_ROOT)/$(SAMPLE_lhcb)~$*

result_read_http.lhcb~%.txt: lhcb
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./lhcb -i $(DATA_REMOTE)/$(SAMPLE_lhcb)~$*

result_read_http.lhcb+%ms~zstd.root.txt: lhcb
	./add_latency $(NET_DEV) $*
	ping -c1 $(DATA_HOST)
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./lhcb -i $(DATA_REMOTE)/$(SAMPLE_lhcb)~zstd.root
	./add_latency $(NET_DEV) 0

result_read_http.lhcb+%ms~zstd.ntuple.txt: lhcb
	./add_latency $(NET_DEV) $*
	ping -c1 $(DATA_HOST)
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./lhcb -i $(DATA_REMOTE)/$(SAMPLE_lhcb)~zstd.ntuple
	./add_latency $(NET_DEV) 0


result_read_mem.cms~%.txt: cms
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./cms -i $(DATA_ROOT)/$(SAMPLE_cms)~$*

result_read_mem.cms+rdf~%.txt: cms
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./cms -r -i $(DATA_ROOT)/$(SAMPLE_cms)~$*

result_read_mem.cms+mmap~%.txt: cms
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./cms -m -i $(DATA_ROOT)/$(SAMPLE_cms)~$*

result_read_optane.cms~%.txt: cms
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./cms -i $(DATA_ROOT)/$(SAMPLE_cms)~$*

result_read_optane.cms+mmap~%.txt: cms
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./cms -m -i $(DATA_ROOT)/$(SAMPLE_cms)~$*

result_read_ssd.cms~%.txt: cms
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./cms -i $(DATA_ROOT)/$(SAMPLE_cms)~$*

result_read_ssd.cms+rdf~%.txt: cms
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./cms -r -i $(DATA_ROOT)/$(SAMPLE_cms)~$*

result_read_ssd.cms+mmap~%.txt: cms
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./cms -m -i $(DATA_ROOT)/$(SAMPLE_cms)~$*

result_read_ssd.cms+N%~none.ntuple.txt: cms
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./cms -c $* -i $(DATA_ROOT)/$(SAMPLE_cms)~none.ntuple

result_read_ssd.cms+N%~zstd.ntuple.txt: cms
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./cms -c $* -i $(DATA_ROOT)/$(SAMPLE_cms)~zstd.ntuple

result_read_hdd.cms~%.txt: cms
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./cms -i $(DATA_ROOT)/$(SAMPLE_cms)~$*

result_read_hdd.cms+rdf~%.txt: cms
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./cms -r -i $(DATA_ROOT)/$(SAMPLE_cms)~$*

result_read_http.cms~%.txt: cms
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./cms -i $(DATA_REMOTE)/$(SAMPLE_cms)~$*

result_read_http.cms+%ms~zstd.root.txt: cms
	./add_latency $(NET_DEV) $*
	ping -c1 $(DATA_HOST)
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./cms -i $(DATA_REMOTE)/$(SAMPLE_cms)~zstd.root
	./add_latency $(NET_DEV) 0

result_read_http.cms+%ms~zstd.ntuple.txt: cms
	./add_latency $(NET_DEV) $*
	ping -c1 $(DATA_HOST)
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./cms -i $(DATA_REMOTE)/$(SAMPLE_cms)~zstd.ntuple
	./add_latency $(NET_DEV) 0



result_read_mem.h1X10~%.txt: h1
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./h1 -i $(DATA_ROOT)/$(SAMPLE_h1X10)~$*

result_read_mem.h1X10+rdf~%.txt: h1
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./h1 -r -i $(DATA_ROOT)/$(SAMPLE_h1X10)~$*

result_read_mem.h1X10+mmap~%.txt: h1
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./h1 -m -i $(DATA_ROOT)/$(SAMPLE_h1X10)~$*

result_read_optane.h1X10~%.txt: h1
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./h1 -i $(DATA_ROOT)/$(SAMPLE_h1X10)~$*

result_read_optane.h1X10+mmap~%.txt: h1
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./h1 -m -i $(DATA_ROOT)/$(SAMPLE_h1X10)~$*

result_read_ssd.h1X10~%.txt: h1
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./h1 -i $(DATA_ROOT)/$(SAMPLE_h1X10)~$*

result_read_ssd.h1X10+rdf~%.txt: h1
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./h1 -r -i $(DATA_ROOT)/$(SAMPLE_h1X10)~$*

result_read_ssd.h1X10+mmap~%.txt: h1
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./h1 -m -i $(DATA_ROOT)/$(SAMPLE_h1X10)~$*

result_read_ssd.h1X10+N%~none.ntuple.txt: h1
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./h1 -c $* -i $(DATA_ROOT)/$(SAMPLE_h1X10)~none.ntuple

result_read_ssd.h1X10+N%~zstd.ntuple.txt: h1
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./h1 -c $* -i $(DATA_ROOT)/$(SAMPLE_h1X10)~zstd.ntuple

result_read_hdd.h1X10~%.txt: h1
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./h1 -i $(DATA_ROOT)/$(SAMPLE_h1X10)~$*

result_read_hdd.h1X10+rdf~%.txt: h1
	BM_CACHED=0 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./h1 -r -i $(DATA_ROOT)/$(SAMPLE_h1X10)~$*

result_read_http.h1X10~%.txt: h1
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./h1 -i $(DATA_REMOTE)/$(SAMPLE_h1X10)~$*

result_read_http.h1X10+%ms~zstd.root.txt: h1
	./add_latency $(NET_DEV) $*
	ping -c1 $(DATA_HOST)
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./h1 -i $(DATA_REMOTE)/$(SAMPLE_h1X10)~zstd.root
	./add_latency $(NET_DEV) 0

result_read_http.h1X10+%ms~zstd.ntuple.txt: h1
	./add_latency $(NET_DEV) $*
	ping -c1 $(DATA_HOST)
	BM_CACHED=1 BM_GREP=Runtime-Analysis: ./bm_timing.sh $@ \
		./h1 -i $(DATA_REMOTE)/$(SAMPLE_h1X10)~zstd.ntuple
	./add_latency $(NET_DEV) 0


result_read_%.txt: # result_read_%~*.txt
	BM_OUTPUT=$@ BM_FIELD=realtime BM_RESULT_SET=result_read_$* ./bm_combine.sh

result_media.txt: result_read_hdd.*~zstd.*.txt \
	result_read_http.*+10ms~zstd.*.txt \
	result_read_ssd.h1X10~zstd.*.txt result_read_ssd.cms~zstd.*.txt result_read_ssd.lhcb~zstd.*.txt
	BM_OUTPUT=$@ BM_FIELD=realtime ./bm_medium.sh $^

result_streams.txt: result_read_ssd.*+N*.ntuple.txt \
	result_read_mem.lhcb~none.ntuple.txt \
	result_read_mem.cms~none.ntuple.txt  \
	result_read_mem.h1X10~none.ntuple.txt \
	result_read_mem.lhcb~zstd.ntuple.txt \
	result_read_mem.cms~zstd.ntuple.txt  \
	result_read_mem.h1X10~zstd.ntuple.txt
	BM_OUTPUT=$@ BM_FIELD=realtime ./bm_streams.sh $^

result_mmap.txt: result_read_optane.*~none.ntuple.txt \
	result_read_ssd.*+N16~none.ntuple.txt \
	result_read_ssd.*+mmap~none.ntuple.txt \
	result_read_mem.*~none.ntuple.txt
	BM_OUTPUT=$@ BM_FIELD=realtime ./bm_mmap.sh $^

result_ssd.txt: result_read_ssd.*~none.root.txt \
	result_read_ssd.*~zstd.root.txt \
	result_read_ssd.*+N16~none.ntuple.txt \
	result_read_ssd.*+N16~zstd.ntuple.txt
	BM_OUTPUT=$@ BM_FIELD=realtime ./bm_ssd.sh $^


graph_size.%.root: result_size_%.txt
	root -q -l -b 'bm_size.C("$*", "Storage Efficiency $(NAME_$*)")'


graph_read_mem.lhcb@evs.root: result_read_mem.lhcb.txt result_size_lhcb.txt bm_events_lhcb
	root -q -l -b 'bm_timing.C("result_read_mem.lhcb", "result_size_lhcb.txt", "MEMORY CACHED READ throughput $(NAME_lhcb)", "$@", $(shell cat bm_events_lhcb), -1, true)'

graph_read_mem.cms@evs.root: result_read_mem.cms.txt result_size_cms.txt bm_events_cms
	root -q -l -b 'bm_timing.C("result_read_mem.cms", "result_size_cms.txt", "MEMORY CACHED READ throughput $(NAME_cms)", "$@", $(shell cat bm_events_cms), -1, true)'

graph_read_mem.h1@evs.root: result_read_mem.h1.txt result_size_h1.txt bm_events_h1
	root -q -l -b 'bm_timing.C("result_read_mem.h1", "result_size_h1.txt", "MEMORY CACHED READ throughput $(NAME_h1)", "$@", $(shell cat bm_events_h1), -1, true)'

graph_read_mem.h1X10@evs.root: result_read_mem.h1X10.txt result_size_h1X10.txt bm_events_h1X10
	root -q -l -b 'bm_timing.C("result_read_mem.h1X10", "result_size_h1X10.txt", "MEMORY CACHED READ throughput $(NAME_h1X10)", "$@", $(shell cat bm_events_h1X10), -1, true)'


graph_read_ssd.lhcb@evs.root: result_read_ssd.lhcb.txt result_size_lhcb.txt bm_events_lhcb
	root -q -l -b 'bm_timing.C("result_read_ssd.lhcb", "result_size_lhcb.txt", "SSD READ throughput $(NAME_lhcb)", "$@", $(shell cat bm_events_lhcb), -1, true)'

graph_read_ssd.cms@evs.root: result_read_ssd.cms.txt result_size_cms.txt bm_events_cms
	root -q -l -b 'bm_timing.C("result_read_ssd.cms", "result_size_cms.txt", "SSD READ throughput $(NAME_cms)", "$@", $(shell cat bm_events_cms), -1, true)'

graph_read_ssd.h1X10@evs.root: result_read_ssd.h1X10.txt result_size_h1X10.txt bm_events_h1X10
	root -q -l -b 'bm_timing.C("result_read_ssd.h1X10", "result_size_h1X10.txt", "SSD READ throughput $(NAME_h1X10)", "$@", $(shell cat bm_events_h1X10), -1, true)'


graph_read_hdd.lhcb@evs.root: result_read_hdd.lhcb.txt result_size_lhcb.txt bm_events_lhcb
	root -q -l 'bm_timing.C("result_read_hdd.lhcb", "result_size_lhcb.txt", "HDD READ throughput $(NAME_lhcb)", "$@", $(shell cat bm_events_lhcb), -1, true)'

graph_read_hdd.cms@evs.root: result_read_hdd.cms.txt result_size_cms.txt bm_events_cms
	root -q -l 'bm_timing.C("result_read_hdd.cms", "result_size_cms.txt", "HDD READ throughput $(NAME_cms)", "$@", $(shell cat bm_events_cms), -1, true)'

graph_read_hdd.h1X10@evs.root: result_read_hdd.h1X10.txt result_size_h1X10.txt bm_events_h1X10
	root -q -l 'bm_timing.C("result_read_hdd.h1X10", "result_size_h1X10.txt", "HDD READ throughput $(NAME_h1X10)", "$@", $(shell cat bm_events_h1X10), -1, true)'


#graph_read_http.lhcb@evs.root: result_read_http.lhcb.txt result_size_lhcb.txt bm_events_lhcb
#	root -q -l 'bm_timing.C("result_read_hdd.lhcb", "result_size_lhcb.txt", "HDD READ throughput $(NAME_lhcb)", "$@", $(shell cat bm_events_lhcb), -1, true)'
#
#graph_read_http.cms@evs.root: result_read_hdd.cms.txt result_size_cms.txt bm_events_cms
#	root -q -l 'bm_timing.C("result_read_hdd.cms", "result_size_cms.txt", "HDD READ throughput $(NAME_cms)", "$@", $(shell cat bm_events_cms), -1, true)'
#
#graph_read_http.h1X10@evs.root: result_read_hdd.h1X10.txt result_size_h1X10.txt bm_events_h1X10
#	root -q -l 'bm_timing.C("result_read_hdd.h1X10", "result_size_h1X10.txt", "HDD READ throughput $(NAME_h1X10)", "$@", $(shell cat bm_events_h1X10), -1, true)'


# graph_mmap_mem.root: result_mmap_mem.txt result_size_*.txt
# 	root -q -l 'bm_mmap.C("result_mmap_$*", )'

graph_media.root: result_media.txt
	root -q -l -b 'bm_medium.C("result_media", "READ throughput using different physical data sources (zstd compressed)", "$@")'

graph_streams.root: result_streams.txt
	root -q -l -b 'bm_streams.C("result_streams", "RNTuple SSD READ throughput using concurrent streams", "$@")'

graph_mmap.root: result_mmap.txt
	root -q -l -b 'bm_mmap.C("result_mmap", "RNTuple OPTANE NVDIMM READ throughput uncompressed data with read() and mmap()", "$@")'

graph_ssd.root: result_ssd.txt
	root -q -l -b 'bm_ssd.C("result_ssd", "Read throuput from SSD TTree vs. RNTuple", "$@")'


graph_%.pdf: graph_%.root
	# root -q -l -b 'bm_convert_to_pdf.C("graph_$*")'


### CLEAN ######################################################################

clean:
	rm -f util.o cms_dimuon ntuple_info ntuple_dump tree_info fuse_forward clock
	rm -f cms atlas lhcb h1 gen_lhcb gen_atlas gen_cms gen_h1
	rm -f AutoDict_*
