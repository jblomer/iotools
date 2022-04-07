#include <TFile.h>
#include <TTree.h>

#include <cstdio>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>

static constexpr const char *kTreeName = "hugefile";

void Hugefile_Read(const std::string &filename) {
   std::unique_ptr<TFile> file{TFile::Open(filename.c_str(), "READ")};
   auto tree = file->Get<TTree>(kTreeName);

   float field[3]{};
   for (size_t i = 0; i < std::extent<decltype(field)>::value; ++i)
     tree->SetBranchAddress(("field_" + std::to_string(i)).c_str(), &field[i]);

   auto nEntries = tree->GetEntries();
   for (decltype(nEntries) i = 0; i < nEntries; ++i) {
      tree->LoadTree(i);
      tree->GetEntry(i);
      if (i % 10000 == 0)
         printf("Read %lu entries\r", (unsigned long)i);
   }
   printf("Read %lu entries\n", (unsigned long)nEntries);
}

void Hugefile_Write(const std::string &filename, size_t nEntries) {
   TTree::SetMaxTreeSize(std::numeric_limits<Long_t>::max());
   std::unique_ptr<TFile> file{TFile::Open(filename.c_str(), "RECREATE")};
   file->SetCompressionLevel(ROOT::RCompressionSetting::ELevel::kUncompressed);
   auto tree = std::make_unique<TTree>(kTreeName, kTreeName);

   float field[3]{};
   for (size_t i = 0; i < std::extent<decltype(field)>::value; ++i) {
      auto branchName = "field_" + std::to_string(i);
      tree->Branch(branchName.c_str(), &field[i], (branchName + "/F").c_str());
   }

   for (size_t i = 0; i < nEntries; ++i) {
      field[0] = static_cast<float>(i);
      field[1] = static_cast<float>(i) + 0.1f;
      field[2] = static_cast<float>(i) + 1.1f;
      tree->Fill();
      if (i % 10000 == 0)
         printf("Wrote %lu entries\r", (unsigned long)i);
   }
   tree->Write();
   printf("Wrote %lu entries\n", (unsigned long)nEntries);
}
