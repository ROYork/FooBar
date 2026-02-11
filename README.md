# FooBar
A  powerful collection of cross-platform C++ libraries.

## Tested on the Following compilers/platforms

| Platform    | Compiler   | Architecture  |
| ----------- | ---------- | ------------- |
| Linux       | GNU 9.4.0  | AARCH64       |
| Windows     | MSVC 19/22 | x86_64        |

## Build 

### Linux

```bash
cmake -B ./build/ -S . -DCMAKE_BUILD_TYPE=Release && cmake --build ./build/ -j
```
Force the compiler to use C++17 mode
```bash
cmake -B ./build/ -S . -DCMAKE_BUILD_TYPE=Release -DFB_FORCE_CXX17=ON && cmake --build ./build/ -j
```

### Windows
```bash
cmake -B ./build/ -S . -DCMAKE_BUILD_TYPE=Release 
cmake --build build --config Release -j
```
Force the compiler to use C++17 mode
```bash
cmake -B ./build/ -S . -DCMAKE_BUILD_TYPE=Release -DFB_FORCE_CXX17=ON 
cmake --build ./build/ --config Release -j
```
