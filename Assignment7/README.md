# Assignment7：路径追踪学习与提交入口

当前提交说明见 [Assignment7_code/README.md](Assignment7_code/README.md)，
正式图片在 [Assignment7_code/images/](Assignment7_code/images/)。
基础路径追踪与求交迁移已实现，本地结果包含 784×784、16 SPP 主图（作者记录 59 秒）
及 32 SPP 对比图；多线程和 Microfacet 未实现，课程虚拟机仍待验证。

## 提交材料

- [提交检查清单](提交检查清单.md)：已核验事项、未验证边界和打包说明。
- [打包脚本](prepare_submission.ps1)：白名单复制源码、模型、README 和图片，生成独立 ZIP。
- 运行方式（在本目录）：`./prepare_submission.ps1`。
- 打包到仓库被忽略的 `tmp/assignment7-submission/`，不会删除或改写现有源码、构建和图片。
- 上传前按课程要求将 ZIP 改名为 `姓名_Homework7.zip`。

## 学习过程文档

以下文档保留了从初始框架到最终调试的过程，其中的旧 TODO、默认参数和检查结论
应按当时快照理解；**当前实现与提交状态以代码目录 README 为准**。

- [作业要求与学习路线](Assignment7_作业要求与学习路线.md)
- [环境搭建与运行](Assignment7_环境搭建与运行.md)
- [框架函数讲解](Assignment7_框架函数讲解.md)
- [castRay 实现任务拆解](Assignment7_castRay_实现任务拆解.md)
- [castRay 检查、优化和条纹诊断](Assignment7_castRay_检查与优化建议.md)

学习笔记、`review_assets/`、原始课程 PDF 和本地构建目录不放入课程提交包。
