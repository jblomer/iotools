#include "Event_v2.hxx"

void write()
{
   gSystem->Load("./libEvent_v2.so");

   auto f = TFile::Open("data.root", "RECREATE");
   auto t = new TTree("t", "");

   Event event;
   t->Branch("event", &event);

   event.fX = 1.0;
   event.fY = 2.0;
   t->Fill();

   t->Scan();

   t->Write();
   f->Close();
}
