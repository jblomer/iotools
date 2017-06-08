#ifndef EVENT_H_
#define EVENT_H_

#include <array>

struct KaonCandidate {
  double h_px, h_py, h_pz;
  double h_prob_k, h_prob_pi;
  int h_charge;
  int h_is_muon;
  double h_ip_chi2;  // unused
};

struct Event {
  double b_flight_distance;  // unused
  double b_vertex_chi2;  // unused
  std::array<KaonCandidate, 3> kaon_candidates;
};


class FlatEvent {
 public:

  void FromEvent(const Event &e) {
    b_flight_distance = e.b_flight_distance;
    b_vertex_chi2 = e.b_vertex_chi2;
    h1_px = e.kaon_candidates[0].h_px;
    h1_py = e.kaon_candidates[0].h_py;
    h1_pz = e.kaon_candidates[0].h_pz;
    h1_prob_k = e.kaon_candidates[0].h_prob_k;
    h1_prob_pi = e.kaon_candidates[0].h_prob_pi;
    h1_charge = e.kaon_candidates[0].h_charge;
    h1_is_muon = e.kaon_candidates[0].h_is_muon;
    h1_ip_chi2 = e.kaon_candidates[0].h_ip_chi2;
    h2_px = e.kaon_candidates[1].h_px;
    h2_py = e.kaon_candidates[1].h_py;
    h2_pz = e.kaon_candidates[1].h_pz;
    h2_prob_k = e.kaon_candidates[1].h_prob_k;
    h2_prob_pi = e.kaon_candidates[1].h_prob_pi;
    h2_charge = e.kaon_candidates[1].h_charge;
    h2_is_muon = e.kaon_candidates[1].h_is_muon;
    h2_ip_chi2 = e.kaon_candidates[1].h_ip_chi2;
    h3_px = e.kaon_candidates[2].h_px;
    h3_py = e.kaon_candidates[2].h_py;
    h3_pz = e.kaon_candidates[2].h_pz;
    h3_prob_k = e.kaon_candidates[2].h_prob_k;
    h3_prob_pi = e.kaon_candidates[2].h_prob_pi;
    h3_charge = e.kaon_candidates[2].h_charge;
    h3_is_muon = e.kaon_candidates[2].h_is_muon;
    h3_ip_chi2 = e.kaon_candidates[2].h_ip_chi2;
  }

  double b_flight_distance;
  double b_vertex_chi2;
  double h1_px;
  double h1_py;
  double h1_pz;
  double h1_prob_k;
  double h1_prob_pi;
  int h1_charge;
  int h1_is_muon;
  double h1_ip_chi2;
  double h2_px;
  double h2_py;
  double h2_pz;
  double h2_prob_k;
  double h2_prob_pi;
  int h2_charge;
  int h2_is_muon;
  double h2_ip_chi2;
  double h3_px;
  double h3_py;
  double h3_pz;
  double h3_prob_k;
  double h3_prob_pi;
  int h3_charge;
  int h3_is_muon;
  double h3_ip_chi2;
};

#endif  // EVENT_H_
