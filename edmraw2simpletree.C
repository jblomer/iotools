#include <vector>
#include <iostream>
#include <TInterpreter.h>
#include <ROOT/RDataFrame.hxx>

// http://opendata.cern.ch/record/63
auto filename = "/data/cms/D66F223A-6A9C-E111-AF57-003048F118D4.root";

class FEDRawData
{
public:
   vector<unsigned char> data_; //
};

class DoNotRecordParents
{
   ClassDef(DoNotRecordParents, 128);
};

class FEDRawDataCollection : public DoNotRecordParents
{
public:
   vector<FEDRawData> data_; //
   ClassDef(FEDRawDataCollection, 128);
};

namespace edm
{

template <typename T0>
class Wrapper;
template <>
class Wrapper<FEDRawDataCollection>
{

public:
   bool present;
   FEDRawDataCollection obj;
};
} // namespace edm

void distil()
{
   gInterpreter->GenerateDictionary("vector<vector<unsigned char>>");
   gInterpreter->GenerateDictionary("DoNotRecordParents");
   gInterpreter->GenerateDictionary("FEDRawData");
   gInterpreter->GenerateDictionary("FEDRawDataCollection");

   ROOT::EnableImplicitMT(4);
   TFile f(filename);
   ROOT::RDataFrame rdf("Events", &f);

   auto toVecOfVec = [](const FEDRawDataCollection &c) {
      vector<vector<unsigned char>> v;
      for (const auto &w : c.data_)
      {
         v.emplace_back(w.data_);
      }
      return v;
   };

   rdf.Define("v", toVecOfVec, {"FEDRawDataCollection_rawDataCollector__LHC.obj"})
       .Filter([](ULong64_t n) {if (n%1000 == 0)cout << n << endl;return true; }, {"rdfentry_"})
       .Snapshot<vector<vector<unsigned char>>>("Events", "rawContent.root", {"v"});
}

void edmraw2simpletree() {
  distil();
}
