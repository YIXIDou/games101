# 学习日志

用于记录每次作业的进度、关键知识点、调试过程和后续复盘。状态建议使用：`未开始`、`进行中`、`已完成`、`待复盘`。

| 内容 | 状态 | 关键主题 | 最近更新 |
| --- | --- | --- | --- |
| PA0 | 未记录 | Eigen、齐次坐标、CMake | - |
| Assignment 1 | 进行中 | MVP 变换、基础光栅化 | 2026-09-05 |
| Assignment 2 | 未开始 | 三角形光栅化、深度缓冲、MSAA | - |
| Assignment 3 | 未开始 | 插值、纹理、Blinn-Phong、Bump/Displacement | - |
| Assignment 4 | 未开始 | Bézier 曲线、de Casteljau 算法 | - |
| Assignment 5 | 未开始 | 光线与三角形求交、Whitted 光线追踪 | - |
| Assignment 6 | 未开始 | 包围盒、BVH、SAH | - |
| Assignment 7 | 未开始 | 路径追踪、蒙特卡洛积分、Russian Roulette | - |
| Assignment 8 | 未开始 | 质点弹簧系统、显式与半隐式积分 | - |

## 单次学习记录模板

### YYYY-MM-DD - 主题

- 目标：
- 完成内容：
- 遇到的问题：
- 原因分析：
- 解决方法：
- 验证结果：
- 下一步：

## 环境问题记录

### 2026-09-05 - Windows 本地环境

- 使用 MSYS2 UCRT64，确保 GCC、CMake、Eigen 和 OpenCV 来自同一工具链。
- 多文件作业通过 CMake 构建，避免使用 VS Code 的单文件编译按钮。
- VirtualBox 采用 `VBoxVGA` 兼容控制器运行课程 Ubuntu 18.04 镜像。
- 宿主和来宾 Guest Additions 已更新到一致版本。

