#!/bin/sh

if [ x$DATA_ROOT != "x" ]; then
  SELECT_DATA_ROOT="DATA_ROOT=$DATA_ROOT"
fi

if [ x$NET_DEV != "x" ]; then
  SELECT_NET_DEV="NET_DEV=$NET_DEV"
fi

for sample in lhcb cms h1X10; do
  for latency in 0 10 50 100; do
    for compression in none zstd; do
      for format in ntuple root; do
        make $SELECT_DATA_ROOT $SELECT_NET_DEV \
          result_read_http.${sample}+${latency}ms~${compression}.${format}.txt
      done
    done
  done
done
