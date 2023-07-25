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
   std::cout << "Usage: " << progname << " -o <ntuple-path> -c <compression> -p <page-size> -x <cluster-size> [-m(t)] <H1 root file>"
             << std::endl;
}

int main(int argc, char **argv)
{
   std::string inputFile = "gg_data.root";
   std::string outputPath = ".";
   int compressionSettings = 0;
   std::string compressionShorthand = "none";
   std::string treeName = "mini";
   size_t pagesize = (64 * 1024);
   size_t clustersize = (50 * 1000 * 1000);

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
   std::string dsName = "gg_data";
   std::string outputFile = outputPath + "/" + dsName + "~" + compressionShorthand + "_" +
                            std::to_string(pagesize) + "_" + std::to_string(clustersize) + ".ntuple";
   unlink(outputFile.c_str());
   auto importer = RNTupleImporter::Create(inputFile, treeName, outputFile);
   auto options = importer->GetWriteOptions();
   options.SetCompression(compressionSettings);
   options.SetApproxUnzippedPageSize(pagesize);
   options.SetApproxZippedClusterSize(clustersize);
   importer->SetWriteOptions(options);
   importer->Import();

   return 0;
}
