## Dependencies

## Building

    git submodule update --init
    mkdir build && cd build && cmake -G Ninja .. && ninja
    
## Serialization format

The format is currently very simple and will absolutely change.

The file represents an array of function calls. Each call starts with
a 32-bit function ID. TODO: parameter encoding
