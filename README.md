# CITS3003 Graphics and Animation Project

## Group Members

| Name | Student ID |
|---|---|
| Zikun Hu | 24339091 |
| Hongshen Zheng | 24367195 |


## Operating System Used

This project was developed and tested on:

```text
macOS Tahoe 26.3.1
```


## Requirements

The project uses the provided CITS3003 project skeleton and its dependencies.

Main requirements:

- C++ compiler with C++17 support
- CMake
- OpenGL-compatible environment
- GLFW
- GLM
- ImGui
- Other libraries included or configured by the provided project skeleton

If using the provided project setup, most dependencies should already be included in the project structure or configured through CMake.

## Build Instructions

From the project root directory, create and configure a build folder:

```bash
cmake -S . -B build
```

Build the project:

```bash
cmake --build build
```


## Fixing the Initial CMake Configuration Error

When using newer CMake versions, especially CMake 4.x, the first configuration may fail because some third-party libraries still declare compatibility with very old CMake versions. The error usually looks similar to:

```text
Compatibility with CMake < 3.5 has been removed from CMake.
```

This project fixes that by setting the minimum policy version in the root `CMakeLists.txt`:

```cmake
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
```

