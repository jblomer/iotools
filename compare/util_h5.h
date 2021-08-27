/**
 * Copyright CERN; jblomer@cern.ch
 */

#ifndef UTIL_H5_H_
#define UTIL_H5_H_

#include <hdf5.h>

class H5Row {
 public:
  static constexpr hsize_t kDefaultDimension = 8556118;

  struct DataSet {
    double b_flight_distance;
    double b_vertex_chi2;
    double h1_px, h1_py, h1_pz;
    double h1_prob_k, h1_prob_pi;
    int h1_charge, h1_is_muon;
    double h1_ip_chi2;
    double h2_px, h2_py, h2_pz;
    double h2_prob_k, h2_prob_pi;
    int h2_charge, h2_is_muon;
    double h2_ip_chi2;
    double h3_px, h3_py, h3_pz;
    double h3_prob_k, h3_prob_pi;
    int h3_charge, h3_is_muon;
    double h3_ip_chi2;
  };

  explicit H5Row();
  ~H5Row();

  hid_t fTypeId;
};

#endif  // UTIL_H5_H_
