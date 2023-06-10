#include <ROOT/RNTuple.hxx>

#include <TSystem.h>

#include <iostream>
#include <string>
#include <vector>

#include <unistd.h>

//#include "h1event.h"
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
   std::cout << "Usage: " << progname << " [-l additional_lib.so] FILE_NAME NTUPLE_NAME" << std::endl;
}

int main(int argc, char **argv) {
   int c;
   std::vector<std::string> libs;
   while ((c = getopt(argc, argv, "hl:")) != -1) {
      switch (c) {
      case 'h':
         Usage(argv[0]);
         return 0;
      case 'l':
         libs.emplace_back(optarg);
         break;
      default:
         fprintf(stderr, "Unknown option: -%c\n", c);
         Usage(argv[0]);
         return 1;
      }
   }
   if ((argc - optind) != 2) {
      Usage(argv[0]);
      return 1;
   }

   for (const auto &libpath : libs)
      gSystem->Load(libpath.c_str());
   ntuple_info(argv[optind], argv[optind+1]);
}
