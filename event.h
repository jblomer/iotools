#ifndef EVENT_H_
#define EVENT_H_

#include <array>
#include <vector>

struct KaonCandidate {
  /*KaonCandidate() : h_px(0.0), h_py(0.0), h_pz(0.0),
    h_prob_k(0.0), h_prob_pi(0.0), h_is_muon(0), h_ip_chi2(0.0)
  { }*/
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


class DeepEvent {
 public:
  void FromEvent(const Event &e) {
    b_flight_distance = e.b_flight_distance;
    b_vertex_chi2 = e.b_vertex_chi2;
    kaon_candidates.clear();
    for (unsigned i = 0; i < 3; ++i) {
      kaon_candidates.push_back(e.kaon_candidates[i]);
    }
  }

  void ToEvent(Event *e) {
    e->b_flight_distance = b_flight_distance;
    e->b_vertex_chi2 = b_vertex_chi2;
    for (unsigned i = 0; i < 3; ++i) {
      assert(kaon_candidates.size() == 3);
      e->kaon_candidates[i] = kaon_candidates[i];
    }
  }

  double b_flight_distance;  // unused
  double b_vertex_chi2;  // unused

  //KaonCandidate *kaon_candidates; //
  //int kaon_candidate_count;
  std::vector<KaonCandidate> kaon_candidates;
};


struct CSplitEvent {
  CSplitEvent()
    : b_flight_distance(0.0)
    , b_vertex_chi2(0.0)
    , nKaons(0)
    , h_px(nullptr)
    , h_py(nullptr)
    , h_pz(nullptr)
    , h_prob_k(nullptr)
    , h_prob_pi(nullptr)
    , h_charge(nullptr)
    , h_is_muon(nullptr)
    , h_ip_chi2(nullptr)
  { }

  ~CSplitEvent() {
    delete[] h_px;
    delete[] h_py;
    delete[] h_pz;
    delete[] h_prob_k;
    delete[] h_prob_pi;
    delete[] h_charge;
    delete[] h_is_muon;
    delete[] h_ip_chi2;
  }

  void FromEvent(const Event &e) {
    b_flight_distance = e.b_flight_distance;
    b_vertex_chi2 = e.b_vertex_chi2;
    nKaons = 3;

    if (h_px == nullptr) {
      h_px = new double[3];
      h_py = new double[3];
      h_pz = new double[3];
      h_prob_k = new double[3];
      h_prob_pi = new double[3];
      h_charge = new int[3];
      h_is_muon = new int[3];
      h_ip_chi2 = new double[3];
    }

    for (unsigned i = 0; i < 3; ++i) {
      h_px[i] = e.kaon_candidates[i].h_px;
      h_py[i] = e.kaon_candidates[i].h_py;
      h_pz[i] = e.kaon_candidates[i].h_pz;
      h_prob_k[i] = e.kaon_candidates[i].h_prob_k;
      h_prob_pi[i] = e.kaon_candidates[i].h_prob_pi;
      h_charge[i] = e.kaon_candidates[i].h_charge;
      h_is_muon[i] = e.kaon_candidates[i].h_is_muon;
      h_ip_chi2[i] = e.kaon_candidates[i].h_ip_chi2;
    }
  }

  void ToEvent(Event *e) {
    e->b_flight_distance = b_flight_distance;
    e->b_vertex_chi2 = b_vertex_chi2;

    assert(nKaons == 3);
    for (unsigned i = 0; i < 3; ++i)
        e->kaon_candidates[i].h_is_muon = h_is_muon[i];
    if (h_px != nullptr) {
      for (unsigned i = 0; i < 3; ++i) {
        e->kaon_candidates[i].h_px = h_px[i];
        e->kaon_candidates[i].h_py = h_py[i];
        e->kaon_candidates[i].h_pz = h_pz[i];
        e->kaon_candidates[i].h_prob_k = h_prob_k[i];
        e->kaon_candidates[i].h_prob_pi = h_prob_pi[i];
        e->kaon_candidates[i].h_charge = h_charge[i];
      }
    }
  }

  double b_flight_distance;  // unused
  double b_vertex_chi2;  // unused

  int nKaons;
  double *h_px; ///<[nKaons]
  double *h_py; ///<[nKaons]
  double *h_pz; ///<[nKaons]
  double *h_prob_k; ///<[nKaons]
  double *h_prob_pi; ///<[nKaons]
  int *h_charge; ///<[nKaons]
  int *h_is_muon; ///<[nKaons]
  double *h_ip_chi2; ///<[nKaons]
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
