#include "pumpkintown_dlib.hh"

#include <cstdio>
#include <cstdlib>
#include <stdexcept>

#include <dlfcn.h>

#include "pumpkintown_gl_enum.hh"
#include "pumpkintown_gl_types.hh"
#include "pumpkintown_serialize.hh"

void* get_libgl() {
  static void* libgl = NULL;
  if (!libgl) {
    libgl = dlopen("libGL.so", RTLD_LAZY);
  }
  return libgl;
}

void* get_real_proc_addr(const char* name) {
  static void* (*get)(const char* name) = NULL;
  if (!get) {
    get = reinterpret_cast<void* (*)(const char* name)>(dlsym(get_libgl(), "glXGetProcAddress"));
  }
  return get(name);
}

void* get_real_proc_addr(const unsigned char* name) {
  return get_real_proc_addr(reinterpret_cast<const char*>(name));
}

namespace pumpkintown {

Serialize* serialize() {
  static Serialize s;
  if (!s.is_open()) {
    if (!s.open("trace")) {
      // TODO
      fprintf(stderr, "failed to open trace file\n");
    }
  }
  return &s;
}

}
