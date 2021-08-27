#include "util.h"
#include "util_h5.h"

#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <string>

static void Usage(char *progname) {
  printf("Usage: %s -i <input ROOT file> -o <output hdf5 file>\n", progname);
}

int main(int argc, char **argv) {
  std::string inputPath;
  std::string outputPath;

  int c;
  while ((c = getopt(argc, argv, "hvi:o:")) != -1) {
    switch (c) {
      case 'h':
      case 'v':
        Usage(argv[0]);
        return 0;
      case 'i':
        inputPath = optarg;
        break;
      case 'o':
        outputPath = optarg;
        break;
      default:
        fprintf(stderr, "Unknown option: -%c\n", c);
        Usage(argv[0]);
        return 1;
    }
  }
  assert(!inputPath.empty() && !outputPath.empty());

  printf("Converting %s --> %s\n", inputPath.c_str(), outputPath.c_str());
  EventReaderRoot reader;
  reader.Open(inputPath);

  H5Row h5row;
  auto fileId = H5Fcreate(outputPath.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  assert(fileId >= 0);
  auto spaceId = H5Screate_simple(1, &h5row.kDefaultDimension, NULL);
  assert(spaceId >= 0);
  auto setId = H5Dcreate(fileId, "/DecayTree", h5row.fTypeId, spaceId,
                         H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  assert(setId >= 0);
  auto memSpaceId = H5Screate(H5S_SCALAR);
  assert(memSpaceId >= 0);

  H5Row::DataSet dataset;
  hsize_t nEvent = 0;
  while (reader.NextEvent()) {
    if (reader.fPos % 100000 == 0)
      printf(" ... processed %d events\n", reader.fPos);

    dataset.b_flight_distance = reader.fEvent.B_FlightDistance;
    dataset.b_vertex_chi2     = reader.fEvent.B_VertexChi2;
    dataset.h1_px             = reader.fEvent.H1_PX;
    dataset.h1_py             = reader.fEvent.H1_PY;
    dataset.h1_pz             = reader.fEvent.H1_PZ;
    dataset.h1_prob_k         = reader.fEvent.H1_ProbK;
    dataset.h1_prob_pi        = reader.fEvent.H1_ProbPi;
    dataset.h1_charge         = reader.fEvent.H1_Charge;
    dataset.h1_is_muon        = reader.fEvent.H1_isMuon;
    dataset.h1_ip_chi2        = reader.fEvent.H1_IpChi2;
    dataset.h2_px             = reader.fEvent.H2_PX;
    dataset.h2_py             = reader.fEvent.H2_PY;
    dataset.h2_pz             = reader.fEvent.H2_PZ;
    dataset.h2_prob_k         = reader.fEvent.H2_ProbK;
    dataset.h2_prob_pi        = reader.fEvent.H2_ProbPi;
    dataset.h2_charge         = reader.fEvent.H2_Charge;
    dataset.h2_is_muon        = reader.fEvent.H2_isMuon;
    dataset.h2_ip_chi2        = reader.fEvent.H2_IpChi2;
    dataset.h3_px             = reader.fEvent.H3_PX;
    dataset.h3_py             = reader.fEvent.H3_PY;
    dataset.h3_pz             = reader.fEvent.H3_PZ;
    dataset.h3_prob_k         = reader.fEvent.H3_ProbK;
    dataset.h3_prob_pi        = reader.fEvent.H3_ProbPi;
    dataset.h3_charge         = reader.fEvent.H3_Charge;
    dataset.h3_is_muon        = reader.fEvent.H3_isMuon;
    dataset.h3_ip_chi2        = reader.fEvent.H3_IpChi2;

    hsize_t count = 1;
    auto retval = H5Sselect_hyperslab(spaceId, H5S_SELECT_SET, &nEvent, NULL, &count, NULL);
    assert(retval >= 0);
    retval = H5Dwrite(setId, h5row.fTypeId, memSpaceId, spaceId, H5P_DEFAULT, &dataset);
    assert(retval >= 0);
    nEvent++;
  }
  printf("[done] processed %d events\n", reader.fPos);

  H5Sclose(memSpaceId);
  H5Dclose(setId);
  H5Sclose(spaceId);
  H5Fclose(fileId);

  return 0;
}
