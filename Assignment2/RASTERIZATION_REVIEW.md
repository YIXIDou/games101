# `insideTriangle()` 与 `rasterize_triangle()` 算法复查及改进建议

复查对象：[`Assignment2_code/rasterizer.cpp`](Assignment2_code/rasterizer.cpp)
中的 `insideTriangle()` 和 `rasterize_triangle()`。

本文先判断当前算法是否正确，再列出不影响基础作业正确性、但有助于提升健壮性、
可读性和效率的改进方向。本文没有直接修改源码。

## 1. 总体结论

当前两个函数对于 Assignment 2 提供的非退化三角形都是正确的，已经满足普通三
角形栅格化和 Z-buffer 的基础要求。

验证结果：

- 工程使用 UCRT64 GCC 和 OpenCV 正常编译、链接；
- 程序正常退出并生成 700×700 结果图片；
- 两个三角形均被完整填充，没有此前的随机散点；
- 重叠部分由更靠近相机的浅绿色三角形遮挡蓝色三角形；
- `insideTriangle()` 对 5,871 个非退化三角形采样点的结果与独立参考边函数完全
  一致；
- 内部点返回 `true`，外部点返回 `false`；
- 边中点和三个顶点均被视为三角形内部；
- 顺时针和逆时针顶点顺序均能正确判断。

本次结论针对基础作业。以下内容尚未包含在当前实现中，但不属于基础算法错误：

- 退化三角形的专门处理；
- top-left 边界规则；
- 2×2 MSAA；
- 对任意视锥体参数和通用顶点属性的完整透视校正。

## 2. 两个函数之间的职责

完整的像素处理过程是：

```text
rasterize_triangle()
├─ 计算屏幕空间包围盒
├─ 遍历候选像素
├─ 排除屏幕外像素
├─ 使用像素中心调用 insideTriangle()
├─ 计算重心坐标和插值深度
├─ 将深度与 depth_buf 比较
└─ 更近时同时更新深度与颜色
```

两者的分工是：

- `insideTriangle()` 只回答“这个采样点是否被三角形覆盖”；
- `rasterize_triangle()` 决定“检查哪些采样点，以及覆盖后如何进行深度和颜色
  更新”。

因此，即使点内测试正确，错误的缓冲下标仍会制造散点；反过来，即使深度缓冲
正确，错误的点内测试也会造成三角形缺失。

## 3. `insideTriangle()` 正确性复查

### 3.1 当前算法

当前实现分别构造：

```text
AP、BP、CP
AB、BC、CA
```

随后计算：

```text
AP × AB
BP × BC
CP × CA
```

如果三个叉积的 z 分量全部非负或全部非正，就认为采样点在三角形内部。

### 3.2 为什么算法正确

每个叉积的符号表示采样点位于对应有向边的哪一侧。如果点位于三角形内部，它
必须位于三条有向边的同一侧，因此三个符号一致。

教材也经常写成：

```text
AB × AP
BC × BP
CA × CP
```

这与当前写法的符号刚好相反，但三次都反转后，同号判断结果不变。

当前代码同时判断：

```text
全部 >= 0
或
全部 <= 0
```

所以它不依赖三角形是顺时针还是逆时针排列。

### 3.3 边界行为

因为判断使用 `>= 0` 和 `<= 0`，叉积为零的点也会通过。这意味着：

- 三条边属于三角形；
- 三个顶点属于三角形；
- 两个共享边的三角形可能都认为共享边上的采样点属于自己。

前两项符合基础作业要求。共享边的唯一归属可以在以后通过 top-left rule 处理。

## 4. `insideTriangle()` 改进建议

以下内容不是当前正确性的必要修复。

### 4.1 避免双下划线变量名

当前变量名如 `__vap`、`__vbp`、`__v1` 以双下划线开头。C++ 将包含双下划线的
标识符保留给编译器和标准库实现，用户代码不应使用。

建议使用具有几何含义的名称：

```cpp
Vector3f ap, bp, cp;
Vector3f ab, bc, ca;
```

这样也能直接看出正确配对关系：`ap/ab`、`bp/bc`、`cp/ca`。

### 4.2 每个叉积只计算一次

当前长 `return` 表达式在两个逻辑分支中重复计算三个叉积。Debug 构建中可能实
际执行六次。

建议缓存结果：

```cpp
const float cross_ab = ap.cross(ab).z();
const float cross_bc = bp.cross(bc).z();
const float cross_ca = cp.cross(ca).z();

const bool all_non_negative =
    cross_ab >= 0.0f && cross_bc >= 0.0f && cross_ca >= 0.0f;
const bool all_non_positive =
    cross_ab <= 0.0f && cross_bc <= 0.0f && cross_ca <= 0.0f;

return all_non_negative || all_non_positive;
```

### 4.3 使用二维 edge function

当前方法把二维问题放入 `Vector3f`，执行三维叉积后只取 z 分量。数学上正确，
但可以直接使用二维边函数：

```cpp
static float edgeFunction(
    const Vector3f& a,
    const Vector3f& b,
    float x,
    float y)
{
    return (b.x() - a.x()) * (y - a.y()) -
           (b.y() - a.y()) * (x - a.x());
}
```

然后分别计算 `edge(AB, P)`、`edge(BC, P)`、`edge(CA, P)`。这样可以减少临时
向量，并更接近软件光栅化器常用的 edge function 表达。

### 4.4 让参数类型表达“三个顶点”

当前参数是原始指针：

```cpp
const Vector3f* _v
```

指针本身不携带数组长度。项目使用 C++17，可以改为三元素数组引用：

```cpp
static bool insideTriangle(
    float x,
    float y,
    const Vector3f (&vertices)[3]);
```

`t.v` 仍可直接传入，而且编译器能够检查参数确实是三个元素的数组。

### 4.5 检查退化三角形

三个顶点共线或重合时，三角形面积为零。点内测试可能把线段上的采样点判断为
内部，随后的重心坐标计算还会出现零分母。

课程测试三角形不是退化三角形，因此不影响当前作业。若需要增强健壮性，可以
先计算有向面积，在绝对值接近零时直接跳过三角形。

### 4.6 Top-left rule

基础实现把所有边界都算作内部。更严格的光栅化器通常采用 top-left rule，让共
享边只归属于其中一个三角形，避免共面三角形的结果依赖绘制顺序。

建议在普通栅格化和 Z-buffer 完全稳定后再研究该规则。

## 5. `rasterize_triangle()` 正确性复查

### 5.1 包围盒

当前代码分别取得三个顶点 x、y 坐标的最小值和最大值：

```text
left   = min(x0, x1, x2)
right  = max(x0, x1, x2)
bottom = min(y0, y1, y2)
top    = max(y0, y1, y2)
```

然后从 `floor(min)` 遍历到 `ceil(max)`。这个范围略微保守，可能多检查一圈必然
不在内部的像素，但不会漏掉应该覆盖的像素，因此算法正确。

### 5.2 屏幕边界

当前循环在访问缓冲之前检查：

```cpp
x + 0.5f >= 0
x + 0.5f < width
y + 0.5f >= 0
y + 0.5f < height
```

因为 x、y 是整数，这能保证后续 `get_index(x, y)` 的范围是：

```text
0 <= x < width
0 <= y < height
```

因此当前实现不会因为屏幕外三角形而直接访问负下标或超过缓冲区末尾。

### 5.3 像素中心采样

当前代码使用：

```cpp
insideTriangle(x + 0.5f, y + 0.5f, t.v)
```

并用相同的像素中心计算重心坐标。这保证覆盖测试和深度插值针对同一个采样位
置，是正确的做法。

### 5.4 深度插值

当前代码使用课程框架给出的：

```text
重心坐标
→ w 的倒数
→ 三个顶点 z 的加权和
→ 乘以 w_reciprocal
```

公式使用位置、顺序正确。对于当前框架，`Triangle::toVector4()` 中的 w 为 1，
因此表达式会退化为屏幕空间 z 的重心插值，但保留课程给出的写法有利于理解后
续透视校正插值。

### 5.5 缓冲下标

当前实现已经改为：

```cpp
const int index = get_index(x, y);
```

这与 `set_pixel()` 使用的坐标翻转方式一致，修复了之前 `x * y` 造成的下标碰撞
和随机散点。

### 5.6 深度测试

当前条件是：

```cpp
if (z_interpolated < depth_buf[index])
```

本框架中深度越小表示越靠近相机，深度缓冲每帧初始化为正无穷。因此：

- 第一个有效片元一定通过；
- 后来的近片元覆盖远片元；
- 后来的远片元被丢弃；
- 深度相等时保留先绘制的片元。

该行为正确，并且生成图中重叠区域的遮挡关系也与预期一致。

### 5.7 颜色和深度更新

通过深度测试后，代码同时更新 `depth_buf` 和颜色缓冲，因此二者保持同步。此前
通过错误下标写入颜色的问题已经消失。

## 6. `rasterize_triangle()` 改进建议

以下内容同样不是基础算法错误。

### 6.1 在进入循环前裁剪包围盒

当前先遍历完整包围盒，再对每个像素执行屏幕边界判断。三角形大部分位于屏幕外
时，会循环检查大量不可能写入的点。

可以直接把整数边界限制到屏幕范围：

```cpp
const int min_x = std::max(0, static_cast<int>(std::floor(left)));
const int max_x = std::min(width - 1,
                           static_cast<int>(std::ceil(right)));
const int min_y = std::max(0, static_cast<int>(std::floor(bottom)));
const int max_y = std::min(height - 1,
                           static_cast<int>(std::ceil(top)));
```

这样内层循环不再需要为每个像素重复做四次屏幕范围比较。

### 6.2 删除重复的颜色写入

当前通过深度测试后先执行：

```cpp
frame_buf[index] = t.getColor();
```

随后又调用：

```cpp
set_pixel(Vector3f(x, y, z_interpolated), t.getColor());
```

现在两者使用相同下标，所以不会造成错误，但颜色被写了两次。按照作业框架的提
示，可以只保留 `set_pixel()`；或者为了性能直接写 `frame_buf[index]`，但两者不
需要同时存在。

### 6.3 缓存像素中心

`x + 0.5f`、`y + 0.5f` 在边界检查、点内判断和重心坐标计算中重复出现。可以在
循环内保存：

```cpp
const float sample_x = static_cast<float>(x) + 0.5f;
const float sample_y = static_cast<float>(y) + 0.5f;
```

这样能明确所有计算使用相同采样点，并减少长表达式。

### 6.4 改进边界变量命名

`l_boundary`、`r_boundary`、`t_boundary`、`b_boundary` 能够使用，但容易在 top 和
bottom 之间来回确认。

推荐使用：

```text
min_x、max_x、min_y、max_y
```

它们与 `std::min`、`std::max` 以及循环条件能直接对应。

### 6.5 使用 initializer-list 形式的 `min/max`

当前嵌套调用：

```cpp
std::min(std::min(x0, x1), x2)
```

可以写为：

```cpp
std::min({x0, x1, x2})
std::max({x0, x1, x2})
```

表达更接近“在三个顶点中求最值”。

### 6.6 调整循环顺序提高缓存局部性

帧缓冲是一维行优先数组，同一行中相邻 x 的地址连续。当前外层遍历 x、内层遍历
y，会在内存中按整行跨度跳跃。

可以改成：

```cpp
for (int y = min_y; y <= max_y; ++y)
{
    for (int x = min_x; x <= max_x; ++x)
    {
        // ...
    }
}
```

这样相邻迭代通常访问相邻像素，对缓存更友好。700×700 的课程场景差异不会很
大，但这是常见的图像处理循环习惯。

### 6.7 缓存三角形颜色

当前每个通过深度测试的像素都会调用 `t.getColor()`。本作业每个三角形使用固定
颜色，可以在循环外保存：

```cpp
const Vector3f triangle_color = t.getColor();
```

之后直接复用。

### 6.8 使用 `const`

以下值计算完成后不会改变，可以声明为 `const`：

- `v`；
- 浮点包围盒边界；
- 整数循环边界；
- `alpha`、`beta`、`gamma`；
- `w_reciprocal`；
- `z_interpolated`；
- `index`；
- 三角形颜色。

`const` 能表达数据流，也能防止后续重构时意外修改中间结果。

### 6.9 让 `set_pixel()` 复用 `get_index()`

这不在 `rasterize_triangle()` 内部，但与其缓冲访问有关。当前 `get_index()` 和
`set_pixel()` 各自写了一遍二维坐标转下标公式。如果以后只修改其中一处，两种
缓冲可能再次失去同步。

更安全的做法是让 `set_pixel()` 内部调用 `get_index()`，使坐标映射只有一个实
现来源。

### 6.10 移除完成后的 TODO 注释

两个函数已经实现，但上方仍保留“TODO”文字。可以把它们改为解释算法意图的注
释，例如：

```cpp
// Test the pixel center against the three directed triangle edges.
// Rasterize the screen-space bounding box with per-pixel depth testing.
```

## 7. 两个函数可以共同优化的方向

### 7.1 复用 edge function 计算重心坐标

当前 `insideTriangle()` 计算三次边函数，点通过后
`computeBarycentric2D()` 又执行一组相似运算。更高效的光栅化器可以使用同一组
edge function：

```text
边函数符号 → 判断覆盖
边函数值 / 三角形面积 → 得到重心坐标
```

这样可以减少重复计算。不过课程已经提供 `computeBarycentric2D()`，基础作业保
持现有结构更容易与题目对应。

### 7.2 增量计算边函数

像素从 `(x, y)` 移动到 `(x + 1, y)` 或 `(x, y + 1)` 时，线性边函数的变化量是
固定的。成熟的软件光栅化器会在包围盒左上角计算一次，然后通过加法逐像素更
新，而不是每次重新计算乘法。

这属于性能优化，不建议在基础版本尚未完成测试时引入。

### 7.3 为 MSAA 抽象采样位置

当前每像素只有中心点：

```text
(0.5, 0.5)
```

2×2 MSAA 可以将同一套覆盖和深度逻辑应用到四个偏移位置，例如：

```text
(0.25, 0.25)
(0.75, 0.25)
(0.25, 0.75)
(0.75, 0.75)
```

如果将“测试一个样本并更新样本深度/颜色”整理成独立步骤，后续从一个样本扩展
为四个样本会更清晰。

## 8. 推荐的代码整理顺序

建议不要一次加入所有优化，可以按以下顺序逐步整理并在每步后重新生成图片：

1. 将双下划线变量改为 `ap/bp/cp`、`ab/bc/ca`；
2. 缓存 `insideTriangle()` 的三个叉积；
3. 删除 `rasterize_triangle()` 中重复的颜色写入；
4. 缓存 `sample_x`、`sample_y` 和三角形颜色；
5. 在循环前裁剪包围盒；
6. 将循环改成外层 y、内层 x；
7. 让 `set_pixel()` 调用 `get_index()`；
8. 增加退化三角形保护；
9. 基础结果稳定后再实现 MSAA 或 top-left rule。

前七项应保持当前输出不变；如果某一步改变了三角形形状或遮挡关系，说明重构中
引入了新错误，可以立即回到前一步排查。

## 9. 当前实现已经满足的检查清单

- [x] 使用包围盒减少遍历区域；
- [x] 使用像素中心进行覆盖测试；
- [x] 正确判断顺时针和逆时针三角形；
- [x] 使用重心坐标插值深度；
- [x] 使用 `get_index(x, y)` 访问深度缓冲；
- [x] 深度更小时更新缓冲；
- [x] 颜色与深度同步更新；
- [x] 屏幕外像素不会访问缓冲；
- [x] 两个测试三角形的遮挡关系正确；
- [ ] 退化三角形保护；
- [ ] top-left rule；
- [ ] 2×2 MSAA。

最后三项是健壮性、精确边界规则和提高题，不影响当前基础作业完成状态。
