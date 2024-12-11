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
}
