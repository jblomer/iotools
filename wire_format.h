/**
 * Copyright CERN; jblomer@cern.ch
 */

#ifndef WIRE_FORMAT_H_
#define WIRE_FORMAT_H_

#include <inttypes.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IOTRACE_MAX_FILENAME 20

enum iotrace_ops {
  IOO_OPEN,
  IOO_READ,
  IOO_SEEK,
  IOO_CLOSE,
};

struct iotrace_frame {
  enum iotrace_ops op;
  int fd;
  int64_t duration_ns;

  union {
    struct {
      int64_t size;
    } read;

    struct {
      int64_t offset;
    } seek;
  } info;
} __attribute__((__packed__));

#ifdef __cplusplus
}
#endif

#endif  // WIRE_FORMAT_H_
