# GAMES101 学习记录

这是我的 GAMES101 计算机图形学课程学习仓库，用于记录作业实现、实验结果、问题排查和阶段性总结。

本仓库只保存个人编写的代码和原创学习笔记。课程课件、作业 PDF、原始框架压缩包、虚拟机镜像及安装程序不会上传；如需课程原始资料，请从课程官方渠道获取。

## 学习环境

- Windows 11
- Visual Studio Code
- MSYS2 UCRT64 / GCC
- CMake + Ninja
- Eigen、OpenCV
- Oracle VirtualBox 中的课程 Ubuntu 环境用于最终兼容性验证

详细配置见 [开发环境说明](docs/DEVELOPMENT.md)，课程进度和复盘记录见 [学习日志](LEARNING_LOG.md)。

## 目录约定

```text
GAMES101_Homework/
├── pa0/                  # 环境与齐次变换练习
├── Assignment1/          # 光栅化基础
│   └── code/
├── Assignment2/          # 三角形光栅化与抗锯齿
├── Assignment3/          # 管线、插值、纹理与着色
├── Assignment4/          # Bézier 曲线
├── Assignment5/          # 光线追踪基础
├── Assignment6/          # BVH 加速结构
├── Assignment7/          # 路径追踪
├── Assignment8/          # 质点弹簧系统
├── docs/                 # 自己编写的环境和学习文档
└── LEARNING_LOG.md       # 进度与复盘
```

目录会随着学习进度逐步补齐。

## Assignment7 提交入口

基础路径追踪、三个求交函数迁移及阴影条纹修复已实现；结果与运行方法见
[Assignment7 提交说明](Assignment7/Assignment7_code/README.md)，
学习文档和打包入口见 [Assignment7 导航](Assignment7/README.md)。
主图为 784×784、16 SPP，作者记录渲染 59 秒；课程虚拟机兼容性仍待验证。

## 构建方式

每个作业都是独立的 CMake 工程。进入包含 `CMakeLists.txt` 的目录后执行：

```bash
cmake -S . -B build-ucrt -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64

cmake --build build-ucrt
```

对于使用旧版 CMake 声明的工程，可以追加：

```bash
-DCMAKE_POLICY_VERSION_MINIMUM=3.5
```

请使用生成在 `build-ucrt/` 内的目标程序，不要使用 VS Code 的“生成活动文件”单独编译 `main.cpp`。

使用 VS Code 时，建议将整个 `GAMES101_Homework` 作为工作区打开，再通过
`CMake: Select Project` 切换 PA0、Assignment 1 等独立工程。UCRT64 的公共设置
保存在仓库根目录的 `.vscode/settings.json`，无需为每份作业重复配置。

## 仓库原则

- 先理解算法，再实现和记录。
- 每次提交尽量只包含一个明确主题。
- 不提交构建目录、可执行文件、课程原始资料或虚拟机文件。
- Windows 下完成日常开发后，重要作业会在课程 Ubuntu 环境中重新构建验证。
- 本仓库内容仅供个人学习复盘，请不要直接复制作为课程作业提交。

## 版权与说明

课程名称、题目描述和原始代码框架的权利归其原作者及课程团队所有。本仓库不是课程官方网站，也不代表课程团队。更多说明见 [NOTICE](NOTICE.md)。
