#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTuple.hxx>

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

   gSystem->Load("./libTriggerRecord.so");
   auto model = RNTupleModel::Create();
   auto fldTR = model->MakeField<TriggerRecord>("TriggerRecord");
   auto writer = RNTupleWriter::Recreate(std::move(model), "DUNE", outputFile);

   auto fid = H5Fopen(inputFile.c_str(), H5P_DEFAULT, H5F_ACC_RDONLY);
   assert(fid >= 0);
   auto gid_root = H5Gopen(fid, "/", H5P_DEFAULT);
   assert(gid_root >= 0);

   H5Literate(gid_root, H5_INDEX_NAME, H5_ITER_NATIVE, NULL, FillGroups, NULL);

   for (const auto &tr : gLastGroups) {
      auto gid_tr = H5Gopen(gid_root, tr.c_str(), H5P_DEFAULT);
      assert(gid_tr >= 0);
      auto gid_rawdata = H5Gopen(gid_tr, "RawData", H5P_DEFAULT);
      assert(gid_rawdata >= 0);

      fldTR->fStreams.clear();
      fldTR->fTRName = tr;
      std::cout << "writing trigger record " << tr << std::endl;

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

         fldTR->fStreams.emplace_back(std::move(stream));
         std::cout << "    adding stream " << ds << std::endl;

         H5Sclose(sid);
         H5Dclose(did);
      }

      H5Gclose(gid_rawdata);
      H5Gclose(gid_tr);

      writer->Fill();
   }

   H5Gclose(gid_root);
   H5Fclose(fid);

   return 0;
}
