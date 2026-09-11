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

## PA0

PA0 只需要 C++ 编译器、CMake、Ninja 和 Eigen。若尚未安装，可在 MSYS2
UCRT64 终端执行：

```bash
pacman -S --needed \
  mingw-w64-ucrt-x86_64-toolchain \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-eigen3
```

命令行验证 PA0：

```bash
cd /d/Project/Games101/GAMES101_Homework/pa0
cmake -S . -B build-ucrt -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64
cmake --build build-ucrt
./build-ucrt/Transformation.exe
```

PA0 的 CMake 配置通过 `Eigen3::Eigen` 引入 Eigen，不需要把 Eigen 的绝对
头文件路径写进源代码或 VS Code 配置。

## VS Code

建议安装：

- C/C++ (`ms-vscode.cpptools`)
- CMake Tools (`ms-vscode.cmake-tools`)

请直接用 VS Code 打开整个 `GAMES101_Homework` 文件夹。仓库根目录的
`.vscode/settings.json` 统一选择 UCRT64 工具链，并将每份作业作为独立的
CMake 工程管理；不需要在每个作业目录中复制 `.vscode`。

当前工作区包含 PA0、Assignment 1、Assignment 2 与 Assignment 3。打开命令面板后：

1. 执行 `CMake: Select Project`，选择正在学习的作业。
2. 首次构建或更换工具链后执行 `CMake: Delete Cache and Reconfigure`。
3. 执行 `CMake: Build`。
4. 需要调试时先执行 `CMake: Set Debug Target`，再执行 `CMake: Debug`。

以后加入新作业，只需在根目录 `.vscode/settings.json` 的
`cmake.sourceDirectory` 数组中增加其 `CMakeLists.txt` 所在目录。例如：

```json
"cmake.sourceDirectory": [
    "${workspaceFolder}/pa0",
    "${workspaceFolder}/Assignment1/code",
    "${workspaceFolder}/Assignment2/Assignment2_code",
    "${workspaceFolder}/Assignment3/Assignment3_code"
]
```

这里应填写真正包含 `CMakeLists.txt` 的目录；有些作业的工程根目录可能是
`AssignmentN/code`。`${sourceDirectory}/build-ucrt` 会保证各作业使用互不干扰的
构建目录。

不要选择 `C/C++: g++.exe 生成和调试活动文件`，因为该命令只编译当前源文件，不会自动加入其他 `.cpp`、OpenCV 头文件和链接库。

不建议把所有作业合成一个顶层 CMake 工程。课程各作业的目标、依赖和框架版本
可能不同，保持独立可以减少目标重名、配置串扰和跨平台兼容问题。

Assignment 2 和 Assignment 3 还提供了 `ucrt-debug` CMake preset。在各自代码目录中可以直接运行：

```bash
cmake --preset ucrt-debug
cmake --build --preset ucrt-debug
```

该 preset 会把 UCRT64 的动态库目录放在构建进程的 `PATH` 最前面，避免误用
`C:/mingw64` 中的编译器或库。

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
