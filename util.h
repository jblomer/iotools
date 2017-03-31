/**
 * Copyright CERN; jblomer@cern.ch
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <string>

enum class FileFormats { kRoot, kHdf5, kSqlite };

FileFormats GetFileFormat(const std::string &suffix);
std::string StripSuffix(const std::string &path);

#endif  // UTIL_H_
