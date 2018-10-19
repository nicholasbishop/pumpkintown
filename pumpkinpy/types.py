import attr

@attr.s
class Type:
    ctype = attr.ib()
    stype = attr.ib(default=None)
    printf = attr.ib(default=None)

    @property
    def is_void(self):
        return self.ctype == 'void'


@attr.s
class Param:
    ptype = attr.ib()
    name = attr.ib()
    array = attr.ib(default=None)
    offset = attr.ib(default=None)
    custom_print = attr.ib(default=None)

    def cxx(self):
        return '{} {}'.format(self.ptype.ctype, self.name)

    def is_replayable(self):
        return self.ptype.stype != 'const void*' or self.array or self.offset

    def array_element_printf(self):
        return {'float': '%f',
                'int32_t': '%d',
                'uint8_t': '%u',
                'uint32_t': '%u'}[self.array]


@attr.s
class Function:
    function_id = attr.ib()
    name = attr.ib()
    return_type = attr.ib()
    params = attr.ib()
    custom_replay = attr.ib(default=False)
    no_replay = attr.ib(default=False)
    custom_io = attr.ib(default=False)
    trace_append = attr.ib(default=False)

    def param(self, name):
        for param in self.params:
            if param.name == name:
                return param
        raise KeyError('parameter not found: ' + name)

    def has_return(self):
        return not self.return_type.is_void

    def cxx_decl(self):
        params = ', '.join(param.cxx() for param in self.params)
        return '{} {}({})'.format(self.return_type.ctype, self.name, params)

    def cxx_prototype(self):
        return '{};'.format(self.cxx_decl())

    def cxx_function_type_alias(self):
        return 'using Fn = {} (*)({});'.format(
            self.return_type.ctype,
            ', '.join(param.ptype.ctype for param in self.params))

    def cxx_call_args(self):
        return ', '.join(param.name for param in self.params)

    def cxx_struct_name(self):
        return 'Fn{}{}'.format(self.name[0].upper(), self.name[1:])

    def is_replayable(self):
        if self.no_replay:
            return False
        if self.custom_io or self.custom_replay:
            return True
        for param in self.params:
            if not param.is_replayable():
                return False
        return True

    def has_array_params(self):
        for param in self.params:
            if param.array:
                return True
        return False

    def is_empty(self):
        return not self.params and not self.has_return()


class Source:
    def __init__(self):
        self.lines = []

    def add(self, item):
        if isinstance(item, str):
            self.lines.append(item)
        else:
            self.lines += item

    def add_cxx_include(self, path, system=False):
        if system:
            string = '<{}>'.format(path)
        else:
            string = '"{}"'.format(path)
        self.lines.append('#include {}'.format(string))

    def add_guard(self, name):
        self.lines = ['#ifndef ' + name, '#define ' + name] + self.lines
        self.lines.append('#endif')

    def text(self):
        return '\n'.join(self.lines) + '\n'

    def write(self, path):
        with open(path, 'w') as wfile:
            wfile.write(self.text())
