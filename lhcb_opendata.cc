/**
 * Copyright CERN; jblomer@cern.ch
 */

#include "lhcb_opendata.h"

#include <fcntl.h>
#include <hdf5_hl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <TChain.h>
#include <TTreeReader.h>

#include "util.h"


#define AVRO_APP avro_schema_record_field_append

AvroRow::AvroRow() {
  int retval;
  schema_ = avro_schema_record("event", "lhcb.cern.ch");
  retval =
    AVRO_APP(schema_, "b_flight_distance", avro_schema_double()) ||
    AVRO_APP(schema_, "b_vertex_chi2", avro_schema_double()) ||
    AVRO_APP(schema_, "h1_px", avro_schema_double()) ||
    AVRO_APP(schema_, "h1_py", avro_schema_double()) ||
    AVRO_APP(schema_, "h1_pz", avro_schema_double()) ||
    AVRO_APP(schema_, "h1_prob_k", avro_schema_double()) ||
    AVRO_APP(schema_, "h1_prob_pi", avro_schema_double()) ||
    AVRO_APP(schema_, "h1_charge", avro_schema_int()) ||
    AVRO_APP(schema_, "h1_is_muon", avro_schema_int()) ||
    AVRO_APP(schema_, "h1_ip_chi2", avro_schema_double()) ||
    AVRO_APP(schema_, "h2_px", avro_schema_double()) ||
    AVRO_APP(schema_, "h2_py", avro_schema_double()) ||
    AVRO_APP(schema_, "h2_pz", avro_schema_double()) ||
    AVRO_APP(schema_, "h2_prob_k", avro_schema_double()) ||
    AVRO_APP(schema_, "h2_prob_pi", avro_schema_double()) ||
    AVRO_APP(schema_, "h2_charge", avro_schema_int()) ||
    AVRO_APP(schema_, "h2_is_muon", avro_schema_int()) ||
    AVRO_APP(schema_, "h2_ip_chi2", avro_schema_double()) ||
    AVRO_APP(schema_, "h3_px", avro_schema_double()) ||
    AVRO_APP(schema_, "h3_py", avro_schema_double()) ||
    AVRO_APP(schema_, "h3_pz", avro_schema_double()) ||
    AVRO_APP(schema_, "h3_prob_k", avro_schema_double()) ||
    AVRO_APP(schema_, "h3_prob_pi", avro_schema_double()) ||
    AVRO_APP(schema_, "h3_charge", avro_schema_int()) ||
    AVRO_APP(schema_, "h3_is_muon", avro_schema_int()) ||
    AVRO_APP(schema_, "h3_ip_chi2", avro_schema_double());

  assert(retval == 0);
}


//------------------------------------------------------------------------------



std::unique_ptr<EventWriter> EventWriter::Create(FileFormats format) {
  switch (format) {
    case FileFormats::kH5Row:
      return std::unique_ptr<EventWriter>(new EventWriterH5Row());
    case FileFormats::kH5Column:
      return std::unique_ptr<EventWriter>(new EventWriterH5Column());
    case FileFormats::kSqlite:
      return std::unique_ptr<EventWriter>(new EventWriterSqlite());
    case FileFormats::kAvroDeflated:
      return std::unique_ptr<EventWriter>(new EventWriterAvro(true));
    case FileFormats::kAvroInflated:
      return std::unique_ptr<EventWriter>(new EventWriterAvro(false));
    case FileFormats::kProtobufDeflated:
      return std::unique_ptr<EventWriter>(new EventWriterProtobuf(true));
    case FileFormats::kProtobufInflated:
      return std::unique_ptr<EventWriter>(new EventWriterProtobuf(false));
    case FileFormats::kRootDeflated:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(true));
    case FileFormats::kRootInflated:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(false));
    case FileFormats::kParquetInflated:
      return std::unique_ptr<EventWriter>(new EventWriterParquet(
        EventWriterParquet::CompressionAlgorithms::kCompressionNone));
    case FileFormats::kParquetDeflated:
      return std::unique_ptr<EventWriter>(new EventWriterParquet(
        EventWriterParquet::CompressionAlgorithms::kCompressionDeflate));
    case FileFormats::kParquetSnappy:
      return std::unique_ptr<EventWriter>(new EventWriterParquet(
        EventWriterParquet::CompressionAlgorithms::kCompressionSnappy));
    default:
      abort();
  }
}


std::unique_ptr<EventReader> EventReader::Create(FileFormats format) {
  switch (format) {
    case FileFormats::kParquetDeflated:
    case FileFormats::kParquetInflated:
    case FileFormats::kParquetSnappy:
      return std::unique_ptr<EventReader>(new EventReaderParquet());
    case FileFormats::kRoot:
    case FileFormats::kRootDeflated:
    case FileFormats::kRootInflated:
      return std::unique_ptr<EventReader>(new EventReaderRoot());
    case FileFormats::kSqlite:
      return std::unique_ptr<EventReader>(new EventReaderSqlite());
    case FileFormats::kH5Row:
      return std::unique_ptr<EventReader>(new EventReaderH5Row());
    case FileFormats::kH5Column:
      return std::unique_ptr<EventReader>(new EventReaderH5Column());
    case FileFormats::kAvroDeflated:
    case FileFormats::kAvroInflated:
      return std::unique_ptr<EventReader>(new EventReaderAvro());
    case FileFormats::kProtobufDeflated:
      return std::unique_ptr<EventReader>(new EventReaderProtobuf(true));
    case FileFormats::kProtobufInflated:
      return std::unique_ptr<EventReader>(new EventReaderProtobuf(false));
    default:
      abort();
  }
}


//------------------------------------------------------------------------------


void EventWriterH5Row::Open(const std::string &path) {
  file_id_ = H5Fcreate(path.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  assert(file_id_ >= 0);

  space_id_ = H5Screate_simple(1, &kDimension, NULL);
  assert(space_id_ >= 0);

  set_id_ = H5Dcreate(file_id_, "/DecayTree", type_id_, space_id_,
                      H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  assert(set_id_ >= 0);

  mem_space_id_ = H5Screate(H5S_SCALAR);
  assert(mem_space_id_ >= 0);
}


void EventWriterH5Row::WriteEvent(const Event &event) {
  DataSet dataset;
  dataset.b_flight_distance = event.b_flight_distance;
  dataset.b_vertex_chi2 = event.b_vertex_chi2;
  dataset.h1_px = event.kaon_candidates[0].h_px;
  dataset.h1_py = event.kaon_candidates[0].h_py;
  dataset.h1_pz = event.kaon_candidates[0].h_pz;
  dataset.h1_prob_k = event.kaon_candidates[0].h_prob_k;
  dataset.h1_prob_pi = event.kaon_candidates[0].h_prob_pi;
  dataset.h1_charge = event.kaon_candidates[0].h_charge;
  dataset.h1_is_muon = event.kaon_candidates[0].h_is_muon;
  dataset.h1_ip_chi2 = event.kaon_candidates[0].h_ip_chi2;
  dataset.h2_px = event.kaon_candidates[1].h_px;
  dataset.h2_py = event.kaon_candidates[1].h_py;
  dataset.h2_pz = event.kaon_candidates[1].h_pz;
  dataset.h2_prob_k = event.kaon_candidates[1].h_prob_k;
  dataset.h2_prob_pi = event.kaon_candidates[1].h_prob_pi;
  dataset.h2_charge = event.kaon_candidates[1].h_charge;
  dataset.h2_is_muon = event.kaon_candidates[1].h_is_muon;
  dataset.h2_ip_chi2 = event.kaon_candidates[1].h_ip_chi2;
  dataset.h3_px = event.kaon_candidates[2].h_px;
  dataset.h3_py = event.kaon_candidates[2].h_py;
  dataset.h3_pz = event.kaon_candidates[2].h_pz;
  dataset.h3_prob_k = event.kaon_candidates[2].h_prob_k;
  dataset.h3_prob_pi = event.kaon_candidates[2].h_prob_pi;
  dataset.h3_charge = event.kaon_candidates[2].h_charge;
  dataset.h3_is_muon = event.kaon_candidates[2].h_is_muon;
  dataset.h3_ip_chi2 = event.kaon_candidates[2].h_ip_chi2;

  hsize_t count = 1;
  herr_t retval;
  retval = H5Sselect_hyperslab(
    space_id_, H5S_SELECT_SET, &nevent_, NULL, &count, NULL);
  assert(retval >= 0);

  retval = H5Dwrite(set_id_, type_id_, mem_space_id_, space_id_,
                    H5P_DEFAULT, &dataset);
  assert(retval >= 0);

  nevent_++;
}


void EventWriterH5Row::Close() {
  H5Sclose(mem_space_id_);
  H5Dclose(set_id_);
  H5Sclose(space_id_);
  H5Fclose(file_id_);
}


//------------------------------------------------------------------------------


void EventWriterProtobuf::Open(const std::string &path) {
  fd_ = open(path.c_str(), O_WRONLY | O_TRUNC | O_CREAT,
             S_IREAD | S_IWRITE | S_IRGRP | S_IROTH);
  assert(fd_ >= 0);
  file_ostream_ = new google::protobuf::io::FileOutputStream(fd_);
  assert(file_ostream_);
  if (compressed_) {
    gzip_ostream_ = new google::protobuf::io::GzipOutputStream(file_ostream_);
    assert(gzip_ostream_);
    ostream_ = new google::protobuf::io::CodedOutputStream(gzip_ostream_);
  } else {
    ostream_ = new google::protobuf::io::CodedOutputStream(file_ostream_);
  }
  assert(ostream_);
}


void EventWriterProtobuf::WriteEvent(const Event &event) {
  PbEvent pb_event;
  pb_event.set_b_flight_distance(event.b_flight_distance);
  pb_event.set_b_vertex_chi2(event.b_vertex_chi2);
  pb_event.set_h1_px(event.kaon_candidates[0].h_px);
  pb_event.set_h1_py(event.kaon_candidates[0].h_py);
  pb_event.set_h1_pz(event.kaon_candidates[0].h_pz);
  pb_event.set_h1_prob_k(event.kaon_candidates[0].h_prob_k);
  pb_event.set_h1_prob_pi(event.kaon_candidates[0].h_prob_pi);
  pb_event.set_h1_charge(event.kaon_candidates[0].h_charge);
  pb_event.set_h1_is_muon(event.kaon_candidates[0].h_is_muon);
  pb_event.set_h1_ip_chi2(event.kaon_candidates[0].h_ip_chi2);
  pb_event.set_h2_px(event.kaon_candidates[1].h_px);
  pb_event.set_h2_py(event.kaon_candidates[1].h_py);
  pb_event.set_h2_pz(event.kaon_candidates[1].h_pz);
  pb_event.set_h2_prob_k(event.kaon_candidates[1].h_prob_k);
  pb_event.set_h2_prob_pi(event.kaon_candidates[1].h_prob_pi);
  pb_event.set_h2_charge(event.kaon_candidates[1].h_charge);
  pb_event.set_h2_is_muon(event.kaon_candidates[1].h_is_muon);
  pb_event.set_h2_ip_chi2(event.kaon_candidates[1].h_ip_chi2);
  pb_event.set_h3_px(event.kaon_candidates[2].h_px);
  pb_event.set_h3_py(event.kaon_candidates[2].h_py);
  pb_event.set_h3_pz(event.kaon_candidates[2].h_pz);
  pb_event.set_h3_prob_k(event.kaon_candidates[2].h_prob_k);
  pb_event.set_h3_prob_pi(event.kaon_candidates[2].h_prob_pi);
  pb_event.set_h3_charge(event.kaon_candidates[2].h_charge);
  pb_event.set_h3_is_muon(event.kaon_candidates[2].h_is_muon);
  pb_event.set_h3_ip_chi2(event.kaon_candidates[2].h_ip_chi2);

  ostream_->WriteLittleEndian32(pb_event.ByteSize());
  bool retval = pb_event.SerializeToCodedStream(ostream_);
  assert(retval);
}


void EventWriterProtobuf::Close() {
  delete ostream_;
  delete gzip_ostream_;
  delete file_ostream_;
  close(fd_);
}



//------------------------------------------------------------------------------


void EventWriterAvro::Open(const std::string &path) {
  unlink(path.c_str());
  int retval = avro_file_writer_create_with_codec(
    path.c_str(), schema_, &db_, compressed_ ? "deflate" : "null", 0);
  if (retval != 0)
    fprintf(stderr, "avro error %s\n", avro_strerror());
  assert(retval == 0);
}


void EventWriterAvro::WriteEvent(const Event &event) {
  int retval;
  avro_datum_t record = avro_record(schema_);

  avro_datum_t b_flight_distance = avro_double(event.b_flight_distance);
  avro_datum_t b_vertex_chi2 = avro_double(event.b_vertex_chi2);
  avro_datum_t h1_px = avro_double(event.kaon_candidates[0].h_px);
  avro_datum_t h1_py = avro_double(event.kaon_candidates[0].h_py);
  avro_datum_t h1_pz = avro_double(event.kaon_candidates[0].h_pz);
  avro_datum_t h1_prob_k = avro_double(event.kaon_candidates[0].h_prob_k);
  avro_datum_t h1_prob_pi = avro_double(event.kaon_candidates[0].h_prob_pi);
  avro_datum_t h1_charge = avro_int32(event.kaon_candidates[0].h_charge);
  avro_datum_t h1_is_muon = avro_int32(event.kaon_candidates[0].h_is_muon);
  avro_datum_t h1_ip_chi2 = avro_double(event.kaon_candidates[0].h_ip_chi2);
  avro_datum_t h2_px = avro_double(event.kaon_candidates[1].h_px);
  avro_datum_t h2_py = avro_double(event.kaon_candidates[1].h_py);
  avro_datum_t h2_pz = avro_double(event.kaon_candidates[1].h_pz);
  avro_datum_t h2_prob_k = avro_double(event.kaon_candidates[1].h_prob_k);
  avro_datum_t h2_prob_pi = avro_double(event.kaon_candidates[1].h_prob_pi);
  avro_datum_t h2_charge = avro_int32(event.kaon_candidates[1].h_charge);
  avro_datum_t h2_is_muon = avro_int32(event.kaon_candidates[1].h_is_muon);
  avro_datum_t h2_ip_chi2 = avro_double(event.kaon_candidates[1].h_ip_chi2);
  avro_datum_t h3_px = avro_double(event.kaon_candidates[2].h_px);
  avro_datum_t h3_py = avro_double(event.kaon_candidates[2].h_py);
  avro_datum_t h3_pz = avro_double(event.kaon_candidates[2].h_pz);
  avro_datum_t h3_prob_k = avro_double(event.kaon_candidates[2].h_prob_k);
  avro_datum_t h3_prob_pi = avro_double(event.kaon_candidates[2].h_prob_pi);
  avro_datum_t h3_charge = avro_int32(event.kaon_candidates[2].h_charge);
  avro_datum_t h3_is_muon = avro_int32(event.kaon_candidates[2].h_is_muon);
  avro_datum_t h3_ip_chi2 = avro_double(event.kaon_candidates[2].h_ip_chi2);

  retval =
    avro_record_set(record, "b_flight_distance", b_flight_distance) ||
    avro_record_set(record, "b_vertex_chi2", b_vertex_chi2) ||
    avro_record_set(record, "h1_px", h1_px) ||
    avro_record_set(record, "h1_py", h1_py) ||
    avro_record_set(record, "h1_pz", h1_pz) ||
    avro_record_set(record, "h1_prob_k", h1_prob_k) ||
    avro_record_set(record, "h1_prob_pi", h1_prob_pi) ||
    avro_record_set(record, "h1_charge", h1_charge) ||
    avro_record_set(record, "h1_is_muon", h1_is_muon) ||
    avro_record_set(record, "h1_ip_chi2", h1_ip_chi2) ||
    avro_record_set(record, "h2_px", h2_px) ||
    avro_record_set(record, "h2_py", h2_py) ||
    avro_record_set(record, "h2_pz", h2_pz) ||
    avro_record_set(record, "h2_prob_k", h2_prob_k) ||
    avro_record_set(record, "h2_prob_pi", h2_prob_pi) ||
    avro_record_set(record, "h2_charge", h2_charge) ||
    avro_record_set(record, "h2_is_muon", h2_is_muon) ||
    avro_record_set(record, "h2_ip_chi2", h2_ip_chi2) ||
    avro_record_set(record, "h3_px", h3_px) ||
    avro_record_set(record, "h3_py", h3_py) ||
    avro_record_set(record, "h3_pz", h3_pz) ||
    avro_record_set(record, "h3_prob_k", h3_prob_k) ||
    avro_record_set(record, "h3_prob_pi", h3_prob_pi) ||
    avro_record_set(record, "h3_charge", h3_charge) ||
    avro_record_set(record, "h3_is_muon", h3_is_muon) ||
    avro_record_set(record, "h3_ip_chi2", h3_ip_chi2);
  assert(retval == 0);

  retval = avro_file_writer_append(db_, record);

  avro_datum_decref(b_flight_distance);
  avro_datum_decref(b_vertex_chi2);
  avro_datum_decref(h1_px);
  avro_datum_decref(h1_py);
  avro_datum_decref(h1_pz);
  avro_datum_decref(h1_prob_k);
  avro_datum_decref(h1_prob_pi);
  avro_datum_decref(h1_charge);
  avro_datum_decref(h1_is_muon);
  avro_datum_decref(h1_ip_chi2);
  avro_datum_decref(h2_px);
  avro_datum_decref(h2_py);
  avro_datum_decref(h2_pz);
  avro_datum_decref(h2_prob_k);
  avro_datum_decref(h2_prob_pi);
  avro_datum_decref(h2_charge);
  avro_datum_decref(h2_is_muon);
  avro_datum_decref(h2_ip_chi2);
  avro_datum_decref(h3_px);
  avro_datum_decref(h3_py);
  avro_datum_decref(h3_pz);
  avro_datum_decref(h3_prob_k);
  avro_datum_decref(h3_prob_pi);
  avro_datum_decref(h3_charge);
  avro_datum_decref(h3_is_muon);
  avro_datum_decref(h3_ip_chi2);
  avro_datum_decref(record);
}


void EventWriterAvro::Close() {
  avro_file_writer_flush(db_);
  avro_file_writer_close(db_);
}



//------------------------------------------------------------------------------


void EventWriterH5Column::Open(const std::string &path) {
  file_id_ = H5Fcreate(path.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  assert(file_id_ >= 0);

  mem_space_id_ = H5Screate(H5S_SCALAR);
  assert(mem_space_id_ >= 0);

  SetupColumns(file_id_);
}


void EventWriterH5Column::WriteDouble(hid_t set_id, const double *value) {
  herr_t retval;
  retval = H5Dwrite(set_id, H5T_NATIVE_DOUBLE, mem_space_id_, space_id_,
                    H5P_DEFAULT, value);
  assert(retval >= 0);
}


void EventWriterH5Column::WriteInt(hid_t set_id, const int *value) {
  herr_t retval;
  retval = H5Dwrite(set_id, H5T_NATIVE_INT, mem_space_id_, space_id_,
                    H5P_DEFAULT, value);
  assert(retval >= 0);
}


void EventWriterH5Column::WriteBool(hid_t set_id, const bool *value) {
  int int_val = *value;
  herr_t retval;
  retval = H5Dwrite(set_id, H5T_NATIVE_INT, mem_space_id_, space_id_,
                    H5P_DEFAULT, &int_val);
  assert(retval >= 0);
}


void EventWriterH5Column::WriteEvent(const Event &event) {
  hsize_t count = 1;
  herr_t retval;
  retval = H5Sselect_hyperslab(
    space_id_, H5S_SELECT_SET, &nevent_, NULL, &count, NULL);
  assert(retval >= 0);

  WriteDouble(sid_b_flight_distance_, &event.b_flight_distance);
  WriteDouble(sid_b_vertex_chi2_, &event.b_vertex_chi2);
  WriteDouble(sid_h1_px_, &event.kaon_candidates[0].h_px);
  WriteDouble(sid_h1_py_, &event.kaon_candidates[0].h_py);
  WriteDouble(sid_h1_pz_, &event.kaon_candidates[0].h_pz);
  WriteDouble(sid_h1_prob_k_, &event.kaon_candidates[0].h_prob_k);
  WriteDouble(sid_h1_prob_pi_, &event.kaon_candidates[0].h_prob_pi);
  WriteBool(sid_h1_charge_, &event.kaon_candidates[0].h_charge);
  WriteBool(sid_h1_is_muon_, &event.kaon_candidates[0].h_is_muon);
  WriteDouble(sid_h1_ip_chi2_, &event.kaon_candidates[0].h_ip_chi2);
  WriteDouble(sid_h2_px_, &event.kaon_candidates[1].h_px);
  WriteDouble(sid_h2_py_, &event.kaon_candidates[1].h_py);
  WriteDouble(sid_h2_pz_, &event.kaon_candidates[1].h_pz);
  WriteDouble(sid_h2_prob_k_, &event.kaon_candidates[1].h_prob_k);
  WriteDouble(sid_h2_prob_pi_, &event.kaon_candidates[1].h_prob_pi);
  WriteBool(sid_h2_charge_, &event.kaon_candidates[1].h_charge);
  WriteBool(sid_h2_is_muon_, &event.kaon_candidates[1].h_is_muon);
  WriteDouble(sid_h2_ip_chi2_, &event.kaon_candidates[1].h_ip_chi2);
  WriteDouble(sid_h3_px_, &event.kaon_candidates[2].h_px);
  WriteDouble(sid_h3_py_, &event.kaon_candidates[2].h_py);
  WriteDouble(sid_h3_pz_, &event.kaon_candidates[2].h_pz);
  WriteDouble(sid_h3_prob_k_, &event.kaon_candidates[2].h_prob_k);
  WriteDouble(sid_h3_prob_pi_, &event.kaon_candidates[2].h_prob_pi);
  WriteBool(sid_h3_charge_, &event.kaon_candidates[2].h_charge);
  WriteBool(sid_h3_is_muon_, &event.kaon_candidates[2].h_is_muon);
  WriteDouble(sid_h3_ip_chi2_, &event.kaon_candidates[2].h_ip_chi2);

  nevent_++;
}


void EventWriterH5Column::Close() {
  H5Sclose(mem_space_id_);
  H5Sclose(space_id_);
  H5Fclose(file_id_);
}


//------------------------------------------------------------------------------


void EventWriterParquet::AddDoubleField(
  const char *name,
  parquet::schema::NodeVector *fields)
{
  fields->push_back(parquet::schema::PrimitiveNode::Make(
    name,
    parquet::Repetition::REQUIRED,
    parquet::Type::DOUBLE,
    parquet::LogicalType::NONE));
}


void EventWriterParquet::AddIntField(
  const char *name,
  parquet::schema::NodeVector *fields)
{
  fields->push_back(parquet::schema::PrimitiveNode::Make(
    name,
    parquet::Repetition::REQUIRED,
    parquet::Type::INT32,
    parquet::LogicalType::NONE));
}


void EventWriterParquet::Open(const std::string &path) {
  parquet::schema::NodeVector fields;
  AddDoubleField("b_flight_distance", &fields);
  AddDoubleField("b_vertex_chi2", &fields);
  AddDoubleField("h1_px", &fields);
  AddDoubleField("h1_py", &fields);
  AddDoubleField("h1_pz", &fields);
  AddDoubleField("h1_prob_k", &fields);
  AddDoubleField("h1_prob_pi", &fields);
  AddIntField("h1_charge", &fields);
  AddIntField("h1_is_muon", &fields);
  AddDoubleField("h1_ip_chi2", &fields);
  AddDoubleField("h2_px", &fields);
  AddDoubleField("h2_py", &fields);
  AddDoubleField("h2_pz", &fields);
  AddDoubleField("h2_prob_k", &fields);
  AddDoubleField("h2_prob_pi", &fields);
  AddIntField("h2_charge", &fields);
  AddIntField("h2_is_muon", &fields);
  AddDoubleField("h2_ip_chi2", &fields);
  AddDoubleField("h3_px", &fields);
  AddDoubleField("h3_py", &fields);
  AddDoubleField("h3_pz", &fields);
  AddDoubleField("h3_prob_k", &fields);
  AddDoubleField("h3_prob_pi", &fields);
  AddIntField("h3_charge", &fields);
  AddIntField("h3_is_muon", &fields);
  AddDoubleField("h3_ip_chi2", &fields);

  schema_ = std::static_pointer_cast<parquet::schema::GroupNode>(
    parquet::schema::GroupNode::Make(
      "schema", parquet::Repetition::REQUIRED, fields));

  unlink(path.c_str());
  PARQUET_THROW_NOT_OK(
    arrow::io::FileOutputStream::Open(path.c_str(), &out_file_));

  parquet::WriterProperties::Builder builder;
  switch (compression_) {
    case CompressionAlgorithms::kCompressionNone:
      builder.compression(parquet::Compression::UNCOMPRESSED);
      break;
    case CompressionAlgorithms::kCompressionDeflate:
      builder.compression(parquet::Compression::GZIP);
      break;
    case CompressionAlgorithms::kCompressionSnappy:
      builder.compression(parquet::Compression::SNAPPY);
      break;
    default:
      abort();
  }
  properties_ = builder.build();

  file_writer_ =
    parquet::ParquetFileWriter::Open(out_file_, schema_, properties_);
}


parquet::DoubleWriter *EventWriterParquet::NextColumnDouble() {
  return static_cast<parquet::DoubleWriter *>(rg_writer_->NextColumn());
}


parquet::Int32Writer *EventWriterParquet::NextColumnInt() {
  return static_cast<parquet::Int32Writer *>(rg_writer_->NextColumn());
}


void EventWriterParquet::WriteDouble(
  double value,
  parquet::DoubleWriter *writer)
{
  writer->WriteBatch(1, nullptr, nullptr, &value);
}

void EventWriterParquet::WriteInt(int value, parquet::Int32Writer *writer) {
  writer->WriteBatch(1, nullptr, nullptr, &value);
}


#define WR_DOUBLE(X, Y) Y=NextColumnDouble(); \
  for(auto e:event_buffer_) WriteDouble(e.X,Y);

#define WR_INT(X, Y) Y=NextColumnInt(); \
  for(auto e:event_buffer_) WriteInt(e.X,Y);

void EventWriterParquet::WriteEvent(const Event &event) {
  event_buffer_.push_back(event);
  nevent_++;

  if (((nevent_ % kNumRowsPerGroup) != 0) && (nevent_ < H5Row::kDimension)) {
    return;
  }

  rg_writer_ = file_writer_->AppendRowGroup(event_buffer_.size());

  WR_DOUBLE(b_flight_distance, wr_b_flight_distance_)
  WR_DOUBLE(b_vertex_chi2, wr_b_vertex_chi2_)
  WR_DOUBLE(kaon_candidates[0].h_px, wr_h1_px_)
  WR_DOUBLE(kaon_candidates[0].h_py, wr_h1_py_)
  WR_DOUBLE(kaon_candidates[0].h_pz, wr_h1_pz_)
  WR_DOUBLE(kaon_candidates[0].h_prob_k, wr_h1_prob_k_)
  WR_DOUBLE(kaon_candidates[0].h_prob_pi, wr_h1_prob_pi_)
  WR_INT(kaon_candidates[0].h_charge, wr_h1_charge_)
  WR_INT(kaon_candidates[0].h_is_muon, wr_h1_is_muon_)
  WR_DOUBLE(kaon_candidates[0].h_ip_chi2, wr_h1_ip_chi2_)
  WR_DOUBLE(kaon_candidates[1].h_px, wr_h2_px_)
  WR_DOUBLE(kaon_candidates[1].h_py, wr_h2_py_)
  WR_DOUBLE(kaon_candidates[1].h_pz, wr_h2_pz_)
  WR_DOUBLE(kaon_candidates[1].h_prob_k, wr_h2_prob_k_)
  WR_DOUBLE(kaon_candidates[1].h_prob_pi, wr_h2_prob_pi_)
  WR_INT(kaon_candidates[1].h_charge, wr_h2_charge_)
  WR_INT(kaon_candidates[1].h_is_muon, wr_h2_is_muon_)
  WR_DOUBLE(kaon_candidates[1].h_ip_chi2, wr_h2_ip_chi2_)
  WR_DOUBLE(kaon_candidates[2].h_px, wr_h3_px_)
  WR_DOUBLE(kaon_candidates[2].h_py, wr_h3_py_)
  WR_DOUBLE(kaon_candidates[2].h_pz, wr_h3_pz_)
  WR_DOUBLE(kaon_candidates[2].h_prob_k, wr_h3_prob_k_)
  WR_DOUBLE(kaon_candidates[2].h_prob_pi, wr_h3_prob_pi_)
  WR_INT(kaon_candidates[2].h_charge, wr_h3_charge_)
  WR_INT(kaon_candidates[2].h_is_muon, wr_h3_is_muon_)
  WR_DOUBLE(kaon_candidates[2].h_ip_chi2, wr_h3_ip_chi2_)

  event_buffer_.clear();
}


void EventWriterParquet::Close() {
  file_writer_->Close();
  out_file_->Close();
}

const int EventWriterParquet::kNumRowsPerGroup = 500;

//------------------------------------------------------------------------------


void EventWriterRoot::Open(const std::string &path) {
  output_ = new TFile(path.c_str(), "RECREATE");
  if (!compressed_)
    output_->SetCompressionSettings(0);
  tree_ = new TTree("DecayTree", "");
  tree_->Branch("B_FlightDistance", &event_.b_flight_distance,
                "B_FlightDistance/D");
  tree_->Branch("B_VertexChi2", &event_.b_vertex_chi2, "B_VertexChi2/D");
  tree_->Branch("H1_PX", &event_.kaon_candidates[0].h_px, "H1_PX/D");
  tree_->Branch("H1_PY", &event_.kaon_candidates[0].h_py, "H1_PY/D");
  tree_->Branch("H1_PZ", &event_.kaon_candidates[0].h_pz, "H1_PZ/D");
  tree_->Branch("H1_ProbK", &event_.kaon_candidates[0].h_prob_k,
                "H1_ProbK/D");
  tree_->Branch("H1_ProbPi", &event_.kaon_candidates[0].h_prob_pi,
                "H1_ProbPi/D");
  tree_->Branch("H1_Charge", &event_.kaon_candidates[0].h_charge,
                "H1_Charge/I");
  tree_->Branch("H1_isMuon", &event_.kaon_candidates[0].h_is_muon,
                "H1_isMuon/I");
  tree_->Branch("H1_IpChi2", &event_.kaon_candidates[0].h_ip_chi2,
                "H1_IpChi2/D");
  tree_->Branch("H2_PX", &event_.kaon_candidates[1].h_px, "H2_PX/D");
  tree_->Branch("H2_PY", &event_.kaon_candidates[1].h_py, "H2_PY/D");
  tree_->Branch("H2_PZ", &event_.kaon_candidates[1].h_pz, "H2_PZ/D");
  tree_->Branch("H2_ProbK", &event_.kaon_candidates[1].h_prob_k,
                "H2_ProbK/D");
  tree_->Branch("H2_ProbPi", &event_.kaon_candidates[1].h_prob_pi,
                "H2_ProbPi/D");
  tree_->Branch("H2_Charge", &event_.kaon_candidates[1].h_charge,
                "H2_Charge/I");
  tree_->Branch("H2_isMuon", &event_.kaon_candidates[1].h_is_muon,
                "H2_isMuon/I");
  tree_->Branch("H2_IpChi2", &event_.kaon_candidates[1].h_ip_chi2,
                "H2_IpChi2/D");
  tree_->Branch("H3_PX", &event_.kaon_candidates[2].h_px, "H3_PX/D");
  tree_->Branch("H3_PY", &event_.kaon_candidates[2].h_py, "H3_PY/D");
  tree_->Branch("H3_PZ", &event_.kaon_candidates[2].h_pz, "H3_PZ/D");
  tree_->Branch("H3_ProbK", &event_.kaon_candidates[2].h_prob_k,
                "H3_ProbK/D");
  tree_->Branch("H3_ProbPi", &event_.kaon_candidates[2].h_prob_pi,
                "H3_ProbPi/D");
  tree_->Branch("H3_Charge", &event_.kaon_candidates[2].h_charge,
                "H3_Charge/I");
  tree_->Branch("H3_isMuon", &event_.kaon_candidates[2].h_is_muon,
                "H3_isMuon/I");
  tree_->Branch("H3_IpChi2", &event_.kaon_candidates[2].h_ip_chi2,
                "H3_IpChi2/D");
}


void EventWriterRoot::WriteEvent(const Event &event) {
  event_ = event;
  tree_->Fill();
}


void EventWriterRoot::Close() {
  output_ = tree_->GetCurrentFile();
  output_->Write();
  output_->Close();
  delete output_;
}


//------------------------------------------------------------------------------


void EventWriterSqlite::Open(const std::string &path) {
  assert(db_ == nullptr);
  unlink(path.c_str());  // Re-create database if it exists
  int retval = sqlite3_open_v2(
    path.c_str(), &db_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
  assert(retval == SQLITE_OK);
  printf("created sqlite database %s\n", path.c_str());

  retval = sqlite3_exec(db_, "CREATE TABLE events "
    "(B_FlightDistance REAL, B_VertecChi2 REAL, H1_PX REAL, H1_PY REAL, "
    "H1_PZ REAL, H1_ProbK REAL, H1_ProbPi REAL, H1_Charge INTEGER, "
    "H1_IP2Chi2 REAL, H1_isMuon INTEGER, H2_PX REAL, H2_PY REAL, "
    "H2_PZ REAL, H2_ProbK REAL, H2_ProbPi REAL, H2_Charge INTEGER, "
    "H2_IP2Chi2 REAL, H2_isMuon INTEGER, H3_PX REAL, H3_PY REAL, "
    "H3_PZ REAL, H3_ProbK REAL, H3_ProbPi REAL, H3_Charge INTEGER, "
    "H3_IP2Chi2 REAL, H3_isMuon INTEGER); "
    "BEGIN;",
    nullptr, nullptr, nullptr);
  assert(retval == SQLITE_OK);

  retval = sqlite3_prepare_v2(db_, "INSERT INTO events VALUES "
   "(:B_FlightDistance, :B_VertecChi2, :H1_PX, :H1_PY, :H1_PZ, :H1_ProbK, "
   ":H1_ProbPi, :H1_Charge, :H1_IP2Chi2, :H1_isMuon, :H2_PX, :H2_PY, :H2_PZ, "
   ":H2_ProbK, :H2_ProbPi, :H2_Charge, :H2_IP2Chi2, :H2_isMuon, :H3_PX, "
   ":H3_PY, :H3_PZ, :H3_ProbK, :H3_ProbPi, :H3_Charge, :H3_IP2Chi2, "
   ":H3_isMuon)", -1, &sql_insert_, nullptr);
  assert(retval == SQLITE_OK);
}


void EventWriterSqlite::WriteEvent(const Event &event) {
  int retval;
  retval = sqlite3_bind_double(sql_insert_, 1, event.b_flight_distance);
  retval |= sqlite3_bind_double(sql_insert_, 2, event.b_vertex_chi2);

  retval |= sqlite3_bind_double(sql_insert_, 3, event.kaon_candidates[0].h_px);
  retval |= sqlite3_bind_double(sql_insert_, 4, event.kaon_candidates[0].h_py);
  retval |= sqlite3_bind_double(sql_insert_, 5, event.kaon_candidates[0].h_pz);
  retval |= sqlite3_bind_double(sql_insert_, 6,
                                event.kaon_candidates[0].h_prob_k);
  retval |= sqlite3_bind_double(sql_insert_, 7,
                                event.kaon_candidates[0].h_prob_pi);
  retval |= sqlite3_bind_int(sql_insert_, 8, event.kaon_candidates[0].h_charge);
  retval |= sqlite3_bind_double(sql_insert_, 9,
                                event.kaon_candidates[0].h_ip_chi2);
  retval |= sqlite3_bind_int(sql_insert_, 10,
                             event.kaon_candidates[0].h_is_muon);

  retval |= sqlite3_bind_double(sql_insert_, 11, event.kaon_candidates[1].h_px);
  retval |= sqlite3_bind_double(sql_insert_, 12, event.kaon_candidates[1].h_py);
  retval |= sqlite3_bind_double(sql_insert_, 13, event.kaon_candidates[1].h_pz);
  retval |= sqlite3_bind_double(sql_insert_, 14,
                                event.kaon_candidates[1].h_prob_k);
  retval |= sqlite3_bind_double(sql_insert_, 15,
                                event.kaon_candidates[1].h_prob_pi);
  retval |= sqlite3_bind_int(sql_insert_, 16, event.kaon_candidates[1].h_charge);
  retval |= sqlite3_bind_double(sql_insert_, 17,
                                event.kaon_candidates[1].h_ip_chi2);
  retval |= sqlite3_bind_int(sql_insert_, 18,
                             event.kaon_candidates[1].h_is_muon);

  retval |= sqlite3_bind_double(sql_insert_, 19, event.kaon_candidates[2].h_px);
  retval |= sqlite3_bind_double(sql_insert_, 20, event.kaon_candidates[2].h_py);
  retval |= sqlite3_bind_double(sql_insert_, 21, event.kaon_candidates[2].h_pz);
  retval |= sqlite3_bind_double(sql_insert_, 22,
                                event.kaon_candidates[2].h_prob_k);
  retval |= sqlite3_bind_double(sql_insert_, 23,
                                event.kaon_candidates[2].h_prob_pi);
  retval |= sqlite3_bind_int(sql_insert_, 24, event.kaon_candidates[2].h_charge);
  retval |= sqlite3_bind_double(sql_insert_, 25,
                                event.kaon_candidates[2].h_ip_chi2);
  retval |= sqlite3_bind_int(sql_insert_, 26,
                             event.kaon_candidates[2].h_is_muon);

  assert(retval == SQLITE_OK);
  retval = sqlite3_step(sql_insert_);
  assert(retval == SQLITE_DONE);
  retval = sqlite3_reset(sql_insert_);
  assert(retval == SQLITE_OK);
}


void EventWriterSqlite::Close() {
  int retval = sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, nullptr);
  assert(retval == SQLITE_OK);
  sqlite3_close(db_);
  db_ = nullptr;
}


//------------------------------------------------------------------------------


void EventReaderSqlite::Open(const std::string &path) {
  assert(db_ == nullptr);
  //sqlite3_vfs_register(sqlite3_vfs_find("unix-nolock"), 1);
  int retval = sqlite3_open_v2(
    path.c_str(), &db_, SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX, nullptr);
  assert(retval == SQLITE_OK);

  retval = sqlite3_prepare_v2(db_, "SELECT "
   "H1_PX, H1_PY, H1_PZ, H1_ProbK, H1_ProbPi, H1_Charge, H1_isMuon, H2_PX, "
   "H2_PY, H2_PZ, H2_ProbK, H2_ProbPi, H2_Charge, H2_isMuon, H3_PX, H3_PY, "
   "H3_PZ, H3_ProbK, H3_ProbPi, H3_Charge, H3_isMuon "
   "FROM events;", -1, &sql_select_, nullptr);
  assert(retval == SQLITE_OK);
}


bool EventReaderSqlite::NextEvent(Event *event) {
  int retval = sqlite3_step(sql_select_);
  assert((retval == SQLITE_DONE) || (retval == SQLITE_ROW));
  bool has_more_data = (retval == SQLITE_ROW);
  sqlite3_stmt *s = sql_select_;  // less typing

  event->kaon_candidates[0].h_is_muon = sqlite3_column_int(s, 6);
  if (event->kaon_candidates[0].h_is_muon) return has_more_data;
  event->kaon_candidates[1].h_is_muon = sqlite3_column_int(s, 13);
  if (event->kaon_candidates[1].h_is_muon) return has_more_data;
  event->kaon_candidates[2].h_is_muon = sqlite3_column_int(s, 20);
  if (event->kaon_candidates[2].h_is_muon) return has_more_data;

  event->kaon_candidates[0].h_px = sqlite3_column_double(s, 0);
  event->kaon_candidates[0].h_py = sqlite3_column_double(s, 1);
  event->kaon_candidates[0].h_pz = sqlite3_column_double(s, 2);
  event->kaon_candidates[0].h_prob_k = sqlite3_column_double(s, 3);
  event->kaon_candidates[0].h_prob_pi = sqlite3_column_double(s, 4);
  event->kaon_candidates[0].h_charge = sqlite3_column_int(s, 5);
  event->kaon_candidates[1].h_px = sqlite3_column_double(s, 7);
  event->kaon_candidates[1].h_py = sqlite3_column_double(s, 8);
  event->kaon_candidates[1].h_pz = sqlite3_column_double(s, 9);
  event->kaon_candidates[1].h_prob_k = sqlite3_column_double(s, 10);
  event->kaon_candidates[1].h_prob_pi = sqlite3_column_double(s, 11);
  event->kaon_candidates[1].h_charge = sqlite3_column_int(s, 12);
  event->kaon_candidates[2].h_px = sqlite3_column_double(s, 14);
  event->kaon_candidates[2].h_py = sqlite3_column_double(s, 15);
  event->kaon_candidates[2].h_pz = sqlite3_column_double(s, 16);
  event->kaon_candidates[2].h_prob_k = sqlite3_column_double(s, 17);
  event->kaon_candidates[2].h_prob_pi = sqlite3_column_double(s, 18);
  event->kaon_candidates[2].h_charge = sqlite3_column_int(s, 19);

  return has_more_data;
}


//------------------------------------------------------------------------------


H5Row::H5Row() {
  type_id_ = H5Tcreate(H5T_COMPOUND, sizeof(DataSet));
  assert(type_id_ >= 0);
  H5Tinsert(type_id_, "B_FlightDistance", HOFFSET(DataSet, b_flight_distance),
            H5T_NATIVE_DOUBLE);
  H5Tinsert(type_id_, "B_VertexChi2", HOFFSET(DataSet, b_vertex_chi2),
            H5T_NATIVE_DOUBLE);
  H5Tinsert(type_id_, "H1_PX", HOFFSET(DataSet, h1_px), H5T_NATIVE_DOUBLE);
  H5Tinsert(type_id_, "H1_PY", HOFFSET(DataSet, h1_py), H5T_NATIVE_DOUBLE);
  H5Tinsert(type_id_, "H1_PZ", HOFFSET(DataSet, h1_pz), H5T_NATIVE_DOUBLE);
  H5Tinsert(type_id_, "H1_ProbK", HOFFSET(DataSet, h1_prob_k),
            H5T_NATIVE_DOUBLE);
  H5Tinsert(type_id_, "H1_ProbPi", HOFFSET(DataSet, h1_prob_pi),
            H5T_NATIVE_DOUBLE);
  H5Tinsert(type_id_, "H1_Charge", HOFFSET(DataSet, h1_charge), H5T_NATIVE_INT);
  H5Tinsert(type_id_, "H1_isMuon", HOFFSET(DataSet, h1_is_muon),
            H5T_NATIVE_INT);
  H5Tinsert(type_id_, "H1_IpChi2", HOFFSET(DataSet, h1_ip_chi2),
            H5T_NATIVE_DOUBLE);
  H5Tinsert(type_id_, "H2_PX", HOFFSET(DataSet, h2_px), H5T_NATIVE_DOUBLE);
  H5Tinsert(type_id_, "H2_PY", HOFFSET(DataSet, h2_py), H5T_NATIVE_DOUBLE);
  H5Tinsert(type_id_, "H2_PZ", HOFFSET(DataSet, h2_pz), H5T_NATIVE_DOUBLE);
  H5Tinsert(type_id_, "H2_ProbK", HOFFSET(DataSet, h2_prob_k),
            H5T_NATIVE_DOUBLE);
  H5Tinsert(type_id_, "H2_ProbPi", HOFFSET(DataSet, h2_prob_pi),
            H5T_NATIVE_DOUBLE);
  H5Tinsert(type_id_, "H2_Charge", HOFFSET(DataSet, h2_charge), H5T_NATIVE_INT);
  H5Tinsert(type_id_, "H2_isMuon", HOFFSET(DataSet, h2_is_muon),
            H5T_NATIVE_INT);
  H5Tinsert(type_id_, "H2_IpChi2", HOFFSET(DataSet, h2_ip_chi2),
            H5T_NATIVE_DOUBLE);
  H5Tinsert(type_id_, "H3_PX", HOFFSET(DataSet, h3_px), H5T_NATIVE_DOUBLE);
  H5Tinsert(type_id_, "H3_PY", HOFFSET(DataSet, h3_py), H5T_NATIVE_DOUBLE);
  H5Tinsert(type_id_, "H3_PZ", HOFFSET(DataSet, h3_pz), H5T_NATIVE_DOUBLE);
  H5Tinsert(type_id_, "H3_ProbK", HOFFSET(DataSet, h3_prob_k),
            H5T_NATIVE_DOUBLE);
  H5Tinsert(type_id_, "H3_ProbPi", HOFFSET(DataSet, h3_prob_pi),
            H5T_NATIVE_DOUBLE);
  H5Tinsert(type_id_, "H3_Charge", HOFFSET(DataSet, h3_charge), H5T_NATIVE_INT);
  H5Tinsert(type_id_, "H3_isMuon", HOFFSET(DataSet, h3_is_muon),
            H5T_NATIVE_INT);
  H5Tinsert(type_id_, "H3_IpChi2", HOFFSET(DataSet, h3_ip_chi2),
            H5T_NATIVE_DOUBLE);
}


H5Row::~H5Row() {
  H5Tclose(type_id_);
}


const hsize_t H5Row::kDimension = 8556118;


//------------------------------------------------------------------------------

H5Column::H5Column() {
  space_id_ = H5Screate_simple(1, &kDimension, NULL);
  assert(space_id_ >= 0);
}

H5Column::~H5Column() {
  H5Dclose(sid_b_flight_distance_);
  H5Sclose(space_id_);
}

void H5Column::CreateSetDouble(const char *name, hid_t file_id, hid_t *set_id) {
  *set_id = H5Dcreate(file_id, name, H5T_NATIVE_DOUBLE, space_id_,
                      H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  assert(*set_id >= 0);
}

void H5Column::CreateSetInt(const char *name, hid_t file_id, hid_t *set_id) {
  *set_id = H5Dcreate(file_id, name, H5T_NATIVE_INT, space_id_,
                      H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  assert(*set_id >= 0);
}

void H5Column::OpenSet(const char *name, hid_t file_id, hid_t *set_id) {
  *set_id = H5Dopen(file_id, name, H5P_DEFAULT);
  assert(*set_id >= 0);
}

void H5Column::SetupColumns(hid_t file_id) {
  CreateSetDouble("/b_flight_distance", file_id, &sid_b_flight_distance_);
  CreateSetDouble("/b_vertex_chi2", file_id, &sid_b_vertex_chi2_);
  CreateSetDouble("/h1_px", file_id, &sid_h1_px_);
  CreateSetDouble("/h1_py", file_id, &sid_h1_py_);
  CreateSetDouble("/h1_pz", file_id, &sid_h1_pz_);
  CreateSetDouble("/h1_prob_k", file_id, &sid_h1_prob_k_);
  CreateSetDouble("/h1_prob_pi", file_id, &sid_h1_prob_pi_);
  CreateSetInt("/h1_charge", file_id, &sid_h1_charge_);
  CreateSetInt("/h1_is_muon", file_id, &sid_h1_is_muon_);
  CreateSetDouble("/h1_ip_chi2", file_id, &sid_h1_ip_chi2_);
  CreateSetDouble("/h2_px", file_id, &sid_h2_px_);
  CreateSetDouble("/h2_py", file_id, &sid_h2_py_);
  CreateSetDouble("/h2_pz", file_id, &sid_h2_pz_);
  CreateSetDouble("/h2_prob_k", file_id, &sid_h2_prob_k_);
  CreateSetDouble("/h2_prob_pi", file_id, &sid_h2_prob_pi_);
  CreateSetInt("/h2_charge", file_id, &sid_h2_charge_);
  CreateSetInt("/h2_is_muon", file_id, &sid_h2_is_muon_);
  CreateSetDouble("/h2_ip_chi2", file_id, &sid_h2_ip_chi2_);
  CreateSetDouble("/h3_px", file_id, &sid_h3_px_);
  CreateSetDouble("/h3_py", file_id, &sid_h3_py_);
  CreateSetDouble("/h3_pz", file_id, &sid_h3_pz_);
  CreateSetDouble("/h3_prob_k", file_id, &sid_h3_prob_k_);
  CreateSetDouble("/h3_prob_pi", file_id, &sid_h3_prob_pi_);
  CreateSetInt("/h3_charge", file_id, &sid_h3_charge_);
  CreateSetInt("/h3_is_muon", file_id, &sid_h3_is_muon_);
  CreateSetDouble("/h3_ip_chi2", file_id, &sid_h3_ip_chi2_);
}

void H5Column::OpenColumns(hid_t file_id) {
  OpenSet("/b_flight_distance", file_id, &sid_b_flight_distance_);
  OpenSet("/b_vertex_chi2", file_id, &sid_b_vertex_chi2_);
  OpenSet("/h1_px", file_id, &sid_h1_px_);
  OpenSet("/h1_py", file_id, &sid_h1_py_);
  OpenSet("/h1_pz", file_id, &sid_h1_pz_);
  OpenSet("/h1_prob_k", file_id, &sid_h1_prob_k_);
  OpenSet("/h1_prob_pi", file_id, &sid_h1_prob_pi_);
  OpenSet("/h1_charge", file_id, &sid_h1_charge_);
  OpenSet("/h1_is_muon", file_id, &sid_h1_is_muon_);
  OpenSet("/h1_ip_chi2", file_id, &sid_h1_ip_chi2_);
  OpenSet("/h2_px", file_id, &sid_h2_px_);
  OpenSet("/h2_py", file_id, &sid_h2_py_);
  OpenSet("/h2_pz", file_id, &sid_h2_pz_);
  OpenSet("/h2_prob_k", file_id, &sid_h2_prob_k_);
  OpenSet("/h2_prob_pi", file_id, &sid_h2_prob_pi_);
  OpenSet("/h2_charge", file_id, &sid_h2_charge_);
  OpenSet("/h2_is_muon", file_id, &sid_h2_is_muon_);
  OpenSet("/h2_ip_chi2", file_id, &sid_h2_ip_chi2_);
  OpenSet("/h3_px", file_id, &sid_h3_px_);
  OpenSet("/h3_py", file_id, &sid_h3_py_);
  OpenSet("/h3_pz", file_id, &sid_h3_pz_);
  OpenSet("/h3_prob_k", file_id, &sid_h3_prob_k_);
  OpenSet("/h3_prob_pi", file_id, &sid_h3_prob_pi_);
  OpenSet("/h3_charge", file_id, &sid_h3_charge_);
  OpenSet("/h3_is_muon", file_id, &sid_h3_is_muon_);
  OpenSet("/h3_ip_chi2", file_id, &sid_h3_ip_chi2_);
}

const hsize_t H5Column::kDimension = 8556118;


//------------------------------------------------------------------------------


void EventReaderH5Row::Open(const std::string &path) {
  file_id_ = H5Fopen(path.c_str(), H5P_DEFAULT, H5F_ACC_RDONLY);
  assert(file_id_ >= 0);
  set_id_ = H5Dopen(file_id_, "/DecayTree", H5P_DEFAULT);
  assert(set_id_ >= 0);
  space_id_ = H5Screate_simple(1, &kDimension, NULL);
  assert(space_id_ >= 0);
  mem_space_id_ = H5Screate(H5S_SCALAR);
  assert(mem_space_id_ >= 0);
}

bool EventReaderH5Row::NextEvent(Event *event) {
  if (nevent_ >= kDimension)
    return false;

  DataSet dataset;

  hsize_t count = 1;
  herr_t retval;
  retval = H5Sselect_hyperslab(
    space_id_, H5S_SELECT_SET, &nevent_, NULL, &count, NULL);
  assert(retval >= 0);

  retval = H5Dread(set_id_, type_id_, mem_space_id_, space_id_, H5P_DEFAULT,
                   &dataset);
  assert(retval >= 0);
  nevent_++;

  event->b_flight_distance = dataset.b_flight_distance;
  event->b_vertex_chi2 = dataset.b_vertex_chi2;
  event->kaon_candidates[0].h_px = dataset.h1_px;
  event->kaon_candidates[0].h_py = dataset.h1_py;
  event->kaon_candidates[0].h_pz = dataset.h1_pz;
  event->kaon_candidates[0].h_prob_k = dataset.h1_prob_k;
  event->kaon_candidates[0].h_prob_pi = dataset.h1_prob_pi;
  event->kaon_candidates[0].h_charge = dataset.h1_charge;
  event->kaon_candidates[0].h_is_muon = dataset.h1_is_muon;
  event->kaon_candidates[0].h_ip_chi2 = dataset.h1_ip_chi2;
  event->kaon_candidates[1].h_px = dataset.h2_px;
  event->kaon_candidates[1].h_py = dataset.h2_py;
  event->kaon_candidates[1].h_pz = dataset.h2_pz;
  event->kaon_candidates[1].h_prob_k = dataset.h2_prob_k;
  event->kaon_candidates[1].h_prob_pi = dataset.h2_prob_pi;
  event->kaon_candidates[1].h_charge = dataset.h2_charge;
  event->kaon_candidates[1].h_is_muon = dataset.h2_is_muon;
  event->kaon_candidates[1].h_ip_chi2 = dataset.h2_ip_chi2;
  event->kaon_candidates[2].h_px = dataset.h3_px;
  event->kaon_candidates[2].h_py = dataset.h3_py;
  event->kaon_candidates[2].h_pz = dataset.h3_pz;
  event->kaon_candidates[2].h_prob_k = dataset.h3_prob_k;
  event->kaon_candidates[2].h_prob_pi = dataset.h3_prob_pi;
  event->kaon_candidates[2].h_charge = dataset.h3_charge;
  event->kaon_candidates[2].h_is_muon = dataset.h3_is_muon;
  event->kaon_candidates[2].h_ip_chi2 = dataset.h3_ip_chi2;

  return true;
}


//------------------------------------------------------------------------------


void EventReaderH5Column::Open(const std::string &path) {
  file_id_ = H5Fopen(path.c_str(), H5P_DEFAULT, H5F_ACC_RDONLY);
  assert(file_id_ >= 0);

  mem_space_id_ = H5Screate(H5S_SCALAR);
  assert(mem_space_id_ >= 0);

  OpenColumns(file_id_);
}


double EventReaderH5Column::ReadDouble(hid_t set_id) {
  double result;
  herr_t retval;
  retval = H5Dread(set_id, H5T_NATIVE_DOUBLE, mem_space_id_, space_id_,
                   H5P_DEFAULT, &result);
  assert(retval >= 0);
  return result;
}


int EventReaderH5Column::ReadInt(hid_t set_id) {
  int result;
  herr_t retval;
  retval = H5Dread(set_id, H5T_NATIVE_INT, mem_space_id_, space_id_,
                   H5P_DEFAULT, &result);
  assert(retval >= 0);
  return result;
}


bool EventReaderH5Column::NextEvent(Event *event) {
  if (nevent_ >= kDimension)
    return false;

  hsize_t count = 1;
  herr_t retval;
  retval = H5Sselect_hyperslab(
    space_id_, H5S_SELECT_SET, &nevent_, NULL, &count, NULL);
  assert(retval >= 0);

  event->kaon_candidates[0].h_is_muon = ReadInt(sid_h1_is_muon_);
  if (event->kaon_candidates[0].h_is_muon) goto next_event_return;
  event->kaon_candidates[1].h_is_muon = ReadInt(sid_h2_is_muon_);
  if (event->kaon_candidates[1].h_is_muon) goto next_event_return;
  event->kaon_candidates[2].h_is_muon = ReadInt(sid_h3_is_muon_);
  if (event->kaon_candidates[2].h_is_muon) goto next_event_return;

  event->kaon_candidates[0].h_px = ReadDouble(sid_h1_px_);
  event->kaon_candidates[0].h_py = ReadDouble(sid_h1_py_);
  event->kaon_candidates[0].h_pz = ReadDouble(sid_h1_pz_);
  event->kaon_candidates[0].h_prob_k = ReadDouble(sid_h1_prob_k_);
  event->kaon_candidates[0].h_prob_pi = ReadDouble(sid_h1_prob_pi_);
  event->kaon_candidates[0].h_charge = ReadInt(sid_h1_charge_);
  event->kaon_candidates[1].h_px = ReadDouble(sid_h2_px_);
  event->kaon_candidates[1].h_py = ReadDouble(sid_h2_py_);
  event->kaon_candidates[1].h_pz = ReadDouble(sid_h2_pz_);
  event->kaon_candidates[1].h_prob_k = ReadDouble(sid_h2_prob_k_);
  event->kaon_candidates[1].h_prob_pi = ReadDouble(sid_h2_prob_pi_);
  event->kaon_candidates[1].h_charge = ReadInt(sid_h2_charge_);
  event->kaon_candidates[2].h_px = ReadDouble(sid_h3_px_);
  event->kaon_candidates[2].h_py = ReadDouble(sid_h3_py_);
  event->kaon_candidates[2].h_pz = ReadDouble(sid_h3_pz_);
  event->kaon_candidates[2].h_prob_k = ReadDouble(sid_h3_prob_k_);
  event->kaon_candidates[2].h_prob_pi = ReadDouble(sid_h3_prob_pi_);
  event->kaon_candidates[2].h_charge = ReadInt(sid_h3_charge_);

 next_event_return:
  nevent_++;
  return true;
}


//------------------------------------------------------------------------------


void EventReaderParquet::Open(const std::string &path) {
  parquet_reader_ = parquet::ParquetFileReader::OpenFile(path, false);
  std::shared_ptr<parquet::FileMetaData> file_metadata =
    parquet_reader_->metadata();

  num_row_groups_ = file_metadata->num_row_groups();
  assert(num_row_groups_ > 0);
}

#define RD_DOUBLE(X, Y) { parquet::DoubleReader *reader = \
  static_cast<parquet::DoubleReader *>(X.get()); \
  int64_t nval; \
  int64_t nread = reader->ReadBatch(1, nullptr, nullptr, &Y, &nval); \
  assert(nread == 1); assert(nval == 1); }

#define RD_BOOL(X, Y) { parquet::Int32Reader *reader = \
  static_cast<parquet::Int32Reader *>(X.get()); \
  int64_t nval; int result; \
  int64_t nread = reader->ReadBatch(1, nullptr, nullptr, &result, &nval); \
  assert(nread == 1); assert(nval == 1); \
  Y = result; }

#define SKIP_DOUBLE(X) { parquet::DoubleReader *reader = \
  static_cast<parquet::DoubleReader *>(X.get()); \
  int nskip = reader->Skip(1); assert(nskip == 1); }

bool EventReaderParquet::NextEvent(Event *event) {
  if (nevent_ == H5Row::kDimension)
    return false;

  if (nevent_ % EventWriterParquet::kNumRowsPerGroup == 0) {
    rg_reader_ = parquet_reader_->RowGroup(nrowgroup_);
    rd_h1_px_ = rg_reader_->Column(2);
    rd_h1_py_ = rg_reader_->Column(3);
    rd_h1_pz_ = rg_reader_->Column(4);
    rd_h1_prob_k_ = rg_reader_->Column(5);
    rd_h1_prob_pi_ = rg_reader_->Column(6);
    rd_h1_charge_ = rg_reader_->Column(7);
    rd_h1_is_muon_ = rg_reader_->Column(8);
    rd_h2_px_ = rg_reader_->Column(10);
    rd_h2_py_ = rg_reader_->Column(11);
    rd_h2_pz_ = rg_reader_->Column(12);
    rd_h2_prob_k_ = rg_reader_->Column(13);
    rd_h2_prob_pi_ = rg_reader_->Column(14);
    rd_h2_charge_ = rg_reader_->Column(15);
    rd_h2_is_muon_ = rg_reader_->Column(16);
    rd_h3_px_ = rg_reader_->Column(18);
    rd_h3_py_ = rg_reader_->Column(19);
    rd_h3_pz_ = rg_reader_->Column(20);
    rd_h3_prob_k_ = rg_reader_->Column(21);
    rd_h3_prob_pi_ = rg_reader_->Column(22);
    rd_h3_charge_ = rg_reader_->Column(23);
    rd_h3_is_muon_ = rg_reader_->Column(24);
    nrowgroup_++;
  }

  nevent_++;

  // Not possible to skip the other rows in lockstep
  RD_BOOL(rd_h1_is_muon_, event->kaon_candidates[0].h_is_muon);
  RD_BOOL(rd_h2_is_muon_, event->kaon_candidates[1].h_is_muon);
  RD_BOOL(rd_h3_is_muon_, event->kaon_candidates[2].h_is_muon);

  RD_DOUBLE(rd_h1_px_, event->kaon_candidates[0].h_px);
  RD_DOUBLE(rd_h1_py_, event->kaon_candidates[0].h_py);
  RD_DOUBLE(rd_h1_pz_, event->kaon_candidates[0].h_pz);
  RD_DOUBLE(rd_h1_prob_k_, event->kaon_candidates[0].h_prob_k);
  RD_DOUBLE(rd_h1_prob_pi_, event->kaon_candidates[0].h_prob_pi);
  RD_BOOL(rd_h1_charge_, event->kaon_candidates[0].h_charge);
  RD_DOUBLE(rd_h2_px_, event->kaon_candidates[1].h_px);
  RD_DOUBLE(rd_h2_py_, event->kaon_candidates[1].h_py);
  RD_DOUBLE(rd_h2_pz_, event->kaon_candidates[1].h_pz);
  RD_DOUBLE(rd_h2_prob_k_, event->kaon_candidates[1].h_prob_k);
  RD_DOUBLE(rd_h2_prob_pi_, event->kaon_candidates[1].h_prob_pi);
  RD_BOOL(rd_h2_charge_, event->kaon_candidates[1].h_charge);
  RD_DOUBLE(rd_h3_px_, event->kaon_candidates[2].h_px);
  RD_DOUBLE(rd_h3_py_, event->kaon_candidates[2].h_py);
  RD_DOUBLE(rd_h3_pz_, event->kaon_candidates[2].h_pz);
  RD_DOUBLE(rd_h3_prob_k_, event->kaon_candidates[2].h_prob_k);
  RD_DOUBLE(rd_h3_prob_pi_, event->kaon_candidates[2].h_prob_pi);
  RD_BOOL(rd_h3_charge_, event->kaon_candidates[2].h_charge);

  return true;
}


//------------------------------------------------------------------------------


void EventReaderProtobuf::Open(const std::string &path) {
  fd_ = open(path.c_str(), O_RDONLY);
  assert(fd_ >= 0);
  file_istream_ = new google::protobuf::io::FileInputStream(fd_);
  assert(file_istream_);
  if (compressed_) {
    gzip_istream_ = new google::protobuf::io::GzipInputStream(file_istream_);
    assert(gzip_istream_);
    istream_ = new google::protobuf::io::CodedInputStream(gzip_istream_);

  } else {
    istream_ = new google::protobuf::io::CodedInputStream(file_istream_);
  }
  assert(istream_);
}


bool EventReaderProtobuf::NextEvent(Event *event) {
  google::protobuf::uint32 size;
  bool has_next = istream_->ReadLittleEndian32(&size);
  if (!has_next)
    return false;

  google::protobuf::io::CodedInputStream::Limit limit =
    istream_->PushLimit(size);
  bool retval = pb_event_.ParseFromCodedStream(istream_);
  assert(retval);
  istream_->PopLimit(limit);

  event->kaon_candidates[0].h_is_muon = pb_event_.h1_is_muon();
  event->kaon_candidates[1].h_is_muon = pb_event_.h2_is_muon();
  event->kaon_candidates[2].h_is_muon = pb_event_.h3_is_muon();
  if (event->kaon_candidates[0].h_is_muon ||
      event->kaon_candidates[1].h_is_muon ||
      event->kaon_candidates[2].h_is_muon)
  {
    return true;
  }
  event->b_flight_distance = pb_event_.b_flight_distance();
  event->b_vertex_chi2 = pb_event_.b_vertex_chi2();
  event->kaon_candidates[0].h_px = pb_event_.h1_px();
  event->kaon_candidates[0].h_py = pb_event_.h1_py();
  event->kaon_candidates[0].h_pz = pb_event_.h1_pz();
  event->kaon_candidates[0].h_prob_k = pb_event_.h1_prob_k();
  event->kaon_candidates[0].h_prob_pi = pb_event_.h1_prob_pi();
  event->kaon_candidates[0].h_charge = pb_event_.h1_charge();
  event->kaon_candidates[0].h_ip_chi2 = pb_event_.h1_ip_chi2();
  event->kaon_candidates[1].h_px = pb_event_.h2_px();
  event->kaon_candidates[1].h_py = pb_event_.h2_py();
  event->kaon_candidates[1].h_pz = pb_event_.h2_pz();
  event->kaon_candidates[1].h_prob_k = pb_event_.h2_prob_k();
  event->kaon_candidates[1].h_prob_pi = pb_event_.h2_prob_pi();
  event->kaon_candidates[1].h_charge = pb_event_.h2_charge();
  event->kaon_candidates[1].h_ip_chi2 = pb_event_.h2_ip_chi2();
  event->kaon_candidates[2].h_px = pb_event_.h3_px();
  event->kaon_candidates[2].h_py = pb_event_.h3_py();
  event->kaon_candidates[2].h_pz = pb_event_.h3_pz();
  event->kaon_candidates[2].h_prob_k = pb_event_.h3_prob_k();
  event->kaon_candidates[2].h_prob_pi = pb_event_.h3_prob_pi();
  event->kaon_candidates[2].h_charge = pb_event_.h3_charge();
  event->kaon_candidates[2].h_ip_chi2 = pb_event_.h3_ip_chi2();

  return true;
}


//------------------------------------------------------------------------------


void EventReaderAvro::Open(const std::string &path) {
  int retval = avro_file_reader(path.c_str(), &db_);
  assert(retval == 0);

  proj_schema_ = avro_schema_record("event", "lhcb.cern.ch");
  retval =
    AVRO_APP(proj_schema_, "h1_px", avro_schema_double()) ||
    AVRO_APP(proj_schema_, "h1_py", avro_schema_double()) ||
    AVRO_APP(proj_schema_, "h1_pz", avro_schema_double()) ||
    AVRO_APP(proj_schema_, "h1_prob_k", avro_schema_double()) ||
    AVRO_APP(proj_schema_, "h1_prob_pi", avro_schema_double()) ||
    AVRO_APP(proj_schema_, "h1_charge", avro_schema_int()) ||
    AVRO_APP(proj_schema_, "h1_is_muon", avro_schema_int()) ||
    AVRO_APP(proj_schema_, "h2_px", avro_schema_double()) ||
    AVRO_APP(proj_schema_, "h2_py", avro_schema_double()) ||
    AVRO_APP(proj_schema_, "h2_pz", avro_schema_double()) ||
    AVRO_APP(proj_schema_, "h2_prob_k", avro_schema_double()) ||
    AVRO_APP(proj_schema_, "h2_prob_pi", avro_schema_double()) ||
    AVRO_APP(proj_schema_, "h2_charge", avro_schema_int()) ||
    AVRO_APP(proj_schema_, "h2_is_muon", avro_schema_int()) ||
    AVRO_APP(proj_schema_, "h3_px", avro_schema_double()) ||
    AVRO_APP(proj_schema_, "h3_py", avro_schema_double()) ||
    AVRO_APP(proj_schema_, "h3_pz", avro_schema_double()) ||
    AVRO_APP(proj_schema_, "h3_prob_k", avro_schema_double()) ||
    AVRO_APP(proj_schema_, "h3_prob_pi", avro_schema_double()) ||
    AVRO_APP(proj_schema_, "h3_charge", avro_schema_int()) ||
    AVRO_APP(proj_schema_, "h3_is_muon", avro_schema_int());

  assert(retval == 0);
}


bool EventReaderAvro::NextEvent(Event *event) {
  avro_datum_t record;
  int retval = avro_file_reader_read(db_, proj_schema_, &record);
  if (retval != 0)
    return false;

  avro_datum_decref(record);

  return true;
}


//------------------------------------------------------------------------------


void EventReaderRoot::Open(const std::string &path) {
  root_chain_ = new TChain("DecayTree");
  std::vector<std::string> vec_paths = SplitString(path, ':');
  for (const auto &p : vec_paths)
    root_chain_->Add(p.c_str());
}


bool EventReaderRoot::NextEvent(Event *event) {
  if (num_events_ < 0) {
    AttachBranches2Event(event);
    num_events_ = root_chain_->GetEntries();
    pos_events_ = 0;
  }
  if (pos_events_ >= num_events_)
    return false;
  root_chain_->GetEntry(pos_events_);
  pos_events_++;
  return true;
}


void EventReaderRoot::AttachBranches2Event(Event *event) {
  root_chain_->SetBranchAddress("H1_PX", &event->kaon_candidates[0].h_px);
  root_chain_->SetBranchAddress("H1_PY", &event->kaon_candidates[0].h_py);
  root_chain_->SetBranchAddress("H1_PZ", &event->kaon_candidates[0].h_pz);
  root_chain_->SetBranchAddress("H1_ProbK",
                                &event->kaon_candidates[0].h_prob_k);
  root_chain_->SetBranchAddress("H1_ProbPi",
                                &event->kaon_candidates[0].h_prob_pi);
  root_chain_->SetBranchAddress("H1_Charge",
                                &event->kaon_candidates[0].h_charge);
  root_chain_->SetBranchAddress("H1_isMuon",
                                &event->kaon_candidates[0].h_is_muon);

  root_chain_->SetBranchAddress("H2_PX", &event->kaon_candidates[1].h_px);
  root_chain_->SetBranchAddress("H2_PY", &event->kaon_candidates[1].h_py);
  root_chain_->SetBranchAddress("H2_PZ", &event->kaon_candidates[1].h_pz);
  root_chain_->SetBranchAddress("H2_ProbK",
                                &event->kaon_candidates[1].h_prob_k);
  root_chain_->SetBranchAddress("H2_ProbPi",
                                &event->kaon_candidates[1].h_prob_pi);
  root_chain_->SetBranchAddress("H2_Charge",
                                &event->kaon_candidates[1].h_charge);
  root_chain_->SetBranchAddress("H2_isMuon",
                                &event->kaon_candidates[1].h_is_muon);

  root_chain_->SetBranchAddress("H3_PX", &event->kaon_candidates[2].h_px);
  root_chain_->SetBranchAddress("H3_PY", &event->kaon_candidates[2].h_py);
  root_chain_->SetBranchAddress("H3_PZ", &event->kaon_candidates[2].h_pz);
  root_chain_->SetBranchAddress("H3_ProbK",
                                &event->kaon_candidates[2].h_prob_k);
  root_chain_->SetBranchAddress("H3_ProbPi",
                                &event->kaon_candidates[2].h_prob_pi);
  root_chain_->SetBranchAddress("H3_Charge",
                                &event->kaon_candidates[2].h_charge);
  root_chain_->SetBranchAddress("H3_isMuon",
                                &event->kaon_candidates[2].h_is_muon);
}


/**
 * Not used for the analysis but required for converting into another file
 * format.
 */
void EventReaderRoot::AttachUnusedBranches2Event(Event *event) {
  root_chain_->SetBranchAddress("B_FlightDistance", &event->b_flight_distance);
  root_chain_->SetBranchAddress("B_VertexChi2", &event->b_vertex_chi2);
  root_chain_->SetBranchAddress("H1_IPChi2",
                                &event->kaon_candidates[0].h_ip_chi2);
  root_chain_->SetBranchAddress("H2_IPChi2",
                                &event->kaon_candidates[1].h_ip_chi2);
  root_chain_->SetBranchAddress("H3_IPChi2",
                                &event->kaon_candidates[2].h_ip_chi2);
}


void EventReaderRoot::PrepareForConversion(Event *event) {
  AttachUnusedBranches2Event(event);
}


//------------------------------------------------------------------------------


unsigned g_skipped = 0;
static double ProcessEvent(const Event &event) {
  unsigned i = 0;
  for (const auto &k : event.kaon_candidates) {
    ++i;
    //printf("%d is muon %d\n", i, k.h_is_muon);
    if (k.h_is_muon) {
      g_skipped++;
      return 0.0;
    }
  }
  //abort();

  double result = 0.0;
  for (const auto &k : event.kaon_candidates) {
    result += k.h_px + k.h_py + k.h_pz + k.h_prob_k + k.h_prob_pi +
              double(k.h_charge);
  }
  return result;
}


int AnalyzeRootOptimized(const std::vector<std::string> &input_paths) {
  TChain root_chain("DecayTree");
  for (const auto &p : input_paths)
    root_chain.Add(p.c_str());
  TTreeReader reader(&root_chain);

  TTreeReaderValue<int> val_h1_is_muon(reader, "H1_isMuon");
  TTreeReaderValue<int> val_h2_is_muon(reader, "H2_isMuon");
  TTreeReaderValue<int> val_h3_is_muon(reader, "H3_isMuon");

  TTreeReaderValue<double> val_h1_px(reader, "H1_PX");
  TTreeReaderValue<double> val_h1_py(reader, "H1_PY");
  TTreeReaderValue<double> val_h1_pz(reader, "H1_PZ");
  TTreeReaderValue<double> val_h1_prob_k(reader, "H1_ProbK");
  TTreeReaderValue<double> val_h1_prob_pi(reader, "H1_ProbPi");
  TTreeReaderValue<int> val_h1_charge(reader, "H1_Charge");

  TTreeReaderValue<double> val_h2_px(reader, "H2_PX");
  TTreeReaderValue<double> val_h2_py(reader, "H2_PY");
  TTreeReaderValue<double> val_h2_pz(reader, "H2_PZ");
  TTreeReaderValue<double> val_h2_prob_k(reader, "H2_ProbK");
  TTreeReaderValue<double> val_h2_prob_pi(reader, "H2_ProbPi");
  TTreeReaderValue<int> val_h2_charge(reader, "H2_Charge");

  TTreeReaderValue<double> val_h3_px(reader, "H3_PX");
  TTreeReaderValue<double> val_h3_py(reader, "H3_PY");
  TTreeReaderValue<double> val_h3_pz(reader, "H3_PZ");
  TTreeReaderValue<double> val_h3_prob_k(reader, "H3_ProbK");
  TTreeReaderValue<double> val_h3_prob_pi(reader, "H3_ProbPi");
  TTreeReaderValue<int> val_h3_charge(reader, "H3_Charge");

  unsigned nread = 0;
  unsigned nskipped = 0;
  double dummy = 0.0;
  while (reader.Next()) {
    nread++;
    if (*val_h1_is_muon || *val_h2_is_muon || *val_h3_is_muon) {
      nskipped++;
      continue;
    }

    dummy +=
      *val_h1_px + *val_h1_py + *val_h1_pz + *val_h1_prob_k + *val_h1_prob_pi +
      double(*val_h1_charge) +
      *val_h2_px + *val_h2_py + *val_h2_pz + *val_h2_prob_k + *val_h2_prob_pi +
      double(*val_h2_charge) +
      *val_h3_px + *val_h3_py + *val_h3_pz + *val_h3_prob_k + *val_h3_prob_pi +
      double(*val_h3_charge);

    if ((nread % 100000) == 0) {
      printf("processed %u k events\n", nread / 1000);
      //printf("dummy is %lf\n", dummy); abort();
    }
  }
  printf("Optimized TTreeReader run: %u events read, %u events skipped "
         "(dummy: %lf)\n", nread, nskipped, dummy);

  return 0;
}


static void Usage(const char *progname) {
  printf("%s [-i input.root] [-i ...] [-r | -o output format]\n", progname);
}


int main(int argc, char **argv) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  std::vector<std::string> input_paths;
  std::string input_suffix;
  std::string output_suffix;
  bool root_optimized = false;
  int c;
  while ((c = getopt(argc, argv, "hvi:o:r")) != -1) {
    switch (c) {
      case 'h':
      case 'v':
        Usage(argv[0]);
        return 0;
      case 'i':
        input_paths.push_back(optarg);
        break;
      case 'o':
        output_suffix = optarg;
        break;
      case 'r':
        root_optimized = true;
        break;
      default:
        fprintf(stderr, "Unknown option: -%c\n", c);
        Usage(argv[0]);
        return 1;
    }
  }

  if (root_optimized) {
    return AnalyzeRootOptimized(input_paths);
  }

  assert(!input_paths.empty());
  input_suffix = GetSuffix(input_paths[0]);
  FileFormats input_format = GetFileFormat(input_suffix);
  std::unique_ptr<EventReader> event_reader {EventReader::Create(input_format)};
  event_reader->Open(JoinStrings(input_paths, ":"));

  Event event;

  std::unique_ptr<EventWriter> event_writer{nullptr};
  if (!output_suffix.empty()) {
    assert(input_format == FileFormats::kRoot);
    event_reader->PrepareForConversion(&event);

    FileFormats output_format = GetFileFormat(output_suffix);
    assert(output_format != FileFormats::kRoot);
    event_writer = EventWriter::Create(output_format);
    event_writer->Open(StripSuffix(input_paths[0]) + "." + output_suffix);
  }

  unsigned i_events = 0;
  double dummy = 0.0;
  while (event_reader->NextEvent(&event)) {
    if (event_writer) {
      event_writer->WriteEvent(event);
    } else {
      dummy += ProcessEvent(event);
    }
    if ((++i_events % 100000) == 0) {
      printf("processed %u k events\n", i_events / 1000);
      //printf("dummy is %lf\n", dummy); abort();
    }
  }

  printf("finished (%u events), result: %lf, skipped %u\n",
         i_events, dummy, g_skipped);
  if (event_writer) event_writer->Close();

  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}
