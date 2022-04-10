#include "cms_ttree.h"
#include "cms_event_h5.h"

#include <h5hep/h5hep.hxx>

#include <cassert>
#include <cstdio>
#include <memory>
#include <unistd.h>

/// Chunk size defaults to RNTuple default page size divided by sizeof(double)
constexpr size_t kDefaultChunkSize = (64 * 1024) / sizeof(double);

static void Usage(char *progname) {
  printf("Usage: %s -i <input ROOT file> -o <output hdf5 file> [-c <compression level>] [-s <chunk size>]\n", progname);
}

/// __COLUMN_MODEL__ is defined via -D compiler option (see Makefile) 
using Builder = h5hep::schema::SchemaBuilder<__COLUMN_MODEL__>;

auto InitSchema() {
  return Builder::MakeStructNode<CmsEventH5>("CmsEventH5", {
      Builder::MakePrimitiveNode<int>("run", HOFFSET(CmsEventH5, run)),
      Builder::MakePrimitiveNode<unsigned int>("luminosityBlock", HOFFSET(CmsEventH5, luminosityBlock)),
      Builder::MakePrimitiveNode<unsigned long>("event", HOFFSET(CmsEventH5, event)),
      Builder::MakePrimitiveNode<bool>("HLT_IsoMu24_eta2p1", HOFFSET(CmsEventH5, HLT_IsoMu24_eta2p1)),
      Builder::MakePrimitiveNode<bool>("HLT_IsoMu24", HOFFSET(CmsEventH5, HLT_IsoMu24)),
      Builder::MakePrimitiveNode<bool>("HLT_IsoMu17_eta2p1_LooseIsoPFTau20", HOFFSET(CmsEventH5, HLT_IsoMu17_eta2p1_LooseIsoPFTau20)),
      Builder::MakePrimitiveNode<int>("PV_npvs", HOFFSET(CmsEventH5, PV_npvs)),
      Builder::MakePrimitiveNode<float>("PV_x", HOFFSET(CmsEventH5, PV_x)),
      Builder::MakePrimitiveNode<float>("PV_y", HOFFSET(CmsEventH5, PV_y)),
      Builder::MakePrimitiveNode<float>("PV_z", HOFFSET(CmsEventH5, PV_z)),
      Builder::MakeCollectionNode("nMuon", HOFFSET(CmsEventH5, nMuon),
				  Builder::MakeStructNode<Muon>("Muon", {
				      Builder::MakePrimitiveNode<float>("Muon_pt", HOFFSET(Muon, Muon_pt)),
				      Builder::MakePrimitiveNode<float>("Muon_eta", HOFFSET(Muon, Muon_eta)),
				      Builder::MakePrimitiveNode<float>("Muon_phi", HOFFSET(Muon, Muon_phi)),
				      Builder::MakePrimitiveNode<float>("Muon_mass", HOFFSET(Muon, Muon_mass)),
				      Builder::MakePrimitiveNode<int>("Muon_charge", HOFFSET(Muon, Muon_charge)),
				      Builder::MakePrimitiveNode<float>("Muon_pfRelIso03_all", HOFFSET(Muon, Muon_pfRelIso03_all)),
				      Builder::MakePrimitiveNode<float>("Muon_pfRelIso04_all", HOFFSET(Muon, Muon_pfRelIso04_all)),
				      Builder::MakePrimitiveNode<bool>("Muon_tightId", HOFFSET(Muon, Muon_tightId)),
				      Builder::MakePrimitiveNode<bool>("Muon_softId", HOFFSET(Muon, Muon_softId)),
				      Builder::MakePrimitiveNode<float>("Muon_dxy", HOFFSET(Muon, Muon_dxy)),
				      Builder::MakePrimitiveNode<float>("Muon_dxyErr", HOFFSET(Muon, Muon_dxyErr)),
				      Builder::MakePrimitiveNode<float>("Muon_dz", HOFFSET(Muon, Muon_dz)),
				      Builder::MakePrimitiveNode<float>("Muon_dzErr", HOFFSET(Muon, Muon_dzErr)),
				      Builder::MakePrimitiveNode<int>("Muon_jetIdx", HOFFSET(Muon, Muon_jetIdx)),
				      Builder::MakePrimitiveNode<int>("Muon_genPartIdx", HOFFSET(Muon, Muon_genPartIdx)),
				    })),
      Builder::MakeCollectionNode("nElectron", HOFFSET(CmsEventH5, nElectron),
				  Builder::MakeStructNode<Electron>("Electron", {
				      Builder::MakePrimitiveNode<float>("Electron_pt", HOFFSET(Electron, Electron_pt)),
				      Builder::MakePrimitiveNode<float>("Electron_eta", HOFFSET(Electron, Electron_eta)),
				      Builder::MakePrimitiveNode<float>("Electron_phi", HOFFSET(Electron, Electron_phi)),
				      Builder::MakePrimitiveNode<float>("Electron_mass", HOFFSET(Electron, Electron_mass)),
				      Builder::MakePrimitiveNode<int>("Electron_charge", HOFFSET(Electron, Electron_charge)),
				      Builder::MakePrimitiveNode<float>("Electron_pfRelIso03_all", HOFFSET(Electron, Electron_pfRelIso03_all)),
				      Builder::MakePrimitiveNode<float>("Electron_dxy", HOFFSET(Electron, Electron_dxy)),
				      Builder::MakePrimitiveNode<float>("Electron_dxyErr", HOFFSET(Electron, Electron_dxyErr)),
				      Builder::MakePrimitiveNode<float>("Electron_dz", HOFFSET(Electron, Electron_dz)),
				      Builder::MakePrimitiveNode<float>("Electron_dzErr", HOFFSET(Electron, Electron_dzErr)),
				      Builder::MakePrimitiveNode<bool>("Electron_cutBasedId", HOFFSET(Electron, Electron_cutBasedId)),
				      Builder::MakePrimitiveNode<bool>("Electron_pfId", HOFFSET(Electron, Electron_pfId)),
				      Builder::MakePrimitiveNode<int>("Electron_jetIdx", HOFFSET(Electron, Electron_jetIdx)),
				      Builder::MakePrimitiveNode<int>("Electron_genPartIdx", HOFFSET(Electron, Electron_genPartIdx)),
				    })),
      Builder::MakeCollectionNode("nTau", HOFFSET(CmsEventH5, nTau),
				  Builder::MakeStructNode<Tau>("Tau", {
				      Builder::MakePrimitiveNode<float>("Tau_pt", HOFFSET(Tau, Tau_pt)),
				      Builder::MakePrimitiveNode<float>("Tau_eta", HOFFSET(Tau, Tau_eta)),
				      Builder::MakePrimitiveNode<float>("Tau_phi", HOFFSET(Tau, Tau_phi)),
				      Builder::MakePrimitiveNode<float>("Tau_mass", HOFFSET(Tau, Tau_mass)),
				      Builder::MakePrimitiveNode<int>("Tau_charge", HOFFSET(Tau, Tau_charge)),
				      Builder::MakePrimitiveNode<int>("Tau_decayMode", HOFFSET(Tau, Tau_decayMode)),
				      Builder::MakePrimitiveNode<float>("Tau_relIso_all", HOFFSET(Tau, Tau_relIso_all)),
				      Builder::MakePrimitiveNode<int>("Tau_jetIdx", HOFFSET(Tau, Tau_jetIdx)),
				      Builder::MakePrimitiveNode<int>("Tau_genPartIdx", HOFFSET(Tau, Tau_genPartIdx)),
				      Builder::MakePrimitiveNode<bool>("Tau_idDecayMode", HOFFSET(Tau, Tau_idDecayMode)),
				      Builder::MakePrimitiveNode<float>("Tau_idIsoRaw", HOFFSET(Tau, Tau_idIsoRaw)),
				      Builder::MakePrimitiveNode<bool>("Tau_idIsoVLoose", HOFFSET(Tau, Tau_idIsoVLoose)),
				      Builder::MakePrimitiveNode<bool>("Tau_idIsoLoose", HOFFSET(Tau, Tau_idIsoLoose)),
				      Builder::MakePrimitiveNode<bool>("Tau_idIsoMedium", HOFFSET(Tau, Tau_idIsoMedium)),
				      Builder::MakePrimitiveNode<bool>("Tau_idIsoTight", HOFFSET(Tau, Tau_idIsoTight)),
				      Builder::MakePrimitiveNode<bool>("Tau_idAntiEleLoose", HOFFSET(Tau, Tau_idAntiEleLoose)),
				      Builder::MakePrimitiveNode<bool>("Tau_idAntiEleMedium", HOFFSET(Tau, Tau_idAntiEleMedium)),
				      Builder::MakePrimitiveNode<bool>("Tau_idAntiEleTight", HOFFSET(Tau, Tau_idAntiEleTight)),
				      Builder::MakePrimitiveNode<bool>("Tau_idAntiMuLoose", HOFFSET(Tau, Tau_idAntiMuLoose)),
				      Builder::MakePrimitiveNode<bool>("Tau_idAntiMuMedium", HOFFSET(Tau, Tau_idAntiMuMedium)),
				      Builder::MakePrimitiveNode<bool>("Tau_idAntiMuTight", HOFFSET(Tau, Tau_idAntiMuTight)),
				    })),
      Builder::MakePrimitiveNode<float>("MET_pt", HOFFSET(CmsEventH5, MET_pt)),
      Builder::MakePrimitiveNode<float>("MET_phi", HOFFSET(CmsEventH5, MET_phi)),
      Builder::MakePrimitiveNode<float>("MET_sumet", HOFFSET(CmsEventH5, MET_sumet)),
      Builder::MakePrimitiveNode<float>("MET_significance", HOFFSET(CmsEventH5, MET_significance)),
      Builder::MakePrimitiveNode<float>("MET_CovXX", HOFFSET(CmsEventH5, MET_CovXX)),
      Builder::MakePrimitiveNode<float>("MET_CovXY", HOFFSET(CmsEventH5, MET_CovXY)),
      Builder::MakePrimitiveNode<float>("MET_CovYY", HOFFSET(CmsEventH5, MET_CovYY)),
      Builder::MakeCollectionNode("nJet", HOFFSET(CmsEventH5, nJet),
				  Builder::MakeStructNode<Jet>("Jet", {
				      Builder::MakePrimitiveNode<float>("Jet_pt", HOFFSET(Jet, Jet_pt)),
				      Builder::MakePrimitiveNode<float>("Jet_eta", HOFFSET(Jet, Jet_eta)),
				      Builder::MakePrimitiveNode<float>("Jet_phi", HOFFSET(Jet, Jet_phi)),
				      Builder::MakePrimitiveNode<float>("Jet_mass", HOFFSET(Jet, Jet_mass)),
				      Builder::MakePrimitiveNode<bool>("Jet_puId", HOFFSET(Jet, Jet_puId)),
				      Builder::MakePrimitiveNode<float>("Jet_btag", HOFFSET(Jet, Jet_btag)),
				    })),
      Builder::MakeCollectionNode("nGenPart", HOFFSET(CmsEventH5, nGenPart),
				  Builder::MakeStructNode<GenPart>("GenPart", {
				      Builder::MakePrimitiveNode<float>("GenPart_pt", HOFFSET(GenPart, GenPart_pt)),
				      Builder::MakePrimitiveNode<float>("GenPart_eta", HOFFSET(GenPart, GenPart_eta)),
				      Builder::MakePrimitiveNode<float>("GenPart_phi", HOFFSET(GenPart, GenPart_phi)),
				      Builder::MakePrimitiveNode<float>("GenPart_mass", HOFFSET(GenPart, GenPart_mass)),
				      Builder::MakePrimitiveNode<int>("GenPart_pdgId", HOFFSET(GenPart, GenPart_pdgId)),
				      Builder::MakePrimitiveNode<int>("GenPart_status", HOFFSET(GenPart, GenPart_status)),
				    })),
    });
}

int main(int argc, char **argv) {
  std::string inputPath;
  std::string outputPath;
  unsigned compressionLevel = 0;
  std::size_t chunkSize = kDefaultChunkSize;

  int c;
  while ((c = getopt(argc, argv, "hvi:o:c:s:")) != -1) {
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
      case 'c':
        compressionLevel = std::atoi(optarg);
        break;
      case 's':
        chunkSize = std::atoi(optarg);
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

  h5hep::WriteProperties props;
  props.SetChunkSize(chunkSize);
  props.SetCompressionLevel(compressionLevel);

  auto schema = InitSchema();
  auto file = h5hep::H5File::Create(outputPath);
  // Use the default RNTuple cluster size as the size for HDF5 chunk cache
  std::static_pointer_cast<h5hep::H5File>(file)->SetCache(50 * 1000 * 1000);
  auto writer = Builder::MakeReaderWriter(file, schema, props);

  // FIXME: `BufferedWriter::Write(T &&)` and `h5hep::Collection` move assignment operator do not work as expected.
  // As a workaround, storage for collections is backed by some `std::vector`s here.
  auto nMuon_chunk = std::make_unique<std::vector<Muon>[]>(chunkSize);
  auto nElectron_chunk = std::make_unique<std::vector<Electron>[]>(chunkSize);
  auto nTau_chunk = std::make_unique<std::vector<Tau>[]>(chunkSize);
  auto nJet_chunk = std::make_unique<std::vector<Jet>[]>(chunkSize);
  auto nGenPart_chunk = std::make_unique<std::vector<GenPart>[]>(chunkSize);

  h5hep::BufferedWriter<CmsEventH5> bw(writer);
  size_t nEvent = 0;
  while (reader.NextEvent()) {
    const auto &row = reader.fEvent;

    auto &nMuon = nMuon_chunk[bw.GetCount()];
    nMuon.clear();
    nMuon.reserve(row.nMuon);
    for (int i = 0; i < row.nMuon; ++i) {
      nMuon.push_back({row.Muon_pt[i], row.Muon_eta[i], row.Muon_phi[i], row.Muon_mass[i], row.Muon_charge[i],
	  row.Muon_pfRelIso03_all[i], row.Muon_pfRelIso04_all[i],
	  row.Muon_tightId[i], row.Muon_softId[i],
	  row.Muon_dxy[i], row.Muon_dxyErr[i],
	  row.Muon_dz[i], row.Muon_dzErr[i],
	  row.Muon_jetIdx[i],
	  row.Muon_genPartIdx[i]});
    }

    auto &nElectron = nElectron_chunk[bw.GetCount()];
    nElectron.clear();
    nElectron.reserve(row.nElectron);
    for (int i = 0; i < row.nElectron; ++i) {
      nElectron.push_back({row.Electron_pt[i], row.Electron_eta[i], row.Electron_phi[i], row.Electron_mass[i], row.Electron_charge[i],
	  row.Electron_pfRelIso03_all[i],
	  row.Electron_dxy[i], row.Electron_dxyErr[i],
	  row.Electron_dz[i], row.Electron_dzErr[i],
	  row.Electron_cutBasedId[i],
	  row.Electron_pfId[i],
	  row.Electron_jetIdx[i],
	  row.Electron_genPartIdx[i]});
    }

    auto &nTau = nTau_chunk[bw.GetCount()];
    nTau.clear();
    nTau.reserve(row.nTau);
    for (int i = 0; i < row.nTau; ++i) {
      nTau.push_back({row.Tau_pt[i], row.Tau_eta[i], row.Tau_phi[i], row.Tau_mass[i], row.Tau_charge[i],
	  row.Tau_decayMode[i],
	  row.Tau_relIso_all[i],
	  row.Tau_jetIdx[i],
	  row.Tau_genPartIdx[i],
	  row.Tau_idDecayMode[i],
	  row.Tau_idIsoRaw[i],
	  row.Tau_idIsoVLoose[i],
	  row.Tau_idIsoLoose[i], row.Tau_idIsoMedium[i], row.Tau_idIsoTight[i],
	  row.Tau_idAntiEleLoose[i], row.Tau_idAntiEleMedium[i], row.Tau_idAntiEleTight[i],
	  row.Tau_idAntiMuLoose[i], row.Tau_idAntiMuMedium[i], row.Tau_idAntiMuTight[i]});
    }

    auto &nJet = nJet_chunk[bw.GetCount()];
    nJet.clear();
    nJet.reserve(row.nJet);
    for (int i = 0; i < row.nJet; ++i) {
      nJet.push_back({row.Jet_pt[i], row.Jet_eta[i], row.Jet_phi[i], row.Jet_mass[i],
	  row.Jet_puId[i],
	  row.Jet_btag[i]});
    }

    auto &nGenPart = nGenPart_chunk[bw.GetCount()];
    nGenPart.clear();
    nGenPart.reserve(row.nGenPart);
    for (int i = 0; i < row.nGenPart; ++i) {
      nGenPart.push_back({row.GenPart_pt[i], row.GenPart_eta[i], row.GenPart_phi[i], row.GenPart_mass[i],
	  row.GenPart_pdgId[i],
	  row.GenPart_status[i]});
    }

    CmsEventH5 out{};
    out.run                = row.run;
    out.luminosityBlock    = row.luminosityBlock;
    out.event              = row.event;
    out.HLT_IsoMu24_eta2p1 = row.HLT_IsoMu24_eta2p1;
    out.HLT_IsoMu24        = row.HLT_IsoMu24;
    out.HLT_IsoMu17_eta2p1_LooseIsoPFTau20 = row.HLT_IsoMu17_eta2p1_LooseIsoPFTau20;
    out.PV_npvs            = row.PV_npvs;
    out.PV_x               = row.PV_x;
    out.PV_y               = row.PV_y;
    out.PV_z               = row.PV_z;
    out.nMuon              = nMuon;
    out.nElectron          = nElectron;
    out.nTau               = nTau;
    out.MET_pt             = row.MET_pt;
    out.MET_phi            = row.MET_phi;
    out.MET_sumet          = row.MET_sumet;
    out.MET_significance   = row.MET_significance;
    out.MET_CovXX          = row.MET_CovXX;
    out.MET_CovXY          = row.MET_CovXY;
    out.MET_CovYY          = row.MET_CovYY;
    out.nJet               = nJet;
    out.nGenPart           = nGenPart;

    bw.Write(out);

    if (reader.fPos % 100000 == 0)
      printf(" ... processed %d events\n", reader.fPos);
    nEvent++;
  }
  printf("[done] processed %d events\n", reader.fPos);
  return 0;
}
