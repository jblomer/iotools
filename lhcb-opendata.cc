/**
 * Copyright CERN; jblomer@cern.ch
 */

#include <unistd.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

#include <TChain.h>


struct KaonCandidate {
  double h_px, h_py, h_pz;
  double h_prob_k, h_prob_pi;
  bool h_charge;
  bool h_is_muon;
  double h_ip_chi2;  // unused
};

struct Event {
  double b_flight_distance;  // unused
  double b_vertex_chi2;  // unused
  std::array<KaonCandidate, 3> kaon_candidates;
};



static void Usage(const char *progname) {
  printf("%s [-i input.root] [-i ...]\n", progname);
}


static void AttachBranches2Event(TChain *root_chain, Event *event) {
  root_chain->SetBranchAddress("H1_PX", &event->kaon_candidates[0].h_px);
  root_chain->SetBranchAddress("H1_PY", &event->kaon_candidates[0].h_py);
  root_chain->SetBranchAddress("H1_PZ", &event->kaon_candidates[0].h_pz);
  root_chain->SetBranchAddress("H1_ProbK", &event->kaon_candidates[0].h_prob_k);
  root_chain->SetBranchAddress("H1_ProbPi",
                               &event->kaon_candidates[0].h_prob_pi);
  root_chain->SetBranchAddress("H1_Charge",
                               &event->kaon_candidates[0].h_charge);
  root_chain->SetBranchAddress("H1_isMuon",
                               &event->kaon_candidates[0].h_is_muon);

  root_chain->SetBranchAddress("H2_PX", &event->kaon_candidates[1].h_px);
  root_chain->SetBranchAddress("H2_PY", &event->kaon_candidates[1].h_py);
  root_chain->SetBranchAddress("H2_PZ", &event->kaon_candidates[1].h_pz);
  root_chain->SetBranchAddress("H2_ProbK", &event->kaon_candidates[1].h_prob_k);
  root_chain->SetBranchAddress("H2_ProbPi",
                               &event->kaon_candidates[1].h_prob_pi);
  root_chain->SetBranchAddress("H2_Charge",
                               &event->kaon_candidates[1].h_charge);
  root_chain->SetBranchAddress("H2_isMuon",
                               &event->kaon_candidates[1].h_is_muon);

  root_chain->SetBranchAddress("H3_PX", &event->kaon_candidates[2].h_px);
  root_chain->SetBranchAddress("H3_PY", &event->kaon_candidates[2].h_py);
  root_chain->SetBranchAddress("H3_PZ", &event->kaon_candidates[2].h_pz);
  root_chain->SetBranchAddress("H3_ProbK", &event->kaon_candidates[1].h_prob_k);
  root_chain->SetBranchAddress("H3_ProbPi",
                               &event->kaon_candidates[2].h_prob_pi);
  root_chain->SetBranchAddress("H3_Charge",
                               &event->kaon_candidates[2].h_charge);
  root_chain->SetBranchAddress("H3_isMuon",
                               &event->kaon_candidates[2].h_is_muon);
}


static void ProcessEvent(const Event &event) {
  for (const auto &k : event.kaon_candidates) {
    if (k.h_is_muon)
      return;
  }

  // Fill Histograms
}


int main(int argc, char **argv) {
  std::vector<std::string> input_paths;
  int c;
  while ((c = getopt(argc, argv, "hvi:")) != -1) {
    switch (c) {
      case 'h':
      case 'v':
        Usage(argv[0]);
        return 0;
      case 'i':
        input_paths.push_back(optarg);
        break;
      default:
        fprintf(stderr, "Unknown option: -%c\n", c);
        Usage(argv[0]);
        return 1;
    }
  }

  assert(!input_paths.empty());

  TChain root_chain("DecayTree");
  for (const auto &p : input_paths)
    root_chain.Add(p.c_str());

  Event event;
  AttachBranches2Event(&root_chain, &event);

  size_t i_event = 0;
  size_t no_events = root_chain.GetEntries();
  for (; i_event < no_events; ++i_event) {
    root_chain.GetEntry(i_event);
    ProcessEvent(event);
  }

  printf("processed %lu events\n", i_event);

  return 0;
}
