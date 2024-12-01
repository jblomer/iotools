#ifdef __ROOTCLING__

#pragma link C++ class Track+;
#pragma link C++ class Event+;

#pragma read sourceClass = "Event" source = "int fId; Track fTrack;" version = "[2]" targetClass = \
   "Event" target = "fNewId" target = "fTrack" code = "{ newObj->fNewId = onfile.fId + 1000; \
      newObj->fTrack = onfile.fTrack; newObj->fTrack.fId = newObj->fNewId; }"

#endif
