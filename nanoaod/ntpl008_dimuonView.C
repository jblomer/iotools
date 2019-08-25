/// \file
/// \ingroup tutorial_ntuple
/// \notebook
/// Convert CMS open data from a TTree to RNTuple.
/// This tutorial illustrates data conversion and data processing with RNTuple and RDataFrame.  In contrast to the
/// LHCb open data tutorial, the data model in this tutorial is not tabular but entries have variable lengths vectors
/// Based on RDataFrame's df102_NanoAODDimuonAnalysis.C
///
/// \macro_image
/// \macro_code
///
/// \date April 2019
/// \author The ROOT Team

// NOTE: The RNTuple classes are experimental at this point.
// Functionality, interface, and data format is still subject to changes.
// Do not use for real data!

#ifdef __CINT__
R__LOAD_LIBRARY(ROOTNTuple)
R__LOAD_LIBRARY(Hist)
#endif

#include <ROOT/RDataFrame.hxx>
#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleDS.hxx>
#include <ROOT/RVec.hxx>

#include <TCanvas.h>
#include <TH1.h>
#include <TH1F.h>
#include <TH1D.h>
#include <TLatex.h>
#include <TStyle.h>
#include <TSystem.h>

#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <utility>

#include "ntuple/classes.hxx"

// Import classes from experimental namespace for the time being
using RNTupleModel = ROOT::Experimental::RNTupleModel;
using RNTupleReader = ROOT::Experimental::RNTupleReader;
using RNTupleWriter = ROOT::Experimental::RNTupleWriter;
using RNTupleDS = ROOT::Experimental::RNTupleDS;

//constexpr char const* kNTupleFileName = "/data/cms/ntuple/ntuple.root";
//constexpr char const* kNTupleFileName = "ntuple/ntuple.root";
constexpr char const* kNTupleFileName = "/data/cms/ntuple/real-lzma.ntuple";


using ColNames_t = std::vector<std::string>;

// This is a custom action for RDataFrame. It does not support parallelism!
// This action writes data from an RDataFrame entry into an ntuple. It is templated on the
// types of the columns to be written and can be used as a generic file format converter.
template <typename... ColumnTypes_t>
class RNTupleHelper : public ROOT::Detail::RDF::RActionImpl<RNTupleHelper<ColumnTypes_t...>> {
public:
   using Result_t = RNTupleWriter;
private:
   using ColumnValues_t = std::tuple<std::shared_ptr<ColumnTypes_t>...>;

   std::string fNTupleName;
   std::string fRootFile;
   ColNames_t fColNames;
   ColumnValues_t fColumnValues;
   static constexpr const auto fNColumns = std::tuple_size<ColumnValues_t>::value;
   std::shared_ptr<RNTupleWriter> fNTuple;
   int fCounter;

   template<std::size_t... S>
   void InitializeImpl(std::index_sequence<S...>) {
      auto eventModel = ROOT::Experimental::RNTupleModel::Create();
      // Create the fields and the shared pointers to the connected values
      std::initializer_list<int> expander{
         (std::get<S>(fColumnValues) = eventModel->MakeField<ColumnTypes_t>(fColNames[S]), 0)...};
      fNTuple = std::move(RNTupleWriter::Recreate(std::move(eventModel), fNTupleName, fRootFile));
   }

   template<std::size_t... S>
   void ExecImpl(std::index_sequence<S...>, ColumnTypes_t... values) {
      // For every entry, set the destination of the ntuple's default entry's shared pointers to the given values,
      // which are provided by RDataFrame
      std::initializer_list<int> expander{(*std::get<S>(fColumnValues) = values, 0)...};
   }

public:
   RNTupleHelper(std::string_view ntupleName, std::string_view rootFile, const ColNames_t& colNames)
      : fNTupleName(ntupleName), fRootFile(rootFile), fColNames(colNames)
   {
      InitializeImpl(std::make_index_sequence<fNColumns>());
   }

   RNTupleHelper(RNTupleHelper&&) = default;
   RNTupleHelper(const RNTupleHelper&) = delete;
   std::shared_ptr<RNTupleWriter> GetResultPtr() const { return fNTuple; }

   void Initialize()
   {
      fCounter = 0;
   }

   void InitTask(TTreeReader *, unsigned int) {}

   /// This is a method executed at every entry
   void Exec(unsigned int slot, ColumnTypes_t... values)
   {
      // Populate the ntuple's fields data locations with the provided values, then write to disk
      ExecImpl(std::make_index_sequence<fNColumns>(), values...);
      fNTuple->Fill();
      if (++fCounter % 100000 == 0)
         std::cout << "Wrote " << fCounter << " entries" << std::endl;
   }

   void Finalize()
   {
      fNTuple->CommitCluster();
   }

   std::string GetActionName() { return "RNTuple Writer"; }
};


/// A wrapper for ROOT's InvariantMass function that takes std::vector instead of RVecs
template <typename T>
T InvariantMassStdVector(std::vector<T>& pt, std::vector<T>& eta, std::vector<T>& phi, std::vector<T>& mass)
{
   assert(pt.size() == eta.size() == phi.size() == mass.size() == 2);

   // We adopt the memory here, no copy
   ROOT::RVec<float> rvPt(pt);
   ROOT::RVec<float> rvEta(eta);
   ROOT::RVec<float> rvPhi(phi);
   ROOT::RVec<float> rvMass(mass);

   return InvariantMass(rvPt, rvEta, rvPhi, rvMass);
}


void ntpl008_dimuonView() {
   gSystem->Load("ntuple/libClasses.so");
   auto model = RNTupleModel::Create();
   auto ntuple = RNTupleReader::Open(std::move(model), "NTuple", kNTupleFileName);

   auto h = new TH1F("Dimuon_mass", "Dimuon_mass", 30000, 0.25, 300);

   auto viewMuon = ntuple->GetViewCollection("nMuon");
   auto viewMuonCharge = viewMuon.GetView<std::int32_t>("nMuon.Muon_charge");
   auto viewMuonPt = viewMuon.GetView<float>("nMuon.Muon_pt");
   auto viewMuonEta = viewMuon.GetView<float>("nMuon.Muon_eta");
   auto viewMuonPhi = viewMuon.GetView<float>("nMuon.Muon_phi");
   auto viewMuonMass = viewMuon.GetView<float>("nMuon.Muon_mass");

   int sum=0;
   for (auto entryId : ntuple->GetViewRange()) {
      if (entryId % 1000 == 0)
         std::cout << "Processed " << entryId << " entries" << std::endl;

      if (viewMuon(entryId) != 2)
         continue;

      std::int32_t charges[2];
      int i = 0;
      for (auto m : viewMuon.GetViewRange(entryId)) {
         charges[i++] = viewMuonCharge(m);
      }
      if (charges[0] == charges[1])
         continue;

      float pt[2];
      float eta[2];
      float phi[2];
      float mass[2];
      i = 0;
      for (auto m : viewMuon.GetViewRange(entryId)) {
         pt[i] = viewMuonPt(m);
         eta[i] = viewMuonEta(m);
         phi[i] = viewMuonPhi(m);
         mass[i] = viewMuonMass(m);
         ++i;
      }

      float x_sum = 0.;
      float y_sum = 0.;
      float z_sum = 0.;
      float e_sum = 0.;
      for (std::size_t i = 0u; i < 2; ++i) {
         // Convert to (e, x, y, z) coordinate system and update sums
         const auto x = pt[i] * std::cos(phi[i]);
         x_sum += x;
         const auto y = pt[i] * std::sin(phi[i]);
         y_sum += y;
         const auto z = pt[i] * std::sinh(eta[i]);
         z_sum += z;
         const auto e = std::sqrt(x * x + y * y + z * z + mass[i] * mass[i]);
         e_sum += e;
      }
      // Return invariant mass with (+, -, -, -) metric
      auto fmass = std::sqrt(e_sum * e_sum - x_sum * x_sum - y_sum * y_sum - z_sum * z_sum);
      h->Fill(fmass);
   }

   gStyle->SetOptStat(0); gStyle->SetTextFont(42);
   auto c = new TCanvas("c", "", 800, 700);
   c->SetLogx(); c->SetLogy();

   h->SetTitle("");
   h->GetXaxis()->SetTitle("m_{#mu#mu} (GeV)"); h->GetXaxis()->SetTitleSize(0.04);
   h->GetYaxis()->SetTitle("N_{Events}"); h->GetYaxis()->SetTitleSize(0.04);
   h->DrawCopy();

   TLatex label; label.SetNDC(true);
   label.DrawLatex(0.175, 0.740, "#eta");
   label.DrawLatex(0.205, 0.775, "#rho,#omega");
   label.DrawLatex(0.270, 0.740, "#phi");
   label.DrawLatex(0.400, 0.800, "J/#psi");
   label.DrawLatex(0.415, 0.670, "#psi'");
   label.DrawLatex(0.485, 0.700, "Y(1,2,3S)");
   label.DrawLatex(0.755, 0.680, "Z");
   label.SetTextSize(0.040); label.DrawLatex(0.100, 0.920, "#bf{CMS Open Data}");
   label.SetTextSize(0.030); label.DrawLatex(0.630, 0.920, "#sqrt{s} = 8 TeV, L_{int} = 11.6 fb^{-1}");
}

int main() {
   ntpl008_dimuonView();
}
