#!/usr/bin/env python3

import argparse
import os
import sys

import PIL.Image

from pumpkinpy import trace_reader

# TODO
sys.path.append('build')
import glmeta


class Exploder:
    def __init__(self, trace, output):
        self._output = output
        self._png_index = 0
        self._reader = trace_reader.TraceReader(trace)

    def save_texture(self, call):
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

    def explode(self):
        while True:
            call = self._reader.read_call()
            if not call:
                continue
            if call.func.name == 'glTexSubImage2D':
                self.save_texture(call)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('trace')
    parser.add_argument('output')
    args = parser.parse_args()

    exploder = Exploder(args.trace, args.output)
    exploder.explode()
    

if __name__ == '__main__':
    main()
