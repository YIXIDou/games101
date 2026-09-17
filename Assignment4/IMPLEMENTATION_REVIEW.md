# Assignment 4 实现评审与改进建议

评审对象：

- `recursive_bezier(const std::vector<cv::Point2f>&, float)`
- `bezier(const std::vector<cv::Point2f>&, cv::Mat&)`

## 1. 结论

在当前程序的正常调用路径下，这两个函数没有发现功能性错误：

- `main()` 收集到 4 个控制点后才调用 `bezier()`；
- `bezier()` 使用整数循环生成 `t`，保证 `t` 始终位于 `[0, 1]`；
- 采样包含 `t = 0` 和 `t = 1`；
- `recursive_bezier()` 正确执行 de Casteljau 的逐层线性插值；
- 得到的曲线点以绿色写入画布；
- 当前工程已使用 MSYS2 UCRT64、OpenCV 5.0.0 成功编译和链接。

此前发生的 `window.at()` 越界问题已经解决。使用整数采样编号计算 `t`，避免了反复累加 `0.001f` 无法准确到达 1、进而无限越界外插的问题。

以下内容属于健壮性、可读性、效率或作业表达方面的改进建议，不代表当前实现无法正常完成必做部分。

## 2. `recursive_bezier()` 评审

当前实现：

```cpp
cv::Point2f recursive_bezier(const std::vector<cv::Point2f> &control_points, float t)
{
    std::vector<cv::Point2f> points = control_points;
    for (size_t step = 1; step < points.size(); step++)
    {
        for (size_t i = 0; i < points.size() - step; i++)
        {
            points[i] = (1.0f - t) * points[i] + t * points[i + 1];
        }
    }

    return points[0];
}
```

### 2.1 正确的部分

对于 `n` 个控制点，每一层都会对相邻点执行：

```text
P_i = (1-t) P_i + t P_(i+1)
```

第一轮产生 `n-1` 个有效点，下一轮产生 `n-2` 个，最终结果保存在 `points[0]`。这与 de Casteljau 算法的数学过程一致。

该实现并没有写死 4 个控制点。只要输入数组非空，它也可以计算 1 个、2 个或更多控制点对应的结果。

### 2.2 建议一：明确非空输入前提

当前程序总是传入 4 个点，所以不会出错。但是如果以后单独调用：

```cpp
recursive_bezier({}, t);
```

最终的 `points[0]` 会访问空数组，产生未定义行为。

可以在函数开头写明或检查前提：

```cpp
CV_Assert(!control_points.empty());
```

这属于健壮性改进，不是当前主程序中的实际错误。

### 2.3 建议二：根据学习目标决定是否真正使用递归

函数名叫 `recursive_bezier`，课程注释也提到了 recursive Bezier algorithm，但当前实现实际上是迭代版本。

迭代实现的数学结果是正确的，而且避免了递归调用开销；如果评分只检查曲线结果，它通常没有问题。但如果希望严格练习递归思想，可以改为：

1. 控制点只有一个时直接返回；
2. 生成长度少一的新控制点数组；
3. 使用新数组和相同的 `t` 再次调用 `recursive_bezier()`。

是否改成递归取决于你的学习目标。不要为了函数名字而认为当前迭代算法在数学上是错的。

### 2.4 建议三：使用更能表达含义的循环变量名

`step` 实际表示 de Casteljau 的“插值层数”。可以考虑改成：

```cpp
for (size_t level = 1; level < points.size(); ++level)
```

这样更容易将代码与算法的三角形插值结构对应起来。

### 2.5 建议四：理解复制的必要性与成本

```cpp
std::vector<cv::Point2f> points = control_points;
```

这份复制是合理的，因为后续计算会原地覆盖中间结果，同时不应破坏原始控制点。

它也意味着每个采样点都会复制并分配一次数组。当前只有 4 个控制点和 1001 次采样，开销非常小，没有必要为作业过早优化。若以后处理大量控制点或曲线，可以复用预分配的临时数组。

## 3. `bezier()` 评审

当前实现：

```cpp
void bezier(const std::vector<cv::Point2f> &control_points, cv::Mat &window)
{
    constexpr int sample_count = 1000;

    for (int i = 0; i <= sample_count; ++i)
    {
        float t = static_cast<float>(i) / sample_count;
        cv::Point2f point = recursive_bezier(control_points, t);
        int x = static_cast<int>(point.x);
        int y = static_cast<int>(point.y);

        CV_Assert(x >= 0 && x < window.cols);
        CV_Assert(y >= 0 && y < window.rows);

        window.at<cv::Vec3b>(y, x)[1] = 255;
    }
}
```

### 3.1 正确的部分

整数循环是当前实现中很好的设计：

```cpp
float t = static_cast<float>(i) / sample_count;
```

它具有以下优点：

- 不依赖浮点数反复累加；
- 循环次数确定；
- `t` 不会超过 1；
- `i = 0` 时得到 `t = 0`；
- `i = sample_count` 时得到 `t = 1`。

访问像素时使用 `(y, x)`，顺序正确；颜色通道 `[1]` 被设为 255，因此绘制的是绿色曲线。

### 3.2 建议一：保留断言用于发现错误，但理解它不是裁剪

当前边界断言可以尽早暴露非法坐标：

```cpp
CV_Assert(x >= 0 && x < window.cols);
CV_Assert(y >= 0 && y < window.rows);
```

在当前程序中，控制点来自窗口内部且 `t∈[0,1]`，Bézier 曲线应位于控制点的凸包中，所以断言正常情况下不会失败。

断言适合调试，因为它能暴露算法或输入问题。不要直接用 `std::clamp()` 掩盖坐标错误，否则错误结果可能被强行画到图像边缘。如果以后允许外部输入控制点，可以显式判断坐标是否合法，并根据程序需求选择跳过或裁剪。

### 3.3 建议二：像素取整方式存在轻微方向偏差

```cpp
int x = static_cast<int>(point.x);
int y = static_cast<int>(point.y);
```

对正数而言，`static_cast<int>` 会直接截去小数部分，例如 `10.9` 变成 `10`。因此采样点会略微偏向左上方像素。

如果只完成基础绘制，这不是功能错误。若希望选择距离曲线点最近的像素，可以使用：

```cpp
int x = cvRound(point.x);
int y = cvRound(point.y);
```

如果准备完成奖励反走样，则不应只选择一个整数像素，而应根据曲线点到周围像素中心的距离给多个相邻像素分配颜色权重。

### 3.4 建议三：给采样数量补充含义说明

`sample_count = 1000` 对 700×700 画布通常足够，但它仍然是一个固定经验值。可以添加简短注释，说明它表示 `[0,1]` 上的区间数量，因此会产生 1001 个采样点。

更进一步可以让采样密度与控制点距离或曲线屏幕尺寸相关，但这超出了本作业必做范围。

### 3.5 建议四：删除已经不再使用的代码

改成整数采样之后，下面的辅助函数已不再使用：

```cpp
bool isZero(float value, float epsilon = 1e-9f)
```

头文件 `<chrono>` 当前也没有被使用。删除未使用的函数和头文件可以减少读者疑问，并让最终提交更整洁。

这只是清理建议，不影响程序正确性。

## 4. 建议的验证方法

### 4.1 与公式版本重合对照

临时同时调用：

```cpp
naive_bezier(control_points, window);
bezier(control_points, window);
```

公式版本绘制红色，de Casteljau 版本绘制绿色。两者重合处应大致呈黄色。由于两种实现都需要把浮点坐标映射到整数像素，个别边缘像素的差异不一定意味着数学算法错误。

### 4.2 检查端点

应满足：

```text
recursive_bezier(control_points, 0) == control_points.front()
recursive_bezier(control_points, 1) == control_points.back()
```

这两项是最容易检查、也最重要的性质。

### 4.3 检查两个控制点

如果输入只有 `A` 和 `B`，结果应该退化为普通线性插值：

```text
t = 0.0 -> A
t = 0.5 -> (A+B)/2
t = 1.0 -> B
```

该测试可以验证逐层插值的基本方向是否写反。

### 4.4 检查共线控制点

把所有控制点放在同一直线上，输出曲线也必须完全落在这条直线上。这能快速发现插值权重或索引错误。

### 4.5 检查任意数量控制点

虽然当前鼠标回调只收集 4 个点，但可以单独用 1、2、3、5 个控制点调用 `recursive_bezier()`，验证核心算法没有写死为三次曲线。

## 5. 改进优先级

建议按照下面的顺序处理：

1. **建议完成**：给 `recursive_bezier()` 增加非空输入断言。
2. **建议完成**：删除未使用的 `isZero()` 和 `<chrono>`。
3. **建议验证**：同时绘制 naive 与 de Casteljau 版本，检查重合情况。
4. **按学习目标选择**：将迭代实现改写为真正的递归实现。
5. **视觉质量改进**：使用最近像素取整，或进一步完成奖励反走样。
6. **暂不需要**：复用临时数组、自适应采样等性能优化。

当前代码已经达到继续进行结果验证、截图和编写提交 README 的阶段。改进时建议一次只改一项，并在每次修改后重新构建和运行，避免把算法改动与显示改动混在一起。

