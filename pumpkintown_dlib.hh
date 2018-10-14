#ifndef PUMPKINTOWN_DLIB_H_
#define PUMPKINTOWN_DLIB_H_

#include <cstdint>

void* get_real_proc_addr(const char* name);
void* get_real_proc_addr(const unsigned char* name);

namespace pumpkintown {

class Serialize;

Serialize* serialize();

bool serialize_standard_gl_gen(int32_t count, uint32_t* array);

}

#endif  // PUMPKINTOWN_DLIB_H_
