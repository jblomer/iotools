/**
 * Copyright CERN; jblomer@cern.ch
 */

#ifndef LHCB_OPENDATA_H_
#define LHCB_OPENDATA_H_

#include <avro.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/gzip_stream.h>
#include <hdf5.h>
#include <sqlite3.h>
#include <unistd.h>
#include <stdint.h>

// Parquet
#include <arrow/io/file.h>
#include <parquet/api/reader.h>
#include <parquet/api/writer.h>

#include <array>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <TFile.h>
#include <TTree.h>

#include "event.h"
#include "lhcb_opendata.pb.h"
#include "util.h"

class TChain;

class H5Row {
 public:
  static const hsize_t kDefaultDimension;
  static const hsize_t kShortDimension;

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

  explicit H5Row();
  ~H5Row();

 protected:
  hid_t type_id_;
};

class H5Column {
 public:
  static const hsize_t kDefaultDimension;
  static const hsize_t kShortDimension;

  H5Column();
  void SetupColumns(hid_t file_id);
  void OpenColumns(hid_t file_id);
  ~H5Column();

 protected:
  hsize_t dimension_;
  hid_t sid_b_flight_distance_;
  hid_t sid_b_vertex_chi2_;
  hid_t sid_h1_px_;
  hid_t sid_h1_py_;
  hid_t sid_h1_pz_;
  hid_t sid_h1_prob_k_;
  hid_t sid_h1_prob_pi_;
  hid_t sid_h1_charge_;
  hid_t sid_h1_is_muon_;
  hid_t sid_h1_ip_chi2_;
  hid_t sid_h2_px_;
  hid_t sid_h2_py_;
  hid_t sid_h2_pz_;
  hid_t sid_h2_prob_k_;
  hid_t sid_h2_prob_pi_;
  hid_t sid_h2_charge_;
  hid_t sid_h2_is_muon_;
  hid_t sid_h2_ip_chi2_;
  hid_t sid_h3_px_;
  hid_t sid_h3_py_;
  hid_t sid_h3_pz_;
  hid_t sid_h3_prob_k_;
  hid_t sid_h3_prob_pi_;
  hid_t sid_h3_charge_;
  hid_t sid_h3_is_muon_;
  hid_t sid_h3_ip_chi2_;
  hid_t space_id_;

 private:
  void CreateSetDouble(const char *name, hid_t file_id, hid_t *set_id);
  void CreateSetInt(const char *name, hid_t file_id, hid_t *set_id);
  void OpenSet(const char *name, hid_t file_id, hid_t *set_id);
};

class AvroRow {
 public:
  AvroRow();
 protected:
  avro_schema_t schema_;
};


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
    : file_id_(-1), set_id_(-1), mem_space_id_(-1), space_id_(-1), nevent_(0),
      dimension_(0) {}
  virtual void Open(const std::string &path) override;
  virtual bool NextEvent(Event *event) override;

 private:
  hid_t file_id_;
  hid_t set_id_;
  hid_t mem_space_id_;
  hid_t space_id_;
  hsize_t nevent_;
  hsize_t dimension_;
};


class EventReaderH5Column : public EventReader, H5Column {
 public:
  EventReaderH5Column()
    : file_id_(-1), mem_space_id_(-1), nevent_(0) {}
  virtual void Open(const std::string &path) override;
  virtual bool NextEvent(Event *event) override;

 private:
  double ReadDouble(hid_t set_id);
  int ReadInt(hid_t set_id);

  hid_t file_id_;
  hid_t mem_space_id_;
  hsize_t nevent_;
};


class EventReaderAvro : public EventReader, AvroRow {
 public:
  EventReaderAvro() { }
  virtual void Open(const std::string &path) override;
  virtual bool NextEvent(Event *event) override;

 private:
  avro_file_reader_t db_;
  avro_schema_t proj_schema_;

  avro_datum_t val_h1_px_;
  avro_datum_t val_h1_py_;
  avro_datum_t val_h1_pz_;
  avro_datum_t val_h1_prob_k_;
  avro_datum_t val_h1_prob_pi_;
  avro_datum_t val_h1_charge_;
  avro_datum_t val_h1_is_muon_;
  avro_datum_t val_h2_px_;
  avro_datum_t val_h2_py_;
  avro_datum_t val_h2_pz_;
  avro_datum_t val_h2_prob_k_;
  avro_datum_t val_h2_prob_pi_;
  avro_datum_t val_h2_charge_;
  avro_datum_t val_h2_is_muon_;
  avro_datum_t val_h3_px_;
  avro_datum_t val_h3_py_;
  avro_datum_t val_h3_pz_;
  avro_datum_t val_h3_prob_k_;
  avro_datum_t val_h3_prob_pi_;
  avro_datum_t val_h3_charge_;
  avro_datum_t val_h3_is_muon_;
};


class EventReaderProtobuf : public EventReader {
 public:
  EventReaderProtobuf(bool compressed)
    : compressed_(compressed)
    , fd_(-1)
    , file_istream_(nullptr)
    , gzip_istream_(nullptr)
    , istream_(nullptr)
    , coded_istream_(nullptr)
    , nevent_(0)
  { }
  virtual void Open(const std::string &path) override;
  virtual bool NextEvent(Event *event) override;

 private:
  bool compressed_;
  int fd_;
  google::protobuf::io::FileInputStream *file_istream_;
  google::protobuf::io::GzipInputStream *gzip_istream_;
  google::protobuf::io::ZeroCopyInputStream *istream_;
  google::protobuf::io::CodedInputStream *coded_istream_;
  unsigned nevent_;
  PbEvent pb_event_;
};


class EventReaderRoot : public EventReader {
 public:
  EventReaderRoot(bool row_wise)
    : root_chain_(nullptr)
    , row_wise_(row_wise)
    , num_events_(-1)
    , pos_events_(-1)
    , read_all_(false) { }
  virtual void Open(const std::string &path) override;
  virtual bool NextEvent(Event *event) override;

  virtual void PrepareForConversion(Event *event) override;

 private:
  void AttachBranches2Event(Event *event);
  void AttachUnusedBranches2Event(Event *event);

  TChain *root_chain_;
  bool row_wise_;
  int num_events_;
  int pos_events_;
  bool read_all_;
  FlatEvent *flat_event_;
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
};


class EventReaderParquet : public EventReader {
 public:
  EventReaderParquet() : nevent_(0), dimension_(0), nrowgroup_(0),
                         num_row_groups_(-1) { }
  virtual void Open(const std::string &path) override;
  virtual bool NextEvent(Event *event) override;

 private:
  unsigned nevent_;
  unsigned dimension_;
  unsigned nrowgroup_;
  int num_row_groups_;
  std::unique_ptr<parquet::ParquetFileReader> parquet_reader_;
  std::shared_ptr<parquet::RowGroupReader> rg_reader_;

  std::shared_ptr<parquet::ColumnReader> rd_h1_px_;
  std::shared_ptr<parquet::ColumnReader> rd_h1_py_;
  std::shared_ptr<parquet::ColumnReader> rd_h1_pz_;
  std::shared_ptr<parquet::ColumnReader> rd_h1_prob_k_;
  std::shared_ptr<parquet::ColumnReader> rd_h1_prob_pi_;
  std::shared_ptr<parquet::ColumnReader> rd_h1_charge_;
  std::shared_ptr<parquet::ColumnReader> rd_h1_is_muon_;
  std::shared_ptr<parquet::ColumnReader> rd_h2_px_;
  std::shared_ptr<parquet::ColumnReader> rd_h2_py_;
  std::shared_ptr<parquet::ColumnReader> rd_h2_pz_;
  std::shared_ptr<parquet::ColumnReader> rd_h2_prob_k_;
  std::shared_ptr<parquet::ColumnReader> rd_h2_prob_pi_;
  std::shared_ptr<parquet::ColumnReader> rd_h2_charge_;
  std::shared_ptr<parquet::ColumnReader> rd_h2_is_muon_;
  std::shared_ptr<parquet::ColumnReader> rd_h3_px_;
  std::shared_ptr<parquet::ColumnReader> rd_h3_py_;
  std::shared_ptr<parquet::ColumnReader> rd_h3_pz_;
  std::shared_ptr<parquet::ColumnReader> rd_h3_prob_k_;
  std::shared_ptr<parquet::ColumnReader> rd_h3_prob_pi_;
  std::shared_ptr<parquet::ColumnReader> rd_h3_charge_;
  std::shared_ptr<parquet::ColumnReader> rd_h3_is_muon_;
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


class EventWriterProtobuf : public EventWriter {
 public:
  EventWriterProtobuf(bool compressed)
    : compressed_(compressed)
    , fd_(-1)
    , file_ostream_(nullptr)
    , gzip_ostream_(nullptr)
    , ostream_(nullptr)
  { }
  virtual void Open(const std::string &path) override;
  virtual void WriteEvent(const Event &event) override;
  virtual void Close() override;

 private:
  bool compressed_;
  int fd_;
  google::protobuf::io::FileOutputStream *file_ostream_;
  google::protobuf::io::GzipOutputStream *gzip_ostream_;
  google::protobuf::io::CodedOutputStream *ostream_;
};


class EventWriterH5Row : public EventWriter, H5Row {
 public:
  EventWriterH5Row()
    : file_id_(-1), space_id_(-1), set_id_(-1), mem_space_id_(-1), nevent_(0),
      dimension_(0) { }
  virtual void Open(const std::string &path) override;
  virtual void WriteEvent(const Event &event) override;
  virtual void Close() override;

 private:
  hid_t file_id_;
  hid_t space_id_;
  hid_t set_id_;
  hid_t mem_space_id_;
  hsize_t nevent_;
  hsize_t dimension_;
};


class EventWriterH5Column : public EventWriter, H5Column {
 public:
  EventWriterH5Column() : file_id_(-1), mem_space_id_(-1), nevent_(0) { }
  virtual void Open(const std::string &path) override;
  virtual void WriteEvent(const Event &event) override;
  virtual void Close() override;

 private:
  void WriteDouble(hid_t space_id, const double *value);
  void WriteInt(hid_t space_id, const int *value);
  void WriteBool(hid_t space_id, const bool *value);

  hid_t file_id_;
  hid_t mem_space_id_;
  hsize_t nevent_;
};


class EventWriterRoot : public EventWriter {
 public:
  enum class CompressionAlgorithms
    { kCompressionNone, kCompressionDeflate, kCompressionLz4 };

  enum class SplitMode
    { kSplitManual, kSplitAuto, kSplitNone };

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
};


class EventWriterParquet : public EventWriter {
 public:
  enum class CompressionAlgorithms
    { kCompressionNone, kCompressionDeflate, kCompressionSnappy };

  static const int kNumRowsPerGroup;


  EventWriterParquet(CompressionAlgorithms compression)
    : compression_(compression), nevent_(0), dimension_(0), rg_writer_(nullptr)
  { }
  virtual void Open(const std::string &path) override;
  virtual void WriteEvent(const Event &event) override;
  virtual void Close() override;

 private:
  void AddDoubleField(const char *name, parquet::schema::NodeVector *fields);
  void AddIntField(const char *name, parquet::schema::NodeVector *fields);
  parquet::DoubleWriter *NextColumnDouble();
  parquet::Int32Writer *NextColumnInt();
  void WriteDouble(double value, parquet::DoubleWriter *writer);
  void WriteInt(int value, parquet::Int32Writer *writer);

  CompressionAlgorithms compression_;
  unsigned nevent_;
  unsigned dimension_;
  parquet::RowGroupWriter *rg_writer_;
  std::shared_ptr<parquet::schema::GroupNode> schema_;
  std::shared_ptr<arrow::io::FileOutputStream> out_file_;
  std::shared_ptr<parquet::WriterProperties> properties_;
  std::shared_ptr<parquet::ParquetFileWriter> file_writer_;

  std::vector<Event> event_buffer_;

  parquet::DoubleWriter *wr_b_flight_distance_;
  parquet::DoubleWriter *wr_b_vertex_chi2_;
  parquet::DoubleWriter *wr_h1_px_;
  parquet::DoubleWriter *wr_h1_py_;
  parquet::DoubleWriter *wr_h1_pz_;
  parquet::DoubleWriter *wr_h1_prob_k_;
  parquet::DoubleWriter *wr_h1_prob_pi_;
  parquet::Int32Writer *wr_h1_charge_;
  parquet::Int32Writer *wr_h1_is_muon_;
  parquet::DoubleWriter *wr_h1_ip_chi2_;
  parquet::DoubleWriter *wr_h2_px_;
  parquet::DoubleWriter *wr_h2_py_;
  parquet::DoubleWriter *wr_h2_pz_;
  parquet::DoubleWriter *wr_h2_prob_k_;
  parquet::DoubleWriter *wr_h2_prob_pi_;
  parquet::Int32Writer *wr_h2_charge_;
  parquet::Int32Writer *wr_h2_is_muon_;
  parquet::DoubleWriter *wr_h2_ip_chi2_;
  parquet::DoubleWriter *wr_h3_px_;
  parquet::DoubleWriter *wr_h3_py_;
  parquet::DoubleWriter *wr_h3_pz_;
  parquet::DoubleWriter *wr_h3_prob_k_;
  parquet::DoubleWriter *wr_h3_prob_pi_;
  parquet::Int32Writer *wr_h3_charge_;
  parquet::Int32Writer *wr_h3_is_muon_;
  parquet::DoubleWriter *wr_h3_ip_chi2_;
};


class EventWriterAvro : public EventWriter, AvroRow {
 public:
  EventWriterAvro(bool compressed) : nevent_(0), compressed_(compressed) { }
  virtual void Open(const std::string &path) override;
  virtual void WriteEvent(const Event &event) override;
  virtual void Close() override;

 private:
  unsigned nevent_;
  bool compressed_;
  avro_file_writer_t db_;
};

#endif  // LHCB_OPENDATA_H_
