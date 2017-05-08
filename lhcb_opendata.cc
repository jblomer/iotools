/**
 * Copyright CERN; jblomer@cern.ch
 */

#include "lhcb_opendata.h"

#include <hdf5_hl.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <cstdio>
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
    case FileFormats::kAvro:
      return std::unique_ptr<EventWriter>(new EventWriterAvro(true));
    case FileFormats::kAvroInflated:
      return std::unique_ptr<EventWriter>(new EventWriterAvro(false));
    default:
      abort();
  }
}


std::unique_ptr<EventReader> EventReader::Create(FileFormats format) {
  switch (format) {
    case FileFormats::kRoot:
      return std::unique_ptr<EventReader>(new EventReaderRoot());
    case FileFormats::kSqlite:
      return std::unique_ptr<EventReader>(new EventReaderSqlite());
    case FileFormats::kH5Row:
      return std::unique_ptr<EventReader>(new EventReaderH5Row());
    case FileFormats::kAvro:
      return std::unique_ptr<EventReader>(new EventReaderAvro());
    case FileFormats::kAvroInflated:
      return std::unique_ptr<EventReader>(new EventReaderAvro());
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

  //hsize_t dim = 8600000;
  //H5LTmake_dataset_double(file_id_, "/B_FlightDistance", )
}


void EventWriterH5Column::WriteEvent(const Event &event) {
  abort();
}


void EventWriterH5Column::Close() {
  H5Fclose(file_id_);
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
                                &event->kaon_candidates[1].h_prob_k);
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


static void ProcessEvent(const Event &event) {
  for (const auto &k : event.kaon_candidates) {
    if (k.h_is_muon)
      return;
  }

  // Fill Histograms
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
      *val_h1_charge +
      *val_h2_px + *val_h2_py + *val_h2_pz + *val_h2_prob_k + *val_h2_prob_pi +
      *val_h2_charge +
      *val_h3_px + *val_h3_py + *val_h3_pz + *val_h3_prob_k + *val_h3_prob_pi +
      *val_h3_charge;

    if ((nread % 100000) == 0) {
      printf("processed %u k events\n", nread / 1000);
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
  while (event_reader->NextEvent(&event)) {
    if (event_writer) {
      event_writer->WriteEvent(event);
    } else {
      ProcessEvent(event);
    }
    if ((++i_events % 100000) == 0) {
      printf("processed %u k events\n", i_events / 1000);
    }
  }

  printf("finished (%u events)\n", i_events);
  if (event_writer) event_writer->Close();

  return 0;
}
