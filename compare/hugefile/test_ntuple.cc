#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleOptions.hxx>
#include <TROOT.h>

#include <cstdio>
#include <memory>
#include <string>
#include <type_traits>

using RNTupleModel = ROOT::Experimental::RNTupleModel;
using RNTupleReader = ROOT::Experimental::RNTupleReader;
using RNTupleWriter = ROOT::Experimental::RNTupleWriter;
using RNTupleWriteOptions = ROOT::Experimental::RNTupleWriteOptions;

static constexpr const char *kNtupleName = "hugefile";

void Hugefile_Read(const std::string &filename) {
   ROOT::EnableImplicitMT();
   auto model = RNTupleModel::Create();
   auto ntuple = RNTupleReader::Open(std::move(model), kNtupleName, filename);

   auto viewField0 = ntuple->GetView<float>("field_0");
   auto viewField1 = ntuple->GetView<float>("field_1");
   auto viewField2 = ntuple->GetView<float>("field_2");

   for (auto i : ntuple->GetEntryRange()) {
      (void)viewField0(i);
      (void)viewField1(i);
      (void)viewField2(i);
      if (i % 10000 == 0)
         printf("Read %lu entries\r", (unsigned long)i);
   }
   printf("Read %lu entries\n", (unsigned long)ntuple->GetNEntries());
}

void Hugefile_Write(const std::string &filename, size_t nEntries) {
   ROOT::EnableImplicitMT();
   auto model = RNTupleModel::Create();
   std::shared_ptr<float> field[3];
   for (size_t i = 0; i < std::extent<decltype(field)>::value; ++i) {
      field[i] = model->MakeField<float>("field_" + std::to_string(i), 0.0f);
   }

   RNTupleWriteOptions options;
   options.SetCompression(0);
   auto ntuple = RNTupleWriter::Recreate(std::move(model), kNtupleName, filename, options);

   for (size_t i = 0; i < nEntries; ++i) {
      *field[0] = static_cast<float>(i);
      *field[1] = static_cast<float>(i) + 0.1f;
      *field[2] = static_cast<float>(i) + 1.1f;
      ntuple->Fill();
      if (i % 10000 == 0)
         printf("Wrote %lu entries\r", (unsigned long)i);
   }
   printf("Wrote %lu entries\n", (unsigned long)nEntries);
}
