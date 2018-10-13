#!/usr/bin/env python3

import argparse
import enum
import json
import os
import subprocess

import attr

class GlTypes(enum.Enum):

    @classmethod
    def from_name(cls, name):
        for _, member in cls.__members__.items():
            if member.name == name:
                return member
        raise RuntimeError('missing type: ' + name)


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
        return '\n'.join(self.lines)

    def write(self, path):
        with open(path, 'w') as wfile:
            wfile.write(self.text())


@attr.s
class Param:
    ptype = attr.ib()
    name = attr.ib()

    def cxx(self):
        return '{} {}'.format(self.ptype.value, self.name)

    def fbs_type(self):
        return self.ptype.fbs()


@attr.s
class Function:
    name = attr.ib()
    return_type = attr.ib()
    params = attr.ib()
    trace = attr.ib(default=True)

    def has_return(self):
        return self.return_type != GlTypes.Void

    def cxx_decl(self):
        params = ', '.join(param.cxx() for param in self.params)
        return '{} {}({})'.format(self.return_type.value, self.name, params)

    def cxx_prototype(self):
        return '{};'.format(self.cxx_decl())

    def cxx_body(self):
        lines = ['{} {{'.format(self.cxx_decl())]
        # Function type alias
        lines.append('  using Fn = {} (*)({});'.format(
            self.return_type.value,
            ', '.join(param.ptype.value for param in self.params)))
        # Static function pointer to the "real" call
        lines.append('  static Fn real_fn = reinterpret_cast<Fn>(get_real_proc_addr("{}"));'.format(self.name))
        if self.trace:
            lines.append('  flatbuffers::FlatBufferBuilder builder(1024);')
            lines.append('  auto item = CreateTraceItem(builder, Function_{}, '.format(
                self.fbs_struct_name()))
            lines.append('      builder.CreateStruct({}({})).Union());'.format(
                self.fbs_struct_name(),
                ', '.join(param.name for param in self.params)))
            lines.append('  builder.Finish(item);')
            lines.append('  write_trace_item(builder.GetBufferPointer(), builder.GetSize());')
        # Call the real function and return its value if it has one
        lines.append('  {}real_fn({});'.format(
            'return ' if self.has_return() else '',
            ', '.join(param.name for param in self.params)))
        lines.append('}')
        return lines

    def fbs_struct_name(self):
        return 'Fn{}{}'.format(self.name[0].upper(), self.name[1:])

    def fbs_struct(self):
        lines = ['struct {} {{'.format(self.fbs_struct_name())]
        for param in self.params:
            lines.append('  {}: {};'.format(param.name, param.fbs_type()))
        lines.append('}')
        return lines


FUNCTIONS = []
TYPES = {}


def load_glinfo(args):
    global FUNCTIONS
    global TYPES
    path = os.path.join(args.source_dir, 'glinfo.json')
    with open(path) as rfile:
        obj = json.load(rfile)

        for typ in obj['types']:
            name = typ['name']
            flatbuffer = typ.get('flatbuffer')
            printf = typ.get('printf')
            TYPES[name] = Type(name, ctype=typ['c'], flatbuffer, printf])

        for func in obj['functions']:
            params = []
            for param in func['parameters']:
                params.append(Param(GlTypes.from_name(param['type']), param['name']))
            FUNCTIONS.append(Function(func['name'],
                                      GlTypes.from_name(func['return']),
                                      params,
                                      trace=func['trace']))


# (
#     Function('glClear', GlTypes.Void, (Param(GlTypes.Bitfield, 'mask'),)),
#     Function('glClearColor', GlTypes.Void, (Param(GlTypes.Clampf, 'r'),
#                                             Param(GlTypes.Clampf, 'g'),
#                                             Param(GlTypes.Clampf, 'b'),
#                                             Param(GlTypes.Clampf, 'a'))),
#     Function('glEnable', GlTypes.Void, (Param(GlTypes.Enum, 'cap'),)),
#     Function('glGenLists', GlTypes.Uint, (Param(GlTypes.Sizei, 'range'),)),
#     Function('glXCreateContext', GlTypes.GLXContext ( Display *dpy, XVisualInfo *vis,
# 				    GLXContext shareList, Bool direct );
#     Function('glXDestroyContext', GlTypes.Void, (Param(GlTypes.DisplayPtr, 'dpy'),
#                                                  Param(GlTypes.GLXContext, 'ctx')),
#              trace=False)
# )


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
             '#include "pumpkintown_schema_generated.h"',
             'using namespace pumpkintown;'])
    src.add(gen_body())
    return src


def gen_schema_file():
    src = Source()
    lines = ['namespace pumpkintown;',
             'union Function {']
    for func in FUNCTIONS:
        if not func.trace:
            continue
        lines.append('  {},'.format(func.fbs_struct_name()))
    lines.append('}')
    for func in FUNCTIONS:
        if not func.trace:
            continue
        lines += func.fbs_struct()
    lines += ['table TraceItem {',
              '  fn: Function;',
              '}']
    src.add(lines)
    src.add('root_type TraceItem;')
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
        src.add('  case Function_{}:'.format(func.fbs_struct_name()))
        src.add('    {')
        src.add('      auto* fn = item.fn_as_{}();'.format(func.fbs_struct_name()))
        args = []
        placeholders = []
        for param in func.params:
            args.append('fn->{}()'.format(param.name))
            placeholders.append(param.ptype.printf_format_specifier())
        src.add('      printf("{}({});\\n", {});'.format(
            func.name,
            ', '.join(placeholders),
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

    load_functions(args)

    gen_cc_file().write(os.path.join(args.build_dir, 'pumpkintown_gl_gen.cc'))
    gen_hh_file().write(os.path.join(args.build_dir, 'pumpkintown_gl_gen.hh'))

    path = os.path.join(args.build_dir, 'pumpkintown_schema.fbs')
    gen_schema_file().write(path)
    subprocess.check_call(('flatc', '--cpp', path))

    gen_dump_cc().write(os.path.join(args.build_dir, 'pumpkintown_dump_gen.cc'))


if __name__ == '__main__':
    main(
)
