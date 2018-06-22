#ifndef PUMPKINTOWN_FILE_H_
#define PUMPKINTOWN_FILE_H_

#include <stdbool.h>
#include <stdint.h>

#include "pumpkintown_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  PUMPKINTOWN_FILE_TAG_FUNCTION,
  PUMPKINTOWN_FILE_TAG_ARG,
  PUMPKINTOWN_FILE_TAG_END_FUNCTION,
} PumpkintownFileTag;

uint32_t read_uint32(FILE* file, bool* err);
PumpkintownFileTag read_tag(FILE* file, bool* err);

#ifdef __cplusplus
}
#endif

#endif  // PUMPKINTOWN_FILE_H_
