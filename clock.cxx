#include <ROOT/RNTupleMetrics.hxx>

#include <TApplication.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TH1F.h>
#include <TLegend.h>
#include <TPaveStats.h>
#include <TStyle.h>
#include <TSystem.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <future>
#include <limits>
#include <string>

#include <unistd.h>

using RNTupleAtomicCounter = ROOT::Experimental::Detail::RNTupleAtomicCounter;
using RNTupleAtomicTimer = ROOT::Experimental::Detail::RNTupleAtomicTimer;
using RNTupleMetrics = ROOT::Experimental::Detail::RNTupleMetrics;
template<class T>
using RNTupleTickCounter = ROOT::Experimental::Detail::RNTupleTickCounter<T>;

class ClockHist {
  TH1D *fHWall;
  TH1D *fHCpu;
  std::int64_t fMinWall = std::numeric_limits<std::int64_t>::max();
  std::int64_t fMaxWall = std::numeric_limits<std::int64_t>::min();
  double fMinCpu = std::numeric_limits<double>::max();
  double fMaxCpu = std::numeric_limits<double>::lowest();
  std::uint64_t fTotal = 0;

public:
  ClockHist(const std::string &name, std::int64_t min, std::int64_t max) {
    fHWall = new TH1D((name + " Wall").c_str(), "", 250, min, max);
    fHCpu = new TH1D((name + " CPU").c_str(), "", 250, min, max);
  }

  void Fill(std::int64_t wall_ns, double cpu_ns) {
    fMinWall = std::min(fMinWall, wall_ns);
    fMaxWall = std::max(fMaxWall, wall_ns);
    fMinCpu = std::min(fMinCpu, cpu_ns);
    fMaxCpu = std::max(fMaxCpu, cpu_ns);
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
    l->AddEntry(fHWall, "wall time", "f");
    l->AddEntry(fHCpu, "CPU time", "f");
    l->SetTextSize(0.04);
    l->Draw();
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

ClockHist gHistNop("no-op", 0, 1200);          // [0 - 1.2us]
ClockHist gHistSin100("100X sine", 0, 10000);  // [0 - 10us]


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

  printf("press ENTER to exit...\n");
  auto future = std::async(std::launch::async, getchar);
  while (true) {
    gSystem->ProcessEvents();
    if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
       break;
  }
}

static void Usage(const char *progname) {
  printf("%s [-s random seed] [-o output file] [-s(how)]\n", progname);
}

int main(int argc, char **argv) {
  bool show = false;
  std::string output = "clock.root";
  double seed = 42.0;
  int c;
  while ((c = getopt(argc, argv, "hvr:o:s")) != -1) {
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

  RNTupleMetrics metrics("metrics");
  auto ctrWallNop = metrics.MakeCounter<RNTupleAtomicCounter*>("timeWallNop", "ns", "Wall time of a noop");
  auto ctrCpuNop = metrics.MakeCounter<ROOT::Experimental::Detail::RNTupleTickCounter<RNTupleAtomicCounter>*>(
    "timeCpuNop", "ns", "CPU time of a noop");
  auto ctrWallSin100 = metrics.MakeCounter<RNTupleAtomicCounter*>("timeWallSin100", "ns", "Wall time of 100x sine");
  auto ctrCpuSin100 = metrics.MakeCounter<ROOT::Experimental::Detail::RNTupleTickCounter<RNTupleAtomicCounter>*>(
    "timeCpuSin100", "ns", "CPU time of 100x sine");
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

  // 100 times sine
  double sine = seed;
  for (unsigned i = 0; i < 100000; ++i) {
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

  if (show)
    Show();

  auto f = TFile::Open(output.c_str(), "RECREATE");
  f->cd();
  gHistNop.Write();
  gHistSin100.Write();
  f->Close();

  return 0;
}
