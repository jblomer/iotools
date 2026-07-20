ifeq ($(shell root-config --cflags),)
  $(error Cannot find root-config. Please source thisroot.sh)
endif

CXXFLAGS = $(shell root-config --cflags)
LDFLAGS = $(shell root-config --libs)
CC = $(shell root-config --cxx)

.PHONY = all clean

all: soa_demo libSoAPoint.so

libSoAPoint.so: SoAPoint.cxx
	$(CC) -shared -fPIC $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

SoAPoint.cxx: SoAPoint.hxx SoAPointSelection.xml
	rootcling -f $@ $^

soa_demo: soa_demo.cxx SoAPoint.hxx
	$(CC) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f soa_demo SoAPoint.cxx libSoAPoint.so SoAPoint_rdict.pcm soa.root
