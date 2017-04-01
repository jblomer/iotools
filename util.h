/**
 * Copyright CERN; jblomer@cern.ch
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <string>
#include <vector>

enum class FileFormats { kRoot, kHdf5, kSqlite };

FileFormats GetFileFormat(const std::string &suffix);
std::string StripSuffix(const std::string &path);
std::string GetSuffix(const std::string &path);

std::vector<std::string> SplitString(
  const std::string &str,
  const char delim,
  const unsigned max_chunks = 0);
std::string JoinStrings(
  const std::vector<std::string> &strings,
  const std::string &joint);

#endif  // UTIL_H_
