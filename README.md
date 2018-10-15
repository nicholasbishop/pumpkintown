## Dependencies

### Cap'n Proto

https://capnproto.org/install.html#installation-unix

### Waffle

https://github.com/waffle-gl/waffle.git

## Building

    git submodule update --init
    mkdir build && cd build && cmake -G Ninja .. && ninja
    
## Serialization format

The format is currently very simple and will absolutely change.

The file represents an array of function calls. Each call starts with
a 32-bit function ID. TODO: parameter encoding
