/**
 * Copyright CERN; jblomer@cern.ch
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <stdint.h>

#include <string>
#include <vector>

enum class FileFormats
  { kRoot, kH5Row, kH5Column, kAvroDeflated, kAvroInflated,
    kSqlite, kProtobufDeflated, kProtobufInflated, kRootInflated,
    kRootDeflated, kRootLz4, kRootLzma, kRootRow, kRootAutosplitInflated,
    kRootAutosplitDeflated, kRootDeepsplitInflated, kParquetInflated,
    kParquetDeflated, kParquetSnappy };

FileFormats GetFileFormat(const std::string &suffix);
std::string StripSuffix(const std::string &path);
std::string GetSuffix(const std::string &path);
std::string GetFileName(const std::string &path);
std::string GetParentPath(const std::string &path);

std::vector<std::string> SplitString(
  const std::string &str,
  const char delim,
  const unsigned max_chunks = 0);
std::string JoinStrings(
  const std::vector<std::string> &strings,
  const std::string &joint);

uint64_t String2Uint64(const std::string &value);
std::string StringifyUint(const uint64_t value);


#endif  // UTIL_H_
