# Assignment 5 已完成框架函数说明

本文对应 `Assignment5_code/` 当前代码，用来解释课程框架已经实现的函数、类和数据结构。本文不会给出 `Renderer::Render()` 中主射线生成或 `rayTriangleIntersect()` 的完整实现。

## 1. 先看整体调用链

程序的主要执行顺序是：

```text
main()
  ├─ 创建 Scene
  ├─ 创建 Sphere、MeshTriangle 和 Light
  ├─ Scene::Add() 把它们加入场景
  └─ Renderer::Render(scene)
       ├─ 遍历全部像素
       ├─ 为像素生成主射线                  <- 尚待完成
       └─ castRay()
            ├─ trace()
            │    └─ Object::intersect()
            │         ├─ Sphere::intersect()
            │         └─ MeshTriangle::intersect()
            │              └─ rayTriangleIntersect() <- 尚待完成
            ├─ Object::getSurfaceProperties()
            └─ 根据材质计算反射、折射或局部光照
```

理解这条调用链后，可以把框架分成四层：

1. `main.cpp`：描述场景中有什么。
2. `Scene`、`Object`、`Light`：保存场景数据。
3. `trace()` 和各个 `intersect()`：解决“射线击中了谁”。
4. `castRay()`：解决“击中之后像素应该是什么颜色”。

## 2. `Vector.hpp`：基础向量类型和运算

### 2.1 `Vector3f`

`Vector3f` 表示三维浮点向量，也被复用于位置、方向、法线和 RGB 颜色。

```cpp
Vector3f p(1, 2, 3);       // 三维位置或方向
Vector3f black(0);         // 等价于 (0, 0, 0)
Vector3f gray(0.5f);       // 等价于 (0.5, 0.5, 0.5)
```

构造函数：

- `Vector3f()`：三个分量均为 0。
- `Vector3f(float xx)`：三个分量都设为 `xx`。
- `Vector3f(float xx, float yy, float zz)`：分别设置三个分量。

已实现的运算符：

- `v * r`、`r * v`：向量乘标量。
- `v / r`：向量除以标量；调用方需要保证 `r != 0`。
- `a * b`：逐分量相乘，不是点积。
- `a + b`、`a - b`：逐分量加减。
- `-v`：反向向量。
- `a += b`：把 `b` 累加到 `a`。
- `std::cout << v`：输出 `x, y, z`。

逐分量乘法常用于把光照颜色和材质颜色组合起来：

```cpp
Vector3f result = lightColor * surfaceColor;
```

点积必须使用 `dotProduct()`，不能写成 `a * b`。

### 2.2 `Vector2f`

`Vector2f` 表示二维浮点向量。本作业主要用它保存：

- 三角形求交得到的两个重心坐标 `u`、`v`；
- 纹理坐标 `st`。

它提供零初始化、单值初始化、双分量初始化、标量乘法和向量加法。

### 2.3 `lerp()`

```cpp
Vector3f lerp(const Vector3f& a, const Vector3f& b, float t);
```

作用：在 `a` 和 `b` 之间做线性插值。

```text
result = a * (1 - t) + b * t
```

- `t = 0` 时返回 `a`。
- `t = 1` 时返回 `b`。
- `t` 在 0 和 1 之间时得到两者之间的值。

框架在 `MeshTriangle::evalDiffuseColor()` 中用它在两种棋盘颜色之间选择。

### 2.4 `normalize()`

```cpp
Vector3f normalize(const Vector3f& v);
```

作用：返回与 `v` 同方向、长度为 1 的单位向量。若输入是零向量，函数直接返回零向量，避免除以零。

典型用途：

- 把射线方向变成单位向量；
- 计算单位法线；
- 把“交点到光源”的向量变成光照方向。

射线方向归一化后，光线参数 `t` 才可以直观地理解为沿射线前进的距离。

### 2.5 `dotProduct()`

```cpp
float dotProduct(const Vector3f& a, const Vector3f& b);
```

点积常见用途：

- 判断两个方向是同侧还是异侧；
- 计算夹角余弦；
- Lambert 漫反射中的 `max(0, L · N)`；
- 反射、折射和 Fresnel 计算；
- 球体求交方程的系数。

两个单位向量的点积范围是 `[-1, 1]`。

### 2.6 `crossProduct()`

```cpp
Vector3f crossProduct(const Vector3f& a, const Vector3f& b);
```

叉积返回同时垂直于 `a` 和 `b` 的向量，方向遵循右手定则。本作业中它用于：

- 根据三角形的两条边计算面法线；
- Möller-Trumbore 算法中的行列式和重心坐标计算。

叉积顺序会影响结果方向：`crossProduct(a, b) = -crossProduct(b, a)`。

## 3. `global.hpp`：通用数学工具

### 3.1 `kInfinity`

```cpp
constexpr float kInfinity = std::numeric_limits<float>::max();
```

它不是数学上的无穷大，而是 `float` 能表达的最大有限值。求最近交点时，框架先把当前最近距离设为 `kInfinity`，之后任何正常交点都会比它近。

### 3.2 `clamp()`

```cpp
float clamp(float lo, float hi, float v);
```

把 `v` 限制在 `[lo, hi]` 内：

- 小于 `lo` 时返回 `lo`；
- 大于 `hi` 时返回 `hi`；
- 否则返回 `v`。

框架的用途包括：

- 将点积限制在 `[-1, 1]`，避免后续三角函数或平方根受到浮点误差影响；
- 写图片前把颜色限制在 `[0, 1]`。

### 3.3 `solveQuadratic()`

```cpp
bool solveQuadratic(float a, float b, float c, float& x0, float& x1);
```

求解二次方程：

```text
a*x^2 + b*x + c = 0
```

返回值和输出约定：

- 没有实根时返回 `false`。
- 有实根时返回 `true`，并把两个根写入 `x0`、`x1`。
- 函数最后保证 `x0 <= x1`。

实现没有直接对两个根都使用普通求根公式，而是先计算数值上更稳定的 `q`，以减少两个接近数字相减造成的精度损失。

`Sphere::intersect()` 使用它计算射线与球面的两个候选交点。

### 3.4 `MaterialType`

材质枚举决定 `castRay()` 采用哪种着色分支：

- `DIFFUSE_AND_GLOSSY`：漫反射加高光。
- `REFLECTION_AND_REFRACTION`：同时递归追踪反射与折射。
- `REFLECTION`：只递归追踪反射。

### 3.5 `get_random_float()`

返回 `[0, 1]` 范围内的随机浮点数。Assignment 5 当前渲染流程没有使用它；它更适合后续随机采样类作业。

### 3.6 `UpdateProgress()`

```cpp
void UpdateProgress(float progress);
```

根据 `[0, 1]` 范围的进度值，在终端中绘制一个 70 字符宽的进度条。`Renderer::Render()` 每完成一行像素就调用一次。

## 4. `Object.hpp`：所有可求交物体的统一接口

`Object` 是抽象基类。球体和三角形网格都继承它，因此 `trace()` 可以通过同一套接口处理不同几何体。

### 4.1 构造函数 `Object()`

为物体设置默认材质参数：

- `materialType = DIFFUSE_AND_GLOSSY`
- `ior = 1.3`：折射率。
- `Kd = 0.8`：漫反射权重。
- `Ks = 0.2`：高光权重。
- `diffuseColor = (0.2, 0.2, 0.2)`
- `specularExponent = 25`：高光集中程度。

这些字段都是公开的，所以创建物体后可以直接修改：

```cpp
auto sphere = std::make_unique<Sphere>(Vector3f(0, 0, -8), 1.5f);
sphere->materialType = REFLECTION_AND_REFRACTION;
sphere->ior = 1.5f;
```

### 4.2 虚析构函数 `~Object()`

`virtual ~Object() = default` 保证通过 `Object*` 或 `unique_ptr<Object>` 删除派生类对象时，会正确调用派生类析构函数。

### 4.3 `intersect()`

```cpp
virtual bool intersect(
    const Vector3f& orig,
    const Vector3f& dir,
    float& tnear,
    uint32_t& index,
    Vector2f& uv) const = 0;
```

这是纯虚函数，每种几何体必须提供自己的实现。

参数约定：

- `orig`：射线起点。
- `dir`：射线方向。
- `tnear`：输出交点距离。
- `index`：输出被击中的三角形编号；球体不需要它。
- `uv`：输出重心坐标；球体不需要它。
- 返回值：是否存在有效交点。

### 4.4 `getSurfaceProperties()`

在已经知道交点后，返回着色所需的表面信息：

- 表面法线 `N`；
- 表面纹理坐标 `st`。

球体和三角形网格的表面数据计算方式不同，因此它同样是纯虚函数。

### 4.5 `evalDiffuseColor()`

默认实现忽略表面位置，直接返回物体的 `diffuseColor`。三角形网格重写了该函数，用 `st` 坐标生成棋盘格。

## 5. `Sphere.hpp`：球体

### 5.1 `Sphere()`

```cpp
Sphere(const Vector3f& center, float radius);
```

保存球心、半径以及预先计算好的半径平方。保存 `radius2` 可以避免每次求交时重复计算乘法。

### 5.2 `Sphere::intersect()`

将射线方程代入球面方程，得到关于 `t` 的二次方程，再调用 `solveQuadratic()` 求根。

处理过程：

1. 计算二次方程系数 `a`、`b`、`c`。
2. 无实根则表示没有相交。
3. 优先选择较小的根 `t0`。
4. 如果 `t0 < 0`，尝试较大的根 `t1`，这可能表示射线从球内出发。
5. 两个根都小于 0 时，球体位于射线后方。
6. 把有效根写入 `tnear`。

它不需要 `index` 和 `uv`，所以这两个参数在函数签名中没有命名。

### 5.3 `Sphere::getSurfaceProperties()`

球面上一点 `P` 的法线就是从球心指向该点的单位向量：

```text
N = normalize(P - center)
```

## 6. `Triangle.hpp`：三角形网格

### 6.1 `rayTriangleIntersect()`

该函数目前仍是 TODO，不属于已完成部分。它最终需要判断一条射线是否击中由 `v0`、`v1`、`v2` 构成的三角形，并输出：

- `tnear`：射线到交点的参数距离；
- `u`、`v`：两个重心坐标。

后面的 `MeshTriangle::intersect()` 已经写好，并依赖这个函数的结果。

### 6.2 `MeshTriangle()`

构造函数接收：

- 顶点数组 `verts`；
- 顶点索引数组 `vertsIndex`；
- 三角形数量 `numTris`；
- 每个顶点的纹理坐标 `st`。

构造函数先从索引中找到最大顶点编号，据此确定需要复制多少个顶点，然后把顶点、索引和纹理坐标复制到自己的动态数组中。

这意味着创建 `MeshTriangle` 后，原始局部数组离开作用域也不会影响网格对象。

### 6.3 `MeshTriangle::intersect()`

它遍历网格中的每个三角形：

1. 根据索引取出三角形三个顶点。
2. 调用 `rayTriangleIntersect()`。
3. 如果相交且新交点更近，就更新 `tnear`、三角形编号 `index` 和重心坐标 `uv`。
4. 最终返回网格中是否有任意三角形被击中。

当前场景地面只有两个三角形，所以逐个测试足够快。大型场景通常会使用 BVH 等加速结构，但不是本次作业要求。

### 6.4 `MeshTriangle::getSurfaceProperties()`

根据被击中的三角形编号 `index` 和重心坐标 `uv` 计算：

- `N`：由两条边叉乘并归一化得到的面法线；
- `st`：三个顶点纹理坐标的重心插值结果。

纹理坐标插值使用的三个权重是：

```text
1 - u - v, u, v
```

因此 `rayTriangleIntersect()` 输出的 `u`、`v` 不仅用于判断点是否在三角形内，也会直接影响棋盘纹理位置。

### 6.5 `MeshTriangle::evalDiffuseColor()`

把 `st` 坐标放大 5 倍并取小数部分，然后用异或运算判断当前点位于棋盘的哪一格，最终在红色和黄色之间选择。

这里没有读取图片纹理，棋盘图案完全由数学公式程序化生成。

## 7. `Light.hpp`：点光源数据

### 7.1 `Light()`

```cpp
Light(const Vector3f& position, const Vector3f& intensity);
```

保存光源位置和 RGB 强度。

`main.cpp` 中写的是：

```cpp
std::make_unique<Light>(Vector3f(-20, 70, 20), 0.5)
```

虽然第二个参数看起来是单个浮点数，但它会调用 `Vector3f(float)`，转成 `(0.5, 0.5, 0.5)`，即三个颜色通道强度相同的白光。

## 8. `Scene.hpp`：场景容器和渲染参数

### 8.1 `Scene()`

```cpp
Scene(int width, int height);
```

设置输出图像宽度和高度。其余默认渲染参数包括：

- `fov = 90`：垂直视场角，单位是度。
- `backgroundColor`：射线未击中物体时使用的背景颜色。
- `maxDepth = 5`：递归光线追踪的最大深度。
- `epsilon = 0.00001`：新射线起点沿法线偏移的距离，用于防止自相交。

### 8.2 `Scene::Add()`

有两个同名重载：

```cpp
void Add(std::unique_ptr<Object> object);
void Add(std::unique_ptr<Light> light);
```

一个添加物体，另一个添加光源。调用时需要转移所有权：

```cpp
scene.Add(std::move(sphere));
```

`std::move` 之后，对象由 `Scene` 管理，原来的 `unique_ptr` 通常会变成空指针，不应继续通过它访问对象。

### 8.3 `get_objects()` 与 `get_lights()`

返回场景内部物体数组和灯光数组的常量引用：

- 使用引用避免复制整个数组。
- 使用 `const` 防止调用方替换或删除其中的对象。
- `castRay()` 和 `trace()` 使用这些函数读取场景内容。

`Scene.cpp` 当前只包含头文件，没有额外实现，因为成员函数都直接定义在 `Scene.hpp` 中。

## 9. `Renderer.hpp`：命中数据和渲染器接口

### 9.1 `hit_payload`

`hit_payload` 把一次最近命中的结果集中保存起来：

- `tNear`：最近交点的射线参数。
- `index`：被击中的三角形编号。
- `uv`：三角形重心坐标。
- `hit_obj`：被击中物体的非拥有型指针。

`hit_obj` 只用来访问由 `Scene` 持有的对象，不负责释放对象。

### 9.2 `Renderer::Render()` 声明

`Renderer` 对外只提供一个入口：

```cpp
void Render(const Scene& scene);
```

它读取场景但不修改场景，所以参数是 `const Scene&`。

## 10. `Renderer.cpp`：光线追踪主流程

### 10.1 `deg2rad()`

把角度转换成弧度：

```text
radian = degree * pi / 180
```

`std::tan()` 接收弧度，因此计算视场角缩放前必须转换。

### 10.2 `reflect()`

```cpp
Vector3f reflect(const Vector3f& I, const Vector3f& N);
```

根据入射方向 `I` 和单位法线 `N` 计算镜面反射方向：

```text
R = I - 2 * (I · N) * N
```

它被镜面反射材质、高光计算以及反射/折射材质使用。

### 10.3 `refract()`

```cpp
Vector3f refract(const Vector3f& I, const Vector3f& N, float ior);
```

根据 Snell 定律计算折射方向。函数会判断射线是从空气进入物体，还是从物体内部离开，并据此：

- 交换入射介质和出射介质的折射率；
- 必要时反转法线；
- 判断是否发生全反射。

若发生全反射，函数返回零向量；否则返回折射方向。

### 10.4 `fresnel()`

```cpp
float fresnel(const Vector3f& I, const Vector3f& N, float ior);
```

计算 Fresnel 反射比例 `kr`。它同时考虑垂直和平行偏振分量，并在全反射时返回 1。

在透明材质分支中：

```text
最终颜色 = 反射颜色 * kr + 折射颜色 * (1 - kr)
```

观察角度越接近掠射角，通常反射比例越高。

### 10.5 `trace()`

```cpp
std::optional<hit_payload> trace(orig, dir, objects);
```

作用：找出射线在整个场景中击中的最近物体。

处理过程：

1. 把当前最近距离初始化为 `kInfinity`。
2. 遍历所有物体并调用各自的 `intersect()`。
3. 只保存距离更近的命中。
4. 命中时返回 `hit_payload`，没有命中时返回空的 `std::optional`。

使用方式：

```cpp
if (auto payload = trace(orig, dir, scene.get_objects()); payload)
{
    // payload->tNear、payload->hit_obj 等字段在这里有效
}
```

这种写法同时完成变量声明和“是否命中”的判断。`trace()` 只定义在 `Renderer.cpp` 中，没有放入头文件，因此它是该实现文件内部使用的辅助函数，不是 `Renderer` 的公开接口。

### 10.6 `castRay()`

```cpp
Vector3f castRay(orig, dir, scene, depth);
```

作用：追踪一条射线并返回 RGB 颜色，是整个 Whitted-style 光线追踪器的核心调度函数。

共同流程：

1. 如果递归深度超过 `scene.maxDepth`，返回黑色。
2. 默认颜色设为背景色。
3. 调用 `trace()` 查找最近交点。
4. 没有交点则保留背景色。
5. 有交点则计算交点位置、法线和纹理坐标。
6. 根据材质类型进入不同分支。

三种材质分支：

#### 反射并折射

- 计算反射方向和折射方向。
- 把两条新射线的起点沿法线偏移 `epsilon`，防止刚发出的射线再次击中当前表面。
- 递归调用 `castRay()` 获得两条射线的颜色。
- 用 `fresnel()` 返回的比例混合两种颜色。

#### 仅反射

- 计算反射方向。
- 偏移射线起点。
- 递归追踪反射射线，并乘以 Fresnel 反射比例。

#### 漫反射与高光

对每个光源分别计算：

- 从交点指向光源的单位方向；
- Lambert 余弦项 `max(0, L · N)`；
- 从交点到光源的距离平方；
- 阴影射线是否在到达光源前先击中其他物体；
- Phong 风格的高光强度。

最后将漫反射项和高光项相加。

需要特别理解两个细节：

- `depth` 只控制递归反射/折射，不是像素循环次数。初始主射线使用 `depth = 0`。
- `epsilon` 偏移是为了处理浮点误差导致的自阴影或自相交，不是为了让物体真的发生位置变化。

`castRay()` 和 `trace()` 一样只在 `Renderer.cpp` 内部使用。

### 10.7 `Renderer::Render()` 中已完成的部分

该函数仍包含主射线生成 TODO，但外围框架已经完成：

1. 创建大小为 `width * height` 的帧缓冲区。
2. 根据视场角计算 `scale`。
3. 计算图像宽高比 `imageAspectRatio`。
4. 把相机位置设为原点 `eye_pos = (0, 0, 0)`。
5. 用两层循环遍历像素。
6. 对每条主射线调用 `castRay()` 并保存颜色。
7. 每完成一行就更新进度条。
8. 把帧缓冲区写入二进制 PPM 文件 `binary.ppm`。

写文件前，每个颜色通道先被限制到 `[0, 1]`，再乘以 255 转成 8 位颜色。

目前需要你完成的是像素对应的 `x`、`y` 和归一化后的 `dir`，不是重新实现帧缓冲或 PPM 输出。

## 11. `main.cpp`：如何使用整个框架

`main()` 展示了完整的场景搭建方式。

### 11.1 创建场景

```cpp
Scene scene(1280, 960);
```

这会生成 1280 × 960 的输出图像。

### 11.2 创建和设置物体

第一个球使用漫反射/高光材质；第二个球使用反射/折射材质。地面由四个顶点和两个三角形构成。

`std::make_unique<T>()` 创建由 `unique_ptr` 管理的对象。将它加入场景时使用 `std::move()` 转移所有权：

```cpp
scene.Add(std::move(sph1));
```

### 11.3 创建光源

场景添加了两个不同位置的白色点光源。`castRay()` 会遍历它们并累加光照贡献。

### 11.4 开始渲染

```cpp
Renderer renderer;
renderer.Render(scene);
```

运行目录决定 `binary.ppm` 保存在哪里。当前 VS Code 调试配置把工作目录设为 `build-ucrt`，所以结果会出现在：

```text
Assignment5_code/build-ucrt/binary.ppm
```

## 12. 建议的阅读和调试顺序

第一次阅读时建议按下面顺序，而不是按文件名顺序：

1. `main.cpp`：先看场景里有什么。
2. `Scene.hpp`：理解场景如何持有物体和光源。
3. `Object.hpp`：理解统一求交接口。
4. `Sphere.hpp`：先看一个已经完成的求交实现。
5. `Renderer.cpp` 中的 `trace()`：理解如何选择最近交点。
6. `castRay()`：理解命中之后如何着色。
7. `MeshTriangle::intersect()`：理解它期望 TODO 返回什么。
8. 最后再处理 `Renderer::Render()` 和 `rayTriangleIntersect()` 两个 TODO。

适合设置断点的位置：

- `Renderer::Render()` 调用 `castRay()` 前：检查某个像素的主射线。
- `trace()` 调用 `object->intersect()` 后：检查各物体返回的距离。
- `castRay()` 中 `switch` 前：检查命中物体和法线。
- `MeshTriangle::intersect()` 更新 `tnear` 处：检查三角形编号和重心坐标。

由于完整图像会产生大量射线，调试时可以暂时只对中心像素设置条件断点，而不要让调试器在每个像素都停下来。

## 13. 当前完成与未完成边界

已经完成并可直接使用：

- 向量和通用数学工具；
- 场景、物体、球体和光源数据结构；
- 球体求交；
- 三角形网格遍历、法线和纹理坐标插值；
- 最近命中查询；
- 反射、折射、Fresnel、阴影和局部光照；
- 帧缓冲、进度显示和 PPM 输出。

仍需要你完成：

- `Renderer::Render()` 中的主射线生成与方向归一化；
- `rayTriangleIntersect()` 中的 Möller-Trumbore 求交。

因此你的主要任务不是搭建一个完整光线追踪器，而是补上“相机如何发出射线”和“射线如何击中三角形”这两个关键环节，并理解它们怎样接入现有框架。
