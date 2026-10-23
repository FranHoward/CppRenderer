# C++Renderer

这是一个基于用户指定模板搭建的 C++17 渲染器起始项目。

项目已内置：

- GLM 1.0.3（头文件库）
- MiniFB 0.10.1（窗口与像素缓冲显示）
- CMake 构建配置

## Windows 构建

先安装 CMake，并确保 `cmake`、`gcc` 和 `g++` 已加入 `PATH`。然后双击 `build.bat`，或在终端中运行：

```powershell
.\build.bat
```

生成的程序位于 `build\C++Renderer.exe`。

## Linux / macOS 构建

```sh
./build.sh
```

入口文件是 `source/main.cpp`。当前示例使用 GLM 计算颜色，并通过 MiniFB 显示渐变画面。
