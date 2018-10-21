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
            'char': 'c',
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
        self._function_map = {}

    def read(self):
        buf = self._file.read(1)
        if len(buf) == 0:
            raise StopIteration
        elif buf[0] == 1:
            self.read_function_id()
            return self.read()
        elif buf[0] == 2:
            return self.read_call()
        else:
            raise ValueError('invalid message type')

    def read_function_id(self):
        header = struct.Struct('<HB')
        buf = self._file.read(header.size)
        dyn_id, name_len = header.unpack(buf)
        name = self._file.read(name_len).decode('utf-8')
        for func in glmeta.FUNCTIONS:
            if func.name == name:
                func_id = func.function_id
                break
        print(name, dyn_id, func_id)
        self._function_map[dyn_id] = func_id

    def read_call(self):
        header = struct.Struct('<HQ')
        buf = self._file.read(header.size)
        dyn_id, size = header.unpack(buf)
        func = glmeta.FUNCTIONS[self._function_map[dyn_id] - 1]

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
            elif param.array in ('uint8_t', 'char'):
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
