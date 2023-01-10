#include "lhcb_event.h"

#include <h5hep/h5hep.hxx>

#include <cassert>
#include <chrono>
#include <cstdio>
#include <future>
#include <iostream>
#include <string>
#include <unistd.h>

#include <TApplication.h>
#include <TCanvas.h>
#include <TH1D.h>
#include <TROOT.h>
#include <TRootCanvas.h>
#include <TStyle.h>
#include <TSystem.h>

constexpr double kKaonMassMeV = 493.677;

static void Show(TH1D *h) {
  auto app = TApplication("", nullptr, nullptr);

  gStyle->SetTextFont(42);
  auto c = TCanvas("c", "", 800, 700);
  h->GetXaxis()->SetTitle("m_{KKK} [MeV/c^{2}]");
  h->DrawCopy();
  c.Modified();
  c.Update();
  static_cast<TRootCanvas*>(c.GetCanvasImp())
     ->Connect("CloseWindow()", "TApplication", gApplication, "Terminate()");
  app.Run();
}

static double GetP2(double px, double py, double pz)
{
   return px*px + py*py + pz*pz;
}

static double GetKE(double px, double py, double pz)
{
   double p2 = GetP2(px, py, pz);
   return sqrt(p2 + kKaonMassMeV*kKaonMassMeV);
}

static void Usage(char *progname) {
  printf("Usage: %s -i <input HDF5 file> [-s]\n", progname);
}

/// __COLUMN_MODEL__ is defined via -D compiler option (see Makefile) 
using Builder = h5hep::schema::SchemaBuilder<__COLUMN_MODEL__>;

auto InitSchema() {
  // h5hep allows omitting columns in the read schema; here we only specify the required columns
  return Builder::MakeStructNode<LhcbEvent>("LhcbEvent", {
      Builder::MakePrimitiveNode<double>("H1_PX", HOFFSET(LhcbEvent, H1_PX)),
      Builder::MakePrimitiveNode<double>("H1_PY", HOFFSET(LhcbEvent, H1_PY)),
      Builder::MakePrimitiveNode<double>("H1_PZ", HOFFSET(LhcbEvent, H1_PZ)),
      Builder::MakePrimitiveNode<double>("H1_ProbK", HOFFSET(LhcbEvent, H1_ProbK)),
      Builder::MakePrimitiveNode<double>("H1_ProbPi", HOFFSET(LhcbEvent, H1_ProbPi)),
      Builder::MakePrimitiveNode<int>("H1_isMuon", HOFFSET(LhcbEvent, H1_isMuon)),
      Builder::MakePrimitiveNode<double>("H2_PX", HOFFSET(LhcbEvent, H2_PX)),
      Builder::MakePrimitiveNode<double>("H2_PY", HOFFSET(LhcbEvent, H2_PY)),
      Builder::MakePrimitiveNode<double>("H2_PZ", HOFFSET(LhcbEvent, H2_PZ)),
      Builder::MakePrimitiveNode<double>("H2_ProbK", HOFFSET(LhcbEvent, H2_ProbK)),
      Builder::MakePrimitiveNode<double>("H2_ProbPi", HOFFSET(LhcbEvent, H2_ProbPi)),
      Builder::MakePrimitiveNode<int>("H2_isMuon", HOFFSET(LhcbEvent, H2_isMuon)),
      Builder::MakePrimitiveNode<double>("H3_PX", HOFFSET(LhcbEvent, H3_PX)),
      Builder::MakePrimitiveNode<double>("H3_PY", HOFFSET(LhcbEvent, H3_PY)),
      Builder::MakePrimitiveNode<double>("H3_PZ", HOFFSET(LhcbEvent, H3_PZ)),
      Builder::MakePrimitiveNode<double>("H3_ProbK", HOFFSET(LhcbEvent, H3_ProbK)),
      Builder::MakePrimitiveNode<double>("H3_ProbPi", HOFFSET(LhcbEvent, H3_ProbPi)),
      Builder::MakePrimitiveNode<int>("H3_isMuon", HOFFSET(LhcbEvent, H3_isMuon)),
    });
}

int main(int argc, char **argv) {
  std::string inputPath;
  bool show = false;

  int c;
  while ((c = getopt(argc, argv, "hvi:s")) != -1) {
    switch (c) {
      case 'h':
      case 'v':
        Usage(argv[0]);
        return 0;
      case 'i':
        inputPath = optarg;
        break;
      case 's':
	show = true;
	break;
      default:
        fprintf(stderr, "Unknown option: -%c\n", c);
        Usage(argv[0]);
        return 1;
    }
  }
  assert(!inputPath.empty());

  auto ts_init = std::chrono::steady_clock::now();

  auto schema = InitSchema();
  auto file = h5hep::H5File::Open(inputPath);
  // Use the default RNTuple cluster size as the size for HDF5 chunk cache
  std::static_pointer_cast<h5hep::H5File>(file)->SetCache(50 * 1000 * 1000);
  auto reader = Builder::MakeReaderWriter(file, schema);

  auto num_chunks = reader->GetNChunks();
  auto chunk = std::make_unique<LhcbEvent[]>(reader->GetWriteProperties().GetChunkSize());

  auto hMass = new TH1D("B_mass", "", 500, 5050, 5500);
  
  std::chrono::steady_clock::time_point ts_first = std::chrono::steady_clock::now();
  size_t count = 0;
  for (size_t chunkIdx = 0; chunkIdx < num_chunks; ++chunkIdx) {
    printf("processed %lu k events\n", count / 1000);

    auto num_rows = reader->ReadChunk(chunkIdx, chunk.get());
    for (size_t i = 0; i < num_rows; ++i) {
      const auto &row = chunk[i];
      if (row.H1_isMuon || row.H2_isMuon || row.H3_isMuon) {
	continue;
      }

      constexpr double prob_k_cut = 0.5;
      if (row.H1_ProbK < prob_k_cut) continue;
      if (row.H2_ProbK < prob_k_cut) continue;
      if (row.H3_ProbK < prob_k_cut) continue;

      constexpr double prob_pi_cut = 0.5;
      if (row.H1_ProbPi > prob_pi_cut) continue;
      if (row.H2_ProbPi > prob_pi_cut) continue;
      if (row.H3_ProbPi > prob_pi_cut) continue;

      double b_px = row.H1_PX + row.H2_PX + row.H3_PX;
      double b_py = row.H1_PY + row.H2_PY + row.H3_PY;
      double b_pz = row.H1_PZ + row.H2_PZ + row.H3_PZ;
      double b_p2 = GetP2(b_px, b_py, b_pz);
      double k1_E = GetKE(row.H1_PX, row.H1_PY, row.H1_PZ);
      double k2_E = GetKE(row.H2_PX, row.H2_PY, row.H2_PZ);
      double k3_E = GetKE(row.H3_PX, row.H3_PY, row.H3_PZ);
      double b_E = k1_E + k2_E + k3_E;
      double b_mass = sqrt(b_E*b_E - b_p2);
      hMass->Fill(b_mass);
    }
    count += num_rows;
  }
  auto ts_end = std::chrono::steady_clock::now();
  auto runtime_init = std::chrono::duration_cast<std::chrono::microseconds>(ts_first - ts_init).count();
  auto runtime_analyze = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_first).count();

  std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
  std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;

  if (show)
    Show(hMass);
  delete hMass;

  return 0;
}
