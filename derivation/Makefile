ifeq ($(shell root-config --cflags),)
  $(error Cannot find root-config. Please source thisroot.sh)
endif
CXXFLAGS = -Wall -O3 $(shell root-config --cflags)
LDFLAGS = $(shell root-config --libs)

.PHONY = all clean dataclean

all: derivation

derivation: derivation.cxx
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

dataclean:
	rm -f NTPL_*.root

clean: dataclean
	rm -f derivation
