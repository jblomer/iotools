CFLAGS_CUSTOM = -std=c99 -Wall -pthread -g -O2
CXXFLAGS_CUSTOM = -std=c++11 -Wall -pthread -Wall -g -O2 \
		  -I/opt/avro-c-1.8.2/include \
		  -I/opt/parquet-cpp-1.2.0/include
CXXFLAGS_ROOT = $(shell root-config --cflags)
LDFLAGS_CUSTOM = -L/opt/avro-c-1.8.2/lib \
		 -L/opt/parquet-cpp-1.2.0/lib64
LIBS_PARQUET = -lthrift -lboost_regex
LDFLAGS_ROOT = $(shell root-config --libs) -lTreePlayer
CFLAGS = $(CFLAGS_CUSTOM)
CXXFLAGS = $(CXXFLAGS_CUSTOM) $(CXXFLAGS_ROOT)
LDFLAGS = $(LDFLAGS_CUSTOM) $(LDFLAGS_ROOT)

ROOTSYS_LZ4 = /opt/root_lz4
CXXFLAGS_ROOT_LZ4 = $(shell $(ROOTSYS_LZ4)/bin/root-config --cflags) -DHAS_LZ4
LDFLAGS_ROOT_LZ4 = $(shell $(ROOTSYS_LZ4)/bin/root-config --libs) -lTreePlayer

AVRO_TOOLS=/opt/avro-java-1.8.2/avro-tools-1.8.2.jar

all: lhcb_opendata libEvent.so
#all: libiotrace.so iotrace_capture iotrace_test \
#  atlas_aod \
#	lhcbOpenData.class \
#  lhcb_opendata lhcb_opendata.lz4 \
#  libEvent.so \
#  precision_test \
#	mkfaulty \
#	fuse_forward \
#	avro-java/lhcb/cern/ch/Event.class

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


avro-java/lhcb/cern/ch/Event.java: avro-java/schema.json
	java -jar $(AVRO_TOOLS) compile schema avro-java/schema.json avro-java/

avro-java/lhcb/cern/ch/Event.class: avro-java/lhcb/cern/ch/Event.java
	cd avro-java/lhcb/cern/ch && javac Event.java

lhcbOpenData.class: lhcbOpenData.java avro-java/lhcb/cern/ch/Event.class
	CLASSPATH=avro-java:$$CLASSPATH javac lhcbOpenData.java


event.cxx: event.h event_linkdef.h
	rootcling -f $@ event.h event_linkdef.h

libEvent.so: event.cxx
	g++ -shared -fPIC -o$@ $(CXXFLAGS) event.cxx $(LDFLAGS)

lhcb_opendata.pb.cc: lhcb_opendata.proto
	protoc --cpp_out=. lhcb_opendata.proto

lhcb_opendata: lhcb_opendata.cc lhcb_opendata.h util.h util.o lhcb_opendata.pb.cc event.h
	g++ $(CXXFLAGS) -o lhcb_opendata lhcb_opendata.cc lhcb_opendata.pb.cc util.o \
		-lhdf5 -lhdf5_hl -lsqlite3 -lavro -lprotobuf $(LDFLAGS) -lz $(LIBS_PARQUET)

lhcb_opendata.lz4: lhcb_opendata.cc lhcb_opendata.h util.h util.o lhcb_opendata.pb.cc
	g++ $(CXXFLAGS_CUSTOM) $(CXXFLAGS_ROOT_LZ4) -o $@ \
		lhcb_opendata.cc lhcb_opendata.pb.cc util.o \
		-lhdf5 -lhdf5_hl -lsqlite3 -lavro -lprotobuf \
		$(LDFLAGS_CUSTOM) $(LDFLAGS_ROOT_LZ4) \
		-lz $(LIBS_PARQUET)



schema_aod/aod.cxx: $(wildcard schema_aod/*.h)
	cd schema_aod && \
	  rootcling -f aod.cxx xAOD__PhotonAuxContainer_v3.h aod_linkdef.h

schema_aod/libAod.so: schema_aod/aod.cxx
	cd schema_aod && \
		g++ -shared -fPIC -olibAod.so $(CXXFLAGS) aod.cxx $(LDFLAGS)

atlas_aod: atlas_aod.cc util.h util.o schema_aod/libAod.so
	g++ $(CXXFLAGS) -Ischema_aod -o $@ atlas_aod.cc util.o $(LDFLAGS)



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

BM_DATA_PREFIX = data/lhcb/MagnetDown/B2HHH
BM_SHORTDATA_PREFIX = data/lhcb/Short/B2HHH
BM_FAULTYDATA_PREFIX = data/lhcb/Faulty/B2HHH
BM_USBDATA_PATH = data/usb-storage/benchmark-root/lhcb/MagnetDown
BM_USBDATA_PREFIX = $(BM_USBDATA_PATH)/B2HHH
BM_EOSDATA_PATH = /eos/pps/users/jblomer
BM_EOSDATA_PREFIX = $(BM_EOSDATA_PATH)/B2HHH

BM_BINEXT_root-lz4 = .lz4
BM_ENV_root-lz4 = LD_LIBRARY_PATH=/opt/root_lz4/lib:$$LD_LIBRARY_PATH

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
	echo "root-inflated root-deflated root-lz4 root-lzma protobuf-inflated protobuf-deflated sqlite h5 parquet-inflated parquet-deflated avro-inflated avro-deflated" > bm_formats
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

result_iopattern_read.%~java.txt: fuse_forward lhcbOpenData.class
	CLASSPATH=avro-java:$$CLASSPATH ./bm_iopattern.sh -o $@ -w $(shell pwd)/$(BM_DATA_PREFIX).$* \
	  java lhcbOpenData @MOUNT_DIR@/B2HHH.$*
	sed -i -e "1i# $* $(shell stat -c %s $(BM_DATA_PREFIX).$*)" $@

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

graph_iopattern_read@acat.root:
	rm -f result_iopattern_read.*
	cp acat_result_all/result_iopattern_read.* .
	$(MAKE) graph_iopattern_read.root
	mv graph_iopattern_read.root $@

graph_iopattern_plot@acat.root:
	rm -f result_iopattern_plot.*
	cp acat_result_all/result_iopattern_plot.* .
	$(MAKE) graph_iopattern_plot.root
	mv graph_iopattern_plot.root $@

graph_read_mem~evs@acat.root:
	rm -f result_read_mem.*
	cp acat_result_all/result_read_mem.* .
	rm -f result_read_mem.avro-inflated.txt result_read_mem.avro-deflated.txt
	$(MAKE) graph_read_mem~evs.root
	mv graph_read_mem~evs.root $@

graph_read_ssd~evs@acat.root:
	rm -f result_read_ssd.*
	cp acat_result_all/result_read_ssd.* .
	rm -f result_read_ssd.avro-inflated.txt result_read_ssd.avro-deflated.txt
	$(MAKE) graph_read_ssd~evs.root
	mv graph_read_ssd~evs.root $@

graph_read_ssdXslim~evs@acat.root:
	rm -f result_read_ssd.*
	cp acat_result_all/result_read_ssd.* .
	rm -f result_read_ssd.avro-inflated.txt result_read_ssd.avro-deflated.txt
	$(MAKE) BM_ASPECT_RATIO=1 graph_read_ssd~evs.root
	mv graph_read_ssd~evs.root $@

graph_read_ssd~mbs@acat.root:
	rm -f result_read_ssd.*
	cp acat_result_all/result_read_ssd.* .
	rm -f result_read_ssd.avro-inflated.txt result_read_ssd.avro-deflated.txt
	$(MAKE) graph_read_ssd~mbs.root
	mv graph_read_ssd~mbs.root $@

graph_plot_ssd~evs@acat.root:
	rm -f result_plot_ssd.*
	cp acat_result_all/result_plot_ssd.* .
	rm -f result_plot_ssd.avro-inflated.txt result_plot_ssd.avro-deflated.txt
	$(MAKE) graph_plot_ssd~evs.root
	mv graph_plot_ssd~evs.root $@

graph_plot_ssdXslim~evs@acat.root:
	rm -f result_plot_ssd.*
	cp acat_result_all/result_plot_ssd.* .
	rm -f result_plot_ssd.avro-inflated.txt result_plot_ssd.avro-deflated.txt
	$(MAKE) BM_ASPECT_RATIO=1 graph_plot_ssd~evs.root
	mv graph_plot_ssd~evs.root $@

graph_read_eos~evs@acat.root:
	rm -f result_read_eos.*
	cp acat_result_all/result_read_eos.* .
	rm -f result_read_eos.avro-inflated.txt result_read_eos.avro-deflated.txt
	$(MAKE) graph_read_eos~evs.root
	mv graph_read_eos~evs.root $@

graph_read_eosXzoom~evs@acat.root:
	rm -f result_read_eos.*
	cp acat_result_all/result_read_eos.* .
	rm -f result_read_eos.avro-inflated.txt result_read_eos.avro-deflated.txt
	$(MAKE) graph_read_eosXzoom~evs.root
	mv graph_read_eosXzoom~evs.root $@

graph_read_eosXzoom~mbs@acat.root:
	rm -f result_read_eos.*
	cp acat_result_all/result_read_eos.* .
	rm -f result_read_eos.avro-inflated.txt result_read_eos.avro-deflated.txt
	$(MAKE) graph_read_eosXzoom~mbs.root
	mv graph_read_eosXzoom~mbs.root $@



result_eostraffic_read.%.txt: lhcb_opendata
	$(BM_ENV_$*) ./bm_traffic.sh -o $@ -- ./lhcb_opendata$(BM_BINEXT_$*) -i $(BM_EOSDATA_PREFIX).$*

result_eostraffic_plot.%.txt: lhcb_opendata
	$(BM_ENV_$*) ./bm_traffic.sh -o $@ -- ./lhcb_opendata$(BM_BINEXT_$*) -i $(BM_EOSDATA_PREFIX).$* -p


result_read_mem.%.txt: lhcb_opendata
	BM_CACHED=1 $(BM_ENV_$*) ./bm_timing.sh $@ ./lhcb_opendata$(BM_BINEXT_$*) -i $(BM_DATA_PREFIX).$*

result_read_mem.%+times10.txt: lhcb_opendata
	BM_CACHED=1 $(BM_ENV_$*) ./bm_timing.sh $@ \
	  ./lhcb_opendata$(BM_BINEXT_$*) -i $(BM_DATA_PREFIX).times10.$*

result_read_mem.%~java.txt: lhcbOpenData.class
	BM_CACHED=1 CLASSPATH=avro-java:$$CLASSPATH ./bm_timing.sh $@ java lhcbOpenData $(BM_DATA_PREFIX).$*

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

result_read_ssd.%~java.txt: lhcb_opendata
	BM_CACHED=1 CLASSPATH=avro-java:$$CLASSPATH ./bm_timing.sh $@ java lhcbOpenData $(BM_DATA_PREFIX).$*

result_read_ssd.%~treereader.txt: lhcb_opendata
	BM_CACHED=0 ./bm_timing.sh $@ ./lhcb_opendata -i $(BM_DATA_PREFIX).$* -r

result_read_ssd.%~dataframe.txt: lhcb_opendata
	BM_CACHED=0 ./bm_timing.sh $@ ./lhcb_opendata -i $(BM_DATA_PREFIX).$* -f

result_read_ssd.%~dataframemt.txt: lhcb_opendata
	BM_CACHED=0 ./bm_timing.sh $@ ./lhcb_opendata -i $(BM_DATA_PREFIX).$* -g

result_plot_ssd.%.txt: lhcb_opendata
	BM_CACHED=0 $(BM_ENV_$*) ./bm_timing.sh $@ ./lhcb_opendata$(BM_BINEXT_$*) -p -i $(BM_DATA_PREFIX).$*

result_plot_ssd.%~java.txt: lhcbOpenData.class
	BM_CACHED=0 CLASSPATH=avro-java:$$CLASSPATH ./bm_timing.sh $@ java lhcbOpenData $(BM_DATA_PREFIX).$* -p

result_read_hdd.%.txt: lhcb_opendata
	BM_CACHED=0 $(BM_ENV_$*) ./bm_timing.sh $@ ./lhcb_opendata$(BM_BINEXT_$*) -i $(BM_USBDATA_PREFIX).$*

result_read_eos.%.txt: lhcb_opendata
	BM_CACHED=0 $(BM_ENV_$*) ./bm_timing.sh $@ ./lhcb_opendata$(BM_BINEXT_$*) -i $(BM_EOSDATA_PREFIX).$*

result_read_eos.%~java.txt: lhcbOpenData.class
	BM_CACHED=0 CLASSPATH=avro-java:$$CLASSPATH ./bm_timing.sh $@ java lhcbOpenData $(BM_EOSDATA_PREFIX).$*

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
	root -q -l 'bm_timing.C("result_read_mem", "READ throughput LHCb OpenData, warm cache", "$@", 8000000, true)'

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
	rm -f result_read_mem.*.txt
	cp acat_result_flavor/* .
	BM_FIELD=realtime BM_RESULT_SET=result_read_mem ./bm_combine.sh
	sed -i -e 's/^/flavor-/' result_read_mem.txt
	root -q -l 'bm_timing.C("result_read_mem", "READ Throughput LHCb OpenData, warm cache", "$@", 8000000, true, 2)'

graph_detail_split~evs.root: $(wildcard result_read_mem.*.txt)
	rm -f result_read_mem.*.txt
	cp acat_result_split/* .
	BM_FIELD=realtime BM_RESULT_SET=result_read_mem ./bm_combine.sh
	sed -i -e 's/^/split-/' result_read_mem.txt
	root -q -l 'bm_timing.C("result_read_mem", "READ Throughput LHCb OpenData, warm cache", "$@", 8000000, true, 4)'

graph_detail_splitpattern.root:
	rm -f result_iopattern_read.*.txt
	cp acat_result_split/* .
	for f in result_iopattern_read.*.txt; do sed -i -e 's/^# /# split-/' $$f; done
	$(MAKE) BM_IOPATTERN_SKIPCOLOR=0 graph_iopattern_read.root
	mv graph_iopattern_read.root $@


graph_%.pdf: graph_%.root
	root -q -l 'bm_convert_to_pdf.C("graph_$*")'

clean:
	rm -f libiotrace.so iotrace.o iotrace_capture iotrace.fanout iotrace_test \
	  atlas_aod \
		util.o \
	  lhcb_opendata lhcb_opendata.lz4 \
		mkfaulty \
		schema_aod/aod.cxx schema_aod/libAod.so schema_aod/aod_rdict.pcm \
		fuse_forward
	rm -rf avro-java/lhcb

benchmark_clean:
	rm -f result_* graph_*
