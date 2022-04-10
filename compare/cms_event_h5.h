#ifndef CMS_EVENT_H5_H_
#define CMS_EVENT_H5_H_

#include <h5hep/h5hep.hxx>

struct Muon {
  float Muon_pt;
  float Muon_eta;
  float Muon_phi;
  float Muon_mass;
  int Muon_charge;
  float Muon_pfRelIso03_all;
  float Muon_pfRelIso04_all;
  bool Muon_tightId;
  bool Muon_softId;
  float Muon_dxy;
  float Muon_dxyErr;
  float Muon_dz;
  float Muon_dzErr;
  int Muon_jetIdx;
  int Muon_genPartIdx;
};

struct Electron {
  float Electron_pt;
  float Electron_eta;
  float Electron_phi;
  float Electron_mass;
  int Electron_charge;
  float Electron_pfRelIso03_all;
  float Electron_dxy;
  float Electron_dxyErr;
  float Electron_dz;
  float Electron_dzErr;
  bool Electron_cutBasedId;
  bool Electron_pfId;
  int Electron_jetIdx;
  int Electron_genPartIdx;
};

struct Tau {
  float Tau_pt;
  float Tau_eta;
  float Tau_phi;
  float Tau_mass;
  int Tau_charge;
  int Tau_decayMode;
  float Tau_relIso_all;
  int Tau_jetIdx;
  int Tau_genPartIdx;
  bool Tau_idDecayMode;
  float Tau_idIsoRaw;
  bool Tau_idIsoVLoose;
  bool Tau_idIsoLoose;
  bool Tau_idIsoMedium;
  bool Tau_idIsoTight;
  bool Tau_idAntiEleLoose;
  bool Tau_idAntiEleMedium;
  bool Tau_idAntiEleTight;
  bool Tau_idAntiMuLoose;
  bool Tau_idAntiMuMedium;
  bool Tau_idAntiMuTight;
};

struct Jet {
  float Jet_pt;
  float Jet_eta;
  float Jet_phi;
  float Jet_mass;
  bool Jet_puId;
  float Jet_btag;
};

struct GenPart {
  float GenPart_pt;
  float GenPart_eta;
  float GenPart_phi;
  float GenPart_mass;
  int GenPart_pdgId;
  int GenPart_status;
};

struct CmsEventH5 {
  int run;
  unsigned int luminosityBlock;
  unsigned long long event;
  bool HLT_IsoMu24_eta2p1;
  bool HLT_IsoMu24;
  bool HLT_IsoMu17_eta2p1_LooseIsoPFTau20;
  int PV_npvs;
  float PV_x;
  float PV_y;
  float PV_z;
  h5hep::Collection<Muon> nMuon;
  h5hep::Collection<Electron> nElectron;
  h5hep::Collection<Tau> nTau;
  float MET_pt;
  float MET_phi;
  float MET_sumet;
  float MET_significance;
  float MET_CovXX;
  float MET_CovXY;
  float MET_CovYY;
  h5hep::Collection<Jet> nJet;
  h5hep::Collection<GenPart> nGenPart;
};

#endif // CMS_EVENT_H5_H_
