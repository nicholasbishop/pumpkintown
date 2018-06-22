#include <cstdio>

#include "pumpkintown_file.h"

uint32_t read_uint32(FILE* file, bool* err) {
  uint32_t data;
  *err = fread(&data, sizeof(data), 1, file) == sizeof(data);
  return data;
}

PumpkintownFileTag read_tag(FILE* file, bool* err) {
  return static_cast<PumpkintownFileTag>(read_uint32(file, err));
}
