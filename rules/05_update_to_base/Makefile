ifeq ($(shell root-config --cflags),)
  $(error Cannot find root-config. Please source thisroot.sh)
endif

CXXFLAGS = $(shell root-config --cflags)
LDFLAGS = $(shell root-config --libs) -lROOTNTuple

all: libEvent_v2.so libEvent_v3.so

libEvent_v2.so: Event_v2.cxx
	$(CXX) -shared -fPIC -o$@ $(CXXFLAGS) $< $(LDFLAGS)

Event_v2.cxx: Event_v2.hxx Event_v2_LinkDef.h
	rootcling -f $@ $^

libEvent_v3.so: Event_v3.cxx
	$(CXX) -shared -fPIC -o$@ $(CXXFLAGS) $< $(LDFLAGS)

Event_v3.cxx: Event_v3.hxx Event_v3_LinkDef.h
	rootcling -f $@ $^

clean:
	rm -f Event_v2.cxx libEvent_v2.so Event_v2_rdict.pcm
	rm -f Event_v3.cxx libEvent_v3.so Event_v3_rdict.pcm
	rm -f data.root
