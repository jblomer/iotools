#ifndef TriggerRecord_H
#define TriggerRecord_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class TriggerRecord {
public:
   class Stream {
   public:
      std::string fStreamName;
      std::vector<std::byte> fData;
   };

   std::string fTRName;
   std::string fFragmentTypeSourceIdMap;
   std::string fRecordHeaderSourceId;
   std::string fSourceIdPathMap;
   std::string fSubdetectorSourceIdMap;

   std::vector<Stream> fStreams;
};

#endif // TriggerRecord_H
