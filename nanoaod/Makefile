CXX = g++
CXXFLAGS = -O2 -g $(shell root-config --cflags)
LDFLAGS = $(shell root-config --libs)

all: ntpl007_dimuonTree ntpl008_dimuonView

ntpl007_dimuonTree: ntpl007_dimuonTree.C
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

ntpl008_dimuonView: ntpl008_dimuonView.C
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS) -lROOTNTuple

clean:
	rm -f ntpl007_dimuonTree ntpl008_dimuonView