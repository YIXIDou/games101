# Assignment 3：学习与开发指南

依据本目录 Assignment3.pdf（2020 版）整理，保留学习范围、复盘顺序和工作量估计。基础算法现已实现；当前完成状态、构建运行与提交说明以 [README](README.md) 为准。函数接口见 [FRAMEWORK_GUIDE](FRAMEWORK_GUIDE.md)，可选优化见 [RASTERIZATION_REVIEW](RASTERIZATION_REVIEW.md)。

## 作业内容与验收目标

这是 CPU 软件渲染器作业：把作业 2 的三角形光栅化扩展为逐片元着色管线。核心工作集中在 2 个 .cpp 文件的 6 个函数。

| 位置 | 要做的事 | 验收方式 |
| --- | --- | --- |
| main.cpp / get_projection_matrix | 迁移自己前面作业的投影矩阵，检查近远平面及坐标约定 | 与光栅化一起完成后，能看到完整模型 |
| rasterizer.cpp / rasterize_triangle | 包围盒遍历、覆盖测试、深度测试；插值颜色、法线、UV 和着色位置；构造 payload，调用 fragment_shader 并写入帧缓冲 | normal 模式显示平滑变化的法线颜色 |
| main.cpp / phong_fragment_shader | 实现 Blinn–Phong 环境光、漫反射和高光，累计光源贡献 | phong 模式有合理明暗和高光 |
| main.cpp / texture_fragment_shader | 读取 UV 对应纹理颜色，将其归一化后作为漫反射系数 kd，复用光照计算 | texture 模式显示奶牛贴图和光照 |
| main.cpp / bump_fragment_shader | 由高度图差分和 TBN 基变换扰动法线 | bump 模式显示扰动后的法线；框架此处输出法线颜色 |
| main.cpp / displacement_fragment_shader | 在 bump 基础上按高度修改着色位置，再计算光照 | displacement 模式出现高度细节的光照效果 |

PDF 基础分合计 50 分：提交及编译运行 5、属性插值 10、Blinn–Phong 20、纹理映射 5、bump 与 displacement 共 10。

加分项共 8 分：其他含顶点法线的 OBJ 模型及结果 3 分；在 Texture 中实现 getColorBilinear，并在 shader 中调用、提供普通采样与双线性采样对比 5 分。可用更小纹理放大差异。

现有 normal shader、OBJ 加载器、模型和贴图已提供。无需自己编写 OBJ 解析器或 OpenGL/GLSL 程序；此处 shader 是 C++ 回调。

## 学到什么程度

建议以“能独立解释并实现”以下 8 组知识为目标：

1. MVP、齐次除法、视口变换及各阶段坐标空间；理解保留的 w 与深度之间的关系。
2. 包围盒、像素中心、三角形覆盖测试、z-buffer：复习作业 2。
3. 重心坐标与属性插值：知道为什么投影后属性需要透视校正，避免把屏幕深度和视空间属性的插值混为一谈。
4. 法线：单位化、逆转置变换，插值后为什么还要归一化。
5. Blinn–Phong：点到光源/观察者方向、半程向量、点积截断、距离平方衰减、分量相乘及颜色范围。
6. 纹理：UV 到像素坐标、v 轴方向、最近邻采样；双线性采样属于加分内容。
7. bump：高度差分、切线/副切线/法线组成的 TBN、局部法线到着色空间的转换。
8. displacement：位置扰动与法线扰动的区别。该框架在 fragment shader 中改着色位置，并不重新细分或光栅化位移后的网格，不能据此期待真正改变模型轮廓。

实现需要会用 Eigen 的 dot、cross、normalized、cwiseProduct、矩阵乘法与向量分量访问。阅读框架时理解 std::function 回调与 payload 数据传递即可，不必逐行研究 OBJ_Loader.h 或 OpenCV 内部实现。

本地课件重点：第 7 讲（光照与 Blinn–Phong）、第 8 讲（着色频率、管线、纹理）、第 9 讲（重心坐标、纹理查询及应用）。MVP 和光栅化不熟时回顾第 3–6 讲。完成本作业不要求先学完后续光线追踪、路径追踪和动画。

## 学习顺序与工作量参考

以下保留开工时的有效学习/编码时间估计，不是实际耗时记录或课程统计。前提是作业 1、2 已能自己解释，且具备基础 C++ 能力。

| 阶段 | 估计时间 |
| --- | --- |
| 阅读框架，复习相关课件 | 2–3 小时 |
| 迁移投影和光栅化，完成属性插值，跑通 normal | 2–4 小时 |
| Blinn–Phong 和 texture | 2–3 小时 |
| bump 和 displacement | 3–5 小时 |
| 排错、保存对比图和整理报告 | 1–3 小时 |

基本要求约 10–18 小时；若课件和前两次作业都很熟，约 6–10 小时；若 C++、坐标变换与向量运算也需补课，预留 20–30 小时以上。加分项另留 2–5 小时。每天 2–3 小时，可按约一周安排。

按 normal → phong → texture → bump → displacement 复盘各阶段，区分法线可视化、光照、材质纹理和高度细节的作用。

## 复盘时需要区分的约定

- 光栅化使用像素中心的重心坐标进行覆盖测试，已绕过参数为 int 的旧 insideTriangle 辅助函数。
- get_index 和 set_pixel 已统一采用 (height-1-y)*width+x，区分像素编号和带 0.5 偏移的采样位置。
- Texture::getColor 在 u=1 或 v=0 时可能访问图像边界外；实现纹理和高度差分时处理 UV 边界，特别是 u+1/width、v+1/height。
- TBN 注释公式在法线靠近 y 轴时有分母为零风险，需要理解并处理退化情况。
- payload.view_pos 与法线来自 view 空间，光源和观察者也应采用一致空间；区分遵循课程参考参数与严格统一坐标空间。
- PDF 有“默认 normal”及 shader 数量的旧描述；当前代码无参数默认是 phong，应明确传 normal。

## 已配置的开发环境

使用同一套 MSYS2 UCRT64：G++ 16.2.0、CMake 4.4.2、Eigen 5.0.1、OpenCV 5.0.0、Ninja、GDB 17.2。VS Code 的 C/C++ 与 CMake Tools 扩展已安装。

- Assignment3_code/CMakeLists.txt：C++17，通过 Eigen3::Eigen 和 OpenCV 导入依赖；避免硬编码 Linux 头文件路径。
- Assignment3_code/CMakePresets.json：ucrt-debug，独立 build-ucrt，固定 UCRT64 工具链及 PATH。
- 仓库根目录 .vscode/settings.json：加入 Assignment3/Assignment3_code，清除通用调试里遗留的作业 1 参数。
- .vscode/tasks.json：配置、构建与环境检查任务。
- .vscode/launch.json：Assignment3 专用程序路径、工作目录、构建前置任务，以及着色器选择。
- --check-environment：只加载模型与两张贴图，不执行渲染算法；CTest 验证该入口。

### VS Code 操作

1. 打开整个 GAMES101_Homework 文件夹，根目录 .vscode 才会生效。
2. 通过“终端 → 运行任务”选择 Assignment3: Check environment，会依次配置、构建和检查。
3. 调试时先选“Assignment3：环境检查（算法未完成时使用）”，F5 可验证断点。
4. 选择“Assignment3：选择着色器并保存图片”，选择所需模式；默认图片写入 build-ucrt/output.png。
5. 使用 CMake Tools 面板时选择 Assignment3/Assignment3_code 项目、ucrt-debug preset（如果出现选择）、Rasterizer 目标。

### PowerShell 命令

在仓库根目录执行：

```powershell
$env:PATH = "C:/msys64/ucrt64/bin;" + $env:PATH
Set-Location Assignment3/Assignment3_code
cmake --preset ucrt-debug
cmake --build --preset ucrt-debug
ctest --test-dir build-ucrt --output-on-failure
Set-Location build-ucrt
./Rasterizer.exe --check-environment
```

在 build-ucrt 目录运行：

```powershell
./Rasterizer.exe normal.png normal
./Rasterizer.exe phong.png phong
./Rasterizer.exe texture.png texture
./Rasterizer.exe bump.png bump
./Rasterizer.exe displacement.png displacement
```

模型路径是 ../models，因此运行工作目录必须是 Assignment3_code 下的构建目录；不要在仓库根目录直接调用 exe。无参数进入交互窗口，默认 phong，a/d 旋转、Esc 退出。

课程 Ubuntu 环境仍可在 Assignment3_code 中执行 cmake -S . -B build 和 cmake --build build，然后进入 build 运行；Windows preset 专用于本机。尚未在课程虚拟机验证。

环境配置阶段已验证模型、贴图加载和 GDB 断点。最新构建及五模式运行状态统一记录在 README，避免重复维护验收结果。

## 提交材料

按本地 2020 版 PDF：包含 CMakeLists.txt 和全部源代码；建立 images/ 保存结果图；编写 README.md 说明完成的得分点、各函数实现和加分图；打包为“姓名_Homework3.zip”。清理 build、build-ucrt、.vscode 等开发文件。PDF 的提交平台和日期是历史说明。

build-ucrt 已由仓库 .gitignore 排除；需要保留的最终实验图片应复制到 images/。提交状态见 README。不要把课程 PDF、下载压缩包或生成的 exe 提交到 Git。
