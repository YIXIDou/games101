# Assignment 6 已有框架函数讲解

本文对应 `Assignment6_code` 中尚未完成核心 TODO 之前的代码状态，目的是解释课程框架已经提供了什么、各函数在整个光线追踪流程中的位置，以及阅读和调试时应该怎样使用它们。

本文不会给出四个课程 TODO 的可直接提交实现。文中的“已实现”表示源码中已有函数体，不等于它对所有边界情况都绝对健壮。

## 1. 先看完整执行流程

当前程序预期的数据流如下：

```text
main()
  ├─ 创建 Scene
  ├─ MeshTriangle 读取 bunny.obj
  │    ├─ 把 OBJ 中的每个面转换为 Triangle
  │    └─ 为这些 Triangle 构建模型内部 BVH
  ├─ 把兔子和光源加入 Scene
  ├─ Scene::buildBVH() 构建场景级 BVH
  └─ Renderer::Render()
       ├─ 为每个像素生成主光线（待补全）
       ├─ Scene::castRay()
       │    ├─ Scene::intersect()
       │    │    └─ BVHAccel::Intersect()
       │    │         └─ BVHAccel::getIntersection()（待补全）
       │    └─ 根据材质计算反射、折射或局部光照
       └─ 将 framebuffer 写入 binary.ppm
```

这个工程中有两层 BVH：

- `MeshTriangle` 内部的 BVH：管理兔子模型的所有三角形。
- `Scene` 的 BVH：管理场景中的所有 `Object`。当前场景只有一个兔子对象，但框架仍然按通用场景组织。

理解这两层关系非常重要。场景光线先命中 `MeshTriangle` 这个对象，再由模型内部 BVH 找到具体的 `Triangle`。

## 2. 数学基础：`Vector.hpp`

### 2.1 `Vector3f`

`Vector3f` 同时被用来表示：

- 三维点，例如光线起点和三角形顶点；
- 三维向量，例如方向和法线；
- RGB 颜色，例如 `(1, 0, 0)` 表示红色；
- 光照强度。

三个构造函数的用法：

```cpp
Vector3f zero;              // (0, 0, 0)
Vector3f gray(0.5f);        // (0.5, 0.5, 0.5)
Vector3f point(1, 2, 3);    // (1, 2, 3)
```

框架重载了向量加减、标量乘除和分量乘法：

```cpp
Vector3f p = origin + direction * t;
Vector3f color = lightAmt * diffuseColor;
```

其中 `Vector3f * Vector3f` 是逐分量相乘，不是点积或叉积。点积和叉积必须调用专门函数。

### 2.2 `normalize`

```cpp
Vector3f unitDirection = normalize(direction);
```

作用是把向量长度缩放到 1。主光线方向、表面法线、反射方向和光照方向通常都应该归一化。

零向量不会被除以零，函数会原样返回零向量。

### 2.3 `dotProduct`

```cpp
float d = dotProduct(a, b);
```

点积常用于：

- 判断两个方向的夹角关系；
- Lambert 漫反射中的 `max(0, L·N)`；
- 反射、折射和 Fresnel 公式；
- Möller-Trumbore 求交中的行列式与重心坐标计算。

如果两个单位向量的点积大于 0，它们大体同向；小于 0 则大体反向；等于 0 表示垂直。

### 2.4 `crossProduct`

```cpp
Vector3f normal = normalize(crossProduct(v1 - v0, v2 - v0));
```

叉积返回同时垂直于两个输入向量的向量。三角形构造函数用它根据两条边求表面法线。

叉积与顶点顺序有关。交换两个输入向量会使结果反向，因此框架假设三角形顶点按一致的逆时针顺序排列。

### 2.5 `Min`、`Max` 与 `lerp`

`Vector3f::Min` 和 `Vector3f::Max` 对三个分量分别取最小值或最大值，主要用于合并包围盒。

```cpp
Vector3f lower = Vector3f::Min(a, b);
Vector3f upper = Vector3f::Max(a, b);
```

`lerp(a, b, t)` 进行线性插值：

```cpp
Vector3f mixed = lerp(colorA, colorB, 0.25f);
```

当 `t=0` 时得到 `a`，当 `t=1` 时得到 `b`。`MeshTriangle::evalDiffuseColor` 用它混合棋盘格的两种颜色。

### 2.6 `Vector2f`

`Vector2f` 主要保存二维纹理坐标 `(u, v)`。本作业的兔子渲染几乎不依赖纹理，但接口仍保留了它。

### 2.7 当前限制

`Vector3f` 的只读下标访问 `const operator[]` 已定义，可以读取 `v[0]`、`v[1]`、`v[2]`。非只读版本在头文件中声明但没有定义，因此不要写依赖 `v[index] = value` 的代码，除非先补全该通用工具函数。

## 3. 通用辅助函数：`global.hpp`

### 3.1 `clamp`

```cpp
float result = clamp(0.0f, 1.0f, value);
```

把数值限制在 `[lo, hi]` 范围内。输出 PPM 时，颜色会先被限制到 `[0,1]`，再乘以 255 转成字节。

### 3.2 `solveQuadratic`

```cpp
float t0, t1;
bool hasRoots = solveQuadratic(a, b, c, t0, t1);
```

求解一元二次方程，并保证返回时 `t0 <= t1`。球体求交把光线方程代入球面方程后会得到二次方程，因此 `Sphere` 的几个求交函数都使用它。

返回 `false` 表示判别式小于零，没有实数交点。

### 3.3 `get_random_float`

返回 `[0,1]` 内的随机浮点数。`AreaLight::SamplePoint` 用两个随机数在面光源的局部 `u/v` 方向上采样。

### 3.4 `UpdateProgress`

```cpp
UpdateProgress(currentRow / float(totalRows));
```

在终端中显示渲染进度条。它只负责显示，不参与渲染计算。

### 3.5 `EPSILON` 与 `kInfinity`

- `EPSILON` 在 `Renderer.cpp` 中定义为很小的正数，用于浮点比较和把二次光线起点稍微移离表面，避免自相交。
- `kInfinity` 是可表示的最大 `float`，常作为“尚未找到交点”的初始最近距离。

## 4. 光线：`Ray.hpp`

`Ray` 表示参数方程：

```text
P(t) = origin + t * direction
```

构造方法：

```cpp
Ray ray(origin, normalize(direction));
```

主要成员：

- `origin`：光线起点；
- `direction`：光线方向；
- `direction_inv`：方向各分量的倒数，供 AABB 求交减少重复除法；
- `t_min`、`t_max`：参数的有效范围；
- `t`：框架保留的传播时间字段，本作业没有实际使用。

函数调用运算符可以直接取得光线上参数为 `t` 的位置：

```cpp
Vector3f hitPoint = ray(t);
```

这与手写 `ray.origin + ray.direction * t` 等价。

注意：构造函数不会自动归一化方向，调用者应根据语义自行决定是否调用 `normalize`。

## 5. 相交结果：`Intersection.hpp`

`Intersection` 是 BVH、几何体和着色器之间传递命中信息的统一数据结构。

默认构造出的对象表示“没有命中”：

- `happened = false`；
- `distance = infinity`；
- `obj = nullptr`；
- `m = nullptr`。

一次有效命中通常需要填写：

- `happened`：是否命中；
- `coords`：世界空间交点；
- `normal`：交点法线；
- `distance`：沿光线到交点的参数或距离；
- `obj`：被命中的对象；
- `m`：该对象的材质。

使用形式：

```cpp
Intersection hit = object->getIntersection(ray);
if (hit.happened) {
    // 使用 hit.coords、hit.normal、hit.m 等信息
}
```

BVH 遍历左右子树后，也会通过比较 `distance` 选择较近的有效结果。

## 6. 材质：`Material.hpp`

`MaterialType` 提供三种材质类型：

- `DIFFUSE_AND_GLOSSY`：漫反射加高光；
- `REFLECTION_AND_REFRACTION`：反射加折射；
- `REFLECTION`：镜面反射。

构造示例：

```cpp
Material material(
    DIFFUSE_AND_GLOSSY,
    Vector3f(0.5f, 0.5f, 0.5f),
    Vector3f(0.0f));
material.Kd = 0.6f;
material.Ks = 0.0f;
material.specularExponent = 0.0f;
```

常用 getter：

- `getType()`：决定 `Scene::castRay` 走哪条着色分支；
- `getColor()`：返回基础颜色；
- `getEmission()`：返回自发光颜色。

`getColorAt(u, v)` 当前固定返回黑色，并未真正实现纹理采样。默认构造函数也没有初始化 `ior`、`Kd`、`Ks` 和 `specularExponent`，所以新建材质后应显式设置实际会使用的参数。兔子模型的构造过程已经设置了漫反射相关参数。

## 7. 光源：`Light.hpp` 与 `AreaLight.hpp`

### 7.1 `Light`

`Light` 保存点光源的位置和 RGB 强度：

```cpp
auto light = std::make_unique<Light>(
    Vector3f(-20, 70, 20),
    Vector3f(1));
scene.Add(std::move(light));
```

当前 `main` 中的两个光源就是这样加入场景的。`Scene` 使用 `unique_ptr` 持有光源并负责释放。

### 7.2 `AreaLight`

`AreaLight` 继承自 `Light`，额外保存法线和局部 `u/v` 方向。`SamplePoint()` 可以随机返回面光源上的一点。

但当前 `Scene::castRay` 检测到 `AreaLight` 后会直接跳过，因此面光源采样不属于本作业的有效渲染路径。

## 8. 包围盒辅助功能：`Bounds3.hpp`

`Bounds3` 表示轴对齐包围盒（AABB），只需保存两个角点：

- `pMin`：三个坐标都最小的角；
- `pMax`：三个坐标都最大的角。

其余六个角可以由这两个点的坐标组合得到，因此不需要单独保存。

### 8.1 构造函数

默认构造函数创建一个“空包围盒”：`pMin` 极大、`pMax` 极小。随后连续调用 `Union` 时，它会被第一个真实点或包围盒正确扩展。

```cpp
Bounds3 empty;
Bounds3 pointBox(point);
Bounds3 box(pointA, pointB);
```

双点构造函数会自行对每个坐标取最小值和最大值，因此传参顺序不重要。

### 8.2 `Diagonal`

```cpp
Vector3f size = box.Diagonal();
```

返回 `pMax - pMin`，也就是包围盒在 X、Y、Z 三个方向上的长度。

### 8.3 `maxExtent`

```cpp
int axis = box.maxExtent();
```

返回最长轴编号：X 为 0，Y 为 1，Z 为 2。已有的 BVH 构建函数利用它选择本次节点的划分方向。

### 8.4 `SurfaceArea`

返回包围盒表面积：

```text
2 * (dx*dy + dx*dz + dy*dz)
```

必做的普通 BVH 构建目前不使用它；实现 SAH 加分项时才会用表面积估算一次划分的代价。

### 8.5 `Centroid`

```cpp
Vector3f center = box.Centroid();
```

返回包围盒中心点 `(pMin+pMax)/2`。`recursiveBuild` 根据所有图元包围盒的质心决定排序和划分。

### 8.6 `Intersect(const Bounds3&)`

这个重载计算两个包围盒的重叠区域，返回的仍然是一个 `Bounds3`。它不是本作业要求完成的“光线与包围盒相交”。

### 8.7 `Offset`

把包围盒内部一点转换到相对于包围盒的归一化坐标。若点位于 `pMin`，结果接近 0；位于 `pMax`，结果接近 1。

### 8.8 `Overlaps` 与 `Inside`

- `Overlaps(b1, b2)`：判断两个 AABB 是否在三个轴上都重叠；
- `Inside(p, b)`：判断点 `p` 是否位于包围盒 `b` 内，边界也算在内部。

### 8.9 `operator[]`

```cpp
const Vector3f& lower = box[0];
const Vector3f& upper = box[1];
```

索引 0 返回 `pMin`，其他值返回 `pMax`。它方便根据光线方向选择近端或远端平面。

### 8.10 `Union`

两个全局重载分别用于：

```cpp
Bounds3 mergedBoxes = Union(boxA, boxB);
Bounds3 expandedBox = Union(box, point);
```

`recursiveBuild` 用它合并子节点或物体的包围盒。

`Bounds3::IntersectP(ray, invDir, dirIsNeg)` 是课程要求完成的核心函数，目前不能使用。

## 9. 几何对象接口：`Object.hpp`

`Object` 是所有可求交物体的抽象基类。BVH 只保存 `Object*`，因此既能管理球，也能管理单个三角形或整个三角网格。

接口职责：

- `intersect(ray)`：只询问是否命中；
- `intersect(ray, tnear, index)`：命中时更新最近距离和图元编号；
- `getIntersection(ray)`：返回完整 `Intersection`；
- `getSurfaceProperties(...)`：取得法线和纹理坐标；
- `evalDiffuseColor(st)`：计算表面漫反射颜色；
- `getBounds()`：返回对象的 AABB。

本次 BVH 遍历真正需要关注的是 `getBounds()` 和 `getIntersection()`。

## 10. 球体参考实现：`Sphere.hpp`

虽然当前 `main` 没有创建球体，但 `Sphere` 是理解框架接口最清楚的参考。

### 10.1 三种求交接口

`intersect(ray)` 只返回布尔值；带 `tnear` 的重载会把最近的正交点距离写入输出参数；`getIntersection(ray)` 则构造完整的命中记录。

求交过程是：

1. 把射线方程代入球面方程；
2. 使用 `solveQuadratic` 求 `t0`、`t1`；
3. 优先选择较近的非负根；
4. 如果两个根都在光线起点后方，则没有命中。

### 10.2 `getIntersection`

它展示了有效命中应该怎样填写：

```cpp
result.happened = true;
result.coords = ray.origin + ray.direction * t0;
result.normal = normalize(result.coords - center);
result.m = this->m;
result.obj = this;
result.distance = t0;
```

阅读和实现 `Triangle::getIntersection` 时，可以把这里当作“输出结构应该填哪些字段”的参考，而不是照搬球体的数学求交过程。

### 10.3 其他函数

- `getSurfaceProperties`：用球心指向交点的方向作为法线；
- `evalDiffuseColor`：直接返回材质基础颜色；
- `getBounds`：返回 `center ± radius` 形成的 AABB。

## 11. 三角形与网格：`Triangle.hpp`

### 11.1 `rayTriangleIntersect`

文件顶部的辅助函数已经包含一套 Möller-Trumbore 求交流程，并通过 `tnear`、`u`、`v` 输出交点参数和重心坐标。

它目前通过 `det < 0` 拒绝背面三角形，相当于开启背面剔除；而且用 `det == 0` 直接比较浮点数。它是旧接口所用的辅助函数，不等于课程指定的 `Triangle::getIntersection` 已经完成。

### 11.2 `Triangle` 构造函数

构造时保存三个顶点，预计算两条边，并用叉积求单位法线：

```cpp
Triangle triangle(v0, v1, v2, material);
```

预计算 `e1`、`e2` 和 `normal` 可以避免每条光线重复计算相同几何信息。

### 11.3 已有的简单接口

- `getSurfaceProperties`：把预计算法线写入输出参数；
- `getBounds`：把三个顶点合并成一个 AABB；
- `evalDiffuseColor`：固定返回中灰色。

### 11.4 `MeshTriangle` 构造函数

```cpp
MeshTriangle bunny("../models/bunny/bunny.obj");
```

它执行以下工作：

1. 调用第三方 `objl::Loader::LoadFile` 读取 OBJ；
2. 断言文件中只有一个 mesh；
3. 每三个顶点创建一个 `Triangle`；
4. 把模型坐标统一放大 60 倍；
5. 计算整个模型的 `bounding_box`；
6. 给每个三角形创建灰色漫反射材质；
7. 收集所有 `Triangle*`，构建模型内部 BVH。

`MeshTriangle::getIntersection(ray)` 本身不做求交，而是把工作委托给内部 `bvh->Intersect(ray)`。`getBounds()` 则把整个模型的包围盒提供给场景级 BVH。

### 11.5 不要误用的旧接口

`MeshTriangle::intersect(ray)` 当前固定返回 `true`。另一个带 `tnear/index` 的重载访问 `vertices`、`vertexIndex` 和 `numTriangles`，但当前 OBJ 构造流程没有初始化这些旧成员。

类似地，`Triangle::intersect(ray)` 固定返回 `true`，带输出参数的重载固定返回 `false`。当前设计的有效路径应通过 `getIntersection`，不要把这些桩函数当成正确实现。

`Triangle::getIntersection` 中的数学推导已经写到 `t_tmp`，但还没有把结果填入 `Intersection`，它仍是课程要求完成的函数。

## 12. BVH 节点与构建：`BVH.hpp`、`BVH.cpp`

### 12.1 `BVHBuildNode`

每个节点保存：

- `bounds`：该节点覆盖的全部物体的包围盒；
- `left`、`right`：两个子节点；
- `object`：叶节点对应的具体物体。

叶节点的 `object` 非空，左右孩子为空；内部节点通常相反。

### 12.2 `BVHAccel` 构造函数

```cpp
BVHAccel accel(objects, 1, BVHAccel::SplitMethod::NAIVE);
```

构造函数保存物体列表，调用 `recursiveBuild` 创建树，并输出构建耗时。当前实际构建逻辑使用普通的中位数划分，`SplitMethod::SAH` 只是为加分项保留的枚举值。

### 12.3 `recursiveBuild`

这是框架已经实现的核心构建函数。

当物体数为 1：

- 创建叶节点；
- 节点包围盒等于物体包围盒；
- `object` 指向该物体。

当物体数为 2：

- 分别递归构造左右叶节点；
- 当前节点包围盒等于左右包围盒的并集。

当物体数大于 2：

1. 合并所有物体包围盒的质心，得到 `centroidBounds`；
2. 用 `maxExtent()` 找出质心分布最长的轴；
3. 按该轴上的质心坐标排序；
4. 在中点把物体列表分成左右两半；
5. 递归构建两个子树；
6. 合并左右子树的包围盒。

这是“最长轴 + 中位数”的朴素 BVH 构建策略。它的目标是让树大致平衡，但不会像 SAH 那样估算实际求交代价。

### 12.4 `Intersect`

```cpp
Intersection hit = bvh.Intersect(ray);
```

这是外部使用 BVH 的统一入口。空树直接返回默认的未命中结果；非空树把根节点和光线交给 `getIntersection`。

`getIntersection` 才负责包围盒测试、叶节点求交、递归孩子和选择最近命中，目前尚未完成。

### 12.5 当前未接通的声明

`WorldBound()`、析构函数和 `BVHAccel::IntersectP(ray)` 在头文件中有声明，但源码没有定义。当前主流程没有调用它们，所以工程仍可链接；不要在新代码里调用这些接口，否则会产生链接错误。

## 13. 场景：`Scene.hpp` 与 `Scene.cpp`

### 13.1 场景配置和容器

`Scene(width, height)` 设置输出分辨率。类中还保存视场角、背景色和最大递归深度。

```cpp
Scene scene(1280, 960);
scene.Add(&object);
scene.Add(std::make_unique<Light>(position, intensity));
```

物体使用裸指针保存，因此 `Scene` 不负责释放物体；光源使用 `unique_ptr`，由 `Scene` 管理生命周期。

### 13.2 `buildBVH`

把场景的 `objects` 交给 `BVHAccel`，建立场景级的朴素 BVH：

```cpp
scene.buildBVH();
```

必须在所有物体加入场景后、第一次求交前调用。

### 13.3 `intersect`

```cpp
Intersection hit = scene.intersect(ray);
```

它是一个很薄的转发函数，直接调用场景 BVH 的 `Intersect`。

### 13.4 `trace`

`trace` 会线性遍历所有物体，通过旧版 `intersect` 接口查找最近对象。它代表未使用 BVH 时的朴素方案，当前 `castRay` 已经改用 `Scene::intersect`，所以它不在主要调用链上。

由于三角形和网格的旧 `intersect` 重载含有桩代码，不应把 `trace` 当成本作业当前版本的可靠回退方案。

### 13.5 `reflect`

根据入射方向 `I` 和单位法线 `N` 计算镜面反射方向：

```cpp
Vector3f reflected = scene.reflect(I, N);
```

公式为 `I - 2(I·N)N`。

### 13.6 `refract`

根据 Snell 定律计算折射方向。函数会区分光线从外部进入和从内部离开，并在发生全反射时返回零向量。

参数 `ior` 是材质折射率。

### 13.7 `fresnel`

```cpp
float kr;
scene.fresnel(I, N, ior, kr);
```

计算反射能量比例 `kr`。透明材质使用 `kr` 混合反射颜色与折射颜色；全反射时 `kr=1`。

### 13.8 `castRay`

这是着色阶段的核心函数：

```cpp
Vector3f color = scene.castRay(ray, 0);
```

主要流程：

1. 超过 `maxDepth` 时返回黑色，终止递归；
2. 调用 `Scene::intersect` 查找最近交点；
3. 未命中时返回背景色；
4. 命中时取得材质、交点、法线和表面属性；
5. 根据材质类型选择反射折射、纯反射或漫反射/高光分支。

在漫反射分支中，每个点光源会提供：

- `LdotN` 形式的 Lambert 漫反射；
- 反射方向与视线方向形成的 Phong 高光；
- 从交点向光源发射阴影光线，命中其他物体时判定为阴影。

二次光线起点会沿法线偏移 `EPSILON`，避免它立刻再次命中当前表面。

当前框架的面光源分支未实现，阴影逻辑也是课程简化版本；它们不属于本次必做 BVH 任务。

## 14. 渲染器：`Renderer.hpp` 与 `Renderer.cpp`

### 14.1 `deg2rad`

把角度转换成弧度。`tan` 接收弧度，因此视场角在参与投影计算前必须转换。

### 14.2 `Renderer::Render`

该函数目前只有一部分完成。

已经提供的部分包括：

- 创建 `width * height` 大小的 framebuffer；
- 根据垂直视场角计算成像平面缩放系数；
- 计算图像宽高比；
- 遍历每一个像素；
- 计算像素中心在成像平面上的 `x/y`；
- 显示进度条；
- 把 framebuffer 按二进制 PPM 格式写入 `binary.ppm`。

尚缺的部分是根据 `x/y` 构造并归一化主光线、调用 `scene.castRay`，以及把颜色写入当前 framebuffer 像素。因此现在直接运行只会得到无意义结果。

完成后的一般使用方式是：

```cpp
Renderer renderer;
renderer.Render(scene);
```

输出文件写入进程当前工作目录。VS Code 调试配置把工作目录设为 `build-ucrt`，所以结果会出现在该目录中。

## 15. 程序入口：`main.cpp`

`main` 展示了整个框架的标准使用方式：

1. `Scene scene(1280, 960)` 创建场景；
2. 从 `../models/bunny/bunny.obj` 读取兔子模型；
3. `scene.Add(&bunny)` 加入物体；
4. 加入两个点光源；
5. `scene.buildBVH()` 构建场景 BVH；
6. `Renderer::Render(scene)` 执行渲染；
7. 输出渲染耗时。

模型路径是相对于运行时工作目录解析的。正确方式是从 `Assignment6_code/build-ucrt` 运行：

```powershell
cd build-ucrt
./RayTracing.exe
```

如果从 `Assignment6_code` 直接运行 `build-ucrt/RayTracing.exe`，`../models` 会指向错误位置。

## 16. 第三方 OBJ 加载器：`OBJ_Loader.hpp`

这是一个单头文件第三方加载器。Assignment 6 只使用其很小一部分接口：

```cpp
objl::Loader loader;
loader.LoadFile(filename);
auto mesh = loader.LoadedMeshes[0];
```

随后读取 `mesh.Vertices` 中的顶点位置。完成本作业不需要深入修改该文件，也不建议把调试重点放在第三方解析代码上。

## 17. 构建配置如何使用

`CMakeLists.txt` 把所有源码编译成 `RayTracing` 可执行文件，使用 C++17。

已经提供的 `CMakePresets.json` 固定使用 MSYS2 UCRT64 工具链。命令行构建方法：

```powershell
cd D:/Project/Games101/GAMES101_Homework/Assignment6/Assignment6_code
C:/msys64/ucrt64/bin/cmake.exe --preset ucrt-debug --fresh
C:/msys64/ucrt64/bin/cmake.exe --build --preset ucrt-debug
```

在 VS Code 中可以运行：

- `Assignment6: Configure`：重新生成构建系统；
- `Assignment6: Build`：配置并编译；
- `Assignment6：渲染 binary.ppm（完成 TODO 后）`：编译后使用 GDB 启动渲染器。

在四个作业 TODO 完成之前，不应把成功编译理解为程序已经可以正确渲染。

## 18. 当前未完成或不可直接使用的边界

| 位置 | 当前状态 | 说明 |
| --- | --- | --- |
| `Renderer::Render` | 部分完成 | 已算像素坐标和文件输出，缺主光线、`castRay` 调用和 framebuffer 写入 |
| `Triangle::getIntersection` | 部分完成 | 已算重心坐标与 `t_tmp`，缺 `Intersection` 回填 |
| `Bounds3::IntersectP` | 未完成 | 作业核心：光线与 AABB 求交 |
| `BVHAccel::getIntersection` | 未完成 | 作业核心：BVH 递归遍历和最近交点选择 |
| `Triangle::intersect` 重载 | 桩函数 | 固定返回 `true/false`，不要作为正确求交使用 |
| `MeshTriangle::intersect` 重载 | 旧接口/桩函数 | 当前模型构造流程不支持其中依赖的旧数组 |
| `Material::getColorAt` | 占位 | 固定返回黑色，没有纹理采样 |
| `AreaLight` 在 `castRay` 中的处理 | 未接通 | 检测到面光源后跳过 |
| `BVHAccel::WorldBound`、析构和成员 `IntersectP` | 只有声明 | 当前未调用；调用会导致链接错误 |
| `Scene::HandleAreaLight` | 只有声明 | 当前未调用，也没有定义 |
| `Vector3f` 非 const 下标运算符 | 只有声明 | 不要用下标方式修改向量 |

## 19. 推荐阅读与调试顺序

建议按下面的顺序阅读，而不是从 `main.cpp` 开始逐文件随机跳转：

1. `Vector.hpp`、`Ray.hpp`、`Intersection.hpp`：理解数据如何表示；
2. `Bounds3.hpp`：理解 BVH 节点包围盒需要的辅助操作；
3. `Object.hpp`、`Sphere.hpp`：理解几何对象接口和完整命中记录；
4. `Triangle.hpp`：理解兔子最终如何产生三角形交点；
5. `BVH.cpp::recursiveBuild`：理解树是怎样建出来的；
6. `Scene::intersect`、`Scene::castRay`：理解求交结果怎样进入着色；
7. `Renderer::Render` 和 `main`：最后串联完整流程。

完成 TODO 后，适合设置断点的位置：

- `BVHAccel::recursiveBuild`：观察叶节点与内部节点结构；
- `Bounds3::IntersectP`：只跟踪一条测试光线的三个轴区间；
- `BVHAccel::getIntersection`：观察包围盒裁剪和左右子树返回值；
- `Triangle::getIntersection`：检查 `t/u/v` 和最终 `Intersection`；
- `Scene::castRay`：检查命中结果如何转化为颜色。

先保证必做路径正确，再考虑 SAH、面光源或旧接口清理。这样可以把课程核心问题与框架遗留问题分开。
