#include "util_h5.h"

#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <string>
#include <chrono>
#include <iostream>
#include <future>

#include <TApplication.h>
#include <TCanvas.h>
#include <TH1D.h>
#include <TROOT.h>
#include <TStyle.h>
#include <TSystem.h>

constexpr double kKaonMassMeV = 493.677;

static void Show(TH1D *h) {
  new TApplication("", nullptr, nullptr);

  gStyle->SetTextFont(42);
  auto c = new TCanvas("c", "", 800, 700);
  h->GetXaxis()->SetTitle("m_{KKK} [MeV/c^{2}]");
  h->DrawCopy();
  c->Modified();

  std::cout << "press ENTER to exit..." << std::endl;
  auto future = std::async(std::launch::async, getchar);
  while (true) {
    gSystem->ProcessEvents();
    if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
      break;
  }
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

  H5Row h5row;
  auto fileId = H5Fopen(inputPath.c_str(), H5P_DEFAULT, H5F_ACC_RDONLY);
  assert(fileId >= 0);
  auto setId = H5Dopen(fileId, "/DecayTree", H5P_DEFAULT);
  assert(setId >= 0);
  auto spaceId = H5Screate_simple(1, &H5Row::kDefaultDimension, NULL);
  assert(spaceId >= 0);
  auto memSpaceId = H5Screate(H5S_SCALAR);
  assert(memSpaceId >= 0);

  H5Row::DataSet row;
  hsize_t count = 1;

  auto hMass = new TH1D("B_mass", "", 500, 5050, 5500);
  
  std::chrono::steady_clock::time_point ts_first = std::chrono::steady_clock::now();
  for (hsize_t i = 0; i < H5Row::kDefaultDimension; ++i) {
    if (i && (i % 100000 == 0))
      printf("  ... processed %llu events\n", i);

    auto retval = H5Sselect_hyperslab(spaceId, H5S_SELECT_SET, &i, NULL, &count, NULL);
    assert(retval >= 0);
    retval = H5Dread(setId, h5row.fTypeId, memSpaceId, spaceId, H5P_DEFAULT, &row);
    assert(retval >= 0);

    if (row.h1_is_muon || row.h2_is_muon || row.h3_is_muon) {
      continue;
    }

    constexpr double prob_k_cut = 0.5;
    if (row.h1_prob_k < prob_k_cut) continue;
    if (row.h2_prob_k < prob_k_cut) continue;
    if (row.h3_prob_k < prob_k_cut) continue;

    constexpr double prob_pi_cut = 0.5;
    if (row.h1_prob_pi > prob_pi_cut) continue;
    if (row.h2_prob_pi > prob_pi_cut) continue;
    if (row.h3_prob_pi > prob_pi_cut) continue;

    double b_px = row.h1_px + row.h2_px + row.h3_px;
    double b_py = row.h1_py + row.h2_py + row.h3_py;
    double b_pz = row.h1_pz + row.h2_pz + row.h3_pz;
    double b_p2 = GetP2(b_px, b_py, b_pz);
    double k1_E = GetKE(row.h1_px, row.h1_py, row.h1_pz);
    double k2_E = GetKE(row.h2_px, row.h2_py, row.h2_pz);
    double k3_E = GetKE(row.h3_px, row.h3_py, row.h3_pz);
    double b_E = k1_E + k2_E + k3_E;
    double b_mass = sqrt(b_E*b_E - b_p2);
    hMass->Fill(b_mass);
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
