#!/bin/sh

SUM=0
in_large=0
while read; do
  size=$(echo $REPLY | cut -d" " -f2)
  if [ $in_large -eq 0 ]; then
    SUM=$((SUM + 1))
  fi
  if [ "x$size" = "x131072" ]; then
    in_large=1
  else
    in_large=0
  fi
  #echo $size
done
echo $SUM
