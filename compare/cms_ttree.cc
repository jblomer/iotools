#include "cms_ttree.h"

#include <type_traits>
#include <vector>

#include <TChain.h>

void EventReaderRoot::Open(const std::string &path) {
  fChain = new TChain("Events");
  fChain->Add(path.c_str());

  fChain->SetBranchAddress("run", &fEvent.run, &fBranch[0]);
  fChain->SetBranchAddress("luminosityBlock", &fEvent.luminosityBlock, &fBranch[1]);
  fChain->SetBranchAddress("event", &fEvent.event, &fBranch[2]);
  fChain->SetBranchAddress("HLT_IsoMu24_eta2p1", &fEvent.HLT_IsoMu24_eta2p1, &fBranch[3]);
  fChain->SetBranchAddress("HLT_IsoMu24", &fEvent.HLT_IsoMu24, &fBranch[4]);
  fChain->SetBranchAddress("HLT_IsoMu17_eta2p1_LooseIsoPFTau20", &fEvent.HLT_IsoMu17_eta2p1_LooseIsoPFTau20, &fBranch[5]);
  fChain->SetBranchAddress("PV_npvs", &fEvent.PV_npvs, &fBranch[6]);
  fChain->SetBranchAddress("PV_x", &fEvent.PV_x, &fBranch[7]);
  fChain->SetBranchAddress("PV_y", &fEvent.PV_y, &fBranch[8]);
  fChain->SetBranchAddress("PV_z", &fEvent.PV_z, &fBranch[9]);
  fChain->SetBranchAddress("nMuon", &fEvent.nMuon, &fBranch[10]);
  fChain->SetBranchAddress("Muon_pt", &fEvent.Muon_pt, &fBranch[11]);
  fChain->SetBranchAddress("Muon_eta", &fEvent.Muon_eta, &fBranch[12]);
  fChain->SetBranchAddress("Muon_phi", &fEvent.Muon_phi, &fBranch[13]);
  fChain->SetBranchAddress("Muon_mass", &fEvent.Muon_mass, &fBranch[14]);
  fChain->SetBranchAddress("Muon_charge", &fEvent.Muon_charge, &fBranch[15]);
  fChain->SetBranchAddress("Muon_pfRelIso03_all", &fEvent.Muon_pfRelIso03_all, &fBranch[16]);
  fChain->SetBranchAddress("Muon_pfRelIso04_all", &fEvent.Muon_pfRelIso04_all, &fBranch[17]);
  fChain->SetBranchAddress("Muon_tightId", &fEvent.Muon_tightId, &fBranch[18]);
  fChain->SetBranchAddress("Muon_softId", &fEvent.Muon_softId, &fBranch[19]);
  fChain->SetBranchAddress("Muon_dxy", &fEvent.Muon_dxy, &fBranch[20]);
  fChain->SetBranchAddress("Muon_dxyErr", &fEvent.Muon_dxyErr, &fBranch[21]);
  fChain->SetBranchAddress("Muon_dz", &fEvent.Muon_dz, &fBranch[22]);
  fChain->SetBranchAddress("Muon_dzErr", &fEvent.Muon_dzErr, &fBranch[23]);
  fChain->SetBranchAddress("Muon_jetIdx", &fEvent.Muon_jetIdx, &fBranch[24]);
  fChain->SetBranchAddress("Muon_genPartIdx", &fEvent.Muon_genPartIdx, &fBranch[25]);
  fChain->SetBranchAddress("nElectron", &fEvent.nElectron, &fBranch[26]);
  fChain->SetBranchAddress("Electron_pt", &fEvent.Electron_pt, &fBranch[27]);
  fChain->SetBranchAddress("Electron_eta", &fEvent.Electron_eta, &fBranch[28]);
  fChain->SetBranchAddress("Electron_phi", &fEvent.Electron_phi, &fBranch[29]);
  fChain->SetBranchAddress("Electron_mass", &fEvent.Electron_mass, &fBranch[30]);
  fChain->SetBranchAddress("Electron_charge", &fEvent.Electron_charge, &fBranch[31]);
  fChain->SetBranchAddress("Electron_pfRelIso03_all", &fEvent.Electron_pfRelIso03_all, &fBranch[32]);
  fChain->SetBranchAddress("Electron_dxy", &fEvent.Electron_dxy, &fBranch[33]);
  fChain->SetBranchAddress("Electron_dxyErr", &fEvent.Electron_dxyErr, &fBranch[34]);
  fChain->SetBranchAddress("Electron_dz", &fEvent.Electron_dz, &fBranch[35]);
  fChain->SetBranchAddress("Electron_dzErr", &fEvent.Electron_dzErr, &fBranch[36]);
  fChain->SetBranchAddress("Electron_cutBasedId", &fEvent.Electron_cutBasedId, &fBranch[37]);
  fChain->SetBranchAddress("Electron_pfId", &fEvent.Electron_pfId, &fBranch[38]);
  fChain->SetBranchAddress("Electron_jetIdx", &fEvent.Electron_jetIdx, &fBranch[39]);
  fChain->SetBranchAddress("Electron_genPartIdx", &fEvent.Electron_genPartIdx, &fBranch[40]);
  fChain->SetBranchAddress("nTau", &fEvent.nTau, &fBranch[41]);
  fChain->SetBranchAddress("Tau_pt", &fEvent.Tau_pt, &fBranch[42]);
  fChain->SetBranchAddress("Tau_eta", &fEvent.Tau_eta, &fBranch[43]);
  fChain->SetBranchAddress("Tau_phi", &fEvent.Tau_phi, &fBranch[44]);
  fChain->SetBranchAddress("Tau_mass", &fEvent.Tau_mass, &fBranch[45]);
  fChain->SetBranchAddress("Tau_charge", &fEvent.Tau_charge, &fBranch[46]);
  fChain->SetBranchAddress("Tau_decayMode", &fEvent.Tau_decayMode, &fBranch[47]);
  fChain->SetBranchAddress("Tau_relIso_all", &fEvent.Tau_relIso_all, &fBranch[48]);
  fChain->SetBranchAddress("Tau_jetIdx", &fEvent.Tau_jetIdx, &fBranch[49]);
  fChain->SetBranchAddress("Tau_genPartIdx", &fEvent.Tau_genPartIdx, &fBranch[50]);
  fChain->SetBranchAddress("Tau_idDecayMode", &fEvent.Tau_idDecayMode, &fBranch[51]);
  fChain->SetBranchAddress("Tau_idIsoRaw", &fEvent.Tau_idIsoRaw, &fBranch[52]);
  fChain->SetBranchAddress("Tau_idIsoVLoose", &fEvent.Tau_idIsoVLoose, &fBranch[53]);
  fChain->SetBranchAddress("Tau_idIsoLoose", &fEvent.Tau_idIsoLoose, &fBranch[54]);
  fChain->SetBranchAddress("Tau_idIsoMedium", &fEvent.Tau_idIsoMedium, &fBranch[55]);
  fChain->SetBranchAddress("Tau_idIsoTight", &fEvent.Tau_idIsoTight, &fBranch[56]);
  fChain->SetBranchAddress("Tau_idAntiEleLoose", &fEvent.Tau_idAntiEleLoose, &fBranch[57]);
  fChain->SetBranchAddress("Tau_idAntiEleMedium", &fEvent.Tau_idAntiEleMedium, &fBranch[58]);
  fChain->SetBranchAddress("Tau_idAntiEleTight", &fEvent.Tau_idAntiEleTight, &fBranch[59]);
  fChain->SetBranchAddress("Tau_idAntiMuLoose", &fEvent.Tau_idAntiMuLoose, &fBranch[60]);
  fChain->SetBranchAddress("Tau_idAntiMuMedium", &fEvent.Tau_idAntiMuMedium, &fBranch[61]);
  fChain->SetBranchAddress("Tau_idAntiMuTight", &fEvent.Tau_idAntiMuTight, &fBranch[62]);
  fChain->SetBranchAddress("MET_pt", &fEvent.MET_pt, &fBranch[63]);
  fChain->SetBranchAddress("MET_phi", &fEvent.MET_phi, &fBranch[64]);
  fChain->SetBranchAddress("MET_sumet", &fEvent.MET_sumet, &fBranch[65]);
  fChain->SetBranchAddress("MET_significance", &fEvent.MET_significance, &fBranch[66]);
  fChain->SetBranchAddress("MET_CovXX", &fEvent.MET_CovXX, &fBranch[67]);
  fChain->SetBranchAddress("MET_CovXY", &fEvent.MET_CovXY, &fBranch[68]);
  fChain->SetBranchAddress("MET_CovYY", &fEvent.MET_CovYY, &fBranch[69]);
  fChain->SetBranchAddress("nJet", &fEvent.nJet, &fBranch[70]);
  fChain->SetBranchAddress("Jet_pt", &fEvent.Jet_pt, &fBranch[71]);
  fChain->SetBranchAddress("Jet_eta", &fEvent.Jet_eta, &fBranch[72]);
  fChain->SetBranchAddress("Jet_phi", &fEvent.Jet_phi, &fBranch[73]);
  fChain->SetBranchAddress("Jet_mass", &fEvent.Jet_mass, &fBranch[74]);
  fChain->SetBranchAddress("Jet_puId", &fEvent.Jet_puId, &fBranch[75]);
  fChain->SetBranchAddress("Jet_btag", &fEvent.Jet_btag, &fBranch[76]);
  fChain->SetBranchAddress("nGenPart", &fEvent.nGenPart, &fBranch[77]);
  fChain->SetBranchAddress("GenPart_pt", &fEvent.GenPart_pt, &fBranch[78]);
  fChain->SetBranchAddress("GenPart_eta", &fEvent.GenPart_eta, &fBranch[79]);
  fChain->SetBranchAddress("GenPart_phi", &fEvent.GenPart_phi, &fBranch[80]);
  fChain->SetBranchAddress("GenPart_mass", &fEvent.GenPart_mass, &fBranch[81]);
  fChain->SetBranchAddress("GenPart_pdgId", &fEvent.GenPart_pdgId, &fBranch[82]);
  fChain->SetBranchAddress("GenPart_status", &fEvent.GenPart_status, &fBranch[83]);

  fNEvents = fChain->GetEntries();
  fPos = 0;
}

bool EventReaderRoot::NextEvent() {
  if (fPos >= fNEvents)
    return false;

  for (size_t i = 0; i < std::extent<decltype(fBranch)>::value; ++i)
    fBranch[i]->GetEntry(fPos);

  fPos++;
  return true;
}
