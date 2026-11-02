ifeq ($(shell root-config --cflags),)
  $(error Cannot find root-config. Please source thisroot.sh)
endif
CXXFLAGS = -Wall -O3 -g $(shell root-config --cflags)
LDFLAGS = $(shell root-config --libs)

.PHONY = all clean dataclean

all: huge

huge: huge.cxx
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

dataclean:
	rm -f huge.root

clean: dataclean
	rm -f huge
