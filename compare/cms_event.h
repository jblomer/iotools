#ifndef CMS_EVENT_H_
#define CMS_EVENT_H_

#define MUON_COUNT 13
#define ELECTRON_COUNT 7
#define TAU_COUNT 34
#define JET_COUNT 33
#define GENPART_COUNT 37

struct CmsEvent {
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
  int nMuon;
  float Muon_pt[MUON_COUNT];
  float Muon_eta[MUON_COUNT];
  float Muon_phi[MUON_COUNT];
  float Muon_mass[MUON_COUNT];
  int Muon_charge[MUON_COUNT];
  float Muon_pfRelIso03_all[MUON_COUNT];
  float Muon_pfRelIso04_all[MUON_COUNT];
  bool Muon_tightId[MUON_COUNT];
  bool Muon_softId[MUON_COUNT];
  float Muon_dxy[MUON_COUNT];
  float Muon_dxyErr[MUON_COUNT];
  float Muon_dz[MUON_COUNT];
  float Muon_dzErr[MUON_COUNT];
  int Muon_jetIdx[MUON_COUNT];
  int Muon_genPartIdx[MUON_COUNT];
  int nElectron;
  float Electron_pt[ELECTRON_COUNT];
  float Electron_eta[ELECTRON_COUNT];
  float Electron_phi[ELECTRON_COUNT];
  float Electron_mass[ELECTRON_COUNT];
  int Electron_charge[ELECTRON_COUNT];
  float Electron_pfRelIso03_all[ELECTRON_COUNT];
  float Electron_dxy[ELECTRON_COUNT];
  float Electron_dxyErr[ELECTRON_COUNT];
  float Electron_dz[ELECTRON_COUNT];
  float Electron_dzErr[ELECTRON_COUNT];
  bool Electron_cutBasedId[ELECTRON_COUNT];
  bool Electron_pfId[ELECTRON_COUNT];
  int Electron_jetIdx[ELECTRON_COUNT];
  int Electron_genPartIdx[ELECTRON_COUNT];
  int nTau;
  float Tau_pt[TAU_COUNT];
  float Tau_eta[TAU_COUNT];
  float Tau_phi[TAU_COUNT];
  float Tau_mass[TAU_COUNT];
  int Tau_charge[TAU_COUNT];
  int Tau_decayMode[TAU_COUNT];
  float Tau_relIso_all[TAU_COUNT];
  int Tau_jetIdx[TAU_COUNT];
  int Tau_genPartIdx[TAU_COUNT];
  bool Tau_idDecayMode[TAU_COUNT];
  float Tau_idIsoRaw[TAU_COUNT];
  bool Tau_idIsoVLoose[TAU_COUNT];
  bool Tau_idIsoLoose[TAU_COUNT];
  bool Tau_idIsoMedium[TAU_COUNT];
  bool Tau_idIsoTight[TAU_COUNT];
  bool Tau_idAntiEleLoose[TAU_COUNT];
  bool Tau_idAntiEleMedium[TAU_COUNT];
  bool Tau_idAntiEleTight[TAU_COUNT];
  bool Tau_idAntiMuLoose[TAU_COUNT];
  bool Tau_idAntiMuMedium[TAU_COUNT];
  bool Tau_idAntiMuTight[TAU_COUNT];
  float MET_pt;
  float MET_phi;
  float MET_sumet;
  float MET_significance;
  float MET_CovXX;
  float MET_CovXY;
  float MET_CovYY;
  int nJet;
  float Jet_pt[JET_COUNT];
  float Jet_eta[JET_COUNT];
  float Jet_phi[JET_COUNT];
  float Jet_mass[JET_COUNT];
  bool Jet_puId[JET_COUNT];
  float Jet_btag[JET_COUNT];
  int nGenPart;
  float GenPart_pt[GENPART_COUNT];
  float GenPart_eta[GENPART_COUNT];
  float GenPart_phi[GENPART_COUNT];
  float GenPart_mass[GENPART_COUNT];
  int GenPart_pdgId[GENPART_COUNT];
  int GenPart_status[GENPART_COUNT];
};

#endif // CMS_EVENT_H_
