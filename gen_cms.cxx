#include <ROOT/RNTupleImporter.hxx>

#include <TROOT.h>

#include <cstdio>
#include <iostream>
#include <string>

#include <unistd.h>

#include "util.h"

using RNTupleImporter = ROOT::Experimental::RNTupleImporter;

static void Usage(char *progname)
{
   std::cout << "Usage: " << progname << " -i <ttjet_13tev_june2019.root> -o <ntuple-path> -c <compression> [-m(t)]"
             << std::endl;
}

int main(int argc, char **argv)
{
   std::string inputFile = "ttjet.root";
   std::string outputPath = ".";
   int compressionSettings = 0;
   std::string compressionShorthand = "none";
   std::string treeName = "Events";
   size_t pagesize = 65536;
   size_t clustersize = 52428800;

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
   std::string dsName = "ttjet";
   std::string outputFile = outputPath + "/" + dsName + "~" + compressionShorthand + "_" +
                            std::to_string(pagesize) + "_" + std::to_string(clustersize) + ".ntuple";

   unlink(outputFile.c_str());
   auto importer = RNTupleImporter::Create(inputFile, treeName, outputFile).Unwrap();
   auto options = importer->GetWriteOptions();
   options.SetCompression(compressionSettings);
   options.SetApproxUnzippedPageSize(pagesize);
   options.SetApproxZippedClusterSize(clustersize);
   importer->SetWriteOptions(options);
   importer->Import().ThrowOnError();

   return 0;
}
