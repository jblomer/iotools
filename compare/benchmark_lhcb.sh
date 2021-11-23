#!/bin/bash

# * NOTE: adjust these values before running the benchmark
CLEAR_PAGE_CACHE=../clear_page_cache
GEN_LHCB=../gen_lhcb
GEN_LHCB_H5_ROW=./gen_lhcb_h5_row
GEN_LHCB_H5_COLUMN=./gen_lhcb_h5_column
GEN_LHCB_PARQUET=./gen_lhcb_parquet
LHCB=../lhcb
LHCB_H5_ROW=./lhcb_h5_row
LHCB_H5_COLUMN=./lhcb_h5_column
LHCB_PARQUET=./lhcb_parquet

COMPRESSION=zstd
H5_COMPRESSIONLEVEL=3

H5_CHUNK_SIZE=8192 # Matches the size of a RNTuple page
PARQUET_CHUNK_SIZE=267378 # Matches the average size of a RNTuple cluster for B2HHH 

PATH_B2HHH_NONE=/path/to/B2HHH~none.root

declare -A BASE_PATH
BASE_PATH[SSD]=/path/to/ssd
BASE_PATH[CephFS]=/path/to/cephfs
BASE_PATH[HDD]=/path/to/hdd
BASE_PATH[warmCache]=/path/to/hdd

function LogAndGetRuntimeAnalysis() {
    LOG_FILE=$1
    shift;
    $@ | tee -a $LOG_FILE | awk '/Runtime-Analysis/ { match($2, "([0-9]*)us", m); print (m[1] / 1000); }'
}

function LogAndGetTime() {
    LOG_FILE=$1
    shift;
    (time $@) |& tee -a $LOG_FILE | awk '/^real/ { match($2, "([0-9]*)m([0-9.]*)s", m); print ((m[1] * 60) + m[2]) * 1000; }'
}

function test_gen_lhcb() {
    declare -A RESULTS

    # RNTuple
    for i in HDD SSD CephFS; do
	RESULTS[$i]=$(LogAndGetTime gen_lhcb_ntuple.log \
				    ${GEN_LHCB} -i ${PATH_B2HHH_NONE} -o ${BASE_PATH[$i]} -c ${COMPRESSION})
    done
    echo RNTuple ${RESULTS[HDD]} ${RESULTS[SSD]} ${RESULTS[CephFS]}

    # HDF5/row-wise
    for i in HDD SSD CephFS; do
	RESULTS[$i]=$(LogAndGetTime gen_lhcb_h5rowwise.log \
				    ${GEN_LHCB_H5_ROW} -i ${PATH_B2HHH_NONE} -o ${BASE_PATH[$i]}/B2HHH_row~${H5_COMPRESSIONLEVEL}.h5 -c ${H5_COMPRESSIONLEVEL} -s ${H5_CHUNK_SIZE})
    done
    echo HDF5/row-wise ${RESULTS[HDD]} ${RESULTS[SSD]} ${RESULTS[CephFS]}

    # HDF5/column-wise
    for i in HDD SSD CephFS; do
	RESULTS[$i]=$(LogAndGetTime gen_lhcb_h5colwise.log \
				    ${GEN_LHCB_H5_COLUMN} -i ${PATH_B2HHH_NONE} -o ${BASE_PATH[$i]}/B2HHH_col~${H5_COMPRESSIONLEVEL}.h5 -c ${H5_COMPRESSIONLEVEL} -s ${H5_CHUNK_SIZE})
    done
    echo HDF5/column-wise ${RESULTS[HDD]} ${RESULTS[SSD]} ${RESULTS[CephFS]}

    # Parquet
    for i in HDD SSD CephFS; do
	RESULTS[$i]=$(LogAndGetTime gen_lhcb_parquet.log \
				    ${GEN_LHCB_PARQUET} -i ${PATH_B2HHH_NONE} -o ${BASE_PATH[$i]}/B2HHH~${COMPRESSION}.parquet -c ${COMPRESSION} -s ${PARQUET_CHUNK_SIZE})
    done
    echo Parquet ${RESULTS[HDD]} ${RESULTS[SSD]} ${RESULTS[CephFS]}
}

function test_lhcb() {
    declare -A RESULTS

    # TTree
    for i in HDD SSD CephFS warmCache; do
	if [ $i != warmCache ]; then ${CLEAR_PAGE_CACHE}; fi
	RESULTS[$i]=$(LogAndGetRuntimeAnalysis lhcb_ttree.log \
					       ${LHCB} -i ${BASE_PATH[$i]}/B2HHH~${COMPRESSION}.root)
    done
    echo TTree ${RESULTS[HDD]} ${RESULTS[SSD]} ${RESULTS[CephFS]} ${RESULTS[warmCache]}

    # RNTuple
    for i in HDD SSD CephFS warmCache; do
	if [ $i != warmCache ]; then ${CLEAR_PAGE_CACHE}; fi
	RESULTS[$i]=$(LogAndGetRuntimeAnalysis lhcb_ntuple.log \
					       ${LHCB} -i ${BASE_PATH[$i]}/B2HHH~${COMPRESSION}.ntuple)
    done
    echo RNTuple ${RESULTS[HDD]} ${RESULTS[SSD]} ${RESULTS[CephFS]} ${RESULTS[warmCache]}

    # HDF5/row-wise
    for i in HDD SSD CephFS warmCache; do
	if [ $i != warmCache ]; then ${CLEAR_PAGE_CACHE}; fi
	RESULTS[$i]=$(LogAndGetRuntimeAnalysis lhcb_h5rowwise.log \
					       ${LHCB_H5_ROW} -i ${BASE_PATH[$i]}/B2HHH_row~${H5_COMPRESSIONLEVEL}.h5)
    done
    echo HDF5/row-wise ${RESULTS[HDD]} ${RESULTS[SSD]} ${RESULTS[CephFS]} ${RESULTS[warmCache]}

    # HDF5/column-wise
    for i in HDD SSD CephFS warmCache; do
	if [ $i != warmCache ]; then ${CLEAR_PAGE_CACHE}; fi
	RESULTS[$i]=$(LogAndGetRuntimeAnalysis lhcb_h5colwise.log \
					       ${LHCB_H5_COLUMN} -i ${BASE_PATH[$i]}/B2HHH_col~${H5_COMPRESSIONLEVEL}.h5)
    done
    echo HDF5/column-wise ${RESULTS[HDD]} ${RESULTS[SSD]} ${RESULTS[CephFS]} ${RESULTS[warmCache]}

    # Parquet
    for i in HDD SSD CephFS warmCache; do
	if [ $i != warmCache ]; then ${CLEAR_PAGE_CACHE}; fi
	RESULTS[$i]=$(LogAndGetRuntimeAnalysis lhcb_parquet.log \
					       ${LHCB_PARQUET} -i ${BASE_PATH[$i]}/B2HHH~${COMPRESSION}.parquet)
    done
    echo Parquet ${RESULTS[HDD]} ${RESULTS[SSD]} ${RESULTS[CephFS]} ${RESULTS[warmCache]}
}

echo "==== gen_lhcb TESTS START ===="
echo -e "\tHDD\tSSD\tCephFS"
test_gen_lhcb;

echo "==== lhcb TESTS START ===="
echo -e "\tHDD\tSSD\tCephFS\twarmCache"
test_lhcb;
