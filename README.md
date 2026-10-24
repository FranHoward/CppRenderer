# C++Renderer

This is a starter C++17 renderer project built from a user-provided template.

The project includes:

- GLM 1.0.3 (header-only mathematics library)
- MiniFB 0.10.1 (window and pixel-buffer display)
- CMake build configuration

## Building on Windows

Install CMake and ensure that `cmake`, `gcc`, and `g++` are available in your `PATH`. Then double-click `build.bat`, or run the following command in a terminal:

```powershell
.\build.bat
```

The generated executable is located at `build\C++Renderer.exe`.

## Building on Linux or macOS

```sh
./build.sh
```

The application entry point is `source/main.cpp`. The current example uses GLM for color calculations and MiniFB to display a gradient.
