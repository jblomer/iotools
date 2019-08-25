#ifdef __CINT__
R__LOAD_LIBRARY(ROOTNTuple)
#endif

#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleModel.hxx>

#include <iostream>

using ENTupleInfo = ROOT::Experimental::ENTupleInfo;
using RNTupleModel = ROOT::Experimental::RNTupleModel;
using RNTupleReader = ROOT::Experimental::RNTupleReader;

void ntpl009_details(const std::string &path, const std::string &ntupleName = "NTuple") {
   auto model = RNTupleModel::Create();
   auto ntuple = RNTupleReader::Open(std::move(model), ntupleName, path);

   ntuple->PrintInfo(ENTupleInfo::kStorageDetails, std::cout);
}
