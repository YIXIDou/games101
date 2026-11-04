# Assignment 6 BVH 遍历检查与改进建议

本文针对当前 `BVHAccel::getIntersection` 实现进行检查。结论是：在当前 Assignment 6 的节点构建方式和 `Intersection` 约定下，函数没有发现功能错误，已经满足普通 BVH 遍历的必做要求。

本文只记录验证结果和进一步改进空间，不修改现有算法。

## 1. 当前实现完成了什么

当前函数的执行顺序是：

1. 空节点返回默认未命中结果；
2. 根据光线方向构造 X、Y、Z 三个正方向标志；
3. 调用当前节点的 `Bounds3::IntersectP`；
4. 包围盒未命中时直接返回，剪掉整个子树；
5. 叶节点调用实际对象的 `getIntersection`；
6. 内部节点递归访问左右子树；
7. 只有一侧命中时返回该侧；
8. 两侧都命中时比较 `distance`，返回较近结果。

这个顺序是正确的。尤其是包围盒检测位于叶节点求交和子树递归之前，能够真正减少三角形求交次数。

## 2. 验证结果

### 2.1 构建验证

使用 `ucrt-debug` preset 构建成功，`RayTracing.exe` 链接成功。

### 2.2 小型 BVH 定向测试

使用带调用计数器的假对象构造两叶节点 BVH，测试结果如下：

| 场景 | 结果 |
| --- | --- |
| 左右子树都命中，选择较近结果 | 通过 |
| 只有右子树命中 | 通过 |
| 左右子树都未命中 | 通过 |
| 根节点包围盒未命中，不访问叶对象 | 通过 |
| 负方向光线 | 通过 |
| 从根包围盒内部出发，并剪掉位于后方的子节点 | 通过 |
| 叶节点包围盒未命中，不调用物体求交 | 通过 |
| 传入空节点 | 通过 |

包围盒未命中时，叶对象调用次数为 0，证明剪枝确实发生，而不是只获得了正确结果但仍遍历全部叶节点。

### 2.3 完整兔子场景

从 `Assignment6_code/build-ucrt` 运行程序：

- 进程正常退出，退出码为 0；
- 两层 BVH 均成功构建；
- 1280×960 渲染约 2 秒完成；
- 成功生成 `binary.ppm`；
- 输出图像包含完整兔子模型、正常背景和明暗关系；
- 没有出现全背景、全黑、模型缺块或明显的错误遮挡。

这些结果说明当前遍历函数不仅通过独立测试，也能接通实际的：

```text
Scene BVH
  → MeshTriangle BVH
    → Triangle::getIntersection
      → Scene::castRay
```

## 3. 建议一：删除未使用变量

当前函数中还保留了：

```cpp
double distance = std::numeric_limits<double>::max();
```

后续逻辑已经改成直接检查 `happened` 并比较左右结果的 `distance`，所以这个变量不再使用。

开启 `-Wall -Wextra` 后编译器会报告：

```text
warning: unused variable 'distance'
```

建议删除该变量。这不会改变行为，只能减少干扰并保持代码与当前算法一致。

优先级：低，但建议立即清理。

## 4. 建议二：清理已经完成的 TODO

函数开头仍保留：

```cpp
// TODO Traverse the BVH to find intersection
```

遍历已经完成，继续保留 TODO 会让后续复查者误以为函数仍未实现。

可以改为描述算法职责的注释，例如：

```cpp
// Prune by node bounds, then return the nearest leaf intersection.
```

`Bounds3::IntersectP` 中也存在已经完成但仍保留的 TODO 和大段旧实现注释，可以在提交前统一清理。

优先级：低，属于可读性改进。

## 5. 建议三：直接初始化左右结果

当前写法：

```cpp
Intersection leftIntersection, rightIntersection;
leftIntersection = getIntersection(node->left, ray);
rightIntersection = getIntersection(node->right, ray);
```

可以简化为：

```cpp
Intersection leftIntersection = getIntersection(node->left, ray);
Intersection rightIntersection = getIntersection(node->right, ray);
```

这样可以减少“先默认构造、再赋值”的步骤，也让变量一出现就具有最终语义。

现代编译器通常会优化掉不必要复制，所以性能差异很小，主要收益是表达更清楚。

优先级：低。

## 6. 建议四：方向符号只计算一次

当前每进入一个 BVH 节点都会计算：

```cpp
ray.direction.x > 0
ray.direction.y > 0
ray.direction.z > 0
```

一次只有三个比较，当前作业规模下影响很小。但一条光线会访问多个节点，因此可以在 `BVHAccel::Intersect` 中只计算一次，再传给内部递归辅助函数。

可以保留课程要求的公共接口：

```cpp
Intersection BVHAccel::Intersect(const Ray& ray) const;
```

另外增加私有辅助函数，例如概念上：

```cpp
Intersection getIntersectionInternal(
    BVHBuildNode* node,
    const Ray& ray,
    const std::array<int, 3>& dirIsPositive) const;
```

`ray.direction_inv` 已经在 `Ray` 构造时计算，不需要再次预计算。

优先级：低到中。只有性能分析证明方向计算占比明显时才值得改动。

## 7. 建议五：近子节点优先遍历

当前内部节点总是：

```text
先遍历左子树
再遍历右子树
最后比较距离
```

更高效的方法是：

1. 分别求左右子包围盒的进入参数 `tEnter`；
2. 先遍历 `tEnter` 较小的子树；
3. 得到当前最近交点距离；
4. 如果另一子树的 `tEnter` 已经大于当前最近距离，就不访问另一子树。

当前 `Bounds3::IntersectP` 只返回布尔值，没有返回 `tEnter`，所以还不能直接进行这种优化。可以额外设计内部包围盒函数，输出进入和离开参数：

```cpp
bool IntersectP(
    const Ray& ray,
    const Vector3f& invDir,
    const std::array<int, 3>& dirIsPositive,
    double& tEnter,
    double& tExit) const;
```

这项优化能减少“两个子包围盒都相交，但较远子树不可能产生更近结果”时的遍历工作。

优先级：中，属于普通 BVH 遍历的重要性能优化，但不是 Assignment 6 必做要求。

## 8. 建议六：利用 `ray.t_max` 传播当前最近距离

另一种剪枝思路是：找到一个交点后，把当前最近距离作为后续包围盒测试的最大有效参数。

概念上：

```text
第一次命中距离 = closest
后续只关心 [ray.t_min, closest]
```

如果另一个节点的包围盒只在 `closest` 之后才被光线命中，就可以直接剪掉。

当前 `Ray` 由值传递到 `Triangle::getIntersection`，而 `getIntersection` 的光线参数是 const 引用，所以若要传播动态 `t_max`，需要设计独立的“当前最近距离”参数，或者为子遍历创建限制范围的 Ray 副本。

这项改进需要谨慎设计，避免反射/阴影光线之间错误共享状态。

优先级：中到高，适合完成基础作业后作为实验。

## 9. 建议七：迭代遍历替代递归

可以使用显式栈保存待访问节点：

```cpp
std::vector<BVHBuildNode*> stack;
```

潜在收益：

- 减少递归函数调用开销；
- 避免极深树导致调用栈问题；
- 更容易控制近节点优先顺序；
- 更方便统计访问节点数。

当前 `recursiveBuild` 使用中位数划分，生成的树通常比较平衡，递归深度约为 `O(log N)`，因此现有递归版本足够稳定。

优先级：低，不建议在提交前为此增加复杂度。

## 10. 建议八：增加遍历统计

为了判断优化是否真正有效，可以临时统计：

- 每条光线访问的 BVH 节点数；
- 包围盒测试次数；
- 叶节点访问次数；
- 三角形求交次数；
- 因包围盒未命中而剪枝的节点数。

只比较总渲染时间容易受到机器负载、文件写入和编译模式影响。节点访问数和三角形求交次数更能直接说明算法改进是否有效。

统计代码建议只用于实验，不必保留在最终提交版本，或者通过编译开关控制。

优先级：中，尤其适合将来比较普通 BVH 与 SAH。

## 11. 相关的健壮性改进

以下内容不属于当前 `getIntersection` 的功能错误，但与 BVH 生命周期有关。

### 11.1 初始化空树的 `root`

构造函数在 `primitives.empty()` 时会提前返回，但当前初始化列表没有显式设置：

```cpp
root = nullptr;
```

当前兔子场景一定包含三角形，因此没有触发。为了让空 BVH 的 `Intersect` 可靠工作，建议在构造函数初始化列表中把 `root` 初始化为 `nullptr`。

### 11.2 实现析构或使用智能指针

`recursiveBuild` 使用 `new BVHBuildNode` 创建节点，但类中声明的析构函数没有在当前源码中实现。当前程序生命周期较短，操作系统最终会回收内存，但从 C++ 资源管理角度仍应释放节点。

可选方案：

- 析构函数递归删除左右子树；
- 把 `left/right/root` 改为 `std::unique_ptr`；
- 明确 BVH 对物体指针只观察、不拥有，而对节点内存负责。

这属于框架工程质量改进，不是本次普通 BVH 查找评分点。

## 12. 建议优先级

提交前建议完成：

1. 删除未使用的 `distance`；
2. 清理已经完成的 TODO 和旧注释；
3. 显式初始化空树 `root`；
4. 再做一次全新配置、编译和渲染。

完成作业后再考虑：

1. 方向符号只计算一次；
2. 近子节点优先遍历；
3. 用当前最近距离进一步裁剪；
4. 添加遍历统计；
5. 迭代遍历；
6. SAH 构建与普通 BVH 性能比较。

当前版本已经具备正确的普通 BVH 遍历功能。后续优化应以可测量的节点访问次数、三角形求交次数或稳定的渲染耗时为依据，避免为了减少几行代码而增加不必要的复杂度。
