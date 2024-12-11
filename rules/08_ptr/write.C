#include "Event_v2.hxx"

void write()
{
   gSystem->Load("./libEvent_v2.so");

   auto f = TFile::Open("data.root", "RECREATE");
   auto t = new TTree("t", "");

   Event event;
   t->Branch("event", &event);

   event.fTrack.fPtr = new Track{7};
   t->Fill();

   t->Scan();

   t->Write();
   f->Close();

   f = TFile::Open("data.root");
   t = (TTree *)f->Get("t");

   Event *evPtr = nullptr;
   t->SetBranchAddress("event", &evPtr);

   for (int i = 0; i < t->GetEntries(); ++i) {
      t->GetEntry(i);

      std::cout << evPtr->fTrack.fPtr << std::endl;
      if (evPtr->fTrack.fPtr)
         std::cout << evPtr->fTrack.fPtr->fFoo << std::endl;
   }
}
