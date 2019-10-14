#!/bin/bash

BM_FIELD=${BM_FIELD:-realtime}

if [ -f $BM_OUTPUT ]; then
  mv $BM_OUTPUT $BM_OUTPUT.save
fi

if [ "x$BM_RESULT_SET" != "x" ]; then
   for result in ${BM_RESULT_SET}*~*.txt; do
     if ! echo $result | grep -q +mmap; then
       method="direct"
       if echo $result | grep -q "+"; then
         method=$(echo $result | cut -d~ -f1 | cut -d+ -f2)
       fi
       format_suffix=$(echo $result | cut -d~ -f2)
       compression=$(echo $format_suffix | cut -d. -f1)
       container=$(echo $format_suffix | cut -d. -f2)
       format="$container+$method-$compression"
       grep "^${BM_FIELD}" $result | awk -v format=$format \
         '{ for(i=2; i<NF; i++) printf "%s",$i OFS; if(NF) printf "%s",$NF; printf ORS} BEGIN {printf "%s ", format}' \
         >> $BM_OUTPUT
     fi
   done
else
  for result in $@; do
     method="direct"
     if echo $result | grep -q "+"; then
       method=$(echo $result | cut -d~ -f1 | cut -d+ -f2)
     fi
     sample=$(echo $result | cut -d. -f2 | cut -d+ -f1 | cut -d~ -f1)
     format_suffix=$(echo $result | cut -d~ -f2)
     compression=$(echo $format_suffix | cut -d. -f1)
     format="$sample+$method-$compression"
     grep "^${BM_FIELD}" $result | awk -v format=$format \
       '{ for(i=2; i<NF; i++) printf "%s",$i OFS; if(NF) printf "%s",$NF; printf ORS} BEGIN {printf "%s ", format}' \
       >> $BM_OUTPUT
   done
fi
