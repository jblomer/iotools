#!/bin/sh

PREFIX=data/lhcb/MagnetDown/B2HHH
NEVENT=$(cat bm_events)

for format in $(cat bm_formats); do
  size=$(stat --format=%s ${PREFIX}.${format})
  size_per_event=$(echo "scale=2; $size/$NEVENT" | bc)
  echo "$format $size_per_event"
done
