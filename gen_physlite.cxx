#include <ROOT/RField.hxx>
#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleOptions.hxx>

#include <TBranch.h>
#include <TFile.h>
#include <TLeaf.h>
#include <TROOT.h>
#include <TTree.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <utility>

#include <unistd.h>

#include "util.h"

using RCollectionNTupleWriter = ROOT::Experimental::RCollectionNTupleWriter;
using RFieldBase = ROOT::Experimental::Detail::RFieldBase;
using RNTupleModel = ROOT::Experimental::RNTupleModel;
using RNTupleWriteOptions = ROOT::Experimental::RNTupleWriteOptions;
using RNTupleWriter = ROOT::Experimental::RNTupleWriter;

static std::string SanitizeBranchName(std::string name)
{
   size_t pos = 0;
   while ((pos = name.find(".", pos)) != std::string::npos) {
      name.replace(pos, 1, "__");
      pos += 2;
   }
   return name;
}

struct FlatField {
   std::string treeName;
   std::string ntupleName;
   std::string typeName;
   std::unique_ptr<unsigned char []> treeBuffer; // todo (now used for collection fields)
   std::unique_ptr<unsigned char []> ntupleBuffer; // todo (now used for collection fields)
   unsigned int fldSize;
};

struct LeafCountCollection {
   std::string treeName;
   std::string ntupleName;
   std::vector<FlatField> collectionFields;
   std::shared_ptr<RCollectionNTupleWriter> collectionWriter;
   int count = 0; // TODO: this can be also an uint or something else ?

   LeafCountCollection(const std::string &n) : treeName(n), ntupleName(SanitizeBranchName(treeName)) {}
};

static void Usage(char *progname)
{
   std::cout << "Usage: " << progname << " -i <physlite.root> -o <ntuple-path> -c <compression> "
             << "-H <header path> -b <bloat factor> [-m(t)] [-t(ree name)]"
             << std::endl;
}

int main(int argc, char **argv)
{
   std::string inputFile = "physlite.root";
   std::string outputPath = ".";
   int compressionSettings = 0;
   std::string compressionShorthand = "none";
   std::string headers;
   unsigned bloatFactor = 1;
   std::string treeName = "tree";

   int c;
   while ((c = getopt(argc, argv, "hvi:o:c:H:b:mt:")) != -1) {
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
      case 'H':
         headers = optarg;
         break;
      case 'b':
         bloatFactor = std::stoi(optarg);
         assert(bloatFactor > 0);
         break;
      case 'm':
         ROOT::EnableImplicitMT();
         break;
      case 't':
         treeName = optarg;
         break;
      default:
         fprintf(stderr, "Unknown option: -%c\n", c);
         Usage(argv[0]);
         return 1;
      }
   }
   std::string dsName = "physlite";
   if (bloatFactor > 1) {
      std::cout << " ... bloat factor x" << bloatFactor << std::endl;
      dsName += "X" + std::to_string(bloatFactor);
   }
   std::string outputFile = outputPath + "/" + dsName + "~" + compressionShorthand + ".ntuple";

   std::vector<FlatField> flatFields;
   std::vector<std::unique_ptr<LeafCountCollection>> leafCountCollections;

   std::unique_ptr<TFile> f(TFile::Open(inputFile.c_str()));
   assert(f && ! f->IsZombie());

   auto tree = f->Get<TTree>(treeName.c_str());
   for (auto b : TRangeDynCast<TBranch>(*tree->GetListOfBranches())) {
      // The dynamic cast to TBranch should never fail for GetListOfBranches()
      assert(b);
      assert(b->GetNleaves() == 1);
      auto l = static_cast<TLeaf*>(b->GetListOfLeaves()->First());

      std::cout << l->GetName() << " [" << l->GetTypeName() << "]" << std::endl;
      auto field = RFieldBase::Create(l->GetName(), l->GetTypeName()).Unwrap();
      auto szLeaf = l->GetLeafCount();
      if (szLeaf) {
         auto iter = find_if(std::begin(leafCountCollections), std::end(leafCountCollections),
            [szLeaf](const std::unique_ptr<LeafCountCollection> &c){return c->treeName == szLeaf->GetName();});
         if (iter == leafCountCollections.end()) {
            auto newCollection = std::make_unique<LeafCountCollection>(szLeaf->GetName());
            newCollection->count = szLeaf->GetMaximum();
            leafCountCollections.push_back(std::move(newCollection));
            iter = leafCountCollections.begin() + leafCountCollections.size() - 1;
         }
         (*iter)->collectionFields.push_back({l->GetName(), SanitizeBranchName(l->GetName()), l->GetTypeName()});
      } else {
         flatFields.push_back({l->GetName(), SanitizeBranchName(l->GetName()), l->GetTypeName()});
      }
   }

   auto model = RNTupleModel::Create();

   for (auto &c : leafCountCollections) {
      auto needle = std::find_if(std::begin(flatFields), std::end(flatFields),
                                 [&c](const FlatField &f){return f.treeName == c->treeName;});
      assert(needle != flatFields.end());
      flatFields.erase(needle);

      //std::cout << "SetBranchAddress " << c->treeName << std::endl;
      auto collectionModel = RNTupleModel::Create();
      for (auto &f : c->collectionFields) {
         auto field = RFieldBase::Create(f.ntupleName, f.typeName).Unwrap();
         f.fldSize = field->GetValueSize();

         f.treeBuffer = std::make_unique<unsigned char []>(field->GetValueSize() * c->count);
         tree->SetBranchAddress(f.treeName.c_str(), (void *)f.treeBuffer.get());

         //collectionModel->AddField(std::move(field));
         //fDefaultEntry->AddValue(field->GenerateValue());
         f.ntupleBuffer = std::make_unique<unsigned char []>(field->GetValueSize());
         collectionModel->GetDefaultEntry()->CaptureValue(field->CaptureValue(f.ntupleBuffer.get()));
         collectionModel->GetFieldZero()->Attach(std::move(field));
      }
      tree->SetBranchAddress(c->treeName.c_str(), (void *)&c->count);

      c->collectionWriter = model->MakeCollection(c->ntupleName, std::move(collectionModel));
   }

   for (auto &f : flatFields) {
      std::cout << f.ntupleName << std::endl;

      auto field = RFieldBase::Create(f.ntupleName, f.typeName).Unwrap();
      model->AddField(std::move(field));

      // We connect the model's default entry's memory location for the new field to the branch, so that we can
      // fill the ntuple with the data read from the TTree
      void *fieldDataPtr = model->GetDefaultEntry()->GetValue(f.ntupleName).GetRawPtr();
      tree->SetBranchAddress(f.treeName.c_str(), fieldDataPtr);
   }

   RNTupleWriteOptions options;
   options.SetCompression(compressionSettings);
   auto ntuple = RNTupleWriter::Recreate(std::move(model), "events", outputFile, options);

   auto nEntries = tree->GetEntries();
   for (decltype(nEntries) i = 0; i < nEntries; ++i) {
      tree->GetEntry(i);

      for (const auto &c : leafCountCollections) {
         //std::cout << "FILLING " << i << " " << c->treeName << " " << c->count << std::endl;
         for (int l = 0; l < c->count; ++l) {
            for (auto &f : c->collectionFields) {
               memcpy(f.ntupleBuffer.get(), f.treeBuffer.get() + (l * f.fldSize), f.fldSize);
            }
            c->collectionWriter->Fill();
            //std::cout << "FILLING " << i << " " << c->treeName << " " << c->count << std::endl;
         }
      }

      ntuple->Fill();

      if (i && i % 10000 == 0)
         std::cout << "Wrote " << i << " entries" << std::endl;
   }
}
