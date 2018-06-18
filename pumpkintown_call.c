#include <stdio.h>

#include "pumpkintown_call.h"

void pumpkintown_set_call_name(PumpkintownCall* call, const char* name) {
  printf("%s\n", name);
}

void pumpkintown_append_call_arg(PumpkintownCall* call,
                                 const PumpkintownTypeUnion value) {}
