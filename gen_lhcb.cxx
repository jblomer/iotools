#include <ROOT/RNTupleImporter.hxx>

#include <TROOT.h>

#include <cstdio>
#include <iostream>
#include <string>

#include <unistd.h>

#include "util.h"

using RNTupleImporter = ROOT::Experimental::RNTupleImporter;

void Usage(char *progname)
{
   std::cout << "Usage: " << progname << " -o <ntuple-path> -c <compression> -p <page-size> -x <cluster-size> [-m(t)] <LHCb root file>"
             << std::endl;
}

int main(int argc, char **argv)
{
   std::string inputFile = "B2HHH.root";
   std::string outputPath = ".";
   int compressionSettings = 0;
   std::string compressionShorthand = "none";
   std::string treeName = "DecayTree";
   int pagesize = -1;
   int clustersize = -1;

   int c;
   while ((c = getopt(argc, argv, "hvi:o:c:p:x:m")) != -1)
   {
      switch (c)
      {
      case 'h':
      case 'v':
         Usage(argv[0]);
         return 0;
      case 'i':
         inputFile = optarg;
         break;
      case 'o':
         outputPath = optarg;
         break;
      case 'c':
         compressionSettings = GetCompressionSettings(optarg);
         compressionShorthand = optarg;
         break;
      case 'm':
         ROOT::EnableImplicitMT();
         break;
      case 'p':
         pagesize = atoi(optarg);
         break;
      case 'x':
         clustersize = atoi(optarg);
         break;
      default:
         fprintf(stderr, "Unknown option: -%c\n", c);
         Usage(argv[0]);
         return 1;
      }
   }
   std::cout << "clustersize: " << clustersize << std::endl;

   std::string dsName = "B2HHH";
   std::string outputFile = outputPath + "/" + dsName + "~" + compressionShorthand;
   unlink(outputFile.c_str());
   auto importer = RNTupleImporter::Create(inputFile, treeName, outputFile);
   auto options = importer->GetWriteOptions();
   options.SetCompression(compressionSettings);

   // Change pagesize and add pagesize to outputfile if pagesize was given
   if (pagesize >= 0)
   {
      options.SetApproxUnzippedPageSize(pagesize);
      options.SetApproxUnzippedPageSize(pagesize);
      outputFile += "_pagesize=" std::to_string(pagesize);
   }

   // Change clustersize and add clustersize to outputfile if clustersize was given
   if (clustersize >= 0)
   {
      options.SetApproxZippedClusterSize(clustersize);
      outputFile += "_clustersize=" std::to_string(clustersize);
   }

   outputFile += ".ntuple";

   importer->SetWriteOptions(options);
   importer->Import();

   return 0;
}
