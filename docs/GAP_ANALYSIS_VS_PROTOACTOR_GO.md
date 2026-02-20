# ProtoActor-C++ 与 ProtoActor-Go 功能差异分析

本文档结合 `docs/` 描述与代码实现，对比本仓库与 [ProtoActor-Go](https://github.com/asynkron/protoactor-go) 的功能差异，给出**实际完成度**与**主要缺口**。

**当前版本约定**：**远程通信、集群（跨节点）、虚拟 Actor (Grains)** 在项目文档中**统一标注为暂不支持**；本地核心 Actor、路由、持久化（内存）等为支持状态。

---

## 一、文档声称 vs 代码实际

### 1.1 文档中的完成度（COMPARISON_AND_MIGRATION.md / README）

| 模块           | 文档完成度 | 说明 |
|----------------|------------|------|
| Actor/PID/Context/Props | 100% | 与实现一致 |
| 消息传递 / 生命周期 / 监督 / 行为 / 中间件 | 100% | 与实现一致 |
| gRPC 远程通信 | 100% | **高估**：见下节 |
| Protobuf/JSON 序列化 | 100% | **高估**：占位实现 |
| 集群成员 / Gossip / Pub/Sub | 100% | **部分高估**：Gossip 未真正发消息 |
| 虚拟 Actor (Grains) | 90% | 依赖未实现的 SpawnNamed |
| 身份查找 / PID 缓存 | 100% | 缓存与查找逻辑有，但依赖 SpawnNamed |

### 1.2 总体结论

- **本地单机**（Actor、消息、监督、路由、持久化内存实现、行为、中间件）：与文档一致，可视为 **95%+**，与 ProtoActor-Go 在“单进程内”能力上基本对齐。
- **远程与集群**（跨节点通信、Grains、身份查找、Gossip 实际传播）：文档标 100% 的项在代码中多为**骨架/占位**，实际完成度明显低于文档，**跨节点与集群功能是当前与 Go 版的主要差异**。

---

## 二、分模块差异（按代码实际情况）

### 2.1 核心 Actor（与 Go 差异小）

| 功能         | 实现情况 | 说明 |
|--------------|----------|------|
| Actor / PID / Context / Props / ActorSystem | ✅ 完整 | API 与 Go 对应 |
| Send / Request / RequestFuture | ✅ 完整 | 含信封与 Header |
| 生命周期 (Started/Stopping/Stopped/Restarting) | ✅ 完整 | |
| 监督 (OneForOne / AllForOne / Restarting) | ✅ 完整 | |
| 行为 (Become/Unbecome) | ✅ 完整 | |
| 中间件 (Receiver/Sender/Spawn/ContextDecorator) | ✅ 完整 | |
| EventStream | ✅ 完整 | |
| Future / 超时 | ✅ 完整 | |

**结论**：本地核心与 ProtoActor-Go 功能对齐，差异主要在语言/风格（如 MessageAs\<T\>、shared_ptr），无大块缺失。

---

### 2.2 路由（与 Go 差异小）

| 功能         | 实现情况 | 说明 |
|--------------|----------|------|
| 广播 / 轮询 / 随机 / 一致性哈希 | ✅ 完整 | `router.cpp` 等有完整实现 |

**结论**：路由与 Go 版一致，无缺口。

---

### 2.3 持久化（与 Go 差异小，依赖可选）

| 功能         | 实现情况 | 说明 |
|--------------|----------|------|
| 事件溯源 (EventStore) | ✅ 有 | 内存实现完整 |
| 快照 (Snapshot) | ✅ 有 | 内存实现完整 |
| Provider 接口 / InMemoryProvider | ✅ 有 | |
| Couchbase 等外部 Provider | 可选 | 文档有提，代码以内存为主 |

**结论**：持久化“模型 + 内存实现”与 Go 对齐；与 Go 的差异主要在是否提供更多存储后端，属扩展层面。

---

### 2.4 远程通信（与 Go 差异大）

| 功能         | 文档 | 代码实际 | 说明 |
|--------------|------|----------|------|
| gRPC 服务端监听 | 100% | ✅ 有 | `StartGrpcServer` 可启动 gRPC |
| 远程消息发送 (RemoteDeliver) | 100% | ⚠️ 骨架 | `EndpointManager::RemoteDeliver` 存在，但 EndpointWriter 实际网络发送、批处理与 gRPC 协议未完全打通 |
| **remote::SpawnNamed** | 100% | ❌ 未实现 | 固定返回 `not_supported`，无跨节点创建 Actor |
| Protobuf 序列化 | 100% | ❌ 占位 | `ProtoSerializer` 返回空/抛异常，未真正序列化 |
| JSON 序列化 | 100% | ❌ 占位 | `JsonSerializer` 依赖 rapidjson，未实现真正序列化 |
| ListProcesses RPC | - | ❌ UNIMPLEMENTED | gRPC 返回 UNIMPLEMENTED |
| GetProcessDiagnostics RPC | - | ❌ UNIMPLEMENTED | gRPC 返回 UNIMPLEMENTED |
| Endpoint 连接管理 / 断线/事件 | - | ⚠️ 多处 TODO | 连接信息、断线通知、EventStream 发布等未接好 |
| 消息批处理 (MessageBatch) | - | ⚠️ TODO | endpoint_reader 中未实现 |

**结论**：远程通信**接口与骨架**在，但**跨节点可用的“创建 + 发消息 + 序列化”链路未闭环**，与 Go 的 `remote` 相比差距大，是当前与 Go 的**主要功能差**之一。

---

### 2.5 集群（与 Go 差异大）

| 功能         | 文档 | 代码实际 | 说明 |
|--------------|------|----------|------|
| MemberList / Member | 100% | ✅ 有 | 成员数据结构与更新有 |
| Gossip 协议 (Gossiper/Informer) | 100% | ⚠️ 半成品 | 本地状态与合并有，**“Send gossip update via remote” 未实现**，Gossip 不跨节点 |
| BlockList 与 MemberList 联动 | - | ❌ TODO | MemberList 中 “Implement BlockList in Remote” 未做 |
| 拓扑共识 (topology hash) | - | ❌ TODO | InitializeTopologyConsensus 为空 |
| **IdentityLookup (Grains)** | 90% | ⚠️ 依赖 Remote | 逻辑有（缓存、选成员、调 Remote），但 `remote::SpawnNamed` 未实现，故 Grains 实际不可用 |
| **GetCluster(actor_system)** | - | ❌ 返回 nullptr | “Get cluster from extensions” 未实现，集群从 Extensions 取不到 |
| ClusterProvider (Consul 等) | - | ⚠️ TODO | HTTP 请求 Consul API 未实现 |
| VirtualActorCount | - | ⚠️ 固定 0 | “Sum up counts from all kinds” 未实现 |
| Pub/Sub | 100% | ✅ 有 | 本地 Pub/Sub 有实现 |

**结论**：集群**数据结构和本地逻辑**在，但**依赖远程的 SpawnNamed、Gossip 发送、BlockList、GetCluster** 等未接好，与 Go 的完整集群（含跨节点 Gossip、Grains）相比差距大。

---

### 2.6 序列化（与 Go 差异大）

| 功能         | 文档 | 代码实际 |
|--------------|------|----------|
| Protobuf | 100% | 占位：Serialize 返回空，Deserialize 抛异常 |
| JSON (rapidjson) | 100% | 占位：未实现真正序列化/反序列化 |

**结论**：远程与集群要真正互通，必须补齐至少一种可用的序列化（如 Protobuf），当前与 Go 的“开箱即用”有差距。

---

## 三、差异汇总表（与 ProtoActor-Go 对比）

| 维度           | 与 Go 一致或接近 | 与 Go 有显著差异 |
|----------------|------------------|-------------------|
| 本地 Actor 模型 | Actor/PID/Context/Props、Send/Request/Future、生命周期、监督、行为、中间件、EventStream | - |
| 路由           | 广播/轮询/随机/一致性哈希 | - |
| 持久化         | 事件溯源 + 快照（内存） | 更多存储后端（可选） |
| 远程           | gRPC 服务端、RemoteDeliver 骨架 | **SpawnNamed、真实序列化、ListProcesses/Diagnostics、Endpoint 生命周期** |
| 集群           | MemberList、Pub/Sub、Gossiper/Informer 本地逻辑 | **Gossip 跨节点发送、BlockList 联动、GetCluster、IdentityLookup(Grains) 依赖 SpawnNamed** |
| 虚拟 Actor     | 身份查找与 PID 缓存逻辑 | **依赖 SpawnNamed，当前不可用** |

---

## 四、量化估计（供参考）

- **仅看“单进程内”能力**（不含远程/集群/序列化）：与 Go 约 **95%+** 对齐，文档中的 95%+ 主要反映这部分。
- **若把“远程 + 集群 + 序列化”算入整体功能**：  
  - 接口与骨架已存在，但**端到端跨节点与 Go 对齐的部分**约在 **60%～70%**（本地满配 + 远程/集群部分通路）。  
  - **要达到与 Go 同等的“分布式 + Grains”体验**，仍需补齐：  
    - `remote::SpawnNamed` 的真实实现  
    - 至少一种可用的 Protobuf（或 JSON）序列化  
    - Gossip 通过 remote 实际发送  
    - GetCluster 从 Extensions 获取  
    - （可选）BlockList 与 MemberList 联动、ListProcesses/GetProcessDiagnostics、ClusterProvider

---

## 五、建议的后续对齐顺序（若目标为接近 Go）

1. **序列化**：实现 Protobuf 序列化/反序列化（或先实现 JSON），使远程消息可真实传输。
2. **SpawnNamed**：在 remote 层实现跨节点创建 Actor（含 gRPC 调用与序列化），打通 Grains/IdentityLookup。
3. **Gossip**：在 gossip 中调用 remote 发送 Gossip 更新，使集群拓扑能跨节点同步。
4. **GetCluster**：在 Extensions 中注册 Cluster，实现 `GetCluster(actor_system)`。
5. **BlockList / Endpoint 事件 / ListProcesses 等**：按需补齐，完善运维与一致性。

当前项目已明确 **remote/cluster 两大功能暂缓优化**，上述差异可作为恢复该部分开发时的优先级参考。

---

## 六、参考文档与代码位置

- 功能对比与迁移：`docs/COMPARISON_AND_MIGRATION.md`
- 架构与模块：`docs/ARCHITECTURE.md`
- 远程相关：`src/remote/`（remote.cpp、grpc_service.cpp、endpoint_*.cpp、*_serializer.cpp）
- 集群相关：`src/cluster/`（cluster.cpp、member_list.cpp、gossip.cpp、gossiper.cpp、identity_lookup.cpp）
- 当前限制与路线图：`README.md` 中「当前限制与路线图」
