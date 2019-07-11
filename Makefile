CFLAGS_CUSTOM = -std=c99 -Wall -pthread -g -O2
CXXFLAGS_CUSTOM = -std=c++14 -Wall -pthread -Wall -g -O2
CXXFLAGS_ROOT = $(shell root-config --cflags)
LDFLAGS_CUSTOM =
LDFLAGS_ROOT = $(shell root-config --libs) -lTreePlayer -lROOTNTuple
CFLAGS = $(CFLAGS_CUSTOM)
CXXFLAGS = $(CXXFLAGS_CUSTOM) $(CXXFLAGS_ROOT)
LDFLAGS = $(LDFLAGS_CUSTOM) $(LDFLAGS_ROOT)

all: libiotrace.so iotrace_capture iotrace_test \
  lhcb_opendata \
  libEvent.so \
  precision_test \
	mkfaulty \
	fuse_forward

.PHONY = clean benchmarks benchmark_clean

mkfaulty: mkfaulty.cc
	g++ $(CXXFLAGS) -o mkfaulty mkfaulty.cc

iotrace.o: iotrace.c wire_format.h
	gcc $(CFLAGS) -fPIC -c iotrace.c

libiotrace.so: iotrace.o
	gcc -pthread -shared -Wl,-soname,libiotrace.so -o libiotrace.so iotrace.o -lc -ldl -lrt

iotrace_capture: capture.cc wire_format.h
	g++ $(CXXFLAGS) -o iotrace_capture capture.cc $(LDFLAGS)

iotrace_test: test.cc
	g++ $(CXXFLAGS) -o iotrace_test test.cc


event.cxx: event.h event_linkdef.h
	rootcling -f $@ event.h event_linkdef.h

libEvent.so: event.cxx
	g++ -shared -fPIC -o$@ $(CXXFLAGS) event.cxx $(LDFLAGS)

lhcb_opendata: lhcb_opendata.cc lhcb_opendata.h util.h util.o event.h
	g++ $(CXXFLAGS) -o lhcb_opendata lhcb_opendata.cc util.o $(LDFLAGS)


fuse_forward: fuse_forward.cc
	g++ $(CXXFLAGS_CUSTOM) -o fuse_forward fuse_forward.cc $(LDFLAGS_CUSTOM) \
	  -lfuse

precision_test: precision_test.cc
	g++ $(CXXFLAGS) -o precision_test precision_test.cc $(LDFLAGS)

util.o: util.cc util.h
	g++ $(CXXFLAGS_CUSTOM) -c util.cc

clear_page_cache: clear_page_cache.c
	gcc -Wall -g -o clear_page_cache clear_page_cache.c
	sudo chown root clear_page_cache
	sudo chmod 4755 clear_page_cache

BM_DATA_PREFIX = data/B2HHH
BM_SHORTDATA_PREFIX = data/Short/B2HHH
BM_FAULTYDATA_PREFIX = data/Faulty/B2HHH
BM_USBDATA_PATH = data/usb-storage/benchmark-root/lhcb/MagnetDown
BM_USBDATA_PREFIX = $(BM_USBDATA_PATH)/B2HHH
BM_EOSDATA_PATH = /eos/pps/users/jblomer
BM_EOSDATA_PREFIX = $(BM_EOSDATA_PATH)/B2HHH

BM_BITFLIP_NITER = 100
BM_BITFLIP_EXPECTED = 56619375330.364952

BM_IOPATTERN_SKIPCOLOR=38
BM_ASPECT_RATIO=0

benchmarks: graph_size.root \
	result_read_mem.graph.root \
	result_read_ssd.graph.root \
	result_read_hdd.graph.root



result_size.txt: bm_events bm_formats bm_size.sh
	./bm_size.sh > result_size.txt

result_size_overview.txt: bm_size.sh
	mv bm_formats bm_formats.save
	echo "root-inflated root-deflated root-lz4 root-lzma" > bm_formats
	./bm_size.sh > result_size_overview.txt
	mv bm_formats.save bm_formats

graph_size.root: result_size.txt bm_size.C
	root -q -l bm_size.C

graph_size_overview.root: result_size_overview.txt bm_size.C
	root -q -l 'bm_size.C("size_overview")'


result_bitflip.%.txt: lhcb_opendata mkfaulty bm_bitflip.sh
	$(BM_ENV_$*) ./bm_bitflip.sh -i $(BM_SHORTDATA_PREFIX).$* -o $(BM_FAULTYDATA_PREFIX).$* \
	  -n $(BM_BITFLIP_NITER) -e $(BM_BITFLIP_EXPECTED) -- \
		./lhcb_opendata$(BM_BINEXT_$*) -i $(BM_FAULTYDATA_PREFIX).$* -s | tee $@


result_iotrace.%.root: lhcb_opendata
	$(BM_ENV_$*) ./bm_iotrace.sh -o $@ -w B2HHH.$* ./lhcb_opendata$(BM_BINEXT_$*) -i $(BM_DATA_PREFIX).$*


result_iopattern_read.%.txt: fuse_forward lhcb_opendata
	$(BM_ENV_$*) ./bm_iopattern.sh -o $@ -w $(shell pwd)/$(BM_DATA_PREFIX).$* \
	  ./lhcb_opendata $(BM_BINEXT_$*) -i @MOUNT_DIR@/B2HHH.$*
	sed -i -e "1i# $* $(shell stat -c %s $(BM_DATA_PREFIX).$*)" $@

result_iopattern_plot.%.txt: fuse_forward lhcb_opendata
	$(BM_ENV_$*) ./bm_iopattern.sh -o $@ -w $(shell pwd)/$(BM_DATA_PREFIX).$* \
	  ./lhcb_opendata $(BM_BINEXT_$*) -i @MOUNT_DIR@/B2HHH.$* -p
	sed -i -e "1i# $* $(shell stat -c %s $(BM_DATA_PREFIX).$*)" $@


graph_iopattern_read.root: $(wildcard result_iopattern_read.*.txt)
	echo "$^"
	root -q -l 'bm_iopattern.C("$^", "$@", $(BM_IOPATTERN_SKIPCOLOR))'

graph_iopattern_plot.root: $(wildcard result_iopattern_plot.*.txt)
	echo "$^"
	root -q -l 'bm_iopattern.C("$^", "$@", $(BM_IOPATTERN_SKIPCOLOR))'



result_eostraffic_read.%.txt: lhcb_opendata
	$(BM_ENV_$*) ./bm_traffic.sh -o $@ -- ./lhcb_opendata$(BM_BINEXT_$*) -i $(BM_EOSDATA_PREFIX).$*

result_eostraffic_plot.%.txt: lhcb_opendata
	$(BM_ENV_$*) ./bm_traffic.sh -o $@ -- ./lhcb_opendata$(BM_BINEXT_$*) -i $(BM_EOSDATA_PREFIX).$* -p


result_read_mem.%.txt: lhcb_opendata
	BM_CACHED=1 $(BM_ENV_$*) ./bm_timing.sh $@ ./lhcb_opendata$(BM_BINEXT_$*) -i $(BM_DATA_PREFIX).$*

result_read_mem.%+times10.txt: lhcb_opendata
	BM_CACHED=1 $(BM_ENV_$*) ./bm_timing.sh $@ \
	  ./lhcb_opendata$(BM_BINEXT_$*) -i $(BM_DATA_PREFIX).times10.$*

result_read_mem.%~dataframe.txt: lhcb_opendata
	BM_CACHED=1 ./bm_timing.sh $@ ./lhcb_opendata -i $(BM_DATA_PREFIX).$* -f

result_read_mem.%~dataframe+times10.txt: lhcb_opendata
	BM_CACHED=1 ./bm_timing.sh $@ \
	  ./lhcb_opendata -i $(BM_DATA_PREFIX).times10.$* -f

result_read_mem.%~dataframemt.txt: lhcb_opendata
	BM_CACHED=1 ./bm_timing.sh $@ ./lhcb_opendata -i $(BM_DATA_PREFIX).$* -g

result_read_mem.%~dataframemt+times10.txt: lhcb_opendata
	BM_CACHED=1 ./bm_timing.sh $@ \
	  ./lhcb_opendata -i $(BM_DATA_PREFIX).times10.$* -g

result_read_mem.%~dataframenoht.txt: lhcb_opendata
	BM_CACHED=1 ./bm_timing.sh $@ ./lhcb_opendata -i $(BM_DATA_PREFIX).$* -G

result_read_mem.%~dataframenoht+times10.txt: lhcb_opendata
	BM_CACHED=1 ./bm_timing.sh $@ \
	  ./lhcb_opendata -i $(BM_DATA_PREFIX).times10.$* -G

result_read_mem.%~treereader.txt: lhcb_opendata
	BM_CACHED=1 ./bm_timing.sh $@ ./lhcb_opendata -i $(BM_DATA_PREFIX).$* -r

result_read_mem.%~treereader+times10.txt: lhcb_opendata
	BM_CACHED=1 ./bm_timing.sh $@ \
	  ./lhcb_opendata -i $(BM_DATA_PREFIX).times10.$* -r


result_read_ssd.%.txt: lhcb_opendata
	BM_CACHED=0 $(BM_ENV_$*) ./bm_timing.sh $@ ./lhcb_opendata$(BM_BINEXT_$*) -i $(BM_DATA_PREFIX).$*

result_read_ssd.%~treereader.txt: lhcb_opendata
	BM_CACHED=0 ./bm_timing.sh $@ ./lhcb_opendata -i $(BM_DATA_PREFIX).$* -r

result_read_ssd.%~dataframe.txt: lhcb_opendata
	BM_CACHED=0 ./bm_timing.sh $@ ./lhcb_opendata -i $(BM_DATA_PREFIX).$* -f

result_read_ssd.%~dataframemt.txt: lhcb_opendata
	BM_CACHED=0 ./bm_timing.sh $@ ./lhcb_opendata -i $(BM_DATA_PREFIX).$* -g

result_plot_ssd.%.txt: lhcb_opendata
	BM_CACHED=0 $(BM_ENV_$*) ./bm_timing.sh $@ ./lhcb_opendata$(BM_BINEXT_$*) -p -i $(BM_DATA_PREFIX).$*

result_read_hdd.%.txt: lhcb_opendata
	BM_CACHED=0 $(BM_ENV_$*) ./bm_timing.sh $@ ./lhcb_opendata$(BM_BINEXT_$*) -i $(BM_USBDATA_PREFIX).$*

result_read_eos.%.txt: lhcb_opendata
	BM_CACHED=0 $(BM_ENV_$*) ./bm_timing.sh $@ ./lhcb_opendata$(BM_BINEXT_$*) -i $(BM_EOSDATA_PREFIX).$*

result_read_eos.%~dataframe.txt: lhcb_opendata
	BM_CACHED=0 ./bm_timing.sh $@ ./lhcb_opendata -i $(BM_EOSDATA_PREFIX).$* -f

result_plot_eos.%.txt: lhcb_opendata
	BM_SLEEP=10 BM_CACHED=0 $(BM_ENV_$*) ./bm_timing.sh $@ \
	  ./lhcb_opendata$(BM_BINEXT_$*) -p -i $(BM_EOSDATA_PREFIX).$*

result_write_ssd.%.txt: lhcb_opendata
	BM_CACHED=1 $(BM_ENV_$*) ./bm_timing.sh $@ ./lhcb_opendata$(BM_BINEXT_$*) -i $(BM_DATA_PREFIX).root -o $*

result_write_hdd.%.txt: lhcb_opendata
	BM_CACHED=1 $(BM_ENV_$*) ./bm_timing.sh $@ ./lhcb_opendata$(BM_BINEXT_$*) -i $(BM_DATA_PREFIX).root -o $* \
		  -d $(BM_USBDATA_PATH)

result_eostraffic_read.txt: $(wildcard result_eostraffic_read.*.txt)
	BM_FIELD="RX_bytes" BM_RESULT_SET=result_eostraffic_read ./bm_combine.sh

result_eostraffic_plot.txt: $(wildcard result_eostraffic_plot.*.txt)
	BM_FIELD="RX_bytes" BM_RESULT_SET=result_eostraffic_plot ./bm_combine.sh


graph_read_mem~evs.root: $(wildcard result_read_mem.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_mem ./bm_combine.sh
	root -q -l 'bm_timing.C("result_read_mem", "READ throughput LHCb OpenData, warm cache", "$@", 9000000, true)'

graph_read_mem~mbs.root: $(wildcard result_read_mem.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_mem ./bm_combine.sh
	root -q -l 'bm_timing.C("result_read_mem", "READ throughput LHCb OpenData, warm cache", "$@", 1450, false)'

graph_read_ssd~evs.root: $(wildcard result_read_ssd.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_ssd ./bm_combine.sh
	root -q -l 'bm_timing.C("result_read_ssd", "READ throughput LHCb OpenData, SSD cold cache", "$@", 8000000, true, 0, $(BM_ASPECT_RATIO))'

graph_read_ssd~mbs.root: $(wildcard result_read_ssd.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_ssd ./bm_combine.sh
	root -q -l 'bm_timing.C("result_read_ssd", "READ throughput LHCb OpenData, SSD cold cache", "$@", 1450, false)'

graph_plot_ssd~evs.root: $(wildcard result_plot_ssd.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_plot_ssd ./bm_combine.sh
	root -q -l 'bm_timing.C("result_plot_ssd", "PLOT 2 VARIABLES throughput LHCb OpenData, SSD cold cache", "$@", 8000000, true, 0, $(BM_ASPECT_RATIO))'

graph_plot_ssd~mbs.root: $(wildcard result_plot_ssd.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_plot_ssd ./bm_combine.sh
	root -q -l 'bm_timing.C("result_plot_ssd", "PLOT 2 VARIABLES throughput LHCb OpenData, SSD cold cache", "$@", 1450, false)'

graph_read_hdd.root: $(wildcard result_read_hdd.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_hdd ./bm_combine.sh
	root -q -l 'bm_timing.C("result_read_hdd", "READ throughput LHCb OpenData, HDD cold cache", "$@", 1450)'

graph_read_hddXzoom.root: $(wildcard result_read_hdd.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_hdd ./bm_combine.sh
	root -q -l 'bm_timing.C("result_read_hdd", "READ throughput LHCb OpenData, HDD cold cache", "$@", -1)'

graph_read_eos~evs.root: $(wildcard result_read_eos.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_eos ./bm_combine.sh
	root -q -l 'bm_timing.C("result_read_eos", "READ throughput LHCb OpenData, EOS (LAN)", "$@", 8000000, true)'

graph_read_eosXzoom~evs.root: $(wildcard result_read_eos.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_eos ./bm_combine.sh
	root -q -l 'bm_timing.C("result_read_eos", "READ throughput LHCb OpenData, EOS (LAN)", "$@", 1000000, true)'

graph_read_eos~mbs.root: $(wildcard result_read_eos.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_eos ./bm_combine.sh
	root -q -l 'bm_timing.C("result_read_eos", "READ throughput LHCb OpenData, EOS (LAN)", "$@", 1450, false)'

graph_read_eosXzoom~mbs.root: $(wildcard result_read_eos.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_eos ./bm_combine.sh
	root -q -l 'bm_timing.C("result_read_eos", "READ throughput LHCb OpenData, EOS (LAN)", "$@", 150, false)'

graph_read_eos23.root: $(wildcard result_read_eos23.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_eos23 ./bm_combine.sh
	root -q -l 'bm_timing.C("result_read_eos23", "READ throughput LHCb OpenData, EOS (LAN)", "$@", -1)'

graph_read_eos33.root: $(wildcard result_read_eos33.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_eos33 ./bm_combine.sh
	root -q -l 'bm_timing.C("result_read_eos33", "READ throughput LHCb OpenData, EOS (+10ms)", "$@", -1)'

graph_read_eos43.root: $(wildcard result_read_eos43.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_eos43 ./bm_combine.sh
	root -q -l 'bm_timing.C("result_read_eos43", "READ throughput LHCb OpenData, EOS (+20ms)", "$@", -1)'

graph_write_ssd~mbs.root: $(wildcard result_write_ssd.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_write_ssd ./bm_combine.sh
	root -q -l 'bm_timing.C("result_write_ssd", "WRITE throughput LHCb OpenData, SSD", "$@", 230, false)'

graph_write_hdd.root: $(wildcard result_write_hdd.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_write_hdd ./bm_combine.sh
	root -q -l 'bm_timing.C("result_write_hdd", "WRITE throughput LHCb OpenData, HDD", "$@", 230)'


DETAIL_SERIALIZATION = result_read_mem.root-inflated.txt \
  result_read_mem.protobuf-inflated.txt \
	result_read_mem.rootrow-inflated~unfixed.txt \
	result_read_mem.rootrow-inflated~fixed.txt \
	result_write_ssd.root-inflated.txt \
  result_write_ssd.protobuf-inflated.txt \
	result_write_ssd.rootrow-inflated~unfixed.txt \
	result_write_ssd.rootrow-inflated~fixed.txt
detail_serialization.txt: $(DETAIL_SERIALIZATION)
	rm -f $@
	cat result_read_mem.root-inflated.txt | grep "^realtime:" | \
	  sed 's/realtime:/serialization-root-read/' >> $@
	cat result_read_mem.rootrow-inflated~unfixed.txt | grep "^realtime:" | \
	  sed 's/realtime:/serialization-rootrow~unfixed-read/' >> $@
	cat result_read_mem.rootrow-inflated~fixed.txt | grep "^realtime:" | \
	  sed 's/realtime:/serialization-rootrow~fixed-read/' >> $@
	cat result_read_mem.protobuf-inflated.txt | grep "^realtime:" | \
	  sed 's/realtime:/serialization-protobuf-read/' >> $@
	cat result_write_ssd.root-inflated.txt | grep "^realtime:" | \
	  sed 's/realtime:/serialization-root-write/' >> $@
	cat result_write_ssd.rootrow-inflated~unfixed.txt | grep "^realtime:" | \
	  sed 's/realtime:/serialization-rootrow~unfixed-write/' >> $@
	cat result_write_ssd.rootrow-inflated~fixed.txt | grep "^realtime:" | \
	  sed 's/realtime:/serialization-rootrow~fixed-write/' >> $@
	cat result_write_ssd.protobuf-inflated.txt | grep "^realtime:" | \
	  sed 's/realtime:/serialization-protobuf-write/' >> $@

graph_detail_serialization~evs.root: detail_serialization.txt
	root -q -l 'bm_timing.C("detail_serialization", "ROW-WISE throughput LHCb OpenData", "$@", 8000000, true, 1)'


graph_detail_flavor~evs.root: $(wildcard result_read_mem.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_mem ./bm_combine.sh
	sed -i -e 's/^/flavor-/' result_read_mem.txt
	root -q -l 'bm_timing.C("result_read_mem", "READ Throughput LHCb OpenData, warm cache", "$@", 8000000, true, 2)'

graph_detail_split~evs.root: $(wildcard result_read_mem.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_mem ./bm_combine.sh
	sed -i -e 's/^/split-/' result_read_mem.txt
	root -q -l 'bm_timing.C("result_read_mem", "READ Throughput LHCb OpenData, warm cache", "$@", 8000000, true, 4)'

graph_detail_splitpattern.root:
	for f in result_iopattern_read.*.txt; do sed -i -e 's/^# /# split-/' $$f; done
	$(MAKE) BM_IOPATTERN_SKIPCOLOR=0 graph_iopattern_read.root
	mv graph_iopattern_read.root $@


graph_%.pdf: graph_%.root
	root -q -l 'bm_convert_to_pdf.C("graph_$*")'

clean:
	rm -f libiotrace.so iotrace.o iotrace_capture iotrace.fanout iotrace_test \
	  util.o \
	  lhcb_opendata \
		mkfaulty \
		fuse_forward

benchmark_clean:
	rm -f result_* graph_*
