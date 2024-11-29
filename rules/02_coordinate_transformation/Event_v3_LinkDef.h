#ifdef __ROOTCLING__

#pragma link C++ class Event+;

#pragma read sourceClass = "Event" source = "float fX; float fY" version = "[2]" targetClass = \
   "Event" target = "fR" target = "fPhi" code = "{ newObj->SetFromEuclidian(onfile.fX, onfile.fY); }"

#endif

