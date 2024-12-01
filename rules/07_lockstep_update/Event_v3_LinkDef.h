#ifdef __ROOTCLING__

#pragma link C++ class Track+;
#pragma link C++ class Event+;

#pragma read sourceClass = "Track" source = "std::string fProperties;" version = "[2]" targetClass = \
   "Track" target = "fProperties" code = "{ newObj->fProperties = onfile.fProperties == \"x\" ? 1 : 0; }"

#pragma read sourceClass = "Event" source = "Track fTrack;" version = "[2]" targetClass = \
   "Event" target = "fHasTrackOfPropertyX" code = "{ newObj->fHasTrackOfPropertyX = onfile.fTrack.fProperties == 1; }"

#endif
