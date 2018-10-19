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
class Context:
    var = attr.ib()


class Exploder:
    def __init__(self, trace, output):
        self._output = output
        self._png_index = 0
        self._reader = trace_reader.TraceReader(trace)
        self._src = Source()
        self._context_map = {}
        self._next_id = 0

    def _take_id(self):
        nid = self._next_id
        self._next_id += 1
        return nid

    def save_texture_png(self, call):
        width = call.fields['width']
        height = call.fields['height']
        size = (width, height)
        pixels = call.fields['pixels']
        fmt = call.fields['format']
        typ = call.fields['type']

        if fmt == glmeta.GL_RGBA and typ == glmeta.GL_UNSIGNED_BYTE:
            mode = 'RGBA'
        elif fmt == glmeta.GL_BGRA and typ == glmeta.GL_UNSIGNED_BYTE:
            # TODO
            mode = 'RGBA'
        else:
            raise ValueError(fmt, typ)

        img = PIL.Image.frombytes(mode, size, pixels, decoder_name='raw')

        path = os.path.join(self._output, 'tex_{:04}.png'.format(self._png_index))
        self._png_index += 1

        with open(path, 'wb') as wfile:
            img.save(wfile)

    def save_raw(self, prefix, index, data):
        path = os.path.join(self._output, 'tex_{:04}.raw'.format(index))
        with open(path, 'wb') as wfile:
            wfile.write(data)

    def create_context(self, call):
        var = f'ctx{self._take_id()}'
        # TODO
        share_ctx = 'nullptr'
        self._src.add(f'  auto {var} = waffle_context_create(config, {share_ctx});')
        self._context_map[call.fields['return_value']] = Context(var)

    def make_context_current(self, call):
        orig_ctx = call.fields['ctx']
        if orig_ctx == 0:
            context = 'nullptr'
        else:
            context = self._context_map[orig_ctx].var
        self._src.add(f'  waffle_make_current(display, window, {context});')

    def standard_gen(self, call):
        count = call.fields['n']
        var = f'id{self._take_id()}'
        self._src.add(f'  GLuint {var}[{count}] = {{0}};')
        self._src.add(f'  {call.func.name}({count}, {var});')

    def standard_delete(self, call):
        # TODO
        pass

    # TODO dedup with tex_image
    def buffer_data(self, call):
        index = self._take_id()
        self.save_raw('buf_', index, call.fields['data'])
        args = []
        self._src.add('  {')
        for param in call.func.params:
            if param.name == 'data':
                self._src.add('    std::vector<uint8_t> vec;')
                # TODO load the data from a file
                args.append('vec.data()')
            else:
                args.append(str(call.fields[param.name]))
        args = ', '.join(args)
        self._src.add(f'    {call.func.name}({args});')
        self._src.add('  }')

    def tex_image(self, call):
        index = self._take_id()
        self.save_raw('tex_', index, call.fields['pixels'])
        args = []
        self._src.add('  {')
        for param in call.func.params:
            if param.name == 'pixels':
                self._src.add('    std::vector<uint8_t> vec;')
                # TODO load the data from a file
                args.append('vec.data()')
            else:
                args.append(str(call.fields[param.name]))
        args = ', '.join(args)
        self._src.add(f'    {call.func.name}({args});')
        self._src.add('  }')

    def handle_call(self, call):
        if call.func.name in ('glTexSubImage2D', 'glTexImage2D'):
            # TODO
            # self.save_texture_png(call)
            pass

        if call.func.name in ('glXCreateNewContext', 'glXCreateContextAttribsARB'):
            self.create_context(call)
        elif call.func.name == 'glXMakeContextCurrent':
            self.make_context_current(call)
        elif call.func.name in ('glGenBuffers', 'glGenTextures',
                                'glGenFramebuffers', 'glGenVertexArrays',
                                'glGenRenderbuffers'):
            self.standard_gen(call)
        elif call.func.name in ('glDeleteTextures',):
            self.standard_delete(call)
        elif call.func.name == 'glTexImage2D':
            self.tex_image(call)
        elif call.func.name == 'glBufferData':
            self.buffer_data(call)
        elif call.func.name in ('glXWaitGL', 'glXWaitX'):
            # TODO
            pass
        else:
            args = []
            for param in call.func.params:
                if param.array:
                    # TODO
                    args.append('todo')
                    continue
                args.append(str(call.fields[param.name]))
            args = ', '.join(args)
            self._src.add(f'  {call.func.name}({args});')

    def explode(self):
        self._src.add_cxx_include('vector', system=True)
        self._src.add_cxx_include('epoxy/gl.h', system=True)
        self._src.add_cxx_include('waffle-1/waffle.h', system=True)
        self._src.add('void draw(waffle_display* display, waffle_window* window, waffle_config* config) {')

        try:
            while True:
                call = self._reader.read_call()
                if call:
                    self.handle_call(call)
        except StopIteration:
            pass

        self._src.add('}')

        path = os.path.join(self._output, 'replay.cc')
        with open(path, 'w') as wfile:
            wfile.write(self._src.text())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('trace')
    parser.add_argument('output')
    args = parser.parse_args()

    exploder = Exploder(args.trace, args.output)
    exploder.explode()

    shutil.copy('templates/Makefile', args.output)
    

if __name__ == '__main__':
    main()
