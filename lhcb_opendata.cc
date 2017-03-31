/**
 * Copyright CERN; jblomer@cern.ch
 */

#include "lhcb_opendata.h"

#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include <TChain.h>

#include "util.h"


std::unique_ptr<EventWriter> EventWriter::Create(FileFormats format) {
  switch (format) {
    case FileFormats::kSqlite:
      return std::unique_ptr<EventWriter>(new EventWriterSqlite());
    default:
      abort();
  }
}


//------------------------------------------------------------------------------


void EventWriterSqlite::Open(const std::string &path) {
  assert(db_ == NULL);
  int retval = sqlite3_open_v2(
    path.c_str(), &db_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
  assert(retval == SQLITE_OK);
  printf("created sqlite database %s\n", path.c_str());
}

void EventWriterSqlite::WriteEvent(const Event &event) {

}

void EventWriterSqlite::Close() {
  sqlite3_close(db_);
  db_ = NULL;
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


/**
 * Not used for the analysis but required for converting into another file
 * format.
 */
void AttachUnusedBranches2Event(TChain *root_chain, Event *event) {
  root_chain->SetBranchAddress("B_FlightDistance", &event->b_flight_distance);
  root_chain->SetBranchAddress("B_VertexChi2", &event->b_vertex_chi2);
  root_chain->SetBranchAddress("H1_IPChi2",
                               &event->kaon_candidates[0].h_ip_chi2);
  root_chain->SetBranchAddress("H2_IPChi2",
                               &event->kaon_candidates[1].h_ip_chi2);
  root_chain->SetBranchAddress("H3_IPChi2",
                               &event->kaon_candidates[2].h_ip_chi2);
}


static void ProcessEvent(const Event &event) {
  for (const auto &k : event.kaon_candidates) {
    if (k.h_is_muon)
      return;
  }

  // Fill Histograms
}


static void Usage(const char *progname) {
  printf("%s [-i input.root] [-i ...] [-o output format]\n", progname);
}


int main(int argc, char **argv) {
  std::vector<std::string> input_paths;
  std::string output_format;
  int c;
  while ((c = getopt(argc, argv, "hvi:o:")) != -1) {
    switch (c) {
      case 'h':
      case 'v':
        Usage(argv[0]);
        return 0;
      case 'i':
        input_paths.push_back(optarg);
        break;
      case 'o':
        output_format = optarg;
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
  std::unique_ptr<EventWriter> event_writer{nullptr};
  if (!output_format.empty()) {
    AttachUnusedBranches2Event(&root_chain, &event);
    FileFormats format = GetFileFormat(output_format);
    assert(format != FileFormats::kRoot);
    event_writer = EventWriter::Create(format);
    event_writer->Open(StripSuffix(input_paths[0]) + "." + output_format);
  }

  size_t i_event = 0;
  size_t no_events = root_chain.GetEntries();
  for (; i_event < no_events; ++i_event) {
    root_chain.GetEntry(i_event);
    if (event_writer) {
      event_writer->WriteEvent(event);
    } else {
      ProcessEvent(event);
    }
  }

  printf("processed %lu events\n", i_event);
  if (event_writer) event_writer->Close();

  return 0;
}
