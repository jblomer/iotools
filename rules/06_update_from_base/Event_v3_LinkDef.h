#ifdef __ROOTCLING__

#pragma link C++ class Base+;
#pragma link C++ class Event+;

#pragma read sourceClass = "Event" source = "int fBadBaseName;" version = "[2]" targetClass = \
   "Event" target = "fBetterDerivedName" code = "{ newObj->fBetterDerivedName = onfile.fBadBaseName; }"

#endif
