#ifndef LHCB_TTREE_H_
#define LHCB_TTREE_H_

#include "lhcb_event.h"

#include <string>

class TChain;
class TBranch;

class EventReaderRoot {
 public:
  EventReaderRoot() = default;
  void Open(const std::string &path);
  bool NextEvent();

  LhcbEvent fEvent;
  int fNEvents = -1;
  int fPos = -1;

 private:
  TChain *fChain = nullptr;

  TBranch *br_b_flight_distance_ = nullptr;
  TBranch *br_b_vertex_chi2_     = nullptr;
  TBranch *br_h1_px_             = nullptr;
  TBranch *br_h1_py_             = nullptr;
  TBranch *br_h1_pz_             = nullptr;
  TBranch *br_h1_prob_k_         = nullptr;
  TBranch *br_h1_prob_pi_        = nullptr;
  TBranch *br_h1_charge_         = nullptr;
  TBranch *br_h1_is_muon_        = nullptr;
  TBranch *br_h1_ip_chi2_        = nullptr;
  TBranch *br_h2_px_             = nullptr;
  TBranch *br_h2_py_             = nullptr;
  TBranch *br_h2_pz_             = nullptr;
  TBranch *br_h2_prob_k_         = nullptr;
  TBranch *br_h2_prob_pi_        = nullptr;
  TBranch *br_h2_charge_         = nullptr;
  TBranch *br_h2_is_muon_        = nullptr;
  TBranch *br_h2_ip_chi2_        = nullptr;
  TBranch *br_h3_px_             = nullptr;
  TBranch *br_h3_py_             = nullptr;
  TBranch *br_h3_pz_             = nullptr;
  TBranch *br_h3_prob_k_         = nullptr;
  TBranch *br_h3_prob_pi_        = nullptr;
  TBranch *br_h3_charge_         = nullptr;
  TBranch *br_h3_is_muon_        = nullptr;
  TBranch *br_h3_ip_chi2_        = nullptr;
};

#endif // LHCB_TTREE_H_
