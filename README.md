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

The file represents an array of messages. Each message starts with an
8-bit tag. Currently two message types are defined: function ID
messages and function call messages.

A function name message defines a 16-bit ID that will be used for a
particular function in all following function call messages. The
encoding is a uint16 unique ID followed by a uint8 string length
followed by the function name string.

A function call message starts with a 16-bit function ID followed by a
packed structure containing the return value and argument values of a
function call. Array arguments append array data after the struct.
