#!/bin/sh

if [ x$DATA_ROOT != "x" ]; then
  SELECT_DATA_ROOT="DATA_ROOT=$DATA_ROOT"
fi

for sample in lhcb cms h1X10; do
  make $SELECT_DATA_ROOT result_read_optane.${sample}~none.ntuple.txt
done

for sample in lhcb cms h1X10; do
  make $SELECT_DATA_ROOT result_read_optane.${sample}+mmap~none.ntuple.txt
done
