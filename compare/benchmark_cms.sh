#!/bin/bash
source ./benchmark_common.sh

# * NOTE: adjust these values before running the benchmark
CLEAR_PAGE_CACHE=../clear_page_cache
GEN_CMS=../gen_physlite
GEN_CMS_H5_ROW=./gen_cms_h5_row
GEN_CMS_H5_COLUMN=./gen_cms_h5_column
GEN_CMS_PARQUET=./gen_cms_parquet
CMS=./cms_10br
CMS_H5_ROW=./cms_10br_h5_row
CMS_H5_COLUMN=./cms_10br_h5_column
CMS_PARQUET=./cms_10br_parquet

COMPRESSION=zstd
H5_COMPRESSIONLEVEL=3

H5_CHUNK_SIZE=16384 # Matches the size of a RNTuple page
PARQUET_CHUNK_SIZE=214059 # Matches the average size of a RNTuple cluster for higgs-to-4leptons-CMS-open-MC.root

function test_gen_cms() {
    declare -A RESULTS

    # RNTuple
    for i in SSD CephFS HDD; do
	RESULTS[$i]=$(LogAndGetTime gen_cms_ntuple.log \
				    ${GEN_CMS} -i ${PATH_HIGGS4LEPTONS} -o ${BASE_PATH_CMS[$i]} -c ${COMPRESSION} -t Events)
	mv ${BASE_PATH_CMS[$i]}/{physlite,higgs4leptons}~${COMPRESSION}.ntuple
    done
    echo RNTuple ${RESULTS[HDD]} ${RESULTS[SSD]} ${RESULTS[CephFS]}

    # HDF5/row-wise
    for i in SSD CephFS HDD; do
	RESULTS[$i]=$(LogAndGetTime gen_cms_h5rowwise.log \
				    ${GEN_CMS_H5_ROW} -i ${PATH_HIGGS4LEPTONS} -o ${BASE_PATH_CMS[$i]}/higgs4leptons_row~${H5_COMPRESSIONLEVEL}.h5 -c ${H5_COMPRESSIONLEVEL} -s ${H5_CHUNK_SIZE})
    done
    echo HDF5/row-wise ${RESULTS[HDD]} ${RESULTS[SSD]} ${RESULTS[CephFS]}

    # HDF5/column-wise
    for i in SSD CephFS HDD; do
	RESULTS[$i]=$(LogAndGetTime gen_cms_h5colwise.log \
				    ${GEN_CMS_H5_COLUMN} -i ${PATH_HIGGS4LEPTONS} -o ${BASE_PATH_CMS[$i]}/higgs4leptons_col~${H5_COMPRESSIONLEVEL}.h5 -c ${H5_COMPRESSIONLEVEL} -s ${H5_CHUNK_SIZE})
    done
    echo HDF5/column-wise ${RESULTS[HDD]} ${RESULTS[SSD]} ${RESULTS[CephFS]}

    # Parquet
    for i in SSD CephFS HDD; do
	RESULTS[$i]=$(LogAndGetTime gen_cms_parquet.log \
				    ${GEN_CMS_PARQUET} -i ${PATH_HIGGS4LEPTONS} -o ${BASE_PATH_CMS[$i]}/higgs4leptons~${COMPRESSION}.parquet -c ${COMPRESSION} -s ${PARQUET_CHUNK_SIZE})
    done
    echo Parquet ${RESULTS[HDD]} ${RESULTS[SSD]} ${RESULTS[CephFS]}
}

function test_cms_10br() {
    declare -A RESULTS

    # TTree
    for i in SSD CephFS HDD warmCache; do
	if [ $i != warmCache ]; then ${CLEAR_PAGE_CACHE}; fi
	RESULTS[$i]=$(LogAndGetRuntimeAnalysis cms_10br_ttree.log \
					       ${CMS} -i ${BASE_PATH_CMS[$i]}/higgs4leptons~${COMPRESSION}.root -m ${TTREE_RNTUPLE_OTHERARGS})
    done
    echo -e "TTree\t ${RESULTS[HDD]}\t${RESULTS[SSD]}\t${RESULTS[CephFS]}\t${RESULTS[warmCache]}"

    # RNTuple
    for i in SSD CephFS HDD warmCache; do
	if [ $i != warmCache ]; then ${CLEAR_PAGE_CACHE}; fi
	RESULTS[$i]=$(LogAndGetRuntimeAnalysis cms_10br_ntuple.log \
					       ${CMS} -i ${BASE_PATH_CMS[$i]}/higgs4leptons~${COMPRESSION}.ntuple -m ${TTREE_RNTUPLE_OTHERARGS})
    done
    echo -e "RNTuple\t ${RESULTS[HDD]}\t${RESULTS[SSD]}\t${RESULTS[CephFS]}\t${RESULTS[warmCache]}"

    # HDF5/row-wise
    for i in SSD CephFS HDD warmCache; do
	if [ $i != warmCache ]; then ${CLEAR_PAGE_CACHE}; fi
	RESULTS[$i]=$(LogAndGetRuntimeAnalysis cms_10br_h5rowwise.log \
					       ${CMS_H5_ROW} -i ${BASE_PATH_CMS[$i]}/higgs4leptons_row~${H5_COMPRESSIONLEVEL}.h5)
    done
    echo -e "HDF5/row-wise\t ${RESULTS[HDD]}\t${RESULTS[SSD]}\t${RESULTS[CephFS]}\t${RESULTS[warmCache]}"

    # HDF5/column-wise
    for i in SSD CephFS HDD warmCache; do
	if [ $i != warmCache ]; then ${CLEAR_PAGE_CACHE}; fi
	RESULTS[$i]=$(LogAndGetRuntimeAnalysis cms_10br_h5colwise.log \
					       ${CMS_H5_COLUMN} -i ${BASE_PATH_CMS[$i]}/higgs4leptons_col~${H5_COMPRESSIONLEVEL}.h5)
    done
    echo -e "HDF5/column-wise\t ${RESULTS[HDD]}\t${RESULTS[SSD]}\t${RESULTS[CephFS]}\t${RESULTS[warmCache]}"

    # Parquet
    for i in SSD CephFS HDD warmCache; do
	if [ $i != warmCache ]; then ${CLEAR_PAGE_CACHE}; fi
	RESULTS[$i]=$(LogAndGetRuntimeAnalysis cms_10br_parquet.log \
					       ${CMS_PARQUET} -i ${BASE_PATH_CMS[$i]}/higgs4leptons~${COMPRESSION}.parquet)
    done
    echo -e "Parquet\t ${RESULTS[HDD]}\t${RESULTS[SSD]}\t${RESULTS[CephFS]}\t${RESULTS[warmCache]}"
}

echo "==== gen_cms TESTS START ===="
echo -e "\t HDD\tSSD\tCephFS"
test_gen_cms;

echo "==== cms_10br TESTS START ===="
echo -e "\t HDD\tSSD\tCephFS\twarmCache"
test_cms_10br;
