#!/bin/sh

set -e

BENCHMARKS="atlas cms h1 lhcb"
ORIGINS="tmpfs ssd hdd xrootd"
FORMATS="tree ntuple"
NRUNS=10
OUTDIR="./chep23"

FILE_atlas="gg_data"
FILE_cms="ttjet_13tev_june2019"
FILE_h1="h1dstX10"
FILE_lhcb="B2HHH"

PREFIX_tmpfs="/data/tmpfs/chep23"
PREFIX_ssd="/data/ssdext4/chep23"
PREFIX_hdd="/data/hddext4/chep23"
PREFIX_xrootd="root://eosproject.cern.ch//eos/project/r/root-eos/public/RNTuple/chep23"

SUFFIX_tree="root"
SUFFIX_ntuple="ntuple"

for o in $ORIGINS; do
  for b in $BENCHMARKS; do
    for r in $(seq 1 $NRUNS); do	  
      ./clear_page_cache
      for f in $FORMATS; do 
        PREFIXVAR=PREFIX_$o
        SUFFIXVAR=SUFFIX_$f
        FILEVAR=FILE_$b
        ./$b -r -p -i ${!PREFIXVAR}/${!FILEVAR}~zstd.${!SUFFIXVAR} 2>&1 | tee $OUTDIR/$b-$o-$f-$r.txt
      done
    done
  done
done

