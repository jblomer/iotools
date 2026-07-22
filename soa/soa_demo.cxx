#include "SoAPoint.hxx"

#include <ROOT/RField.hxx>
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleReader.hxx>
#include <ROOT/RNTupleWriter.hxx>

#include <TSystem.h>

#include <cstddef>
#include <iostream>
#include <utility>

const constexpr char *kFileName = "soa.root";
const constexpr char *kNtplName = "ntpl";

static void PrintPoint(const SoAPoint &point)
{
   std::cout << "fMyScalar: " << point.fMyScalar << '\n';
   std::cout << "fX: ";
   for (auto v : point.fSoALayout.fX) {
      std::cout << v << ' ';
   }
   std::cout << '\n';
   std::cout << "fY: ";
   for (auto v : point.fSoALayout.fY) {
      std::cout << v << ' ';
   }
   std::cout << '\n';
}

// Write a single SoAPoint entry (using adopted memory)
static void Write()
{
   std::size_t N = 16;

   SoAPoint point;
   point.InitWithAdoptedMemory(N);
   point.fMyScalar = 137;

   for (std::size_t i = 0; i < N; ++i) {
      point.fSoALayout.fX[i] = 2.0 * i;
      point.fSoALayout.fY[i] = 2.0 * i + 1.0;
   }

   auto model = ROOT::RNTupleModel::CreateBare();
   model->AddField(std::make_unique<ROOT::RField<SoAPoint>>("point"));
   auto writer = ROOT::RNTupleWriter::Recreate(std::move(model), kNtplName, kFileName);
   auto e = writer->GetModel().CreateBareEntry();
   e->BindRawPtr("point", &point);
   writer->Fill(*e);
}

// All the magic of reading into adopted memory is done here in the I/O customization rule of SoAPoint.
// This is similar to the current CMSSW approach.
// A disadvantage of this approach is that the SoA type is buffered first and the SoA columns get memcpy'd into
// their final destination.
static void ReadOneShot()
{
   SoAPoint point;

   auto reader = ROOT::RNTupleReader::Open(kNtplName, kFileName);

   auto viewPoint = reader->GetView("point", &point);
   viewPoint(0);

   PrintPoint(point);
}

// A more clever, piecewise approach to reading SoAPoint:
//   1. Query the column size
//   2. Read the rectangular SoA part (fSoALayout) directly into the final destination (e.g., pinned memory)
//   3. Read all other parts of the SoAPoint (if any)
//
// Note that this approach works perfectly fine with the I/O customization rule set on SoAPoint because we never
// read SoAPoint itself, but we read its members.
//
// I fact, if the I/O customization rule was not present, it would also be fine to query the column size, prepare
// the fSoALayout member, and then to read the entire SoAPoint object in one go.
static void ReadClever()
{
   SoAPoint point;

   auto reader = ROOT::RNTupleReader::Open(kNtplName, kFileName);

   auto viewN = reader->GetCollectionView("point.fSoALayout");
   auto viewPointSoA = reader->GetView("point.fSoALayout", &point.fSoALayout, "SoAPoint::SoALayout");
   auto viewPointScalar = reader->GetView("point.fMyScalar", &point.fMyScalar);

   point.InitWithAdoptedMemory(viewN(0));
   viewPointSoA(0);
   viewPointScalar(0);

   PrintPoint(point);
}

int main()
{
   gSystem->Load("./libSoAPoint.so");

   Write();
   std::cout << "wrote " << kFileName << '\n';

   // Quick cross-check: what's in the written file?
   auto reader = ROOT::RNTupleReader::Open(kNtplName, kFileName);
   std::cout << "on-disk schema:\n";
   reader->PrintInfo();
   std::cout << "first entry in SoA layout:\n";
   reader->Show(0);

   // Showcase two possible ways of reading the data
   ReadOneShot();
   ReadClever();

   return 0;
}
