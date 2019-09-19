/**
 * Author jblomer@cern.ch
 */

#ifndef LHCB_OPENDATA_H_
#define LHCB_OPENDATA_H_

#include <unistd.h>
#include <stdint.h>

#include <array>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <TFile.h>
#include <TTree.h>

#include "event.h"
#include "util.h"

class TChain;

class EventReader {
 public:
  EventReader() : plot_only_(false), short_read_(false) { }

  static std::unique_ptr<EventReader> Create(FileFormats format);

  virtual void Open(const std::string &path) = 0;
  virtual bool NextEvent(Event *event) = 0;

  virtual void PrepareForConversion(Event *event) { abort(); }

  void set_plot_only(bool value) { plot_only_ = value; }
  void set_short_read(bool value) { short_read_ = value; }

 protected:
   bool plot_only_;  // read only few branches
   bool short_read_;
};


class EventReaderRoot : public EventReader {
 public:
  enum class SplitMode
    { kSplitManual, kSplitAuto, kSplitDeep, kSplitNone };

  EventReaderRoot(SplitMode split_mode)
    : root_chain_(nullptr)
    , split_mode_(split_mode)
    , num_events_(-1)
    , pos_events_(-1)
    , read_all_(false) { }
  virtual void Open(const std::string &path) final;
  virtual bool NextEvent(Event *event) final;

  virtual void PrepareForConversion(Event *event) override;

 private:
  int GetIsMuon(Event *event, unsigned candidate_num);
  void AttachBranches2Event(Event *event);
  void AttachBranches2EventManual(Event *event);
  void AttachBranches2EventAuto();
  void AttachBranches2EventDeep();
  void AttachBranches2EventNone();
  /**
   * Always in manual split mode (original file)
   */
  void AttachUnusedBranches2Event(Event *event);

  TChain *root_chain_;
  SplitMode split_mode_;
  int num_events_;
  int pos_events_;
  bool read_all_;
  FlatEvent *flat_event_;
  DeepEvent *deep_event_;
  TBranch *br_flat_event_;
  TBranch *br_b_flight_distance_;
  TBranch *br_b_vertex_chi2_;
  TBranch *br_h1_px_;
  TBranch *br_h1_py_;
  TBranch *br_h1_pz_;
  TBranch *br_h1_prob_k_;
  TBranch *br_h1_prob_pi_;
  TBranch *br_h1_charge_;
  TBranch *br_h1_is_muon_;
  TBranch *br_h1_ip_chi2_;
  TBranch *br_h2_px_;
  TBranch *br_h2_py_;
  TBranch *br_h2_pz_;
  TBranch *br_h2_prob_k_;
  TBranch *br_h2_prob_pi_;
  TBranch *br_h2_charge_;
  TBranch *br_h2_is_muon_;
  TBranch *br_h2_ip_chi2_;
  TBranch *br_h3_px_;
  TBranch *br_h3_py_;
  TBranch *br_h3_pz_;
  TBranch *br_h3_prob_k_;
  TBranch *br_h3_prob_pi_;
  TBranch *br_h3_charge_;
  TBranch *br_h3_is_muon_;
  TBranch *br_h3_ip_chi2_;

  TBranch *br_deep_event_;
  TBranch *br_h_px_;
  TBranch *br_h_py_;
  TBranch *br_h_pz_;
  TBranch *br_h_prob_k_;
  TBranch *br_h_prob_pi_;
  TBranch *br_h_charge_;
  TBranch *br_h_is_muon_;
  TBranch *br_h_ip_chi2_;
};




class EventWriter {
 public:
  static std::unique_ptr<EventWriter> Create(FileFormats format);
  EventWriter() : short_write_(false) { }

  virtual void Open(const std::string &path) = 0;
  virtual void WriteEvent(const Event &event) = 0;
  virtual void Close() = 0;

  void set_short_write(bool value) { short_write_ = value; }

 protected:
  bool short_write_;
};



class EventWriterRoot : public EventWriter {
 public:
  enum class CompressionAlgorithms
    { kCompressionNone, kCompressionDeflate, kCompressionLz4,
      kCompressionLzma };

  enum class SplitMode
    { kSplitManual, kSplitAuto, kSplitDeep, kSplitNone };

  EventWriterRoot(CompressionAlgorithms compression, SplitMode split_mode)
    : compression_(compression)
    , split_mode_(split_mode)
    , output_(nullptr)
    , tree_(nullptr) { }
  virtual void Open(const std::string &path) override;
  virtual void WriteEvent(const Event &event) override;
  virtual void Close() override;

 private:
  CompressionAlgorithms compression_;
  SplitMode split_mode_;
  TFile *output_;
  TTree *tree_;
  Event event_;
  FlatEvent *flat_event_;
  DeepEvent *deep_event_;
  unsigned nevent = 0;
};


#endif  // LHCB_OPENDATA_H_
