import struct
import sys

import attr
import png

# TODO
sys.path.append('build')
import glmeta

@attr.s
class Call:
    func = attr.ib()
    fields = attr.ib()

    
def py_struct_type(stype):
    return {'const void*': 'Q',
            'double': 'd',
            'float': 'f',
            'int32_t': 'i',
            'uint8_t': 'B',
            'uint32_t': 'I',
            'uint64_t': 'Q',
    }[stype]


class TraceReader:
    def __init__(self, path):
        self._file = open(path, 'rb')

    def read_call(self):
        header = struct.Struct('<HQ')
        buf = self._file.read(header.size)
        function_id, size = header.unpack(buf)
        func = glmeta.FUNCTIONS[function_id - 1]

        if not func.is_replayable():
            return

        fmt = '<'
        # Add return value
        if func.has_return():
            fmt += py_struct_type(func.return_type.stype)
        # Add parameters
        field_names = []
        for param in func.params:
            if param.array:
                field_names += [f'{param.name}_length', param.name]
                fmt += 'QQ'
            elif param.offset:
                field_names.append(param.name)
                fmt += 'Q'
            elif param.ptype.stype:
                field_names.append(param.name)
                fmt += py_struct_type(param.ptype.stype)
            else:
                raise RuntimeError()
        body = struct.Struct(fmt)
        buf = self._file.read(body.size)
        field_values = body.unpack(buf)

        call = Call(func, dict(zip(field_names, field_values)))

        if func.custom_io:
            if func.name == 'glShaderSource':
                return self.read_shader_source(call)

        for param in func.params:
            if param.array:
                elem = struct.Struct(py_struct_type(param.array))
                buf = self._file.read(elem.size * call.fields[f'{param.name}_length'])
                call.fields[param.name] = list(elem.iter_unpack(buf))

        return call

    def read_shader_source(self, call):
        count = call.fields['count']
        if count > 0:
            elem = struct.Struct('i')
            buf = self._file.read(count * elem.size)
            lengths = list(elem.iter_unpack(buf))

            source = ''
            for length in lengths:
                source += self._file.read(length[0]).decode('utf-8')[:-1]

            call.fields['source'] = source
        return call
