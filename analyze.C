#include <iostream>

#include <TFile.h>
#include <TTree.h>

void analyze(TString input = "iotrace.root") {
  TFile file(input);
  TTree *events = (TTree *) file.Get("Events");

  int N = events->GetEntries();
  std::cout << "Number of Events: " << N << std::endl;

  ULong64_t var_seqno;
  ULong64_t var_duration_ns;
  Long64_t var_size;
  Long64_t var_offset;
  events->SetBranchAddress("seqno", &var_seqno);
  events->SetBranchAddress("size", &var_size);
  events->SetBranchAddress("offset", &var_offset);
  events->SetBranchAddress("duration_ns", &var_duration_ns);

  ULong64_t sum_read_ns = 0;
  ULong64_t sum_seek_ns = 0;
  ULong64_t sum_size = 0;
  ULong64_t n_read = 0;
  ULong64_t n_seek = 0;
  ULong64_t file_pos = 0;

  for (int i = 0; i < N; ++i) {
    events->GetEntry(i);
    if (var_size >= 0) {
      n_read++;
      sum_size += var_size;
      sum_read_ns += var_duration_ns;
      file_pos += var_size;
    }
    if (var_offset > 0) {
      sum_seek_ns += var_duration_ns;
    }
  }

  std::cout << "# Reads: " << n_read << "    Size(kB): " << sum_size/1024
            << " read-time(ms): " << sum_read_ns/(1024*1024)
            << " seek-time(ms): " << sum_seek_ns/(1024*1024) << std::endl;
}
