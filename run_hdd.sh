#!/bin/sh

if [ x$DATA_ROOT != "x" ]; then
  SELECT_DATA_ROOT="DATA_ROOT=$DATA_ROOT"
fi

for sample in lhcb cms h1X10; do
  for compression in none lz4 zstd zlib lzma; do
    for format in ntuple root; do
      make $SELECT_DATA_ROOT result_read_hdd.${sample}~${compression}.${format}.txt
    done
  done
done