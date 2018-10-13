#include <cstdio>

static void on_initialize() __attribute__((constructor));
static void on_shutdown() __attribute__((destructor));

void on_initialize() {
  printf("I'm a constructor\n");
}

void on_shutdown() {
  printf("I'm a destructor\n");
}
