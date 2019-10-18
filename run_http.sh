#!/bin/sh

if [ x$DATA_ROOT != "x" ]; then
  SELECT_DATA_ROOT="DATA_ROOT=$DATA_ROOT"
fi

for sample in lhcb cms h1X10; do
  for latency in 0 10 100; do
    for compression in none zstd; do
      for format in ntuple root; do
        make $SELECT_DATA_ROOT result_read_http.${sample}+${latency}ms~${compression}.${format}.txt
      done
    done
  done
done
