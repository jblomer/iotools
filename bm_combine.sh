#!/bin/bash

BM_FIELD=${BM_FIELD:-realtime}
BM_RESULT_SET=${BM_RESULT_SET:-result_read_mem}
BM_OUTPUT=${BM_RESULT_SET}.txt

if [ -f $BM_OUTPUT ]; then
  mv $BM_OUTPUT $BM_OUTPUT.save
fi

for result in ${BM_RESULT_SET}~*.txt; do
  format=$(echo $result | sed 's/[^~]*~\([^.]*\)\.txt$/\1/')
  grep "^${BM_FIELD}" $result | awk -v format=$format \
    '{ for(i=2; i<NF; i++) printf "%s",$i OFS; if(NF) printf "%s",$NF; printf ORS} BEGIN {printf "%s ", format}' \
    >> $BM_OUTPUT
done
