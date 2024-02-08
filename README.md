ROOT RNTuple Virtual Probe Station
==================================

This folder contains HEP sample anayses in order to measure and compare the
behavior of TTree and RNTuple.  Every analysis processes a single file with
event data -- either in TTree format or in RNTuple format -- and fills
control histograms.  Every sample analysis is implemented in a single binary
that contains the analysis code in three flavors:

  - Direct access with TTree, using GetBranchAddress() style code
  - Direct access with RNTuple, using type-safe Views (similar to TTreeReader)
  - RDataFrame style (TODO: make all analyses available as RDF)

There are corresponding data generation binaries (`gen_...`) to produce
the input files from publicly available master sources.

Samples
-------

The samples are atlas, cms, h1, lhcb.

ATLAS: H --> gg 2020 open data (cf. tutorial df104)
Medium dense reading (~25%, 12/81 features), non-trivial analysis code.
Background channels are skipped for the sake of benchmarking.
7.8 million events, 76k selected events. EDM consists of (non-nested) `std::vector`s.

CMS: 2019 ttjet nanoAOD (cf. tutorial df102).
Sparse reading (<1%, 6/1479 features). 1.6 million events, 141k selected events. EDM with collections

H1: ROOT's H1 standard analysis (cf. df101 tutorial).
The data are concatenated 10 times for a larger running time.
Medium dense reading (~10%, 16/152 features). 2.8 million events, 75k selected events. EDM with collections.

LHCB: LHCb run 1 open data (B mass spectrum).
Dense reading (>75%, 18/26 features). 8.5 million events, 24k selected events. Fully flat EDM.


Building
--------

Use the Makefile to build the binaries and produce benchmark results and plots.
To build only the benchmarks, run `make benchmarks`.
The Makefile requires ROOT available in the environment. ROOT should be built using

    cmake -Droot7=on -DCMAKE_BUILD_TYPE=relwithdebinfo


Running the Benchmarks
----------------------

The input data can be generated or will be automatically downloaded from

    https://root.cern/files/RNTuple

Each benchmark takes the same input parameters:

    - `-i` input file, ending either in `.root` (tree) or `.ntuple` (ntuple)
    - `-s` show the control plot
    - `-p` show the tree/ntuple performance statistics
    - `-r` run the benchmark with RDataFrame instead of hand-written event loop
    - `-m` enable implicit multi-threading (paralle RNTuple page decompression, parallel RDF event loop)
    - `-x` cluster bunch size; a value less than 1 will disable the cluster cache

The real-time timing uses std::chrono::steady_clock and starts with the second
event (direct access) or with an artificial first filter (RDF).

In order to clear the file system page cache between benchmark runs, use the `clear_page_cache` utility.
It can be created with `make clear_page_cache`, which requires sudo privileges.
The `clear_page_cache` utility is not removed by `make clean`.
It works on Linux only.

Example
-------

```
make benchmarks
./atlas -mps -i gg_data~zstd.ntuple
./cms -mps -i ttjet_13tev_june2019~lzma.root # NOTE: the data file is not public. Ask us for further information.
./h1 -mps -i h1dstX10~zstd.ntuple
./lhcb -mps -i B2HHH~zstd.ntuple
```
