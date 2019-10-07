ROOT RNTuple Virtual Probe Station
==================================

This folder contains HEP sample anayses in order to measure and compare the
behavior of TTree and RNTuple.  Every analysis processes a single file with
event data -- either in TTree format or in RNTuple raw format -- and fills
control histograms.  Every sample analysis is implemented in a single binary
that contains the analysis code in three flavors:

  - Direct access with TTree, using GetBranchAddress() style code
  - Direct access with RNTuple, using type-safe Views (similar to TTreeReader)
  - RDataFrame style

There are corresponding data generation binaries (`gen_...`) to produce
the input files from publicly available master sources.

Samples
-------

The samples are h1, lhcb, cms (TODO: describe).


Building
--------

Use the Makefile to build the binaries and produce benchmark results and plots.
The Makefile requires the following ROOT in your enfironment:

    https://github.com/jblomer/root/tree/ntuple-chep19


It should be build using

    cmake -Droot7=on -Dcxx14=on -DCMAKE_BUILD_TYPE=relwithdebinfo


Running the Benchmarks
----------------------

The input data can be generated or downloaded from

    https://cernbox.cern.ch/index.php/s/fVT9JicZHCUDGqn

Each benchmark takes the same input parameters:

    - `-i` input file, ending either in `.root` (tree) or `.ntuple` (ntuple raw)
    - `-s` optionally show the control plot
    - `-p` optionally show the tree/ntuple performance statistics
    - `-r` optionally, run the benchmark with RDataFrame instead of direct access
    - `-R` use RDF with implicit multi-threading

The real-time timing uses std::chrono::steady_clock and starts with the second
event (direct access) or with an artificial first filter (RDF).s

