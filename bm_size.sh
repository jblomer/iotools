#!/bin/sh

DATA_PREFIX=$1
FILE_NAME=$2
NEVENT=$3

for format in $(cat bm_formats); do
  type=$(echo $format | cut -d- -f1)
  compression=$(echo $format | cut -d- -f2)
  subdir=$type
  if [ $type = "root" ]; then
    subdir="tree"
  fi
  size=$(stat --format=%s ${DATA_PREFIX}/${subdir}/${FILE_NAME}~${compression}.${type})
  size_per_event=$(echo "scale=2; $size/$NEVENT" | bc)
  echo "$format $size_per_event"
done
