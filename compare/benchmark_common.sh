PATH_B2HHH_NONE=/path/to/B2HHH~none.root
PATH_HIGGS4LEPTONS=/path/to/higgs-to-4leptons.root
TTREE_RNTUPLE_OTHERARGS='-x 5'

declare -A BASE_PATH
BASE_PATH_LHCB[SSD]=/path/to/ssd
BASE_PATH_LHCB[CephFS]=/path/to/cephfs
BASE_PATH_LHCB[HDD]=/path/to/hdd
BASE_PATH_LHCB[warmCache]=/path/to/hdd

BASE_PATH_CMS[SSD]=/path/to/ssd
BASE_PATH_CMS[CephFS]=/path/to/cephfs
BASE_PATH_CMS[HDD]=/path/to/hdd
BASE_PATH_CMS[warmCache]=/path/to/hdd

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
