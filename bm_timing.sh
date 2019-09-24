#!/bin/sh

BM_NITER=${BM_NITER:-6}
BM_CACHED=${BM_CACHED:-1}
BM_SLEEP=${BM_SLEEP:-0}
BM_OUTPUT=$1
shift 1
echo "Benchmarking $@"

rm -f $BM_OUTPUT
if [ $BM_CACHED -eq 1 ]; then
  $@
fi

if [ -f $BM_OUTPUT ]; then
  mv $BM_OUTPUT $BM_OUTPUT.save
fi

for i in $(seq 1 $BM_NITER); do
  if [ $BM_CACHED -ne 1 ]; then
    free && ./clear_page_cache && free
  fi
  if [ $i -eq 1 ]; then
    format_string="%C (%x)\nrealtime: %e\nusertime: %U\nkerneltime: %S\nrssmax: %M\nmemavg %K\nnswitch: %c\nnwait: %w\nnread: %I\nnwrite: %O"
  else
    format_string="(%x)\n%e\n%U\n%S\n%M\n%K\n%c\n%w\n%I\n%O"
  fi
  this_result=$(mktemp)
  /bin/time -o $this_result -f "$format_string" $@
  if [ $i -eq 1 ]; then
    mv $this_result ${BM_OUTPUT}.working
  else
    paste ${BM_OUTPUT}.working $this_result > ${BM_OUTPUT}.paste
    mv ${BM_OUTPUT}.paste ${BM_OUTPUT}.working
  fi
  rm -f $this_result
  if [ $BM_SLEEP -gt 0 ]; then
    sleep $BM_SLEEP
  fi
done
mv ${BM_OUTPUT}.working ${BM_OUTPUT}
