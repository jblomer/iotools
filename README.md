Benchmark for reading and writing a typical HEP ntuple with several libraries / data formats.

The data files can be found here: https://cernbox.cern.ch/index.php/s/bWn6oeSUyvtHP4t

To run, setup the ROOT environment and call 'make' to build the binaries.
Create a folder or symlink data/ that points to the B2HHH.$suffix files.
Starting from the B2HHH.root file, other formats can be created like

    ./lhcb_opendata -i data/B2HHH.root -o root-lz4

To run the benchmarks, call the result_...txt targets, like

    make result_size.txt
    make result_read_mem.root-lz4.txt
    make result_read_mem.root~dataframe.txt
    ...

Then create the plots by calling

    make graph_size.pdf
    make graph_read_mem~evs.pdf  # show events per second on the y axis
    make graph_read_mem~mbs.pdf  # show MB/s on the y axis

