#include <ROOT/RField.hxx>
#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleOptions.hxx>

#include <TBranch.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1F.h>
#include <TLeaf.h>
#include <TTree.h>

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
   std::cout << "Usage: " << progname << " -i <B2HHH.root> -o <ntuple-path> -c <compression>" << std::endl;
}


int main(int argc, char **argv) {
   std::string inputFile = "B2HHH.root";
   std::string outputPath = ".";
   int compressionSettings = 0;
   std::string compressionShorthand = "none";

   int c;
   while ((c = getopt(argc, argv, "hvi:o:c:")) != -1) {
      switch (c) {
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
      default:
         fprintf(stderr, "Unknown option: -%c\n", c);
         Usage(argv[0]);
         return 1;
      }
   }
   std::string outputFile = outputPath + "/B2HHH~" + compressionShorthand + ".ntuple";
   std::cout << "Converting " << inputFile << " --> " << outputFile << std::endl;

   std::unique_ptr<TFile> f(TFile::Open(inputFile.c_str()));
   assert(f && ! f->IsZombie());

   // Get a unique pointer to an empty RNTuple model
   auto model = RNTupleModel::Create();

   // We create RNTuple fields based on the types found in the TTree
   // This simple approach only works for trees with simple branches and only one leaf per branch
   auto tree = f->Get<TTree>("DecayTree");
   for (auto b : TRangeDynCast<TBranch>(*tree->GetListOfBranches())) {
      // The dynamic cast to TBranch should never fail for GetListOfBranches()
      assert(b);

      // We assume every branch has a single leaf
      TLeaf *l = static_cast<TLeaf*>(b->GetListOfLeaves()->First());

      // Create an ntuple field with the same name and type than the tree branch
      auto field = RFieldBase::Create(l->GetName(), l->GetTypeName());
      std::cout << "Convert leaf " << l->GetName() << " [" << l->GetTypeName() << "]"
                << " --> " << "field " << field->GetName() << " [" << field->GetType() << "]" << std::endl;

      // Hand over ownership of the field to the ntuple model.  This will also create a memory location attached
      // to the model's default entry, that will be used to place the data supposed to be written
      model->AddField(std::unique_ptr<RFieldBase>(field));

      // We connect the model's default entry's memory location for the new field to the branch, so that we can
      // fill the ntuple with the data read from the TTree
      void *fieldDataPtr = model->GetDefaultEntry()->GetValue(l->GetName()).GetRawPtr();
      tree->SetBranchAddress(b->GetName(), fieldDataPtr);
   }

   // The new ntuple takes ownership of the model
   RNTupleWriteOptions options;
   options.SetCompression(compressionSettings);
   auto ntuple = RNTupleWriter::Recreate(std::move(model), "DecayTree", outputFile, options);

   auto nEntries = tree->GetEntries();
   for (decltype(nEntries) i = 0; i < nEntries; ++i) {
      tree->GetEntry(i);
      ntuple->Fill();

      if (i && i % 100000 == 0)
         std::cout << "Wrote " << i << " entries" << std::endl;
   }
}
