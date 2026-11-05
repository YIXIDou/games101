# Assignment7：已提供框架函数的作用与用法

> 本文讲解课程初始框架，旧 TODO 和占位接口按当时源码理解。
> 当前提交状态见 [提交 README](Assignment7_code/README.md)。

核对日期：2026-11-05。本文依据当前 `Assignment7_code/` 的实际源码编写。
目标是让你知道“已经有什么工具、如何调用、返回什么、哪些地方不能直接用”，
方便自己完成路径追踪。本次只新增讲解文档和 README 导航，没有修改算法。

配套文档：[作业要求与学习路线](Assignment7_作业要求与学习路线.md)、
[环境搭建与运行](Assignment7_环境搭建与运行.md)。

## 1. 先看全局：哪些已经实现，哪些还没完成

框架将工作分为四层：

```text
main                           创建材质、模型、场景
  ├─ MeshTriangle 构造          加载 OBJ、创建三角形、建立网格内部 BVH
  ├─ Scene::Add                将模型登记到场景
  ├─ Scene::buildBVH           建立场景级 BVH
  └─ Renderer::Render          遍历像素、生成相机光线、做 SPP 平均、写图
       └─ Scene::castRay       【待完成】计算一条光线路径的颜色
            ├─ Scene::intersect     求场景交点的入口
            ├─ Scene::sampleLight   在光源表面采样
            └─ Material             sample / pdf / eval
```

“入口已经写好”和“底层算法已经可用”是两回事。例如 `Scene::intersect()`
已经有完整函数体，但它调用的 BVH 遍历仍是 TODO，当前不能作为正确求交使用。

| 状态 | 主要内容 | 现在应怎样对待 |
| --- | --- | --- |
| 已提供主要实现 | 场景搭建、BVH 构建、像素循环、向量工具、材质接口 | 阅读下文，理解参数与适用条件 |
| 已提供调用入口，依赖未完成算法 | `Scene::intersect()`、`BVHAccel::Intersect()`、`MeshTriangle::getIntersection()` | 先迁移三个求交函数，再用来验证交点 |
| 你要完成 | `Bounds3::IntersectP()`、`BVHAccel::getIntersection()`、`Triangle::getIntersection()`、`Scene::castRay()` | 这四处不是本篇所称的完成函数 |
| 原框架旧接口或占位 | 部分 `intersect()` 重载、`getColorAt()`、缺少定义的声明 | 不要仅凭名字判断可用，见第 11 节 |
| 有实现但有适用边界 | 面积采样、球面采样、部分包围盒工具 | 先理解设计，再看第 12 节中的源码局限 |

建议先读第 2～6 节，再按需要查阅构建、数学工具和旧接口。第三方 OBJ 解析器
内部的字符串处理、三角剖分等细节不逐个展开；本作业主要使用它的模型加载入口。

## 2. 两个贯穿整个项目的数据类型

### 2.1 `Ray`：起点和方向组成一条光线

源码：[Ray.hpp](Assignment7_code/Ray.hpp)。

```cpp
Ray(const Vector3f& ori, const Vector3f& dir, const double _t = 0.0);
Vector3f operator()(double t) const;
```

构造函数保存起点、方向，预计算逐分量倒数 `direction_inv`，并设置 `t_min=0`、
`t_max=double` 的最大值。它**不会自动归一化方向**，也不会检查零方向。

`ray(s)` 返回 `origin + s * direction`，即光线参数为 `s` 时的位置。

```cpp
Vector3f origin(0, 0, 0);
Vector3f direction = normalize(Vector3f(0, 0, 2));
Ray ray(origin, direction);
Vector3f point = ray(5.0); // (0, 0, 5)
```

注意：

- 只有方向长度为 1 时，参数 `s` 才能直接解释为沿光线走过的距离。
- 成员 `ray.t` 是另存的时间参数，`ray(s)` 不会修改它，也不是在读取它。
- `t_min/t_max` 只是字段；求交实现必须主动使用它们才会形成截断范围。
- 某方向分量为 0 时，倒数可能是无穷大，包围盒求交须处理这种情况。

### 2.2 `Intersection`：既装求交结果，也被复用来装采样点

源码：[Intersection.hpp](Assignment7_code/Intersection.hpp)。

| 字段 | 表示什么 | 使用前提 |
| --- | --- | --- |
| `happened` | 是否真正求交命中 | 先检查它，再读取命中结果 |
| `coords` | 命中点或采样点的世界坐标 | 由对应函数写入 |
| `normal` | 表面法线 | 默认三角形/球体实现提供单位法线 |
| `distance` | 求交光线的参数值 | 单位方向时可当作距离 |
| `obj` | 命中的几何对象指针 | 不一定等于场景中的整个网格对象 |
| `m` | 命中表面的材质指针 | 普通命中需要求交函数填写 |
| `emit` | 发光值 | 光源网格采样会填写；不要假设所有求交都会填写 |
| `tcoords` | 预留的纹理相关坐标 | 默认主流程没有自动计算它 |

默认构造得到 `happened=false`、很大的 `distance`、空的 `obj/m`；向量字段默认是零。

**重点：采样结果不等于命中结果。** `Triangle::Sample()` 只写坐标、法线和 PDF，
不会设置 `happened=true`、`obj` 或 `m`。`MeshTriangle::Sample()` 会额外写 `emit`。
因此不能用 `lightSample.happened` 判断采样成功，也不能直接解引用 `lightSample.m`。

## 3. 场景函数：你在路径追踪中最常接触的入口

源码：[Scene.hpp](Assignment7_code/Scene.hpp)、[Scene.cpp](Assignment7_code/Scene.cpp)。

### 3.1 `Scene(w, h)`：创建场景并设置输出尺寸

```cpp
Scene(int w, int h);
```

它把宽高存入 `width/height`。其他字段使用类中的默认值，例如 `fov=40`、
`RussianRoulette=0.8`。构造场景不会加载模型，也不会自动建立 BVH。

`maxDepth`、`backgroundColor` 等只是配置字段，是否起作用取决于算法有没有读取。
当前空的 `castRay()` 不会自动执行深度限制、返回背景色或进行 RR。

### 3.2 `Scene::Add(Object*)`：把模型加入场景

```cpp
void Add(Object* object);
const std::vector<Object*>& get_objects() const;
```

`Add()` 将对象指针追加到 `objects`；`get_objects()` 返回这个数组的只读引用。
这里没有复制模型，也没有接管原始指针的生命周期。

```cpp
Scene scene(128, 128);
Material white(DIFFUSE, Vector3f(0));
white.Kd = Vector3f(0.7f);
MeshTriangle mesh("../models/cornellbox/floor.obj", &white);
scene.Add(&mesh);
scene.buildBVH();
```

示例只演示搭建和构建顺序，不会渲染。模型路径相对于运行时工作目录。
`mesh` 和 `white` 必须在后续使用场景期间一直存活；不要登记马上离开作用域的局部对象。

另一个重载 `Add(std::unique_ptr<Light>)` 存入的是 `lights`，可用 `get_lights()` 访问。
**当前 `sampleLight()` 遍历的是 `objects` 中的发光几何体，不是 `lights`。**
默认场景的灯是 `light.obj` 网格加发光材质。

### 3.3 `Scene::buildBVH()`：为整个场景建立加速树

```cpp
void buildBVH();
```

内部执行 `new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE)`，并保存到 `scene.bvh`。
应在 `Add()` 完成后调用，在第一次场景求交之前调用。

它只负责**建树**，不会测试光线，也不会生成颜色。建树成功不能证明 BVH 遍历正确。
之后若新增对象，原来的树不会自动更新；当前实现也没有处理反复重建时旧树的释放。
本作业的标准用法是建好非空静态场景后构建一次。

### 3.4 `Scene::intersect()`：路径追踪用的求交入口

```cpp
Intersection intersect(const Ray& ray) const;
```

函数本身只有一层转发：`return bvh->Intersect(ray);`。
预期返回光线最近的有效交点；未命中时返回 `happened=false`。

三个求交 TODO 完成以后，可以按下面方式使用：

```cpp
Intersection hit = scene.intersect(ray);
if (hit.happened && hit.m != nullptr) {
    Vector3f position = hit.coords;
    Vector3f normal = hit.normal;
    Vector3f emission = hit.m->getEmission();
    // 在这里继续分析命中表面。
}
```

前提是场景 BVH 已建立，且所有模型/材质仍有效。不要在当前求交 TODO 未完成时
运行此示例来判断环境是否正常；也不要在 `happened=false` 时访问 `hit.m`。

### 3.5 `Scene::sampleLight()`：在发光物体上采样一个位置

```cpp
void sampleLight(Intersection& pos, float& pdf) const;
```

两个参数都通过引用输出：`pos` 用来接收采样位置等信息，`pdf` 接收面积概率密度。
它不接收着色点，所以不可能替你判断“这个灯点对当前表面是否可见”。

实现分为三步：

1. 遍历 `objects`，调用 `hasEmit()`，求所有发光物体面积之和。
2. 在累计面积区间里选中一个发光对象，面积大的对象被选中的机会更高。
3. 调用这个对象的 `Sample(pos, pdf)`，由下层采样器填写结果。

```cpp
Intersection lightPoint;
float pdfArea = 0.0f;
scene.sampleLight(lightPoint, pdfArea);
if (pdfArea > 0.0f) {
    Vector3f x = lightPoint.coords;
    Vector3f lightNormal = lightPoint.normal;
    Vector3f emitted = lightPoint.emit; // 当前默认灯是 MeshTriangle
}
```

使用前提是场景中有面积为正的发光对象。当前实现没有明确报告“没有光源”或无效面积，
所以调用者应先初始化输出 PDF，并保证场景设置合理。

采样之后仍需由你计算表面到灯的方向、距离、两端余弦和可见性。
**这个 PDF 是对面积的密度，不是材质的方向密度。** 多光源和底层分布的局限见第 12 节。

## 4. 材质三个接口：方向、密度、反射能力

源码：[Material.hpp](Assignment7_code/Material.hpp)。当前枚举只有 `DIFFUSE`。
虽然 PDF 可能写 `Material.cpp`，这份代码的实现实际在头文件中。

### 4.1 构造函数与简单查询

```cpp
Material(MaterialType t = DIFFUSE, Vector3f e = Vector3f(0, 0, 0));
MaterialType getType();
Vector3f getEmission();
bool hasEmission();
```

- 构造函数保存类型 `m_type` 与发光值 `m_emission`。
- `getType()` 返回材质类别。
- `getEmission()` 返回 RGB 发光值；它没有自动乘面积、距离衰减或余弦。
- `hasEmission()` 用 `m_emission.norm() > EPSILON` 判断是否发光。

```cpp
Material matte(DIFFUSE, Vector3f(0)); // 不自行发光
matte.Kd = Vector3f(0.7f, 0.2f, 0.2f); // RGB 漫反射系数

Material lamp(DIFFUSE, Vector3f(10.0f)); // 三通道发光值都是 10
lamp.Kd = Vector3f(0.65f);
```

`Kd` 是反射系数，`m_emission` 是自行发出的光，含义不同。构造函数不为你设置
想要的反射颜色：`Kd/Ks` 默认由 `Vector3f` 初始化为零；`ior/specularExponent`
等普通标量在这个构造函数中没有初始化，扩展到相关模型前应主动设置。

### 4.2 `Material::sample()`：随机选择下一条反弹方向

```cpp
Vector3f sample(const Vector3f& wi, const Vector3f& N);
```

| 参数/返回 | 当前实现中的实际作用 |
| --- | --- |
| `wi` | 保留的已有方向参数；当前 DIFFUSE 分支不使用它 |
| `N` | 当前表面的单位法线，要求非零 |
| 返回值 | 世界坐标中的随机方向，落在 `N` 朝向的半球 |

内部先生成两个随机数。在局部坐标中，让 `z` 均匀覆盖 `[0,1]`、方位角均匀覆盖一圈，
得到单位半球方向，再调用 `toWorld()` 转到世界坐标。

返回的只是方向，不是颜色、BRDF、PDF 或一条带起点的 `Ray`。这里使用**均匀半球采样**，
不是余弦加权采样。

### 4.3 `Material::pdf()`：查询这个方向的采样密度

```cpp
float pdf(const Vector3f& wi, const Vector3f& wo, const Vector3f& N);
```

当前 DIFFUSE 分支不使用第一个参数，只判断第二个方向 `wo`：

```text
dot(wo, N) > 0  → 返回 1/(2π)
否则           → 返回 0
```

这是 `sample()` 对应的**立体角密度**，单位可理解为 `1/sr`。
概率密度不是某条精确方向“发生的概率”，不需要总在数值上小于 1。
当前半球边界使用严格 `>0`，实现估计式时应检查 PDF 为正再做除法。

### 4.4 `Material::eval()`：计算 BRDF

```cpp
Vector3f eval(const Vector3f& wi, const Vector3f& wo, const Vector3f& N);
```

当前实现同样不使用第一个方向；第二个方向在法线正半球时返回 `Kd/π`，否则返回黑色。

**它返回的是反射能力，不是完整光照贡献。** 没有自动乘入射光、表面余弦、距离项，
也没有自动除 PDF。尽管它检查了方向正反，返回值仍没有乘 `dot(N, wo)`。

三个函数一起使用的最小示例：

```cpp
Material matte(DIFFUSE, Vector3f(0));
matte.Kd = Vector3f(0.7f);
Vector3f N(0, 1, 0);
Vector3f incoming(0, -1, 0);

Vector3f sampledDirection = matte.sample(incoming, N);
float pdfDirection = matte.pdf(incoming, sampledDirection, N);
Vector3f brdf = matte.eval(incoming, sampledDirection, N);
```

这段只演示材质查询，不是路径追踪实现。这里第一参数表示已有光线方向，第二参数传
新采样方向；漫反射实现忽略第一参数，所以不会帮你检出方向约定不一致。
以后实现 Microfacet 时，须重新核对两个方向的入射/出射约定。

| 函数 | 回答的问题 | 不负责什么 |
| --- | --- | --- |
| `sample()` | 下一次朝哪个方向走？ | 不返回反射颜色 |
| `pdf()` | 采样这个方向时密度多大？ | 不返回亮度 |
| `eval()` | 材质在这些方向之间怎样反射？ | 不返回最终光照 |

### 4.5 `toWorld()`：把局部方向转到法线所在的世界坐标

```cpp
Vector3f toWorld(const Vector3f& a, const Vector3f& N);
```

它在 `Material` 中是私有函数，外部不能直接调用，由 `sample()` 内部调用。
它用 `N` 构造两个垂直方向 `B/C`，再返回 `a.x*B + a.y*C + a.z*N`。

可以理解为：采样时把局部正 Z 轴当作“朝外”，转换后这个方向变为真实法线 `N`。
要求 `N` 是单位非零法线，否则基底和方向长度可能不正确；它没有自动验证或修复法线。

## 5. 几何体的采样：灯面上的点是怎样取出来的

源码：[Triangle.hpp](Assignment7_code/Triangle.hpp)、[BVH.cpp](Assignment7_code/BVH.cpp)。

### 5.1 `Triangle` 构造函数：预计算边、法线、面积

```cpp
Triangle(Vector3f v0, Vector3f v1, Vector3f v2, Material* m = nullptr);
```

保存顶点和材质指针，并计算：

```text
e1 = v1-v0
e2 = v2-v0
normal = normalize(cross(e1,e2))
area = |cross(e1,e2)| / 2
```

顶点顺序决定法线朝向。默认材质指针可以为空，但后面的 `hasEmit()` 会解引用它；
参与场景使用时应传有效材质。退化三角形面积为零，不能直接使用其面积采样 PDF。

### 5.2 `Triangle::Sample()`：在一个三角形上均匀取点

```cpp
void Sample(Intersection& pos, float& pdf);
```

用 `s=sqrt(random)` 和另一个随机数 `u` 生成重心权重：

```text
权重 = (1-s, s(1-u), su)
位置 = 三个顶点按权重相加
pdf = 1/triangle_area
```

三个权重非负、总和为 1，所以点落在三角形内。这里开平方是为了让采样对面积均匀，
不能把它和 BVH 的累计面积区间选择混为一谈。

输出只有 `pos.coords`、`pos.normal` 和 `pdf`；不会填写材质、对象、发光值或命中标记。

### 5.3 `getArea()`、`hasEmit()`、`getBounds()`

| 对象 | 函数作用 |
| --- | --- |
| `Triangle::getArea()` | 返回该三角形面积 |
| `Triangle::hasEmit()` | 转发到材质的 `hasEmission()` |
| `Triangle::getBounds()` | 返回包含三个顶点的轴对齐包围盒 |
| `MeshTriangle::getArea()` | 返回所有三角形面积之和 |
| `MeshTriangle::hasEmit()` | 查询整个网格共享材质是否发光 |
| `MeshTriangle::getBounds()` | 返回构造时计算好的整个网格包围盒 |

它们通过 `Object` 的虚函数接口被 BVH 和 `sampleLight()` 调用。
注意网格面积是实际三角形面积之和，不是网格包围盒的表面积。

### 5.4 `BVHAccel::getSample()` / `Sample()`：从网格的多个三角形中选一个

```cpp
void getSample(BVHBuildNode* node, float p, Intersection& pos, float& pdf);
void Sample(Intersection& pos, float& pdf);
```

每个 BVH 节点存了子树中几何体面积之和 `node->area`。`getSample()` 把它当作
累计面积区间：落在左子树面积内就往左走，否则减掉左侧面积后往右走。

到叶子时，调用具体对象的 `Sample()`，然后将返回 PDF 乘叶子面积；外层
`BVHAccel::Sample()` 再除整棵树面积。

**设计意图**是：按面积选中三角形，再在选中的三角形内部均匀取点。
若选叶过程满足该分布，最终网格面积密度就应为 `1/网格总面积`。
但当前 `Sample()` 对区间参数用了 `sqrt(random)`，分布有不一致，见第 12 节。
不要把“代码返回这个值”直接理解为“它一定是真实的采样密度”。

`getSample()` 通常由 `Sample()` 递归使用，你在路径追踪中不需要手动调用它。
两者都要求已有非空、面积为正、结构有效的 BVH。

### 5.5 `MeshTriangle::Sample()`：给采样点补上网格的发光值

```cpp
void Sample(Intersection& pos, float& pdf);
```

它先调用网格内部 `bvh->Sample(pos,pdf)`，得到点与法线，再设置
`pos.emit = m->getEmission()`。默认灯使用这个类，因此 `sampleLight()` 的调用者
可以直接从输出的 `emit` 读取灯的发光值。

完整调用关系为：

```text
Scene::sampleLight
  → 选中一个发光 MeshTriangle
  → MeshTriangle::Sample
      → BVHAccel::Sample
          → getSample 逐层选择叶子
              → Triangle::Sample：坐标、法线、面积 PDF
      → 补充 pos.emit
```

`Sample` 的首字母是大写；材质的 `sample` 是小写，前者取几何位置，后者取反弹方向。

## 6. 渲染器：如何把“单条光线的结果”变成整幅图

源码：[Renderer.cpp](Assignment7_code/Renderer.cpp)。

### 6.1 `deg2rad()`：角度转弧度

```cpp
float deg2rad(const float& deg);
```

计算 `deg*π/180`。场景的 `fov` 用角度表示，而 `tan()` 接收弧度，所以生成相机光线
之前需要转换。这个辅助函数位于 `Renderer.cpp`，不应当作其他源文件可直接使用的公共接口。

### 6.2 `Renderer::Render(const Scene&)`：逐像素采样并输出图片

```cpp
void Render(const Scene& scene);
```

这个函数已提供像素循环，但依赖尚未完成的 `castRay()`，当前不能完整验证渲染。
它的工作顺序是：

1. 创建 `width*height` 个零颜色作为 framebuffer。
2. 根据视角、宽高比计算相机视平面的范围，使用固定相机 `(278,273,-800)`。
3. 遍历像素中心 `(i+0.5,j+0.5)`，映射到屏幕坐标，再计算单位方向 `(-x,y,1)`。
4. 对同一像素调用 `castRay()` 共 `spp` 次，将每次结果除以 `spp` 再累加。
5. 更新进度，最后把 framebuffer 写入 `binary.ppm`。

```cpp
Renderer renderer;
renderer.Render(scene); // 只有四处 TODO 完成后才运行
```

这里 `spp=16`，分辨率来自 `main.cpp` 的 `Scene(784,784)`。
修改这些数值后要重新编译，没有相应的命令行参数。

两点容易误解：

- 同一像素的相机起点和方向在 16 次调用中完全相同；随机性来自你将实现的路径采样，
  框架没有在像素内抖动位置，因此这些 SPP 不能直接当作已实现的抗锯齿采样。
- `castRay()` 应返回当前路径估计的 RGB 值，不用再在里面除一次相机的 SPP；
  像素平均已由渲染器完成。

写图时每通道先 `clamp(0,1,value)`，再取 `pow(value,0.6)` 并映射到 0～255。
这是框架采用的显示映射，不是严格的 sRGB 编码。不要在路径反弹中提前做这种裁剪与映射。
结果写入**进程工作目录**；原实现没有检查 `fopen()` 是否失败，需保证目录可写。

## 7. 模型加载与 BVH 构建：主算法背后的准备工作

### 7.1 `MeshTriangle(filename, material)`：从 OBJ 创建网格

源码：[Triangle.hpp](Assignment7_code/Triangle.hpp)。

```cpp
MeshTriangle(const std::string& filename, Material* mt = new Material());
```

实际步骤为：

1. 调用 `objl::Loader::LoadFile(filename)`。
2. 断言只加载出一个 mesh，使用 `LoadedMeshes[0]`。
3. 每三个顶点构造一个 `Triangle`，共享传入的材质指针。
4. 用全部顶点的坐标最小/最大值计算整个网格包围盒。
5. 累加三角形面积，将三角形指针交给 `BVHAccel` 建树。

`objl::Loader::LoadFile()` 位于第三方 [OBJ_Loader.hpp](Assignment7_code/OBJ_Loader.hpp)，
返回加载是否成功，并填写 `LoadedMeshes` 等数据。当前网格构造函数没有检查它的布尔返回值，
因此模型路径错误可能表现为断言失败；Release 中还可能继续访问无效下标。

标准路径是 `../models/cornellbox/*.obj`，从 `build-ucrt/` 或 `build-ucrt-release/`
运行才能正确解析。构造后不要随意改变 `triangles` 的存储位置或追加导致 vector 扩容的三角形，
已有 BVH 中保存的是它们的地址。

### 7.2 `BVHAccel` 构造函数：启动递归构建并计时

源码：[BVH.hpp](Assignment7_code/BVH.hpp)、[BVH.cpp](Assignment7_code/BVH.cpp)。

```cpp
BVHAccel(std::vector<Object*> p,
         int maxPrimsInNode = 1,
         SplitMethod splitMethod = SplitMethod::NAIVE);
```

保存对象列表和配置，调用 `recursiveBuild()`，打印建树耗时。
当前代码实际执行的是中位数划分；虽然枚举里有 `SAH`，构建函数没有按它切换算法。
同样，不能靠改变 `maxPrimsInNode` 就获得多对象叶子；当前实际叶子构建逻辑仍是单对象。

### 7.3 `BVHAccel::recursiveBuild()`：按质心排序并分成左右子树

```cpp
BVHBuildNode* recursiveBuild(std::vector<Object*> objects);
```

- 1 个对象：创建叶子，保存对象、包围盒、面积。
- 2 个对象：分别递归创建两个子节点，再合并包围盒与面积。
- 更多对象：先计算质心分布包围盒，选最长轴，按该轴上的对象质心排序，从中间一分为二。

它返回子树根节点。你通常只调用 `Scene::buildBVH()`，不用自己递归调用。

本工程有**两层 BVH**：场景 BVH 的叶子是网格，网格内部 BVH 的叶子是三角形。
最后命中结果里的 `obj` 因而通常是具体 `Triangle*`，而不是整个 `MeshTriangle*`。
做灯光可见性判断时不要想当然地把它和场景级网格地址直接比较。

### 7.4 `BVHAccel::Intersect()` 和 `MeshTriangle::getIntersection()`：两层转发

```cpp
Intersection BVHAccel::Intersect(const Ray& ray) const;
Intersection MeshTriangle::getIntersection(Ray ray);
```

前者检查根节点并调用待完成的遍历；后者把求交交给网格内部 BVH。
完整预期路径是：

```text
Scene::intersect
  → 场景 BVH::Intersect
      → getIntersection【TODO】
          → MeshTriangle::getIntersection
              → 网格 BVH::Intersect
                  → getIntersection【同一个 TODO】
                      → Triangle::getIntersection【TODO】
```

它们返回 `Intersection`，不是颜色。当前这些入口存在，不代表底层求交已完成。

## 8. 向量、包围盒与通用工具速查

### 8.1 `Vector3f`：既表示几何向量，也表示 RGB

源码：[Vector.hpp](Assignment7_code/Vector.hpp)。

| 接口 | 作用/用法 | 注意事项 |
| --- | --- | --- |
| `Vector3f()` | 得到 `(0,0,0)` | 默认颜色是黑色 |
| `Vector3f(s)` | 得到 `(s,s,s)` | `Vector3f(1)` 三通道都是 1 |
| `Vector3f(x,y,z)` | 指定三个分量 | 可表示位置、方向或 RGB |
| `a+b`、`a-b`、`-a`、`a+=b` | 分量相加减 | 点减点可得到方向向量 |
| `a*s`、`s*a`、`a/s` | 标量缩放 | 除法不检查零 |
| `a*b`（两个向量） | 分量相乘 | RGB 逐通道调制；不是点乘或叉乘 |
| `v.norm()` | 向量长度 | 不是长度平方 |
| `v.normalized()` | 返回归一化副本 | 不修改 `v`，没有零长度保护 |
| `normalize(v)` | 返回单位向量；零向量原样返回 | 不修改 `v`，但零向量仍不适合作为光线方向 |
| `dotProduct(a,b)` | 点乘，返回标量 | 两个单位方向的点乘才是夹角余弦 |
| `crossProduct(a,b)` | 叉乘，返回向量 | 用于法线和面积，交换参数会反向 |
| `Vector3f::Min/Max(a,b)` | 对每个分量取最小/最大 | 用于包围盒扩展 |
| `lerp(a,b,t)` | `a*(1-t)+b*t` | 不自动限制 `t`，可用于颜色插值 |
| `operator<<` | 打印三个分量 | 适合输出调试数据 |

`Vector2f` 提供二维构造、加法和标量乘法，主要用于旧 UV 接口；它不是自动带纹理功能的类。

索引接口另有一个当前实现问题：只定义了 `double operator[](int) const`，
非 const 的 `double& operator[](int)` 只有声明。对非 const 向量写 `v[0]` 可能选中
缺少定义的重载而产生链接错误。学习时优先用 `v.x/v.y/v.z`，不要把它当作完整可写数组。

### 8.2 `Bounds3`：轴对齐包围盒工具

源码：[Bounds3.hpp](Assignment7_code/Bounds3.hpp)。以下都是已写出的几何工具；
光线测试 `Bounds3::IntersectP()` 仍然是你的 TODO。

| 接口 | 作用与主要使用位置 |
| --- | --- |
| `Bounds3()` | 建立用于逐步合并的空包围盒初始状态；不能当作已有有效尺寸的盒子 |
| `Bounds3(p)` | 用一个点初始化，最小角和最大角相同 |
| `Bounds3(p1,p2)` | 每个轴取较小/较大坐标作为边界 |
| `Diagonal()` | `pMax-pMin`，即三个轴上的跨度 |
| `maxExtent()` | 最长轴索引：0 为 X、1 为 Y、2 为 Z；用于选择 BVH 排序轴 |
| `Centroid()` | `(pMin+pMax)/2`，包围盒中心，不是三角形重心 |
| `SurfaceArea()` | `2*(dx*dy+dx*dz+dy*dz)`，盒子表面积，不是实际物体面积 |
| `Union(box1,box2)` | 返回包含两个盒子的最小 AABB，用于递归合并 |
| `Union(box,p)` | 把一个点纳入包围盒 |
| `Offset(p)` | 返回点相对最小角的坐标，非零跨度轴除以该轴跨度；不裁剪到 `[0,1]` |
| `Overlaps(b1,b2)` | 三个轴区间都重叠则为真，边界接触算重叠 |
| `Inside(p,b)` | 点是否落在盒内，包含边界 |
| `operator[](i)` | `i==0` 返回 `pMin`，其他值返回 `pMax`，不做有效下标检查 |
| `Intersect(b)` | 意图计算两个盒子的交；当前不应作为可靠的无交检测，见第 12 节 |

例子：`Union(Bounds3(Vector3f(0)), Vector3f(1,2,3))` 得到边界 `(0,0,0)` 到 `(1,2,3)`。
不要把 `Bounds3::Intersect()`（盒与盒）和 `Bounds3::IntersectP()`（盒与光线）混淆。

### 8.3 `global.hpp` 的辅助函数与常量

源码：[global.hpp](Assignment7_code/global.hpp)。

| 接口 | 作用/示例 | 使用边界 |
| --- | --- | --- |
| `clamp(lo,hi,v)` | 裁剪范围，如 `clamp(0,1,1.5f)` 得到 1 | 参数顺序是下界、上界、值；不是先传值 |
| `get_random_float()` | 返回近似 `[0,1)` 的均匀随机浮点数 | 当前每次重新创建、播种生成器，可能较慢；尾部 `[1,6]` 注释不符合代码 |
| `solveQuadratic(a,b,c,x0,x1)` | 解 `a*x²+b*x+c=0`，成功时经引用输出升序实根 | 返回 false 时不要使用输出；要求 `a!=0`，不处理线性退化 |
| `UpdateProgress(progress)` | 终端显示 70 格进度条，通常传 0～1 | 不自动限制范围；与光照计算无关 |
| `kInfinity` | float 最大有限值，用作“很远”初值 | 不是 IEEE 正无穷 |
| `EPSILON` | 当前定义在 `Renderer.cpp`，值为 `1e-5` | 是误差阈值，不会自动消除所有自相交问题 |

`solveQuadratic()` 在判别式为正时使用较稳定的求根形式，再通过根的乘积求另一根。
球体求交使用它；默认 Cornell Box 使用三角形网格，不会走球体求交。

## 9. 球体与旧反射工具：了解用途即可

### 9.1 `Sphere` 的实现

源码：[Sphere.hpp](Assignment7_code/Sphere.hpp)。默认 `main.cpp` 没有创建球体。

| 函数 | 作用与输出 |
| --- | --- |
| `Sphere(center,radius,material)` | 保存球心/半径/材质，预计算半径平方和表面积 `4πr²` |
| `intersect(ray)` | 解球面二次方程，有非负根则返回 true |
| `intersect(ray,tnear,index)` | 类似，但额外输出所选根 `tnear`；当前不写 `index` |
| `getIntersection(ray)` | 返回球体的坐标、外法线、距离、对象和材质；不设置发光字段 `emit` |
| `getSurfaceProperties(...)` | 通过 `N=normalize(P-center)` 输出法线，当前不写纹理坐标 `st` |
| `getBounds()` | 返回 `center-radius` 到 `center+radius` 的 AABB |
| `getArea()`、`hasEmit()` | 查询球面积、查询材质发光 |
| `Sample(pos,pdf)` | 产生球面上的点、法线、发光值，返回声明的面积 PDF；分布有问题，见第 12 节 |

要求半径为正，材质有效。求交选正向根的实现没有主动检查 `ray.t_min/t_max`，
也没有使用 EPSILON 排除非常近的自相交；扩展场景时需自行考虑。
`Sphere::evalDiffuseColor()` 是空返回体，不属于已经可用的函数。

### 9.2 `reflect()`、`refract()`、`fresnel()`

这些工具在 `Scene.hpp` 中是公开方法，在 `Material.hpp` 中还有一套私有实现。
默认漫反射 `sample/pdf/eval` 没有调用它们，因此函数存在不代表已经支持玻璃或镜面材质。

| 函数 | 输入/输出与意义 |
| --- | --- |
| `reflect(I,N)` | 入射传播方向 `I`、单位法线 `N` → 镜面反射方向 `I-2*dot(I,N)*N` |
| `refract(I,N,ior)` | 入射方向、法线、折射率 → Snell 定律折射方向；区分进入和离开介质，全反射时返回零向量 |
| `fresnel(I,N,ior,kr)` | 经引用输出反射比例 `kr`；全反射时为 1，非吸收介质可据此分配反射/折射能量 |

通常要求方向和法线归一化，折射率为有效正值。全反射的零向量不能继续用来创建
有效传播方向。`Material` 里的版本是私有成员，不能直接从 `castRay()` 外部调用。

### 9.3 `Light`、`AreaLight::SamplePoint()`

源码：[Light.hpp](Assignment7_code/Light.hpp)、[AreaLight.hpp](Assignment7_code/AreaLight.hpp)。

`Light(p,i)` 保存位置和强度；`AreaLight` 额外给出法线及两个边向量 `u/v`。
`SamplePoint()` 返回 `position + random_u*u + random_v*v`，只返回坐标，没有 PDF。

当前 `u=(1,0,0)`、`v=(0,0,1)`，`length=100` 没有乘进采样公式，因此不能认为
它正在采样一个 100×100 的灯。默认 Cornell Box 用的是发光网格，不走这个类。
实现本作业基础光照时使用 `Scene::sampleLight()` 即可。

## 10. 环境检查入口的作用

源码：[main.cpp](Assignment7_code/main.cpp)。这是之前环境配置时新增的辅助分支。

从对应构建目录运行：

```powershell
./RayTracing.exe --check-environment
```

它仍执行创建材质、加载六个模型、建立网格 BVH 和场景 BVH；随后检查网格非空和根节点存在，
打印网格/三角形数量并返回，不会进入 `Renderer::Render()`。

这个入口能帮助区分程序启动、模型路径等环境问题和光照算法问题。它不调用四处 TODO，
不证明求交、采样分布或图像正确，也不会生成 `binary.ppm`。不带参数才进入正常渲染路径。

## 11. 旧接口、占位代码与只有声明的函数

这部分用于避免“看到函数名就拿来用”。它们不等于要你额外实现所有旧接口。

| 接口 | 当前实际情况 | 对本作业的建议 |
| --- | --- | --- |
| `Scene::trace()` | 有遍历代码，但调用旧 `Object::intersect(ray,t,index)`，并依赖调用者初始化 `tNear` | 主路径用 `Scene::intersect()`；当前网格旧数据成员不完整，不要用它替代 BVH 路径 |
| `Triangle::intersect(ray)` | 无条件返回 true | 占位，不能用来做可见性判断 |
| `Triangle::intersect(ray,t,index)` | 无条件返回 false | 占位，不是你要补的 `getIntersection()` |
| `MeshTriangle::intersect(ray)` | 无条件返回 true | 占位 |
| `MeshTriangle::intersect(ray,t,index)` | 使用 `vertices/vertexIndex/numTriangles` 旧存储 | 当前构造函数填的是 `triangles`，没有建立这套旧存储；避免调用 |
| 全局 `rayTriangleIntersect()` | 有重心坐标求解，但不检查 `tnear>0`，会剔除反面，并可能在失败前改写部分输出 | 属于旧路径辅助函数；不要把 true 直接当作完整的正向有效命中 |
| `Triangle::getSurfaceProperties()` | 只写法线 `N`，没有写 `st` | 主路径用 `Intersection`，不要假设此接口提供纹理坐标 |
| `MeshTriangle::getSurfaceProperties()` | 使用前述旧顶点/纹理存储 | 当前默认主路径不使用 |
| `Triangle::evalDiffuseColor()` | 固定返回 `(0.5,0.5,0.5)` | 当前路径追踪材质来自 `m->eval()`，不是它 |
| `MeshTriangle::evalDiffuseColor()` | 根据 UV 生成双色棋盘格 | 是旧程序纹理示例，不会自动作用到 `Material::Kd` |
| `Material::getColorAt()` | 忽略 UV，固定返回零向量 | 占位，不是已实现的贴图采样 |
| `Sphere::evalDiffuseColor()` | 没有返回值 | 不可调用来取得有效颜色 |
| `BVHAccel::WorldBound()`、`BVHAccel::IntersectP()`、`~BVHAccel()` | 有声明，当前源码里未找到定义 | 不要直接新增调用或删除 BVH 对象，否则可能产生链接错误 |
| `Scene::HandleAreaLight()` | 有声明，当前源码里未找到定义 | 不属于当前漫反射主路径 |
| 非 const `Vector3f::operator[]` | 只有声明 | 见第 8 节，优先访问 `.x/.y/.z` |

`Object` 中的纯虚函数只是统一接口，不是已经实现的公共算法。
本作业应该补齐的四个 TODO 见第 1 节，不需要为了阅读框架把所有历史接口都补完。

## 12. 已有实现的边界：哪些不能当作通用正确工具

以下是静态源码分析，不是本次进行了修复或完成渲染验证。

### 12.1 多个发光对象：`sampleLight()` 的 PDF 缺少对象选择概率

设两个光源面积是 2 和 6。按面积选第一个光源的概率是 `2/8`，在其中均匀取点的
条件密度是 `1/2`，所以最终全局面积密度应该为 `(2/8)*(1/2)=1/8`。

当前 `sampleLight()` 直接返回所选对象的 `Sample()` 输出，没有再乘选择该对象的概率。
所以不能把它当作任意多光源场景都正确的全局 PDF 接口。默认场景只有一个发光网格，
这一层的对象选择概率是 1；它并不消除下一项的网格内部采样问题。

### 12.2 `BVHAccel::Sample()` 的累计面积参数分布不均匀

源码使用 `p=sqrt(U)*总面积`。如果要按累计面积区间均匀选位置，`p` 本身应均匀。
例如两个等面积三角形各占一半区间，当前左边被选中的概率是
`P(sqrt(U)<0.5)=0.25`，不是应有的 `0.5`。

与此同时，函数仍返回按均匀面积采样推导的 PDF，因此分布与密度不匹配，可能带来偏差。
不要因为 `Triangle::Sample()` 内部也有开平方，就认为这两处开平方的作用相同。
这一局限也存在于默认网格灯的采样链中，不保证单光源下就没有影响。

### 12.3 `Sphere::Sample()` 不是真正的均匀球面面积采样

它均匀选择极角 `phi` 和方位角 `theta`，这会在两极附近采得更密；
但返回的 PDF 仍是均匀面积密度 `1/area`。因此添加发光球时不能直接假设它无偏。
当前默认场景没有球，不需要先扩展球体才能完成 Cornell Box 基础部分。

### 12.4 `Bounds3::Intersect()` 无法可靠表示“不相交”

它先求两个盒子的区间交上下界，再调用会自动重排最小/最大坐标的双点构造函数。
如果两盒已经分离，反过来的上下界可能被重排成一个看似有效的盒子。
只需要判断盒与盒重叠时，用现有 `Overlaps()` 的逻辑更符合其用途。

### 12.5 空 BVH、原始指针和资源管理

`BVHAccel` 构造函数遇到空对象数组就返回，但没有先将 `root` 初始化为空；
所以不能假设之后的 `if (!root)` 能安全保护任意空树。
当前标准场景有非空模型；环境检查验证的是这个场景，没有测试通用空场景。

节点、材质和 BVH 的原始指针管理也不是完整的生产级资源管理。
本作业按“创建一次静态场景并渲染”的方式使用，不要据此推断可以安全任意复制、删除或反复重建。

## 13. 写作业时放在旁边的速查表

| 你现在想做什么 | 应查哪个函数/数据 |
| --- | --- |
| 求光线在场景中的最近交点 | `Scene::intersect()`，前提是求交 TODO 已完成 |
| 判断是否命中、取材质 | `hit.happened`，再使用 `hit.m` |
| 取命中点/法线 | `hit.coords`、`hit.normal` |
| 判断命中材质是否发光 | `hit.m->hasEmission()` |
| 取命中材质发光值 | `hit.m->getEmission()`；不要假设命中结果的 `emit` 已填写 |
| 在默认网格灯上取点 | `Scene::sampleLight(pos,pdfArea)`，同时留意采样局限 |
| 读取灯采样信息 | `pos.coords`、`pos.normal`、`pos.emit` |
| 随机反弹方向 | `hit.m->sample(existingDirection, hit.normal)` |
| 查询反弹方向 PDF | `hit.m->pdf(existingDirection, sampledDirection, hit.normal)` |
| 查询 BRDF | `hit.m->eval(existingDirection, sampledDirection, hit.normal)` |
| 算方向或距离 | `normalize(x-p)`、`(x-p).norm()` |
| RGB 逐通道调制 | `Vector3f * Vector3f` |
| 算余弦 | 对单位方向使用 `dotProduct()`，根据方向约定处理正反 |
| 生成下一条光线 | `Ray(origin, unitDirection)`，自相交控制由算法负责 |
| 查看单纯环境是否可用 | `RayTracing.exe --check-environment` |

阅读到这里，应能够解释三个区别：**几何采样取位置，材质采样取方向；面积 PDF 和
方向 PDF 对应不同积分；交点/BRDF/发光值都不是已经算好的最终像素颜色。**
