#ifndef PUMPKINTOWN_DLIB_H_
#define PUMPKINTOWN_DLIB_H_

#include <cstdint>

void* get_real_proc_addr(const char* name);
void* get_real_proc_addr(const unsigned char* name);

namespace pumpkintown {

class Serialize;

Serialize* serialize();

}

#endif  // PUMPKINTOWN_DLIB_H_
