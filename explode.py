#!/usr/bin/env python3

import argparse
import os
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
    def __init__(self, args):
        self.output = args.output
        self.reader = trace_reader.TraceReader(args.trace)
        self.src = Source()
        self.context_map = {}
        self.next_id = 0
        self.ctx = None
        self.call_index = 0
        self.enable_png_texture_dump = args.png_textures

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
        share_ctx = 'null'
        if orig_share_ctx:
            share_ctx = self.context_map[orig_share_ctx].var
            res = self.context_map[orig_share_ctx].res
        else:
            res = Resources()
        self.src.add(f'context_create {share_ctx} -> {var}')
        self.context_map[call.fields['return_value']] = Context(var, res)

    def make_context_current(self, call):
        orig_ctx = call.fields['ctx']
        if orig_ctx == 0:
            context = 'null'
            self.ctx = None
        else:
            context = self.context_map[orig_ctx].var
            self.ctx = self.context_map[orig_ctx]
        self.src.add(f'make_current {context}')

    def standard_gen(self, call):
        count = call.fields['n']
        var = f'id{self.take_id()}'
        self.src.add(f'{call.func.name} {count} {var}')

    def standard_delete(self, call):
        # TODO
        pass

    def buffer_data(self, call):
        index = self.take_id()
        save_name = self.save_raw('buf', index, call.fields['data'])
        target = hex(call.fields['target'])
        size = call.fields['size']
        data = f'file:{save_name}'
        usage = hex(call.fields['usage'])
        self.src.add(f'glBufferData {target} {size} {data} {usage}')

    def tex_image(self, call):
        index = self.take_id()
        save_name = self.save_raw('tex', index, call.fields['pixels'])
        args = []
        for param in call.func.params:
            if param.name == 'pixels':
                if call.fields['pixels_length'] == 0:
                    args.append('null')
                else:
                    args.append(f'file:{save_name}')
            else:
                args.append(hex(call.fields[param.name]))
        args = ' '.join(args)
        self.src.add(f'{call.func.name} {args}')

    def shader_source(self, call):
        index = self.take_id()
        save_path = self.save_raw('shader', index,
                                  call.fields['source'].encode('utf-8'))
        shader = call.fields['shader']
        shader = self.ctx.res.shaders[shader]
        self.src.add(f'glShaderSource {shader} file:{save_path}')

    def program_binary(self, call):
        index = self.take_id()
        save_path = self.save_raw('program', index, call.fields['binary'])
        self.src.add(f'const auto vec = read_all("{save_path}");')
        fmt = call.fields['binaryFormat']
        length = call.fields['length']
        program = self.ctx.res.programs[call.fields['program']]
        self.src.add(f'glProgramBinary({program}, {fmt}, vec.data(), {length});')

    def standard_create(self, call):
        prefix = 'glCreate'
        name = call.func.name[len(prefix):].lower()
        args = []
        for param in call.func.params:
            args.append(hex(call.fields[param.name]))
        args = ' '.join(args)
        var = f'{name}{self.take_id()}'
        self.src.add(f'{call.func.name} {args} -> {var}')
        self.ctx.res[name][call.return_value] = var

    def use_program(self, call):
        program = call.fields['program']
        if program != 0:
            program = self.ctx.res.programs[program]
        self.src.add(f'glUseProgram {program}')

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
        #self.src.add(f'  fprintf(stderr, "{self.call_index} {name}\\n");')

        if self.enable_png_texture_dump and name in ('glTexSubImage2D', 'glTexImage2D'):
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
                      'glXSwapIntervalMESA',
                      'eglGetCurrentContext',
                      'eglGetCurrentSurface'):
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
                    values = ' '.join(str(elem) for elem in field)
                    self.src.add(f'array {arr} {param.array} {values}')
                    args.append(arr)
                elif param.offset:
                    args.append(str(field))
                elif isinstance(field, float):
                    args.append(str(field))
                else:
                    args.append(hex(field))
            args = ' '.join(args)
            self.src.add(f'{name} {args}')
        #self.check_gl_errors(call)
        # if name == 'glDrawElements' and self.call_index > 25000:
        #     self.src.add(f'  capture("fbo{self.call_index}.png");')
        self.call_index += 1

    def explode(self):
        try:
            while True:
                call = self.reader.read()
                if call:
                    self.handle_call(call)
        except StopIteration:
            pass

        path = os.path.join(self.output, 'trace')
        with open(path, 'w') as wfile:
            wfile.write(self.src.text())


def create_template_link(args, name):
    path = os.path.join(args.output, name)
    target = os.path.join(os.pardir, 'templates', name)
    if os.path.exists(path):
        os.remove(path)
    os.symlink(target, path)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--png-textures', action='store_true')
    parser.add_argument('trace')
    parser.add_argument('output')
    args = parser.parse_args()

    exploder = Exploder(args)
    exploder.explode()

    create_template_link(args, 'Makefile')
    create_template_link(args, 'replay.cc')
    create_template_link(args, 'replay.hh')
    create_template_link(args, 'stb_image_write.h')
    

if __name__ == '__main__':
    main()
