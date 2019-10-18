#!/bin/sh

if [ x$DATA_ROOT != "x" ]; then
  SELECT_DATA_ROOT="DATA_ROOT=$DATA_ROOT"
fi

for sample in lhcb cms h1X10; do
  for compression in none zstd; do
    for format in ntuple root; do
      for nstreams in 1 2 4 8 16 32 64; do
        make $SELECT_DATA_ROOT result_read_ssd.${sample}+N${nstreams}~${compression}.${format}.txt
      done
    done
  done
done
