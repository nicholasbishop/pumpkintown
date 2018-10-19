#ifndef PUMPKINTOWN_GL_TYPES_HH_
#define PUMPKINTOWN_GL_TYPES_HH_

#include <cstddef>
#include <cstdint>

typedef unsigned int GLbitfield;
typedef unsigned char GLboolean;
typedef signed char GLbyte;
typedef char GLchar;
typedef float GLclampf;
typedef double GLdouble;
typedef unsigned int GLenum;
typedef float GLfloat;
typedef unsigned int GLhandleARB;
typedef int GLint;
typedef short GLshort;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned int GLuint;
typedef uint64_t GLuint64;

typedef GLint GLfixed;

typedef void* GLXContext;
typedef void* GLXDrawable;
typedef void Display;
typedef bool Bool;
typedef void XVisualInfo;

using GLDEBUGPROC = void (*)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);
using GLDEBUGPROCARB = void (*)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);
using GLXContext = void*;
using GLXContextID = void*;
using GLXFBConfig = void*;
using GLXPbuffer = void*;
using GLXPixmap = void*;
using GLXWindow = void*;
using GLcharARB = char;
using GLclampd = double;
using GLint64 = int64_t;
using GLintptr = ptrdiff_t;
using GLintptrARB = ptrdiff_t;
using GLsizeiptr = ptrdiff_t;
using GLsizeiptrARB = ptrdiff_t;
using GLsync = void*;
using GLuint64EXT = uint64_t;
using GLushort = unsigned short;
using Pixmap = void*;
using Window = void*;
using GLXFBConfigSGIX = void*;
using GLXPbufferSGIX = void*;
using GLvdpauSurfaceNV = void*;
using Font = void*;
using GLDEBUGPROCKHR = void*;
using GLeglImageOES = void*;
using GLvoid = void;
using GLhalfNV = unsigned short;
using GLint64EXT = int64_t;
using GLeglClientBufferEXT = void*;
using GLclampx = int;

#endif  // PUMPKINTOWN_GL_TYPES_HH_
