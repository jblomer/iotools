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
#include <chrono>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <Compression.h>
#ifndef HAS_LZ4
#include <ROOT/RDataFrame.hxx>
#endif
#include <TChain.h>
#include <TClassTable.h>
#include <TLeafElement.h>
#include <TSystem.h>
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
      return std::unique_ptr<EventWriter>(new EventWriterProtobuf(true, false));
    case FileFormats::kProtobufInflated:
      return std::unique_ptr<EventWriter>(
        new EventWriterProtobuf(false, false));
    case FileFormats::kProtobufDeepInflated:
      return std::unique_ptr<EventWriter>(
        new EventWriterProtobuf(false, true));
    case FileFormats::kRootDeflated:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(
        EventWriterRoot::CompressionAlgorithms::kCompressionDeflate,
        EventWriterRoot::SplitMode::kSplitManual));
    case FileFormats::kRootLz4:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(
        EventWriterRoot::CompressionAlgorithms::kCompressionLz4,
        EventWriterRoot::SplitMode::kSplitManual));
    case FileFormats::kRootLzma:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(
        EventWriterRoot::CompressionAlgorithms::kCompressionLzma,
        EventWriterRoot::SplitMode::kSplitManual));
    case FileFormats::kRootInflated:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(
        EventWriterRoot::CompressionAlgorithms::kCompressionNone,
        EventWriterRoot::SplitMode::kSplitManual));
    case FileFormats::kRootRow:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(
        EventWriterRoot::CompressionAlgorithms::kCompressionNone,
        EventWriterRoot::SplitMode::kSplitNone));
    case FileFormats::kRootAutosplitInflated:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(
        EventWriterRoot::CompressionAlgorithms::kCompressionNone,
        EventWriterRoot::SplitMode::kSplitAuto));
    case FileFormats::kRootAutosplitDeflated:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(
        EventWriterRoot::CompressionAlgorithms::kCompressionDeflate,
        EventWriterRoot::SplitMode::kSplitAuto));
    case FileFormats::kRootDeepsplitInflated:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(
        EventWriterRoot::CompressionAlgorithms::kCompressionNone,
        EventWriterRoot::SplitMode::kSplitDeep));
    case FileFormats::kRootDeepsplitDeflated:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(
        EventWriterRoot::CompressionAlgorithms::kCompressionDeflate,
        EventWriterRoot::SplitMode::kSplitDeep));
    case FileFormats::kRootDeepsplitLz4:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(
        EventWriterRoot::CompressionAlgorithms::kCompressionLz4,
        EventWriterRoot::SplitMode::kSplitDeep));
    case FileFormats::kRootCsplitInflated:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(
        EventWriterRoot::CompressionAlgorithms::kCompressionNone,
        EventWriterRoot::SplitMode::kSplitC));
    case FileFormats::kRootCsplitDeflated:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(
        EventWriterRoot::CompressionAlgorithms::kCompressionDeflate,
        EventWriterRoot::SplitMode::kSplitC));
    case FileFormats::kRootLeaflistInflated:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(
        EventWriterRoot::CompressionAlgorithms::kCompressionNone,
        EventWriterRoot::SplitMode::kSplitLeaflist));
    case FileFormats::kRootLeaflistDeflated:
      return std::unique_ptr<EventWriter>(new EventWriterRoot(
        EventWriterRoot::CompressionAlgorithms::kCompressionDeflate,
        EventWriterRoot::SplitMode::kSplitLeaflist));
    default:
      abort();
  }
}


std::unique_ptr<EventReader> EventReader::Create(FileFormats format) {
  switch (format) {
    case FileFormats::kRoot:
    case FileFormats::kRootDeflated:
    case FileFormats::kRootLz4:
    case FileFormats::kRootLzma:
    case FileFormats::kRootInflated:
      return std::unique_ptr<EventReader>(new EventReaderRoot(
        EventReaderRoot::SplitMode::kSplitManual));
    case FileFormats::kRootAutosplitInflated:
    case FileFormats::kRootAutosplitDeflated:
      return std::unique_ptr<EventReader>(new EventReaderRoot(
        EventReaderRoot::SplitMode::kSplitAuto));
    case FileFormats::kRootDeepsplitInflated:
    case FileFormats::kRootDeepsplitDeflated:
    case FileFormats::kRootDeepsplitLz4:
      return std::unique_ptr<EventReader>(new EventReaderRoot(
        EventReaderRoot::SplitMode::kSplitDeep));
    case FileFormats::kRootCsplitInflated:
    case FileFormats::kRootCsplitDeflated:
      return std::unique_ptr<EventReader>(new EventReaderRoot(
        EventReaderRoot::SplitMode::kSplitC));
    case FileFormats::kRootLeaflistInflated:
    case FileFormats::kRootLeaflistDeflated:
      return std::unique_ptr<EventReader>(new EventReaderRoot(
        EventReaderRoot::SplitMode::kSplitLeaflist));
    case FileFormats::kRootRow:
      return std::unique_ptr<EventReader>(new EventReaderRoot(
        EventReaderRoot::SplitMode::kSplitNone));
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
      return std::unique_ptr<EventReader>(new EventReaderProtobuf(true, false));
    case FileFormats::kProtobufInflated:
      return std::unique_ptr<EventReader>(
        new EventReaderProtobuf(false, false));
    case FileFormats::kProtobufDeepInflated:
      return std::unique_ptr<EventReader>(
        new EventReaderProtobuf(false, true));
    default:
      abort();
  }
}


//------------------------------------------------------------------------------


void EventWriterH5Row::Open(const std::string &path) {
  dimension_ = short_write_ ? kShortDimension : kDefaultDimension;

  file_id_ = H5Fcreate(path.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  assert(file_id_ >= 0);

  space_id_ = H5Screate_simple(1, &dimension_, NULL);
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
  if (is_deep_) WriteDeepEvent(event);
  else WriteFlatEvent(event);
}

void EventWriterProtobuf::WriteDeepEvent(const Event &event) {
  PbDeepEvent deep_event;
  deep_event.set_b_flight_distance(event.b_flight_distance);
  deep_event.set_b_vertex_chi2(event.b_vertex_chi2);
  for (unsigned i = 0; i < 3; ++i) {
    PbKaonCandidate *kaon_cand = deep_event.add_kaon_candidates();
    kaon_cand->set_h_px(event.kaon_candidates[i].h_px);
    kaon_cand->set_h_py(event.kaon_candidates[i].h_py);
    kaon_cand->set_h_pz(event.kaon_candidates[i].h_pz);
    kaon_cand->set_h_prob_k(event.kaon_candidates[i].h_prob_k);
    kaon_cand->set_h_prob_pi(event.kaon_candidates[i].h_prob_pi);
    kaon_cand->set_h_charge(event.kaon_candidates[i].h_charge);
    kaon_cand->set_h_is_muon(event.kaon_candidates[i].h_is_muon);
    kaon_cand->set_h_ip_chi2(event.kaon_candidates[i].h_ip_chi2);
  }
  ostream_->WriteLittleEndian32(deep_event.ByteSize());
  bool retval = deep_event.SerializeToCodedStream(ostream_);
  assert(retval);
}

void EventWriterProtobuf::WriteFlatEvent(const Event &event) {
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
  dimension_ = short_write_ ? kShortDimension : kDefaultDimension;

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
  WriteInt(sid_h1_charge_, &event.kaon_candidates[0].h_charge);
  WriteInt(sid_h1_is_muon_, &event.kaon_candidates[0].h_is_muon);
  WriteDouble(sid_h1_ip_chi2_, &event.kaon_candidates[0].h_ip_chi2);
  WriteDouble(sid_h2_px_, &event.kaon_candidates[1].h_px);
  WriteDouble(sid_h2_py_, &event.kaon_candidates[1].h_py);
  WriteDouble(sid_h2_pz_, &event.kaon_candidates[1].h_pz);
  WriteDouble(sid_h2_prob_k_, &event.kaon_candidates[1].h_prob_k);
  WriteDouble(sid_h2_prob_pi_, &event.kaon_candidates[1].h_prob_pi);
  WriteInt(sid_h2_charge_, &event.kaon_candidates[1].h_charge);
  WriteInt(sid_h2_is_muon_, &event.kaon_candidates[1].h_is_muon);
  WriteDouble(sid_h2_ip_chi2_, &event.kaon_candidates[1].h_ip_chi2);
  WriteDouble(sid_h3_px_, &event.kaon_candidates[2].h_px);
  WriteDouble(sid_h3_py_, &event.kaon_candidates[2].h_py);
  WriteDouble(sid_h3_pz_, &event.kaon_candidates[2].h_pz);
  WriteDouble(sid_h3_prob_k_, &event.kaon_candidates[2].h_prob_k);
  WriteDouble(sid_h3_prob_pi_, &event.kaon_candidates[2].h_prob_pi);
  WriteInt(sid_h3_charge_, &event.kaon_candidates[2].h_charge);
  WriteInt(sid_h3_is_muon_, &event.kaon_candidates[2].h_is_muon);
  WriteDouble(sid_h3_ip_chi2_, &event.kaon_candidates[2].h_ip_chi2);

  nevent_++;
}


void EventWriterH5Column::Close() {
  H5Sclose(mem_space_id_);
  H5Sclose(space_id_);
  H5Fclose(file_id_);
}


//------------------------------------------------------------------------------


void EventWriterRoot::Open(const std::string &path) {
  nevent = 0;
  flat_event_ = new FlatEvent();
  deep_event_ = new DeepEvent();
  csplit_event_ = new CSplitEvent();

  output_ = new TFile(path.c_str(), "RECREATE");
  switch (compression_) {
    case CompressionAlgorithms::kCompressionNone:
      output_->SetCompressionSettings(0);
      break;
    case CompressionAlgorithms::kCompressionDeflate:
      output_->SetCompressionSettings(104);
      break;
    case CompressionAlgorithms::kCompressionLz4:
      output_->SetCompressionSettings(ROOT::CompressionSettings(ROOT::kLZ4, 1));
      break;
    case CompressionAlgorithms::kCompressionLzma:
      output_->SetCompressionSettings(
        ROOT::CompressionSettings(ROOT::kLZMA, 1));
      break;
    default:
      abort();
  }

  if (split_mode_ == SplitMode::kSplitNone) {
    tree_ = new TTree("DecayTree", "");
    tree_->Branch("EventBranch", &flat_event_, 32000, 0);
    printf("WRITING WITH SPLIT LEVEL 0\n");
    return;
  } else if (split_mode_ == SplitMode::kSplitAuto) {
    tree_ = new TTree("DecayTree", "");
    tree_->Branch("EventBranch", &flat_event_, 32000, 99);
    printf("AUTO-SPLIT, FLAT WRITING WITH SPLIT LEVEL 99\n");
    return;
  } else if (split_mode_ == SplitMode::kSplitDeep) {
    tree_ = new TTree("DecayTree", "");
    tree_->SetBit(TTree::kOnlyFlushAtCluster);
    tree_->SetAutoFlush(133000);
    ROOT::TIOFeatures features;
    features.Set(ROOT::Experimental::EIOFeatures::kGenerateOffsetMap);
    tree_->SetIOFeatures(features);
    tree_->Branch("EventBranch", &deep_event_, 32000, 99);
    printf("AUTO-SPLIT, DEEP WRITING WITH SPLIT LEVEL 99\n");
    return;
  } else if (split_mode_ == SplitMode::kSplitC) {
    tree_ = new TTree("DecayTree", "");
    tree_->SetBit(TTree::kOnlyFlushAtCluster);
    tree_->SetAutoFlush(133000);
    ROOT::TIOFeatures features;
    features.Set(ROOT::Experimental::EIOFeatures::kGenerateOffsetMap);
    tree_->SetIOFeatures(features);
    tree_->Branch("EventBranch", &csplit_event_, 32000, 99);
    printf("AUTO-SPLIT, C-SPLIT WRITING WITH SPLIT LEVEL 99\n");
    return;
  } else if (split_mode_ == SplitMode::kSplitLeaflist) {
    tree_ = new TTree("DecayTree", "");
    tree_->SetAutoFlush(0);
    ROOT::TIOFeatures features;
    features.Set(ROOT::Experimental::EIOFeatures::kGenerateOffsetMap);
    tree_->SetIOFeatures(features);
    csplit_event_->FromEvent(Event());
    tree_->Branch("b_flight_distance", &(csplit_event_->b_flight_distance), "B_FlightDistance/D");
    tree_->Branch("b_vertex_chi2", &(csplit_event_->b_vertex_chi2), "b_vertex_chi2/D");
    tree_->Branch("nKaons", &(csplit_event_->nKaons), "nKaons/I");
    tree_->Branch("h_px", &(csplit_event_->h_px[0]), "h_px[nKaons]/D");
    tree_->Branch("h_py", &(csplit_event_->h_py[0]), "h_py[nKaons]/D");
    tree_->Branch("h_pz", &(csplit_event_->h_pz[0]), "h_pz[nKaons]/D");
    tree_->Branch("h_prob_k", &(csplit_event_->h_prob_k[0]), "h_prob_k[nKaons]/D");
    tree_->Branch("h_prob_pi", &(csplit_event_->h_prob_pi[0]), "h_prob_pi[nKaons]/D");
    tree_->Branch("h_charge", &(csplit_event_->h_charge[0]), "h_charge[nKaons]/I");
    tree_->Branch("h_is_muon", &(csplit_event_->h_is_muon[0]), "h_is_muon[nKaons]/I");
    tree_->Branch("h_ip_chi2", &(csplit_event_->h_ip_chi2[0]), "h_ip_chi2[nKaons]/D");
    return;
  }

  tree_ = new TTree("DecayTree", "");
  tree_->SetAutoFlush(0);
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
  if (split_mode_ != SplitMode::kSplitManual) {
    if (split_mode_ == SplitMode::kSplitDeep) {
      deep_event_->FromEvent(event);
    } else if (split_mode_ == SplitMode::kSplitC) {
      csplit_event_->FromEvent(event);
    } else if (split_mode_ == SplitMode::kSplitLeaflist) {
      csplit_event_->FromEvent(event);
    } else {
      flat_event_->FromEvent(event);
    }
  } else {
    event_ = event;
  }

  tree_->Fill();
  nevent++;
}


void EventWriterRoot::Close() {
  output_ = tree_->GetCurrentFile();
  output_->Write();
  output_->Close();
  delete output_;
  delete flat_event_;
  delete deep_event_;
  delete csplit_event_;
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

  if (plot_only_) {
    retval = sqlite3_prepare_v2(db_,
     "SELECT H1_isMuon, H1_PX FROM events;", -1, &sql_select_, nullptr);
  } else {
    retval = sqlite3_prepare_v2(db_, "SELECT "
     "H1_PX, H1_PY, H1_PZ, H1_ProbK, H1_ProbPi, H1_Charge, H1_isMuon, H2_PX, "
     "H2_PY, H2_PZ, H2_ProbK, H2_ProbPi, H2_Charge, H2_isMuon, H3_PX, H3_PY, "
     "H3_PZ, H3_ProbK, H3_ProbPi, H3_Charge, H3_isMuon "
     "FROM events;", -1, &sql_select_, nullptr);
  }
  assert(retval == SQLITE_OK);
}


bool EventReaderSqlite::NextEvent(Event *event) {
  int retval = sqlite3_step(sql_select_);
  assert((retval == SQLITE_DONE) || (retval == SQLITE_ROW));
  bool has_more_data = (retval == SQLITE_ROW);
  sqlite3_stmt *s = sql_select_;  // less typing

  if (plot_only_) {
    event->kaon_candidates[0].h_is_muon = sqlite3_column_int(s, 0);
    if (event->kaon_candidates[0].h_is_muon) return has_more_data;
    event->kaon_candidates[0].h_px = sqlite3_column_double(s, 1);
    return has_more_data;
  }

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


const hsize_t H5Row::kDefaultDimension = 8556118;
const hsize_t H5Row::kShortDimension = 500000;


//------------------------------------------------------------------------------

H5Column::H5Column() : dimension_(0) { }

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
  space_id_ = H5Screate_simple(1, &dimension_, NULL);
  assert(space_id_ >= 0);

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
  space_id_ = H5Screate_simple(1, &dimension_, NULL);
  assert(space_id_ >= 0);

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

const hsize_t H5Column::kDefaultDimension = 8556118;
const hsize_t H5Column::kShortDimension = 500000;


//------------------------------------------------------------------------------


void EventReaderH5Row::Open(const std::string &path) {
  dimension_ = short_read_ ? kShortDimension : kDefaultDimension;

  file_id_ = H5Fopen(path.c_str(), H5P_DEFAULT, H5F_ACC_RDONLY);
  assert(file_id_ >= 0);
  set_id_ = H5Dopen(file_id_, "/DecayTree", H5P_DEFAULT);
  assert(set_id_ >= 0);
  space_id_ = H5Screate_simple(1, &dimension_, NULL);
  assert(space_id_ >= 0);
  mem_space_id_ = H5Screate(H5S_SCALAR);
  assert(mem_space_id_ >= 0);
}

bool EventReaderH5Row::NextEvent(Event *event) {
  if (nevent_ >= dimension_)
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
  dimension_ = short_read_ ? kShortDimension : kDefaultDimension;
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
  if (nevent_ >= dimension_)
    return false;

  hsize_t count = 1;
  herr_t retval;
  retval = H5Sselect_hyperslab(
    space_id_, H5S_SELECT_SET, &nevent_, NULL, &count, NULL);
  assert(retval >= 0);

  event->kaon_candidates[0].h_is_muon = ReadInt(sid_h1_is_muon_);
  if (event->kaon_candidates[0].h_is_muon) goto next_event_return;

  if (plot_only_) {
    event->kaon_candidates[0].h_px = ReadDouble(sid_h1_px_);
    goto next_event_return;
  }

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



void EventReaderProtobuf::Open(const std::string &path) {
  fd_ = open(path.c_str(), O_RDONLY);
  assert(fd_ >= 0);
  file_istream_ = new google::protobuf::io::FileInputStream(fd_);
  assert(file_istream_);
  if (compressed_) {
    gzip_istream_ = new google::protobuf::io::GzipInputStream(file_istream_);
    assert(gzip_istream_);
    istream_ = gzip_istream_;
  } else {
    istream_ = file_istream_;
  }
}


bool EventReaderProtobuf::SerializeFlatEvent(Event *event) {
  event->kaon_candidates[0].h_is_muon = pb_event_.h1_is_muon();
  if (event->kaon_candidates[0].h_is_muon) return true;

  if (plot_only_) {
    event->kaon_candidates[0].h_px = pb_event_.h1_px();
    return true;
  }

  event->kaon_candidates[1].h_is_muon = pb_event_.h2_is_muon();
  if (event->kaon_candidates[1].h_is_muon) return true;
  event->kaon_candidates[2].h_is_muon = pb_event_.h3_is_muon();
  if (event->kaon_candidates[2].h_is_muon) return true;

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


bool EventReaderProtobuf::SerializeDeepEvent(Event *event) {
  assert(pb_deep_event_.kaon_candidates().size() == 3);
  event->kaon_candidates[0].h_is_muon =
    pb_deep_event_.kaon_candidates()[0].h_is_muon();
  if (event->kaon_candidates[0].h_is_muon) return true;

  if (plot_only_) {
    event->kaon_candidates[0].h_px =
      pb_deep_event_.kaon_candidates()[0].h_px();
    return true;
  }

  event->kaon_candidates[1].h_is_muon =
    pb_deep_event_.kaon_candidates()[1].h_is_muon();
  if (event->kaon_candidates[1].h_is_muon) return true;
  event->kaon_candidates[2].h_is_muon =
    pb_deep_event_.kaon_candidates()[2].h_is_muon();
  if (event->kaon_candidates[2].h_is_muon) return true;

  event->b_flight_distance = pb_deep_event_.b_flight_distance();
  event->b_vertex_chi2 = pb_deep_event_.b_vertex_chi2();
  for (unsigned i = 0; i < 3; ++i) {
    event->kaon_candidates[i].h_px = pb_deep_event_.kaon_candidates()[i].h_px();
    event->kaon_candidates[i].h_py = pb_deep_event_.kaon_candidates()[i].h_py();
    event->kaon_candidates[i].h_pz = pb_deep_event_.kaon_candidates()[i].h_pz();
    event->kaon_candidates[i].h_prob_k =
      pb_deep_event_.kaon_candidates()[i].h_prob_k();
    event->kaon_candidates[i].h_prob_pi =
      pb_deep_event_.kaon_candidates()[i].h_prob_pi();
    event->kaon_candidates[i].h_charge =
      pb_deep_event_.kaon_candidates()[i].h_charge();
    event->kaon_candidates[i].h_ip_chi2 =
      pb_deep_event_.kaon_candidates()[i].h_ip_chi2();
  }

  return true;
}


bool EventReaderProtobuf::NextEvent(Event *event) {
  if (nevent_ % 100000 == 0) {
    delete coded_istream_;
    coded_istream_ = new google::protobuf::io::CodedInputStream(istream_);
  }

  google::protobuf::uint32 size;
  bool has_next = coded_istream_->ReadLittleEndian32(&size);
  if (!has_next)
    return false;

  google::protobuf::io::CodedInputStream::Limit limit =
    coded_istream_->PushLimit(size);
  bool retval;
  if (is_deep_)
   retval = pb_deep_event_.ParseFromCodedStream(coded_istream_);
  else
   retval = pb_event_.ParseFromCodedStream(coded_istream_);
  assert(retval);
  coded_istream_->PopLimit(limit);

  nevent_++;

  if (is_deep_)
   return SerializeDeepEvent(event);
  else
   return SerializeFlatEvent(event);
}


//------------------------------------------------------------------------------


void EventReaderAvro::Open(const std::string &path) {
  int retval = avro_file_reader(path.c_str(), &db_);
  assert(retval == 0);

  proj_schema_ = avro_schema_record("event", "lhcb.cern.ch");
  if (plot_only_) {
    retval =
      AVRO_APP(proj_schema_, "h1_is_muon", avro_schema_int()) ||
      AVRO_APP(proj_schema_, "h1_px", avro_schema_double());
  } else {
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
  }

  assert(retval == 0);
}


#define AVRO_GET_DOUBLE(X, Y, Z) avro_record_get(record, X, &Y); \
  avro_double_get(Y, &Z); \
  avro_datum_decref(Y);
#define AVRO_GET_INT(X, Y, Z) avro_record_get(record, X, &Y); \
  avro_int32_get(Y, &Z); \
  avro_datum_decref(Y);

bool EventReaderAvro::NextEvent(Event *event) {
  avro_datum_t record;
  int retval = avro_file_reader_read(db_, proj_schema_, &record);
  if (retval != 0)
    return false;

  AVRO_GET_INT("h1_is_muon", val_h1_is_muon_,
               event->kaon_candidates[0].h_is_muon);
  if (event->kaon_candidates[0].h_is_muon) {
    avro_datum_decref(record);
    return true;
  }

  if (plot_only_) {
    AVRO_GET_DOUBLE("h1_px", val_h1_px_, event->kaon_candidates[0].h_px);
    avro_datum_decref(record);
    return true;
  }

  AVRO_GET_INT("h2_is_muon", val_h2_is_muon_,
               event->kaon_candidates[1].h_is_muon);
  if (event->kaon_candidates[1].h_is_muon) {
    avro_datum_decref(record);
    return true;
  }
  AVRO_GET_INT("h3_is_muon", val_h3_is_muon_,
               event->kaon_candidates[2].h_is_muon);
  if (event->kaon_candidates[2].h_is_muon) {
    avro_datum_decref(record);
    return true;
  }

  AVRO_GET_DOUBLE("h1_px", val_h1_px_, event->kaon_candidates[0].h_px);
  AVRO_GET_DOUBLE("h1_py", val_h1_px_, event->kaon_candidates[0].h_py);
  AVRO_GET_DOUBLE("h1_pz", val_h1_px_, event->kaon_candidates[0].h_pz);
  AVRO_GET_DOUBLE("h1_prob_k", val_h1_prob_k_,
                  event->kaon_candidates[0].h_prob_k);
  AVRO_GET_DOUBLE("h1_prob_pi", val_h1_prob_pi_,
                  event->kaon_candidates[0].h_prob_pi);
  AVRO_GET_INT("h1_charge", val_h1_charge_,
               event->kaon_candidates[0].h_charge);
  AVRO_GET_DOUBLE("h2_px", val_h2_px_, event->kaon_candidates[1].h_px);
  AVRO_GET_DOUBLE("h2_py", val_h2_px_, event->kaon_candidates[1].h_py);
  AVRO_GET_DOUBLE("h2_pz", val_h2_px_, event->kaon_candidates[1].h_pz);
  AVRO_GET_DOUBLE("h2_prob_k", val_h2_prob_k_,
                  event->kaon_candidates[1].h_prob_k);
  AVRO_GET_DOUBLE("h2_prob_pi", val_h2_prob_pi_,
                  event->kaon_candidates[1].h_prob_pi);
  AVRO_GET_INT("h2_charge", val_h2_charge_,
               event->kaon_candidates[1].h_charge);
  AVRO_GET_DOUBLE("h3_px", val_h3_px_, event->kaon_candidates[2].h_px);
  AVRO_GET_DOUBLE("h3_py", val_h3_px_, event->kaon_candidates[2].h_py);
  AVRO_GET_DOUBLE("h3_pz", val_h3_px_, event->kaon_candidates[2].h_pz);
  AVRO_GET_DOUBLE("h3_prob_k", val_h3_prob_k_,
                  event->kaon_candidates[2].h_prob_k);
  AVRO_GET_DOUBLE("h3_prob_pi", val_h3_prob_pi_,
                  event->kaon_candidates[2].h_prob_pi);
  AVRO_GET_INT("h3_charge", val_h3_charge_,
               event->kaon_candidates[2].h_charge);

  avro_datum_decref(record);

  return true;
}


//------------------------------------------------------------------------------


void EventReaderRoot::Open(const std::string &path) {
  root_chain_ = new TChain("DecayTree");
  std::vector<std::string> vec_paths = SplitString(path, ':');
  for (const auto &p : vec_paths)
    root_chain_->Add(p.c_str());
  flat_event_ = new FlatEvent();
  deep_event_ = new DeepEvent();
  csplit_event_ = new CSplitEvent();
}


int EventReaderRoot::GetIsMuon(Event *event, unsigned candidate_num) {
  assert(candidate_num <= 2);
  if (split_mode_ == SplitMode::kSplitAuto) {
    switch (candidate_num) {
      case 0: return flat_event_->H1_isMuon;
      case 1: return flat_event_->H2_isMuon;
      case 2: return flat_event_->H3_isMuon;
    }
  } else if (split_mode_ == SplitMode::kSplitDeep) {
    return deep_event_->kaon_candidates[candidate_num].h_is_muon;
  } else if (split_mode_ == SplitMode::kSplitC) {
    return csplit_event_->h_is_muon[candidate_num];
  }

  return event->kaon_candidates[candidate_num].h_is_muon;
}


bool EventReaderRoot::NextEvent(Event *event) {
  if (num_events_ < 0) {
    AttachBranches2Event(event);
    num_events_ = root_chain_->GetEntries();
    pos_events_ = 0;
  }
  if (pos_events_ >= num_events_)
    return false;

  if (split_mode_ == SplitMode::kSplitNone) {
    br_flat_event_->GetEntry(pos_events_);
    flat_event_->ToEvent(event);
    pos_events_++;
    return true;
  }
  bool is_flat_event = (split_mode_ == SplitMode::kSplitAuto);
  bool is_deep_event = (split_mode_ == SplitMode::kSplitDeep);
  bool is_csplit_event = (split_mode_ == SplitMode::kSplitC);
  bool is_leaflist_event = (split_mode_ == SplitMode::kSplitLeaflist);
  bool uses_csplit = (is_csplit_event || is_leaflist_event);
  bool is_nested_event = (is_deep_event || is_csplit_event || is_leaflist_event);

  if (uses_csplit) br_nkaons_->GetEntry(pos_events_);
  if (!read_all_) {
    bool skip;
    if (is_nested_event) br_h_is_muon_->GetEntry(pos_events_);

    if (!is_nested_event) br_h1_is_muon_->GetEntry(pos_events_);
    skip = GetIsMuon(event, 0);
    if (skip) {
      if (is_flat_event) flat_event_->ToEvent(event);
      if (is_deep_event) deep_event_->ToEvent(event);
      if (uses_csplit) csplit_event_->ToEvent(event);
      pos_events_++; return true;
    }
    if (plot_only_) {
      if (is_nested_event) br_h_px_->GetEntry(pos_events_);
      if (!is_nested_event) br_h1_px_->GetEntry(pos_events_);
      if (is_flat_event) flat_event_->ToEvent(event);
      if (is_deep_event) deep_event_->ToEvent(event);
      if (uses_csplit) csplit_event_->ToEvent(event);
      pos_events_++;
      return true;
    }
    if (!is_nested_event) br_h2_is_muon_->GetEntry(pos_events_);
    skip = GetIsMuon(event, 1);
    if (skip) {
      if (is_flat_event) flat_event_->ToEvent(event);
      if (is_deep_event) deep_event_->ToEvent(event);
      if (uses_csplit) csplit_event_->ToEvent(event);
      pos_events_++; return true;
    }
    if (!is_nested_event) br_h3_is_muon_->GetEntry(pos_events_);
    skip = GetIsMuon(event, 2);
    if (skip) {
      if (is_flat_event) flat_event_->ToEvent(event);
      if (is_deep_event) deep_event_->ToEvent(event);
      if (uses_csplit) csplit_event_->ToEvent(event);
      pos_events_++; return true;
    }
  } else {
    br_h1_is_muon_->GetEntry(pos_events_);
    br_h2_is_muon_->GetEntry(pos_events_);
    br_h3_is_muon_->GetEntry(pos_events_);
    br_b_flight_distance_->GetEntry(pos_events_);
    br_b_vertex_chi2_->GetEntry(pos_events_);
    br_h1_ip_chi2_->GetEntry(pos_events_);
    br_h2_ip_chi2_->GetEntry(pos_events_);
    br_h3_ip_chi2_->GetEntry(pos_events_);
  }

  if (is_nested_event) {
    br_h_px_->GetEntry(pos_events_);
    br_h_py_->GetEntry(pos_events_);
    br_h_pz_->GetEntry(pos_events_);
    br_h_prob_k_->GetEntry(pos_events_);
    br_h_prob_pi_->GetEntry(pos_events_);
    br_h_charge_->GetEntry(pos_events_);
  } else {
    br_h1_px_->GetEntry(pos_events_);
    br_h1_py_->GetEntry(pos_events_);
    br_h1_pz_->GetEntry(pos_events_);
    br_h1_prob_k_->GetEntry(pos_events_);
    br_h1_prob_pi_->GetEntry(pos_events_);
    br_h1_charge_->GetEntry(pos_events_);
    br_h2_px_->GetEntry(pos_events_);
    br_h2_py_->GetEntry(pos_events_);
    br_h2_pz_->GetEntry(pos_events_);
    br_h2_prob_k_->GetEntry(pos_events_);
    br_h2_prob_pi_->GetEntry(pos_events_);
    br_h2_charge_->GetEntry(pos_events_);
    br_h3_px_->GetEntry(pos_events_);
    br_h3_py_->GetEntry(pos_events_);
    br_h3_pz_->GetEntry(pos_events_);
    br_h3_prob_k_->GetEntry(pos_events_);
    br_h3_prob_pi_->GetEntry(pos_events_);
    br_h3_charge_->GetEntry(pos_events_);
  }

  if (is_flat_event) flat_event_->ToEvent(event);
  if (is_deep_event) deep_event_->ToEvent(event);
  if (uses_csplit) csplit_event_->ToEvent(event);

  pos_events_++;
  return true;
}


void EventReaderRoot::AttachBranches2Event(Event *event) {
  switch (split_mode_) {
    case SplitMode::kSplitManual:
      AttachBranches2EventManual(event);
      break;
    case SplitMode::kSplitNone:
      AttachBranches2EventNone();
      break;
    case SplitMode::kSplitAuto:
      AttachBranches2EventAuto();
      break;
    case SplitMode::kSplitDeep:
      AttachBranches2EventDeep();
      break;
    case SplitMode::kSplitC:
      AttachBranches2EventCSplit();
      break;
    case SplitMode::kSplitLeaflist:
      AttachBranches2EventLeaflist();
      break;
    default:
      abort();
  }
}

void EventReaderRoot::AttachBranches2EventNone() {
  root_chain_->SetBranchAddress("EventBranch", &flat_event_, &br_flat_event_);
}

void EventReaderRoot::AttachBranches2EventAuto() {
  root_chain_->SetBranchAddress("EventBranch", &flat_event_, &br_flat_event_);
  if (plot_only_) {
    br_h1_is_muon_ = root_chain_->GetBranch("H1_isMuon");
    br_h1_px_ = root_chain_->GetBranch("H1_PX");
    return;
  }

  br_h1_px_ = root_chain_->GetBranch("H1_PX");
  br_h1_py_ = root_chain_->GetBranch("H1_PY");
  br_h1_pz_ = root_chain_->GetBranch("H1_PZ");
  br_h1_prob_k_ = root_chain_->GetBranch("H1_ProbK");
  br_h1_prob_pi_ = root_chain_->GetBranch("H1_ProbPi");
  br_h1_charge_ = root_chain_->GetBranch("H1_Charge");
  br_h1_is_muon_ = root_chain_->GetBranch("H1_isMuon");
  br_h2_px_ = root_chain_->GetBranch("H2_PX");
  br_h2_py_ = root_chain_->GetBranch("H2_PY");
  br_h2_pz_ = root_chain_->GetBranch("H2_PZ");
  br_h2_prob_k_ = root_chain_->GetBranch("H2_ProbK");
  br_h2_prob_pi_ = root_chain_->GetBranch("H2_ProbPi");
  br_h2_charge_ = root_chain_->GetBranch("H2_Charge");
  br_h2_is_muon_ = root_chain_->GetBranch("H2_isMuon");
  br_h3_px_ = root_chain_->GetBranch("H3_PX");
  br_h3_py_ = root_chain_->GetBranch("H3_PY");
  br_h3_pz_ = root_chain_->GetBranch("H3_PZ");
  br_h3_prob_k_ = root_chain_->GetBranch("H3_ProbK");
  br_h3_prob_pi_ = root_chain_->GetBranch("H3_ProbPi");
  br_h3_charge_ = root_chain_->GetBranch("H3_Charge");
  br_h3_is_muon_ = root_chain_->GetBranch("H3_isMuon");
}

void EventReaderRoot::AttachBranches2EventDeep() {
  root_chain_->SetBranchAddress("EventBranch", &deep_event_, &br_deep_event_);

  if (plot_only_) {
    br_h_is_muon_ = root_chain_->GetBranch("kaon_candidates.h_is_muon");
    br_h_px_ = root_chain_->GetBranch("kaon_candidates.h_px");
    return;
  }

  br_h_px_ = root_chain_->GetBranch("kaon_candidates.h_px");
  br_h_py_ = root_chain_->GetBranch("kaon_candidates.h_py");
  br_h_pz_ = root_chain_->GetBranch("kaon_candidates.h_pz");
  br_h_prob_k_ = root_chain_->GetBranch("kaon_candidates.h_prob_k");
  br_h_prob_pi_ = root_chain_->GetBranch("kaon_candidates.h_prob_pi");
  br_h_charge_ = root_chain_->GetBranch("kaon_candidates.h_charge");
  br_h_is_muon_ = root_chain_->GetBranch("kaon_candidates.h_is_muon");
}

void EventReaderRoot::AttachBranches2EventCSplit() {
  root_chain_->SetBranchAddress("EventBranch", &csplit_event_, &br_csplit_event_);
  br_nkaons_ = root_chain_->GetBranch("nKaons");

  if (plot_only_) {
    br_h_is_muon_ = root_chain_->GetBranch("h_is_muon");
    br_h_px_ = root_chain_->GetBranch("h_px");
    return;
  }

  br_h_px_ = root_chain_->GetBranch("h_px");
  br_h_py_ = root_chain_->GetBranch("h_py");
  br_h_pz_ = root_chain_->GetBranch("h_pz");
  br_h_prob_k_ = root_chain_->GetBranch("h_prob_k");
  br_h_prob_pi_ = root_chain_->GetBranch("h_prob_pi");
  br_h_charge_ = root_chain_->GetBranch("h_charge");
  br_h_is_muon_ = root_chain_->GetBranch("h_is_muon");
}

void EventReaderRoot::AttachBranches2EventLeaflist() {
  csplit_event_->FromEvent(Event());
  root_chain_->SetBranchAddress("nKaons", &(csplit_event_->nKaons), &br_nkaons_);

  if (plot_only_) {
    root_chain_->SetBranchAddress("h_is_muon", &(csplit_event_->h_is_muon[0]), &br_h_is_muon_);
    root_chain_->SetBranchAddress("h_px", &(csplit_event_->h_px[0]), &br_h_px_);
    return;
  }

  root_chain_->SetBranchAddress("h_px", &(csplit_event_->h_px[0]), &br_h_px_);
  root_chain_->SetBranchAddress("h_py", &(csplit_event_->h_py[0]), &br_h_py_);
  root_chain_->SetBranchAddress("h_pz", &(csplit_event_->h_pz[0]), &br_h_pz_);
  root_chain_->SetBranchAddress("h_prob_k", &(csplit_event_->h_prob_k[0]), &br_h_prob_k_);
  root_chain_->SetBranchAddress("h_prob_pi", &(csplit_event_->h_prob_pi[0]), &br_h_prob_pi_);
  root_chain_->SetBranchAddress("h_charge", &(csplit_event_->h_charge[0]), &br_h_charge_);
  root_chain_->SetBranchAddress("h_is_muon", &(csplit_event_->h_is_muon[0]), &br_h_is_muon_);
}

void EventReaderRoot::AttachBranches2EventManual(Event *event) {
  if (plot_only_) {
    root_chain_->SetBranchAddress("H1_isMuon",
                                  &event->kaon_candidates[0].h_is_muon,
                                  &br_h1_is_muon_);
    root_chain_->SetBranchAddress("H1_PX", &event->kaon_candidates[0].h_px,
                                  &br_h1_px_);
    return;
  }

  root_chain_->SetBranchAddress("H1_PX", &event->kaon_candidates[0].h_px,
                                &br_h1_px_);
  root_chain_->SetBranchAddress("H1_PY", &event->kaon_candidates[0].h_py,
                                &br_h1_py_);
  root_chain_->SetBranchAddress("H1_PZ", &event->kaon_candidates[0].h_pz,
                                &br_h1_pz_);
  root_chain_->SetBranchAddress("H1_ProbK",
                                &event->kaon_candidates[0].h_prob_k,
                                &br_h1_prob_k_);
  root_chain_->SetBranchAddress("H1_ProbPi",
                                &event->kaon_candidates[0].h_prob_pi,
                                &br_h1_prob_pi_);
  root_chain_->SetBranchAddress("H1_Charge",
                                &event->kaon_candidates[0].h_charge,
                                &br_h1_charge_);
  root_chain_->SetBranchAddress("H1_isMuon",
                                &event->kaon_candidates[0].h_is_muon,
                                &br_h1_is_muon_);

  root_chain_->SetBranchAddress("H2_PX", &event->kaon_candidates[1].h_px,
                                &br_h2_px_);
  root_chain_->SetBranchAddress("H2_PY", &event->kaon_candidates[1].h_py,
                                &br_h2_py_);
  root_chain_->SetBranchAddress("H2_PZ", &event->kaon_candidates[1].h_pz,
                                &br_h2_pz_);
  root_chain_->SetBranchAddress("H2_ProbK",
                                &event->kaon_candidates[1].h_prob_k,
                                &br_h2_prob_k_);
  root_chain_->SetBranchAddress("H2_ProbPi",
                                &event->kaon_candidates[1].h_prob_pi,
                                &br_h2_prob_pi_);
  root_chain_->SetBranchAddress("H2_Charge",
                                &event->kaon_candidates[1].h_charge,
                                &br_h2_charge_);
  root_chain_->SetBranchAddress("H2_isMuon",
                                &event->kaon_candidates[1].h_is_muon,
                                &br_h2_is_muon_);

  root_chain_->SetBranchAddress("H3_PX", &event->kaon_candidates[2].h_px,
                                &br_h3_px_);
  root_chain_->SetBranchAddress("H3_PY", &event->kaon_candidates[2].h_py,
                                &br_h3_py_);
  root_chain_->SetBranchAddress("H3_PZ", &event->kaon_candidates[2].h_pz,
                                &br_h3_pz_);
  root_chain_->SetBranchAddress("H3_ProbK",
                                &event->kaon_candidates[2].h_prob_k,
                                &br_h3_prob_k_);
  root_chain_->SetBranchAddress("H3_ProbPi",
                                &event->kaon_candidates[2].h_prob_pi,
                                &br_h3_prob_pi_);
  root_chain_->SetBranchAddress("H3_Charge",
                                &event->kaon_candidates[2].h_charge,
                                &br_h3_charge_);
  root_chain_->SetBranchAddress("H3_isMuon",
                                &event->kaon_candidates[2].h_is_muon,
                                &br_h3_is_muon_);
}


/**
 * Not used for the analysis but required for converting into another file
 * format.
 */
void EventReaderRoot::AttachUnusedBranches2Event(Event *event) {
  root_chain_->SetBranchAddress("B_FlightDistance", &event->b_flight_distance,
                                &br_b_flight_distance_);
  root_chain_->SetBranchAddress("B_VertexChi2", &event->b_vertex_chi2,
                                &br_b_vertex_chi2_);
  root_chain_->SetBranchAddress("H1_IpChi2",
                                &event->kaon_candidates[0].h_ip_chi2,
                                &br_h1_ip_chi2_);
  root_chain_->SetBranchAddress("H2_IpChi2",
                                &event->kaon_candidates[1].h_ip_chi2,
                                &br_h2_ip_chi2_);
  root_chain_->SetBranchAddress("H3_IpChi2",
                                &event->kaon_candidates[2].h_ip_chi2,
                                &br_h3_ip_chi2_);
  read_all_ = true;
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
  result +=
    event.kaon_candidates[0].h_px +
    event.kaon_candidates[0].h_py +
    event.kaon_candidates[0].h_pz +
    event.kaon_candidates[0].h_prob_k +
    event.kaon_candidates[0].h_prob_pi +
    double(event.kaon_candidates[0].h_charge) +
    event.kaon_candidates[1].h_px +
    event.kaon_candidates[1].h_py +
    event.kaon_candidates[1].h_pz +
    event.kaon_candidates[1].h_prob_k +
    event.kaon_candidates[1].h_prob_pi +
    double(event.kaon_candidates[1].h_charge) +
    event.kaon_candidates[2].h_px +
    event.kaon_candidates[2].h_py +
    event.kaon_candidates[2].h_pz +
    event.kaon_candidates[2].h_prob_k +
    event.kaon_candidates[2].h_prob_pi +
    double(event.kaon_candidates[2].h_charge);

  /*for (const auto &k : event.kaon_candidates) {
    result += k.h_px + k.h_py + k.h_pz + k.h_prob_k + k.h_prob_pi +
              double(k.h_charge);
  }*/
  return result;
}

static double PlotEvent(const Event &event) {
  if (event.kaon_candidates[0].h_is_muon) return 0.0;
  return event.kaon_candidates[0].h_px;
}


int AnalyzeRootDataframe(
  const std::vector<std::string> &input_paths,
  bool plot_only,
  bool multi_threaded,
  bool hyper_threading)
{
#ifdef HAS_LZ4
  return 0;
#else
  TChain root_chain("DecayTree");
  for (const auto &p : input_paths)
    root_chain.Add(p.c_str());
  unsigned nslots = 1;
  if (multi_threaded) {
    if (hyper_threading)
      ROOT::EnableImplicitMT();
    else
      ROOT::EnableImplicitMT(4);
    nslots = ROOT::GetImplicitMTPoolSize();
    printf("Using %u slots\n", nslots);
  }
  ROOT::RDataFrame frame(root_chain);

  std::vector<double> sums(nslots, 0.0);

  auto fn_muon_cut = [](int is_muon) { return !is_muon; };
  auto fn_sum_slot = [&sums](
    unsigned int slot,
    double h1_px,
    double h1_py,
    double h1_pz,
    double h1_prob_k,
    double h1_prob_pi,
    int h1_charge,
    double h2_px,
    double h2_py,
    double h2_pz,
    double h2_prob_k,
    double h2_prob_pi,
    int h2_charge,
    double h3_px,
    double h3_py,
    double h3_pz,
    double h3_prob_k,
    double h3_prob_pi,
    int h3_charge)
  {
    sums[slot] +=
      h1_px +
      h1_py +
      h1_pz +
      h1_prob_k +
      h1_prob_pi +
      double(h1_charge) +
      h2_px +
      h2_py +
      h2_pz +
      h2_prob_k +
      h2_prob_pi +
      double(h2_charge) +
      h3_px +
      h3_py +
      h3_pz +
      h3_prob_k +
      h3_prob_pi +
      double(h3_charge);
  };

  frame.Filter(fn_muon_cut, {"H1_isMuon"})
         .Filter(fn_muon_cut, {"H2_isMuon"})
         .Filter(fn_muon_cut, {"H3_isMuon"})
         .ForeachSlot(fn_sum_slot, {
           "H1_PX",
           "H1_PY",
           "H1_PZ",
           "H1_ProbK",
           "H1_ProbPi",
           "H1_Charge",
           "H2_PX",
           "H2_PY",
           "H2_PZ",
           "H2_ProbK",
           "H2_ProbPi",
           "H2_Charge",
           "H3_PX",
           "H3_PY",
           "H3_PZ",
           "H3_ProbK",
           "H3_ProbPi",
           "H3_Charge"});

  double total_sum = 0.0;
  for (unsigned i = 0; i < sums.size(); ++i)
    total_sum += sums[i];
  unsigned nevent = root_chain.GetEntries();
  printf("finished (%u events), result: %lf, skipped ?\n",
         nevent, total_sum);
  return 0;
#endif
}


int AnalyzeRootOptimized(
  const std::vector<std::string> &input_paths,
  bool plot_only)
{
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

    if (plot_only) {
      if (*val_h1_is_muon) {
        nskipped++;
        continue;
      }
      dummy += *val_h1_px;
      continue;
    }

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
  printf("%s [-i input.root] [-i ...] "
         "[-r | -o output format [-d outdir] [-b bloat factor]]\n"
         "[-s(short file)] [-f|-g (data frame / mt)]\n", progname);
}


int main(int argc, char **argv) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  if (!TClassTable::GetDict("FlatEvent")) {
      gSystem->Load("./libEvent.so");
   }

   TLeafElement test;
   std::cout << "TEST: test.CanGenerateOffsetArray() " << test.CanGenerateOffsetArray() << std::endl;

  std::vector<std::string> input_paths;
  std::string input_suffix;
  std::string output_suffix;
  std::string outdir;
  bool root_optimized = false;
  bool root_dataframe = false;
  bool root_dataframe_mt = false;
  bool root_dataframe_ht = true;
  bool plot_only = false;  // read only 2 branches
  bool short_file = false;
  unsigned bloat_factor = 1;
  int c;
  while ((c = getopt(argc, argv, "hvi:o:rb:d:psfgG")) != -1) {
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
      case 'd':
        outdir = optarg;
        break;
      case 'r':
        root_optimized = true;
        break;
      case 'b':
        bloat_factor = String2Uint64(optarg);
        break;
      case 'p':
        plot_only = true;
        break;
      case 's':
        short_file = true;
        break;
      case 'f':
        root_dataframe = true;
        break;
      case 'g':
        root_dataframe = true;
        root_dataframe_mt = true;
        break;
      case 'G':
        root_dataframe = true;
        root_dataframe_mt = true;
        root_dataframe_ht = false;
        break;
      default:
        fprintf(stderr, "Unknown option: -%c\n", c);
        Usage(argv[0]);
        return 1;
    }
  }

  assert(!input_paths.empty());
  assert(bloat_factor > 0);
  input_suffix = GetSuffix(input_paths[0]);
  FileFormats input_format = GetFileFormat(input_suffix);
  if (root_optimized) {
    if (input_format == FileFormats::kRoot ||
        input_format == FileFormats::kRootInflated ||
        input_format == FileFormats::kRootDeflated ||
        input_format == FileFormats::kRootAutosplitInflated ||
        input_format == FileFormats::kRootAutosplitDeflated)
    {
      return AnalyzeRootOptimized(input_paths, plot_only);
    } else {
      printf("ignoring ROOT optimization flag\n");
    }
  }

  if (root_dataframe) {
    if (input_format == FileFormats::kRoot ||
        input_format == FileFormats::kRootInflated ||
        input_format == FileFormats::kRootDeflated ||
        input_format == FileFormats::kRootAutosplitInflated ||
        input_format == FileFormats::kRootAutosplitDeflated)
    {
      return AnalyzeRootDataframe(input_paths, plot_only,
                                  root_dataframe_mt, root_dataframe_ht);
    } else {
      printf("ignoring ROOT dataframe flag\n");
    }
  }

  std::unique_ptr<EventReader> event_reader {EventReader::Create(input_format)};
  event_reader->set_short_read(short_file);
  event_reader->Open(JoinStrings(input_paths, ":"));

  Event event;

  std::unique_ptr<EventWriter> event_writer{nullptr};
  if (!output_suffix.empty()) {
    assert((input_format == FileFormats::kRoot) ||
           (input_format == FileFormats::kRootInflated) ||
           (input_format == FileFormats::kRootDeflated));
    event_reader->PrepareForConversion(&event);

    FileFormats output_format = GetFileFormat(output_suffix);
    assert(output_format != FileFormats::kRoot);
    event_writer = EventWriter::Create(output_format);
    event_writer->set_short_write(short_file);
    std::string bloat_extension;
    if (bloat_factor > 1)
      bloat_extension = "times" + StringifyUint(bloat_factor) + ".";
    std::string dest_dir = GetParentPath(input_paths[0]);
    if (!outdir.empty())
      dest_dir = outdir;
    event_writer->Open(dest_dir + "/" + StripSuffix(GetFileName(input_paths[0]))
                       + "." + bloat_extension + output_suffix);
  }

  std::chrono::high_resolution_clock stopwatch;
  auto start_time = stopwatch.now();

  event_reader->set_plot_only(plot_only);
  unsigned i_events = 0;
  double dummy = 0.0;
  while (event_reader->NextEvent(&event)) {
    if (event_writer) {
      for (unsigned i = 0; i < bloat_factor; ++i)
        event_writer->WriteEvent(event);
    } else {
      if (plot_only)
        dummy += PlotEvent(event);
      else
        dummy += ProcessEvent(event);
    }
    if ((++i_events % 100000) == 0) {
      //printf("processed %u events\n", i_events);
      printf("processed %u k events\n", i_events / 1000);
      //printf(" ... H_PX[1] = %lf\n", event.kaon_candidates[0].h_px);
      //printf(" ... dummy is %lf\n", dummy); abort();
    }
    if (short_file && (i_events == 500000))
      break;
  }

  auto end_time = stopwatch.now();
  auto diff = end_time - start_time;
  auto milliseconds =
    std::chrono::duration_cast<std::chrono::milliseconds>(diff);

  printf("finished (%u events), result: %lf, skipped %u\n",
         i_events, dummy, g_skipped);
  std::cout << "Took " << milliseconds.count() << " ms" << std::endl;
  if (event_writer) event_writer->Close();

  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}
