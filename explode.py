#!/usr/bin/env python3

import argparse

from pumpkinpy import trace_reader

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('path')
    args = parser.parse_args()

    reader = trace_reader.TraceReader(args.path)
    while not reader.done():
        reader.read_function()

if __name__ == '__main__':
    main()
