# 使用样例 (examples)

本目录为**使用样例**，用于演示如何在应用中使用 ProtoActor C++，不作为测试用例。

| 样例 | 说明 |
|------|------|
| **hello_world** | 最小示例：创建 ActorSystem、Spawn 一个 Actor、发送一条消息 |
| **supervision_example** | 监督与错误处理示例 |

构建后运行（在 `build` 目录下）：

```bash
./bin/hello_world
./bin/supervision_example
```

测试用例请参见 [tests/](../tests/) 目录。
