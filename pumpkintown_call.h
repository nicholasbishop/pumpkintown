#ifndef PUMPKINTOWN_CALL_H_
#define PUMPKINTOWN_CALL_H_

#include "pumpkintown_types.h"

typedef struct {
  char* name;
  PumpkintownTypeUnion* parameters;
  int num_parameters;
} PumpkintownCall;

void pumpkintown_set_call_name(PumpkintownCall* call, const char* name);
void pumpkintown_append_call_arg(PumpkintownCall* call,
                                 const PumpkintownTypeUnion value);
void pumpkintown_end_call();

#endif  // PUMPKINTOWN_CALL_H_
