#ifndef SOA_POINT__
#define SOA_POINT__

#include <ROOT/RVec.hxx>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

// Eventually: the code to be generated
struct SoAPoint {
   // On-disk schema
   struct UnderlyingRecord {
      float fX = 0.0;
      float fY = 0.0;

      ClassDefNV(UnderlyingRecord, 2)
   };

   // A SoA layout compatible with the underlying record
   struct SoALayout {
      ROOT::RVec<float> fX;
      ROOT::RVec<float> fY;

      ClassDefNV(SoALayout, 2)
   };

   // Small helper to allocate a buffer that is used for adoption by the SoA columns
   void InitWithAdoptedMemory(std::size_t N)
   {
      fMemory = std::make_unique<std::byte[]>(N * sizeof(UnderlyingRecord));
      fSoALayout.fX = ROOT::RVec<float>(reinterpret_cast<float *>(fMemory.get()), N);
      fSoALayout.fY = ROOT::RVec<float>(reinterpret_cast<float *>(fMemory.get()) + N, N);
   }

   // The "rectangular" part of the SoA type
   SoALayout fSoALayout;

   // Scalars (if needed)
   std::uint32_t fMyScalar = 0;
   // ...

   // Transient memory buffer for columns
   std::unique_ptr<std::byte[]> fMemory; //!

   ClassDefNV(SoAPoint, 2)
};

#endif // SOA_POINT__
