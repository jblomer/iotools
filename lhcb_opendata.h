/**
 * Copyright CERN; jblomer@cern.ch
 */

#ifndef LHCB_OPENDATA_H_
#define LHCB_OPENDATA_H_

#include <hdf5.h>
#include <sqlite3.h>
#include <unistd.h>
#include <stdint.h>

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

class H5Row {
 public:
  static const hsize_t kDimension;

  struct DataSet {
    double b_flight_distance;
    double b_vertex_chi2;
    double h1_px, h1_py, h1_pz;
    double h1_prob_k, h1_prob_pi;
    int h1_charge, h1_is_muon;
    double h1_ip_chi2;
    double h2_px, h2_py, h2_pz;
    double h2_prob_k, h2_prob_pi;
    int h2_charge, h2_is_muon;
    double h2_ip_chi2;
    double h3_px, h3_py, h3_pz;
    double h3_prob_k, h3_prob_pi;
    int h3_charge, h3_is_muon;
    double h3_ip_chi2;
  };

  H5Row();
  ~H5Row();

 protected:
  hid_t type_id_;
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


class EventReaderH5Row : public EventReader, H5Row {
 public:
  EventReaderH5Row()
    : file_id_(-1), set_id_(-1), mem_space_id_(-1), space_id_(-1), nevent_(0) {}
  virtual void Open(const std::string &path) override;
  virtual bool NextEvent(Event *event) override;

 private:
  hid_t file_id_;
  hid_t set_id_;
  hid_t mem_space_id_;
  hid_t space_id_;
  hsize_t nevent_;
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


class EventWriterH5Row : public EventWriter, H5Row {
 public:
  EventWriterH5Row()
    : file_id_(-1), space_id_(-1), set_id_(-1), mem_space_id_(-1), nevent_(0) {}
  virtual void Open(const std::string &path) override;
  virtual void WriteEvent(const Event &event) override;
  virtual void Close() override;

 private:
  hid_t file_id_;
  hid_t space_id_;
  hid_t set_id_;
  hid_t mem_space_id_;
  hsize_t nevent_;
};


class EventWriterH5Column : public EventWriter {
 public:
  EventWriterH5Column() : file_id_(-1) { }
  virtual void Open(const std::string &path) override;
  virtual void WriteEvent(const Event &event) override;
  virtual void Close() override;

 private:
  hid_t file_id_;
};

#endif  // LHCB_OPENDATA_H_
