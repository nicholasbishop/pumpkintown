#!/usr/bin/env python3

import argparse
import os
import shutil
import sys

import PIL.Image
import attr

from pumpkinpy import trace_reader
from pumpkinpy.types import Source

# TODO
sys.path.append('build')
import glmeta

@attr.s
class Resources:
    programs = attr.ib(factory=dict)
    shaders = attr.ib(factory=dict)

    def __getitem__(self, name):
        return getattr(self, name + 's')

@attr.s
class Context:
    var = attr.ib()
    res = attr.ib()


class Exploder:
    def __init__(self, trace, output):
        self.output = output
        self.reader = trace_reader.TraceReader(trace)
        self.src = Source()
        self.context_map = {}
        self.next_id = 0
        self.ctx = None
        self.call_index = 0

    def take_id(self):
        nid = self.next_id
        self.next_id += 1
        return nid

    def save_texture_png(self, call):
        width = call.fields['width']
        height = call.fields['height']
        size = (width, height)
        pixels = call.fields['pixels']
        fmt = call.fields['format']
        typ = call.fields['type']

        if not pixels:
            return

        if fmt == glmeta.GL_RGBA and typ == glmeta.GL_UNSIGNED_BYTE:
            mode = 'RGBA'
        elif fmt == glmeta.GL_RGB and typ == glmeta.GL_UNSIGNED_BYTE:
            mode = 'RGB'
        elif fmt == glmeta.GL_RGBA and typ == glmeta.GL_FLOAT:
            mode = 'F'
        elif fmt == glmeta.GL_RED and typ == glmeta.GL_UNSIGNED_BYTE:
            mode = 'L'
        elif fmt == glmeta.GL_BGRA and typ == glmeta.GL_UNSIGNED_BYTE:
            # TODO
            mode = 'RGBA'
        else:
            raise ValueError(hex(fmt), hex(typ))

        img = PIL.Image.frombytes(mode, size, pixels, decoder_name='raw')

        path = os.path.join(
            self.output, 'tex_{:04}.png'.format(self.call_index))

        with open(path, 'wb') as wfile:
            img.save(wfile)

    def save_raw(self, prefix, index, data):
        if not data:
            return
        ext = 'raw'
        if prefix == 'shader':
            ext = 'txt'
        name = '{}_{:04}.{}'.format(prefix, index, ext)
        path = os.path.join(self.output, name)
        with open(path, 'wb') as wfile:
            wfile.write(data)
        return path

    def create_context(self, call):
        var = f'ctx{self.take_id()}'
        orig_share_ctx = call.fields.get('share_context')
        share_ctx = 'nullptr'
        if orig_share_ctx:
            share_ctx = self.context_map[orig_share_ctx].var
            res = self.context_map[orig_share_ctx].res
        else:
            res = Resources()
        self.src.add(f'  auto {var} = waffle_context_create(config, {share_ctx});')
        self.context_map[call.fields['return_value']] = Context(var, res)

    def make_context_current(self, call):
        orig_ctx = call.fields['ctx']
        if orig_ctx == 0:
            context = 'nullptr'
            self.ctx = None
        else:
            context = self.context_map[orig_ctx].var
            self.ctx = self.context_map[orig_ctx]
        self.src.add(f'  waffle_make_current(display, window, {context});')

    def standard_gen(self, call):
        count = call.fields['n']
        var = f'id{self.take_id()}'
        self.src.add(f'  GLuint {var}[{count}] = {{0}};')
        self.src.add(f'  {call.func.name}({count}, {var});')

    def standard_delete(self, call):
        # TODO
        pass

    # TODO dedup with tex_image
    def buffer_data(self, call):
        index = self.take_id()
        save_name = self.save_raw('buf', index, call.fields['data'])
        args = []
        self.src.add('  {')
        for param in call.func.params:
            if param.name == 'data':
                self.src.add(f'    const auto vec = read_all("{save_name}");')
                args.append('vec.data()')
            else:
                args.append(str(call.fields[param.name]))
        args = ', '.join(args)
        self.src.add(f'    {call.func.name}({args});')
        self.src.add('  }')

    def tex_image(self, call):
        index = self.take_id()
        save_name = self.save_raw('tex', index, call.fields['pixels'])
        args = []
        self.src.add('  {')
        for param in call.func.params:
            if param.name == 'pixels':
                if call.fields['pixels_length'] == 0:
                    args.append('nullptr')
                else:
                    self.src.add(f'    const auto vec = read_all("{save_name}");')
                    args.append('vec.data()')
            else:
                args.append(str(call.fields[param.name]))
        args = ', '.join(args)
        self.src.add(f'    {call.func.name}({args});')
        self.src.add('  }')

    def shader_source(self, call):
        index = self.take_id()
        save_path = self.save_raw('shader', index,
                                  call.fields['source'].encode('utf-8'))
        shader = call.fields['shader']
        shader = self.ctx.res.shaders[shader]
        self.src.add('  {')
        self.src.add(f'    const auto vec = read_all("{save_path}");')
        self.src.add(f'    const char* src = reinterpret_cast<const char*>(vec.data());')
        self.src.add(f'    glShaderSource({shader}, 1, &src, nullptr);')
        self.src.add('  }')

    def program_binary(self, call):
        index = self.take_id()
        save_path = self.save_raw('program', index, call.fields['binary'])
        self.src.add('  {')
        self.src.add(f'    const auto vec = read_all("{save_path}");')
        fmt = call.fields['binaryFormat']
        length = call.fields['length']
        program = self.ctx.res.programs[call.fields['program']]
        self.src.add(f'    glProgramBinary({program}, {fmt}, vec.data(), {length});')
        self.src.add('  }')

    def standard_create(self, call):
        prefix = 'glCreate'
        name = call.func.name[len(prefix):].lower()
        args = []
        for param in call.func.params:
            args.append(str(call.fields[param.name]))
        args = ', '.join(args)
        var = f'{name}{self.take_id()}'
        self.src.add(f'  const auto {var} = {call.func.name}({args});')
        self.ctx.res[name][call.return_value] = var

    def use_program(self, call):
        program = call.fields['program']
        if program != 0:
            program = self.ctx.res.programs[program]
        self.src.add(f'  glUseProgram({program});')

    def check_gl_errors(self, call):
        self.src.add('  check_gl_error();')
        if call.func.name == 'glLinkProgram':
            program = call.fields['program']
            if program != 0:
                program = self.ctx.res.programs[program]
            self.src.add(f'  check_program_link({program});')
        elif call.func.name == 'glCompileShader':
            shader = call.fields['shader']
            if shader != 0:
                shader = self.ctx.res.shaders[shader]
            self.src.add(f'  check_shader_compile({shader});')

    def handle_call(self, call):
        name = call.func.name
        self.src.add(f'  fprintf(stderr, "{self.call_index} {name}\\n");')

        if name in ('glTexSubImage2D', 'glTexImage2D'):
            self.save_texture_png(call)

        if name in ('glXCreateNewContext', 'glXCreateContextAttribsARB',
                    'eglCreateContext'):
            self.create_context(call)
        elif name in ('glXMakeContextCurrent', 'eglMakeCurrent'):
            self.make_context_current(call)
        elif name in ('glGenBuffers', 'glGenTextures',
                      'glGenFramebuffers', 'glGenVertexArrays',
                      'glGenRenderbuffers'):
            self.standard_gen(call)
        elif name in ('glDeleteTextures',):
            self.standard_delete(call)
        elif name in ('glCreateProgram', 'glCreateShader'):
            self.standard_create(call)
        elif name == 'glUseProgram':
            self.use_program(call)
        elif name in ('glTexImage2D', 'glTexSubImage2D'):
            self.tex_image(call)
        elif name == 'glBufferData':
            self.buffer_data(call)
        elif name == 'glShaderSource':
            self.shader_source(call)
        elif name == 'glProgramBinary':
            self.program_binary(call)
        elif name in ('glXWaitGL', 'glXWaitX',
                      'glXSwapIntervalMESA'):
            # TODO
            pass
        else:
            args = []
            for param in call.func.params:
                field = call.fields[param.name]
                if param.resource:
                    if field == 0:
                        args.append(0)
                    else:
                        args.append(self.ctx.res[param.resource][field])
                elif param.array:
                    arr = f'arr{self.take_id()}'
                    length = call.fields[f'{param.name}_length']
                    init = ', '.join(str(elem) for elem in field)
                    self.src.add(f'  const {param.array} {arr}[{length}] = {{{init}}};')
                    args.append(arr)
                elif param.offset:
                    args.append(f'(const void*){field}')
                else:
                    args.append(str(field))
            args = ', '.join(args)
            self.src.add(f'  {name}({args});')
        self.check_gl_errors(call)
        self.call_index += 1

    def explode(self):
        self.src.add_cxx_include('vector', system=True)
        self.src.add_cxx_include('epoxy/egl.h', system=True)
        self.src.add_cxx_include('epoxy/gl.h', system=True)
        self.src.add_cxx_include('waffle-1/waffle.h', system=True)
        self.src.add_cxx_include('replay.hh')
        self.src.add('void draw(waffle_display* display, waffle_window* window, waffle_config* config) {')

        try:
            while True:
                call = self.reader.read()
                if call:
                    self.handle_call(call)
        except StopIteration:
            pass

        self.src.add('}')

        path = os.path.join(self.output, 'draw.cc')
        with open(path, 'w') as wfile:
            wfile.write(self.src.text())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('trace')
    parser.add_argument('output')
    args = parser.parse_args()

    exploder = Exploder(args.trace, args.output)
    exploder.explode()

    shutil.copy('templates/Makefile', args.output)
    shutil.copy('templates/replay.cc', args.output)
    shutil.copy('templates/replay.hh', args.output)
    

if __name__ == '__main__':
    main()
