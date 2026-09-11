# GAMES101 Assignment 3：Pipeline and Shading

基于课程框架完成 CPU 光栅化、Blinn–Phong 光照、纹理映射、凹凸映射与片元着色位置位移。基础算法已实现，加分项留待后续完成。

## 功能与渲染模式

| 函数 | 当前实现 | 对应模式 |
| --- | --- | --- |
| get_projection_matrix | 透视投影矩阵 | 全部 |
| rasterize_triangle | 像素中心采样、覆盖与深度测试、四种属性的透视校正插值、片元着色 | 全部 |
| normal_fragment_shader | 框架提供的法线可视化 | normal |
| phong_fragment_shader | 环境光、漫反射、高光与多光源累加 | phong |
| texture_fragment_shader | UV 查询纹理，以纹理颜色作为 kd 计算光照 | texture |
| bump_fragment_shader | 高度差分、TBN 变换、扰动法线可视化 | bump |
| displacement_fragment_shader | 沿原法线移动着色位置，用新位置和新法线计算光照 | displacement |

额外模型渲染与双线性纹理采样尚未完成；getColorBilinear 仅为后续接口声明，当前程序没有调用它。基础项的评分总额为 50 分，此处列出实现范围，不代表正式评分。

## 文档导航

- [知识与学习指南](STUDY_GUIDE.md)：学习范围、主要知识点和课程要求。
- [框架函数讲解](FRAMEWORK_GUIDE.md)：函数作用、参数、调用流程与示例。
- [实现复盘与改进建议](RASTERIZATION_REVIEW.md)：按阶段保留的改进建议及已知限制。

## 构建与运行

Windows 使用同一套 MSYS2 UCRT64 的 G++、CMake、Ninja、Eigen、OpenCV 和 GDB。用 VS Code 打开整个 GAMES101_Homework 文件夹，根目录 .vscode 配置才会生效。

从仓库根目录执行 PowerShell 命令：

```powershell
$env:PATH = "C:/msys64/ucrt64/bin;" + $env:PATH
Set-Location Assignment3/Assignment3_code
cmake --preset ucrt-debug
cmake --build --preset ucrt-debug
ctest --test-dir build-ucrt --output-on-failure
Set-Location build-ucrt
./Rasterizer.exe normal.png normal
./Rasterizer.exe phong.png phong
./Rasterizer.exe texture.png texture
./Rasterizer.exe bump.png bump
./Rasterizer.exe displacement.png displacement
```

模型按 ../models 查找，运行工作目录应为 Assignment3_code 下的构建目录。无参数启动交互窗口，默认 phong，a/d 旋转、Esc 退出。

VS Code 可选择“Assignment3：选择着色器并保存图片”后按 F5。`--check-environment` 只检查模型、贴图和运行依赖，不验证渲染算法。

Linux 可在 Assignment3_code 中使用 `cmake -S . -B build`、`cmake --build build`，然后进入 build 运行。当前尚未在课程 Ubuntu 虚拟机验证。

## 验证记录

2026-09-11：修正双线性采样占位声明的 Eigen 命名空间后，在独立的 build-ucrt-submit-check 目录完成构建，CTest 环境检查 1/1 通过；normal、phong、texture、bump、displacement 五种模式均正常退出并生成 PNG。各模式此前也进行过图像检查；本次没有对所有边界场景或其他模型做系统测试。

本次五张检查图片位于 Assignment3_code/build-ucrt-submit-check，属于被 Git 忽略的构建产物。正式 images/ 结果集尚待整理，本文不引用不存在的结果图片。

## 实现约定与限制

- 当前投影的近处深度较大，光栅化对屏幕深度取负后采用小值优先比较。
- shader 保留课程框架的观察者、灯光参数和逐灯环境项累计方式；实际相机观察空间的一致性问题见改进记录。
- 纹理端点与邻居采样边界、退化法线等保护暂未扩展。用户选择按课程默认场景继续，不能据此保证任意输入安全。
- displacement 仅改变片元着色位置，不重新生成几何或更新深度，因此不会真实改变轮廓与遮挡。

## Git 与课程提交

Git 中保留源代码、必要工程配置、模型资源、个人文档与最终结果图，不提交 PDF、压缩包、可执行文件或构建缓存。仓库根目录的 VS Code 配置可保留在 Git；课程作业打包时则按要求排除开发配置和构建目录。

提交前整理 images/ 的五种结果图，检查暂存区不要混入其他作业的修改。课程 2020 版说明要求源码、CMakeLists.txt、README 和 images，并以“姓名_Homework3.zip”打包；平台和日期属于历史信息。
