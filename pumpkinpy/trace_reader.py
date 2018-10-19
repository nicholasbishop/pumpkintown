import struct

import attr

@attr.s
class Call:
    pass


class TraceReader:
    def __init__(self, path):
        self._file = open(path, 'rb')

    def done(self):
        # TODO
        return False

    def read_call(self):
        header = struct.Struct('<HQ')
        buf = self._file.read(header.size)
        function_id, size = header.unpack(buf)
        print(function_id, size)
        self._file.read(size)
