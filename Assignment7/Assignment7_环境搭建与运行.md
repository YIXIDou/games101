# Assignment7：Windows 环境搭建与运行

> 本文主要记录初始环境搭建快照。当前核心函数已实现，源码默认 SPP 已改为 128；
> 实际提交状态、图片参数与运行方法以 [提交 README](Assignment7_code/README.md) 为准。

验证日期：2026-11-05。适用目录：`Assignment7/Assignment7_code/`。
学习内容、评分和耗时见 [作业要求与学习路线](Assignment7_作业要求与学习路线.md)。

## 1. 当前状态

本机已有完整 MSYS2 UCRT64 工具链，本次没有重装软件，也不需要 OpenCV、Eigen、
GPU SDK 或额外下载模型。工程使用 C++17，OBJ 加载器和所需模型已包含在源码目录里。

| 项目 | 本次结果 |
| --- | --- |
| GCC/G++ | UCRT64 16.2.0 |
| CMake | UCRT64 4.4.2 |
| Ninja | UCRT64 1.13.2 |
| GDB | UCRT64 17.2 |
| VS Code 扩展文件 | 检查到 `ms-vscode.cpptools`、`ms-vscode.cmake-tools` 已安装目录 |
| Debug | 全新配置、编译与链接通过，生成 `build-ucrt/RayTracing.exe` |
| Release | 全新配置、编译与链接通过，生成 `build-ucrt-release/RayTracing.exe` |
| 两种模式的 CTest | 各 1/1 通过；加载 6 个网格、32 个三角形，构建各网格和场景 BVH |
| GDB | Debug 环境检查入口执行后正常退出 |
| 最终渲染与虚拟机 | 尚未验证，四处核心 TODO 保留给你完成 |

VS Code 图形界面的按钮和 F5 尚未手动操作验证；对应构建命令、程序和 GDB 已实跑。
环境检查不执行求交或路径追踪，不生成图片，也不表示算法通过。

## 2. 本次配置了什么

| 文件 | 用途 |
| --- | --- |
| `Assignment7_code/CMakePresets.json` | 固定 UCRT64 编译器、Ninja、PATH，分离 Debug/Release，提供构建与测试预设 |
| `Assignment7_code/CMakeLists.txt` | 要求 C++17，关闭编译器语言扩展，注册独立环境检查 CTest |
| `GAMES101_Homework/.vscode/settings.json` | 在已有多作业列表中加入 Assignment7，保留原有工程入口 |
| `GAMES101_Homework/.vscode/tasks.json` | 新增作业 7 的 Debug/Release 配置、构建和检查任务；默认构建切到作业 7 Debug |
| `GAMES101_Homework/.vscode/launch.json` | 新增环境检查、Debug 渲染、Release 渲染入口，固定工作目录、GDB 与运行时 PATH |
| `Assignment7_code/main.cpp` | 增加 `--check-environment`，在模型加载和 BVH 构建后返回，避开未完成算法 |
| `Assignment7_code/README.md` | 替换框架空白说明，明确当前完成状态和文档入口 |

`Scene::castRay()` 和三个求交 TODO 都未实现；没有迁移作业 6 代码，没有加入加分项。
构建文件在被 `.gitignore` 排除的目录中；没有改动原始 PDF 或压缩包。

这份配置针对 VS Code 的 `settings.json`，不需要额外创建 Visual Studio 的
`CMakeSettings.json`。预设是 Windows 专用；Ubuntu 仍可用普通 CMake 配置。
预设文件格式需要 CMake ≥ 3.21；源码工程本身保持最低 CMake 3.10。
预设机制可参考 [CMake 官方文档](https://cmake.org/cmake/help/v4.4/manual/cmake-presets.7.html)。

## 3. 在 VS Code 中使用

**请用 VS Code 打开 `GAMES101_Homework/`，不是外层 `Games101/`。**
本次修改的 `.vscode` 在内层仓库中；只打开 `Assignment7_code/` 也不会使用这些根目录任务。

推荐第一次这样操作：

1. 执行菜单“终端 → 运行任务”，选 `Assignment7: Check environment Debug`。
2. 它会依次配置、构建、执行环境检查。预期最后看到 `100% tests passed`。
3. `Ctrl+Shift+B` 默认运行 `Assignment7: Build Debug`，适合编辑后重新编译。
4. 左侧“运行和调试”中选 `Assignment7：环境检查（现在可运行）`，再按 F5。
5. 完成四处 TODO 后，才使用带“渲染”的 Debug/Release 调试配置。

使用 CMake Tools 面板时，在命令面板执行 `CMake: Select Active Folder`，选
`Assignment7/Assignment7_code`；如果使用预设模式，再选择 `ucrt-debug` 或
`ucrt-release`。界面措辞可能随扩展语言变化；固定任务不依赖当前选中了哪个同名目标。

不要使用“生成活动文件”或通用的单文件 C++ 运行按钮：这个工程需要链接多个 `.cpp`。
在 `Scene::castRay()` 等处打断点调试时选 Debug；Release 用于正式耗时测量，
优化后变量和断点位置可能不如 Debug 直观。

## 4. PowerShell 可复制命令

以下第一条从 **`GAMES101_Homework/` 仓库根目录**执行。后续预设命令都在
包含 `CMakePresets.json` 的 `Assignment7_code/` 中运行。

### 4.1 配置、编译和检查 Debug（现在可以执行）

```powershell
Set-Location ./Assignment7/Assignment7_code
$env:PATH = 'C:/msys64/ucrt64/bin;' + $env:PATH

& 'C:/msys64/ucrt64/bin/cmake.exe' --preset ucrt-debug
& 'C:/msys64/ucrt64/bin/cmake.exe' --build --preset ucrt-debug
& 'C:/msys64/ucrt64/bin/ctest.exe' --preset ucrt-debug
```

三步分别负责生成构建计划、编译链接、检查程序启动和资源加载；每步成功再继续。
PATH 只在当前终端会话生效，不改系统设置。预设和 VS Code 任务也已携带 UCRT64 PATH。

也可直接运行环境检查，以看到模型统计：

```powershell
# 当前在 Assignment7_code/
Push-Location ./build-ucrt
try {
    & ./RayTracing.exe --check-environment
} finally {
    Pop-Location
}
```

预期包含：

```text
Environment check passed: 6 meshes, 32 triangles.
Intersection and path-tracing TODOs were not executed.
```

### 4.2 配置、编译和检查 Release（现在也可以执行）

```powershell
# 当前在 Assignment7_code/
$env:PATH = 'C:/msys64/ucrt64/bin;' + $env:PATH
& 'C:/msys64/ucrt64/bin/cmake.exe' --preset ucrt-release
& 'C:/msys64/ucrt64/bin/cmake.exe' --build --preset ucrt-release
& 'C:/msys64/ucrt64/bin/ctest.exe' --preset ucrt-release
```

### 4.3 真正开始渲染（完成四处 TODO 后再执行）

```powershell
# 当前在 Assignment7_code/，已经完成算法并重新构建 Release
$env:PATH = 'C:/msys64/ucrt64/bin;' + $env:PATH
Push-Location ./build-ucrt-release
try {
    & ./RayTracing.exe
} finally {
    Pop-Location
}
```

正常渲染会在工作目录生成 `binary.ppm`，即 `build-ucrt-release/binary.ppm`；
Debug 渲染则在 `build-ucrt/`。重复运行会覆盖该模式目录下同名图片，需要对比时先另存。
输出为 P6 二进制 PPM，不是损坏的文本文件，应用支持 PPM 的图片查看器打开。
需要 PNG 时请真正转换格式，不要只改扩展名。

**运行时必须进入构建目录**：`main.cpp` 中使用 `../models/cornellbox/*.obj`。
从仓库根目录直接运行 exe 不会自动切换工作目录，可能加载模型失败。

### 4.4 调整调试/正式参数

当前参数直接写在源码中，没有 `--spp`、`--width` 等命令行选项：

| 想调整什么 | 位置 | 当前值 |
| --- | --- | --- |
| 图片宽高 | `main.cpp`：`Scene scene(784, 784)` | 784×784 |
| 每像素采样数 | `Renderer.cpp`：`int spp = 16` | 16 |
| RR 存活概率 | `Scene.hpp`：`RussianRoulette` | 0.8 |
| 输出名 | `Renderer.cpp`：`fopen("binary.ppm", "wb")` | binary.ppm |

修改后要重新构建。建议先用 64×64、1 SPP 定位问题，再逐步增加；
提交图要求至少 512×512、8 SPP。保持 SPP 为正整数。
现有 `maxDepth=1` 不会自动实现递归限制，终止规则属于你要完成的算法。

## 5. 常见问题及处理

### 出现 `no return statement in function returning non-void`

当前 `Bounds3::IntersectP()`、`BVHAccel::getIntersection()`、`Scene::castRay()`
没有返回值，这是未完成算法的警告。构建可以成功，但实际调用会产生未定义行为。
不要为了消除警告随便返回黑色或关闭警告，按学习指南完成函数。

另一个警告来自课程原始 `Sphere::evalDiffuseColor()` 的空返回体；默认 Cornell Box
使用网格，没有调用它。若以后添加球体并走该接口，应补全其行为。本次保留原样。

### 找不到预设或 `CMakeLists.txt`

检查终端位置是否为 `Assignment7/Assignment7_code/`。不要在 `build-ucrt/` 中
运行 `cmake --preset ...`。若工具版本太旧，则先确认正在调用 UCRT64 的 CMake。

### 编译器显示 `C:/mingw64`，或生成器/旧缓存冲突

在源码目录重新配置选定模式：

```powershell
& 'C:/msys64/ucrt64/bin/cmake.exe' --preset ucrt-debug --fresh
& 'C:/msys64/ucrt64/bin/cmake.exe' --build --preset ucrt-debug
```

Release 同理使用 `ucrt-release`。`--fresh` 重建配置缓存，不修改源码。
检查对应目录的 `CMakeCache.txt` 中编译器和 Ninja 是否都在 `C:/msys64/ucrt64/bin/`。
不要混用 MSVC、`C:/mingw64`、Conda 或 MSYS 环境中的编译器和库。

### 启动提示缺少 DLL 或返回 `0xc0000135`

先将 UCRT64 的 `bin` 放在当前终端 PATH 前面，再运行，不要从别的工具链拷贝 DLL。
本次 Debug、Release 和 GDB 的环境检查均实跑通过；如果换电脑，重新检查运行时依赖。

### 模型加载失败或 `LoadedMeshes.size() == 1` 断言失败

优先检查工作目录与 `models/cornellbox/` 是否齐全。Debug 模式便于定位断言；
Release 关闭断言后，错误路径可能表现为崩溃。VS Code 调试入口和 CTest 已固定正确目录。

### 环境检查通过，但图片黑色、黑条、偏亮、噪声很多

环境检查只加载模型和构建 BVH；检查算法的 PDF、余弦方向、可见性、RR、光源处理
以及自相交问题。提高 SPP 可以减少正常采样噪声，不能修正错误的光照公式。

### 渲染很慢

先确认使用 Release、小图和低 SPP，再定位。当前随机数函数每次重建生成器有潜在开销；
多线程是选做内容。不能用本次只加载模型的环境检查耗时估算正式渲染速度。

### VS Code 没出现任务或在编译别的作业

确认打开的是 `GAMES101_Homework/`，然后使用名称明确以 `Assignment7:` 开头的任务。
多个作业都叫 `RayTracing`，仅选同名目标不足以区分工程。

## 6. 换电脑时的安装与课程虚拟机验证

本机无需执行安装。若另一台 Windows 电脑缺少工具，在 **MSYS2 UCRT64** 终端
安装同一套工具包（不是 PowerShell）：

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-gdb
```

包名已按 MSYS2 官方页面核对：[GCC](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-gcc)、
[CMake](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-cmake)、
[Ninja](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-ninja)、
[GDB](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-gdb)。

VS Code 安装 C/C++ 和 CMake Tools。若 MSYS2 不在默认 `C:/msys64`，同步调整
预设和 `.vscode` 中的工具路径。本次没有执行包安装命令。

完成核心算法后，在课程 Ubuntu 虚拟机中从源码目录执行普通 CMake 构建即可，
不要使用含 Windows 路径的 UCRT 预设：

```bash
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux -j2
ctest --test-dir build-linux --output-on-failure
cd build-linux
./RayTracing
```

这组 Linux 命令是待验证步骤，不是本次已运行的结果。

## 7. 文件管理与提交

- `build-ucrt/`、`build-ucrt-release/`、`build-linux/` 已由仓库的 `build-*/` 规则排除；生成物不要强制加入 Git。
- 模型、源码、CMake、个人说明可以版本管理；原始课程 PDF、下载压缩包继续只作本地参考。
- 正式结果图另存到新建的 `images/`，避免仅留在被忽略的构建目录。
- 当前整个 `Assignment7/` 是本地新增、尚未被 Git 跟踪的目录；本次没有暂存、提交或推送。
- 最终打包要复制完整源码/模型、CMake、README 和结果图，排除所有构建和临时目录。
