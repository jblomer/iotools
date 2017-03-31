/**
 * Copyright CERN; jblomer@cern.ch
 */

#ifndef LHCB_OPENDATA_H_
#define LHCB_OPENDATA_H_

#include "sqlite3.h"

#include <array>
#include <memory>
#include <string>

#include "util.h"

struct KaonCandidate {
  double h_px, h_py, h_pz;
  double h_prob_k, h_prob_pi;
  bool h_charge;
  bool h_is_muon;
  double h_ip_chi2;  // unused
};

struct Event {
  double b_flight_distance;  // unused
  double b_vertex_chi2;  // unused
  std::array<KaonCandidate, 3> kaon_candidates;
};


class EventWriter {
 public:
  static std::unique_ptr<EventWriter> Create(FileFormats format);

  virtual void Open(const std::string &path) = 0;
  virtual void WriteEvent(const Event &event) = 0;
  virtual void Close() = 0;
};


class EventWriterSqlite : public EventWriter {
 public:
  EventWriterSqlite() : db_(nullptr) { }
  virtual void Open(const std::string &path) override;
  virtual void WriteEvent(const Event &event) override;
  virtual void Close() override;

 private:
  sqlite3 *db_;
};

#endif  // LHCB-OPENDATA_H_
