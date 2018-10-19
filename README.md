## Dependencies

### Waffle

https://github.com/waffle-gl/waffle.git

### Python3

- attrs
- purepng

## Building

    git submodule update --init
    mkdir build && cd build && cmake -G Ninja .. && ninja
    
## Serialization format

The format is currently very simple and will absolutely change.

The file represents an array of function calls. Each call starts with
a 16-bit function ID, followed by a 64-bit length field. TODO:
parameter encoding
