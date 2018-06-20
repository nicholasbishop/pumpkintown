#!/usr/bin/env python3

import enum
from xml.etree import ElementTree

import attr


def indent(line, depth=2):
    return ' ' * depth + line

@attr.s
class Type:
    name = attr.ib()
    typedef = attr.ib()

    @classmethod
    def from_xml(cls, node):
        name = node.find('name')
        requires = node.get('requires')
        if requires == 'khrplatform':
            return None
        if name is not None:
            if ' ' in name.text or name.text == 'GLvoid':
                return None
            if name.tail != ';':
                return None
            return cls(name.text, node.text)

    def gen_c_wrap_function_proto(self):
        yield 'PumpkintownTypeUnion pumpkintown_wrap_{}({} value);'.format(
            self.name, self.name)

    def gen_c_wrap_function(self):
        yield 'PumpkintownTypeUnion pumpkintown_wrap_{}({} value) {{'.format(
            self.name, self.name)
        yield '  PumpkintownTypeUnion result;'
        yield '  result.value.member_{} = value;'.format(self.name)
        yield '  result.type = PUMPKINTOWN_TYPE_{};'.format(self.name.upper())
        yield '  return result;'
        yield '}'


def gen_c_type_enum(types):
    yield 'typedef enum {'
    for gltype in types:
        yield '  PUMPKINTOWN_TYPE_{},'.format(gltype.name.upper())
    yield '} PumpkintownTypeEnum;'


@attr.s
class Command:
    name = attr.ib()
    return_type = attr.ib(default='void')
    parameters = attr.ib(default=attr.Factory(list))

    @classmethod
    def from_xml(cls, node):
        proto = node.find('proto')
        if not proto:
            return None
        name = proto.find('name').text
        if name in ('glGetVkProcAddrNV', 'glXAssociateDMPbufferSGIX',
                    'glXGetProcAddress', 'glXCreateGLXVideoSourceSGIX',
                    'glXGetProcAddressARB', 'glXGetSwapIntervalMESA',
                    'glDebugMessageCallbackKHR'):
            return None
        ptype = proto.find('ptype')
        rtype = proto.text if proto.text else ''
        if ptype is not None:
            rtype += ptype.text
            rtype += ptype.tail
        rtype = rtype.strip()
        parameters = []
        for param in node.iter('param'):
            parameter = Parameter.from_xml(param)
            if not parameter:
                return None
            parameters.append(parameter)
        return Command(name, return_type=rtype, parameters=parameters)

    def gen_c(self, record_args=True):
        params = ', '.join(param.gen_c() for param in self.parameters)
        yield 'GLAPI {} GLAPIENTRY {}({}) {{'.format(self.return_type, self.name, params)
        yield '  static {} (*real_call)({}) = NULL;'.format(self.return_type, params)
        yield '  PumpkintownCall call;'
        yield '  pumpkintown_set_call_name(&call, "{}");'.format(self.name)
        if record_args:
            for param in self.parameters:
                if param.ptype.strip().endswith('*'):
                    continue
                yield '  pumpkintown_append_call_arg(&call, pumpkintown_wrap_{}({}));'.format(
                    param.ptype, param.name)
        yield '  if (!real_call) {'
        yield '    real_call = get_real_proc_addr("{}");'.format(self.name)
        yield '    if (!real_call) {'
        yield '      printf("failed to load {}\\n");'.format(self.name)
        yield '    }'
        yield '  }'
        args = ', '.join(param.name for param in self.parameters)
        maybe_ret = '' if self.return_type == 'void' else 'return '
        yield '  {}real_call({});'.format(maybe_ret, args)
        yield '}'


@attr.s
class Parameter:
    name = attr.ib()
    ptype = attr.ib()

    @classmethod
    def from_xml(cls, node):
        name = node.find('name')
        ptype = node.find('ptype')
        full_type = node.text if node.text else ''
        if ptype is not None:
            full_type += ptype.text
            full_type += ptype.tail
        return cls(name.text, ptype=full_type.strip())

    def gen_c(self):
        return '{} {}'.format(self.ptype, self.name)


def gen_c_get_proc_address(name, commands):
    yield 'void* get_proc_address_{}(const char* name) {{'.format(name)
    for command in commands:
        yield '  if (strcmp(name, "{}") == 0) {{'.format(command.name)
        yield '    return {};'.format(command.name)
        yield '  }'
    yield '  return NULL;'
    yield '}'


def write_lines(wfile, lines):
    for line in lines:
        wfile.write(line + '\n')


def write_line(wfile, line):
    wfile.write(line + '\n')


def parse_xml(name, commands, types):
    tree = ElementTree.parse(name)
    root = tree.getroot()

    for node in root.iter('command'):
        command = Command.from_xml(node)
        if command:
            commands.append(command)
        # else:
        #     print('skipping', ElementTree.tostring(node))

    for node in root.iter('type'):
        gltype = Type.from_xml(node)
        if gltype:
            types.append(gltype)


def main():
    commands = []
    types = [Type('GLhandleARB', 'typedef unsigned int '),
             Type('GLintptr', 'typedef ptrdiff_t '),
             Type('GLsizeiptr', 'typedef ptrdiff_t ')]
    parse_xml('gl.xml', commands, types)
    glx_commands = []
    parse_xml('glx.xml', glx_commands, [])

    with open('pumpkintown_types.h', 'w') as wfile:
        wfile.write('#ifndef PUMPKINTOWN_TYPES_H_\n')
        wfile.write('#define PUMPKINTOWN_TYPES_H_\n')
        wfile.write('#include <stddef.h>\n')
        wfile.write('#include <stdint.h>\n')
        wfile.write('#include <GL/gl.h>\n')
        wfile.write('typedef int GLclampx;\n')
        write_lines(wfile, gen_c_type_enum(types))
        wfile.write('typedef struct {\n')
        wfile.write('  union {\n')
        for gltype in types:
            wfile.write('    {} member_{};\n'.format(gltype.name, gltype.name))
        wfile.write('  } value;\n')
        wfile.write('  PumpkintownTypeEnum type;\n')
        wfile.write('} PumpkintownTypeUnion;\n')
        for gltype in types:
            write_lines(wfile, gltype.gen_c_wrap_function_proto())
        wfile.write('#endif  // PUMPKINTOWN_TYPES_H_\n')

    with open('pumpkintown_types.c', 'w') as wfile:
        wfile.write('#include "pumpkintown_types.h"\n')
        for gltype in types:
            write_lines(wfile, gltype.gen_c_wrap_function())

    with open('pumpkintown_commands.c', 'w') as wfile:
        write_line(wfile, '#include <GL/gl.h>')
        write_line(wfile, '#include <stdio.h>')
        write_line(wfile, '#include <string.h>')
        write_line(wfile, '#include <dlfcn.h>')
        write_line(wfile, '#include "pumpkintown_call.h"')
        write_line(wfile, '#include "pumpkintown_dlib.h"')
        write_line(wfile, '#include "pumpkintown_types.h"')
        for command in commands:
            write_lines(wfile, command.gen_c(record_args=False))
        write_lines(wfile, gen_c_get_proc_address('gl', commands))

    with open('pumpkintown_glx_commands.c', 'w') as wfile:
        write_line(wfile, '#include <stdio.h>')
        write_line(wfile, '#include <string.h>')
        write_line(wfile, '#include <GL/glx.h>')
        write_line(wfile, '#include <dlfcn.h>')
        write_line(wfile, '#include "pumpkintown_call.h"')
        write_line(wfile, '#include "pumpkintown_dlib.h"')
        for command in glx_commands:
            write_lines(wfile, command.gen_c(record_args=False))
        write_lines(wfile, gen_c_get_proc_address('glx', glx_commands))


if __name__ == '__main__':
    main()
