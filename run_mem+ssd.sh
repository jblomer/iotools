#!/bin/sh

if [ x$DATA_ROOT != "x" ]; then
  SELECT_DATA_ROOT="DATA_ROOT=$DATA_ROOT"
fi

for sample in lhcb cms h1X10; do
  for medium in mem ssd; do
    for compression in none lz4 zstd zlib lzma; do
      for format in ntuple root; do
        make $SELECT_DATA_ROOT result_read_${medium}.${sample}~${compression}.${format}.txt
      done
    done
  done
done

for sample in lhcb cms h1X10; do
  for medium in mem ssd; do
    make $SELECT_DATA_ROOT result_read_${medium}.${sample}+mmap~none.ntuple.txt
  done
done
