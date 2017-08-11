/**
 * Copyright CERN; jblomer@cern.ch
 */

#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <string>

#include <TBranch.h>
#include <TChain.h>
#include <TClassTable.h>
#include <TSystem.h>

#include "xAOD__PhotonAuxContainer_v3.h"

using namespace std;

void Usage(char *progname) {
  printf("%s -i <input file>\n", progname);
}

int main(int argc, char **argv) {
  if (!TClassTable::GetDict("xAOD::PhotonAuxContainer_v3")) {
      gSystem->Load("schema_aod/libAod.so");
   }

  string input_path;
  int c;
  while ((c = getopt(argc, argv, "hvi:")) != -1) {
    switch (c) {
      case 'h':
      case 'v':
        Usage(argv[0]);
        return 0;
      case 'i':
        input_path = optarg;
        break;
      default:
        fprintf(stderr, "Unknown option: -%c\n", c);
        Usage(argv[0]);
        return 1;
    }
  }

  if (input_path.empty()) {
    Usage(argv[0]);
    return 1;
  }

  TChain *root_chain = new TChain("CollectionTree");
  root_chain->Add(input_path.c_str());
  unsigned nevents = root_chain->GetEntries();
  printf("Found %u events\n", nevents);

  xAOD::PhotonAuxContainer_v3 *photon_aux = new xAOD::PhotonAuxContainer_v3();
  TBranch *br_photons_aux;
  root_chain->SetBranchAddress("PhotonsAux.", &photon_aux, &br_photons_aux);

  for (unsigned i = 0; i < 15; ++i) {
    br_photons_aux->GetEntry(i);

  }
  /*br_photons_aux->GetEntry(0);
  br_photons_aux->GetEntry(1);
  printf("pt size is %lu, values 0 1 2 are %f %f %f\n",
         photon_aux->pt.size(),
         photon_aux->pt[0], photon_aux->pt[1], photon_aux->pt[2]);*/

  return 0;
}
