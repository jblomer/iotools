#!/bin/sh

DATA_PREFIX=$1
FILE_NAME=$2
NEVENT=$3

for format in $(cat bm_formats); do
  suffix=$(echo $format | cut -d- -f1)
  compression=$(echo $format | cut -d- -f2)
  size=$(stat --format=%s ${DATA_PREFIX}/${FILE_NAME}~${compression}.${suffix})
  size_per_event=$(echo "scale=2; $size/$NEVENT" | bc)
  echo "$format $size_per_event"
done
