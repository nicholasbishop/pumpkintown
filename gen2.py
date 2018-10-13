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
    stype = attr.ib()
    printf = attr.ib()

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
    trace = attr.ib(default=True)

    def has_return(self):
        return not self.return_type.is_void

    def cxx_decl(self):
        params = ', '.join(param.cxx() for param in self.params)
        return '{} {}({})'.format(self.return_type.ctype, self.name, params)

    def cxx_prototype(self):
        return '{};'.format(self.cxx_decl())

    def cxx_body(self):
        lines = ['{} {{'.format(self.cxx_decl())]
        # Function type alias
        lines.append('  using Fn = {} (*)({});'.format(
            self.return_type.ctype,
            ', '.join(param.ptype.ctype for param in self.params)))
        # Static function pointer to the "real" call
        lines.append('  static Fn real_fn = reinterpret_cast<Fn>(get_real_proc_addr("{}"));'.format(self.name))
        if self.is_serializable():
            lines.append('  pumpkintown::serialize()->write(pumpkintown::FunctionId::{});'.format(
                self.name))
            for param in self.params:
                lines.append('  pumpkintown::serialize()->write(static_cast<{}>({}));'.format(
                    param.ptype.stype, param.name))
        else:
            # TODO
            pass

        # Call the real function and return its value if it has one
        lines.append('  {}real_fn({});'.format(
            'return ' if self.has_return() else '',
            ', '.join(param.name for param in self.params)))
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
    src.add('extern "C" void* glXGetProcAddressARB(const char* name) {')
    for func in FUNCTIONS:
        src.add('  if (strcmp(name, "{}") == 0) {{'.format(func.name))
        src.add('    return reinterpret_cast<void*>(&{});'.format(func.name))
        src.add('  }')
    src.add('  fprintf(stderr, "unknown function: %s\\n", name);')
    src.add('  return nullptr;')
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
    src.add_guard('PUMPKINTOWN_FUNCTION_ID_H_')
    return src


def gen_schema_file():
    src = Source()
    src.add('@0xdf015bce75b91ed3;')
    src.add('using Cxx = import "/capnp/c++.capnp";')
    src.add('$Cxx.namespace("pumpkintown");')
    src.add('struct Function {')
    src.add('  union {')
    index = 0
    for func in FUNCTIONS:
        if not func.is_serializable():
            continue
        src.add('    {} @{} :{};'.format(func.capnp_union_name(), index,
                                         func.capnp_struct_name()))
        index += 1
    src.add('  }')
    src.add('}')
    for func in FUNCTIONS:
        if not func.is_serializable():
            continue
        src.add(func.capnp_struct())
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
    src.add('  if (!deserialize->read(&function_id)) {')
    src.add('    return false;')
    src.add('  }')
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
            src.add('      if (!deserialize->read(&{})) {{'.format(param.name))
            src.add('        return false;')
            src.add('      }')
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

    #path = os.path.join(args.build_dir, 'pumpkintown_schema.capnp')
    #gen_schema_file().write(path)
    #subprocess.check_call(('capnp', 'compile', '-oc++', path))

    gen_dump_cc().write(os.path.join(args.build_dir, 'pumpkintown_dump_gen.cc'))


if __name__ == '__main__':
    main(
)
