# Assignment 1 已有代码导读

本文解释 Assignment 1 代码框架中**已经实现的函数**。目标不是记住每行代码，
而是弄清楚数据怎样从三维顶点一路流向屏幕像素，以及待完成的模型矩阵、投影
矩阵会在哪里被使用。

本文不提供 `get_model_matrix()` 和 `get_projection_matrix()` 的答案。这两个函数
仍然是本次作业需要自己完成的核心内容。

## 1. 先看完整执行流程

程序的主要调用关系如下：

```text
main()
├─ 创建 rasterizer 和三角形数据
├─ load_positions() / load_indices()
└─ 每一帧
   ├─ clear()
   ├─ set_model() / set_view() / set_projection()
   ├─ draw()
   │  ├─ 组合 MVP 矩阵
   │  ├─ 变换三个顶点
   │  ├─ 齐次除法
   │  ├─ 视口变换
   │  └─ rasterize_wireframe()
   │     └─ draw_line()
   │        └─ set_pixel()
   └─ frame_buffer() → OpenCV 显示或保存图片
```

一句话概括：`main()` 准备数据和变换矩阵，`rasterizer` 把变换后的三角形画进
帧缓冲，OpenCV 再把帧缓冲显示或保存下来。

---

## 2. `main.cpp`：组织整个程序

源码位置：[code/main.cpp](code/main.cpp)

### 2.1 `get_view_matrix()`：把世界移动到相机坐标系

```cpp
Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos)
```

参数 `eye_pos` 是相机在世界空间中的位置，返回值是 4×4 视图变换矩阵。

当前函数构造了一个平移矩阵：

```text
物体移动量 = -相机位置
```

例如相机位于 `(0, 0, 5)`，就把整个世界平移 `(0, 0, -5)`。可以把它理解为：
相机不动，把场景向相反方向移动，最终得到相机眼中的坐标。

使用位置：

```cpp
Eigen::Vector3f eye_pos = {0, 0, 5};
r.set_view(get_view_matrix(eye_pos));
```

当前作业中的相机没有旋转，所以视图矩阵只包含平移。如果相机还具有朝向，完整
的视图变换还需要旋转，但这不属于本次作业要求。

### 2.2 `main()`：创建场景、运行渲染循环

```cpp
int main(int argc, const char** argv)
```

`argc` 表示命令行参数数量，`argv` 保存每个参数的字符串。

`main()` 依次完成以下工作：

1. 读取旋转角度和输出文件名。
2. 创建一个 `700 × 700` 的光栅化器。
3. 设置相机位置。
4. 定义三个顶点和一个三角形的索引。
5. 把顶点、索引装入光栅化器。
6. 设置 Model、View、Projection 三个矩阵。
7. 调用 `draw()` 绘制三角形。
8. 用 OpenCV 显示窗口或保存 PNG。

三角形的顶点是：

```cpp
{2, 0, -2}, {0, 2, -2}, {-2, 0, -2}
```

索引 `{0, 1, 2}` 表示使用第 0、1、2 个顶点组成一个三角形。把顶点数据和索引
数据分开，是图形 API 中很常见的组织方式：多个三角形可以共享同一个顶点。

#### 交互模式

不传参数时进入循环：

```bash
./build-ucrt/Rasterizer.exe
```

- `A`：角度增加 10 度。
- `D`：角度减少 10 度。
- `Esc`：退出程序。

循环每次都会清空缓冲区、根据当前角度重新计算矩阵并绘制一帧。

#### 命令行输出模式

代码希望支持下面两种形式：

```bash
./build-ucrt/Rasterizer.exe -r 20
./build-ucrt/Rasterizer.exe -r 20 image.png
```

第二条命令中：

- `argv[0]` 是程序路径；
- `argv[1]` 是 `-r`；
- `argv[2]` 是字符串 `"20"`，通过 `std::stof()` 转为浮点角度；
- `argv[3]` 是输出文件名。

需要注意：当前参数判断中，只有恰好提供 4 个参数时才会继续渲染。只提供
`-r 20` 时，程序会在 `else return 0;` 处提前退出，并不会生成注释所说的
`output.png`。这是现有框架行为与注释不一致的地方，不是矩阵造成的问题。

### 2.3 OpenCV 部分的作用

```cpp
cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
image.convertTo(image, CV_8UC3, 1.0f);
```

第一行让 `cv::Mat` 使用帧缓冲中的连续内存；`CV_32FC3` 表示每个像素由三个
32 位浮点数构成。第二行把它转换成常见的三个 8 位通道，以便显示或写入图片。

```cpp
cv::imshow("image", image); // 在窗口中显示
cv::imwrite(filename, image); // 保存到文件
cv::waitKey(10); // 等待 10 ms 并读取按键
```

---

## 3. `rasterizer.hpp`：光栅化器的接口和数据

源码位置：[code/rasterizer.hpp](code/rasterizer.hpp)

### 3.1 `Buffers` 与位运算符

```cpp
enum class Buffers
{
    Color = 1,
    Depth = 2
};
```

`Buffers` 表示可以清空的缓冲区。框架为它实现了 `operator|` 和 `operator&`：

- `Color | Depth`：组合多个标记；
- `buff & Color`：判断组合中是否包含颜色缓冲标记。

所以可以在一次调用中同时清空两类缓冲区：

```cpp
r.clear(rst::Buffers::Color | rst::Buffers::Depth);
```

这种用整数不同二进制位表示多个开关的做法通常称为位标志或 bit flags。

### 3.2 `Primitive`：指定图元类型

```cpp
enum class Primitive
{
    Line,
    Triangle
};
```

它告诉 `draw()` 要绘制哪一种基本图元。Assignment 1 的 `draw()` 只接受
`Triangle`；传入其他类型会抛出异常。

### 3.3 `pos_buf_id` 与 `ind_buf_id`：带类型的缓冲区编号

```cpp
struct pos_buf_id { int pos_id = 0; };
struct ind_buf_id { int ind_id = 0; };
```

这两个结构体内部都只是整数，但它们属于不同类型。因此下面这样的参数顺序错误
可以被编译器发现：

```cpp
r.draw(ind_id, pos_id, rst::Primitive::Triangle); // 类型不匹配
```

比起所有编号都直接使用 `int`，这种设计提供了额外的类型安全。

### 3.4 `frame_buffer()`：把渲染结果交给调用者

```cpp
std::vector<Eigen::Vector3f>& frame_buffer();
```

它返回颜色帧缓冲的引用。每个 `Vector3f` 表示一个像素的三个颜色分量。
`main()` 通过 `.data()` 得到连续内存地址，再交给 OpenCV。

返回的是引用，不会复制整个 700×700 的缓冲区。不过调用者也因此能够修改内部
数据，并且不能在 `rasterizer` 对象销毁后继续使用这段引用。

### 3.5 `get_next_id()`：生成新的资源编号

```cpp
int get_next_id() { return next_id++; }
```

它返回当前编号，再将 `next_id` 加一。加载每一份顶点或索引数据时都会得到一个
不同编号，之后可以通过编号从 `std::map` 中取回数据。

---

## 4. `rasterizer.cpp`：从顶点变成屏幕线条

源码位置：[code/rasterizer.cpp](code/rasterizer.cpp)

### 4.1 构造函数 `rasterizer()`

```cpp
rst::rasterizer::rasterizer(int w, int h)
```

它记录画布宽高，并把颜色缓冲与深度缓冲调整为 `w * h` 个元素。

```cpp
rst::rasterizer r(700, 700);
```

上面的调用会准备 490000 个颜色元素和 490000 个深度元素。构造函数只分配空间，
初始值会在每一帧开始时由 `clear()` 设置。

### 4.2 `load_positions()`：加载顶点位置

```cpp
rst::pos_buf_id load_positions(
    const std::vector<Eigen::Vector3f>& positions);
```

它做三件事：

1. 取得一个新编号；
2. 把顶点数组复制到内部的 `pos_buf`；
3. 返回带类型的 `pos_buf_id`。

使用示例：

```cpp
std::vector<Eigen::Vector3f> pos{
    {2, 0, -2}, {0, 2, -2}, {-2, 0, -2}
};
auto pos_id = r.load_positions(pos);
```

### 4.3 `load_indices()`：加载三角形索引

```cpp
rst::ind_buf_id load_indices(
    const std::vector<Eigen::Vector3i>& indices);
```

它的流程与 `load_positions()` 相同，只不过保存的是索引。每个 `Vector3i` 中的
三个整数分别指出一个三角形使用哪三个顶点。

```cpp
std::vector<Eigen::Vector3i> ind{{0, 1, 2}};
auto ind_id = r.load_indices(ind);
```

### 4.4 `set_model()`、`set_view()`、`set_projection()`

```cpp
void set_model(const Eigen::Matrix4f& m);
void set_view(const Eigen::Matrix4f& v);
void set_projection(const Eigen::Matrix4f& p);
```

这三个函数分别把矩阵复制到光栅化器内部。它们本身不做矩阵计算，也不会立即
绘制；真正使用矩阵的是随后调用的 `draw()`。

典型顺序是：

```cpp
r.set_model(get_model_matrix(angle));
r.set_view(get_view_matrix(eye_pos));
r.set_projection(get_projection_matrix(45, 1, 0.1, 50));
r.draw(pos_id, ind_id, rst::Primitive::Triangle);
```

### 4.5 `clear()`：开始新的一帧

```cpp
void rst::rasterizer::clear(rst::Buffers buff)
```

- 清除颜色时，把所有像素设为 `(0, 0, 0)`，即黑色；
- 清除深度时，把所有深度设为正无穷，表示尚未有物体占据该像素。

如果不清空颜色缓冲，上一帧的线条会残留；如果后续作业不清空深度缓冲，上一帧
的深度也可能错误地遮挡新画面。

Assignment 1 目前只画线框，没有执行深度测试，因此深度缓冲在这里已经分配和
清空，但还没有真正参与遮挡判断。

### 4.6 `to_vec4()`：把三维点变成齐次坐标

```cpp
auto to_vec4(const Eigen::Vector3f& v3, float w = 1.0f)
```

它把 `(x, y, z)` 转换成 `(x, y, z, w)`。默认 `w = 1`，表示输入是一个位置点，
可以受到矩阵中的平移影响。

```cpp
mvp * to_vec4(vertex, 1.0f)
```

如果表示方向向量，齐次坐标通常使用 `w = 0`，这样平移不会影响方向。不过当前
函数只被用于三角形顶点。

### 4.7 `draw()`：整个框架最重要的已完成函数

```cpp
void rst::rasterizer::draw(
    rst::pos_buf_id pos_buffer,
    rst::ind_buf_id ind_buffer,
    rst::Primitive type);
```

`draw()` 的输入不是原始数组，而是之前加载数据时返回的两个编号。

#### 第一步：检查图元类型并取出数据

函数拒绝除三角形以外的图元，然后从内部 `map` 中取得顶点数组和索引数组。

#### 第二步：组合 MVP 矩阵

```cpp
Eigen::Matrix4f mvp = projection * view * model;
```

Eigen 在这里使用列向量，顶点写在矩阵右边：

```text
最终顶点 = Projection × View × Model × 原始顶点
```

虽然代码从左向右写成 `P × V × M`，真正作用在顶点上的先后顺序是从右向左：
先模型变换，再视图变换，最后投影变换。

#### 第三步：变换三个顶点

索引 `i[0]`、`i[1]`、`i[2]` 找到组成当前三角形的三个顶点。每个顶点先转换为
四维齐次坐标，再左乘 MVP 矩阵。

#### 第四步：齐次除法

```cpp
vec /= vec.w();
```

投影后的 `x`、`y`、`z` 仍处于齐次坐标。除以 `w` 后，它们进入规范化设备坐标
（NDC），理论范围通常为 `[-1, 1]`。这一步正是透视近大远小最终生效的地方。

#### 第五步：视口变换

```cpp
vert.x() = 0.5 * width  * (vert.x() + 1.0);
vert.y() = 0.5 * height * (vert.y() + 1.0);
```

它把 NDC 中的 `[-1, 1]` 映射到屏幕尺寸 `[0, width]` 和 `[0, height]`。
`z` 也被映射到深度范围，但 Assignment 1 尚未使用深度测试。

#### 第六步：生成临时三角形并绘制线框

变换后的三个顶点被写入一个临时 `Triangle` 对象，然后调用
`rasterize_wireframe()` 绘制三条边。

当前代码在循环中连续三次执行了完全相同的 `setVertex(i, ...)`。这三次赋值结果
相同，所以只是冗余，不会让顶点被设置到三个不同位置。

### 4.8 `rasterize_wireframe()`：把三角形拆成三条边

```cpp
void rst::rasterizer::rasterize_wireframe(const Triangle& t)
```

它分别连接：

- `c → a`
- `c → b`
- `b → a`

三条线刚好形成三角形边界。该函数只画边，不填充三角形内部。

### 4.9 `draw_line()`：Bresenham 直线算法

```cpp
void rst::rasterizer::draw_line(
    Eigen::Vector3f begin,
    Eigen::Vector3f end);
```

这个函数使用 Bresenham 算法选择最接近理想直线的整数像素。算法根据斜率将情况
分成两类：

- `|dy| <= |dx|`：主要沿 x 方向前进；
- `|dy| > |dx|`：主要沿 y 方向前进。

变量 `px`、`py` 是误差判定量，用整数加减决定什么时候让另一个坐标也移动一格。
它最终不断调用 `set_pixel()`，将经过的像素设为白色。

本次作业不要求实现或修改 Bresenham 算法。现阶段知道它的输入是两个屏幕坐标、
输出是一串近似直线的像素即可。

### 4.10 `set_pixel()`：写入一个屏幕像素

```cpp
void rst::rasterizer::set_pixel(
    const Eigen::Vector3f& point,
    const Eigen::Vector3f& color);
```

函数先检查 `(x, y)` 是否处于画布范围，然后把二维坐标转换为一维数组下标，最后
将颜色写入 `frame_buf`。

数组是一维的，所以理论上的二维像素 `(x, y)` 需要转换为类似下面的形式：

```text
index = row * width + column
```

代码使用 `height - y` 翻转 y 轴，因为数学坐标通常向上为正，而图像数组通常从
顶部向下排列。

需要注意：当前下标公式是 `(height - y) * width + x`。当 `y == 0` 时可能刚好
越过数组末尾；更严格的翻转通常会考虑 `height - 1 - y`。正常示例不一定触发
这个边界问题，但调试靠近屏幕底边的图形时应当知道它的存在。

### 4.11 `get_index()`：计算帧缓冲下标

```cpp
int rst::rasterizer::get_index(int x, int y)
```

它封装了二维坐标到一维数组下标的计算。不过当前 `set_pixel()` 又直接写了一遍
相同公式，因此 `get_index()` 在 Assignment 1 的实际绘制路径中没有被调用。

---

## 5. `Triangle`：保存一个三角形的数据

源码位置：[code/Triangle.hpp](code/Triangle.hpp) 和
[code/Triangle.cpp](code/Triangle.cpp)

### 5.1 `Triangle()`：初始化三角形

构造函数把三个顶点位置、三个顶点颜色和三个纹理坐标初始化为零。

法线数组 `normal` 没有在这个构造函数中初始化，但 Assignment 1 不使用法线，
因此不会影响当前线框渲染。

### 5.2 `a()`、`b()`、`c()`：读取三个顶点

```cpp
Eigen::Vector3f a() const;
Eigen::Vector3f b() const;
Eigen::Vector3f c() const;
```

它们分别返回 `v[0]`、`v[1]`、`v[2]`，让线框函数能用更直观的名字访问三角形
三个顶点。

### 5.3 `setVertex()`：设置顶点位置

```cpp
void Triangle::setVertex(int ind, Eigen::Vector3f ver)
```

`ind` 应为 0、1 或 2，`ver` 是要写入的位置。函数没有检查索引范围，因此调用者
必须保证 `ind` 合法。

### 5.4 `setNormal()`：设置顶点法线

```cpp
void Triangle::setNormal(int ind, Vector3f n)
```

它保存第 `ind` 个顶点的法线。法线会在后续光照计算中使用，Assignment 1 暂时
不需要它。

### 5.5 `setColor()`：设置并归一化顶点颜色

```cpp
void Triangle::setColor(int ind, float r, float g, float b)
```

输入颜色范围是 `[0, 255]`。函数先检查三个分量是否合法，再除以 255，将颜色
保存为 `[0, 1]` 范围；输入越界时会抛出 `std::runtime_error`。

当前 `draw()` 虽然给三个顶点设置了红、绿、蓝颜色，但 `draw_line()` 使用的是
固定白色，并没有读取顶点颜色。因此 Assignment 1 看到的是白色线框，而不是
三条彩色边。

### 5.6 `setTexCoord()`：设置纹理坐标

```cpp
void Triangle::setTexCoord(int ind, float s, float t)
```

它保存某个顶点的二维纹理坐标 `(s, t)`。纹理坐标描述图片上的采样位置，将在
后续纹理映射作业中发挥作用，Assignment 1 暂时不使用。

### 5.7 `toVector4()`：批量转换三个顶点

```cpp
std::array<Vector4f, 3> Triangle::toVector4() const
```

它把三个 `Vector3f` 顶点转换成 `w = 1` 的 `Vector4f`，并返回固定长度为 3 的
数组。

当前 `rasterizer::draw()` 使用的是独立辅助函数 `to_vec4()`，没有调用这个成员
函数。它仍然展示了如何用 `std::transform` 对数组中的每个元素执行同一种转换。

---

## 6. 最小使用范式

忽略窗口和文件输出，一个最小的调用顺序可以概括为：

```cpp
rst::rasterizer r(width, height);

auto pos_id = r.load_positions(positions);
auto ind_id = r.load_indices(indices);

r.clear(rst::Buffers::Color | rst::Buffers::Depth);
r.set_model(model);
r.set_view(view);
r.set_projection(projection);
r.draw(pos_id, ind_id, rst::Primitive::Triangle);

auto& pixels = r.frame_buffer();
```

必须注意顺序：

1. 先创建光栅化器；
2. 再加载几何数据；
3. 每帧先清空缓冲；
4. 设置三种矩阵；
5. 最后调用 `draw()`；
6. 从 `frame_buffer()` 读取结果。

如果只设置了矩阵却没有调用 `draw()`，帧缓冲不会出现新的三角形。

---

## 7. 建议的阅读优先级

### 必须理解

1. `main()` 中一帧的执行顺序。
2. `set_model()`、`set_view()`、`set_projection()` 与 `draw()` 的关系。
3. `draw()` 中 `projection * view * model` 的乘法顺序。
4. 顶点为什么要从三维变成四维。
5. 齐次除法和 NDC 到屏幕坐标的转换。
6. 顶点数组和索引数组为什么分开保存。

### 了解用途即可

1. Bresenham 直线算法的每个误差变量。
2. OpenCV 内部如何管理图像内存。
3. `normal`、`tex_coords` 等后续作业才会使用的数据。
4. 深度缓冲的完整测试和更新算法。

### 完成 TODO 前可以自测的问题

- 为什么相机在 `(0, 0, 5)` 时，世界需要向 z 轴负方向移动？
- 为什么 MVP 写成 `P * V * M`，但顶点先经历 Model 变换？
- 为什么顶点需要 `w = 1`？
- 为什么投影完成后必须除以 `w`？
- NDC 中的 `x = -1` 和 `x = 1` 分别落在屏幕哪里？
- `load_positions()` 为什么返回编号，而不是每次让 `draw()` 接收整个数组？

如果能够不用看代码回答这些问题，就已经掌握了框架中与 Assignment 1 最相关的
部分。
