#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleReader.hxx>
#include <ROOT/RNTupleWriter.hxx>
#include <ROOT/RNTupleWriteOptions.hxx>

#include <TROOT.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

static const char *kFileName = "NTPL_SMALLPAGES.root";

int main()
{
   ROOT::EnableImplicitMT();

   printf("Start writing\n");

   if (!std::filesystem::exists(kFileName)) {
      auto model = ROOT::RNTupleModel::Create();
      std::vector<std::shared_ptr<int>> ptrs;
      for (int i = 0; i < 10; ++i) {
         auto ptr = model->MakeField<std::vector<float>>("F" + std::to_string(i));
         ptr->emplace_back(i);
      }
      ROOT::RNTupleWriteOptions opts;
      opts.SetMaxUnzippedPageSize(16);
      opts.SetCompression(501);
      auto writer = ROOT::RNTupleWriter::Recreate(std::move(model), "ntpl", kFileName, opts);
      for (int i = 0; i < 300000; ++i) {
         writer->Fill();
         if (i % 100000 == 0)
            writer->CommitCluster();
      }
   } else {
      printf(" ... exists\n");
   }

   printf("Start reading\n");

   auto ts_read = std::chrono::steady_clock::now();

   auto reader = ROOT::RNTupleReader::Open("ntpl", kFileName);
   for (auto i : *reader) {
      if (i % 25000 == 0)
         printf("  ... event number %lu\n", i);
      reader->LoadEntry(i);
   }

   auto ts_done = std::chrono::steady_clock::now();
   auto duration_read = std::chrono::duration_cast<std::chrono::milliseconds>(ts_done - ts_read).count();
   std::cout << "Reading took " << duration_read << " milliseconds\n";

   return 0;
}
