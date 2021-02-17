using RNTupleReader = ROOT::Experimental::RNTupleReader;
using RNTupleDescriptor = ROOT::Experimental::RNTupleDescriptor;
using DescriptorId_t = ROOT::Experimental::DescriptorId_t;


TH2D *gHistRatioFloat[3];
TH2D *gHistRatioDouble[3];
TH2D *gHistRatioInt32[3];
TH2D *gHistRatioInt64[3];
TH2D *gHistRatioIndex[3];

struct ColumnSize {
   std::string fName;
   DescriptorId_t fColId;
   std::int64_t fSize;

   bool operator <(const ColumnSize &other) {
      return fSize > other.fSize;
   }
};
std::vector<ColumnSize> gColumnsInMemory;
std::vector<ColumnSize> gColumnsOnDisk;

DescriptorId_t GetOtherId(const RNTupleDescriptor &desc1, const RNTupleDescriptor &desc2, DescriptorId_t fieldId)
{
   if (fieldId == desc1.GetFieldZeroId())
      return desc2.GetFieldZeroId();

   const auto &f = desc1.GetFieldDescriptor(fieldId);

   auto otherParent = GetOtherId(desc1, desc2, f.GetParentId());
   return desc2.FindFieldId(f.GetFieldName(), otherParent);
}

double GetCompressionOfColumn(const RNTupleDescriptor &desc, DescriptorId_t colId)
{
   const auto &colDesc = desc.GetColumnDescriptor(colId);
   auto elementSize = ROOT::Experimental::Detail::RColumnElementBase::Generate(colDesc.GetModel().GetType())->GetSize();

   std::uint64_t bytesOnStorage = 0;
   std::uint64_t bytesInMemory = 0;

   auto clusterId = desc.FindClusterId(colId, 0);
   while (clusterId != ROOT::Experimental::kInvalidDescriptorId) {
      const auto &clusterDesc = desc.GetClusterDescriptor(clusterId);
      const auto &pageRange = clusterDesc.GetPageRange(colId);
      for (const auto &page : pageRange.fPageInfos) {
         bytesOnStorage += page.fLocator.fBytesOnStorage;
         bytesInMemory += page.fNElements * elementSize;
      }

      clusterId = desc.FindNextClusterId(clusterId);
   }

   return double(bytesOnStorage) / double(bytesInMemory);
}

std::int64_t GetMemSizeOfColumn(const RNTupleDescriptor &desc, DescriptorId_t colId)
{
   const auto &colDesc = desc.GetColumnDescriptor(colId);
   auto elementSize = ROOT::Experimental::Detail::RColumnElementBase::Generate(colDesc.GetModel().GetType())->GetSize();
   return desc.GetNElements(colId) * elementSize;
}

std::int64_t GetOnDiskSizeOfColumn(const RNTupleDescriptor &desc, DescriptorId_t colId)
{
   const auto &colDesc = desc.GetColumnDescriptor(colId);
   std::uint64_t bytesOnStorage = 0;

   auto clusterId = desc.FindClusterId(colId, 0);
   while (clusterId != ROOT::Experimental::kInvalidDescriptorId) {
      const auto &clusterDesc = desc.GetClusterDescriptor(clusterId);
      const auto &pageRange = clusterDesc.GetPageRange(colId);
      for (const auto &page : pageRange.fPageInfos)
         bytesOnStorage += page.fLocator.fBytesOnStorage;

      clusterId = desc.FindNextClusterId(clusterId);
   }

   return bytesOnStorage;
}

void CompareImpl(const RNTupleDescriptor &desc1, const RNTupleDescriptor &desc2, DescriptorId_t fieldId)
{
   auto fieldName = desc1.GetQualifiedFieldName(fieldId);
   auto otherFieldId = GetOtherId(desc1, desc2, fieldId);

   for (const auto &c : desc1.GetColumnRange(fieldId)) {
      const auto otherColumnId = desc2.FindColumnId(otherFieldId, c.GetIndex());
      std::cout << std::setw(30);
      std::cout << fieldName << " [" << c.GetIndex() << "]";

      auto factor1 = GetCompressionOfColumn(desc1, c.GetId());
      auto factor2 = GetCompressionOfColumn(desc2, otherColumnId);
      std::cout << ": " << factor1 << " " << factor2 << std::endl;
      // split / non-split
      auto ratio = factor2 / factor1;
      auto memSizeMb = double(GetMemSizeOfColumn(desc1, c.GetId())) / 1000. / 1000.;

      if (isnan(ratio))
         continue;

      if (ratio > 1.5)
         std::cerr << "WARNING!!!!" << std::endl;

      switch (c.GetModel().GetType()) {
      case ROOT::Experimental::EColumnType::kReal32:
         gHistRatioFloat[0]->Fill(factor1, memSizeMb);
         gHistRatioFloat[1]->Fill(factor2, memSizeMb);
         gHistRatioFloat[2]->Fill(ratio, memSizeMb);
         break;
      case ROOT::Experimental::EColumnType::kReal64:
         gHistRatioDouble[0]->Fill(factor1, memSizeMb);
         gHistRatioDouble[1]->Fill(factor2, memSizeMb);
         gHistRatioDouble[2]->Fill(ratio, memSizeMb);
         break;
      case ROOT::Experimental::EColumnType::kInt32:
         gHistRatioInt32[0]->Fill(factor1, memSizeMb);
         gHistRatioInt32[1]->Fill(factor2, memSizeMb);
         gHistRatioInt32[2]->Fill(ratio, memSizeMb);
         break;
      case ROOT::Experimental::EColumnType::kInt64:
         gHistRatioInt64[0]->Fill(factor1, memSizeMb);
         gHistRatioInt64[1]->Fill(factor2, memSizeMb);
         gHistRatioInt64[2]->Fill(ratio, memSizeMb);
         break;
      case ROOT::Experimental::EColumnType::kIndex:
         gHistRatioIndex[0]->Fill(factor1, memSizeMb);
         gHistRatioIndex[1]->Fill(factor2, memSizeMb);
         gHistRatioIndex[2]->Fill(ratio, memSizeMb);
         break;
      case ROOT::Experimental::EColumnType::kBit:
         // Ok, nothing to split
         break;
      case ROOT::Experimental::EColumnType::kByte:
         // Ok, nothing to split
         break;
      default:
         std::cerr << "unsupported column type! " << (int)c.GetModel().GetType() << std::endl;
         abort();
      }
   }

   for (const auto &f : desc1.GetFieldRange(fieldId))
      CompareImpl(desc1, desc2, f.GetId());
}

void GetColumnSizes(std::int64_t &largestColumn, const RNTupleDescriptor &desc, DescriptorId_t fieldId)
{
   for (const auto &c : desc.GetColumnRange(fieldId)) {
      auto colSizeInMemory = GetMemSizeOfColumn(desc, c.GetId());
      largestColumn = std::max(colSizeInMemory, largestColumn);
      ColumnSize cs{desc.GetQualifiedFieldName(fieldId) + "[" + std::to_string(c.GetIndex()) + "]",
                    c.GetId(),
                    colSizeInMemory};
      gColumnsInMemory.emplace_back(cs);
      cs.fSize = GetOnDiskSizeOfColumn(desc, c.GetId());
      gColumnsOnDisk.emplace_back(cs);
   }

   for (const auto &f : desc.GetFieldRange(fieldId))
      GetColumnSizes(largestColumn, desc, f.GetId());
}


void splitzip(std::string ntupleName, std::string file1, std::string file2,
              std::string outfile = "splitzip.root")
{
   auto ntuple1 = RNTupleReader::Open(ntupleName, file1);
   auto ntuple2 = RNTupleReader::Open(ntupleName, file2);

   const auto &desc1 = ntuple1->GetDescriptor();
   const auto &desc2 = ntuple2->GetDescriptor();

   std::int64_t largestColumn = 0;
   GetColumnSizes(largestColumn, desc1, desc1.GetFieldZeroId());
   double largestColumnMB = double(largestColumn) / 1000. / 1000.;
   largestColumnMB *= 1.2;

   auto f = TFile::Open(outfile.c_str(), "RECREATE");
   gHistRatioFloat[0]  = new TH2D("n_floats",  "Non-split float",            25, 0, 1.1, 25, 0, largestColumnMB);
   gHistRatioDouble[0] = new TH2D("n_doubles", "Non-split double",           25, 0, 1.1, 25, 0, largestColumnMB);
   gHistRatioInt32[0]  = new TH2D("n_int32s",  "Non-split int32",            25, 0, 1.1, 25, 0, largestColumnMB);
   gHistRatioInt64[0]  = new TH2D("n_int64s",  "Non-split int64",            25, 0, 1.1, 25, 0, largestColumnMB);
   gHistRatioIndex[0]  = new TH2D("n_offsets", "Non-split offset",           25, 0, 1.1, 25, 0, largestColumnMB);
   gHistRatioFloat[1]  = new TH2D("s_floats",  "split float",                25, 0, 1.1, 25, 0, largestColumnMB);
   gHistRatioDouble[1] = new TH2D("s_doubles", "split double",               25, 0, 1.1, 25, 0, largestColumnMB);
   gHistRatioInt32[1]  = new TH2D("s_int32s",  "split int32",                25, 0, 1.1, 25, 0, largestColumnMB);
   gHistRatioInt64[1]  = new TH2D("s_int64s",  "split int64",                25, 0, 1.1, 25, 0, largestColumnMB);
   gHistRatioIndex[1]  = new TH2D("s_offsets", "split offset",               25, 0, 1.1, 25, 0, largestColumnMB);
   gHistRatioFloat[2]  = new TH2D("r_floats",  "ratio split/unsplit float",  25, 0.25, 1.5, 25, 0, largestColumnMB);
   gHistRatioDouble[2] = new TH2D("r_doubles", "ratio split/unsplit double", 25, 0.25, 1.5, 25, 0, largestColumnMB);
   gHistRatioInt32[2]  = new TH2D("r_int32s",  "ratio split/unsplit int32",  25, 0.25, 1.5, 25, 0, largestColumnMB);
   gHistRatioInt64[2]  = new TH2D("r_int64s",  "ratio split/unsplit int64",  25, 0.25, 1.5, 25, 0, largestColumnMB);
   gHistRatioIndex[2]  = new TH2D("r_offsets", "ratio split/unsplit offset", 25, 0.25, 1.5, 25, 0, largestColumnMB);

   CompareImpl(desc1, desc2, desc1.GetFieldZeroId());

   std::sort(gColumnsInMemory.begin(), gColumnsInMemory.end());
   std::cout << std::endl << "Largest columns in memory" << std::endl;
   for (unsigned i = 0; i < 8; ++i) {
      std::cout << gColumnsInMemory[i].fName << " " << gColumnsInMemory[i].fSize / 1000 / 1000 << "MB" << std::endl;
   }
   std::cout << std::endl << "Largest columns on disk" << std::endl;
   for (unsigned i = 0; i < 8; ++i) {
      std::cout << gColumnsOnDisk[i].fName << " " << gColumnsInMemory[i].fSize / 1000 / 1000 << "MB" << std::endl;
   }

   TCanvas *c1 = new TCanvas("c1", "c1", 0, 0, 1000, 1000);
   c1->SetTitle((file1 + " vs. " + file2).c_str());
   c1->Divide(3, 5);
   for (int i = 0; i < 3; ++i) {
      c1->cd(0 + (i+1));
      gHistRatioFloat[i]->Draw("colz");
      if (i < 2) gHistRatioFloat[i]->SetXTitle("Compression Ratio");
      gHistRatioFloat[i]->SetYTitle("Column Size (MB)");

      c1->cd(3 + (i+1));
      gHistRatioDouble[i]->Draw("colz");
      if (i < 2) gHistRatioDouble[i]->SetXTitle("Compression Ratio");
      gHistRatioDouble[i]->SetYTitle("Column Size (MB)");

      c1->cd(6 + (i+1));
      gHistRatioIndex[i]->Draw("colz");
      if (i < 2) gHistRatioIndex[i]->SetXTitle("Compression Ratio");
      gHistRatioIndex[i]->SetYTitle("Column Size (MB)");

      c1->cd(9 + (i+1));
      gHistRatioInt32[i]->Draw("colz");
      if (i < 2) gHistRatioInt32[i]->SetXTitle("Compression Ratio");
      gHistRatioInt32[i]->SetYTitle("Column Size (MB)");

      c1->cd(12 + (i+1));
      gHistRatioInt64[i]->Draw("colz");
      if (i < 2) gHistRatioInt64[i]->SetXTitle("Compression Ratio");
      gHistRatioInt64[i]->SetYTitle("Column Size (MB)");
   }

   for (int i = 0; i < 3; ++i) {
      gHistRatioFloat[i]->Write();
      gHistRatioDouble[i]->Write();
      gHistRatioInt32[i]->Write();
      gHistRatioInt64[i]->Write();
      gHistRatioIndex[i]->Write();
   }
   c1->Write();
   f->Close();

   delete c1;
   delete f;
}

void runAll() {
   std::string samples[4] = { "B2HHH", "h1dstX10", "ttjet_13tev_june2019", "gg_data" };
   std::string ntupleNames[4] = { "DecayTree", "h42", "NTuple", "mini" };
   std::string zipAlgos[4] = { "lz4", "zlib", "zstd", "lzma" };

   std::string unsplitPath = "/data/calibration/";
   std::string splitPath = "/data/calibration/split/";

   for (unsigned i = 0; i < 4; ++i) {
      for (unsigned j = 0; j < 4; ++j) {
         std::string inputFile = samples[i] + "~" + zipAlgos[j] + ".ntuple";
         std::string outputFile = std::string("splitzip-") + samples[i] + "~" + zipAlgos[j] + ".root";
         splitzip(ntupleNames[i], unsplitPath + inputFile, splitPath + inputFile, outputFile);
      }
   }
}
