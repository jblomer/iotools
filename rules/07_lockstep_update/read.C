#include "Event_v3.hxx"

void read()
{
   gSystem->Load("./libEvent_v3.so");

   auto f = TFile::Open("data.root");
   auto t = f->Get<TTree>("t");

   Event *event = nullptr;
   t->SetBranchAddress("event", &event);

   for (int i = 0; i < t->GetEntries(); ++i) {
      t->GetEntry(i);

      // WRONG RESULT
      std::cout << event->fHasTrackOfPropertyX << std::endl;
   }
}
