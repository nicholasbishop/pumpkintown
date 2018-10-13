#!/usr/bin/env python3

import argparse
import enum
import json
import os
import subprocess

import attr

import parse_xml


def underscore_to_camel_case(string):
    output = ''
    cap_next = False
    for char in string:
        if char == '_':
            cap_next = True
        elif cap_next:
            cap_next = False
            output += char.upper()
        else:
            output += char
    return output


@attr.s
class Type:
    ctype = attr.ib()
    capnp = attr.ib()
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

    def capnp_type(self):
        return self.ptype.capnp


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
        if self.trace:
            lines.append('  flatbuffers::FlatBufferBuilder builder(1024);')
            lines.append('  auto item = CreateTraceItem(builder, Function_{}, '.format(
                self.capnp_struct_name()))
            lines.append('      builder.CreateStruct({}({})).Union());'.format(
                self.capnp_struct_name(),
                ', '.join(param.name for param in self.params)))
            lines.append('  builder.Finish(item);')
            lines.append('  write_trace_item(builder.GetBufferPointer(), builder.GetSize());')
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
            if param.capnp_type() is None:
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
            capnp = typ.get('capnp')
            printf = typ.get('printf')
            types[ctype] = Type(ctype, capnp, printf)

        # for func in obj['functions']:
        #     params = []
        #     for param in func.get('parameters', []):
        #         params.append(Param(TYPES[param['type']], param['name']))
        #     return_type = TYPES[func.get('return', 'Void')]
        #     trace = func.get('trace', True)
        #     FUNCTIONS.append(Function(func['name'],
        #                               return_type,
        #                               params,
        #                               trace=trace))

    path = os.path.join(args.source_dir, 'OpenGL-Registry/xml/gl.xml')
    commands = parse_xml.parse_xml(path)
    for command in commands:
        try:
            rtype = types[command.return_type]
            params = []
            for param in command.params:
                params.append(Param(name=param.name, ptype=types[param.ptype]))
            FUNCTIONS.append(Function(command.name, rtype, params))
        except KeyError as err:
            print(err)


# (
#     Function('glClearColor', GlTypes.Void, (Param(GlTypes.Clampf, 'r'),
#                                             Param(GlTypes.Clampf, 'g'),
#                                             Param(GlTypes.Clampf, 'b'),
#                                             Param(GlTypes.Clampf, 'a'))),


def gen_exports():
    lines = ['extern "C" {']
    for func in FUNCTIONS:
        lines.append(func.cxx_prototype())
    lines += ['}']
    return lines


def gen_body():
    lines = []
    for func in FUNCTIONS:
        lines += func.cxx_body()
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
    src.add(['#include "pumpkintown_dlib.hh"',
             '#include "pumpkintown_gl_gen.hh"',
             '#include "pumpkintown_schema.capnp.h"',
             'using namespace pumpkintown;'])
    src.add(gen_body())
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
    src.add('using namespace pumpkintown;')
    src.add('void handle_trace_item(const TraceItem& item) {')
    src.add('  switch (item.fn_type()) {')
    src.add('  case Function_NONE:')
    src.add('    break;')
    for func in FUNCTIONS:
        if not func.trace:
            continue
        src.add('  case Function_{}:'.format(func.capnp_struct_name()))
        src.add('    {')
        if func.params:
            src.add('      auto* fn = item.fn_as_{}();'.format(func.capnp_struct_name()))
        args = []
        placeholders = []
        for param in func.params:
            args.append('fn->{}()'.format(param.name))
            placeholders.append(param.ptype.printf)
        src.add('      printf("{}({});\\n"{}{});'.format(
            func.name,
            ', '.join(placeholders),
            ', ' if func.params else '',
            ', '.join(args)))
        src.add('      break;')
        src.add('    }')
    src.add('  }')
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

    path = os.path.join(args.build_dir, 'pumpkintown_schema.capnp')
    gen_schema_file().write(path)
    subprocess.check_call(('capnp', 'compile', '-oc++', path))

    gen_dump_cc().write(os.path.join(args.build_dir, 'pumpkintown_dump_gen.cc'))


if __name__ == '__main__':
    main(
)
