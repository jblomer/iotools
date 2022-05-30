#include <ROOT/RField.hxx>
#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleOptions.hxx>

#include <TBranch.h>
#include <TBranchElement.h>
#include <TBranchSTL.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1F.h>
#include <TLeaf.h>
#include <TTree.h>
#include <TROOT.h>

#include <cassert>
#include <iostream>
#include <map>
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
   std::cout << "Usage: " << progname <<
      " [-b <bloat factor>] -i <gg_*.root> -o <ntuple-path> [-m(t)] -c <compression>" << std::endl;
}


int main(int argc, char **argv) {
   std::string inputFile = "gg_data.root";
   std::string outputPath = ".";
   int compressionSettings = 0;
   std::string compressionShorthand = "none";
   int bloatFactor = 1;

   int c;
   while ((c = getopt(argc, argv, "hvi:o:c:mb:")) != -1) {
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
      case 'm':
         ROOT::EnableImplicitMT();
         break;
      case 'b':
         bloatFactor = atoi(optarg);
         break;
      default:
         fprintf(stderr, "Unknown option: -%c\n", c);
         Usage(argv[0]);
         return 1;
      }
   }
   std::string bloatTag;
   if (bloatFactor > 1)
      bloatTag = std::string("X") + std::to_string(bloatFactor);
   std::string flavor = SplitString(GetFileName(StripSuffix(inputFile)), '~')[0];
   std::string outputFile = outputPath + "/" + flavor + bloatTag + "~" + compressionShorthand + ".ntuple";
   std::cout << "Converting " << inputFile << " --> " << outputFile << std::endl;

   std::unique_ptr<TFile> f(TFile::Open(inputFile.c_str()));
   assert(f && ! f->IsZombie());

   // Get a unique pointer to an empty RNTuple model
   auto model = RNTupleModel::CreateBare();
   std::map<std::string, void *> fieldAdresses;

   // We create RNTuple fields based on the types found in the TTree
   // This simple approach only works for trees with simple branches and only one leaf per branch
   auto tree = f->Get<TTree>("mini");
   tree->SetImplicitMT(false);
   for (auto b : TRangeDynCast<TBranch>(*tree->GetListOfBranches())) {
      // The dynamic cast to TBranch should never fail for GetListOfBranches()
      assert(b);

      // We assume every branch has a single leaf
      TLeaf *l = static_cast<TLeaf*>(b->GetListOfLeaves()->First());

      // Create an ntuple field with the same name and type than the tree branch
      auto field = RFieldBase::Create(l->GetName(), l->GetTypeName()).Unwrap();
      std::cout << "Convert leaf " << l->GetName() << " [" << l->GetTypeName() << "]"
                << " --> " << "field " << field->GetName() << " [" << field->GetType() << "]" << std::endl;

      auto fieldName = field->GetName();
      auto typeName = field->GetType();

      if (typeid(*b) == typeid(TBranchSTL) || typeid(*b) == typeid(TBranchElement)) {
         // TODO: generic way of dealing with vector<T>
         if (typeName == "std::vector<bool>") {
            std::vector<bool> **v = new std::vector<bool> *();
            tree->SetBranchAddress(b->GetName(), v);
            fieldAdresses[fieldName] = *v;
         } else if (typeName == "std::vector<float>") {
            std::vector<float> **v = new std::vector<float> *();
            tree->SetBranchAddress(b->GetName(), v);
            fieldAdresses[fieldName] = *v;
         } else if (typeName == "std::vector<std::int32_t>") {
            std::vector<std::int32_t> **v = new std::vector<std::int32_t> *();
            tree->SetBranchAddress(b->GetName(), v);
            fieldAdresses[fieldName] = *v;
         } else if (typeName == "std::vector<std::uint32_t>") {
            std::vector<std::uint32_t> **v = new std::vector<std::uint32_t> *();
            tree->SetBranchAddress(b->GetName(), v);
            fieldAdresses[fieldName] = *v;
         } else {
            std::cout << "Unhandled " << typeName << std::endl;
            assert(false);
         }
      } else {
         void *addr = field->GenerateValue().GetRawPtr();
         fieldAdresses[fieldName] = addr;
         tree->SetBranchAddress(b->GetName(), addr);
      }

      // Hand over ownership of the field to the ntuple model.  This will also create a memory location attached
      // to the model's default entry, that will be used to place the data supposed to be written
      model->AddField(std::move(field));
   }
   model->Freeze();

   auto entry = model->CreateBareEntry();
   for (const auto &f : fieldAdresses) {
      entry->CaptureValueUnsafe(f.first, f.second);
   }

   // The new ntuple takes ownership of the model
   RNTupleWriteOptions options;
   options.SetCompression(compressionSettings);
   //options.SetNEntriesPerCluster(128000);
   auto ntuple = RNTupleWriter::Recreate(std::move(model), "mini", outputFile, options);

   for (int b = 0; b < bloatFactor; ++b) {
     auto nEntries = tree->GetEntries();
     std::cout << "Processing " << nEntries << " entries" << std::endl;
     for (decltype(nEntries) i = 0; i < nEntries; ++i) {
        tree->GetEntry(i);
        ntuple->Fill(*entry);

        if (i && i % 100000 == 0)
           std::cout << "Wrote " << i << " entries" << std::endl;
     }
   }

   std::cout << "Done" << std::endl;
   tree->ResetBranchAddresses();
}
