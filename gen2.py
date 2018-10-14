#!/usr/bin/env python3

import argparse
import enum
import json
import os
import subprocess

import attr

import parse_xml


@attr.s
class Type:
    ctype = attr.ib()
    stype = attr.ib(default=None)
    printf = attr.ib(default=None)

    @property
    def is_void(self):
        return self.ctype == 'void'


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


@attr.s
class Param:
    ptype = attr.ib()
    name = attr.ib()

    def cxx(self):
        return '{} {}'.format(self.ptype.ctype, self.name)

    def serialize_type(self):
        return self.ptype.stype


@attr.s
class Function:
    name = attr.ib()
    return_type = attr.ib()
    params = attr.ib()

    def has_return(self):
        return not self.return_type.is_void

    def cxx_decl(self):
        params = ', '.join(param.cxx() for param in self.params)
        return '{} {}({})'.format(self.return_type.ctype, self.name, params)

    def cxx_prototype(self):
        return '{};'.format(self.cxx_decl())

    def cxx_function_type_alias(self):
        return 'using Fn = {} (*)({});'.format(
            self.return_type.ctype,
            ', '.join(param.ptype.ctype for param in self.params))

    def cxx_body(self):
        lines = ['{} {{'.format(self.cxx_decl())]
        lines.append('  ' + self.cxx_function_type_alias())
        # Static function pointer to the "real" call
        lines.append('  static Fn real_fn = reinterpret_cast<Fn>(get_real_proc_addr("{}"));'.format(self.name))
        lines.append('  pumpkintown::serialize()->write(pumpkintown::FunctionId::{});'.format(
            self.name))
        #lines.append('  fprintf(stderr, "trace: {}\\n");'.format(self.name))
        if self.is_serializable():
            for param in self.params:
                lines.append('  pumpkintown::serialize()->write(static_cast<{}>({}));'.format(
                    param.ptype.stype, param.name))

        # Call the real function
        lines.append('  {}real_fn({});'.format(
            'auto return_value = ' if self.has_return() else '',
            ', '.join(param.name for param in self.params)))

        if self.name == 'glGenTextures':
            lines.append('  pumpkintown::serialize_standard_gl_gen({});'.format(
                ', '.join(param.name for param in self.params)))
        elif self.name == 'glTexImage2D':
            lines.append('  pumpkintown::serialize_tex_image_2d({});'.format(
                ', '.join(param.name for param in self.params)))

        # Return the real function's result if it has one
        if self.has_return():
            lines.append('  return return_value;')
        lines.append('}')
        return lines

    def capnp_struct_name(self):
        return 'Fn{}{}'.format(self.name[0].upper(), self.name[1:]).replace('_', '')

    def capnp_union_name(self):
        name = self.capnp_struct_name()
        return name[0].lower() + name[1:]

    def is_serializable(self):
        for param in self.params:
            if param.serialize_type() is None:
                return False
        return True

    def capnp_struct(self):
        lines = ['struct {} {{'.format(self.capnp_struct_name())]
        for index, param in enumerate(self.params):
            lines.append('  {} @{} :{};'.format(underscore_to_camel_case(param.name),
                                                index, param.capnp_type()))
        lines.append('}')
        return lines


FUNCTIONS = []


def load_glinfo(args):
    global FUNCTIONS
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
            ptr_ctype = ctype + ' *'
            const_ptr_ctype = 'const ' + ptr_ctype
            const_ptr_ptr_ctype = 'const ' + ptr_ctype + '*'
            types[const_ctype] = Type(const_ctype)
            types[ptr_ctype] = Type(ptr_ctype)
            types[const_ptr_ctype] = Type(const_ptr_ctype)
            types[const_ptr_ptr_ctype] = Type(const_ptr_ptr_ctype)

    path1 = os.path.join(args.source_dir, 'OpenGL-Registry/xml/gl.xml')
    path2 = os.path.join(args.source_dir, 'OpenGL-Registry/xml/glx.xml')
    commands = parse_xml.parse_xml(path1)
    commands += parse_xml.parse_xml(path2)
    for command in commands:
        try:
            rtype = types[command.return_type]
            params = []
            for param in command.params:
                params.append(Param(name=param.name, ptype=types[param.ptype]))
            FUNCTIONS.append(Function(command.name, rtype, params))
        except KeyError as err:
            print(err)


def gen_exports():
    lines = ['extern "C" {']
    for func in FUNCTIONS:
        lines.append(func.cxx_prototype())
    lines += ['}']
    return lines


def gen_hh_file():
    src = Source()
    src.add('#ifndef PUMPKINTOWN_GL_GEN_HH_')
    src.add('#define PUMPKINTOWN_GL_GEN_HH_')
    src.add_cxx_include('pumpkintown_gl_types.hh')
    src.add(gen_exports())
    src.add('#endif')
    return src


def gen_cc_file():
    src = Source()
    src.add_cxx_include('cstring', system=True)
    src.add_cxx_include('pumpkintown_dlib.hh')
    src.add_cxx_include('pumpkintown_gl_gen.hh')
    src.add_cxx_include('pumpkintown_serialize.hh')
    for func in FUNCTIONS:
        src.add(func.cxx_body())
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


def gen_function_id_header():
    src = Source()
    src.add('namespace pumpkintown {')
    src.add('enum class FunctionId {')
    src.add('  Invalid,')
    for func in FUNCTIONS:
        src.add('  {},'.format(func.name))
    src.add('};')
    src.add('}')
    src.add_guard('PUMPKINTOWN_FUNCTION_ID_HH_')
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


def gen_replay_cc():
    src = Source()
    src.add_cxx_include('pumpkintown_replay.hh')
    src.add_cxx_include('waffle-1/waffle.h', system=True)
    src.add_cxx_include('pumpkintown_deserialize.hh')
    src.add_cxx_include('pumpkintown_gl_functions.hh')
    src.add_cxx_include('pumpkintown_gl_types.hh')
    src.add('namespace pumpkintown {')
    src.add('bool Replay::replay() {')
    src.add('  FunctionId function_id{FunctionId::Invalid};')
    src.add('  deserialize_->read(&function_id);')
    src.add('  switch (function_id) {')
    src.add('  case FunctionId::Invalid:')
    src.add('    break;')
    for func in FUNCTIONS:
        src.add('  case FunctionId::{}:'.format(func.name))
        src.add('    printf("{}\\n");'.format(func.name))
        if func.name == 'glXSwapBuffers':
            src.add('    waffle_window_swap_buffers(waffle_window_);')
            src.add('    break;')
            continue
        elif func.name == 'glGenTextures':
            src.add('    gen_textures();')
            src.add('    break;')
            continue
        elif func.name == 'glBindTexture':
            src.add('    bind_texture();')
            src.add('    break;')
            continue
        elif func.name == 'glTexImage2D':
            src.add('    tex_image_2d();')
            src.add('    break;')
            continue
        elif not func.is_serializable():
            src.add('    printf("stub\\n");')
            src.add('    break;')
            continue
        src.add('    {')
        for param in func.params:
            src.add('      {} {};'.format(param.ptype.stype, param.name))
            src.add('      deserialize_->read(&{});'.format(param.name))
        src.add('      {}({});'.format(func.name, ', '.join(param.name for param in func.params)))
        src.add('      break;')
        src.add('    }')
    src.add('  }')
    src.add('  return true;')
    src.add('}')
    src.add('}')
    return src


def gen_dump_cc():
    src = Source()
    src.add_cxx_include('pumpkintown_dump.hh')
    src.add_cxx_include('cstdio', system=True)
    src.add_cxx_include('pumpkintown_deserialize.hh')
    src.add_cxx_include('pumpkintown_function_id.hh')
    src.add('namespace pumpkintown {')
    src.add('bool handle_trace_item(Deserialize* deserialize) {')
    src.add('  FunctionId function_id{FunctionId::Invalid};')
    src.add('  deserialize->read(&function_id);')
    src.add('  switch (function_id) {')
    src.add('  case FunctionId::Invalid:')
    src.add('    break;')
    for func in FUNCTIONS:
        src.add('  case FunctionId::{}:'.format(func.name))
        if not func.is_serializable():
            src.add('    break;')
            continue
        src.add('    {')
        args = []
        placeholders = []
        for param in func.params:
            src.add('      {} {};'.format(param.serialize_type(), param.name))
            src.add('      deserialize->read(&{});'.format(param.name))
            args.append(param.name)
            placeholders.append(param.ptype.printf)
        src.add('      printf("{}({});\\n"{}{});'.format(
            func.name,
            ', '.join(placeholders),
            ', ' if func.params else '',
            ', '.join(args)))
        src.add('      break;')
        src.add('    }')
    src.add('  }')
    src.add('  return true;')
    src.add('}')
    src.add('}')
    return src


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('build_dir')
    parser.add_argument('source_dir')
    args = parser.parse_args()

    load_glinfo(args)

    gen_cc_file().write(os.path.join(args.build_dir, 'pumpkintown_gl_gen.cc'))
    gen_hh_file().write(os.path.join(args.build_dir, 'pumpkintown_gl_gen.hh'))

    gen_function_id_header().write(os.path.join(args.build_dir, 'pumpkintown_function_id.hh'))

    gen_gl_functions_header().write(os.path.join(args.build_dir, 'pumpkintown_gl_functions.hh'))
    gen_gl_functions_source().write(os.path.join(args.build_dir, 'pumpkintown_gl_functions.cc'))

    gen_dump_cc().write(os.path.join(args.build_dir, 'pumpkintown_dump_gen.cc'))
    gen_replay_cc().write(os.path.join(args.build_dir, 'pumpkintown_replay_gen.cc'))


if __name__ == '__main__':
    main(
)
