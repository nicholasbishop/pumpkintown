#ifndef PUMPKINTOWN_DLIB_H_
#define PUMPKINTOWN_DLIB_H_

#include <cstdint>

void* get_real_proc_addr(const char* name);
void* get_real_proc_addr(const unsigned char* name);

void write_trace_item(const uint8_t* buf, uint64_t size);

#endif  // PUMPKINTOWN_DLIB_H_
