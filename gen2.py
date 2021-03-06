#!/usr/bin/env python3

import argparse
import enum
import json
import os
import subprocess
import sys

import attr

from pumpkinpy import parse_xml
from pumpkinpy.types import Source

FUNCTIONS = None
ENUMS = None


def load_meta(args):
    sys.path.append(args.build_dir)
    import glmeta
    global FUNCTIONS
    global ENUMS
    FUNCTIONS = glmeta.FUNCTIONS
    ENUMS = glmeta.ENUMS


@attr.s
class AnyType:
    ctype = attr.ib()

    @property
    def short_name(self):
        return self.ctype.replace('_t', '')

    @property
    def enum_name(self):
        name = self.short_name
        if name == 'bool':
            return 'Boolean'
        elif name.startswith('uint'):
            return name[0:2].upper() + name[2:]
        else:
            return name.capitalize()

    @property
    def union_name(self):
        name = self.short_name
        if 'int' in name:
            return name
        else:
            return name[0]


def gen_any_header():
    types = [AnyType('bool'), AnyType('char'),
             AnyType('float'), AnyType('double')]
    for num in (8, 16, 32, 64):
        types.append(AnyType('int{}_t'.format(num)))
        types.append(AnyType('uint{}_t'.format(num)))

    src = Source()
    src.add_cxx_include('cstdint', system=True)
    src.add_cxx_include('sstream', system=True)
    src.add_cxx_include('string', system=True)
    src.add('class Any {')
    src.add(' public:')
    src.add('  enum class Type {')
    for typ in types:
        src.add('    {},'.format(typ.enum_name))
    src.add('  };')
    # Constructors
    src.add('  Any() = default;')
    for typ in types:
        src.add('  Any(const {} value) : type_(Type::{}) {{'.format(
            typ.ctype, typ.enum_name))
        src.add('    value_.{} = value;'.format(typ.union_name))
        src.add('  }')
    # Accessors
    for typ in types:
        src.add('  {} as_{}() const {{'.format(typ.ctype, typ.short_name))
        src.add('    if (type_ != Type::{}) {{'.format(typ.enum_name))
        src.add('      throw std::runtime_error("wrong Any type");')
        src.add('    }')
        src.add('    return value_.{};'.format(typ.union_name))
        src.add('  }')
    # String conversion
    src.add('  std::string to_string() const {')
    src.add('    std::ostringstream ss;')
    src.add('    switch (type_) {')
    for typ in types:
        src.add('    case Type::{}:'.format(typ.enum_name))
        src.add('      ss << value_.{};'.format(typ.union_name))
        src.add('      return ss.str();')
    src.add('    }')
    src.add('    return "error";')
    src.add('  }')
    src.add(' private:')
    # Union
    src.add('  union {')
    for typ in types:
        src.add('    {} {};'.format(typ.ctype, typ.union_name))
    src.add('  } value_;')
    # Discriminator
    src.add('  Type type_;')
    src.add('};')
    src.add_guard('PUMPKINTOWN_ANY_HH_')
    return src


def gen_exports():
    lines = ['extern "C" {']
    for func in FUNCTIONS:
        lines.append(func.cxx_prototype())
    lines += ['}']
    return lines


def gen_trace_header():
    src = Source()
    src.add('#ifndef PUMPKINTOWN_GL_GEN_HH_')
    src.add('#define PUMPKINTOWN_GL_GEN_HH_')
    src.add_cxx_include('pumpkintown_gl_types.hh')
    src.add(gen_exports())
    src.add('namespace pumpkintown {')
    src.add('namespace real {')
    for func in FUNCTIONS:
        src.add(func.cxx_prototype())
    src.add('}')
    src.add('}')
    src.add('#endif')
    return src


def gen_trace_source():
    src = Source()
    src.add_cxx_include('pumpkintown_trace_gen.hh')
    src.add_cxx_include('cstring', system=True)
    src.add_cxx_include('mutex', system=True)
    src.add_cxx_include('pumpkintown_function_structs.hh')
    src.add_cxx_include('pumpkintown_dlib.hh')
    src.add_cxx_include('pumpkintown_serialize.hh')
    src.add_cxx_include('pumpkintown_trace.hh')
    src.add('  static std::mutex mutex;')
    for func in FUNCTIONS:
        src.add('{} {{'.format(func.cxx_decl()))
        if True:
            src.add('  pumpkintown::print_tids();')
        if True:
            src.add('  fprintf(stderr, "{}\\n");'.format(func.name))
        # Call the real function and capture its return value if it has one
        src.add('  {}pumpkintown::real::{}({});'.format(
            'auto return_value = ' if func.has_return() else '',
            func.name,
            ', '.join(param.name for param in func.params)))
        # Optional custom actions
        if func.trace_append:
            src.add('  pumpkintown::trace_append_{}({});'.format(
                func.name, func.cxx_call_args()))
        src.add('  std::lock_guard<std::mutex> lock(mutex);')
        # Store the function ID
        src.add('  pumpkintown::begin_message("{}");'.format(
            func.name))
        if func.is_replayable() and not func.is_empty():
            src.add('  pumpkintown::{} fn;'.format(func.cxx_struct_name()))
            if func.has_return() and func.return_type.stype:
                src.add('  fn.return_value = return_value;')
            for param in func.params:
                if param.array:
                    src.add('  fn.{0} = reinterpret_cast<const {1}*>({0});'.format(
                        param.name, param.array))
                else:
                    src.add('  fn.{0} = {0};'.format(param.name))
            if func.has_array_params() and not func.custom_io:
                src.add('  fn.finalize();')
            src.add('  pumpkintown::serialize()->write(fn.num_bytes());')
            src.add('  fn.write_to_file(pumpkintown::serialize()->file());')
            # Prevent double free
            for param in func.params:
                if param.array:
                    src.add('  fn.{} = nullptr;'.format(param.name))
        else:
            src.add('  pumpkintown::serialize()->write(static_cast<uint64_t>(0));')
        # Return the real function's result if it has one
        if func.has_return():
            src.add('  return return_value;')
        src.add('}')
    src.add('extern "C" void* glXGetProcAddress(const char* name) {')
    for func in FUNCTIONS:
        src.add('  if (strcmp(name, "{}") == 0) {{'.format(func.name))
        src.add('    return reinterpret_cast<void*>(&{});'.format(func.name))
        src.add('  }')
    src.add('  fprintf(stderr, "unknown function: %s\\n", name);')
    src.add('  return nullptr;')
    src.add('}')
    src.add('extern "C" void* glXGetProcAddressARB(const char* name) {')
    src.add('  return glXGetProcAddress(name);')
    src.add('}')
    # Add pumpkintown::real:: implementations
    src.add('namespace pumpkintown {')
    src.add('namespace real {')
    for func in FUNCTIONS:
        src.add('{} {{'.format(func.cxx_decl()))
        # Declare Function pointer type
        src.add('  ' + func.cxx_function_type_alias())
        # Static function pointer to the "real" call
        src.add('  static Fn real_fn = reinterpret_cast<Fn>(get_real_proc_addr("{}"));'.format(func.name))
        # Call the real function and return its value if it has one
        src.add('  {}real_fn({});'.format(
            'return ' if func.has_return() else '',
            ', '.join(param.name for param in func.params)))
        src.add('}')
    src.add('}')
    src.add('}')
    return src


def gen_gl_enum_header():
    src = Source()
    src.add_cxx_include('cstdint', system=True)
    for name, value in ENUMS.items():
        if value >= 0:
            value = '0x{:x}'.format(value)
        src.add('constexpr auto {} = {};'.format(name, value))
    # TODO(nicholasbishop): should the enum be namespaced as well?
    src.add('namespace pumpkintown {')
    src.add('const char* lookup_gl_enum_string(int64_t val);')
    src.add('}')
    src.add_guard('PUMPKINTOWN_GL_ENUM_HH_')
    return src


def gen_gl_enum_source():
    enum_map = {}
    for name, value in ENUMS.items():
        if value in enum_map:
            enum_map[value].append(name)
        else:
            enum_map[value] = [name]

    src = Source()
    src.add_cxx_include('pumpkintown_gl_enum.hh')
    src.add('namespace pumpkintown {')
    src.add('const char* lookup_gl_enum_string(const int64_t val) {')
    src.add('  switch(val) {')
    for value, names in enum_map.items():
        # Skip GL_TIMEOUT_IGNORED, it's too big for an int64_t
        if value >= 0xffffffffffffffff:
            continue
        if value >= 0:
            value = '0x{:x}'.format(value)
        src.add('  case {}:'.format(value))
        src.add('    return "{}";'.format('/'.join(names)))
    src.add('  }')
    src.add('  return "unknown-GLenum";')
    src.add('}')
    src.add('}')
    return src


def gen_function_id_header():
    src = Source()
    src.add_cxx_include('cstdint', system=True)
    src.add_cxx_include('string', system=True)
    src.add('namespace pumpkintown {')
    src.add('enum class FunctionId : uint16_t {')
    src.add('  Invalid = 0,')
    for func in FUNCTIONS:
        src.add('  {} = {},'.format(func.name, func.function_id))
    src.add('};')
    src.add('FunctionId function_id_from_name(const std::string& name);')
    src.add('}')
    src.add_guard('PUMPKINTOWN_FUNCTION_ID_HH_')
    return src


def gen_function_id_source():
    src = Source()
    src.add_cxx_include('pumpkintown_function_id.hh')
    src.add('namespace pumpkintown {')
    src.add('FunctionId function_id_from_name(const std::string& name) {')
    for func in FUNCTIONS:
        src.add('  if (name == "{}") {{'.format(func.name))
        src.add('    return FunctionId::{};'.format(func.name))
        src.add('}')
    src.add('  return FunctionId::Invalid;')
    src.add('}')
    src.add('}')
    return src


def gen_function_structs_header():
    src = Source()
    src.add_cxx_include('cstdio', system=True)
    src.add_cxx_include('cstdint', system=True)
    src.add_cxx_include('string', system=True)
    src.add('namespace pumpkintown {')
    for func in FUNCTIONS:
        if func.is_empty():
            continue
        src.add('struct __attribute__((__packed__)) {} {{'.format(
            func.cxx_struct_name()))
        if func.has_array_params():
            src.add('  ~{}();'.format(func.cxx_struct_name()))
            src.add('  void finalize();')
        src.add('  uint64_t num_bytes() const;')
        src.add('  std::string to_string() const;')
        src.add('  void read_from_file(FILE* f);')
        src.add('  void write_to_file(FILE* f);')
        if func.has_return() and func.return_type.stype:
            src.add('  {} return_value;'.format(func.return_type.stype))
        for param in func.params:
            if param.array:
                src.add('  uint64_t {}_length;'.format(param.name))
                src.add('  const {}* {};'.format(param.array, param.name))
            elif param.offset:
                src.add('  {} {};'.format(param.offset, param.name))
            else:
                src.add('  {} {};'.format(param.ptype.stype, param.name))
        src.add('};')
    src.add('}')
    src.add_guard('PUMPKINTOWN_FUNCTION_STRUCTS_HH_')
    return src


def gen_function_structs_source():
    src = Source()
    src.add_cxx_include('pumpkintown_function_structs.hh')
    src.add_cxx_include('sstream', system=True)
    src.add_cxx_include('pumpkintown_gl_enum.hh')
    src.add_cxx_include('pumpkintown_gl_util.hh')
    src.add_cxx_include('pumpkintown_io.hh')
    src.add('namespace pumpkintown {')
    # TODO(nicholasbishop): currently the format is a little
    # inefficient for pointers. We read and write them as part of
    # the struct, but the actual values aren't used.
    #
    # TODO(nicholasbishop): byte order
    for func in FUNCTIONS:
        if not func.is_replayable() or func.is_empty():
            continue
        if func.custom_io:
            continue
        if func.has_array_params():
            src.add('{0}::~{0}() {{'.format(func.cxx_struct_name()))
            for param in func.params:
                if param.array:
                    src.add('  delete[] {};'.format(param.name))
            src.add('}')
        src.add('uint64_t {}::num_bytes() const {{'.format(func.cxx_struct_name()))
        size_args = ['sizeof(*this)']
        for param in func.params:
            if param.array:
                size_args.append('({0}_length * sizeof(*{0}))'.format(param.name))
        src.add('  return {};'.format(' + '.join(size_args)))
        src.add('}')
        src.add('std::string {}::to_string() const {{'.format(func.cxx_struct_name()))
        src.add('  std::string result = "{}(";'.format(func.name))
        first = True
        for param in func.params:
            if first:
                first = False
            else:
                src.add('  result += ", ";')
            if param.array:
                pass
            elif param.custom_print:
                src.add('  result += {}({});'.format(
                    param.custom_print, param.name))
            elif param.offset:
                src.add('  {')
                src.add('    std::ostringstream oss;')
                src.add('    oss << {};'.format(param.name))
                src.add('    result += oss.str();')
                src.add('  }')
            elif param.ptype.ctype == 'GLenum':
                src.add('  result += lookup_gl_enum_string({});'.format(param.name))
            else:
                src.add('  result += pumpkintown::to_string({});'.format(param.name))
        src.add('  std::string return_string;')
        if func.has_return():
            src.add('  return_string = " -> " + pumpkintown::to_string(return_value);')
        src.add('  return result + ")" + return_string;'.format(func.name))
        src.add('}')
        src.add('void {}::read_from_file(FILE* f) {{'.format(func.cxx_struct_name()))
        src.add('  read_exact(f, this, sizeof(*this));')
        for param in func.params:
            if param.array:
                src.add('  if ({}_length > 0) {{'.format(param.name))
                src.add('    {0}* {1}_tmp = new {0}[{1}_length];'.format(
                    param.array, param.name))
                src.add('    read_exact(f, {0}_tmp, {0}_length * sizeof(*{0}));'.format(
                    param.name))
                src.add('    {0} = {0}_tmp;'.format(param.name))
                src.add('  } else {')
                src.add('    {} = nullptr;'.format(param.name))
                src.add('  }')
        src.add('}')
        src.add('void {}::write_to_file(FILE* f) {{'.format(func.cxx_struct_name()))
        src.add('  write_exact(f, this, sizeof(*this));')
        for param in func.params:
            if param.array:
                src.add('  write_exact(f, {0}, {0}_length * sizeof(*{0}));'.format(
                    param.name))
        src.add('  fflush(f);')
        src.add('}')
    src.add('}')
    return src


def gen_gl_functions_header():
    src = Source()
    src.add_cxx_include('pumpkintown_gl_types.hh')
    src.add('namespace pumpkintown {')
    for func in FUNCTIONS:
        src.add('{} {}({});'.format(
            func.return_type.ctype,
            func.name,
            ', '.join(param.ptype.ctype for param in func.params)))
    src.add('}')
    src.add_guard('PUMPKINTOWN_GL_FUNCTIONS_HH_')
    return src


def gen_gl_functions_source():
    src = Source()
    src.add_cxx_include('pumpkintown_gl_functions.hh')
    src.add_cxx_include('waffle-1/waffle.h', system=True)
    src.add('namespace pumpkintown {')
    for func in FUNCTIONS:
        src.add('{} {}({}) {{'.format(
            func.return_type.ctype,
            func.name,
            ', '.join(param.cxx() for param in func.params)))
        src.add(func.cxx_function_type_alias())
        src.add('  Fn fn = reinterpret_cast<Fn>(waffle_get_proc_address("{}"));'.format(
            func.name))
        src.add('  {}fn({});'.format(
            'return ' if func.has_return() else '',
            ', '.join(param.name for param in func.params)))
        src.add('}')
    src.add('}')
    return src


def gen_replay_source():
    src = Source()
    src.add_cxx_include('pumpkintown_replay.hh')
    src.add_cxx_include('vector', system=True)
    src.add_cxx_include('waffle-1/waffle.h', system=True)
    src.add_cxx_include('pumpkintown_function_id.hh')
    src.add_cxx_include('pumpkintown_function_structs.hh')
    src.add_cxx_include('pumpkintown_gl_functions.hh')
    src.add_cxx_include('pumpkintown_gl_types.hh')
    src.add('namespace pumpkintown {')
    src.add('void Replay::replay_one() {')
    src.add('  switch (iter_.function_id()) {')
    src.add('  case FunctionId::Invalid:')
    src.add('    break;')
    for func in FUNCTIONS:
        src.add('  case FunctionId::{}:'.format(func.name))
        if func.no_replay:
            src.add('    iter_.skip();')
            src.add('    break;')
        if func.name == 'glXSwapBuffers':
            src.add('    printf("{}\\n");'.format(func.name))
            src.add('    waffle_window_swap_buffers(window_);')
            src.add('    break;')
            continue
        elif not func.is_replayable():
            src.add('    printf("{}\\n");'.format(func.name))
            src.add('    printf("stub\\n");')
            src.add('    break;')
            continue
        src.add('    {')
        if not func.is_empty():
            src.add('      {} fn;'.format(func.cxx_struct_name()))
            src.add('      fn.read_from_file(iter_.file());')
            src.add('      printf("%s\\n", fn.to_string().c_str());')
        if func.custom_replay:
            src.add('      custom_{}(fn);'.format(func.name))
        else:
            src.add('      {}({});'.format(
                func.name, ', '.join('fn.' + param.name for param in func.params)))
        src.add('      break;')
        src.add('    }')
    src.add('  }')
    src.add('}')
    src.add('}')
    return src


def gen_express_source():
    src = Source()
    src.add_cxx_include('epoxy/egl.h', system=True)
    src.add_cxx_include('epoxy/gl.h', system=True)
    src.add_cxx_include('pumpkintown_express.hh')
    src.add('namespace pumpkintown {')
    src.add('void Express::dispatch() {')
    for func in FUNCTIONS:
        if not func.is_replayable():
            continue
        # TODO
        if func.name in ('glBindShadingRateImageNV',
                         'glBufferAttachMemoryNV',
                         'glDrawMeshTasksIndirectNV',
                         'glDrawMeshTasksNV',
                         'glFramebufferFetchBarrierEXT',
                         'glMultiDrawMeshTasksIndirectCountNV',
                         'glMultiDrawMeshTasksIndirectNV',
                         'glNamedBufferAttachMemoryNV',
                         'glNamedRenderbufferStorageMultisampleAdvancedAMD',
                         'glRenderbufferStorageMultisampleAdvancedAMD',
                         'glResetMemoryObjectParameterNV',
                         'glScissorExclusiveNV',
                         'glShadingRateImageBarrierNV',
                         'glShadingRateSampleOrderNV',
                         'glTexAttachMemoryNV',
                         'glTextureAttachMemoryNV'):
            continue
        if func.name in ('eglCreateContext', 'eglMakeCurrent'):
            continue
        if func.name.startswith('glX'):
            continue
        src.add('  if (iter_.function() == "{}") {{'.format(func.name))
        if func.name == 'glShaderSource':
            src.add('    shader_source();')
        else:
            if func.name.startswith('glGen'):
                src.add('    uint32_arrays_[arg(1)] = '
                        'std::vector<uint32_t>{};')
                src.add('    uint32_arrays_[arg(1)].resize(to_uint32(arg(0)));')
            args = []
            for index, param in enumerate(func.params):
                arg = 'arg({})'.format(index)
                stype = param.ptype.stype
                if param.array:
                    args.append('{} == "null" ? nullptr : '
                                '{}_arrays_.at({}).data()'.format(
                        arg,
                        param.array.replace('_t', ''), arg))
                elif param.offset:
                    args.append('to_offset({})'.format(arg))
                elif stype and stype != 'const void*':
                    args.append('to_{}({})'.format(
                        stype.replace('_t', ''), arg))
            args = ', '.join(args)
            capture = ''
            if func.has_return():
                capture = 'vars_[iter_.return_value()] = '
            src.add('    {}{}({});'.format(capture, func.name, args))
        src.add('    return;')
        src.add('  }')
    src.add('  if (iter_.function() == "context_create") {')
    src.add('    create_context();')
    src.add('    return;')
    src.add('  }')
    src.add('  if (iter_.function() == "make_current") {')
    src.add('    make_context_current();')
    src.add('    return;')
    src.add('  }')

    src.add('  fprintf(stderr, "unhandled function: %s\\n", iter_.function().c_str());')
    src.add('}')
    src.add('}')
    return src


def gen_dump_cc():
    # Yes, this is Python code that generates C++ code that generates
    # C++ code. I'm sorry :(
    src = Source()
    src.add_cxx_include('pumpkintown_dump.hh')
    src.add_cxx_include('cstdio', system=True)
    src.add_cxx_include('pumpkintown_gl_enum.hh')
    src.add_cxx_include('pumpkintown_function_structs.hh')
    src.add_cxx_include('pumpkintown_function_id.hh')
    src.add('namespace pumpkintown {')
    src.add('void Dump::dump_one() {')
    src.add('  switch (iter_.function_id()) {')
    src.add('  case FunctionId::Invalid:')
    src.add('    break;')
    for func in FUNCTIONS:
        src.add('  case FunctionId::{}:'.format(func.name))
        if not func.is_replayable():
            src.add('    break;')
            continue
        if func.custom_io:
            src.add('    break;')
            continue
        src.add('    {')
        if func.params:
            src.add('      {} fn;'.format(func.cxx_struct_name()))
            src.add('      fn.read(iter_.file());')
        args = []
        placeholders = []
        src.add('      printf("{\\n");')
        for param in func.params:
            if param.array:
                src.add('      printf("  const {} {}[] = {{\\n");'.format(
                    param.array, param.name))
                src.add('      for (uint64_t i = 0; i < fn.{}_length; i++) {{'.format(
                    param.name))
                src.add('        printf("    {},\\n", fn.{}[i]);'.format(
                    param.array_element_printf(), param.name))
                src.add('      }')
                src.add('      printf("  }\\n");')
                placeholders.append(param.name)
            elif param.ptype.ctype == 'GLenum':
                args.append('lookup_gl_enum_string(fn.{})'.format(param.name))
                placeholders.append('%s')
            else:
                args.append('fn.{}'.format(param.name))
                placeholders.append(param.ptype.printf)
        src.add('      printf("  {}({});\\n"{}{});'.format(
            func.name,
            ', '.join(placeholders),
            ', ' if func.params else '',
            ', '.join(args)))
        src.add('      printf("}\\n");')
        src.add('      break;')
        src.add('    }')
    src.add('  }')
    src.add('}')
    src.add('}')
    return src


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('build_dir')
    parser.add_argument('source_dir')
    args = parser.parse_args()

    load_meta(args)

    gen_any_header().write(os.path.join(args.build_dir, 'pumpkintown_any.hh'))

    gen_trace_source().write(os.path.join(args.build_dir, 'pumpkintown_trace_gen.cc'))
    gen_trace_header().write(os.path.join(args.build_dir, 'pumpkintown_trace_gen.hh'))

    gen_function_id_header().write(os.path.join(args.build_dir, 'pumpkintown_function_id.hh'))
    gen_function_id_source().write(os.path.join(args.build_dir, 'pumpkintown_function_id.cc'))
    gen_function_structs_header().write(os.path.join(args.build_dir, 'pumpkintown_function_structs.hh'))
    gen_function_structs_source().write(os.path.join(args.build_dir, 'pumpkintown_function_structs.cc'))

    gen_gl_enum_header().write(os.path.join(args.build_dir, 'pumpkintown_gl_enum.hh'))
    gen_gl_enum_source().write(os.path.join(args.build_dir, 'pumpkintown_gl_enum.cc'))

    gen_gl_functions_header().write(os.path.join(args.build_dir, 'pumpkintown_gl_functions.hh'))
    gen_gl_functions_source().write(os.path.join(args.build_dir, 'pumpkintown_gl_functions.cc'))

    #gen_dump_cc().write(os.path.join(args.build_dir, 'pumpkintown_dump_gen.cc'))
    gen_replay_source().write(os.path.join(args.build_dir, 'pumpkintown_replay_gen.cc'))
    gen_express_source().write(os.path.join(args.build_dir, 'pumpkintown_express_gen.cc'))


if __name__ == '__main__':
    main()
