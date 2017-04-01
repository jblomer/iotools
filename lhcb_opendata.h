/**
 * Copyright CERN; jblomer@cern.ch
 */

#ifndef LHCB_OPENDATA_H_
#define LHCB_OPENDATA_H_

#include <sqlite3.h>
#include <unistd.h>

#include <array>
#include <memory>
#include <string>

#include "util.h"

class TChain;

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


class EventReader {
 public:
  static std::unique_ptr<EventReader> Create(FileFormats format);

  virtual void Open(const std::string &path) = 0;
  virtual bool NextEvent(Event *event) = 0;

  virtual void PrepareForConversion(Event *event) { abort(); }
};


class EventReaderSqlite : public EventReader {
 public:
  EventReaderSqlite() : db_(nullptr), sql_select_(nullptr) { }
  virtual void Open(const std::string &path) override;
  virtual bool NextEvent(Event *event) override;

 private:
  sqlite3 *db_;
  sqlite3_stmt *sql_select_;
};


class EventReaderRoot : public EventReader {
 public:
  EventReaderRoot() : root_chain_(nullptr), num_events_(-1), pos_events_(-1) { }
  virtual void Open(const std::string &path) override;
  virtual bool NextEvent(Event *event) override;

  virtual void PrepareForConversion(Event *event) override;

 private:
  void AttachBranches2Event(Event *event);
  void AttachUnusedBranches2Event(Event *event);

  TChain *root_chain_;
  int num_events_;
  int pos_events_;
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
  EventWriterSqlite() : db_(nullptr), sql_insert_(nullptr) { }
  virtual void Open(const std::string &path) override;
  virtual void WriteEvent(const Event &event) override;
  virtual void Close() override;

 private:
  sqlite3 *db_;
  sqlite3_stmt *sql_insert_;
};

#endif  // LHCB_OPENDATA_H_
