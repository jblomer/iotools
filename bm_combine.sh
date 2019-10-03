#!/bin/bash

BM_FIELD=${BM_FIELD:-realtime}
BM_RESULT_SET=${BM_RESULT_SET:-result_read_mem}
BM_OUTPUT=${BM_RESULT_SET}.txt

if [ -f $BM_OUTPUT ]; then
  mv $BM_OUTPUT $BM_OUTPUT.save
fi

for result in ${BM_RESULT_SET}*~*.txt; do
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
done
