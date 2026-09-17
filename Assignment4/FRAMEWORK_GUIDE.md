# Assignment 4 代码框架导读

本文解释 `Assignment4_code/main.cpp` 中**已经完成的代码**如何协作，并说明两个待实现函数在框架中的位置。本文不会给出 de Casteljau 算法的完整实现。

## 1. 程序整体流程

程序的运行顺序可以概括为：

```text
main 创建黑色画布和窗口
  -> 注册 mouse_handler
  -> 不断显示窗口并等待鼠标输入
  -> mouse_handler 收集 4 个控制点
  -> main 画出白色控制点
  -> 收集完 4 点后调用 naive_bezier
  -> 显示并保存曲线图片
  -> 等待按键后退出
```

当前代码默认调用已经实现的 `naive_bezier()`，因此即使两个 TODO 还没有完成，也可以运行程序并看到一条红色三次 Bézier 曲线。

## 2. 全局控制点容器 `control_points`

```cpp
std::vector<cv::Point2f> control_points;
```

它不是函数，而是理解整个程序的入口：

- `std::vector` 表示长度可以变化的动态数组；
- `cv::Point2f` 表示由两个 `float` 构成的二维点 `(x, y)`；
- `mouse_handler()` 向其中加入用户点击的位置；
- `main()` 读取它并绘制白色控制点；
- `naive_bezier()` 读取其中的 4 个点来计算曲线。

这里使用全局变量，是因为 OpenCV 会在鼠标事件发生时自行调用回调函数。全局容器让回调函数和主循环能够访问同一批控制点。更大型的程序通常会通过 `userdata` 传递状态，以减少全局变量，但本作业的写法更容易入门。

## 3. `mouse_handler()`：接收鼠标点击

### 函数签名

```cpp
void mouse_handler(int event, int x, int y, int flags, void *userdata)
```

这是一个 OpenCV 鼠标回调函数。你不需要自己直接调用它，而是通过下面的语句把它注册给窗口：

```cpp
cv::setMouseCallback("Bezier Curve", mouse_handler, nullptr);
```

注册后，窗口内发生鼠标事件时，OpenCV 会自动调用 `mouse_handler()`。

### 参数含义

| 参数 | 作用 | 本作业中的使用情况 |
| --- | --- | --- |
| `event` | 鼠标事件类型，例如按下左键、移动鼠标 | 判断是否为 `cv::EVENT_LBUTTONDOWN` |
| `x` | 鼠标在窗口中的横坐标 | 作为控制点的 x 坐标 |
| `y` | 鼠标在窗口中的纵坐标 | 作为控制点的 y 坐标 |
| `flags` | Ctrl、Shift、鼠标按钮状态等附加信息 | 当前未使用 |
| `userdata` | 注册回调时传入的自定义数据指针 | 当前传入 `nullptr`，未使用 |

### 函数做了什么

```cpp
if (event == cv::EVENT_LBUTTONDOWN && control_points.size() < 4)
```

只有同时满足以下条件才会记录控制点：

1. 用户按下鼠标左键；
2. 当前控制点数量少于 4。

```cpp
control_points.emplace_back(x, y);
```

这行直接在 `vector` 尾部构造一个 `cv::Point2f(x, y)`。收集到 4 个点以后，后续点击会被忽略。

### 如何观察它

运行程序后依次点击窗口。每次有效点击都会在终端打印坐标，例如：

```text
Left button of the mouse is clicked - position (120, 350)
```

如果断点调试，可以在 `control_points.emplace_back(x, y)` 处设置断点，观察 `event`、`x`、`y` 和 `control_points.size()`。

## 4. `naive_bezier()`：用显式公式绘制三次 Bézier 曲线

### 函数签名

```cpp
void naive_bezier(const std::vector<cv::Point2f> &points, cv::Mat &window)
```

参数含义：

- `points`：控制点数组，以常量引用传入。函数可以读取它，但不能修改它；引用传递避免复制整个数组。
- `window`：OpenCV 图像，以非常量引用传入，因此函数可以直接修改画布中的像素。
- 返回类型是 `void`，因为结果直接写入 `window`，不需要另外返回。

### 为什么它只能处理 4 个控制点

函数直接取出：

```cpp
auto &p_0 = points[0];
auto &p_1 = points[1];
auto &p_2 = points[2];
auto &p_3 = points[3];
```

随后使用三次 Bézier 曲线公式：

```text
B(t) = (1-t)^3 P0
     + 3t(1-t)^2 P1
     + 3t^2(1-t) P2
     + t^3 P3
```

它是“naive”的原因不是结果错误，而是公式被写死为四控制点对应的三次曲线。若改成 3 个、5 个或更多控制点，就要重新推导并改写公式。de Casteljau 算法则可以用同一种过程处理任意数量的控制点。

### 参数 `t` 表示什么

```cpp
for (double t = 0.0; t <= 1.0; t += 0.001)
```

`t` 是曲线参数：

- `t = 0` 时，曲线点是 `P0`；
- `t = 1` 时，曲线点是 `P3`；
- 中间值产生曲线中间的点；
- 步长 `0.001` 大约会采样 1001 次，足以让离散像素看起来像连续曲线。

需要注意：`t` 不是“已经走过的实际弧长比例”。相同的 `t` 步长不保证相邻采样点在屏幕上的距离完全相同。

### 如何修改像素

```cpp
window.at<cv::Vec3b>(point.y, point.x)[2] = 255;
```

这行包含几个重要细节：

- 图像访问顺序是 `window.at(row, column)`，因此写成 `(y, x)`，不是 `(x, y)`；
- `cv::Vec3b` 表示一个包含 3 个 8 位无符号整数的像素；
- 索引 `[2]` 修改第 3 个颜色通道；
- 当前画布按 OpenCV 的三通道方式存储，这里将该通道设为 255，所以曲线显示为红色。

如果坐标在图像范围外，直接使用 `at()` 可能触发错误或非法访问。本作业的控制点来自 700×700 窗口内部，而且 Bézier 曲线位于控制点的凸包内，所以正常点击时曲线也会留在画布范围内。

### 它在作业中的用途

`naive_bezier()` 除了提供一条可见曲线，还充当你实现 de Casteljau 算法后的参考答案：

- 保留它绘制红色曲线；
- 让自己的 `bezier()` 绘制绿色曲线；
- 两条曲线位置一致时，颜色会叠加并大致显示为黄色。

这种方法只能证明大量采样点看起来重合，不是严格的自动化测试，但非常适合本作业的直观检查。

## 5. `main()`：组织窗口、输入、绘制和退出

`main()` 是程序入口，负责把所有部件连接起来。

### 5.1 创建画布

```cpp
cv::Mat window = cv::Mat(700, 700, CV_8UC3, cv::Scalar(0));
```

它创建一张：

- 高 700、宽 700 的图像；
- 每个通道是 8 位无符号整数；
- 每个像素有 3 个颜色通道；
- 所有通道初值为 0，因此背景是黑色。

```cpp
cv::cvtColor(window, window, cv::COLOR_BGR2RGB);
```

这行对当前纯黑图像没有可见影响，因为黑色的三个通道都是 0。它表达了作者希望进行 BGR/RGB 通道转换的意图；后续分析颜色时，仍应结合 OpenCV 的实际像素存储与显示行为，不要仅凭变量名猜测通道顺序。

### 5.2 创建窗口并注册回调

```cpp
cv::namedWindow("Bezier Curve", cv::WINDOW_AUTOSIZE);
cv::setMouseCallback("Bezier Curve", mouse_handler, nullptr);
```

第一行创建标题为 `Bezier Curve` 的窗口，第二行把鼠标事件交给 `mouse_handler()`。

`cv::WINDOW_AUTOSIZE` 表示窗口尺寸由图像尺寸决定，用户不能任意拉伸图像显示区域。

### 5.3 主事件循环

```cpp
int key = -1;
while (key != 27)
```

ASCII 码 27 是 Esc。理论上，控制点尚未收集完时按 Esc 可以结束循环。

每次循环都会重新绘制已经收集的控制点：

```cpp
for (auto &point : control_points)
{
    cv::circle(window, point, 3, {255, 255, 255}, 3);
}
```

这里在每个控制点位置绘制白色圆点：

- 圆心为 `point`；
- 半径为 3；
- 颜色三个通道均为 255，因此是白色；
- 线宽为 3。

由于画布没有在每一帧清空，已经画过的内容会保留下来。本作业的控制点和曲线都是静态的，因此这种做法足够使用。

### 5.4 收集完 4 个点后绘制曲线

```cpp
if (control_points.size() == 4)
{
    naive_bezier(control_points, window);
    // bezier(control_points, window);
    ...
}
```

当前只启用了公式版本。完成 TODO 后，可以按作业说明取消 `bezier()` 前面的注释，并根据验证需要决定是否同时保留 `naive_bezier()`。

### 5.5 显示、保存和等待

```cpp
cv::imshow("Bezier Curve", window);
cv::imwrite("my_bezier_curve.png", window);
key = cv::waitKey(0);
```

- `imshow()` 把当前 `window` 显示出来；
- `imwrite()` 将图像保存到程序的**当前工作目录**；
- `waitKey(0)` 无限等待键盘输入；
- 按键后执行 `return 0`，程序结束。

在本项目提供的 VS Code 启动配置中，工作目录是 `Assignment4_code/build-ucrt`，因此生成的图片通常位于：

```text
Assignment4_code/build-ucrt/my_bezier_curve.png
```

控制点还不到 4 个时，循环末尾调用的是：

```cpp
key = cv::waitKey(20);
```

它最多等待约 20 毫秒，同时让 OpenCV 处理窗口和鼠标事件。没有这类事件处理调用，窗口可能无法刷新或响应点击。

## 6. 两个 TODO 函数在调用链中的角色

这两个函数尚未完成，因此本节只解释接口，不给出实现。

### `recursive_bezier(control_points, t)`

它应该对一个固定的 `t` 计算**一个**曲线点：

```text
多个控制点 + 一个 t -> 曲线上的一个 Point2f
```

它不负责遍历所有 `t`，也不负责修改图像。这样可以把“数学计算”和“绘制”分开，便于理解和调试。

### `bezier(control_points, window)`

它负责遍历 `[0, 1]` 内的许多 `t`，每次调用 `recursive_bezier()`，再把返回点写入 `window`：

```text
遍历 t
  -> recursive_bezier 计算一个点
  -> 将该点绘制到 window
```

可以把两者理解为：

- `recursive_bezier()` 回答“曲线在这个 `t` 的位置在哪里？”；
- `bezier()` 回答“怎样用许多离散点把整条曲线显示出来？”

## 7. 如何运行现有框架

### VS Code

1. 用 VS Code 打开 `GAMES101_Homework` 文件夹。
2. 打开“运行和调试”。
3. 选择“Assignment4：交互绘制 Bezier 曲线”。
4. 启动调试。
5. 在黑色窗口内依次点击 4 个位置。
6. 观察白色控制点和红色曲线；按任意键退出。

### PowerShell

在 `Assignment4_code` 中运行：

```powershell
C:\msys64\ucrt64\bin\cmake.exe --preset ucrt-debug
C:\msys64\ucrt64\bin\cmake.exe --build --preset ucrt-debug
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
.\build-ucrt\BezierCurve.exe
```

## 8. 建议的阅读和调试顺序

1. 从 `main()` 开始，先理解窗口、循环和 4 点触发条件。
2. 在 `mouse_handler()` 中设置断点，观察点击如何进入 `control_points`。
3. 在 `naive_bezier()` 循环中观察 `t = 0`、`0.5`、`1` 时的 `point`。
4. 手算一组简单控制点，确认曲线首点等于 `P0`、末点等于 `P3`。
5. 最后再独立设计 `recursive_bezier()`，避免一开始同时处理递归、采样和像素绘制。

## 9. 容易混淆的地方

- `window.at()` 接受 `(y, x)`，而鼠标和点通常写作 `(x, y)`。
- `naive_bezier()` 是正确的三次 Bézier 实现，只是缺乏通用性。
- `t` 是参数，不一定是曲线弧长百分比。
- `waitKey()` 不只是读取按键，也承担 OpenCV 窗口事件处理。
- `imwrite()` 的相对路径以程序运行时的工作目录为基准。
- `recursive_bezier()` 应计算一个点，`bezier()` 才负责整条曲线的采样与绘制。
- 当前框架限制鼠标只能录入 4 个点，但作业希望 de Casteljau 核心算法能够处理任意数量的控制点。

