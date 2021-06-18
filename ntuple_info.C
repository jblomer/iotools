#include <ROOT/RNTuple.hxx>

#include <TSystem.h>

#include <iostream>
#include <string>

#include "h1event.h"
//#include "include_cms/classes.hxx"

using ENTupleInfo = ROOT::Experimental::ENTupleInfo;
using RNTupleReader = ROOT::Experimental::RNTupleReader;

void ntuple_info(std::string fileName, std::string ntupleName)
{
   auto ntuple = RNTupleReader::Open(ntupleName, fileName);
   //ntuple->PrintInfo(ENTupleInfo::kSummary);
   ntuple->PrintInfo(ENTupleInfo::kStorageDetails);
}

void Usage(char *progname) {
   std::cout << "Usage: " << progname << " file-name ntuple-name" << std::endl;
}

int main(int argc, char **argv) {
   if (argc < 3) {
      Usage(argv[0]);
      return 1;
   }
   gSystem->Load("include_cms/libClasses.so");
   gSystem->Load("libH1event.so");
   ntuple_info(argv[1], argv[2]);
}
