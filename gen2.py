#!/usr/bin/env python3

import argparse
import enum
import json
import os
import subprocess

import attr

from pumpkinpy import parse_xml
from pumpkinpy.types import Function, Param, Type


class Source:
    def __init__(self):
        self.lines = []

    def add(self, item):
        if isinstance(item, str):
            self.lines.append(item)
        else:
            self.lines += item

    def add_cxx_include(self, path, system=False):
        if system:
            string = '<{}>'.format(path)
        else:
            string = '"{}"'.format(path)
        self.lines.append('#include {}'.format(string))

    def add_guard(self, name):
        self.lines = ['#ifndef ' + name, '#define ' + name] + self.lines
        self.lines.append('#endif')

    def text(self):
        return '\n'.join(self.lines) + '\n'

    def write(self, path):
        with open(path, 'w') as wfile:
            wfile.write(self.text())


FUNCTIONS = []
ENUMS = []


def load_glinfo(args):
    global FUNCTIONS
    global ENUMS
    types = {}
    path = os.path.join(args.source_dir, 'types.json')
    with open(path) as rfile:
        root = json.load(rfile)

        for typ in root:
            ctype = typ['c']
            stype = typ.get('serialize')
            printf = typ.get('printf')
            types[ctype] = Type(ctype, stype, printf)

            const_ctype = 'const ' + ctype
            const_stype = 'const void*' if stype == 'const void*' else None
            ptr_ctype = ctype + ' *'
            const_ptr_ctype = 'const ' + ptr_ctype
            const_ptr_ptr_ctype = 'const ' + ptr_ctype + '*'
            types[const_ctype] = Type(const_ctype, stype=const_stype)
            types[ptr_ctype] = Type(ptr_ctype, printf='%p', stype='const void*')
            types[const_ptr_ctype] = Type(const_ptr_ctype, printf='%p', stype='const void*')
            types[const_ptr_ptr_ctype] = Type(const_ptr_ptr_ctype,
                                              printf='%p', stype='const void*')

    path1 = os.path.join(args.source_dir, 'OpenGL-Registry/xml/gl.xml')
    path2 = os.path.join(args.source_dir, 'OpenGL-Registry/xml/glx.xml')
    registry = parse_xml.Registry()
    parse_xml.parse_xml(registry, path1)
    parse_xml.parse_xml(registry, path2)
    for command in registry.commands:
        try:
            rtype = types[command.return_type]
            params = []
            for param in command.params:
                params.append(Param(name=param.name, ptype=types[param.ptype]))
            FUNCTIONS.append(Function(command.name, rtype, params))
        except KeyError as err:
            print(err)

    ENUMS = registry.enums

    path = os.path.join(args.source_dir, 'overrides.json')
    with open(path) as rfile:
        root = json.load(rfile)

        for key, val in root.items():
            found = False
            for func in FUNCTIONS:
                if func.name == key:
                    for param_name, overrides in val.get('params', {}).items():
                        param = func.param(param_name)
                        param.array = overrides.get('array')
                        param.offset = overrides.get('offset')
                        param.custom_print = overrides.get('custom_print')
                    func.custom_replay = val.get('custom_replay')
                    func.no_replay = val.get('no_replay')
                    func.custom_io = val.get('custom_io')
                    func.trace_append = val.get('trace_append')
                    found = True
                    break
            if not found:
                raise KeyError('func not found: ' + key)


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
    src.add('#endif')
    return src


def gen_trace_source():
    src = Source()
    src.add_cxx_include('pumpkintown_trace.hh')
    src.add_cxx_include('cstring', system=True)
    src.add_cxx_include('pumpkintown_function_structs.hh')
    src.add_cxx_include('pumpkintown_dlib.hh')
    src.add_cxx_include('pumpkintown_custom_trace.hh')
    src.add_cxx_include('pumpkintown_serialize.hh')
    for func in FUNCTIONS:
        src.add('{} {{'.format(func.cxx_decl()))
        src.add('  ' + func.cxx_function_type_alias())
        if False:
            src.add('  fprintf(stderr, "{}\\n");'.format(func.name))
        # Static function pointer to the "real" call
        src.add('  static Fn real_fn = reinterpret_cast<Fn>(get_real_proc_addr("{}"));'.format(func.name))
        # Call the real function
        src.add('  {}real_fn({});'.format(
            'auto return_value = ' if func.has_return() else '',
            ', '.join(param.name for param in func.params)))
        # Optional custom actions
        if func.trace_append:
            src.add('  pumpkintown::trace_append_{}({});'.format(
                func.name, func.cxx_call_args()))
        # Store the function ID
        src.add('  pumpkintown::serialize()->write(pumpkintown::FunctionId::{});'.format(
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
            src.add('  fn.write_from_file(pumpkintown::serialize()->file());')
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
    src.add('namespace pumpkintown {')
    src.add('enum class FunctionId : uint16_t {')
    src.add('  Invalid = 0,')
    # Number each entry to make looking up functions easier during
    # debugging
    for index, func in enumerate(FUNCTIONS):
        src.add('  {} = {},'.format(func.name, index + 1))
    src.add('};')
    src.add('}')
    src.add_guard('PUMPKINTOWN_FUNCTION_ID_HH_')
    return src


def gen_function_name_source():
    src = Source()
    src.add_cxx_include('pumpkintown_function_id.hh')
    src.add('namespace pumpkintown {')
    src.add('const char* function_id_to_string(const FunctionId id) {')
    src.add('  switch (id) {')
    src.add('  case FunctionId::Invalid:')
    src.add('    break;')
    for func in FUNCTIONS:
        src.add('  case FunctionId::{}:'.format(func.name))
        src.add('    return "{}";'.format(func.name))
    src.add('  }')
    src.add('  return "Invalid";')
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
        # TODO(nicholasbishop): consider packing the struct
        src.add('struct {} {{'.format(func.cxx_struct_name()))
        if func.has_array_params():
            src.add('  ~{}();'.format(func.cxx_struct_name()))
            src.add('  void finalize();')
        src.add('  uint64_t num_bytes() const;')
        src.add('  std::string to_string() const;')
        src.add('  void read_from_file(FILE* f);')
        src.add('  void write_from_file(FILE* f);')
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
        src.add('void {}::write_from_file(FILE* f) {{'.format(func.cxx_struct_name()))
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
    src.add_cxx_include('pumpkintown_deserialize.hh')
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

    load_glinfo(args)

    gen_trace_source().write(os.path.join(args.build_dir, 'pumpkintown_trace.cc'))
    gen_trace_header().write(os.path.join(args.build_dir, 'pumpkintown_trace.hh'))

    gen_function_id_header().write(os.path.join(args.build_dir, 'pumpkintown_function_id.hh'))
    gen_function_name_source().write(os.path.join(args.build_dir, 'pumpkintown_function_name.cc'))
    gen_function_structs_header().write(os.path.join(args.build_dir, 'pumpkintown_function_structs.hh'))
    gen_function_structs_source().write(os.path.join(args.build_dir, 'pumpkintown_function_structs.cc'))

    gen_gl_enum_header().write(os.path.join(args.build_dir, 'pumpkintown_gl_enum.hh'))
    gen_gl_enum_source().write(os.path.join(args.build_dir, 'pumpkintown_gl_enum.cc'))

    gen_gl_functions_header().write(os.path.join(args.build_dir, 'pumpkintown_gl_functions.hh'))
    gen_gl_functions_source().write(os.path.join(args.build_dir, 'pumpkintown_gl_functions.cc'))

    #gen_dump_cc().write(os.path.join(args.build_dir, 'pumpkintown_dump_gen.cc'))
    gen_replay_source().write(os.path.join(args.build_dir, 'pumpkintown_replay_gen.cc'))


if __name__ == '__main__':
    main(
)
