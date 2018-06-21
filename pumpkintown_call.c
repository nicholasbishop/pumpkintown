#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pumpkintown_call.h"
#include "pumpkintown_file.h"

void write_data(const void* data, int length) {
  static FILE* file = NULL;
  if (!file) {
    file = fopen("trace", "wb");
  }
  fwrite(data, length, 1, file);
  fflush(file);
}

void write_uint32(const uint32_t data) {
  write_data(&data, sizeof(data));
}

void pumpkintown_set_call_name(PumpkintownCall* call, const char* name) {
  write_uint32(PUMPKINTOWN_FILE_TAG_FUNCTION);
  write_uint32(strlen(name));
  write_data(name, strlen(name));
}

void pumpkintown_append_call_arg(PumpkintownCall* call,
                                 const PumpkintownTypeUnion value) {
  write_uint32(PUMPKINTOWN_FILE_TAG_ARG);
  write_data(&value, sizeof(value));
}

void pumpkintown_end_call() {
  write_uint32(PUMPKINTOWN_FILE_TAG_END_FUNCTION);
}
