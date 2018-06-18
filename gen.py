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

    def gen_c_typedef(self):
        return '{}{};'.format(self.typedef, self.name)

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
        if name == 'glGetVkProcAddrNV':
            return None
        ptype = proto.find('ptype')
        rtype = 'void'
        if ptype is not None:
            rtype = ''
            if proto.text:
                rtype += proto.text
            rtype += ptype.text
            if ptype.tail:
                rtype += ptype.tail
            rtype = rtype.strip()
        parameters = []
        for param in node.iter('param'):
            parameter = Parameter.from_xml(param)
            if not parameter:
                return None
            parameters.append(parameter)
        return Command(name, return_type=rtype, parameters=parameters)

    def gen_c(self):
        params = ', '.join(param.gen_c() for param in self.parameters)
        yield '{} {}({}) {{'.format(self.return_type, self.name, params)
        yield '  static {} (*real_call)({}) = NULL;'.format(self.return_type, params)
        yield '  PumpkintownCall call;'
        yield '  pumpkintown_set_call_name(&call, "{}");'.format(self.name)
        for param in self.parameters:
            if param.ptype.strip().endswith('*'):
                continue
            yield '  pumpkintown_append_call_arg(&call, pumpkintown_wrap_{}({}));'.format(
                param.ptype, param.name)
        yield '  init_libgl();'
        yield '  if (!real_call) {'
        yield '    real_call = dlsym(libgl, "{}");'.format(self.name)
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
        if ptype is not None and ' ' not in ptype.text:
            full_type = ptype.text
            if ptype.tail is not None:
                full_type += ptype.tail
            return cls(name.text, ptype=full_type)

    def gen_c(self):
        return '{} {}'.format(self.ptype, self.name)


def gen_c_init_libgl():
    yield 'void init_libgl() {'
    yield '  if (!libgl) {'
    yield '    libgl = dlopen("libGL.so", RTLD_LAZY);'
    yield '  }'
    yield '}'


def write_lines(wfile, lines):
    for line in lines:
        wfile.write(line + '\n')


def write_line(wfile, line):
    wfile.write(line + '\n')


def main():
    tree = ElementTree.parse('gl.xml')
    root = tree.getroot()

    commands = []
    for command in root.iter('command'):
        command = Command.from_xml(command)
        if command:
            commands.append(command)

    types = []
    for node in root.iter('type'):
        gltype = Type.from_xml(node)
        if gltype:
            types.append(gltype)
    types = [Type('GLhandleARB', 'typedef unsigned int '),
             Type('GLintptr', 'typedef ptrdiff_t '),
             Type('GLsizeiptr', 'typedef ptrdiff_t ')] + types

    with open('pumpkintown_types.h', 'w') as wfile:
        wfile.write('#ifndef PUMPKINTOWN_TYPES_H_\n')
        wfile.write('#define PUMPKINTOWN_TYPES_H_\n')
        wfile.write('#include <stddef.h>\n')
        wfile.write('#include <stdint.h>\n')
        for gltype in types:
            wfile.write(gltype.gen_c_typedef() + '\n')
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
        wfile.write('#include "pumpkintown_call.h"\n')
        wfile.write('#include "pumpkintown_types.h"\n')
        write_line(wfile, '#include <dlfcn.h>')
        write_line(wfile, 'static void* libgl = NULL;')
        write_lines(wfile, gen_c_init_libgl())
        for command in commands:
            write_lines(wfile, command.gen_c())


if __name__ == '__main__':
    main()
