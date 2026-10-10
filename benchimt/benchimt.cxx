#include <ROOT/RColumnElementBase.hxx>
#include <ROOT/RNTupleTypes.hxx>
#include <ROOT/RNTupleUtils.hxx>
#include <ROOT/RPage.hxx>
#include <ROOT/RPageAllocator.hxx>
#include <ROOT/RPageStorage.hxx>

#include <benchmark/benchmark.h>

#include <memory>

struct RSealing : public benchmark::Fixture {
   std::unique_ptr<ROOT::Internal::RColumnElementBase> fElement;
   ROOT::Internal::RPageAllocatorHeap fAllocator;
   ROOT::Internal::RPage fPayload;
   ROOT::Internal::RPageStorage::RSealedPage fZeroPage;
   ROOT::Internal::RPageStorage::RSealedPage fUncompressedPage;
   std::unique_ptr<unsigned char[]> fUncompressedBuffer;
   ROOT::Internal::RPageStorage::RSealedPage f505Page;
   std::unique_ptr<unsigned char[]> f505Buffer;
   ROOT::Internal::RPageStorage::RSealedPage f509Page;
   std::unique_ptr<unsigned char[]> f509Buffer;

   // Avoid GCC warning
   using benchmark::Fixture::SetUp;
   void SetUp(benchmark::State &state) final
   {
      fElement = ROOT::Internal::RColumnElementBase::Generate(ROOT::ENTupleColumnType::kSplitReal32);
      const auto nElements = state.range(0) / fElement->GetSize();

      fZeroPage.SetBuffer(ROOT::Internal::RPage::GetPageZeroBuffer());
      fZeroPage.SetBufferSize(ROOT::Internal::RPage::kPageZeroSize);
      fZeroPage.SetNElements(nElements);

      fPayload = fAllocator.NewPage(fElement->GetSize(), nElements);
      fPayload.GrowUnchecked(nElements);
      for (unsigned int i = 0; i < nElements; ++i) {
         *(static_cast<float *>(fPayload.GetBuffer()) + i) = i;
      }

      fUncompressedBuffer = ROOT::Internal::MakeUninitArray<unsigned char>(
         state.range(0) + ROOT::Internal::RPageStorage::kNBytesPageChecksum);
      ROOT::Internal::RPageSink::RSealPageConfig config;
      config.fPage = &fPayload;
      config.fElement = fElement.get();
      config.fBuffer = fUncompressedBuffer.get();
      fUncompressedPage = ROOT::Internal::RPageSink::SealPage(config);

      f505Buffer = ROOT::Internal::MakeUninitArray<unsigned char>(
         state.range(0) + ROOT::Internal::RPageStorage::kNBytesPageChecksum);
      config.fPage = &fPayload;
      config.fElement = fElement.get();
      config.fBuffer = f505Buffer.get();
      config.fCompressionSettings = 505;
      f505Page = ROOT::Internal::RPageSink::SealPage(config);

      f509Buffer = ROOT::Internal::MakeUninitArray<unsigned char>(
         state.range(0) + ROOT::Internal::RPageStorage::kNBytesPageChecksum);
      config.fPage = &fPayload;
      config.fElement = fElement.get();
      config.fBuffer = f509Buffer.get();
      config.fCompressionSettings = 509;
      f509Page = ROOT::Internal::RPageSink::SealPage(config);
   }
};

BENCHMARK_DEFINE_F(RSealing, Zero)(benchmark::State &state)
{
   for (auto _ : state) {
      benchmark::DoNotOptimize(ROOT::Internal::RPageSource::UnsealPage(fZeroPage, *fElement, fAllocator));
   }
}
BENCHMARK_REGISTER_F(RSealing, Zero)->Range(sizeof(float), ROOT::Internal::RPage::kPageZeroSize);

BENCHMARK_DEFINE_F(RSealing, Uncompressed)(benchmark::State &state)
{
   for (auto _ : state) {
      benchmark::DoNotOptimize(ROOT::Internal::RPageSource::UnsealPage(fUncompressedPage, *fElement, fAllocator));
   }
}
BENCHMARK_REGISTER_F(RSealing, Uncompressed)->Range(sizeof(float), 1024 * 1024);

BENCHMARK_DEFINE_F(RSealing, C505)(benchmark::State &state)
{
   for (auto _ : state) {
      benchmark::DoNotOptimize(ROOT::Internal::RPageSource::UnsealPage(f505Page, *fElement, fAllocator));
   }
}
BENCHMARK_REGISTER_F(RSealing, C505)->Range(sizeof(float), 1024 * 1024);

BENCHMARK_DEFINE_F(RSealing, C509)(benchmark::State &state)
{
   for (auto _ : state) {
      benchmark::DoNotOptimize(ROOT::Internal::RPageSource::UnsealPage(f509Page, *fElement, fAllocator));
   }
}
BENCHMARK_REGISTER_F(RSealing, C509)->Range(sizeof(float), 1024 * 1024);

BENCHMARK_MAIN();
