#!/usr/bin/env python3

import argparse

from pumpkinpy import trace_reader

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('trace')
    parser.add_argument('output')
    args = parser.parse_args()

    reader = trace_reader.TraceReader(args.trace)
    while True:
        reader.read_call()

if __name__ == '__main__':
    main()
