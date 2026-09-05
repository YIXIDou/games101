# 开发环境说明

## 推荐工作流

日常开发在 Windows 11 + VS Code + MSYS2 UCRT64 中完成；提交或阶段验收前，在课程 Ubuntu 虚拟机中做一次干净构建。

不要混用以下工具链：

- 项目使用：`C:/msys64/ucrt64/bin`
- 其他项目可能使用：`C:/mingw64/bin`
- 不使用面向 MSVC 构建的 OpenCV 库连接 UCRT64 GCC

## 安装依赖

在 MSYS2 UCRT64 终端中执行：

```bash
pacman -Syu
```

如果更新过程要求关闭终端，重新打开 UCRT64 后再次运行 `pacman -Syu`。随后安装：

```bash
pacman -S --needed \
  base-devel \
  mingw-w64-ucrt-x86_64-toolchain \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-gdb \
  mingw-w64-ucrt-x86_64-eigen3 \
  mingw-w64-ucrt-x86_64-opencv \
  mingw-w64-ucrt-x86_64-freetype \
  mingw-w64-ucrt-x86_64-glew \
  mingw-w64-ucrt-x86_64-glfw
```

验证工具来源：

```bash
which gcc
which g++
which cmake
which ninja
which gdb
```

它们应指向 `/ucrt64/bin/`。

## VS Code

建议安装：

- C/C++ (`ms-vscode.cpptools`)
- CMake Tools (`ms-vscode.cmake-tools`)

作业目录中的 `.vscode/settings.json` 负责选择 UCRT64 工具链。使用命令面板依次执行：

1. `CMake: Delete Cache and Reconfigure`
2. `CMake: Build`
3. `CMake: Set Debug Target`
4. `CMake: Debug`

不要选择 `C/C++: g++.exe 生成和调试活动文件`，因为该命令只编译当前源文件，不会自动加入其他 `.cpp`、OpenCV 头文件和链接库。

## 命令行构建

```bash
cmake -S . -B build-ucrt -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64

cmake --build build-ucrt
```

旧工程若被新版 CMake 拒绝，可在配置命令中加入：

```bash
-DCMAKE_POLICY_VERSION_MINIMUM=3.5
```

运行程序时应保持 `C:/msys64/ucrt64/bin` 在 `PATH` 中，以便找到 OpenCV 等动态库。

## 与课程 Ubuntu 环境的差异

- Windows 和 Ubuntu 的 GCC、CMake、Eigen、OpenCV 版本可能不同。
- Linux 专用路径（例如 `/usr/local/include`）应通过 CMake 条件处理。
- 不同 OpenCV 和浮点实现可能产生轻微图像或数值差异。
- OpenGL/窗口系统相关作业在 Windows 下可能需要额外适配。
- 最终验证应从新的构建目录开始，避免复用其他工具链生成的缓存。

