PATH_B2HHH_NONE=/path/to/B2HHH~none.root
PATH_HIGGS4LEPTONS=/path/to/higgs-to-4leptons.root

declare -A BASE_PATH
BASE_PATH[SSD]=/path/to/ssd
BASE_PATH[CephFS]=/path/to/cephfs
BASE_PATH[HDD]=/path/to/hdd
BASE_PATH[warmCache]=/path/to/hdd

function LogAndGetRuntimeAnalysis() {
    LOG_FILE=$1
    shift;
    #echo $@ >&2
    $@ | tee -a $LOG_FILE | awk '/Runtime-Analysis/ { match($2, "([0-9]*)us", m); print (m[1] / 1000); }'
}

function LogAndGetTime() {
    LOG_FILE=$1
    shift;
    #echo $@ >&2
    (time $@) |& tee -a $LOG_FILE | awk '/^real/ { match($2, "([0-9]*)m([0-9.]*)s", m); print ((m[1] * 60) + m[2]) * 1000; }'
}
