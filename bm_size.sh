#!/bin/sh

DATA_PREFIX=$1
FILE_NAME=$2
NEVENT=$3

for format in $(cat bm_formats); do
  suffix=$(echo $format | cut -d- -f1)
  compression=$(echo $format | cut -d- -f2)
  path="${DATA_PREFIX}/${FILE_NAME}~${compression}.${suffix}"
  if ! stat $path >/dev/null 2>&1
  then
    echo "Warning: $path not available" >&2
    size=0
  else
    size=$(stat --format=%s $path)
  fi
  size_per_event=$(echo "scale=2; $size/$NEVENT" | bc)
  echo "$format $size_per_event"
done
