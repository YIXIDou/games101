# Assignment 2 已有代码导读

本文只讲解 Assignment 2 框架中已经完成的代码：它们负责什么、数据怎样流动，
以及在实现剩余 TODO 时应该怎样调用这些接口。本文不会给出
`insideTriangle()` 或 `rasterize_triangle()` 的具体答案。

源码目录为 [`Assignment2_code/`](Assignment2_code/)。

## 1. 当前完成状态

| 位置 | 状态 | 作用 |
| --- | --- | --- |
| `get_view_matrix()` | 已完成 | 构造观察变换矩阵 |
| `get_model_matrix()` | 已完成 | 返回本作业要求的单位模型矩阵 |
| `get_projection_matrix()` | 已完成 | 构造透视投影矩阵 |
| `load_positions()` / `load_indices()` / `load_colors()` | 已完成 | 将场景数据装入光栅化器 |
| `computeBarycentric2D()` | 已完成 | 计算屏幕空间重心坐标 |
| `draw()` | 已完成 | 执行 MVP、齐次除法、视口变换并提交三角形 |
| `clear()`、`set_pixel()`、矩阵 setter | 已完成 | 管理帧缓冲、深度缓冲和变换状态 |
| `Triangle` 类 | 已完成 | 保存三角形顶点、颜色、纹理坐标和法线 |
| `insideTriangle()` | **待完成** | 判断采样点是否位于三角形内 |
| `rasterize_triangle()` | **待完成** | 包围盒遍历、深度测试和像素写入 |

`get_projection_matrix()` 上方仍保留了课程框架的 TODO 注释，但函数体现在已经有
完整实现，因此实际状态是“已完成”。

## 2. 一帧图像的完整调用链

```text
main()
├─ 准备顶点、索引和颜色
├─ load_positions() / load_indices() / load_colors()
└─ 绘制一帧
   ├─ clear(Color | Depth)
   ├─ set_model() / set_view() / set_projection()
   ├─ draw()
   │  ├─ 读取顶点、索引和颜色缓冲
   │  ├─ 计算 MVP = Projection × View × Model
   │  ├─ 变换每个顶点
   │  ├─ 齐次除法
   │  ├─ 视口变换
   │  ├─ 构造临时 Triangle
   │  └─ rasterize_triangle()                 ← 待完成
   │     ├─ insideTriangle()                  ← 待完成
   │     ├─ computeBarycentric2D()            ← 已提供
   │     ├─ 深度测试与 depth_buf 更新
   │     └─ set_pixel()
   └─ frame_buffer() → OpenCV 显示或保存 PNG
```

最重要的分界是：`draw()` 处理“顶点如何来到屏幕空间”，而
`rasterize_triangle()` 处理“屏幕空间三角形覆盖了哪些像素”。

## 3. `main.cpp`：准备场景并组织渲染

### 3.1 `get_view_matrix()`：观察变换

```cpp
Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos)
```

参数 `eye_pos` 表示相机在世界空间中的位置。当前函数返回一个平移矩阵，把整个
世界平移 `-eye_pos`：

```text
相机位于 (0, 0, 5)
等价于把场景移动 (0, 0, -5)
```

本作业的相机没有旋转，所以只需要平移。它的使用方式是：

```cpp
Eigen::Vector3f eye_pos{0, 0, 5};
r.set_view(get_view_matrix(eye_pos));
```

`set_view()` 只是保存矩阵；真正的矩阵乘法发生在之后的 `draw()` 中。

### 3.2 `get_model_matrix()`：模型变换

```cpp
Eigen::Matrix4f get_model_matrix(float rotation_angle)
```

它返回单位矩阵，因此顶点不会产生额外平移、旋转或缩放。参数
`rotation_angle` 在本作业中故意没有使用，因为作业说明明确要求模型矩阵保持
单位矩阵。

```cpp
r.set_model(get_model_matrix(0));
```

不要因为参数名中有 `rotation_angle` 就认为这里仍然缺少旋转代码。

### 3.3 `get_projection_matrix()`：透视投影

```cpp
Eigen::Matrix4f get_projection_matrix(
    float eye_fov,
    float aspect_ratio,
    float zNear,
    float zFar);
```

四个参数分别是：

- `eye_fov`：垂直视场角，调用时使用角度制；
- `aspect_ratio`：宽高比；
- `zNear`：近平面到相机的正距离；
- `zFar`：远平面到相机的正距离。

当前实现分三步构造投影：

1. 将透视视锥体压缩成正交长方体；
2. 将长方体中心平移到原点；
3. 将长方体缩放到规范化设备坐标范围。

返回矩阵的乘法顺序是：

```cpp
ortho_scale * ortho_translation * perspective_to_ortho
```

Eigen 使用列向量，因此顶点实际经历变换的顺序是从右向左。调用示例：

```cpp
r.set_projection(get_projection_matrix(45, 1.0f, 0.1f, 50.0f));
```

这里的 `zNear`、`zFar` 以正距离传入，函数内部再根据相机看向负 z 轴的约定转换
为负坐标。调用者不要提前传入负数，否则会把符号处理两次。

### 3.4 `main()`：两种运行模式

```cpp
int main(int argc, const char** argv)
```

程序根据参数数量选择模式。

#### 无参数：窗口模式

```powershell
./build-ucrt/Rasterizer.exe
```

程序持续绘制并通过 OpenCV 显示窗口，按 `Esc` 退出。每一帧都会清空缓冲、设置
三个矩阵并重新绘制；`frame_count` 只用于输出已经绘制的帧数。

#### 一个参数：保存图片模式

```powershell
./build-ucrt/Rasterizer.exe output.png
```

唯一参数会被当作输出文件名。程序只渲染一帧，调用 `cv::imwrite()` 保存图片后
退出。这是做结果验证和提交截图时更稳定的模式。

### 3.5 场景数据

`pos` 保存六个三维顶点，`ind` 用两组三元索引将它们组成两个三角形，`cols`
保存每个顶点的 RGB 颜色：

```cpp
auto pos_id = r.load_positions(pos);
auto ind_id = r.load_indices(ind);
auto col_id = r.load_colors(cols);
```

顶点数组和索引数组分开后，不同三角形便可以共享顶点。这里两个三角形没有共享
顶点，但框架接口采用了图形 API 中常见的组织方式。

### 3.6 OpenCV 如何使用帧缓冲

```cpp
cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
image.convertTo(image, CV_8UC3, 1.0f);
cv::cvtColor(image, image, cv::COLOR_RGB2BGR);
```

- `frame_buffer()` 中每个元素是三个浮点颜色分量；
- `cv::Mat` 直接引用这段连续内存，不先复制整张图；
- `convertTo()` 将浮点颜色转换为 8 位颜色；
- 框架内部使用 RGB，而 OpenCV 保存/显示通常使用 BGR，所以需要交换通道。

## 4. `rasterizer.hpp`：光栅化器的接口和状态

### 4.1 `Buffers` 与位标志

```cpp
enum class Buffers
{
    Color = 1,
    Depth = 2
};
```

`Buffers` 表示要操作的缓冲区。框架重载了 `operator|` 和 `operator&`，所以可以
组合多个标志：

```cpp
r.clear(rst::Buffers::Color | rst::Buffers::Depth);
```

`Color | Depth` 得到组合标志；`clear()` 内部再用 `&` 判断其中是否包含某一项。

### 4.2 `Primitive`：图元类型

```cpp
enum class Primitive
{
    Line,
    Triangle
};
```

调用 `draw()` 时传入 `Primitive::Triangle`。当前 Assignment 2 的 `draw()` 没有
根据 `type` 分支处理，参数主要是为统一接口和后续扩展保留的；本作业不要传入
`Line`。

### 4.3 三类缓冲编号

```cpp
struct pos_buf_id { int pos_id = 0; };
struct ind_buf_id { int ind_id = 0; };
struct col_buf_id { int col_id = 0; };
```

三种结构内部都只有一个整数，但它们是不同的 C++ 类型。这可以阻止下面这种
参数顺序错误：

```cpp
r.draw(ind_id, pos_id, col_id, rst::Primitive::Triangle); // 编译失败
```

正确方式是保存并使用各个 `load_*()` 的返回值，不要自己猜测或构造编号。

### 4.4 `rasterizer()`：创建缓冲区

```cpp
rst::rasterizer::rasterizer(int w, int h)
```

构造函数保存画布宽高，并将颜色缓冲和深度缓冲都调整为 `w * h` 个元素：

```cpp
rst::rasterizer r(700, 700);
```

构造函数负责分配空间，但每一帧仍应调用 `clear()` 设置明确的初始值。

### 4.5 `get_next_id()`：生成资源编号

```cpp
int get_next_id() { return next_id++; }
```

每次加载一份数据时返回当前编号，然后让编号递增。位置、索引和颜色缓冲共用
同一个编号生成器，但返回值会被包装成不同类型。

### 4.6 `frame_buffer()`：取得最终像素

```cpp
std::vector<Eigen::Vector3f>& frame_buffer()
```

它返回颜色缓冲的引用，主要供 OpenCV 使用：

```cpp
auto& pixels = r.frame_buffer();
cv::Mat image(height, width, CV_32FC3, pixels.data());
```

因为返回的是引用，所以不会复制 700×700 个像素；但调用者也可以修改内部数据，
并且不能在 `rasterizer` 被销毁后继续使用这个引用。

## 5. `rasterizer.cpp`：从三维顶点到屏幕三角形

### 5.1 `load_positions()`

```cpp
rst::pos_buf_id load_positions(
    const std::vector<Eigen::Vector3f>& positions);
```

函数取得一个新编号，把整份顶点数组复制进 `pos_buf`，再返回位置缓冲编号。

### 5.2 `load_indices()`

```cpp
rst::ind_buf_id load_indices(
    const std::vector<Eigen::Vector3i>& indices);
```

每个 `Vector3i` 保存组成一个三角形的三个顶点下标。函数将索引数组保存到
`ind_buf` 并返回索引缓冲编号。

### 5.3 `load_colors()`

```cpp
rst::col_buf_id load_colors(
    const std::vector<Eigen::Vector3f>& colors);
```

它保存逐顶点 RGB 颜色并返回颜色缓冲编号。输入颜色使用 `[0, 255]` 范围，稍后
交给 `Triangle::setColor()` 归一化。

三个加载函数都复制传入的 `vector`，所以调用完成后，原始数组即使离开作用域，
光栅化器内部的数据仍然有效。

### 5.4 `to_vec4()`：转换到齐次坐标

```cpp
auto to_vec4(const Eigen::Vector3f& v3, float w = 1.0f)
```

它把 `(x, y, z)` 转换为 `(x, y, z, w)`。顶点位置使用 `w = 1`，这样模型和观察
矩阵中的平移会对它生效：

```cpp
Eigen::Vector4f position = to_vec4(vertex, 1.0f);
```

方向向量通常使用 `w = 0`，但当前函数只用于顶点。

### 5.5 `computeBarycentric2D()`：重心坐标

```cpp
static std::tuple<float, float, float> computeBarycentric2D(
    float x,
    float y,
    const Vector3f* v);
```

函数返回采样点 `(x, y)` 相对于三角形三个顶点的重心坐标：

```text
(alpha, beta, gamma)
```

它们满足：

```text
P = alpha × V0 + beta × V1 + gamma × V2
alpha + beta + gamma = 1
```

在本作业中，它主要用于根据三个顶点的 z 值插值出当前像素的深度。典型调用形式
已经作为注释放在 `rasterize_triangle()` 中：

```cpp
auto [alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);
```

应当只对有效、非退化三角形使用该函数；如果三个顶点共线，公式分母可能为零。

### 5.6 `draw()`：已完成的顶点处理阶段

```cpp
void rst::rasterizer::draw(
    pos_buf_id pos_buffer,
    ind_buf_id ind_buffer,
    col_buf_id col_buffer,
    Primitive type);
```

这是框架中最重要的已完成函数。

#### 第一步：取得资源

```cpp
auto& buf = pos_buf[pos_buffer.pos_id];
auto& ind = ind_buf[ind_buffer.ind_id];
auto& col = col_buf[col_buffer.col_id];
```

因此必须传入对应 `load_*()` 返回的合法编号。`std::map::operator[]` 遇到不存在的
编号会创建空数组，而不是自动报告错误，所以不要手工构造 ID。

#### 第二步：组合 MVP

```cpp
Eigen::Matrix4f mvp = projection * view * model;
```

顶点实际经历的顺序是：

```text
Model → View → Projection
```

#### 第三步：按索引处理每个三角形

`i[0]`、`i[1]`、`i[2]` 指向三角形的三个顶点。每个顶点先转换为齐次坐标，
再左乘 MVP 矩阵。

#### 第四步：齐次除法

```cpp
vec /= vec.w();
```

透视投影后的坐标除以 `w`，进入规范化设备坐标（NDC）。这一步使远处物体在屏幕
上看起来更小。

#### 第五步：视口变换

```cpp
vert.x() = 0.5 * width  * (vert.x() + 1.0);
vert.y() = 0.5 * height * (vert.y() + 1.0);
```

它把 NDC 的 `[-1, 1]` 映射到屏幕坐标。z 坐标通过 `f1`、`f2` 映射到本作业的
正深度范围；深度越小，表示越靠近相机。

#### 第六步：构造 `Triangle`

变换后的屏幕坐标和顶点颜色被写入临时 `Triangle t`，然后传给：

```cpp
rasterize_triangle(t);
```

当前 `setVertex()` 在循环中被连续执行了三次，三次参数完全相同，因此只是冗余
赋值，不会把一个顶点错误写到三个位置，也不影响结果。

### 5.7 三个矩阵 setter

```cpp
void set_model(const Eigen::Matrix4f& m);
void set_view(const Eigen::Matrix4f& v);
void set_projection(const Eigen::Matrix4f& p);
```

它们分别复制一份矩阵到光栅化器内部，本身不执行绘制。必须在调用 `draw()` 前
设置三个矩阵：

```cpp
r.set_model(model);
r.set_view(view);
r.set_projection(projection);
r.draw(pos_id, ind_id, col_id, rst::Primitive::Triangle);
```

### 5.8 `clear()`：开始新的一帧

```cpp
void rst::rasterizer::clear(rst::Buffers buff)
```

- 清除 `Color`：把每个像素设置为黑色 `(0, 0, 0)`；
- 清除 `Depth`：把每个深度设置为正无穷。

正无穷表示该像素尚未被任何物体占据。因此第一个有效片元的有限深度一定能通过
深度测试。每帧通常都应这样调用：

```cpp
r.clear(rst::Buffers::Color | rst::Buffers::Depth);
```

如果不清除颜色，上一帧图像会残留；如果不清除深度，上一帧的物体可能错误遮挡
新一帧。

### 5.9 `get_index()`：二维坐标转一维下标

```cpp
int rst::rasterizer::get_index(int x, int y)
```

帧缓冲和深度缓冲都是一维 `vector`，因此需要把 `(x, y)` 转成数组下标：

```cpp
(height - 1 - y) * width + x
```

其中 `height - 1 - y` 翻转了 y 轴：数学屏幕坐标通常向上增长，而图像内存的行
通常从上到下排列。实现深度测试时，颜色和深度必须使用完全相同的下标转换。

### 5.10 `set_pixel()`：写入颜色缓冲

```cpp
void rst::rasterizer::set_pixel(
    const Eigen::Vector3f& point,
    const Eigen::Vector3f& color);
```

`point.x()` 和 `point.y()` 被当作像素坐标，`color` 是要写入的 RGB 颜色。典型
使用方式是：

```cpp
r.set_pixel(Eigen::Vector3f{x, y, z}, color);
```

z 分量不会参与颜色缓冲下标计算。函数自身没有检查 x、y 是否越界，所以
`rasterize_triangle()` 计算包围盒时必须将遍历范围限制在画布内部，否则可能
访问 `frame_buf` 范围之外。

## 6. `Triangle`：一个待光栅化三角形的数据

### 6.1 成员数据

```cpp
Vector3f v[3];
Vector3f color[3];
Vector2f tex_coords[3];
Vector3f normal[3];
```

- `v`：三个顶点位置；在 `draw()` 之后保存的是屏幕空间坐标；
- `color`：三个顶点颜色，内部范围为 `[0, 1]`；
- `tex_coords`：纹理坐标，Assignment 2 不使用；
- `normal`：法线，Assignment 2 不使用。

### 6.2 `Triangle()`：初始化

构造函数把顶点、颜色和纹理坐标初始化为零。法线数组没有初始化，但当前作业不
读取法线，因此不影响结果。

### 6.3 `setVertex()`

```cpp
void setVertex(int ind, Vector3f ver)
```

将 `ver` 写入第 `ind` 个顶点。`ind` 应为 0、1 或 2；函数不检查越界。

### 6.4 `setColor()` 与 `getColor()`

```cpp
void setColor(int ind, float r, float g, float b);
Vector3f getColor() const;
```

`setColor()` 接收 `[0, 255]` 的 RGB 值，检查范围后除以 255，保存为 `[0, 1]`。
如果输入越界，程序会打印错误并退出。

`getColor()` 只读取第 0 个顶点颜色，再乘以 255 返回。本作业的每个三角形三个
顶点本来就是相同颜色，所以这相当于取得整个三角形的固定颜色。它并没有执行
逐顶点颜色插值。

### 6.5 `setNormal()` 和 `setTexCoord()`

这两个函数分别保存法线和纹理坐标，是为后续着色与纹理作业准备的。在
Assignment 2 中可以了解接口，但不需要调用或修改。

### 6.6 `toVector4()`

```cpp
std::array<Vector4f, 3> toVector4() const
```

它将三个 `Vector3f` 顶点转换为 `w = 1` 的 `Vector4f` 数组。
`rasterize_triangle()` 中已经调用它，以便使用课程框架给出的深度插值代码。

## 7. 最小使用范式

忽略 OpenCV 的显示和保存部分，光栅化器的标准使用顺序如下：

```cpp
rst::rasterizer r(width, height);

auto pos_id = r.load_positions(positions);
auto ind_id = r.load_indices(indices);
auto col_id = r.load_colors(colors);

r.clear(rst::Buffers::Color | rst::Buffers::Depth);
r.set_model(model);
r.set_view(view);
r.set_projection(projection);
r.draw(pos_id, ind_id, col_id, rst::Primitive::Triangle);

auto& pixels = r.frame_buffer();
```

需要特别注意顺序：

1. 创建光栅化器；
2. 加载顶点、索引和颜色；
3. 每帧清空颜色与深度；
4. 设置 Model、View、Projection；
5. 调用 `draw()`；
6. 最后读取 `frame_buffer()`。

只调用 `set_*()` 不会产生任何像素；真正启动管线的是 `draw()`。

## 8. 构建与运行

在 `Assignment2_code` 目录中执行：

```powershell
C:/msys64/ucrt64/bin/cmake.exe --preset ucrt-debug --fresh
C:/msys64/ucrt64/bin/cmake.exe --build --preset ucrt-debug
```

保存一张结果图片时，请在 MSYS2 UCRT64 终端中运行，以确保 OpenCV 等动态库能
从同一套 UCRT64 环境中找到：

```bash
./build-ucrt/Rasterizer.exe output.png
```

也可以在 VS Code 中选择 Assignment 2 工程，构建后使用
“Assignment2：保存结果图片”调试配置。

在两个 TODO 完成之前，程序虽然能够编译和运行，但生成黑色图像是正常现象，
因为 `draw()` 最后调用的 `rasterize_triangle()` 尚未向帧缓冲写入像素。

## 9. 不需要重点研究的内容

- `global.hpp` 当前没有实际定义，只是保留的公共头文件；
- `draw_line()` 是线框版本遗留的私有声明，Assignment 2 当前没有调用它；
- OpenCV 的内部图像编码过程不是本次作业重点；
- 法线、纹理坐标和逐顶点颜色插值将在后续作业中使用。

## 10. 建议阅读顺序

1. 先读 `main()`，看清一帧的调用顺序；
2. 再读 `draw()`，理解 MVP、齐次除法和视口变换；
3. 阅读 `clear()`、`get_index()` 和 `set_pixel()`，理解两类缓冲；
4. 阅读 `Triangle`，确认传给光栅化阶段的数据格式；
5. 最后进入 `rasterize_triangle()`，只补作业要求的像素覆盖与深度测试。

完成 TODO 前，可以用下面的问题自测：

- 为什么每帧必须把深度缓冲清成正无穷？
- 为什么 MVP 的代码写成 `P * V * M`，实际顺序却是 M、V、P？
- 齐次除法发生在视口变换之前有什么意义？
- `get_index()` 为什么需要翻转 y 轴？
- 为什么包围盒必须限制在 `[0, width - 1] × [0, height - 1]`？
- 两个三角形覆盖同一像素时，应该比较并更新哪一份数据？

如果能不看源码回答这些问题，就已经理解了 Assignment 2 框架中与剩余实现最
相关的部分。
