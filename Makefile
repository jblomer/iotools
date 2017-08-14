CFLAGS_CUSTOM = -std=c99 -Wall -pthread -g -O2
CXXFLAGS_CUSTOM = -std=c++11 -Wall -pthread -Wall -g -O2 \
		  -I/opt/avro-c-1.8.1/include \
		  -I/opt/parquet-cpp-1.0.0/include
CXXFLAGS_ROOT = $(shell root-config --cflags)
LDFLAGS_CUSTOM = -L/opt/avro-c-1.8.1/lib \
		 -L/opt/parquet-cpp-1.0.0/lib
LDFLAGS_ROOT = $(shell root-config --libs) -lTreePlayer
CFLAGS = $(CFLAGS_CUSTOM)
CXXFLAGS = $(CXXFLAGS_CUSTOM) $(CXXFLAGS_ROOT)
LDFLAGS = $(LDFLAGS_CUSTOM) $(LDFLAGS_ROOT)

ROOTSYS_LZ4 = /opt/root_lz4
CXXFLAGS_ROOT_LZ4 = $(shell $(ROOTSYS_LZ4)/bin/root-config --cflags) -DHAS_LZ4
LDFLAGS_ROOT_LZ4 = $(shell $(ROOTSYS_LZ4)/bin/root-config --libs) -lTreePlayer

all: libiotrace.so iotrace_capture iotrace_test \
  atlas_aod \
  lhcb_opendata lhcb_opendata.lz4 \
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

lhcb_opendata.pb.cc: lhcb_opendata.proto
	protoc --cpp_out=. lhcb_opendata.proto

lhcb_opendata: lhcb_opendata.cc lhcb_opendata.h util.h util.o lhcb_opendata.pb.cc event.h
	g++ $(CXXFLAGS) -o lhcb_opendata lhcb_opendata.cc lhcb_opendata.pb.cc util.o \
		-lhdf5 -lhdf5_hl -lsqlite3 -lavro -lprotobuf $(LDFLAGS) -lz -lparquet

lhcb_opendata.lz4: lhcb_opendata.cc lhcb_opendata.h util.h util.o lhcb_opendata.pb.cc
	g++ $(CXXFLAGS_CUSTOM) $(CXXFLAGS_ROOT_LZ4) -o $@ \
		lhcb_opendata.cc lhcb_opendata.pb.cc util.o \
		-lhdf5 -lhdf5_hl -lsqlite3 -lavro -lprotobuf \
		$(LDFLAGS_CUSTOM) $(LDFLAGS_ROOT_LZ4) \
		-lz -lparquet



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

benchmarks: graph_size.root \
	result_read_mem.graph.root \
	result_read_ssd.graph.root \
	result_read_hdd.graph.root



result_size.txt: bm_events bm_formats bm_size.sh
	./bm_size.sh > result_size.txt

graph_size.root: result_size.txt bm_size.C
	root -q -l bm_size.C


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
	root -q -l 'bm_iopattern.C("$^", "$@")'

graph_iopattern_plot.root: $(wildcard result_iopattern_plot.*.txt)
	echo "$^"
	root -q -l 'bm_iopattern.C("$^", "$@")'


result_eostraffic_read.%.txt: lhcb_opendata
	$(BM_ENV_$*) ./bm_traffic.sh -o $@ -- ./lhcb_opendata$(BM_BINEXT_$*) -i $(BM_EOSDATA_PREFIX).$*

result_eostraffic_plot.%.txt: lhcb_opendata
	$(BM_ENV_$*) ./bm_traffic.sh -o $@ -- ./lhcb_opendata$(BM_BINEXT_$*) -i $(BM_EOSDATA_PREFIX).$* -p


result_read_mem.%.txt: lhcb_opendata
	BM_CACHED=1 $(BM_ENV_$*) ./bm_timing.sh $@ ./lhcb_opendata$(BM_BINEXT_$*) -i $(BM_DATA_PREFIX).$*

result_read_mem.%~dataframe.txt: lhcb_opendata
	BM_CACHED=1 ./bm_timing.sh $@ ./lhcb_opendata -i $(BM_DATA_PREFIX).$* -f

result_read_mem.%~dataframemt.txt: lhcb_opendata
	BM_CACHED=1 ./bm_timing.sh $@ ./lhcb_opendata -i $(BM_DATA_PREFIX).$* -g

result_read_mem.%~treereader.txt: lhcb_opendata
	BM_CACHED=1 ./bm_timing.sh $@ ./lhcb_opendata -i $(BM_DATA_PREFIX).$* -r

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

result_plot_eos.%.txt: lhcb_opendata
	BM_SLEEP=10 BM_CACHED=0 $(BM_ENV_$*) ./bm_timing.sh $@ \
	  ./lhcb_opendata$(BM_BINEXT_$*) -p -i $(BM_EOSDATA_PREFIX).$*

result_write_ssd.%.txt: lhcb_opendata
	BM_CACHED=1 $(BM_ENV_$*) ./bm_timing.sh $@ ./lhcb_opendata$(BM_BINEXT_$*) -i $(BM_DATA_PREFIX).root -o $*

result_write_hdd.%.txt: lhcb_opendata
	BM_CACHED=1 $(BM_ENV_$*) ./bm_timing.sh $@ ./lhcb_opendata$(BM_BINEXT_$*) -i $(BM_DATA_PREFIX).root -o $* \
		  -d $(BM_USBDATA_PATH)

graph_read_mem.root: $(wildcard result_read_mem.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_mem ./bm_combine.sh
	root -q -l 'bm_timing.C("result_read_mem", "READ throughput LHCb OpenData, warm cache", "$@", 1000)'

graph_read_ssd.root: $(wildcard result_read_ssd.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_ssd ./bm_combine.sh
	root -q -l 'bm_timing.C("result_read_ssd", "READ throughput LHCb OpenData, SSD cold cache", "$@", 1450)'

graph_plot_ssd.root: $(wildcard result_plot_ssd.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_plot_ssd ./bm_combine.sh
	root -q -l 'bm_timing.C("result_plot_ssd", "PLOT 2 VARIABLES throughput LHCb OpenData, SSD cold cache", "$@", 1450)'

graph_read_hdd.root: $(wildcard result_read_hdd.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_hdd ./bm_combine.sh
	root -q -l 'bm_timing.C("result_read_hdd", "READ throughput LHCb OpenData, HDD cold cache", "$@", 1450)'

graph_read_hddXzoom.root: $(wildcard result_read_hdd.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_hdd ./bm_combine.sh
	root -q -l 'bm_timing.C("result_read_hdd", "READ throughput LHCb OpenData, HDD cold cache", "$@", -1)'

graph_read_eos23.root: $(wildcard result_read_eos23.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_eos23 ./bm_combine.sh
	root -q -l 'bm_timing.C("result_read_eos23", "READ throughput LHCb OpenData, EOS (LAN)", "$@", -1)'

graph_read_eos33.root: $(wildcard result_read_eos33.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_eos33 ./bm_combine.sh
	root -q -l 'bm_timing.C("result_read_eos33", "READ throughput LHCb OpenData, EOS (+10ms)", "$@", -1)'

graph_read_eos43.root: $(wildcard result_read_eos43.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_read_eos43 ./bm_combine.sh
	root -q -l 'bm_timing.C("result_read_eos43", "READ throughput LHCb OpenData, EOS (+20ms)", "$@", -1)'

graph_write_ssd.root: $(wildcard result_write_ssd.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_write_ssd ./bm_combine.sh
	root -q -l 'bm_timing.C("result_write_ssd", "WRITE throughput LHCb OpenData, SSD", "$@", 230)'

graph_write_hdd.root: $(wildcard result_write_hdd.*.txt)
	BM_FIELD=realtime BM_RESULT_SET=result_write_hdd ./bm_combine.sh
	root -q -l 'bm_timing.C("result_write_hdd", "WRITE throughput LHCb OpenData, HDD", "$@", 230)'

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

benchmark_clean:
	rm -f result_* graph_*
