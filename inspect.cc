#include <fcntl.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>

#include <iostream>
#include <string>

/// Big-endian 16-bit unsigned integer
class RUInt16BE {
private:
   std::uint16_t fValBE = 0;
   static std::uint16_t Swap(std::uint16_t val) {
      return (val & 0x00FF) << 8 | (val & 0xFF00) >> 8;
   }
public:
   RUInt16BE() = default;
   explicit RUInt16BE(const std::uint16_t val) : fValBE(Swap(val)) {}
   operator std::uint16_t() const {
      return Swap(fValBE);
   }
   RUInt16BE& operator =(const std::uint16_t val) {
      fValBE = Swap(val);
      return *this;
   }
};

/// Big-endian 32-bit unsigned integer
class RUInt32BE {
private:
   std::uint32_t fValBE = 0;
   static std::uint32_t Swap(std::uint32_t val) {
      auto x = (val & 0x0000FFFF) << 16 | (val & 0xFFFF0000) >> 16;
      return (x & 0x00FF00FF) << 8 | (x & 0xFF00FF00) >> 8;
   }
public:
   RUInt32BE() = default;
   explicit RUInt32BE(const std::uint32_t val) : fValBE(Swap(val)) {}
   operator std::uint32_t() const {
      return Swap(fValBE);
   }
   RUInt32BE& operator =(const std::uint32_t val) {
      fValBE = Swap(val);
      return *this;
   }
};

/// Big-endian 32-bit signed integer
class RInt32BE {
private:
   std::int32_t fValBE = 0;
   static std::int32_t Swap(std::int32_t val) {
      auto x = (val & 0x0000FFFF) << 16 | (val & 0xFFFF0000) >> 16;
      return (x & 0x00FF00FF) << 8 | (x & 0xFF00FF00) >> 8;
   }
public:
   RInt32BE() = default;
   explicit RInt32BE(const std::int32_t val) : fValBE(Swap(val)) {}
   operator std::int32_t() const {
      return Swap(fValBE);
   }
   RInt32BE& operator =(const std::int32_t val) {
      fValBE = Swap(val);
      return *this;
   }
};

/// Big-endian 64-bit unsigned integer
class RUInt64BE {
private:
   std::uint64_t fValBE = 0;
   static std::uint64_t Swap(std::uint64_t val) {
      auto x = (val & 0x00000000FFFFFFFF) << 32 | (val & 0xFFFFFFFF00000000) >> 32;
      x = (x & 0x0000FFFF0000FFFF) << 16 | (x & 0xFFFF0000FFFF0000) >> 16;
      return (x & 0x00FF00FF00FF00FF) << 8  | (x & 0xFF00FF00FF00FF00) >> 8;
   }
public:
   RUInt64BE() = default;
   explicit RUInt64BE(const std::uint64_t val) : fValBE(Swap(val)) {}
   operator std::uint64_t() const {
      return Swap(fValBE);
   }
   RUInt64BE& operator =(const std::uint64_t val) {
      fValBE = Swap(val);
      return *this;
   }
};

void Usage(char *progname) {
  printf("Usage: %s <ROOT file> [-b(ig file)]\n", progname);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    Usage(argv[0]);
    return 1;
  }

  bool bigFile = false;
  if ((argc >= 3) && (std::string(argv[2]) == "-b")) {
    bigFile = true;
  }

  std::string path = argv[1];

  int fd = open(path.c_str(), O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Cannot open %s\n", path.c_str());
    return 1;
  }

  lseek(fd, 100, SEEK_SET);

  RUInt32BE nbytes;
  read(fd, &nbytes, sizeof(nbytes));
  printf("NBytes: %u\n", (std::uint32_t)nbytes);

  RUInt16BE version;
  read(fd, &version, sizeof(version));
  printf("Version: %u\n", (std::uint16_t)version);

  RUInt32BE objlen;
  read(fd, &objlen, sizeof(objlen));
  printf("ObjLen: %u\n", (std::uint32_t)objlen);

  RUInt32BE datetime;
  read(fd, &datetime, sizeof(datetime));

  RUInt16BE keylen;
  read(fd, &keylen, sizeof(keylen));
  printf("KeyLen: %u\n", (std::uint16_t)keylen);

  RUInt16BE cycle;
  read(fd, &cycle, sizeof(cycle));
  printf("Cycle: %u\n", (std::uint16_t)cycle);

  RUInt32BE seekkey;
  read(fd, &seekkey, sizeof(seekkey));
  printf("SeekKey: %u\n", (std::uint32_t)seekkey);

  RUInt32BE seekpdir;
  read(fd, &seekpdir, sizeof(seekpdir));
  printf("SeekPDir: %u\n", (std::uint32_t)seekpdir);

  char lclname;
  read(fd, &lclname, sizeof(lclname));
  printf("lclname: %u\n", lclname);

  unsigned char buf[1000];
  read(fd, buf, lclname);

  char lobjname;
  read(fd, &lobjname, sizeof(lobjname));
  printf("lobjname: %u\n", lobjname);

  read(fd, buf, lobjname);

  char ltitle;
  read(fd, &ltitle, sizeof(ltitle));
  printf("ltitle: %u\n", ltitle);

  read(fd, buf, ltitle);

  // TFILE DATA

  read(fd, &lobjname, sizeof(lobjname));
  printf("lobjname(TFile): %u\n", lobjname);
  read(fd, buf, lobjname);

  read(fd, &ltitle, sizeof(ltitle));
  printf("ltitle(TFile): %u\n", ltitle);
  read(fd, buf, ltitle);

  RUInt16BE tfileversion;
  read(fd, &tfileversion, sizeof(tfileversion));
  printf("TFile version: %u\n", (std::uint16_t)tfileversion);
//  char modified;
//  read(fd, &modified, sizeof(modified));
//  printf("modified: %u\n", modified);
//
//  char writable;
//  read(fd, &writable, sizeof(writable));
//  printf("writable: %u\n", writable);

  read(fd, &datetime, sizeof(datetime));
  read(fd, &datetime, sizeof(datetime));

  RUInt32BE nbyteskeys;
  read(fd, &nbyteskeys, sizeof(nbyteskeys));
  printf("NByteKeys: %u\n", (std::uint32_t)nbyteskeys);

  RUInt32BE nbytesname;
  read(fd, &nbytesname, sizeof(nbytesname));
  printf("NByteName: %u\n", (std::uint32_t)nbytesname);

  if (bigFile) {
    RUInt64BE seekdir;
    read(fd, &seekdir, sizeof(seekdir));
    printf("SeekDir: %lu\n", (std::uint64_t)seekdir);

    RUInt64BE seekparent;
    read(fd, &seekparent, sizeof(seekparent));
    printf("SeekParent: %lu\n", (std::uint64_t)seekparent);

    RUInt64BE seekkeys;
    read(fd, &seekkeys, sizeof(seekkeys));
    printf("SeekKeys: %lu\n", (std::uint64_t)seekkeys);
  } else {
    RUInt32BE seekdir;
    read(fd, &seekdir, sizeof(seekdir));
    printf("SeekDir: %u\n", (std::uint32_t)seekdir);

    RUInt32BE seekparent;
    read(fd, &seekparent, sizeof(seekparent));
    printf("SeekParent: %u\n", (std::uint32_t)seekparent);

    RUInt32BE seekkeys;
    read(fd, &seekkeys, sizeof(seekkeys));
    printf("SeekKeys: %u\n", (std::uint32_t)seekkeys);
  }

  RUInt16BE uuidversion;
  read(fd, &uuidversion, sizeof(uuidversion));
  printf("UUID version: %u\n", (std::uint16_t)uuidversion);

  off_t pos = lseek(fd, 0, SEEK_CUR);
  printf("Object size: %lu\n", pos + 16 - 100);

  return 0;
}
