#include <cstdio>

#include "pumpkintown_dlib.hh"

using Bool = bool;
using Display = void;
using GLXContext = void*;
using GLXDrawable = void*;
using GLbitfield = unsigned int;
using GLclampf = float;
using GLdouble = double;
using GLenum = unsigned int;
using GLfloat = float;
using GLubyte = unsigned char;

extern "C" {
  //void glClear(GLbitfield mask);
  //void glClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a);
  //void glEnable(GLenum cap);
  void glEnd();
  void glMatrixMode(GLenum mode);
  void glNormal3f(GLfloat nx,  GLfloat ny,  GLfloat nz);
  void glTranslated(GLdouble x, GLdouble y, GLdouble z);
  void* glXGetProcAddressARB (const GLubyte *);
  void* glXGetProcAddress (const GLubyte *);
  Bool glXMakeCurrent(Display * dpy,  GLXDrawable drawable,  GLXContext ctx);
}

void glClear(GLbitfield mask) {
  printf("glClear\n");
}

void glClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a) {
  printf("glClearColor\n");
}

void glEnable(GLenum cap) {
  printf("glEnable\n");
}

void glEnd() {
  printf("glEnd\n");
}

void glMatrixMode(GLenum mode) {
  printf("glMatrixMode\n");
}

void glNormal3f(GLfloat nx,  GLfloat ny,  GLfloat nz) {
  printf("glNormal3f\n");
}

void glTranslated(GLdouble x, GLdouble y, GLdouble z) {
  printf("glTranslated\n");
}

void* glXGetProcAddress (const GLubyte *name) {
  return get_real_proc_addr(name);
}

void* glXGetProcAddressARB (const GLubyte *) {
  printf("glXGetProcAddressARB\n");
}

Bool glXMakeCurrent(Display * dpy,  GLXDrawable drawable,  GLXContext ctx) {
  printf("glXMakeCurrent\n");
}
