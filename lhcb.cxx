/**
 * Copyright CERN; jblomer@cern.ch
 */

#include "lhcb.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <ROOT/RDataFrame.hxx>
#include <ROOT/RNTupleDS.hxx>
#include <ROOT/RNTupleOptions.hxx>
#include <Compression.h>
#include <TClassTable.h>
#include <TFile.h>
#include <TSystem.h>
#include <TTree.h>
#include <TTreeReader.h>
#include <TTreePerfStats.h>

#include "util.h"

bool g_perf_stats = false;


std::unique_ptr<EventWriter> EventWriter::Create(FileFormats format) {
  switch (format) {
    case FileFormats::kRootDeflated:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(
        EventWriterRoot::CompressionAlgorithms::kCompressionDeflate,
        EventWriterRoot::SplitMode::kSplitManual));
    case FileFormats::kRootLz4:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(
        EventWriterRoot::CompressionAlgorithms::kCompressionLz4,
        EventWriterRoot::SplitMode::kSplitManual));
    case FileFormats::kRootLzma:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(
        EventWriterRoot::CompressionAlgorithms::kCompressionLzma,
        EventWriterRoot::SplitMode::kSplitManual));
    case FileFormats::kRootInflated:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(
        EventWriterRoot::CompressionAlgorithms::kCompressionNone,
        EventWriterRoot::SplitMode::kSplitManual));
    case FileFormats::kRootRow:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(
        EventWriterRoot::CompressionAlgorithms::kCompressionNone,
        EventWriterRoot::SplitMode::kSplitNone));
    case FileFormats::kRootAutosplitInflated:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(
        EventWriterRoot::CompressionAlgorithms::kCompressionNone,
        EventWriterRoot::SplitMode::kSplitAuto));
    case FileFormats::kRootAutosplitDeflated:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(
        EventWriterRoot::CompressionAlgorithms::kCompressionDeflate,
        EventWriterRoot::SplitMode::kSplitAuto));
    case FileFormats::kRootDeepsplitInflated:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(
        EventWriterRoot::CompressionAlgorithms::kCompressionNone,
        EventWriterRoot::SplitMode::kSplitDeep));
    case FileFormats::kRootDeepsplitDeflated:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(
        EventWriterRoot::CompressionAlgorithms::kCompressionDeflate,
        EventWriterRoot::SplitMode::kSplitDeep));
    case FileFormats::kRootDeepsplitLz4:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(
        EventWriterRoot::CompressionAlgorithms::kCompressionLz4,
        EventWriterRoot::SplitMode::kSplitDeep));
    default:
      abort();
  }
}


std::unique_ptr<EventReader> EventReader::Create(FileFormats format) {
  switch (format) {
    case FileFormats::kRoot:
    case FileFormats::kRootDeflated:
    case FileFormats::kRootLz4:
    case FileFormats::kRootLzma:
    case FileFormats::kRootInflated:
      return std::unique_ptr<EventReader>(new EventReaderRoot(
        EventReaderRoot::SplitMode::kSplitManual));
    case FileFormats::kRootAutosplitInflated:
    case FileFormats::kRootAutosplitDeflated:
      return std::unique_ptr<EventReader>(new EventReaderRoot(
        EventReaderRoot::SplitMode::kSplitAuto));
    case FileFormats::kRootDeepsplitInflated:
    case FileFormats::kRootDeepsplitDeflated:
    case FileFormats::kRootDeepsplitLz4:
      return std::unique_ptr<EventReader>(new EventReaderRoot(
        EventReaderRoot::SplitMode::kSplitDeep));
    case FileFormats::kRootRow:
      return std::unique_ptr<EventReader>(new EventReaderRoot(
        EventReaderRoot::SplitMode::kSplitNone));
    case FileFormats::kNtuple:
    case FileFormats::kNtupleInflated:
    case FileFormats::kNtupleDeflated:
      return std::unique_ptr<EventReader>(new EventReaderNtuple());
    default:
      abort();
  }
}


//------------------------------------------------------------------------------


void EventWriterRoot::Open(const std::string &path) {
  nevent = 0;
  flat_event_ = new FlatEvent();
  deep_event_ = new DeepEvent();

  output_ = new TFile(path.c_str(), "RECREATE");
  switch (compression_) {
    case CompressionAlgorithms::kCompressionNone:
      output_->SetCompressionSettings(0);
      break;
    case CompressionAlgorithms::kCompressionDeflate:
      output_->SetCompressionSettings(ROOT::CompressionSettings(ROOT::kZLIB, 1));
      break;
    case CompressionAlgorithms::kCompressionLz4:
      output_->SetCompressionSettings(ROOT::CompressionSettings(ROOT::kLZ4, 1));
      break;
    case CompressionAlgorithms::kCompressionLzma:
      output_->SetCompressionSettings(
        ROOT::CompressionSettings(ROOT::kLZMA, 1));
      break;
    default:
      abort();
  }

  if (split_mode_ == SplitMode::kSplitNone) {
    tree_ = new TTree("DecayTree", "");
    tree_->Branch("EventBranch", &flat_event_, 32000, 0);
    printf("WRITING WITH SPLIT LEVEL 0\n");
    return;
  } else if (split_mode_ == SplitMode::kSplitAuto) {
    tree_ = new TTree("DecayTree", "");
    tree_->Branch("EventBranch", &flat_event_, 32000, 99);
    printf("AUTO-SPLIT, FLAT WRITING WITH SPLIT LEVEL 99\n");
    return;
  } else if (split_mode_ == SplitMode::kSplitDeep) {
    tree_ = new TTree("DecayTree", "");
    tree_->Branch("EventBranch", &deep_event_, 32000, 99);
    printf("AUTO-SPLIT, DEEP WRITING WITH SPLIT LEVEL 99\n");
    return;
  }

  tree_ = new TTree("DecayTree", "");
  tree_->Branch("B_FlightDistance", &event_.b_flight_distance,
                "B_FlightDistance/D");
  tree_->Branch("B_VertexChi2", &event_.b_vertex_chi2, "B_VertexChi2/D");
  tree_->Branch("H1_PX", &event_.kaon_candidates[0].h_px, "H1_PX/D");
  tree_->Branch("H1_PY", &event_.kaon_candidates[0].h_py, "H1_PY/D");
  tree_->Branch("H1_PZ", &event_.kaon_candidates[0].h_pz, "H1_PZ/D");
  tree_->Branch("H1_ProbK", &event_.kaon_candidates[0].h_prob_k,
                "H1_ProbK/D");
  tree_->Branch("H1_ProbPi", &event_.kaon_candidates[0].h_prob_pi,
                "H1_ProbPi/D");
  tree_->Branch("H1_Charge", &event_.kaon_candidates[0].h_charge,
                "H1_Charge/I");
  tree_->Branch("H1_isMuon", &event_.kaon_candidates[0].h_is_muon,
                "H1_isMuon/I");
  tree_->Branch("H1_IpChi2", &event_.kaon_candidates[0].h_ip_chi2,
                "H1_IpChi2/D");
  tree_->Branch("H2_PX", &event_.kaon_candidates[1].h_px, "H2_PX/D");
  tree_->Branch("H2_PY", &event_.kaon_candidates[1].h_py, "H2_PY/D");
  tree_->Branch("H2_PZ", &event_.kaon_candidates[1].h_pz, "H2_PZ/D");
  tree_->Branch("H2_ProbK", &event_.kaon_candidates[1].h_prob_k,
                "H2_ProbK/D");
  tree_->Branch("H2_ProbPi", &event_.kaon_candidates[1].h_prob_pi,
                "H2_ProbPi/D");
  tree_->Branch("H2_Charge", &event_.kaon_candidates[1].h_charge,
                "H2_Charge/I");
  tree_->Branch("H2_isMuon", &event_.kaon_candidates[1].h_is_muon,
                "H2_isMuon/I");
  tree_->Branch("H2_IpChi2", &event_.kaon_candidates[1].h_ip_chi2,
                "H2_IpChi2/D");
  tree_->Branch("H3_PX", &event_.kaon_candidates[2].h_px, "H3_PX/D");
  tree_->Branch("H3_PY", &event_.kaon_candidates[2].h_py, "H3_PY/D");
  tree_->Branch("H3_PZ", &event_.kaon_candidates[2].h_pz, "H3_PZ/D");
  tree_->Branch("H3_ProbK", &event_.kaon_candidates[2].h_prob_k,
                "H3_ProbK/D");
  tree_->Branch("H3_ProbPi", &event_.kaon_candidates[2].h_prob_pi,
                "H3_ProbPi/D");
  tree_->Branch("H3_Charge", &event_.kaon_candidates[2].h_charge,
                "H3_Charge/I");
  tree_->Branch("H3_isMuon", &event_.kaon_candidates[2].h_is_muon,
                "H3_isMuon/I");
  tree_->Branch("H3_IpChi2", &event_.kaon_candidates[2].h_ip_chi2,
                "H3_IpChi2/D");
}


void EventWriterRoot::WriteEvent(const Event &event) {
  if (split_mode_ != SplitMode::kSplitManual) {
    if (split_mode_ == SplitMode::kSplitDeep) {
      deep_event_->FromEvent(event);
    } else {
      flat_event_->FromEvent(event);
    }
  } else {
    event_ = event;
  }

  tree_->Fill();
  nevent++;
}


void EventWriterRoot::Close() {
  output_ = tree_->GetCurrentFile();
  output_->Write();
  output_->Close();
  delete output_;
  delete flat_event_;
  delete deep_event_;
}


//------------------------------------------------------------------------------


void EventReaderRoot::Open(const std::string &path) {
  input_ = TFile::Open(path.c_str());
  tree_ = input_->Get<TTree>("DecayTree");
  flat_event_ = new FlatEvent();
  deep_event_ = new DeepEvent();
  if (g_perf_stats)
    perf_stats_ = new TTreePerfStats("ioperf", tree_);
}


int EventReaderRoot::GetIsMuon(Event *event, unsigned candidate_num) {
  assert(candidate_num <= 2);
  if (split_mode_ == SplitMode::kSplitAuto) {
    switch (candidate_num) {
      case 0: return flat_event_->H1_isMuon;
      case 1: return flat_event_->H2_isMuon;
      case 2: return flat_event_->H3_isMuon;
    }
  } else if (split_mode_ == SplitMode::kSplitDeep) {
    return deep_event_->kaon_candidates[candidate_num].h_is_muon;
  }

  return event->kaon_candidates[candidate_num].h_is_muon;
}


bool EventReaderRoot::NextEvent(Event *event) {
  if (num_events_ < 0) {
    AttachBranches2Event(event);
    num_events_ = tree_->GetEntries();
    pos_events_ = 0;
  }
  if (pos_events_ >= num_events_) {
    if (perf_stats_)
      perf_stats_->Print();
    return false;
  }

  if (split_mode_ == SplitMode::kSplitNone) {
    br_flat_event_->GetEntry(pos_events_);
    flat_event_->ToEvent(event);
    pos_events_++;
    return true;
  }
  bool is_flat_event = (split_mode_ == SplitMode::kSplitAuto);
  bool is_deep_event = (split_mode_ == SplitMode::kSplitDeep);

  if (!read_all_) {
    bool skip;
    if (is_deep_event) br_h_is_muon_->GetEntry(pos_events_);

    if (!is_deep_event) br_h1_is_muon_->GetEntry(pos_events_);
    skip = GetIsMuon(event, 0);
    if (skip) {
      if (is_flat_event) flat_event_->ToEvent(event);
      if (is_deep_event) deep_event_->ToEvent(event);
      pos_events_++; return true;
    }
    if (!is_deep_event) br_h2_is_muon_->GetEntry(pos_events_);
    skip = GetIsMuon(event, 1);
    if (skip) {
      if (is_flat_event) flat_event_->ToEvent(event);
      if (is_deep_event) deep_event_->ToEvent(event);
      pos_events_++; return true;
    }
    if (!is_deep_event) br_h3_is_muon_->GetEntry(pos_events_);
    skip = GetIsMuon(event, 2);
    if (skip) {
      if (is_flat_event) flat_event_->ToEvent(event);
      if (is_deep_event) deep_event_->ToEvent(event);
      pos_events_++; return true;
    }
  } else {
    br_h1_is_muon_->GetEntry(pos_events_);
    br_h2_is_muon_->GetEntry(pos_events_);
    br_h3_is_muon_->GetEntry(pos_events_);
    br_b_flight_distance_->GetEntry(pos_events_);
    br_b_vertex_chi2_->GetEntry(pos_events_);
    br_h1_ip_chi2_->GetEntry(pos_events_);
    br_h2_ip_chi2_->GetEntry(pos_events_);
    br_h3_ip_chi2_->GetEntry(pos_events_);
  }

  if (is_deep_event) {
    br_h_px_->GetEntry(pos_events_);
    br_h_py_->GetEntry(pos_events_);
    br_h_pz_->GetEntry(pos_events_);
    br_h_prob_k_->GetEntry(pos_events_);
    br_h_prob_pi_->GetEntry(pos_events_);
    br_h_charge_->GetEntry(pos_events_);
  } else {
    br_h1_px_->GetEntry(pos_events_);
    br_h1_py_->GetEntry(pos_events_);
    br_h1_pz_->GetEntry(pos_events_);
    br_h1_prob_k_->GetEntry(pos_events_);
    br_h1_prob_pi_->GetEntry(pos_events_);
    br_h1_charge_->GetEntry(pos_events_);
    br_h2_px_->GetEntry(pos_events_);
    br_h2_py_->GetEntry(pos_events_);
    br_h2_pz_->GetEntry(pos_events_);
    br_h2_prob_k_->GetEntry(pos_events_);
    br_h2_prob_pi_->GetEntry(pos_events_);
    br_h2_charge_->GetEntry(pos_events_);
    br_h3_px_->GetEntry(pos_events_);
    br_h3_py_->GetEntry(pos_events_);
    br_h3_pz_->GetEntry(pos_events_);
    br_h3_prob_k_->GetEntry(pos_events_);
    br_h3_prob_pi_->GetEntry(pos_events_);
    br_h3_charge_->GetEntry(pos_events_);
  }

  if (is_flat_event) flat_event_->ToEvent(event);
  if (is_deep_event) deep_event_->ToEvent(event);

  pos_events_++;
  return true;
}


void EventReaderRoot::AttachBranches2Event(Event *event) {
  switch (split_mode_) {
    case SplitMode::kSplitManual:
      AttachBranches2EventManual(event);
      break;
    case SplitMode::kSplitNone:
      AttachBranches2EventNone();
      break;
    case SplitMode::kSplitAuto:
      AttachBranches2EventAuto();
      break;
    case SplitMode::kSplitDeep:
      AttachBranches2EventDeep();
      break;
    default:
      abort();
  }
}

void EventReaderRoot::AttachBranches2EventNone() {
  tree_->SetBranchAddress("EventBranch", &flat_event_, &br_flat_event_);
}

void EventReaderRoot::AttachBranches2EventAuto() {
  tree_->SetBranchAddress("EventBranch", &flat_event_, &br_flat_event_);

  br_h1_px_ = tree_->GetBranch("H1_PX");
  br_h1_py_ = tree_->GetBranch("H1_PY");
  br_h1_pz_ = tree_->GetBranch("H1_PZ");
  br_h1_prob_k_ = tree_->GetBranch("H1_ProbK");
  br_h1_prob_pi_ = tree_->GetBranch("H1_ProbPi");
  br_h1_charge_ = tree_->GetBranch("H1_Charge");
  br_h1_is_muon_ = tree_->GetBranch("H1_isMuon");
  br_h2_px_ = tree_->GetBranch("H2_PX");
  br_h2_py_ = tree_->GetBranch("H2_PY");
  br_h2_pz_ = tree_->GetBranch("H2_PZ");
  br_h2_prob_k_ = tree_->GetBranch("H2_ProbK");
  br_h2_prob_pi_ = tree_->GetBranch("H2_ProbPi");
  br_h2_charge_ = tree_->GetBranch("H2_Charge");
  br_h2_is_muon_ = tree_->GetBranch("H2_isMuon");
  br_h3_px_ = tree_->GetBranch("H3_PX");
  br_h3_py_ = tree_->GetBranch("H3_PY");
  br_h3_pz_ = tree_->GetBranch("H3_PZ");
  br_h3_prob_k_ = tree_->GetBranch("H3_ProbK");
  br_h3_prob_pi_ = tree_->GetBranch("H3_ProbPi");
  br_h3_charge_ = tree_->GetBranch("H3_Charge");
  br_h3_is_muon_ = tree_->GetBranch("H3_isMuon");
}

void EventReaderRoot::AttachBranches2EventDeep() {
  tree_->SetBranchAddress("EventBranch", &deep_event_, &br_deep_event_);

  br_h_px_ = tree_->GetBranch("kaon_candidates.h_px");
  br_h_py_ = tree_->GetBranch("kaon_candidates.h_py");
  br_h_pz_ = tree_->GetBranch("kaon_candidates.h_pz");
  br_h_prob_k_ = tree_->GetBranch("kaon_candidates.h_prob_k");
  br_h_prob_pi_ = tree_->GetBranch("kaon_candidates.h_prob_pi");
  br_h_charge_ = tree_->GetBranch("kaon_candidates.h_charge");
  br_h_is_muon_ = tree_->GetBranch("kaon_candidates.h_is_muon");
}

void EventReaderRoot::AttachBranches2EventManual(Event *event) {
  tree_->SetBranchAddress("H1_PX", &event->kaon_candidates[0].h_px,
                                &br_h1_px_);
  tree_->SetBranchAddress("H1_PY", &event->kaon_candidates[0].h_py,
                                &br_h1_py_);
  tree_->SetBranchAddress("H1_PZ", &event->kaon_candidates[0].h_pz,
                                &br_h1_pz_);
  tree_->SetBranchAddress("H1_ProbK",
                                &event->kaon_candidates[0].h_prob_k,
                                &br_h1_prob_k_);
  tree_->SetBranchAddress("H1_ProbPi",
                                &event->kaon_candidates[0].h_prob_pi,
                                &br_h1_prob_pi_);
  tree_->SetBranchAddress("H1_Charge",
                                &event->kaon_candidates[0].h_charge,
                                &br_h1_charge_);
  tree_->SetBranchAddress("H1_isMuon",
                                &event->kaon_candidates[0].h_is_muon,
                                &br_h1_is_muon_);

  tree_->SetBranchAddress("H2_PX", &event->kaon_candidates[1].h_px,
                                &br_h2_px_);
  tree_->SetBranchAddress("H2_PY", &event->kaon_candidates[1].h_py,
                                &br_h2_py_);
  tree_->SetBranchAddress("H2_PZ", &event->kaon_candidates[1].h_pz,
                                &br_h2_pz_);
  tree_->SetBranchAddress("H2_ProbK",
                                &event->kaon_candidates[1].h_prob_k,
                                &br_h2_prob_k_);
  tree_->SetBranchAddress("H2_ProbPi",
                                &event->kaon_candidates[1].h_prob_pi,
                                &br_h2_prob_pi_);
  tree_->SetBranchAddress("H2_Charge",
                                &event->kaon_candidates[1].h_charge,
                                &br_h2_charge_);
  tree_->SetBranchAddress("H2_isMuon",
                                &event->kaon_candidates[1].h_is_muon,
                                &br_h2_is_muon_);

  tree_->SetBranchAddress("H3_PX", &event->kaon_candidates[2].h_px,
                                &br_h3_px_);
  tree_->SetBranchAddress("H3_PY", &event->kaon_candidates[2].h_py,
                                &br_h3_py_);
  tree_->SetBranchAddress("H3_PZ", &event->kaon_candidates[2].h_pz,
                                &br_h3_pz_);
  tree_->SetBranchAddress("H3_ProbK",
                                &event->kaon_candidates[2].h_prob_k,
                                &br_h3_prob_k_);
  tree_->SetBranchAddress("H3_ProbPi",
                                &event->kaon_candidates[2].h_prob_pi,
                                &br_h3_prob_pi_);
  tree_->SetBranchAddress("H3_Charge",
                                &event->kaon_candidates[2].h_charge,
                                &br_h3_charge_);
  tree_->SetBranchAddress("H3_isMuon",
                                &event->kaon_candidates[2].h_is_muon,
                                &br_h3_is_muon_);
}


/**
 * Not used for the analysis but required for converting into another file
 * format.
 */
void EventReaderRoot::AttachUnusedBranches2Event(Event *event) {
  tree_->SetBranchAddress("B_FlightDistance", &event->b_flight_distance,
                                &br_b_flight_distance_);
  tree_->SetBranchAddress("B_VertexChi2", &event->b_vertex_chi2,
                                &br_b_vertex_chi2_);
  tree_->SetBranchAddress("H1_IpChi2",
                                &event->kaon_candidates[0].h_ip_chi2,
                                &br_h1_ip_chi2_);
  tree_->SetBranchAddress("H2_IpChi2",
                                &event->kaon_candidates[1].h_ip_chi2,
                                &br_h2_ip_chi2_);
  tree_->SetBranchAddress("H3_IpChi2",
                                &event->kaon_candidates[2].h_ip_chi2,
                                &br_h3_ip_chi2_);
  read_all_ = true;
}


void EventReaderRoot::PrepareForConversion(Event *event) {
  AttachUnusedBranches2Event(event);
}


//------------------------------------------------------------------------------


void EventReaderNtuple::Open(const std::string &path) {
  auto model = RNTupleModel::Create();

  h1_px = model->MakeField<double>("H1_PX");
  h1_py = model->MakeField<double>("H1_PY");
  h1_pz = model->MakeField<double>("H1_PZ");
  h1_prob_k = model->MakeField<double>("H1_ProbK");
  h1_prob_pi = model->MakeField<double>("H1_ProbPi");
  h1_charge = model->MakeField<int>("H1_Charge");
  h1_is_muon = model->MakeField<int>("H1_isMuon");
  h2_px = model->MakeField<double>("H2_PX");
  h2_py = model->MakeField<double>("H2_PY");
  h2_pz = model->MakeField<double>("H2_PZ");
  h2_prob_k = model->MakeField<double>("H2_ProbK");
  h2_prob_pi = model->MakeField<double>("H2_ProbPi");
  h2_charge = model->MakeField<int>("H2_Charge");
  h2_is_muon = model->MakeField<int>("H2_isMuon");
  h3_px = model->MakeField<double>("H3_PX");
  h3_py = model->MakeField<double>("H3_PY");
  h3_pz = model->MakeField<double>("H3_PZ");
  h3_prob_k = model->MakeField<double>("H3_ProbK");
  h3_prob_pi = model->MakeField<double>("H3_ProbPi");
  h3_charge = model->MakeField<int>("H3_Charge");
  h3_is_muon = model->MakeField<int>("H3_isMuon");

  using RNTupleReadOptions = ROOT::Experimental::RNTupleReadOptions;
  RNTupleReadOptions options;
  options.SetClusterCache(RNTupleReadOptions::EClusterCache::kOn);
  fReader = RNTupleReader::Open(std::move(model), "DecayTree", path, options);
  fReader->EnableMetrics();
  fNumEvents = fReader->GetNEntries();
}


bool EventReaderNtuple::NextEvent(Event *event) {
  if (fPosEvents >= fNumEvents) {
    fReader->PrintInfo(ROOT::Experimental::ENTupleInfo::kMetrics);
    return false;
  }

  fReader->LoadEntry(fPosEvents++);
  event->kaon_candidates[0].h_is_muon = *h1_is_muon;
  event->kaon_candidates[1].h_is_muon = *h2_is_muon;
  event->kaon_candidates[2].h_is_muon = *h3_is_muon;

  event->kaon_candidates[0].h_px = *h1_px;
  event->kaon_candidates[0].h_py = *h1_py;
  event->kaon_candidates[0].h_pz = *h1_pz;
  event->kaon_candidates[0].h_prob_k = *h1_prob_k;
  event->kaon_candidates[0].h_prob_pi = *h1_prob_pi;
  event->kaon_candidates[0].h_charge = *h1_charge;

  event->kaon_candidates[1].h_px = *h2_px;
  event->kaon_candidates[1].h_py = *h2_py;
  event->kaon_candidates[1].h_pz = *h2_pz;
  event->kaon_candidates[1].h_prob_k = *h2_prob_k;
  event->kaon_candidates[1].h_prob_pi = *h2_prob_pi;
  event->kaon_candidates[1].h_charge = *h2_charge;

  event->kaon_candidates[2].h_px = *h3_px;
  event->kaon_candidates[2].h_py = *h3_py;
  event->kaon_candidates[2].h_pz = *h3_pz;
  event->kaon_candidates[2].h_prob_k = *h3_prob_k;
  event->kaon_candidates[2].h_prob_pi = *h3_prob_pi;
  event->kaon_candidates[2].h_charge = *h3_charge;

  return true;
}


//------------------------------------------------------------------------------


unsigned g_skipped = 0;
static double ProcessEvent(const Event &event) {
  unsigned i = 0;
  for (const auto &k : event.kaon_candidates) {
    ++i;
    //printf("%d is muon %d\n", i, k.h_is_muon);
    if (k.h_is_muon) {
      g_skipped++;
      return 0.0;
    }
  }
  //abort();

  double result = 0.0;
  result +=
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

  /*for (const auto &k : event.kaon_candidates) {
    result += k.h_px + k.h_py + k.h_pz + k.h_prob_k + k.h_prob_pi +
              double(k.h_charge);
  }*/
  return result;
}


int AnalyzeDataframe(
  ROOT::RDataFrame &frame,
  int nslots)
{
  std::vector<double> sums(nslots, 0.0);

  auto fn_muon_cut = [](int is_muon) { return !is_muon; };
  auto fn_sum_slot = [&sums](
    unsigned int slot,
    double h1_px,
    double h1_py,
    double h1_pz,
    double h1_prob_k,
    double h1_prob_pi,
    int h1_charge,
    double h2_px,
    double h2_py,
    double h2_pz,
    double h2_prob_k,
    double h2_prob_pi,
    int h2_charge,
    double h3_px,
    double h3_py,
    double h3_pz,
    double h3_prob_k,
    double h3_prob_pi,
    int h3_charge)
  {
    sums[slot] +=
      h1_px +
      h1_py +
      h1_pz +
      h1_prob_k +
      h1_prob_pi +
      double(h1_charge) +
      h2_px +
      h2_py +
      h2_pz +
      h2_prob_k +
      h2_prob_pi +
      double(h2_charge) +
      h3_px +
      h3_py +
      h3_pz +
      h3_prob_k +
      h3_prob_pi +
      double(h3_charge);
  };

  frame.Filter(fn_muon_cut, {"H1_isMuon"})
         .Filter(fn_muon_cut, {"H2_isMuon"})
         .Filter(fn_muon_cut, {"H3_isMuon"})
         .ForeachSlot(fn_sum_slot, {
           "H1_PX",
           "H1_PY",
           "H1_PZ",
           "H1_ProbK",
           "H1_ProbPi",
           "H1_Charge",
           "H2_PX",
           "H2_PY",
           "H2_PZ",
           "H2_ProbK",
           "H2_ProbPi",
           "H2_Charge",
           "H3_PX",
           "H3_PY",
           "H3_PZ",
           "H3_ProbK",
           "H3_ProbPi",
           "H3_Charge"});

  double total_sum = 0.0;
  for (unsigned i = 0; i < sums.size(); ++i)
    total_sum += sums[i];
  //unsigned nevent = root_chain.GetEntries();
  printf("finished, result: %lf, skipped ?\n", total_sum);
  return 0;
}


int AnalyzeRootOptimized(std::string &path) {
  auto input = TFile::Open(path.c_str());
  auto tree = input->Get<TTree>("DecayTree");
  TTreeReader reader(tree);

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

  return 0;
}


static int AnalyzeNtupleOptimized(const std::string &name)
{
  using RNTupleReadOptions = ROOT::Experimental::RNTupleReadOptions;
  RNTupleReadOptions options;
  options.SetClusterCache(RNTupleReadOptions::EClusterCache::kOn);
  auto ntuple = RNTupleReader::Open("DecayTree", name, options);
  if (g_perf_stats)
    ntuple->EnableMetrics();

  auto viewH1IsMuon = ntuple->GetView<int>("H1_isMuon");
  auto viewH2IsMuon = ntuple->GetView<int>("H2_isMuon");
  auto viewH3IsMuon = ntuple->GetView<int>("H3_isMuon");

  auto viewH1PX = ntuple->GetView<double>("H1_PX");
  auto viewH1PY = ntuple->GetView<double>("H1_PY");
  auto viewH1PZ = ntuple->GetView<double>("H1_PZ");
  auto viewH1ProbK = ntuple->GetView<double>("H1_ProbK");
  auto viewH1ProbPi = ntuple->GetView<double>("H1_ProbPi");
  auto viewH1Charge = ntuple->GetView<int>("H1_Charge");

  auto viewH2PX = ntuple->GetView<double>("H2_PX");
  auto viewH2PY = ntuple->GetView<double>("H2_PY");
  auto viewH2PZ = ntuple->GetView<double>("H2_PZ");
  auto viewH2ProbK = ntuple->GetView<double>("H2_ProbK");
  auto viewH2ProbPi = ntuple->GetView<double>("H2_ProbPi");
  auto viewH2Charge = ntuple->GetView<int>("H2_Charge");

  auto viewH3PX = ntuple->GetView<double>("H3_PX");
  auto viewH3PY = ntuple->GetView<double>("H3_PY");
  auto viewH3PZ = ntuple->GetView<double>("H3_PZ");
  auto viewH3ProbK = ntuple->GetView<double>("H3_ProbK");
  auto viewH3ProbPi = ntuple->GetView<double>("H3_ProbPi");
  auto viewH3Charge = ntuple->GetView<int>("H3_Charge");

  double dummy = 0;
  int nskipped = 0;
  int nevents = 0;
  for (auto i : ntuple->GetViewRange()) {
    nevents++;
    if ((nevents % 100000) == 0) {
      printf("processed %u k events\n", nevents / 1000);
      //printf("dummy is %lf\n", dummy); abort();
    }

    if (viewH1IsMuon(i) || viewH2IsMuon(i) || viewH3IsMuon(i)) {
      nskipped++;
      continue;
    }

    dummy +=
      viewH1PX(i) +
      viewH1PY(i) +
      viewH1PZ(i) +
      viewH1ProbK(i) +
      viewH1ProbPi(i) +
      double(viewH1Charge(i)) +
      //double(viewH1IsMuon(i)) +
      viewH2PX(i) +
      viewH2PY(i) +
      viewH2PZ(i) +
      viewH2ProbK(i) +
      viewH2ProbPi(i) +
      double(viewH2Charge(i)) +
      //double(viewH2IsMuon(i)) +
      viewH3PX(i) +
      viewH3PY(i) +
      viewH3PZ(i) +
      viewH3ProbK(i) +
      viewH3ProbPi(i) +
      double(viewH3Charge(i))
      // + double(viewH3IsMuon(i))
      ;
  }

  printf("Optimized RNTuple run: %u events read, %u events skipped "
         "(dummy: %lf)\n", nevents, nskipped, dummy);
  if (g_perf_stats)
    ntuple->PrintInfo(ROOT::Experimental::ENTupleInfo::kMetrics);

  return 0;
}


static void Usage(const char *progname) {
  printf("%s [-i input.root] [-p(erformance stats)]"
         "[-r | -o output format [-d outdir] [-b bloat factor]]\n"
         "[-V (ntuple optimized)] [-s(short file)] [-f|-g (data frame / mt)]\n",
         progname);
}


int main(int argc, char **argv) {
  std::string input_path;
  std::string input_suffix;
  std::string output_suffix;
  std::string outdir;
  bool root_optimized = false;
  bool root_dataframe = false;
  bool root_dataframe_mt = false;
  bool root_dataframe_ht = true;
  bool short_file = false;
  bool ntuple_optimized = false;
  unsigned bloat_factor = 1;
  int c;
  while ((c = getopt(argc, argv, "hvi:o:rb:d:psfgGV")) != -1) {
    switch (c) {
      case 'h':
      case 'v':
        Usage(argv[0]);
        return 0;
      case 'i':
        input_path = optarg;
        break;
      case 'o':
        output_suffix = optarg;
        break;
      case 'd':
        outdir = optarg;
        break;
      case 'r':
        root_optimized = true;
        break;
      case 'b':
        bloat_factor = String2Uint64(optarg);
        break;
      case 'p':
        g_perf_stats = true;
        break;
      case 's':
        short_file = true;
        break;
      case 'f':
        root_dataframe = true;
        break;
      case 'g':
        root_dataframe = true;
        root_dataframe_mt = true;
        break;
      case 'G':
        root_dataframe = true;
        root_dataframe_mt = true;
        root_dataframe_ht = false;
        break;
      case 'V':
        ntuple_optimized = true;
        break;
      default:
        fprintf(stderr, "Unknown option: -%c\n", c);
        Usage(argv[0]);
        return 1;
    }
  }

  assert(!input_path.empty());
  assert(bloat_factor > 0);
  input_suffix = GetSuffix(input_path);
  FileFormats input_format = GetFileFormat(input_suffix);
  if (root_optimized) {
    if (input_format == FileFormats::kRoot ||
        input_format == FileFormats::kRootInflated ||
        input_format == FileFormats::kRootDeflated ||
        input_format == FileFormats::kRootLz4 ||
        input_format == FileFormats::kRootAutosplitInflated ||
        input_format == FileFormats::kRootAutosplitDeflated)
    {
      return AnalyzeRootOptimized(input_path);
    } else {
      printf("ignoring ROOT optimization flag\n");
    }
  }

  if (root_dataframe) {
    if (input_format == FileFormats::kRoot ||
        input_format == FileFormats::kRootInflated ||
        input_format == FileFormats::kRootDeflated ||
        input_format == FileFormats::kRootLz4 ||
        input_format == FileFormats::kRootAutosplitInflated ||
        input_format == FileFormats::kRootAutosplitDeflated ||
        input_format == FileFormats::kNtuple ||
        input_format == FileFormats::kNtupleDeflated ||
        input_format == FileFormats::kNtupleInflated)
    {
      unsigned nslots = 1;
      if (root_dataframe_mt) {
        if (root_dataframe_ht)
          ROOT::EnableImplicitMT();
        else
          ROOT::EnableImplicitMT(6);
        nslots = ROOT::GetImplicitMTPoolSize();
        printf("Using %u slots\n", nslots);
      }

      if (input_format == FileFormats::kNtuple ||
          input_format == FileFormats::kNtupleDeflated ||
          input_format == FileFormats::kNtupleInflated)
      {
        auto frame = ROOT::Experimental::MakeNTupleDataFrame("DecayTree", input_path);
        return AnalyzeDataframe(frame, nslots);
      } else {
        auto input = TFile::Open(input_path.c_str());
        auto tree = input->Get<TTree>("DecayTree");
        ROOT::RDataFrame frame(*tree);
        return AnalyzeDataframe(frame, nslots);
      }
    } else {
      printf("ignoring ROOT dataframe flag\n");
    }
  }

  if (ntuple_optimized) {
    if (input_format == FileFormats::kNtuple ||
        input_format == FileFormats::kNtupleDeflated ||
        input_format == FileFormats::kNtupleInflated)
    {
      return AnalyzeNtupleOptimized(input_path);
    } else {
      printf("ignoring RNTuple optimized flag\n");
    }
  }

  std::unique_ptr<EventReader> event_reader {EventReader::Create(input_format)};
  event_reader->set_short_read(short_file);
  event_reader->Open(input_path);

  Event event;

  std::unique_ptr<EventWriter> event_writer{nullptr};
  if (!output_suffix.empty()) {
    assert((input_format == FileFormats::kRoot) ||
           (input_format == FileFormats::kRootInflated) ||
           (input_format == FileFormats::kRootDeflated));
    event_reader->PrepareForConversion(&event);

    FileFormats output_format = GetFileFormat(output_suffix);
    assert(output_format != FileFormats::kRoot);
    event_writer = EventWriter::Create(output_format);
    event_writer->set_short_write(short_file);
    std::string bloat_extension;
    if (bloat_factor > 1)
      bloat_extension = "times" + StringifyUint(bloat_factor) + ".";
    std::string dest_dir = GetParentPath(input_path);
    if (!outdir.empty())
      dest_dir = outdir;
    event_writer->Open(dest_dir + "/" + StripSuffix(GetFileName(input_path))
                       + "." + bloat_extension + output_suffix);
  }

  unsigned i_events = 0;
  double dummy = 0.0;
  while (event_reader->NextEvent(&event)) {
    if (event_writer) {
      for (unsigned i = 0; i < bloat_factor; ++i)
        event_writer->WriteEvent(event);
    } else {
      dummy += ProcessEvent(event);
    }
    if ((++i_events % 100000) == 0) {
      //printf("processed %u events\n", i_events);
      printf("processed %u k events\n", i_events / 1000);
      //printf(" ... H_PX[1] = %lf\n", event.kaon_candidates[0].h_px);
      //printf(" ... dummy is %lf\n", dummy); abort();
    }
    if (short_file && (i_events == 500000))
      break;
  }

  printf("finished (%u events), result: %lf, skipped %u\n",
         i_events, dummy, g_skipped);
  if (event_writer) event_writer->Close();

  return 0;
}
