#include "pumpkintown_dlib.h"

#include <stdio.h>
#include <stdlib.h>

#include <dlfcn.h>

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
    get = dlsym(get_libgl(), "glXGetProcAddress");
  }
  return get(name);
}

void* get_proc_address_gl(const char* name);
void* get_proc_address_glx(const char* name);

void* glXGetProcAddress(const char* name) {
  printf("glXGetProcAddress: looking up %s\n", name);
  void* result = get_proc_address_gl(name);
  if (result) {
    return result;
  }
  result = get_proc_address_glx(name);
  if (result) {
    return result;
  }
  printf("glXGetProcAddress: %s not found\n", name);
  return NULL;
}
