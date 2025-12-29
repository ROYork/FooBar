# FooBar
A  powerful collection of cross-platform C++ libraries.
## Build 

### Linux
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
```
### Windows
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release 
cmake --build build --config Release
```