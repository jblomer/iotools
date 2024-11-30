#ifdef __ROOTCLING__

#pragma link C++ class Event+;

#pragma read sourceClass = "Event" source = "float fX; float fY" version = "[2]" targetClass = \
   "Event" target = "fR" target = "fPhi" include = "iostream" code = "{ std::cout << offset_Onfile_Event_fX << std::endl; newObj->SetFromEuclidian(onfile.fX, onfile.fY); std::cout << oldObj->fClass->GetClassVersion() << std::endl; }"

#endif

