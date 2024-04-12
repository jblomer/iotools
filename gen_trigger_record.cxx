#include <cassert>
#include <cstdio>
#include <string>

static void Usage() {
   printf("gen_trigger_record <output.hxx>\n");
}

static constexpr int kNStreamsPerUnit = 40;
static constexpr int kNUnits = 150;
static constexpr int kNTriggers = 10;
static constexpr int kNHWSignalsInterfaces = 10;
static constexpr int kNTRBuilders = 10;

int main(int argc, char **argv)
{
   if (argc < 2 || std::string(argv[1]) == "-h") {
      Usage();
      return 0;
   }

   FILE *f = fopen(argv[1], "w+");
   assert(f);

   fprintf(f,
"#ifndef TriggerRecord_H\n"
"#define TriggerRecord_H\n"
"\n"
"// THIS FILE IS GENERATED!\n"
"\n"
"#include <Rtypes.h>\n"
"\n"
"#include <cstddef>\n"
"#include <cstdint>\n"
"#include <string>\n"
"#include <vector>\n"
"\n"
"class TriggerRecord {\n"
"public:\n"
"   static constexpr int kNStreamsPerUnit = %d;\n"
"   static constexpr int kNUnits = %d;\n"
"   static constexpr int kNTriggers = %d;\n"
"   static constexpr int kNHWSignalsInterfaces = %d;\n"
"   static constexpr int kNTRBuilders = %d;\n"
"\n"
"   enum class EDataType {\n"
"      kWIBEth,\n"
"      kDAPHNEStream,\n"
"      kTriggerRecordHeader,\n"
"      kTriggerCandidate,\n"
"      kTriggerActivity,\n"
"      kTriggerPrimitive,\n"
"      kHardwareSignal\n"
"   };\n"
"\n"
"   struct Stream {\n"
"      EDataType fDataType;\n"
"      std::vector<std::byte> fData;\n"
"   };\n"
"\n"
"   struct DetectorUnit {\n",
   kNStreamsPerUnit, kNUnits, kNTriggers, kNHWSignalsInterfaces, kNTRBuilders);

   for (unsigned i = 0; i < kNStreamsPerUnit; ++i) {
      fprintf(f,
"      Stream fStream%d;\n",
              i);
   };

   fprintf(f,
"   };\n"
"\n"
"   struct Detectors {\n"
   );

   for (unsigned i = 0; i < kNUnits; ++i) {
      fprintf(f,
"      DetectorUnit fUnit%d;\n",
              i);
   };

   fprintf(f,
"   };\n"
"\n"
"   struct HWSignalsInterfaces {\n"
   );

   for (unsigned i = 0; i < kNHWSignalsInterfaces; ++i) {
      fprintf(f,
"      Stream fHWSignalStream%d;\n",
              i);
   };

   fprintf(f,
"   };\n"
"\n"
"   struct Triggers {\n"
   );

   for (unsigned i = 0; i < kNTriggers; ++i) {
      fprintf(f,
"      Stream fTriggerStream%d;\n",
              i);
   };

   fprintf(f,
"   };\n"
"   struct TRBuilders {\n"
   );

   for (unsigned i = 0; i < kNTRBuilders; ++i) {
      fprintf(f,
"      Stream fTRBuilderStream%d;\n",
              i);
   };

   fprintf(f,
"   };\n"
"\n"
"   std::uint64_t fTRID;\n"
"   std::uint64_t fSliceID;\n"
"   std::string fFragmentTypeSourceIdMap;\n"
"   std::string fRecordHeaderSourceId;\n"
"   std::string fSourceIdPathMap;\n"
"   std::string fSubdetectorSourceIdMap;\n"
"\n"
"   Detectors fDetectors;\n"
"   HWSignalsInterfaces fHWInterfaces;\n"
"   Triggers fTriggers;\n"
"   TRBuilders fTRBuilders;\n"
"\n"
"   Stream *GetReadoutStream(int detId, int streamId)\n"
"   {\n"
"      static bool kIsInitialized = false;\n"
"      static size_t kStreamOffsets[%d][%d];\n"
"      if (!kIsInitialized) {\n",
   kNUnits, kNStreamsPerUnit);

   for (unsigned i = 0; i < kNUnits; i++) {
      for (unsigned j = 0; j < kNStreamsPerUnit; j++) {
         fprintf(f,
"         kStreamOffsets[%d][%d] = offsetof(TriggerRecord, fDetectors.fUnit%d.fStream%d);\n",
         i, j, i, j);
      }
   }

   fprintf(f,
"         kIsInitialized = true;\n"
"      }\n"
"      return reinterpret_cast<Stream *>(reinterpret_cast<unsigned char *>(this) + kStreamOffsets[detId][streamId]);\n"
"   }\n"
"\n"
"   Stream *GetHWSignalsInterfaceStream(int id)\n"
"   {\n"
"      static bool kIsInitialized = false;\n"
"      static size_t kStreamOffsets[%d];\n"
"      if (!kIsInitialized) {\n",
   kNHWSignalsInterfaces);

   for (unsigned i = 0; i < kNHWSignalsInterfaces; i++) {
      fprintf(f,
"         kStreamOffsets[%d] = offsetof(TriggerRecord, fHWInterfaces.fHWSignalStream%d);\n",
      i, i);
   }

   fprintf(f,
"         kIsInitialized = true;\n"
"      }\n"
"      return reinterpret_cast<Stream *>(reinterpret_cast<unsigned char *>(this) + kStreamOffsets[id]);\n"
"   }\n"
"\n"
"   Stream *GetTRBuilderStream(int id)\n"
"   {\n"
"      static bool kIsInitialized = false;\n"
"      static size_t kStreamOffsets[%d];\n"
"      if (!kIsInitialized) {\n",
   kNTRBuilders);

   for (unsigned i = 0; i < kNTRBuilders; i++) {
      fprintf(f,
"         kStreamOffsets[%d] = offsetof(TriggerRecord, fTRBuilders.fTRBuilderStream%d);\n",
      i, i);
   }

   fprintf(f,
"         kIsInitialized = true;\n"
"      }\n"
"      return reinterpret_cast<Stream *>(reinterpret_cast<unsigned char *>(this) + kStreamOffsets[id]);\n"
"   }\n"
"\n"
"   Stream *GetTriggerStream(int id)\n"
"   {\n"
"      static bool kIsInitialized = false;\n"
"      static size_t kStreamOffsets[%d];\n"
"      if (!kIsInitialized) {\n",
   kNTriggers);

   for (unsigned i = 0; i < kNTriggers; i++) {
      fprintf(f,
"         kStreamOffsets[%d] = offsetof(TriggerRecord, fTriggers.fTriggerStream%d);\n",
      i, i);
   }

   fprintf(f,
"         kIsInitialized = true;\n"
"      }\n"
"      return reinterpret_cast<Stream *>(reinterpret_cast<unsigned char *>(this) + kStreamOffsets[id]);\n"
"   }\n"
"\n"
"   ClassDefNV(TriggerRecord, 1)\n"
"};\n"
"\n"
"#endif // TriggerRecord_H\n"
   );

   fclose(f);
   return 0;
}
