#include <ROOT/RField.hxx>
#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleOptions.hxx>

#include <TBranch.h>
#include <TCanvas.h>
#include <TChain.h>
#include <TFile.h>
#include <TH1F.h>
#include <TLeaf.h>
#include <TTree.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TTreeReaderArray.h>

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <unistd.h>

#include "util.h"

// Import classes from experimental namespace for the time being
using RNTupleModel = ROOT::Experimental::RNTupleModel;
using RFieldBase = ROOT::Experimental::Detail::RFieldBase;
using RNTupleWriter = ROOT::Experimental::RNTupleWriter;
using RNTupleWriteOptions = ROOT::Experimental::RNTupleWriteOptions;

void Usage(char *progname) {
   std::cout << "Usage: " << progname << " -o <ntuple-path> -c <compression> <H1 dst files>" << std::endl;
}


int main(int argc, char **argv) {
   std::vector<std::string> inputFiles;
   std::string outputPath = ".";
   int compressionSettings = 0;
   std::string compressionShorthand = "none";

   int c;
   while ((c = getopt(argc, argv, "hvo:c:")) != -1) {
      switch (c) {
      case 'h':
      case 'v':
         Usage(argv[0]);
         return 0;
      case 'o':
         outputPath = optarg;
         break;
      case 'c':
         compressionSettings = GetCompressionSettings(optarg);
         compressionShorthand = optarg;
         break;
      default:
         fprintf(stderr, "Unknown option: -%c\n", c);
         Usage(argv[0]);
         return 1;
      }
   }
   for (auto i = optind; i < argc; ++i)
      inputFiles.emplace_back(argv[i]);

   std::string outputFile = outputPath + "/h1dst~" + compressionShorthand + ".ntuple";
   std::cout << "Converting " << JoinStrings(inputFiles, " ") << " --> " << outputFile << std::endl;

   TChain *tree = new TChain();
   for (auto p : inputFiles)
      tree->Add(p.c_str());

   TTreeReader reader(tree);
   TTreeReaderValue<std::int32_t>   nrun(reader, "nrun"); // 0
   TTreeReaderValue<std::int32_t>   nevent(reader, "nevent"); // 1
   TTreeReaderValue<std::int32_t>   nentry(reader, "nentry"); // 2
   TTreeReaderArray<unsigned char>  trelem(reader, "trelem"); // 3
   TTreeReaderArray<unsigned char>  subtr(reader, "subtr"); // 4
   TTreeReaderArray<unsigned char>  rawtr(reader, "rawtr"); // 5
   TTreeReaderArray<unsigned char>  L4subtr(reader, "L4subtr"); // 6
   TTreeReaderArray<unsigned char>  L5class(reader, "L5class"); // 7
   TTreeReaderValue<float>          E33(reader, "E33"); // 8
   TTreeReaderValue<float>          de33(reader, "de33"); // 9
   TTreeReaderValue<float>          x33(reader, "x33"); // 10
   TTreeReaderValue<float>          dx33(reader, "dx33"); // 11
   TTreeReaderValue<float>          y33(reader, "y33"); // 12
   TTreeReaderValue<float>          dy33(reader, "dy33"); // 13
   TTreeReaderValue<float>          E44(reader, "E44"); // 14
   TTreeReaderValue<float>          de44(reader, "de44"); // 15
   TTreeReaderValue<float>          x44(reader, "x44"); // 16
   TTreeReaderValue<float>          dx44(reader, "dx44"); // 17
   TTreeReaderValue<float>          y44(reader, "y44"); // 18
   TTreeReaderValue<float>          dy44(reader, "dy44"); // 19
   TTreeReaderValue<float>          Ept(reader, "Ept"); // 20
   TTreeReaderValue<float>          dept(reader, "dept"); // 21
   TTreeReaderValue<float>          xpt(reader, "xpt"); // 22
   TTreeReaderValue<float>          dxpt(reader, "dxpt"); // 23
   TTreeReaderValue<float>          ypt(reader, "ypt"); // 24
   TTreeReaderValue<float>          dypt(reader, "dypt"); // 25
   TTreeReaderArray<float>          pelec(reader, "pelec"); // 26, size = 4
   TTreeReaderValue<std::int32_t>   flagelec(reader, "flagelec"); // 27
   TTreeReaderValue<float>          xeelec(reader, "xeelec"); // 28
   TTreeReaderValue<float>          yeelec(reader, "yeelec"); // 29
   TTreeReaderValue<float>          Q2eelec(reader, "Q2eelec"); // 30
   TTreeReaderValue<std::int32_t>   nelec(reader, "nelec"); // 31

   TTreeReaderArray<float>          Eelec(reader, "Eelec"); // 32, size = nelec
   TTreeReaderArray<float>          thetelec(reader, "thetelec"); // 33, size = nelec
   TTreeReaderArray<float>          phielec(reader, "phielec"); // 34, size = nelec
   TTreeReaderArray<float>          xelec(reader, "xelec"); // 35, size = nelec
   TTreeReaderArray<float>          Q2elec(reader, "Q2elec"); // 36, size = nelec
   TTreeReaderArray<float>          xsigma(reader, "xsigma"); // 37, size = nelec
   TTreeReaderArray<float>          Q2sigma(reader, "Q2sigma"); // 38, size = nelec

   TTreeReaderArray<float>          sumc(reader, "sumc"); // 39, size = 4
   TTreeReaderValue<float>          sumetc(reader, "sumetc"); // 40
   TTreeReaderValue<float>          yjbc(reader, "yjbc"); // 41
   TTreeReaderValue<float>          Q2jbc(reader, "Q2jbc"); // 42
   TTreeReaderArray<float>          sumct(reader, "sumct"); // 43, size = 4
   TTreeReaderValue<float>          sumetct(reader, "sumetct"); // 44
   TTreeReaderValue<float>          yjbct(reader, "yjbct"); // 45
   TTreeReaderValue<float>          Q2jbct(reader, "Q2jbct"); // 46
   TTreeReaderValue<float>          Ebeamel(reader, "Ebeamel"); // 47
   TTreeReaderValue<float>          Ebeampr(reader, "Ebeampr"); // 48

   TTreeReaderArray<float>          pvtx_d(reader, "xelec"); // 49, size = 3
   TTreeReaderArray<float>          cpvtx_d(reader, "cpvtx_d"); // 50, size = 6
   TTreeReaderArray<float>          pvtx_t(reader, "pvtx_t"); // 51, size = 3
   TTreeReaderArray<float>          cpvtx_t(reader, "cpvtx_t"); // 52, size = 6

   TTreeReaderValue<std::int32_t>   ntrkxy_t(reader, "ntrkxy_t"); // 53
   TTreeReaderValue<float>          prbxy_t(reader, "prbxy_t"); // 54
   TTreeReaderValue<std::int32_t>   ntrkz_t(reader, "ntrkz_t"); // 55
   TTreeReaderValue<float>          prbz_t(reader, "prbz_t"); // 56
   TTreeReaderValue<std::int32_t>   nds(reader, "nds"); // 57
   TTreeReaderValue<std::int32_t>   rankds(reader, "rankds"); // 58
   TTreeReaderValue<std::int32_t>   qds(reader, "qds"); // 59
   TTreeReaderArray<float>          pds_d(reader, "pds_d"); // 60, size = 4
   TTreeReaderValue<float>          ptds_d(reader, "ptds_d"); // 61
   TTreeReaderValue<float>          etads_d(reader, "etads_d"); // 62
   TTreeReaderValue<float>          dm_d(reader, "dm_d"); // 63
   TTreeReaderValue<float>          ddm_d(reader, "ddm_d"); // 64
   TTreeReaderArray<float>          pds_t(reader, "pds_t"); // 65, size = 4
   TTreeReaderValue<float>          dm_t(reader, "dm_t"); // 66
   TTreeReaderValue<float>          ddm_t(reader, "ddm_t"); // 67

   TTreeReaderValue<std::int32_t>   ik(reader, "ik"); // 68
   TTreeReaderValue<std::int32_t>   ipi(reader, "ipi"); // 69
   TTreeReaderValue<std::int32_t>   ipis(reader, "ipis"); // 70
   TTreeReaderArray<float>          pd0_d(reader, "pd0_d"); // 71, size = 4
   TTreeReaderValue<float>          ptd0_d(reader, "ptd0_d"); // 72
   TTreeReaderValue<float>          etad0_d(reader, "etad0_d"); // 73
   TTreeReaderValue<float>          md0_d(reader, "md0_d"); // 74
   TTreeReaderValue<float>          dmd0_d(reader, "dmd0_d"); // 75
   TTreeReaderArray<float>          pd0_t(reader, "pd0_t"); // 76
   TTreeReaderValue<float>          md0_t(reader, "md0_t"); // 77
   TTreeReaderValue<float>          dmd0_t(reader, "dmd0_t"); // 78
   TTreeReaderArray<float>          pk_r(reader, "pk_r"); // 79, size = 4
   TTreeReaderArray<float>          ppi_r(reader, "ppi_r"); // 80, size = 4
   TTreeReaderArray<float>          pd0_r(reader, "pd0_r"); // 81, size = 4

   TTreeReaderValue<float>          md0_r(reader, "md0_r"); // 82
   TTreeReaderArray<float>          Vtxd0_r(reader, "Vtxd0_r"); // 83, size = 3
   TTreeReaderArray<float>          cvtxd0_r(reader, "cvtxd0_r"); // 84, size = 6
   TTreeReaderValue<float>          dxy_r(reader, "dxy_r"); // 85
   TTreeReaderValue<float>          dz_r(reader, "dz_r"); // 86
   TTreeReaderValue<float>          psi_r(reader, "psi_r"); // 87
   TTreeReaderValue<float>          rd0_d(reader, "rd0_d"); // 88
   TTreeReaderValue<float>          drd0_d(reader, "drd0_d"); // 89
   TTreeReaderValue<float>          rpd0_d(reader, "rpd0_d"); // 90
   TTreeReaderValue<float>          drpd0_d(reader, "drpd0_d"); // 91
   TTreeReaderValue<float>          rd0_t(reader, "rd0_t"); // 92
   TTreeReaderValue<float>          drd0_t(reader, "drd0_t"); // 93
   TTreeReaderValue<float>          rpd0_t(reader, "rpd0_t"); // 94
   TTreeReaderValue<float>          drpd0_t(reader, "drpd0_t"); // 95
   TTreeReaderValue<float>          rd0_dt(reader, "rd0_dt"); // 96
   TTreeReaderValue<float>          drd0_dt(reader, "drd0_dt"); // 97
   TTreeReaderValue<float>          prbr_dt(reader, "prbr_dt"); // 98
   TTreeReaderValue<float>          prbz_dt(reader, "prbz_dt"); // 99
   TTreeReaderValue<float>          rd0_tt(reader, "rd0_tt"); // 100
   TTreeReaderValue<float>          drd0_tt(reader, "drd0_tt"); // 101
   TTreeReaderValue<float>          prbr_tt(reader, "prbr_tt"); // 102
   TTreeReaderValue<float>          prbz_tt(reader, "prbz_tt"); // 103
   TTreeReaderValue<std::int32_t>   ijetd0(reader, "ijetd0"); // 104
   TTreeReaderValue<float>          ptr3d0_j(reader, "ptr3d0_j"); // 105
   TTreeReaderValue<float>          ptr2d0_j(reader, "ptr2d0_j"); // 106
   TTreeReaderValue<float>          ptr3d0_3(reader, "ptr3d0_3"); // 107
   TTreeReaderValue<float>          ptr2d0_3(reader, "ptr2d0_3"); // 108
   TTreeReaderValue<float>          ptr2d0_2(reader, "ptr2d0_2"); // 109
   TTreeReaderValue<float>          Mimpds_r(reader, "Mimpds_r"); // 110
   TTreeReaderValue<float>          Mimpbk_r(reader, "Mimpbk_r"); // 111

   TTreeReaderValue<std::int32_t>   ntracks(reader, "ntracks"); // 112
   TTreeReaderArray<float>          pt(reader, "pt"); // 113
   TTreeReaderArray<float>          kappa(reader, "kappa"); // 114
   TTreeReaderArray<float>          phi(reader, "phi"); // 115
   TTreeReaderArray<float>          theta(reader, "theta"); // 116
   TTreeReaderArray<float>          dca(reader, "dca"); // 117
   TTreeReaderArray<float>          z0(reader, "z0"); // 118
   //TTreeReaderArray<float>        covar(reader, "covar"); // 119
   TTreeReaderArray<std::int32_t>   nhitrp(reader, "nhitrp"); // 120
   TTreeReaderArray<float>          prbrp(reader, "prbrp"); // 121
   TTreeReaderArray<std::int32_t>   nhitz(reader, "nhitz"); // 122
   TTreeReaderArray<float>          prbz(reader, "prbz"); // 123
   TTreeReaderArray<float>          rstart(reader, "rstart"); // 124
   TTreeReaderArray<float>          rend(reader, "rend"); // 125
   TTreeReaderArray<float>          lhk(reader, "lhk"); // 126
   TTreeReaderArray<float>          lhpi(reader, "lhpi"); // 127
   TTreeReaderArray<float>          nlhk(reader, "nlhk"); // 128
   TTreeReaderArray<float>          nlhpi(reader, "nlhpi"); // 129
   TTreeReaderArray<float>          dca_d(reader, "dca_d"); // 130
   TTreeReaderArray<float>          ddca_d(reader, "ddca_d"); // 131
   TTreeReaderArray<float>          dca_t(reader, "dca_t"); // 132
   TTreeReaderArray<float>          ddca_t(reader, "ddca_t"); // 133
   TTreeReaderArray<std::int32_t>   muqual(reader, "muqual"); // 134

   TTreeReaderValue<std::int32_t>   imu(reader, "imu"); // 135
   TTreeReaderValue<std::int32_t>   imufe(reader, "imufe"); // 136

   TTreeReaderValue<std::int32_t>   njets(reader, "njets"); // 137
   TTreeReaderArray<float>          E_j(reader, "E_j"); // 138
   TTreeReaderArray<float>          pt_j(reader, "pt_j"); // 139
   TTreeReaderArray<float>          theta_j(reader, "theta_j"); // 140
   TTreeReaderArray<float>          eta_j(reader, "eta_j"); // 141
   TTreeReaderArray<float>          phi_j(reader, "phi_j"); // 142
   TTreeReaderArray<float>          m_j(reader, "m_j"); // 143

   TTreeReaderValue<float>          thrust(reader, "thrust"); // 144
   TTreeReaderArray<float>          pthrust(reader, "pthrust"); // 145, size = 4
   TTreeReaderValue<float>          thrust2(reader, "thrust2"); // 146
   TTreeReaderArray<float>          pthrust2(reader, "pthrust2"); // 147, size = 4
   TTreeReaderValue<float>          spher(reader, "spher"); // 148
   TTreeReaderValue<float>          aplan(reader, "aplan"); // 149
   TTreeReaderValue<float>          plan(reader, "plan"); // 150
   TTreeReaderArray<float>          nnout(reader, "nnout"); // 151
   // The new ntuple takes ownership of the model
   //RNTupleWriteOptions options;
   //options.SetCompression(compressionSettings);
   //options.SetNumElementsPerPage(64000);
   //auto ntuple = RNTupleWriter::Recreate(std::move(model), "DecayTree", outputFile, options);
//
   //auto nEntries = tree->GetEntries();
   //for (decltype(nEntries) i = 0; i < nEntries; ++i) {
   //   tree->GetEntry(i);
   //   ntuple->Fill();
//
   //   if (i && i % 100000 == 0)
   //      std::cout << "Wrote " << i << " entries" << std::endl;
   //}
}
