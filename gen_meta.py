#!/usr/bin/env python3

import argparse
import json
import os

from pumpkinpy import parse_xml
from pumpkinpy.types import Function, Param, Source, Type

def load_glinfo(args):
    functions = []
    types = {}
    path = os.path.join(args.source_dir, 'types.json')
    with open(path) as rfile:
        root = json.load(rfile)

        for typ in root:
            ctype = typ['c']
            stype = typ.get('serialize')
            printf = typ.get('printf')
            types[ctype] = Type(ctype, stype, printf)

            const_ctype = 'const ' + ctype
            const_stype = 'const void*' if stype == 'const void*' else None
            ptr_ctype = ctype + ' *'
            const_ptr_ctype = 'const ' + ptr_ctype
            const_ptr_ptr_ctype = 'const ' + ptr_ctype + '*'
            types[const_ctype] = Type(const_ctype, stype=const_stype)
            types[ptr_ctype] = Type(ptr_ctype, printf='%p', stype='const void*')
            types[const_ptr_ctype] = Type(const_ptr_ctype, printf='%p', stype='const void*')
            types[const_ptr_ptr_ctype] = Type(const_ptr_ptr_ctype,
                                              printf='%p', stype='const void*')

    path1 = os.path.join(args.source_dir, 'OpenGL-Registry/xml/gl.xml')
    path2 = os.path.join(args.source_dir, 'OpenGL-Registry/xml/glx.xml')
    registry = parse_xml.Registry()
    parse_xml.parse_xml(registry, path1)
    parse_xml.parse_xml(registry, path2)
    function_id = 1
    for command in registry.commands:
        try:
            rtype = types[command.return_type]
            params = []
            for param in command.params:
                params.append(Param(name=param.name, ptype=types[param.ptype]))
            functions.append(Function(function_id, command.name, rtype, params))
            function_id += 1
        except KeyError as err:
            print(err)

    enums = registry.enums

    path = os.path.join(args.source_dir, 'overrides.json')
    with open(path) as rfile:
        root = json.load(rfile)

        for key, val in root.items():
            found = False
            for func in functions:
                if func.name == key:
                    for param_name, overrides in val.get('params', {}).items():
                        param = func.param(param_name)
                        param.array = overrides.get('array')
                        param.offset = overrides.get('offset')
                        param.custom_print = overrides.get('custom_print')
                    func.custom_replay = val.get('custom_replay')
                    func.no_replay = val.get('no_replay')
                    func.custom_io = val.get('custom_io')
                    func.trace_append = val.get('trace_append')
                    found = True
                    break
            if not found:
                pass #raise KeyError('func not found: ' + key)

    return functions, enums


def main():
    # TODO deduplicate with gen2
    parser = argparse.ArgumentParser()
    parser.add_argument('build_dir')
    parser.add_argument('source_dir')
    args = parser.parse_args()

    path = os.path.join(args.build_dir, 'glmeta.py')

    functions, enums = load_glinfo(args)

    with open(path, 'w') as wfile:
        src = Source()
        src.add('from pumpkinpy.types import Function, Param, Type')
        src.add('FUNCTIONS = (')
        for func in functions:
            src.add('  {},'.format(func))
        src.add(')')
        src.add('ENUMS = {')
        for key, value in enums.items():
            src.add("  '{}': {}".format(key, value))
        src.add('}')
        for key, value in enums.items():
            src.add('{} = {}'.format(key, value))
        wfile.write(src.text())


if __name__ == '__main__':
    main()
