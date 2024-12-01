#ifdef __ROOTCLING__

#pragma link C++ class Event+;

#pragma read sourceClass = "Event" source = "std::string fProperties;" version = "[2]" targetClass = \
   "Event" target = "fProperties" code = "{ newObj->fProperties = onfile.fProperties == \"x\" ? 1 : 0; }"

#endif
