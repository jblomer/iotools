#ifdef __ROOTCLING__

#pragma link C++ class Base+;
#pragma link C++ class Event+;

// Correct to not give a target??

#pragma read sourceClass = "Event" source = "int fBadDerivedName;" version = "[2]" targetClass = \
   "Event" target = "" code = "{ newObj->fBetterBaseName = onfile.fBadDerivedName; }"

#endif
