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

void* get_libegl() {
  static void* libegl = NULL;
  if (!libegl) {
    libegl = dlopen("libEGL.so", RTLD_LAZY);
  }
  return libegl;
}

void* get_real_proc_addr(const char* name) {
  static void* (*get_gl)(const char* name) =
      reinterpret_cast<void* (*)(const char* name)>(
          dlsym(get_libgl(), "glXGetProcAddress"));
  static void* (*get_egl)(const char* name) =
      reinterpret_cast<void* (*)(const char* name)>(
          dlsym(get_libegl(), "eglGetProcAddress"));
  if (get_egl) {
    void* ret = get_egl(name);
    if (ret) {
      return ret;
    }
  }
  return get_gl(name);
}

void* get_real_proc_addr(const unsigned char* name) {
  return get_real_proc_addr(reinterpret_cast<const char*>(name));
}

// TODO
extern "C" void* glXGetProcAddress(const char* name);
extern "C" void* eglGetProcAddress(const char* name) {
  return glXGetProcAddress(name);
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
