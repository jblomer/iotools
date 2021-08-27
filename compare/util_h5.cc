#include "util_h5.h"

#include <cassert>

H5Row::H5Row() {
  fTypeId = H5Tcreate(H5T_COMPOUND, sizeof(DataSet));
  assert(fTypeId >= 0);
  H5Tinsert(fTypeId, "B_FlightDistance", HOFFSET(DataSet, b_flight_distance),
            H5T_NATIVE_DOUBLE);
  H5Tinsert(fTypeId, "B_VertexChi2", HOFFSET(DataSet, b_vertex_chi2),
            H5T_NATIVE_DOUBLE);
  H5Tinsert(fTypeId, "H1_PX", HOFFSET(DataSet, h1_px), H5T_NATIVE_DOUBLE);
  H5Tinsert(fTypeId, "H1_PY", HOFFSET(DataSet, h1_py), H5T_NATIVE_DOUBLE);
  H5Tinsert(fTypeId, "H1_PZ", HOFFSET(DataSet, h1_pz), H5T_NATIVE_DOUBLE);
  H5Tinsert(fTypeId, "H1_ProbK", HOFFSET(DataSet, h1_prob_k),
            H5T_NATIVE_DOUBLE);
  H5Tinsert(fTypeId, "H1_ProbPi", HOFFSET(DataSet, h1_prob_pi),
            H5T_NATIVE_DOUBLE);
  H5Tinsert(fTypeId, "H1_Charge", HOFFSET(DataSet, h1_charge), H5T_NATIVE_INT);
  H5Tinsert(fTypeId, "H1_isMuon", HOFFSET(DataSet, h1_is_muon),
            H5T_NATIVE_INT);
  H5Tinsert(fTypeId, "H1_IpChi2", HOFFSET(DataSet, h1_ip_chi2),
            H5T_NATIVE_DOUBLE);
  H5Tinsert(fTypeId, "H2_PX", HOFFSET(DataSet, h2_px), H5T_NATIVE_DOUBLE);
  H5Tinsert(fTypeId, "H2_PY", HOFFSET(DataSet, h2_py), H5T_NATIVE_DOUBLE);
  H5Tinsert(fTypeId, "H2_PZ", HOFFSET(DataSet, h2_pz), H5T_NATIVE_DOUBLE);
  H5Tinsert(fTypeId, "H2_ProbK", HOFFSET(DataSet, h2_prob_k),
            H5T_NATIVE_DOUBLE);
  H5Tinsert(fTypeId, "H2_ProbPi", HOFFSET(DataSet, h2_prob_pi),
            H5T_NATIVE_DOUBLE);
  H5Tinsert(fTypeId, "H2_Charge", HOFFSET(DataSet, h2_charge), H5T_NATIVE_INT);
  H5Tinsert(fTypeId, "H2_isMuon", HOFFSET(DataSet, h2_is_muon),
            H5T_NATIVE_INT);
  H5Tinsert(fTypeId, "H2_IpChi2", HOFFSET(DataSet, h2_ip_chi2),
            H5T_NATIVE_DOUBLE);
  H5Tinsert(fTypeId, "H3_PX", HOFFSET(DataSet, h3_px), H5T_NATIVE_DOUBLE);
  H5Tinsert(fTypeId, "H3_PY", HOFFSET(DataSet, h3_py), H5T_NATIVE_DOUBLE);
  H5Tinsert(fTypeId, "H3_PZ", HOFFSET(DataSet, h3_pz), H5T_NATIVE_DOUBLE);
  H5Tinsert(fTypeId, "H3_ProbK", HOFFSET(DataSet, h3_prob_k),
            H5T_NATIVE_DOUBLE);
  H5Tinsert(fTypeId, "H3_ProbPi", HOFFSET(DataSet, h3_prob_pi),
            H5T_NATIVE_DOUBLE);
  H5Tinsert(fTypeId, "H3_Charge", HOFFSET(DataSet, h3_charge), H5T_NATIVE_INT);
  H5Tinsert(fTypeId, "H3_isMuon", HOFFSET(DataSet, h3_is_muon),
            H5T_NATIVE_INT);
  H5Tinsert(fTypeId, "H3_IpChi2", HOFFSET(DataSet, h3_ip_chi2),
            H5T_NATIVE_DOUBLE);
}


H5Row::~H5Row() {
  H5Tclose(fTypeId);
}
