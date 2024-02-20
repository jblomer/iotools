#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleOptions.hxx>
#include <ROOT/RNTuple.hxx>

#include <TFile.h>
#include <TSystem.h>
#include <TROOT.h>

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <utility>

#include <unistd.h>

#include "util.h"
#include "TriggerRecord.hxx"

#include <hdf5_hl.h>

using ROOT::Experimental::RNTupleModel;
using ROOT::Experimental::RNTupleWriter;
using ROOT::Experimental::RNTupleWriteOptions;

std::vector<std::string> gLastGroups;
extern "C" herr_t FillGroups(hid_t loc_id, const char *name, const H5L_info_t *, void *)
{
   hid_t oid = H5Oopen(loc_id, name, H5P_DEFAULT);
   H5O_info_t info;
   H5Oget_info(oid, &info, H5O_INFO_BASIC);
   if (info.type == H5O_TYPE_GROUP) {
      gLastGroups.emplace_back(name);
   }
   H5Oclose(oid);
   return 0;
}

std::vector<std::string> gLastDatasets;
extern "C" herr_t FillDatasets(hid_t loc_id, const char *name, const H5L_info_t *, void *)
{
   hid_t oid = H5Oopen(loc_id, name, H5P_DEFAULT);
   H5O_info_t info;
   H5Oget_info(oid, &info, H5O_INFO_BASIC);
   if (info.type == H5O_TYPE_DATASET) {
      gLastDatasets.emplace_back(name);
   }
   H5Oclose(oid);
   return 0;
}

static std::string GetStringAttr(hid_t did, const std::string &name)
{
   auto aid = H5Aopen_name(did, name.c_str());
   assert(aid > 0);
   char *value = nullptr;
   auto tid = H5Aget_type(aid);
   assert(tid >= 0);
   auto retval = H5Aread(aid, tid, &value);
   assert(retval >= 0);
   H5Aclose(aid);
   return value;
}

static std::uint64_t GetUInt64Attr(hid_t did, const std::string &name)
{
   auto aid = H5Aopen_name(did, name.c_str());
   assert(aid > 0);
   std::uint64_t value = 0;
   auto tid = H5Aget_type(aid);
   assert(tid >= 0);
   auto retval = H5Aread(aid, tid, &value);
   assert(retval >= 0);
   H5Aclose(aid);
   return value;
}

static std::uint32_t GetUInt32Attr(hid_t did, const std::string &name)
{
   auto aid = H5Aopen_name(did, name.c_str());
   assert(aid > 0);
   std::uint32_t value = 0;
   auto tid = H5Aget_type(aid);
   assert(tid >= 0);
   auto retval = H5Aread(aid, tid, &value);
   assert(retval >= 0);
   H5Aclose(aid);
   return value;
}

static void Usage(char *progname)
{
   std::cout << "Usage: " << progname << " -o <ntuple file> -c <compression> [-m(t)] -i <input.hdf5>"
             << std::endl;
}

int main(int argc, char **argv)
{
   std::string inputFile;
   std::string outputFile;
   int compressionSettings = 0;
   std::string compressionShorthand = "none";

   int c;
   while ((c = getopt(argc, argv, "hvi:o:c:m")) != -1) {
      switch (c) {
      case 'h':
      case 'v':
         Usage(argv[0]);
         return 0;
      case 'i':
         inputFile = optarg;
         break;
      case 'o':
         outputFile = optarg;
         break;
      case 'c':
         compressionSettings = GetCompressionSettings(optarg);
         compressionShorthand = optarg;
         break;
      case 'm':
         ROOT::EnableImplicitMT();
         break;
      default:
         fprintf(stderr, "Unknown option: -%c\n", c);
         Usage(argv[0]);
         return 1;
      }
   }

   RNTupleWriteOptions options;
   options.SetCompression(compressionSettings);

   gSystem->Load("./libTriggerRecord.so");

   auto fid = H5Fopen(inputFile.c_str(), H5P_DEFAULT, H5F_ACC_RDONLY);
   assert(fid >= 0);
   auto gid_root = H5Gopen(fid, "/", H5P_DEFAULT);
   assert(gid_root >= 0);

   auto file = TFile::Open(outputFile.c_str(), "RECREATE");
   assert(file && !file->IsZombie());
   file->SetCompressionSettings(compressionSettings);

   auto attrModel = RNTupleModel::Create();
   attrModel->MakeField<std::string>("application_name", GetStringAttr(gid_root, "application_name"));
   attrModel->MakeField<std::string>("closing_timestamp", GetStringAttr(gid_root, "closing_timestamp"));
   attrModel->MakeField<std::string>("creation_timestamp", GetStringAttr(gid_root, "creation_timestamp"));
   attrModel->MakeField<std::uint64_t>("file_index", GetUInt64Attr(gid_root, "file_index"));
   attrModel->MakeField<std::string>("filelayout_params", GetStringAttr(gid_root, "filelayout_params"));
   attrModel->MakeField<std::uint32_t>("filelayout_version", GetUInt32Attr(gid_root, "filelayout_version"));
   attrModel->MakeField<std::string>("operational_environment", GetStringAttr(gid_root, "operational_environment"));
   attrModel->MakeField<std::string>("record_type", GetStringAttr(gid_root, "record_type"));
   attrModel->MakeField<std::uint64_t>("recorded_size", GetUInt64Attr(gid_root, "recorded_size"));
   attrModel->MakeField<std::uint32_t>("run_number", GetUInt32Attr(gid_root, "run_number"));
   attrModel->MakeField<std::string>("source_id_geo_id_map", GetStringAttr(gid_root, "source_id_geo_id_map"));
   auto attrWriter = RNTupleWriter::Append(std::move(attrModel), "Attributes", *file, options);
   attrWriter->Fill();
   attrWriter.reset();

   auto dataModel = RNTupleModel::Create();
   auto trField = dataModel->MakeField<TriggerRecord>("TriggerRecords");
   auto dataWriter = RNTupleWriter::Append(std::move(dataModel), "DUNE", *file, options);

   H5Literate(gid_root, H5_INDEX_NAME, H5_ITER_NATIVE, NULL, FillGroups, NULL);

   for (const auto &tr : gLastGroups) {
      auto gid_tr = H5Gopen(gid_root, tr.c_str(), H5P_DEFAULT);
      assert(gid_tr >= 0);
      auto gid_rawdata = H5Gopen(gid_tr, "RawData", H5P_DEFAULT);
      assert(gid_rawdata >= 0);

      std::cout << "writing trigger record " << tr << std::endl;
      trField->fStreams.clear();
      trField->fTRName = tr;
      trField->fFragmentTypeSourceIdMap = GetStringAttr(gid_tr, "fragment_type_source_id_map");
      trField->fRecordHeaderSourceId = GetStringAttr(gid_tr, "record_header_source_id");
      trField->fSourceIdPathMap = GetStringAttr(gid_tr, "source_id_path_map");
      trField->fSubdetectorSourceIdMap = GetStringAttr(gid_tr, "subdetector_source_id_map");

      gLastDatasets.clear();
      H5Literate(gid_rawdata, H5_INDEX_NAME, H5_ITER_NATIVE, NULL, FillDatasets, NULL);

      for (const auto &ds: gLastDatasets) {
         auto did = H5Dopen2(gid_rawdata, ds.c_str(), H5P_DEFAULT);
         assert(did >= 0);

         assert(H5Tequal(H5Dget_type(did), H5T_STD_I8LE));

         auto sid = H5Dget_space(did);
         assert(sid >= 0);

         assert(H5Sget_simple_extent_type(sid) == H5S_SIMPLE);

         // Stored as a 2D array, actually a 1D array
         auto ndims = H5Sget_simple_extent_ndims(sid);
         assert(ndims == 2);
         hsize_t dims[2];
         ndims = H5Sget_simple_extent_dims(sid, dims, NULL);
         assert(ndims == 2);
         assert(dims[1] == 1);

         TriggerRecord::Stream stream;
         stream.fStreamName = ds;
         stream.fData.resize(dims[0]);
         auto retval = H5Dread(did, H5T_STD_I8LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, &stream.fData[0]);
         assert(retval >= 0);

         trField->fStreams.emplace_back(std::move(stream));
         std::cout << "    adding stream " << ds << std::endl;

         H5Sclose(sid);
         H5Dclose(did);
      }

      H5Gclose(gid_rawdata);
      H5Gclose(gid_tr);

      dataWriter->Fill();
   }

   H5Gclose(gid_root);
   H5Fclose(fid);

   return 0;
}
