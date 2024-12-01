#include "Event_v2.hxx"

void write()
{
   gSystem->Load("./libEvent_v2.so");

   auto f = TFile::Open("data.root", "RECREATE");
   auto t = new TTree("t", "");

   Event event;
   t->Branch("event", &event);

   event.fId = 5;
   event.fTrack.fPt = 1.0;
   event.fTrack.fPt = 5;
   t->Fill();

   t->Scan();

   t->Write();
   f->Close();
}
