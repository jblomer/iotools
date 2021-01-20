#include <ROOT/RNTupleMetrics.hxx>
#include <ROOT/RNTupleZip.hxx>

#include <TApplication.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1F.h>
#include <TLegend.h>
#include <TPaveStats.h>
#include <TRandom.h>
#include <TStyle.h>
#include <TSystem.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <future>
#include <limits>
#include <string>

#include <unistd.h>

using RNTupleAtomicCounter = ROOT::Experimental::Detail::RNTupleAtomicCounter;
using RNTupleAtomicTimer = ROOT::Experimental::Detail::RNTupleAtomicTimer;
using RNTupleCompressor = ROOT::Experimental::Detail::RNTupleCompressor;
using RNTupleDecompressor = ROOT::Experimental::Detail::RNTupleDecompressor;
using RNTupleMetrics = ROOT::Experimental::Detail::RNTupleMetrics;
template<class T>
using RNTupleTickCounter = ROOT::Experimental::Detail::RNTupleTickCounter<T>;

class ClockHist {
  std::string fName;
  TH1D *fHWall;
  TH1D *fHCpu;
  std::int64_t fMinWall = std::numeric_limits<std::int64_t>::max();
  std::int64_t fMaxWall = std::numeric_limits<std::int64_t>::min();
  std::int64_t fSumWall = 0;
  double fMinCpu = std::numeric_limits<double>::max();
  double fMaxCpu = std::numeric_limits<double>::lowest();
  double fSumCpu = 0.0;
  std::uint64_t fTotal = 0;

public:
  ClockHist(const std::string &name, std::int64_t min, std::int64_t max)
    : fName(name)
  {
    fHWall = new TH1D((fName + " Wall").c_str(), "", 250, min, max);
    fHCpu = new TH1D((fName + " CPU").c_str(), "", 250, min, max);
  }

  void Fill(std::int64_t wall_ns, double cpu_ns) {
    fMinWall = std::min(fMinWall, wall_ns);
    fMaxWall = std::max(fMaxWall, wall_ns);
    fMinCpu = std::min(fMinCpu, cpu_ns);
    fMaxCpu = std::max(fMaxCpu, cpu_ns);
    fSumWall += wall_ns;
    fSumCpu += cpu_ns;
    fHWall->Fill(wall_ns);
    fHCpu->Fill(cpu_ns);
    fTotal++;
  }

  void Draw() {
    fHWall->SetMaximum(fTotal * 2);

    fHWall->SetLineColor(kBlue);
    fHWall->SetFillColor(kBlue);
    fHWall->SetFillStyle(3001);
    fHWall->GetXaxis()->SetTitle("Duration (ns)");
    fHWall->Draw();

    fHCpu->SetLineColor(kRed);
    fHCpu->SetFillColor(kRed);
    fHCpu->SetFillStyle(3001);
    fHCpu->Draw("SAMES");

    gPad->Update();
    auto *st = (TPaveStats*)fHCpu->FindObject("stats");
    st->SetY1NDC(st->GetY1NDC() - 0.3);
    st->SetY2NDC(st->GetY2NDC() - 0.3);

    auto *l = new TLegend();
    l->AddEntry(fHWall, "Wall time", "f");
    l->AddEntry(fHCpu, "CPU time", "f");
    l->SetTextSize(0.04);
    l->Draw();

    printf("%-16s Wall[%ld - %ld] Sum = %ld       CPU[%lf - %lf] Sum = %lf ms\n",
           fName.c_str(), fMinWall, fMaxWall, fSumWall, fMinCpu, fMaxCpu,
           fSumCpu / (1000. * 1000.));
  }

  void Write() {
    fHWall->Write();
    fHCpu->Write();
  }
};


class ClockHistRAII {
  ClockHist &fClockHist;
  RNTupleAtomicCounter &fCtrWall;
  RNTupleTickCounter<RNTupleAtomicCounter> &fCtrCpu;

  std::int64_t fValWall;
  std::int64_t fValCpu;

public:
  ClockHistRAII(ClockHist &h, RNTupleAtomicCounter &w, RNTupleTickCounter<RNTupleAtomicCounter> &c)
    : fClockHist(h)
    , fCtrWall(w)
    , fCtrCpu(c)
  {
    fValWall = fCtrWall.GetValue();
    fValCpu = fCtrCpu.GetValue();
  }

  ~ClockHistRAII() {
    auto ticks = fCtrCpu.GetValue() - fValCpu;
    double cpuNanosec = (double(ticks) / double(CLOCKS_PER_SEC)) * (1000. * 1000. * 1000.);
    fClockHist.Fill(fCtrWall.GetValue() - fValWall, cpuNanosec);
  }
};

ClockHist gHistNop("no-op", 0, 1200);                              // [0 - 1.2us]
ClockHist gHistSin100("100X sine", 0, 10000);                      // [0 - 10us]
ClockHist gHistUnzip10k("10kB u-zstd", 8000, 64000);               // [8 - 64us]
ClockHist gHistUnzip10k100X("10kB u-zstd X100", 800000, 3200000);  // [800us - 32ms]



static double Compute(double seed, int iterations) {
  double r = seed;
  for (int i = 0; i < iterations; ++i) {
    r = sin(r);
  }
  return r;
}

static void ClobberMemory() {
  asm volatile("" : : : "memory");
}

static void Show() {
  new TApplication("", nullptr, nullptr);
  gStyle->SetTextFont(42);
  gStyle->SetOptStat(111111);

  auto c = new TCanvas("No-op", "", 800, 700);
  c->SetLogy();
  gHistNop.Draw();
  c->Modified();

  c = new TCanvas("100x Sine", "", 800, 700);
  c->SetLogy();
  gHistSin100.Draw();
  c->Modified();

  c = new TCanvas("10kB Unzip (zstd)", "", 800, 700);
  c->SetLogy();
  gHistUnzip10k.Draw();
  c->Modified();

  c = new TCanvas("Sum over 100x 10kB Unzip (zstd)", "", 800, 700);
  c->SetLogy();
  gHistUnzip10k100X.Draw();
  c->Modified();

  printf("press ENTER to exit...\n");
  auto future = std::async(std::launch::async, getchar);
  while (true) {
    gSystem->ProcessEvents();
    if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
       break;
  }
}


static void Usage(const char *progname) {
  printf("%s [-s random seed] [-o output file] [-b <block size in kB>] "
         "[-i(identical block for decompression)] [-s(how)]\n",
         progname);
}

int main(int argc, char **argv) {
  bool show = false;
  bool use_identical_block = false;
  std::string output = "clock.root";
  double seed = 42.0;
  int blockSizeKB = 10;
  int c;
  while ((c = getopt(argc, argv, "hvr:o:b:is")) != -1) {
    switch (c) {
    case 'h':
    case 'v':
      Usage(argv[0]);
      return 0;
    case 'r':
      seed = atof(optarg);
      break;
    case 'o':
      output = optarg;
      break;
    case 'i':
      use_identical_block = true;
      break;
    case 'b':
      blockSizeKB = atoi(optarg);
      break;
    case 's':
      show = true;
      break;
    default:
      fprintf(stderr, "Unknown option: -%c\n", c);
      Usage(argv[0]);
      return 1;
    }
  }

  printf("Clock information: clock() = %ld    CLOCKS_PER_SEC = %ld\n", clock(), CLOCKS_PER_SEC);

  // Prepare compressed blocks
  gRandom->SetSeed(seed);
  RNTupleCompressor compressor;
  constexpr int kNumBlocks = 1000;
  constexpr int kNumValsPerBlock = 10000 / sizeof(float);
  float *blocks[kNumBlocks];
  std::uint32_t blockSizes[kNumBlocks];
  for (int i = 0; i < kNumBlocks; ++i) {
    blocks[i] = new float[kNumValsPerBlock];
    for (int v = 0; v < kNumValsPerBlock; ++v) {
      blocks[i][v] = gRandom->Gaus();
    }
    blockSizes[i] = compressor(blocks[i], 10000, 505);
    memcpy(blocks[i], compressor.GetZipBuffer(), blockSizes[i]);
    //printf("new block: %d\n", blockSizes[i]);
  }
  printf("Compressed memory blocks ready\n");

  RNTupleMetrics metrics("metrics");
  auto ctrWallNop = metrics.MakeCounter<RNTupleAtomicCounter*>("timeWallNop", "ns", "Wall time of a noop");
  auto ctrCpuNop = metrics.MakeCounter<ROOT::Experimental::Detail::RNTupleTickCounter<RNTupleAtomicCounter>*>(
    "timeCpuNop", "ns", "CPU time of a noop");
  auto ctrWallSin100 = metrics.MakeCounter<RNTupleAtomicCounter*>("timeWallSin100", "ns", "Wall time of 100x sine");
  auto ctrCpuSin100 = metrics.MakeCounter<ROOT::Experimental::Detail::RNTupleTickCounter<RNTupleAtomicCounter>*>(
    "timeCpuSin100", "ns", "CPU time of 100x sine");
  auto ctrWallUnzip10k = metrics.MakeCounter<RNTupleAtomicCounter*>(
    "timeWallUnzip10k", "ns", "Wall time of unzipping 10kB with zstd");
  auto ctrCpuUnzip10k = metrics.MakeCounter<ROOT::Experimental::Detail::RNTupleTickCounter<RNTupleAtomicCounter>*>(
    "timeCpuUnzip10k", "ns", "CPU time of unzipping 10kB with zstd");
  metrics.Enable();

  // No-op
  for (unsigned i = 0; i < 1000000; ++i) {
    {
      ClockHistRAII t(gHistNop, *ctrWallNop, *ctrCpuNop);
      {
        RNTupleAtomicTimer timer(*ctrWallNop, *ctrCpuNop);
      }
    }
    ClobberMemory();
  }
  printf("No-op done\n");

  // 100 times sine
  double sine = seed;
  for (unsigned i = 0; i < 1000000; ++i) {
    {
      ClockHistRAII t(gHistSin100, *ctrWallSin100, *ctrCpuSin100);
      {
        RNTupleAtomicTimer timer(*ctrWallSin100, *ctrCpuSin100);
        sine = Compute(sine, 100);
      }
    }
    ClobberMemory();
  }
  printf("100x sine result: %lf\n", sine);

  // Decompress 10kB
  float dummy = 0.0;
  RNTupleDecompressor decompressor;
  float dest[kNumValsPerBlock];
  int blockIdx = gRandom->Uniform(kNumBlocks - 2) + 1;
  for (unsigned i = 0; i < 1000000; ++i) {
    if (!use_identical_block)
      blockIdx = gRandom->Uniform(kNumBlocks - 2) + 1;
    {
      ClockHistRAII t(gHistUnzip10k, *ctrWallUnzip10k, *ctrCpuUnzip10k);
      {
        RNTupleAtomicTimer timer(*ctrWallUnzip10k, *ctrCpuUnzip10k);
        decompressor(blocks[blockIdx], blockSizes[blockIdx], kNumValsPerBlock * sizeof(float), dest);
      }
    }
    dummy += dest[int(gRandom->Uniform(kNumValsPerBlock - 2) + 1)];

    ClobberMemory();
  }
  printf("Decompression dummy result: %f\n", dummy);

  // Decompress 10kB: sum over 100 runs
  for (unsigned i = 0; i < 10000; ++i) {
    ClockHistRAII t(gHistUnzip10k100X, *ctrWallUnzip10k, *ctrCpuUnzip10k);
    for (unsigned j = 0; j < 100; ++j) {
      if (!use_identical_block)
        blockIdx = gRandom->Uniform(kNumBlocks - 2) + 1;
      {
        RNTupleAtomicTimer timer(*ctrWallUnzip10k, *ctrCpuUnzip10k);
        decompressor(blocks[blockIdx], blockSizes[blockIdx], kNumValsPerBlock * sizeof(float), dest);
      }
      dummy += dest[int(gRandom->Uniform(kNumValsPerBlock - 2) + 1)];
      ClobberMemory();
    } // 100x block
  }
  printf("Decompression Sum(100) dummy result: %f\n", dummy);

  if (show)
    Show();

  auto f = TFile::Open(output.c_str(), "RECREATE");
  f->cd();
  gHistNop.Write();
  gHistSin100.Write();
  gHistUnzip10k.Write();
  gHistUnzip10k100X.Write();
  f->Close();

  return 0;
}
