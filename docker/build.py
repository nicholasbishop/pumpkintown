#!/usr/bin/env python3

import os
import subprocess

def run_cmd(*cmd):
    print(' '.join(cmd))
    subprocess.check_call(cmd)


def main():
    script_dir = os.path.dirname(os.path.realpath(__file__))
    dockerfile = os.path.join(script_dir, 'Dockerfile')
    repodir = os.path.join(script_dir, os.pardir)
    container = 'pumpkintown'
    run_cmd('sudo', 'docker', 'build', '--file', dockerfile,
            '--tag', container, script_dir)
    run_cmd('sudo', 'docker', 'run',
            '--volume', '{}:/pumpkintown'.format(repodir),
            container)


if __name__ == '__main__':
    main()
