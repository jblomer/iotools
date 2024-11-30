void getOnDiskClass()
{
   gSystem->Load("./libEvent_v3.so");

   auto cl = TClass::GetClass("Event", true, true);
   std::cout << "  ... TClass class version: " << cl->GetClassVersion() << std::endl;

   auto f = TFile::Open("data.root");

   // Get the streamer info directly from TClass:
   auto versionedInfo = cl->GetStreamerInfo(2);
   std::cout << "  ... versioned streamer info class version: " << versionedInfo->GetClassVersion() << std::endl;

   // The versioned streamer info knows about the old _and_ the new layout
   versionedInfo->GetElements()->ls();

   // The first element is "Event@@2       @@alloc"
   cl = versionedInfo->GetElement(0)->GetClassPointer();
   std::cout << "  ... old class version: " << cl->GetClassVersion() << std::endl;

   // The first element is "Event@@2       @@alloc"
   cl = versionedInfo->GetElement(0)->GetClassPointer();
   std::cout << "  ... old class version: " << cl->GetClassVersion() << std::endl;

   // The data member offsets, however, are strange!
   // These are the offsets expected for oldObj->fObject in the schema evolution snippet
   std::cout << "TClass offset of fX and fY: " << cl->GetDataMemberOffset("fX") << " "
                                               << cl->GetDataMemberOffset("fY")<< std::endl;

//   for (auto si : TRangeDynCast<TStreamerInfo>(*f->GetStreamerInfoList())) {
//      if (!si || (std::string(si->GetName()) != "Event"))
//         continue;
//
//      std::cout << "  ... streamer info class version: " << si->GetClassVersion() << std::endl;
//
//      auto versionedInfo = cl->GetStreamerInfo(si->GetClassVersion());
//      std::cout << "  ... versioned streamer info class version: " << versionedInfo->GetClassVersion() << std::endl;
//
//      // The versioned streamer info knows about the old _and_ the new layout
//      versionedInfo->GetElements()->ls();
//
//      // The first element is "Event@@2       @@alloc"
//      cl = versionedInfo->GetElement(0)->GetClassPointer();
//      std::cout << "  ... old class version: " << cl->GetClassVersion() << std::endl;
//
//      // The data member offsets, however, are strange!
//      // These are the offsets expected for oldObj->fObject in the schema evolution snippet
//      std::cout << "TClass offset of fX and fY: " << cl->GetDataMemberOffset("fX") << " "
//                                                  << cl->GetDataMemberOffset("fY")<< std::endl;
//
//      si->ls();
//   }
}
