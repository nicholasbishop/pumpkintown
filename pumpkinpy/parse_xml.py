from xml.etree import ElementTree

import attr

@attr.s
class Param:
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


@attr.s
class Command:
    name = attr.ib()
    return_type = attr.ib()
    params = attr.ib()

    @classmethod
    def from_xml(cls, node):
        proto = node.find('proto')
        if not proto:
            return None
        name = proto.find('name').text
        ptype = proto.find('ptype')
        rtype = proto.text if proto.text else ''
        if ptype is not None:
            rtype += ptype.text
            rtype += ptype.tail
        rtype = rtype.strip()
        params = []
        for param in node.iter('param'):
            param = Param.from_xml(param)
            if not param:
                return None
            params.append(param)
        return Command(name, rtype, params)


class Registry:
    def __init__(self):
        self.commands = []
        self.enums = {}


def parse_xml(registry, name):
    tree = ElementTree.parse(name)
    root = tree.getroot()

    for node in root.iter('command'):
        command = Command.from_xml(node)
        if command:
            registry.commands.append(command)

    for node in root.iter('enum'):
        value = node.get('value')
        name = node.get('name')
        if name and value and '"' not in value and 'EGL_CAST' not in value:
            if value.startswith('0x'):
                value = int(value, 16)
            else:
                value = int(value)
            registry.enums[name] = value
