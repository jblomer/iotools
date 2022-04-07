#!/bin/bash
CLEAR_PAGE_CACHE=../../clear_page_cache
DSTAT_INTERVAL=5
DSTAT_OUT=/path/to/dstat_out
OUTPUT_FILE=/path/to/output_file

# If `dstat` is installed, statistics are collected every `$DSTAT_INTERVAL` seconds
DSTAT=`which dstat 2>/dev/null`
function dstat_start() {
    if [ -n "$DSTAT" ]; then
	dstat --nocolor --noupdate --cpu --mem --top-cpu --top-mem $DSTAT_INTERVAL >> $DSTAT_OUT &
	DSTAT_PID=$!
    fi
}
function dstat_stop() {
    if [ -n "$DSTAT" ]; then kill $DSTAT_PID; fi
}
function collect_stats() {
    dstat_start
    time "$@"
    dstat_stop
}

for TEST in test_{ntuple,ttree,parquet,h5_row,h5_column}; do
    # ~10GB, ~100GB, ~250GB, ~1TB
    for N_ENTRIES in 833333334 8333333334 20833333333 83333333334; do
	echo -e "\n==== Starting test ./${TEST} -n $N_ENTRIES ===="
	collect_stats ./${TEST} -n $N_ENTRIES write $OUTPUT_FILE
	${CLEAR_PAGE_CACHE}
	collect_stats ./${TEST} read $OUTPUT_FILE
	rm -f $OUTPUT_FILE
    done
done
