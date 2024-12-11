#ifdef __ROOTCLING__

#pragma link C++ class Event+;

#pragma read sourceClass = "Event" source = "float fX; float fY" version = "[2]" targetClass = \
   "Event" target = "fPhi,fR" include = "iostream" code = "{ newObj->SetFromEuclidian(onfile.fX, onfile.fY); std::cout << oldObj->fClass->GetClassVersion() << std::endl; }"

#endif

