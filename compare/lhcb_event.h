#ifndef LHCB_EVENT_H_
#define LHCB_EVENT_H_

class LhcbEvent {
 public:
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

#endif // LHCB_EVENT_H_
