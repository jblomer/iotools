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
    B_FlightDistance = e.b_flight_distance;
    B_VertexChi2 = e.b_vertex_chi2;
    H1_PX = e.kaon_candidates[0].h_px;
    H1_PY = e.kaon_candidates[0].h_py;
    H1_PZ = e.kaon_candidates[0].h_pz;
    H1_ProbK = e.kaon_candidates[0].h_prob_k;
    H1_ProbPi = e.kaon_candidates[0].h_prob_pi;
    H1_Charge = e.kaon_candidates[0].h_charge;
    H1_isMuon = e.kaon_candidates[0].h_is_muon;
    H1_IpChi2 = e.kaon_candidates[0].h_ip_chi2;
    H2_PX = e.kaon_candidates[1].h_px;
    H2_PY = e.kaon_candidates[1].h_py;
    H2_PZ = e.kaon_candidates[1].h_pz;
    H2_ProbK = e.kaon_candidates[1].h_prob_k;
    H2_ProbPi = e.kaon_candidates[1].h_prob_pi;
    H2_Charge = e.kaon_candidates[1].h_charge;
    H2_isMuon = e.kaon_candidates[1].h_is_muon;
    H2_IpChi2 = e.kaon_candidates[1].h_ip_chi2;
    H3_PX = e.kaon_candidates[2].h_px;
    H3_PY = e.kaon_candidates[2].h_py;
    H3_PZ = e.kaon_candidates[2].h_pz;
    H3_ProbK = e.kaon_candidates[2].h_prob_k;
    H3_ProbPi = e.kaon_candidates[2].h_prob_pi;
    H3_Charge = e.kaon_candidates[2].h_charge;
    H3_isMuon = e.kaon_candidates[2].h_is_muon;
    H3_IpChi2 = e.kaon_candidates[2].h_ip_chi2;
  }

  void ToEvent(Event *e) {
    e->b_flight_distance = B_FlightDistance;
    e->b_vertex_chi2 = B_VertexChi2;
    e->kaon_candidates[0].h_px = H1_PX;
    e->kaon_candidates[0].h_py = H1_PY;
    e->kaon_candidates[0].h_pz = H1_PZ;
    e->kaon_candidates[0].h_prob_k = H1_ProbK;
    e->kaon_candidates[0].h_prob_pi = H1_ProbPi;
    e->kaon_candidates[0].h_charge = H1_Charge;
    e->kaon_candidates[0].h_is_muon = H1_isMuon;
    e->kaon_candidates[0].h_ip_chi2 = H1_IpChi2;
    e->kaon_candidates[1].h_px = H2_PX;
    e->kaon_candidates[1].h_py = H2_PY;
    e->kaon_candidates[1].h_pz = H2_PZ;
    e->kaon_candidates[1].h_prob_k = H2_ProbK;
    e->kaon_candidates[1].h_prob_pi = H2_ProbPi;
    e->kaon_candidates[1].h_charge = H2_Charge;
    e->kaon_candidates[1].h_is_muon = H2_isMuon;
    e->kaon_candidates[1].h_ip_chi2 = H2_IpChi2;
    e->kaon_candidates[2].h_px = H3_PX;
    e->kaon_candidates[2].h_py = H3_PY;
    e->kaon_candidates[2].h_pz = H3_PZ;
    e->kaon_candidates[2].h_prob_k = H3_ProbK;
    e->kaon_candidates[2].h_prob_pi = H3_ProbPi;
    e->kaon_candidates[2].h_charge = H3_Charge;
    e->kaon_candidates[2].h_is_muon = H3_isMuon;
    e->kaon_candidates[2].h_ip_chi2 = H3_IpChi2;
  }

  double B_FlightDistance;
  double B_VertexChi2;
  double H1_PX;
  double H1_PY;
  double H1_PZ;
  double H1_ProbK;
  double H1_ProbPi;
  int H1_Charge;
  int H1_isMuon;
  double H1_IpChi2;
  double H2_PX;
  double H2_PY;
  double H2_PZ;
  double H2_ProbK;
  double H2_ProbPi;
  int H2_Charge;
  int H2_isMuon;
  double H2_IpChi2;
  double H3_PX;
  double H3_PY;
  double H3_PZ;
  double H3_ProbK;
  double H3_ProbPi;
  int H3_Charge;
  int H3_isMuon;
  double H3_IpChi2;
};

#endif  // EVENT_H_
