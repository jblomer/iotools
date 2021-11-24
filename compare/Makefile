CXXFLAGS = -std=c++17 -Wall -pthread -g -O3 -I./h5hep
CXXFLAGS_ROOT = $(shell root-config --cflags)
LDFLAGS =
LDFLAGS_ROOT = $(shell root-config --libs)
LDFLAGS_HDF5 = -lhdf5
LDFLAGS_PARQUET = -larrow -lparquet
BIN = gen_lhcb_h5_row lhcb_h5_row gen_lhcb_h5_column lhcb_h5_column gen_lhcb_parquet lhcb_parquet \
gen_cms_h5_row gen_cms_h5_column gen_cms_parquet

.PHONY = clean h5hep

all: h5hep $(BIN)

clean:
	make -f Makefile.h5hep clean
	rm -f $(BIN) *.o \
		libLhcbEvent.so lhcb_event.cxx lhcb_event_rdict.pcm \
		libCmsEvent.so cms_event.cxx cms_event_rdict.pcm

# External dependencies
h5hep:
	make -f Makefile.h5hep h5hep

# For gen_lhcb_xxx/lhcb_xxx
lhcb_event.cxx: lhcb_event.h lhcb_event_linkdef.h
	rootcling -f $@ $^

libLhcbEvent.so: lhcb_event.cxx
	g++ -shared -fPIC -o$@ $(CXXFLAGS) $(CXXFLAGS_ROOT) $< $(LDFLAGS) $(LDFLAGS_ROOT)

lhcb_ttree.o: lhcb_ttree.cc lhcb_ttree.h lhcb_event.h
	g++ $(CXXFLAGS) $(CXXFLAGS_ROOT) -c -o $@ $<

gen_lhcb_h5_row: gen_lhcb_h5.cc lhcb_ttree.o
	g++ $(CXXFLAGS) $(CXXFLAGS_ROOT) -D__COLUMN_MODEL__=h5hep::ColumnModel::COMPOUND_TYPE -o $@ $^ $(LDFLAGS_ROOT) $(LDFLAGS_HDF5) $(LDFLAGS)

lhcb_h5_row: lhcb_h5.cc
	g++ $(CXXFLAGS) $(CXXFLAGS_ROOT) -D__COLUMN_MODEL__=h5hep::ColumnModel::COMPOUND_TYPE -o $@ $^ $(LDFLAGS_ROOT) $(LDFLAGS_HDF5) $(LDFLAGS)

gen_lhcb_h5_column: gen_lhcb_h5.cc lhcb_ttree.o
	g++ $(CXXFLAGS) $(CXXFLAGS_ROOT) -D__COLUMN_MODEL__=h5hep::ColumnModel::COLUMNAR_FNAL -o $@ $^ $(LDFLAGS_ROOT) $(LDFLAGS_HDF5) $(LDFLAGS)

lhcb_h5_column: lhcb_h5.cc
	g++ $(CXXFLAGS) $(CXXFLAGS_ROOT) -D__COLUMN_MODEL__=h5hep::ColumnModel::COLUMNAR_FNAL -o $@ $^ $(LDFLAGS_ROOT) $(LDFLAGS_HDF5) $(LDFLAGS)

gen_lhcb_parquet: gen_lhcb_parquet.cc lhcb_ttree.o
	g++ $(CXXFLAGS) $(CXXFLAGS_ROOT) -o $@ $^ $(LDFLAGS_ROOT) $(LDFLAGS_PARQUET) $(LDFLAGS)

lhcb_parquet: lhcb_parquet.cc
	g++ $(CXXFLAGS) $(CXXFLAGS_ROOT) -o $@ $^ $(LDFLAGS_ROOT) $(LDFLAGS_PARQUET) $(LDFLAGS)

# For gen_cms_xxx/cms_xxx
cms_event.cxx: cms_event.h cms_event_linkdef.h
	rootcling -f $@ $^

libCmsEvent.so: cms_event.cxx
	g++ -shared -fPIC -o$@ $(CXXFLAGS) $(CXXFLAGS_ROOT) $< $(LDFLAGS) $(LDFLAGS_ROOT)

cms_ttree.o: cms_ttree.cc cms_ttree.h cms_event.h
	g++ $(CXXFLAGS) $(CXXFLAGS_ROOT) -c -o $@ $<

gen_cms_h5_row: gen_cms_h5.cc cms_ttree.o
	g++ $(CXXFLAGS) $(CXXFLAGS_ROOT) -D__COLUMN_MODEL__=h5hep::ColumnModel::COMPOUND_TYPE -o $@ $^ $(LDFLAGS_ROOT) $(LDFLAGS_HDF5) $(LDFLAGS)

#cms_h5_row: cms_h5.cc
#	g++ $(CXXFLAGS) $(CXXFLAGS_ROOT) -D__COLUMN_MODEL__=h5hep::ColumnModel::COMPOUND_TYPE -o $@ $^ $(LDFLAGS_ROOT) $(LDFLAGS_HDF5) $(LDFLAGS)

gen_cms_h5_column: gen_cms_h5.cc cms_ttree.o
	g++ $(CXXFLAGS) $(CXXFLAGS_ROOT) -D__COLUMN_MODEL__=h5hep::ColumnModel::COLUMNAR_FNAL -o $@ $^ $(LDFLAGS_ROOT) $(LDFLAGS_HDF5) $(LDFLAGS)

#cms_h5_column: cms_h5.cc
#	g++ $(CXXFLAGS) $(CXXFLAGS_ROOT) -D__COLUMN_MODEL__=h5hep::ColumnModel::COLUMNAR_FNAL -o $@ $^ $(LDFLAGS_ROOT) $(LDFLAGS_HDF5) $(LDFLAGS)

gen_cms_parquet: gen_cms_parquet.cc cms_ttree.o
	g++ $(CXXFLAGS) $(CXXFLAGS_ROOT) -o $@ $^ $(LDFLAGS_ROOT) $(LDFLAGS_PARQUET) $(LDFLAGS)

#cms_parquet: cms_parquet.cc
#	g++ $(CXXFLAGS) $(CXXFLAGS_ROOT) -o $@ $^ $(LDFLAGS_ROOT) $(LDFLAGS_PARQUET) $(LDFLAGS)
