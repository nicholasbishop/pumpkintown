#!/usr/bin/env python3

import os
import subprocess

def run_cmd(*cmd, cwd=None):
    print(' '.join(cmd))
    subprocess.check_call(cmd, cwd=cwd)


def main():
    build_dir = '/pumpkintown/docker-build'
    if not os.path.exists(build_dir):
        os.mkdir(build_dir)

    run_cmd('cmake', '-G', 'Ninja', '..', cwd=build_dir)

    run_cmd('ninja', cwd=build_dir)

if __name__ == '__main__':
    main()
