#ifdef __ROOTCLING__

#pragma link C++ class Track+;
#pragma link C++ class PtrWrapper+;
#pragma link C++ class Event+;

#pragma read sourceClass = "Event" source = "PtrWrapper fTrack;" version = "[2]" \
   targetClass = "Event" target = "fTrack" code = "{ fTrack = onfile.fTrack.fPtr; onfile.fTrack.fPtr = nullptr; }"

#endif
