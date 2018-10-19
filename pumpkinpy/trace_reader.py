import struct
import sys

import attr

# TODO
sys.path.append('build')
import glmeta

@attr.s
class Call:
    func = attr.ib()
    fields = attr.ib()
    return_value = attr.ib(default=None)

    
def py_struct_type(stype):
    return {'const void*': 'Q',
            'double': 'd',
            'float': 'f',
            'int32_t': 'i',
            'int64_t': 'q',
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
        try:
            function_id, size = header.unpack(buf)
        except struct.error:
            raise StopIteration
        func = glmeta.FUNCTIONS[function_id - 1]

        if not func.is_replayable():
            return

        fmt = '<'
        field_names = []
        # Add return value
        if func.has_return():
            fmt += py_struct_type(func.return_type.stype)
            field_names.append('return_value')
        # Add parameters
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
        if 'return_value' in call.fields:
            call.return_value = call.fields['return_value']

        if func.custom_io:
            if func.name == 'glShaderSource':
                return self.read_shader_source(call)

        for param in func.params:
            if not param.array:
                continue

            length = call.fields[f'{param.name}_length']
            if length == 0:
                call.fields[param.name] = None
            elif param.array == 'uint8_t':
                call.fields[param.name] = self._file.read(length)
            else:
                elem = struct.Struct(py_struct_type(param.array))
                buf = self._file.read(elem.size * length)
                call.fields[param.name] = list(item[0] for item in elem.iter_unpack(buf))

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
