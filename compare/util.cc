#include "util.h"

#include <vector>

#include <TChain.h>

void EventReaderRoot::Open(const std::string &path) {
  fChain = new TChain("DecayTree");
  fChain->Add(path.c_str());

  fChain->SetBranchAddress("B_FlightDistance", &fEvent.B_FlightDistance, &br_b_flight_distance_);
  fChain->SetBranchAddress("B_VertexChi2",     &fEvent.B_VertexChi2,     &br_b_vertex_chi2_);
  fChain->SetBranchAddress("H1_PX",            &fEvent.H1_PX,            &br_h1_px_);
  fChain->SetBranchAddress("H1_PY",            &fEvent.H1_PY,            &br_h1_py_);
  fChain->SetBranchAddress("H1_PZ",            &fEvent.H1_PZ,            &br_h1_pz_);
  fChain->SetBranchAddress("H1_ProbK",         &fEvent.H1_ProbK,         &br_h1_prob_k_);
  fChain->SetBranchAddress("H1_ProbPi",        &fEvent.H1_ProbPi,        &br_h1_prob_pi_);
  fChain->SetBranchAddress("H1_Charge",        &fEvent.H1_Charge,        &br_h1_charge_);
  fChain->SetBranchAddress("H1_isMuon",        &fEvent.H1_isMuon,        &br_h1_is_muon_);
  fChain->SetBranchAddress("H1_IpChi2",        &fEvent.H1_IpChi2,        &br_h1_ip_chi2_);
  fChain->SetBranchAddress("H2_PX",            &fEvent.H2_PX,            &br_h2_px_);
  fChain->SetBranchAddress("H2_PY",            &fEvent.H2_PY,            &br_h2_py_);
  fChain->SetBranchAddress("H2_PZ",            &fEvent.H2_PZ,            &br_h2_pz_);
  fChain->SetBranchAddress("H2_ProbK",         &fEvent.H2_ProbK,         &br_h2_prob_k_);
  fChain->SetBranchAddress("H2_ProbPi",        &fEvent.H2_ProbPi,        &br_h2_prob_pi_);
  fChain->SetBranchAddress("H2_Charge",        &fEvent.H2_Charge,        &br_h2_charge_);
  fChain->SetBranchAddress("H2_isMuon",        &fEvent.H2_isMuon,        &br_h2_is_muon_);
  fChain->SetBranchAddress("H2_IpChi2",        &fEvent.H2_IpChi2,        &br_h2_ip_chi2_);
  fChain->SetBranchAddress("H3_PX",            &fEvent.H3_PX,            &br_h3_px_);
  fChain->SetBranchAddress("H3_PY",            &fEvent.H3_PY,            &br_h3_py_);
  fChain->SetBranchAddress("H3_PZ",            &fEvent.H3_PZ,            &br_h3_pz_);
  fChain->SetBranchAddress("H3_ProbK",         &fEvent.H3_ProbK,         &br_h3_prob_k_);
  fChain->SetBranchAddress("H3_ProbPi",        &fEvent.H3_ProbPi,        &br_h3_prob_pi_);
  fChain->SetBranchAddress("H3_Charge",        &fEvent.H3_Charge,        &br_h3_charge_);
  fChain->SetBranchAddress("H3_isMuon",        &fEvent.H3_isMuon,        &br_h3_is_muon_);
  fChain->SetBranchAddress("H3_IpChi2",        &fEvent.H3_IpChi2,        &br_h3_ip_chi2_);

  fNEvents = fChain->GetEntries();
  fPos = 0;
}

bool EventReaderRoot::NextEvent() {
  if (fPos >= fNEvents)
    return false;

  br_b_flight_distance_->GetEntry(fPos);
  br_b_vertex_chi2_->GetEntry(fPos);
  br_h1_px_->GetEntry(fPos);
  br_h1_py_->GetEntry(fPos);
  br_h1_pz_->GetEntry(fPos);
  br_h1_prob_k_->GetEntry(fPos);
  br_h1_prob_pi_->GetEntry(fPos);
  br_h1_charge_->GetEntry(fPos);
  br_h1_is_muon_->GetEntry(fPos);
  br_h1_ip_chi2_->GetEntry(fPos);
  br_h2_px_->GetEntry(fPos);
  br_h2_py_->GetEntry(fPos);
  br_h2_pz_->GetEntry(fPos);
  br_h2_prob_k_->GetEntry(fPos);
  br_h2_prob_pi_->GetEntry(fPos);
  br_h2_charge_->GetEntry(fPos);
  br_h2_is_muon_->GetEntry(fPos);
  br_h2_ip_chi2_->GetEntry(fPos);
  br_h3_px_->GetEntry(fPos);
  br_h3_py_->GetEntry(fPos);
  br_h3_pz_->GetEntry(fPos);
  br_h3_prob_k_->GetEntry(fPos);
  br_h3_prob_pi_->GetEntry(fPos);
  br_h3_charge_->GetEntry(fPos);
  br_h3_is_muon_->GetEntry(fPos);
  br_h3_ip_chi2_->GetEntry(fPos);

  fPos++;
  return true;
}
