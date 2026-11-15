# CapSurf Development Log (2026-11-15)
## 多智能体（`cp_agy` / `bun_agy` / `capsurf_agy`）异步通信隔离与广播机制
- **问题根因**：直接从 `/data/dev/bun/scripts/` 拷贝的脚本默认 `--by bun_agy`，且 `watch_task.py` 扫描平面目录 `/data/dev/cp/.tasks/resolved/` 时未做 `created_by`/`target_agy` 过滤，导致一方结单会误唤醒另一方；同时 `cpc_brief.py` 硬编码了 `vendor/zig/zig` 与 `backup(_.zig)` 检查。
- **解决方案**：
  1. **身份识别与防碰撞**：`capsurf_agy` 提交工单默认归一化为 `task-capsurf-*`，载荷注入 `"project": "capsurf"`, `"created_by": "capsurf_agy"`, `"target_agy": ["capsurf_agy"]`。
  2. **按 AGY 隔离结单路由**：结单时按 `target_agy` 投递至 `/data/dev/cp/.tasks/resolved/capsurf_agy/` 或 `resolved/bun_agy/`，配合 `/data/dev/cp/.tasks/.ack_capsurf_agy.json` 独立水位线游标实现精准单播唤醒。
  3. **CP 语法更新双端广播**：新增 `task_queue.py broadcast`（及 `resolve --broadcast`），将 `"category": "syntax_update", "broadcast": true` 同时扇出写入 `resolved/capsurf_agy/`、`resolved/bun_agy/` 与 `resolved/`，实现 `bun_agy` 与 `capsurf_agy` 同时无损唤醒。
## 多工单并发结单漏收竞态的根因与三层根治架构
- **历史故障复盘**：旧版 `watch_task.py` 单次只弹出第 1 个文件即 `sys.exit(0)`，且重启时执行 `initial_resolved = set(os.listdir(RESOLVED_DIR))` 将同秒到达（或进程重启间隙到达）的其余 3 个工单误判为“历史已读”，导致漏收。
- **三层根治方案（无需人为 `sleep 2` 或手动写汇总单）**：
  1. **磁盘持久化水位线游标（`.ack_<agent>.json`）**：彻底废除内存快照 `initial_resolved`，仅当工单被 `watch_task.py` 真正打印输出给 AGY 时才写入 `.ack_<agent>.json`。任何在进程重启间隙落盘的结单在下次启动第 0 秒立即补发唤醒。
  2. **200ms 微批聚合窗口（Coalescing Window）+ 批量排空（Batch Drain）**：检测到首个新事件后自动等待 200ms 聚合同批次连续写入的 `.json` 文件，一次性排空并输出全部 `N` 个就绪任务（`[FOUND_TASKS_BATCH] Count: N`），1 次唤醒接收全部结果。
  3. **多工单原子 `resolve`（`nargs="+"`）**：支持 `python3 scripts/task_queue.py resolve id1 id2 id3 id4 --note "..."` 单命令批量结单。

## Milestone 1 核心架构闭环与编译排障技术方案
1. **Arena DOM 线性数组扫描架构 (`src/dom/dom.cp`)**：
   - 彻底废除节点间双向裸指针，以连续数组 `nodes: ArrayListUnmanaged(NodeData)`（`NodeId = u32`）组织树形结构，避免借用检查冲突并实现零循环引用。
   - `querySelectorAll` 退化为 $O(N)$ 连续线性内存扫描，硬件 Prefetcher 充分利用，性能远超基于 DFS 跳转的传统 DOM 树。
2. **Servo 架构选择器匹配 (`src/css/matcher.cp`)**：
   - 解析复合选择器与简单选择器链，以结构伪类（`:nth-child(an+b)` 动态方程求解）与组合子（子代、后代、紧邻兄弟、后续兄弟）逆序状态机匹配。
   - 匹配结果计算 30 位 W3C Specificity（`10-bit ID | 10-bit Class | 10-bit Tag`）。
3. **64 位寄存器位打包权重级联与插槽竞争 (`src/css/cascade.cp`)**：
   - 每条声明赋予 64 位权重：`weight = (important << 63) | (is_inline << 62) | (specificity << 32) | source_order`。
   - `ComputedStyle` 为每个属性槽位分配 8 字节权重寄存器，通过一条汇编指令 `if (weight >= weights[slot])` 决定是否覆盖，极速实现 W3C 级联法则。
4. **盒模型与 Flexbox 布局坐标提取 (`src/layout/layout.cp`)**：
   - 提取 ComputedStyle 盒模型边距、边框、内边距与宽高约束，在单一拓扑序遍历下计算屏幕像素坐标 `Rect { x, y, width, height, visible }`。
   - 验证 Flex 容器在 row 方向的自动主轴间距与内边距偏移（`#submit-btn` 坐标 `x: 225, y: 20, w: 120, h: 44`），以及 `display: none` 节点的零几何与隐藏标记。
5. **排障与语法避坑经验**：
   - **结构体方法首参 `*_` 与接收者 Lowering**：修改自身状态的实例方法首参标注 `*_`，体内用 `_`；传参时避免对局部值变量写 `&var` 调用静态方法导致指针剥离，统一将其声明为实例方法以 `var.method()` 调用。
   - **固定数组转切片歧义**：CP 中 `[a, b, c, d]` 自动推导为零拷贝切片 `[]const T`，若需固定长度结构（如四边数值），显式定义 `BoxQuad { top, right, bottom, left }` 替代 `[4]T`，消除类型断言错误。

## Milestone 2 核心架构闭环与编译排障技术方案
1. **纯 CP HTML5 解析层解耦设计 (`src/html/`)**：
   - 吸收 `html5ever` 的代数类型与 TreeSink 模式，将节点抽象为 `NodeId` (`u32`)，所有建树操作直接下沉至 `dom.cp` 的 Arena 数组，彻底规避传统 C 语言指针树在复杂的领养机构算法（Adoption Agency Algorithm）中可能产生的循环引用与野指针风险。
   - 采用基于返回值的解耦状态反馈协议（`processToken` 返回 `NextTokenizerState`），避免 `TreeBuilder` 与 `Tokenizer` 双向持有可变指针而触发编译器借用冲突（`E0202`/`E0203`）。
2. **W3C 领养机构算法（Adoption Agency Algorithm, AAA）的 CP 极致实现**：
   - 在 `src/html/tree_builder.cp` 中按 W3C 规范 § 13.2.6.4.7 实现 19 步 AAA 算法，通过 `furthest_block` 与 `common_ancestor` 定位、`doc.cloneElementNode` 浅拷贝、`doc.reparentChildren` 批量子节点重挂载，完美解析错位嵌套的格式化标签（如 `<b>1<i>2</b>3</i>` 重排为 `<b>1<i>2</i></b><i>3</i>`）。
3. **表格 Foster Parenting 孤儿节点逃逸机制**：
   - 在 `InTable`、`InTableBody`、`InRow` 等模式下，非预期文本与元素通过 `currentAppropriatePlace` 自动向上探测，逃逸并插入到宿主 `<table>` 前序兄弟位置（`doc.insertBefore`），100% 遵从 HTML5 容错规范。
4. **全链路端到端闭环验证**：
   - 打通「HTML5 文本输入 -> Tokenizer 词法分词 -> TreeBuilder 语法建树 -> Arena DOM -> Selector Matcher 匹配 -> Cascade 样式级联 -> Layout 盒模型与 Flexbox 坐标计算」全链路，全量测试套件（10 个用例）全部绿色通过（`tests/test_css_layout.cp`, `tests/test_html_tokenizer.cp`, `tests/test_html_parser.cp`）。
5. **编译器排障与工单留痕**：
   - **类型别名 `catch` 降低缺陷**：发现 `cpc` 在对结构体类型别名（如 `pub const Bar = Foo; Bar.init() catch ...`）的方法返回值进行 `catch` 降低时，未能展开别名类型，错误生成 Rust 风格的 `switch (__r) { .ok => ..., .err => ... }` 导致 Zig 编译失败。已提炼 10 行最小用例投递工单 `task-capsurf-20261115-alias-catch-lower` 至 `/data/dev/cp/.tasks/pending/`。
   - **闭包结构体类型推断边界**：在 `cap:test` 的 `() => { ... }` 闭包中，通过数组切片链式属性直接赋值给变量时，闭包生成器可能退化推断为 `*anyopaque`。在 `Document` 上封装显式类型返回的方法（`getFirstChild`, `getParent`, `getTextContent`, `getTagName`）既优雅提升 DOM API 易用性，又规避了编译器推断边界。


