/**
 * Copyright CERN; jblomer@cern.ch
 */

#include <unistd.h>

#include <cstdio>
#include <string>

#include <TChain.h>
#include <TTreeReader.h>


struct KaonCandidate {
  double h_px, h_py, h_pz;
  double h_prob_k, h_prob_pi;
  int h_charge;
  int h_is_muon;
  double h_ip_chi2;
};

struct Event {
  double b_flight_distance;
  double b_vertex_chi2;
  std::array<KaonCandidate, 3> kaon_candidates;
};


//------------------------------------------------------------------------------


void AnalyzeRootOptimized(TChain *root_chain) {
  TTreeReader reader(root_chain);

  TTreeReaderValue<int> val_h1_is_muon(reader, "H1_isMuon");
  TTreeReaderValue<int> val_h2_is_muon(reader, "H2_isMuon");
  TTreeReaderValue<int> val_h3_is_muon(reader, "H3_isMuon");

  TTreeReaderValue<double> val_h1_px(reader, "H1_PX");
  TTreeReaderValue<double> val_h1_py(reader, "H1_PY");
  TTreeReaderValue<double> val_h1_pz(reader, "H1_PZ");
  TTreeReaderValue<double> val_h1_prob_k(reader, "H1_ProbK");
  TTreeReaderValue<double> val_h1_prob_pi(reader, "H1_ProbPi");
  TTreeReaderValue<int> val_h1_charge(reader, "H1_Charge");

  TTreeReaderValue<double> val_h2_px(reader, "H2_PX");
  TTreeReaderValue<double> val_h2_py(reader, "H2_PY");
  TTreeReaderValue<double> val_h2_pz(reader, "H2_PZ");
  TTreeReaderValue<double> val_h2_prob_k(reader, "H2_ProbK");
  TTreeReaderValue<double> val_h2_prob_pi(reader, "H2_ProbPi");
  TTreeReaderValue<int> val_h2_charge(reader, "H2_Charge");

  TTreeReaderValue<double> val_h3_px(reader, "H3_PX");
  TTreeReaderValue<double> val_h3_py(reader, "H3_PY");
  TTreeReaderValue<double> val_h3_pz(reader, "H3_PZ");
  TTreeReaderValue<double> val_h3_prob_k(reader, "H3_ProbK");
  TTreeReaderValue<double> val_h3_prob_pi(reader, "H3_ProbPi");
  TTreeReaderValue<int> val_h3_charge(reader, "H3_Charge");

  unsigned nread = 0;
  unsigned nskipped = 0;
  double dummy = 0.0;
  while (reader.Next()) {
    nread++;
    if (*val_h1_is_muon || *val_h2_is_muon || *val_h3_is_muon) {
      nskipped++;
      continue;
    }

    dummy +=
      *val_h1_px + *val_h1_py + *val_h1_pz + *val_h1_prob_k + *val_h1_prob_pi +
      double(*val_h1_charge) +
      *val_h2_px + *val_h2_py + *val_h2_pz + *val_h2_prob_k + *val_h2_prob_pi +
      double(*val_h2_charge) +
      *val_h3_px + *val_h3_py + *val_h3_pz + *val_h3_prob_k + *val_h3_prob_pi +
      double(*val_h3_charge);

    if ((nread % 100000) == 0) {
      printf("processed %u k events\n", nread / 1000);
      //printf("dummy is %lf\n", dummy); abort();
    }
  }
  printf("Optimized TTreeReader run: %u events read, %u events skipped "
         "(dummy: %lf)\n", nread, nskipped, dummy);
}


void AnalyzeRootClassic(TChain *root_chain) {
  Event event;

  root_chain->SetBranchAddress("H1_PX", &event.kaon_candidates[0].h_px);
  root_chain->SetBranchAddress("H1_PY", &event.kaon_candidates[0].h_py);
  root_chain->SetBranchAddress("H1_PZ", &event.kaon_candidates[0].h_pz);
  root_chain->SetBranchAddress("H1_ProbK",
                               &event.kaon_candidates[0].h_prob_k);
  root_chain->SetBranchAddress("H1_ProbPi",
                               &event.kaon_candidates[0].h_prob_pi);
  root_chain->SetBranchAddress("H1_Charge",
                               &event.kaon_candidates[0].h_charge);
  root_chain->SetBranchAddress("H1_isMuon",
                               &event.kaon_candidates[0].h_is_muon);

  root_chain->SetBranchAddress("H2_PX", &event.kaon_candidates[1].h_px);
  root_chain->SetBranchAddress("H2_PY", &event.kaon_candidates[1].h_py);
  root_chain->SetBranchAddress("H2_PZ", &event.kaon_candidates[1].h_pz);
  root_chain->SetBranchAddress("H2_ProbK",
                               &event.kaon_candidates[1].h_prob_k);
  root_chain->SetBranchAddress("H2_ProbPi",
                               &event.kaon_candidates[1].h_prob_pi);
  root_chain->SetBranchAddress("H2_Charge",
                               &event.kaon_candidates[1].h_charge);
  root_chain->SetBranchAddress("H2_isMuon",
                               &event.kaon_candidates[1].h_is_muon);

  root_chain->SetBranchAddress("H3_PX", &event.kaon_candidates[2].h_px);
  root_chain->SetBranchAddress("H3_PY", &event.kaon_candidates[2].h_py);
  root_chain->SetBranchAddress("H3_PZ", &event.kaon_candidates[2].h_pz);
  root_chain->SetBranchAddress("H3_ProbK",
                               &event.kaon_candidates[2].h_prob_k);
  root_chain->SetBranchAddress("H3_ProbPi",
                               &event.kaon_candidates[2].h_prob_pi);
  root_chain->SetBranchAddress("H3_Charge",
                               &event.kaon_candidates[2].h_charge);
  root_chain->SetBranchAddress("H3_isMuon",
                               &event.kaon_candidates[2].h_is_muon);

  double dummy = 0.0;
  unsigned nskip = 0;
  unsigned nevent = root_chain->GetEntries();
  for (unsigned i = 0; i < nevent; ++i) {
    root_chain->GetEntry(i);

    unsigned old_nskip = nskip;
    for (const auto &k : event.kaon_candidates) {
      if (k.h_is_muon) {
        nskip++;
        break;
      }
    }
    if (nskip != old_nskip)
      continue;

    dummy +=
      event.kaon_candidates[0].h_px +
      event.kaon_candidates[0].h_py +
      event.kaon_candidates[0].h_pz +
      event.kaon_candidates[0].h_prob_k +
      event.kaon_candidates[0].h_prob_pi +
      double(event.kaon_candidates[0].h_charge) +
      event.kaon_candidates[1].h_px +
      event.kaon_candidates[1].h_py +
      event.kaon_candidates[1].h_pz +
      event.kaon_candidates[1].h_prob_k +
      event.kaon_candidates[1].h_prob_pi +
      double(event.kaon_candidates[1].h_charge) +
      event.kaon_candidates[2].h_px +
      event.kaon_candidates[2].h_py +
      event.kaon_candidates[2].h_pz +
      event.kaon_candidates[2].h_prob_k +
      event.kaon_candidates[2].h_prob_pi +
      double(event.kaon_candidates[2].h_charge);

    if ((i % 100000) == 0) {
      printf("processed %u k events\n", i / 1000);
    }
  }
  printf("Classic run: %u events read, %u events skipped "
         "(dummy: %lf)\n", nevent, nskip, dummy);
}


static void Usage(const char *progname) {
  printf("%s [-i input.root] [-r]\n", progname);
}


int main(int argc, char **argv) {
  std::string input_path;
  bool root_optimized = false;
  int c;
  while ((c = getopt(argc, argv, "hvi:r")) != -1) {
    switch (c) {
      case 'h':
      case 'v':
        Usage(argv[0]);
        return 0;
      case 'i':
        input_path = optarg;
        break;
      case 'r':
        root_optimized = true;
        break;
      default:
        fprintf(stderr, "Unknown option: -%c\n", c);
        Usage(argv[0]);
        return 1;
    }
  }

  TChain root_chain("DecayTree");
  root_chain.Add(input_path.c_str());

  if (root_optimized)
    AnalyzeRootOptimized(&root_chain);
  else
    AnalyzeRootClassic(&root_chain);

  return 0;
}
