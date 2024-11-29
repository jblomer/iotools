#ifdef __ROOTCLING__

#pragma link C++ class Event+;

#pragma read sourceClass = "Event" source = "float fTemperature" version = "[2]" targetClass = \
   "Event" target = "fTemperature" code = "{ fTemperature = onfile.fTemperature - 273.15; }"

#endif
