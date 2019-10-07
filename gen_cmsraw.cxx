#include <ROOT/RField.hxx>
#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleOptions.hxx>

#include <TBranch.h>
#include <TCanvas.h>
#include <TChain.h>
#include <TFile.h>
#include <TH1F.h>
#include <TLeaf.h>
#include <TTree.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TTreeReaderArray.h>
#include <TSystem.h>

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <unistd.h>

#include "util.h"

// Import classes from experimental namespace for the time being
using RNTupleModel = ROOT::Experimental::RNTupleModel;
using RFieldBase = ROOT::Experimental::Detail::RFieldBase;
using RNTupleWriter = ROOT::Experimental::RNTupleWriter;
using RNTupleWriteOptions = ROOT::Experimental::RNTupleWriteOptions;

void Usage(char *progname) {
   std::cout << "Usage: " << progname << " -o <ntuple output dir> -c <compression> -o <tree input>"
             << std::endl;
}


int main(int argc, char **argv) {
   std::string inputPath;
   std::string outputDir;
   int compressionSettings = 0;
   std::string compressionShorthand = "none";

   int c;
   while ((c = getopt(argc, argv, "hvo:c:i:")) != -1) {
      switch (c) {
      case 'h':
      case 'v':
         Usage(argv[0]);
         return 0;
      case 'o':
         outputDir = optarg;
         break;
      case 'c':
         compressionSettings = GetCompressionSettings(optarg);
         compressionShorthand = optarg;
         break;
      case 'i':
         inputPath = optarg;
         break;
      default:
         fprintf(stderr, "Unknown option: -%c\n", c);
         Usage(argv[0]);
         return 1;
      }
   }
   if (inputPath.empty() || outputDir.empty()) {
      Usage(argv[0]);
      return 1;
   }

   std::string outputFile = outputDir + "/cmsraw~" + compressionShorthand + ".ntuple";
   std::cout << "Converting " << inputPath << " --> " << outputFile << std::endl;

   auto file = TFile::Open(inputPath.c_str());
   auto tree = file->Get<TTree>("Events");
   auto model = RNTupleModel::Create();
   auto vNtuple = model->MakeField<std::vector<std::vector<unsigned char>>>("v");
   RNTupleWriteOptions options;
   options.SetCompression(compressionSettings);
   options.SetNumElementsPerPage(100000);
   auto ntuple = RNTupleWriter::Recreate(std::move(model), "Events", outputFile, options);

   TTreeReader reader(tree);
   TTreeReaderValue<std::vector<std::vector<unsigned char>>> vTree(reader, "v");

   // Fills the ntuple with entries from the TTree.
   int count = 0;
   while(reader.Next()) {
      *vNtuple = *vTree;
      ntuple->Fill();
      if (++count % 1000 == 0)
         std::cout << "Wrote " << count << " events" << std::endl;
   }
}
