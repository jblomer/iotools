#!/bin/sh

PREFIX=${PREFIX:-data/lhcb/MagnetDown/B2HHH}
NITER=3
OUTPUT=$1

rm -f $OUTPUT
for format in $(cat bm_formats); do
  result="$format"
  dat_file="${PREFIX}.${format}"
  for i in $(seq 1 $NITER); do
    free && ./clear_page_cache && free
    /bin/time -f "%e" -o result_single_timing ./lhcb_opendata -i $dat_file -r
    result="$result $(cat result_single_timing)"
  done
  echo "$result" >> $OUTPUT
done
