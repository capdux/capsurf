# CapSurf: Million-Agent Native Headless Browser in CP (`GEMINI.md`)

## 1. 项目定位与核心基建

你正在开发 **CapSurf** —— 一款使用 **CP 编程语言** 原生编写、专为百万级 AI Agent 并发设计的超轻量无头浏览器引擎。

- **核心优势**：零 GC、编译期借用与排他安全（根除内存泄漏与悬垂引用）、单实例百 KB 级内存、支持完整 CSS3 选择器匹配与盒模型坐标计算（`getBoundingClientRect`），同时支持独立单二进制运行（`cap build -r -f`）与 Nest 容器动态库热加载（`cap build -b`，编译宏 `@is_nest`）。
- **核心使命践行（最高优先级）**：
  - **可变性排他收窄**：所有方法与函数严格区分只读（省略首参 + 体内 `_`、形参 `v: T`）与独占可变（首参 `*_`、形参 `v: *T`），消除指针别名引发的并发数据竞争与悬垂引用。
  - **所有权转移明确化**：资源释放与跨状态传递显式标注 `move _` / `move v: T` / `own v: T`，禁止隐式浅拷贝与双重释放。
  - **RAII 生命周期保障**：配合 `defer` 资源析构与 Arena 批量管理，利用 CP 编译器生命周期分析，实现零内存泄漏的工业级运行时。
- **内置一等公民与标准库直接复用（源码位于 `/data/dev/cp/lib/`）**：
  - **全局一等公民（无需 `import`）**：
    - `std.*`：直接使用 `std.*`（**严禁写 `const std = @import("std");`，严禁为调 `std` 使用 `zig {}`**）。
    - `JSON` 与 `RegExp`：直接使用 `JSON.stringify(val)`、`JSON.parse<T>(str)`、`JSON.parse(str)` 以及 `/pattern/flags` 或 `new RegExp(pattern, flags)`（废除旧版 `@json_parse`）。
  - **`cap:css`（已全面实装，路径 `/data/dev/cp/lib/css`）**：源自 Bun/LightningCSS 与 Mozilla Servo，包含完整的 CSS Parser、Selectors、Rules、Properties、Values。
  - **`cap:encoding`（已实装，路径 `/data/dev/cp/lib/encoding`）**：支持 Base64 等标准编码算法。
  - **其他已实装标准库**：`cap:http`、`cap:ws`、`cap:url`、`cap:conf`、`cap:event`（含 `event_loop`）、`cap:io`、`cap:sync`、`cap:net`、`cap:pack`（压缩/归档）、`cap:crypto`。
- **参考文档策略**：
  - 仓库根目录必须维护精简版 **`CP_PITFALLS.md`**（约 3KB，与本文件第 2 节同源维护）。
  - **严禁子智能体全文读取 57KB 的 `/data/dev/cp/cp_agent_ref.md`**！子智能体启动只读 `CP_PITFALLS.md`；仅当主智能体遇到生僻语法时，才用 `rg -n` 按行检索 `/data/dev/cp/cp_agent_ref.md`。

---

## 2. 最新 `cpc` 绝对语法红线与代码生成速查（唯一权威标准）

编写或修改任何 `.cp` 文件时，必须 100% 遵守以下最新 `cpc` 规则（严禁混入 Zig/Rust 旧语法）：

1. **变量与常量**：
   - 仅用 `const`（只读）与 `let`（可变）；**严禁使用 `var` 或 `let mut`**！
   - 忽略表达式返回值统一写：`* = expr;`（**严禁写 `_ = expr;`**，因为 `_` 专用于表示当前实例接收者）。
   - 空切片初始化：直接写 `= []`（**严禁写 `&.{}`**）；若需特定类型可变空切片，可在模块顶层声明 `let empty_x: [0]T = [];` 并取 `empty_x[0..0]`。
2. **函数签名、形参、接收者与返回值**：
   - **方法接收者**：**严禁使用 `self` 或 `this`**！方法体内统一用 `_`（或 `_.field`）访问当前实例。只读实例方法**必须省略首参**（写 `(_)` 会报 `E0123`）；修改字段或 `init` 方法首参写 `*_`；消耗自身所有权写 `move _`。
   - **函数形参**：只读传参写 `v: T`；可变借用形参写 `v: *T`；所有权转移写 `move v: T` 或 `own v: T`（**严禁写 `*v: T`、`mut v: T` 或 `let v: T`**；若需在函数内修改按值传入的标量，在函数体首行写 `let p = v;`）。
   - **返回类型**：有返回值必须写 `): T`；无返回值全场景**直接省略 `: void`**（若可能返回错误写 `fn foo()!`；注：指针类型 `*void` 保留）。
   - **编译期与泛型参数**：泛型推导占位符用 `auto`（严禁 `any`）；编译期类型参数写 `comptime T: type`（不要写 `@comptime`）；线程局部变量写 `@tls pub let x: T = ...;`（`@tls` 必须在 `pub` 之前）。
   - **规避 `.slice(arg)` 解析歧义**：对自定义对象调用单参数 `obj.slice(arg)` 会被当作数组切片，须改写为 `const sliceFn = Type.slice; sliceFn(obj, arg)`。
3. **后缀类型修饰符与指针体系**：
   - **后缀修饰**：可空类型写 `T?`（**严禁写前缀 `?T`**）；错误联合写 `T!` 或 `T ! ErrType`（正常类型在前，严禁写 `Err!T`）；可空且可错写 `T?!`。
   - **指针优先级**：`*T?` 会被解析为 `*(T?)`；**可空指针必须写为 `(*T)?` 或 `*T!?`**。
   - **裸指针与 Unsafe**：无类型裸指针写 `*!`（非空）或 `*!?`（可空）或 `*void`；多元素连续裸指针写 `[*]u8!` / `[*]const u8!`。对裸指针执行切片 `ptr[0..len]`、解引用 `ptr.*` 或 `@ptrCast`/`@alignCast` 必须包裹在 `unsafe { ... }` 块中。
4. **控制流、圆括号铁律与解包 (`:=`)**：
   - **圆括号强制铁律 (`E0101`)**：`if (...)`、`while (...)`、`for (...)`、`switch (...)` 的条件或迭代头**必须外包圆括号 `()`**！
   - **海象解包 (`:=`)**：**彻底禁止 `|capture|` 管道捕获**！只读非空解包写 `if (v := opt)`、`while (v := it.next())`；可变指针就地解包写 `if (*v := opt)`（**严禁 `if (let v = e)`**）。注意：不要在三元或赋值表达式位置（如 `const x = if (v := opt) ...`）使用 `:=`，请先声明 `let x = ...;` 再用语句级 `if (v := opt)`。
   - **循环语法**：`for (v of arr)`（只读）、`for (*v of arr)`（可变）、`for (move v of arr)`（消费），迭代变量无需写 `const`/`let`；支持步进循环 `while (cond) : (i += 1) { ... }`。
   - **逻辑与错误处理**：布尔逻辑严格用 `&&` 与 `||`（严禁 `and` / `or`）；空值合并用 `??`；错误冒泡用后缀 `expr?`（**严禁前缀 `try expr`**）；错误捕获用 `expr catch val`。
   - **`switch` 规范**：缺省通配分支写 `* => ...` 或 `else => ...`；赋给 `u8`/`u32` 等无符号变量时，字面量分支写 `@as(u32, 0)` 避免默认推导为 `i32` 触发 `E0207`。
5. **结构体、枚举、可见性与 `@root struct` 铁律**：
   - **默认可见性**：`struct` 的字段与方法、`enum` 的方法**默认均为 `pub`，严禁在内部写多余的 `pub`**！静态方法必须显式写 `static fn`。
   - **结构体字面量**：写 `Foo{ a: 1, b: 2 }` 或 `{ a: 1, b: 2 }`（字段不加前导点 `.a = 1`）。
   - **二级子结构体提升铁律**：`struct`、`@root struct` 或 `enum` 内部嵌套的二级子结构体或复杂常量初始化器不会被 `cpc` lowering。**必须将所有辅助/子结构体、枚举提升到模块顶层声明（位于 `@root struct` 外部），再在父结构体内写 `pub const Sub = TopLevelSub;`**。`@root struct Name { ... }` 仅包裹自身字段与自身方法。
   - **枚举 `name` 冲突规避**：若带载荷联合体含名为 `name` 的变体，使用 `pub union(enum) Foo` 声明，避免与 `enum` 自动生成的 `name()` 方法冲突。
6. **关键字避让与转义**：
   - `err` 是关键字，**严禁将任何变量、形参或错误捕获载荷命名为 `err`**（统一命名为 `e`）。
   - 遇到 `in`, `as`, `let`, `import`, `arena`, `from`, `impl`, `meta`, `macro`, `err`, `type` 作为字段/方法/枚举变体名时，必须转义为 `@"in"`, `@"arena"`, `@"type"` 等。
7. **模块导入与 C-FFI (`@c`)**：
   - 模块导入：`import Foo from "./foo.cp";` 或 `import { Bar } from "cap:css";`（严禁 `@import`）。
   - C-FFI 块：`@c { "header.h", fn foo(a: i32): i32; struct bar { ... } enum baz: c_int { ... } }`，导出写 `pub @c { ... }`，**块内严禁写 `pub` 或 `extern`**。

---

## 3. 目录拓扑与参考源码映射 (`vendor/`)

工作区 `/data/dev/capsurf/vendor/` 下已备好参考源码，按以下规则吸收迁移（**严禁引入任何 AGPL 代码如 Lightpanda**）：

- `vendor/html5ever` (Rust, MIT/Apache-2.0)：
  - 重点读取 `html5ever/src/tokenizer/` 与 `html5ever/src/tree_builder/`，迁移为纯 CP 代码 `src/html/tokenizer.cp` 与 `src/html/tree_builder.cp`。
  - **关键改造**：彻底抛弃 `StrTendril`，全部替换为 CP 零拷贝切片 `[]const u8` / `string`；将 `TreeSink` 的 `Handle` 固定为 `u32` (`NodeId`)，直接调用 `src/dom/dom.cp` 的方法建树。
- `vendor/lexbor` (C99, Apache-2.0)：
  - 仅提取 `source/lexbor/html/` 下的预生成静态查找表（如命名字符实体表 `&amp;`、标签静态哈希表），转化为 CP 静态常量表 `src/html/entities.cp`。
- `vendor/taffy` (Rust, MIT)：
  - 参考其 Flexbox 与 CSS Grid 布局数学计算逻辑，实现 `src/layout/layout.cp`。
- `vendor/quickjs` (C, MIT)：
  - 通过 `@c` 绑定 `quickjs.h`，在 `src/js/qjs_engine.cp` 中封装 `JSRuntime` 与 `JSContext`，并通过 `JS_SetInterruptHandler` 接入 Reductions 抢占式中断控制，防止网页恶意死循环。
- `vendor/kitesurf` (TS/Rust, Apache-2.0)：
  - 参考其 TypeScript 编写的 CDP 协议栈与 Web API (`window`, `document`, `EventTarget`, `MutationObserver`)，迁移至 `src/cdp/` 与 `src/dom/`。

---

## 4. 核心模块架构规范

### 4.1 零循环引用 Arena DOM 树 (`src/dom/dom.cp`)

- 严禁在 DOM 节点间使用双向裸指针链表！
- 定义 `pub type NodeId = u32; pub const NULL_NODE: NodeId = 0;`。
- `Document` 持有连续动态数组 `nodes: [NodeData]`（下标 `0` 固定为 `NULL_NODE` 哨兵）。
- 每个 `NodeData` 通过 4 字节 `u32` 存储 `parent`、`first_child`、`last_child`、`prev_sibling`、`next_sibling`。节省 40% 内存，100% 免疫借用冲突，且支持 `querySelectorAll` 极速线性数组扫描。

### 4.2 对接 `cap:css` 实现选择器引擎与级联计算 (`src/css/matcher.cp` & `src/css/cascade.cp`)

- 直接复用 `/data/dev/cp/lib/css` (`cap:css`)。
- 在 `src/css/matcher.cp` 中实现基于 `Selector.components.items` 的状态机匹配：支持 `.local_name`、`.id`、`.class`、属性匹配（`=`, `~=`, `|=`, `^=`, `*=`, `$=`）、结构伪类（`:nth-child`, `:first-child`, `:last-child`, `:only-child`, `:empty`, `:root`）、逻辑伪类（`:is`, `:where`, `:not`, `:has`）及全部组合子（`>`, ` `, `+`, `~`）。
- 在 `src/css/cascade.cp` 中实现 64 位位打包权重级联计算：`weight: u64 = (important << 63) | (specificity << 32) | source_order`，遍历 `StyleRule` 的 `declarations` 与 `important_declarations`，生成每个节点的 `ComputedStyle`。

### 4.3 盒模型与 Flex/Grid 几何布局 (`src/layout/layout.cp`)

- 从 `ComputedStyle` 提取 `Display`（区分 `.keyword` 如 `none`/`contents` 与 `.pair` 如 `block`/`flex`/`grid`）、`Visibility`、`BoxSizing`（`content-box` vs `border-box`）、`Size`（`width`/`height`）及 `Margin`/`Padding`。
- 计算每个 `NodeId` 的屏幕像素几何框 `Rect { x: f32, y: f32, width: f32, height: f32, visible: bool }`，支撑 `getBoundingClientRect()` 与 AI Agent 视觉坐标提取。

---

## 5. 分阶段执行流水线（Execution Milestones）

严格按阶段推进，**每批文件必须通过 `cpc fix` + `cpc brief`（或 `cap test`）验证闭环，绝不带病进入下一阶段**：

- **Milestone 1（Arena DOM 与 `cap:css` 选择器/级联/布局闭环）**：
  1. 实现 `src/dom/dom.cp`（`Document`, `NodeData`, `ElementHandle`）。
  2. 实现 `src/css/matcher.cp`（对接 `cap:css` 的 `SelectorList` 与 `Component`）。
  3. 实现 `src/css/cascade.cp` 与 `src/layout/layout.cp`。
  4. 编写 `tests/test_css_layout.cp`，验证 `querySelectorAll` 与 `getBoundingClientRect` 坐标计算准确无误。
- **Milestone 2（纯 CP HTML5 解析器迁移）**：
  1. 从 `vendor/html5ever` 与 `vendor/lexbor` 迁移实现 `src/html/entities.cp`、`src/html/tokenizer.cp` 与 `src/html/tree_builder.cp`。
  2. 实现 `Document.parseHtml(html_str)`，编写 `tests/test_html_parser.cp` 验证复杂嵌套与未闭合 HTML 解析后能直接跑通 CSS 查询与布局。
- **Milestone 3（QuickJS-NG 引擎绑定与 DOM API 挂载）**：
  1. 编译 `vendor/quickjs` 为静态库，在 `src/js/qjs_engine.cp` 中通过 `@c` 绑定并注入 Reductions 中断钩子。
  2. 将 `document.querySelector`、`getElementById`、`createElement`、`innerHTML`、`getBoundingClientRect` 暴露给 JS 上下文，通过 `tests/test_js_dom.cp`。
- **Milestone 4（CDP 协议服务端与双模构建发布）**：
  1. 基于 `cap:http` 与 `cap:ws` 实现 `src/cdp/server.cp`，兼容 Puppeteer / Playwright / AI Agent 连接。
  2. 验证独立构建 `cap build -r -f` 与 Nest 容器构建 `cap build -b`。

---

## 6. 自动化工具链、Token 极致节省与子智能体调度铁律 (CRITICAL)

为防止配额因长上下文累积而耗尽，主/子智能体必须严格执行以下纪律：

1. **强制工具映射与自动化修复流水线**：
   - **若迁移源为 `.zig`**：先运行 `python3 scripts/zig2cp_prep.py <a.zig> ...` 生成 80% 转换初稿。
   - **若迁移源为 `.rs` / `.ts` / 新写 `.cp`**：优先调用 `write_file` / `edit_file` 编写 `.cp` 代码，写完后**第一步立即运行编译器内置自动修复**：
     ```bash
     /data/dev/cp/cap/bin/cpc fix <file.cp>
     ```
     利用编译器自动完成 `E0101`..`E0126`（括号、`?T`->`T?`、`|v|`->`:=`、`* =`、`*_` 等）的多轮就地机械修复！
   - **聚合诊断与门禁验证（严禁直接查看 `cpc` 原始几万行报错！）**：
     统一使用编译器内置 `cpc brief` 子命令（已内置 `zig` 版本兜底与 4 门禁检查，无需调用外部脚本）：
     ```bash
     /data/dev/cp/cap/bin/cpc brief -n 8 <a.cp> ...
     ```
     同类错误在文件中一次性修完再重编，严禁修一行编一次。
   - **大文件阅读**：调用 `read_file`（指定 `StartLine`/`EndLine`）；对 `> 800` 行的大文件（如 `html5ever/tree_builder/mod.rs`），先用 `run_command` 执行 `rg -n` 提取结构大纲，再分段读取。
2. **子智能体「用完即弃」与并发上限 `<= 2` 铁律**：
   - 同时运行的子智能体严格限制在 **最多 2 路**。
   - **彻底禁止通过 `send_message` 向已完成上一批任务的子智能体追加新任务**（复用会导致 200k+ Tokens $O(N^2)$ 爆炸）！每批任务必须通过 `invoke_subagent` 启动**全新子智能体**（从 0 上下文起步）。
   - **踩坑表驱动 Prompt**：给子智能体的 Prompt 只写「先读 `CP_PITFALLS.md`，按规范完成指定文件并通过 `cpc fix` 与 `cpc brief` 门禁」，不再内联长篇语法说明，也不包含复杂正则字符串。
   - **步数熔断与大文件拆分**：子智能体单文件 `<= 50` 步仍未过门禁立即汇报 `BLOCKED` 并停止；`> 2,500` 行的大文件（如 `tree_builder`）按状态段拆给多个全新子智能体分治。
   - **主智能体禁止轮询**：发出子任务后只等唤醒，不得重复调用 `manage_subagents list` 或解析 transcript。
3. **临时文件与中性化汇报规范**：
   - 所有临时探测文件、最小复现用例统一存放在 `temp/` 目录（具备直接清理 `temp/`、`bin/`、`.cap/` 的完全权限，无需向用户确认）。
   - 在撰写文档及汇报时使用中性工程术语（用「异常边界防护 / 畸形报文校验」替代含 attack/exploit 词汇，用「资源耗尽防护」替代 bomb/flood，用「终止生命周期」替代敏感进程控制词）；交互语言统一使用**中文**。
4. **配额耗尽（Quota Exhaustion 429）自主定时恢复**：
   - 若遇到 `RESOURCE_EXHAUSTED (code 429): ... Resets in XhYmZs`，主智能体计算重置秒数 + 120 秒缓冲，调用 `schedule(DurationSeconds=..., Prompt="配额已重置，请检查状态并继续迁移")` 注册单次唤醒定时器后立即休眠结束轮次；唤醒后重派全新子智能体。
5. “针对 > 800 行的大文件，推荐优先使用 rg -n "^(pub\s+)?(fn|class|export|const)\s+" <path> 秒级提取全局结构与方法大纲，再精准使用 read_file 查阅目标实现，消除跨 800行多次分段读取的往返开销。”
6. “修改超过 2000 行的文件时，禁止单回合多次调用编辑工具，必须一次性合并修改（或先拆分子模块）”

---

## 7. 与上游 `cp agy` 跨智能体异步双向通信协议（多 AGY 路由隔离与语法广播）

1. **标准操作指令**：`/root/.agy/comm/COMM.md`文件，熟悉多AGY通信机制与指令。
2. **职责边界与多 AGY 隔离铁律**：

- 当前项目名称为`capsurf`，agy名称为`capsurf_agy`。
- **主智能体 (`capsurf_agy`) 与子智能体严禁直接修改 `/data/dev/cp/cpc/src` 编译器源码！**
- 所有 `cpc` 编译器 Bug 或缺失特性，必须且只能通过 `COMM.md` 中约定的方式提交给上游 `cp agy` 修复。
- **CP 语法更新双端广播**：当上游 `cp agy` 更新 CP 语法规范并广播时，`capsurf_agy` 唤醒后须第一时间同步更新本地 `CP_PITFALLS.md`。

3. **遇到`cp`编译器缺陷时的标准上报流程（绝不单点死锁、绝不用非标语法绕过）**：

- 单个文件编译修复尝试不超过 **5 轮**（Token 熔断安全阀）；
- 提炼 `<= 10` 行最小可复现代码写入 `/data/dev/cp/temp/repro_<name>.cp`；
- 调用投递脚本：
  ```bash
  python3 /root/.agy/comm/task_queue.py submit --id "task-capsurf-YYYYMMDD-<slug>" --repro "/data/dev/cp/temp/repro_<name>.cp" --error "简明报错说明与代码生成根因定位" --by "capsurf_agy"
  ```
- 将受阻文件标记为 `BLOCKED`（保留原 `<stem>.zig` 不被破坏），立即切换迁移下一个无依赖模块。

4. **查看看板状态**：
   ```bash
   python3 /root/.agy/comm/task_queue.py list
   ```
5. **收到 `cp agy` 结单通知时**：必须在对话屏幕显式打印结单详情（Task ID、修复结论、复现验证结果），**第一时间实时追加写入 `SESSION.md`**，并立即用新 `cpc` 编译解封此前 `BLOCKED` 的文件。

---

## 8. 四线文档冷热分离与 Git 原子提交铁律

1. **四线文档冷热分离（仅主智能体维护，子智能体严禁修改）**：
   - **热日志文件（仅记录当日活动，单行精简追加）**：
     - `SESSION.md`：按时间顺序追加用户完整指令、阶段报告、以及与 `cp agy` 的每一次通信详情（严禁覆盖或删减，实时记录）。**审计零丢失与零错配铁律**：每次响应用户指令或收到 `cp_agy` 工单投递/结单通知时，必须在 `SESSION.md` 中成对、按时间顺序真实记录「`[User]` 完整指令 <-> `[Agent]` 对应回应」及「`[COMM -> cp_agy]` 提交 <-> `[cp_agy 结单]` 验收」条目，严禁遗漏、截断或张冠李戴错配！
     - `CHANGELOG.md`：按里程碑追加新增/修改的文件清单。
     - `DEVLOG.md`：按里程碑追加核心技术方案与排障经验。
   - **冷归档文件**：超过当日的历史记录归档至 `SESSION_H.md`、`CHANGELOG_H.md`、`DEVLOG_H.md`。
2. **Git 原子提交铁律（仅主智能体执行）**：
   - 每完成一个 Milestone 子阶段或 3~5 个模块闭环验证后，先更新 `SESSION.md`、`CHANGELOG.md`、`DEVLOG.md`。
   - 精确 `git add` 变更的源码与文档（排除 `temp/` 与 `*.zig.map`），只执行唯一 **1 次** `git commit`（严禁 `--amend` 或事后二次补交）。

---

## 9. 自主推导演进、双智能体流水线与屏幕透明透出铁律 (AUTONOMOUS EXECUTION & TELEMETRY)

1. **自主推导与持续自驱推进铁律**：
   - **自驱推进无间断**：主智能体在完成当前 Phase 任务与提交后，不得停滞等待用户指令，必须自行推导分析依赖拓扑、明确下一个 Milestone/Phase 目标，自动派发全新子智能体推进，循环往复。
   - **双智能体满负荷并行流水线**：始终保持最多 **3 路全新子智能体** 并行推进（严格遵守「用完即弃」铁律，严禁复用已完成上下文）：
     - 一路攻坚原生核心模块（DOM/CSS/JS/网络/存储等）；
     - 一路攻坚配套测试套件、适配驱动或验证框架。
   - **单阶段七步闭环交付**：每个 Phase 必须严格完成「代码编写 -> `cpc fix` -> `cpc brief` 4 门禁 -> 原生单元测试通过 -> E2E 回归验证 -> 四线文档实时更新 -> 唯一 1 次 Git 原子提交」七步闭环，绝不带病进入下一阶段。
   - **合批提速迭代铁律（Batch-and-Verify）**：避免每修 1 个小缺陷就单独编译一次 `bin_dev`（严禁逐点微调反复重编）。对同模块或无冲突的 **3~5 个修复点集中修改源码**后，统一执行 **1 次** `cpc fix` + `cpc brief` 门禁 + `bin_dev` 增量编译 + **合并版单脚本多点 CDP 探针** + `--smoke` + 唯一 1 次原子提交，并持续保持 **3 路测试子智能体**（1 PW + 2 WPT 独立端口）并行回归验证。
2. **实时工作状态透明透出规范（消除黑盒等待）**：
   - **操作前屏幕提示**：在下发子智能体任务、启动耗时编译（`cap build`）或进入反应式休眠前，主智能体必须在屏幕显式输出「当前正在执行的动作卡片」（正在处理的模块、分配的子智能体、预期目标与检验标准）。
   - **唤醒后即时汇总**：子智能体汇报或后台任务完成被系统唤醒时，主智能体必须在屏幕显式打印阶段性成果、测试断言结果看板与后续计划，让用户随时掌握研发动态。

---

## 10. 目录组织拓扑与双轨测试架构（统一 `test/` 分层权威规范）

1. **项目目录拓扑与单数命名规范**：
   - 遵循清晰的职责隔离与目录单数化命名（如 `demo` 替代 `demos`），所有测试资产统一收敛于 `test/` 下：
     - `src/`：CapSurf 纯 CP 核心引擎源码（DOM, CSS, Layout, HTML, Render, JS, CDP, Net, Eval, Storage, XPath 等）。
     - `test/`：统一测试根目录，按测试层级划分子目录：
       - `test/unit/`：纯 CP 编写的白盒单元测试（`test/unit/dom/`, `test/unit/css/`, `test/unit/net/`, `test/unit/eval/`, `test/unit/render/` 等），严禁放入 `.ts`, `.js`, `.html` 等异构文件，确保可通过 `cap test test/unit/` / `cap run` 极速批量执行。
       - `test/e2e/`：黑盒端到端测试套件（统一使用 `bun test` 驱动 TypeScript 原版测试用例）：
         - `test/e2e/*.e2e.test.ts`：从 Kitesurf 迁移的 392 个全量黑盒 E2E 测试用例。
         - `test/e2e/harness/`：独立进程拉起、CDP WebSocket 客户端会话与断言框架（`harness.cjs`, `index.cjs`）。
         - `test/e2e/page/`：Kitesurf 原始测试页面合集（HTML、CSS、Wasm、自定义字体等），供测试服务加载。
         - `test/e2e/ref/`：需转写为纯 CP 白盒测试的原版 TypeScript 测试参考源。
         - `test/e2e/demo/`：172 个功能演示与特殊场景验证 E2E 用例（原 `demos` 单数化规范命名）。
       - `test/wpt/`：Web Platform Tests 清单、预期基线与运行器脚本（`scripts/`）。
       - `test/pptr/`：Puppeteer 上游兼容测试清单、基线与运行器脚本（`scripts/`）。
       - `test/pw/`：Playwright 上游兼容测试清单、基线与运行器脚本（`scripts/`）。
     - `scripts/`：项目工程化与自动化工具链（`bundle_runtime.py`, `watch_task.py`, `task_queue.py`, `e2e_suite.py` 等）。
     - `vendor/`：第三方参考源码与静态资产（`html5ever`, `lexbor`, `taffy`, `quickjs`, `kitesurf`）。
     - `bin/`：编译输出产物（`bin/main`, `bin/test/`）。

2. **双轨测试分工与语言纯洁性铁律**：
   - **白盒 / 纯 CP 原生单元测试（`test/unit/**/*.cp`）**：
     - 验证内部 Arena DOM 结构、CSS 级联权重算法、URL 展开截幅、内存生命周期、XPath 评估等底层算法准确性与内存安全。
     - 必须使用纯 CP 编写，严禁混入 Node.js 运行时或外置依赖，通过 `cap run <file.cp>` 毫秒级闭环。
   - **黑盒 / E2E 远程驱动测试（`test/e2e/*.e2e.test.ts` & `test/e2e/demo/*.e2e.test.ts`）**：
     - **端到端测试严格保留为 TypeScript，统一使用 `bun test` 执行，严禁再生成冗余 `.test.js` 副本或转写为 CP 语言**！
     - 必须以真实的外部客户端视角（如 Playwright / Puppeteer），通过网络 WebSocket 远程连接测试 CapSurf 独立服务。
     - 验证真实网络监听、HTTP 自省、RFC 6455 帧收发、CDP 域状态同步、JS 页面执行与视觉光栅化截屏。

3. **进程级解耦适配架构 (`test/e2e/harness/harness.cjs`)**：
   - 原版 Kitesurf E2E 深度绑定了 Cloudflare Workers 专有的 `cloudflare:test` 和 `env.ENGINE` 内部句柄。
   - CapSurf 通过 `test/e2e/harness/harness.cjs` 与 `test/e2e/shim/cloudflare-test.ts` 统一拉起独立进程（`./bin/main --remote-debugging-port=<port>`），通过标准 CDP 调试端点（`ws://127.0.0.1:<port>/devtools/browser/...`）建立会话，提供标准 `test()` 与 `expect()` 断言器，并在测试结束后优雅销毁进程。

4. **全量 Kitesurf E2E 测试统一执行流水线**：
   - 保持 `test/e2e/*.e2e.test.ts`（392）与 `test/e2e/demo/*.e2e.test.ts`（172）原版 TypeScript 测试文件不变；
   - 统一通过 `CAPSURF_BIN=./bin/main bun test <files...>` 执行，将其无缝集成到每个 Milestone / Phase 的自动化交付回归流水线中。

---

## 11. 实战深度复盘：Token 极致节省、高效并发与架构防坑准则 (TOKEN OPTIMIZATION & WORKFLOW SOLIDIFICATION)

在百万级无头浏览器工程演进中，针对长上下文消耗、多智能体并发冲突与 C-FFI / Arena DOM 阻抗失配，必须严格执行以下实战准则：

### 11.1 多子智能体构建权限隔离与防冲突铁律 (Build Isolation & Race Prevention)

- **痛点根因**：多路子智能体并发运行时，若各自调用 `cap build src/main.cp -f`，会并发争抢写入同一个 `bin/main` 二进制产物。一旦某个子智能体的编译动作删除了正在被 `bun test` 调用的二进制，就会引发 `CapSurf process exited prematurely (code=0)` 偶发假死报错，导致子智能体陷入无谓的排错与盲目重试，浪费数万 Tokens！
- **执行规范**：
  1. **纯 JS/TS Shim 优先轻量打包**：若当前任务仅修改 `src/js/shim/` 下的代码，**严禁执行耗时的 `cap build`**！只需运行毫秒级的 `python3 scripts/bundle_runtime.py` 重新生成 `runtime.js` 与 `runtime_source.cp`，若主二进制已具备动态运行时挂载即可直接运行测试；
  2. **原生 CP 修改的门禁前置与三线无锁物理隔离并行编译**：若涉及修改底层 `.cp` 文件，先跑 `/data/dev/cp/cap/bin/cpc fix` 与 `/data/dev/cp/cap/bin/cpc brief -n 8` 静态门禁；三线并发（主智能体 + Slot A + Slot B）必须按固定槽位分配独立输出目录、调试端口与源码写权限，**严禁多路同时写入同一个 `bin/main`**：
     - **主智能体**：`bin_19231` + `REMOTE_DEBUGGING_PORT=19231`（负责非 `cdp`/`shim` 模块如 `src/layout/`、`src/css/`、`src/render/`、`src/html/`、`src/net/` 或汇总验证）；
     - **Slot A**：`bin_19232` + `REMOTE_DEBUGGING_PORT=19232`（负责 `src/js/shim/` 或 `test/`）；
     - **Slot B**：`bin_19233` + `REMOTE_DEBUGGING_PORT=19233`（负责 `src/cdp/`）；
     - 日常开发统一使用 `ulimit -s 65536 && cap build -m -d -o bin_<port> src/main.cp`（全量 ~12.7s、增量 ~8.5s，零 `flock`、零 `FileBusy`），并严格用 `CAPSURF_BIN=./bin_<port>/main REMOTE_DEBUGGING_PORT=<port> bun test ...` 验证；仅在 Milestone/Phase 收口同步正式产物时由主智能体运行一次 `ulimit -s 65536 && cap build -m -r -o bin_rel src/main.cp && cp bin_rel/main bin/main`。

### 11.2 Arena DOM 唯一真理源与 C-FFI 包装器单例铁律 (Single Source of Truth & Identity Bridge)

- **痛点根因**：Kitesurf 原始运行时的 `_children`、`childNodes` 纯 JS 镜像与底层 CP 原生 Arena DOM 发生脱节：
  - 双写导致倍增（如原生写入文本节点后，JS 端又 new 了一个 `Text` 放入 `_children`，读出 `"cc"` 重复文本）；
  - C-FFI 跨界生成的临时对象导致引用不恒等（`el.parentNode !== el.parentNode`），破坏 JS 的对象恒等律（Equality）并导致临时状态丢失。
- **执行规范**：
  1. **Arena `_nid`（`NodeId`）为绝对权威**：所有节点状态以底层 CP 原生 Arena 数据结构为唯一源头；
  2. **弱引用包装器单例化**：QuickJS 端通过 `__capsurf_wrappers` 或 `WeakMap` 基于 `_nid` 复用包装器，保证对同一个底层的多次读取 `===` 恒等；
  3. **惰性镜像与清空同步**：JS 端的 `_children` 仅作为惰性只读投影，在设置 `textContent` 或 `innerHTML` 时必须同步清空 `this._children = []` 或删除缓存，严禁双端冗余追加。

### 11.3 分域精准聚类攻坚（Domain-Clustered Milestones）

- **痛点根因**：全量 391 个测试若盲目批量执行，遇到包含 20s/60s 超时的未实现特性测试会长时间阻塞；且发散修复多个不同领域的用例会导致上下文膨胀。
- **执行规范**：
  1. 每个 Phase 主智能体依据拓扑依赖，挑选 3~5 个具有共同协议或 DOM 机制的紧凑领域用例（如 Options/Select 集合组、Script IDL 组、Canvas 2D 组、Template 隔离组）；
  2. 派给子智能体的任务明确限制在专属用例范围，跑通即收割，绝不跨域漫游。

### 11.4 Bun 测试诊断流向铁律 (Bun Test Diagnostics Flow)

- **执行规范**：`bun test` 的测试断言详情与执行摘要默认输出在 **`stderr`**，进程返回码 `exitCode === 0` 即代表该文件全部测试 100% 绿色通过。自动化脚本与子智能体诊断测试结果时，必须同时读取 `stderr` 与 `exitCode`，切勿仅因 `stdout` 为空而误判。

### 11.5 精益 Prompt 模板（Ultra-Lean Subagent Prompts）

- **执行规范**：派发全新子智能体时，严禁在 Prompt 中内联长篇背景、代码大纲或复杂正则字符串。统一使用 4 要素极简模板（消耗 `< 500` Tokens）：
  ```text
  你负责 Phase XX Track Y：[模块名称与核心目标]
  1. 先读根目录下 CP_PITFALLS.md；
  2. 核心修改点：[3~5 行明确的代码位置与逻辑动作]；
  3. 门禁验证：运行 cpc fix 与 /data/dev/cp/cap/bin/cpc brief -n 8；
  4. 目标测试：运行 bun test e2e/<target>.e2e.test.ts 确保 100% PASS。
  完成即汇报修改清单与结果。
  ```

### 11.6 零修改摸底、小粒度快进快出与带行号派单铁律 (HIGH-VELOCITY EXECUTION)

- **痛点根因**：实测 `cap build -m -d -o` 仅需 4–6 秒，而单轮推进仍耗时 9–14 分钟（170–240 步），80% 以上时间消耗在“单批塞入 7–9 个无关套件导致 40–60 次 `read_file` 冷启动探索 + 大量 LLM 往返”。
- **执行规范（强制固化）**：
  1. **主智能体一次性批量快扫全量未验证测试**：直接由主控运行带 `timeout 12` 的全量扫盘脚本一次扫完所有未验证套件（替代 3 个一组派子智能体冷启动摸底），**直接 PASS 的当场批量登记，子智能体 100% 只接手已明确报错行号的纯修复单**。
  2. **小粒度与集中式高密收口单**：热文件（`session.cp`, `window.js`, `page.cp`）攒齐 3–5 个独立带行号小修点后打包为 1 张集中收口单，**只改 1 轮、编译 1 次（5 秒）、一次验证**，控制在 **30–50 步（2–4 分钟）** 内快进快出、高频原子提交。
  3. **带行号精准派单 + 亚秒级静态校验**：主智能体先用 `rg -n` 定位关键函数和行号再写入 Prompt，压缩子智能体探索轮数；子智能体改完同一批 `.cp` 文件后先用 `/data/dev/cp/cap/bin/cpc brief -n 8` 一次性静态校验通过，再执行 1 次 `cap build -m -d -o bin_<port>` + `bun test`。
  4. **E2E 端口僵尸进程预检铁律**：启动 `bun test` 前，先清理目标调试端口上的前序遗留进程（`lsof -ti :<port> | xargs -r kill -9`），避免旧 `main` 占端口导致测试卡死并空耗超时。
  5. **双轨零冲突与切片出参包装**：Slot A 专注纯 CP 白盒（严禁改 `src/cdp/` 或跑 `cap build`），1~3 分钟快进快出；遇 `cpc` 剥离 `&slice` 取址时，统一用单字段 `struct { value: []const u8 = "" }` 包装出参。
  6. **诊断超时防护与双端口并发扫盘**：任何临时 `bun -e` WebSocket 诊断脚本必须外加 `timeout 15` 护栏，严禁无超时挂起空转；利用 `bin_19232`（端口 `19232`）与 `bin_19233`（端口 `19233`）物理隔离，可让 Slot A 在不触发编译的前提下并行快扫未记录 E2E 用例，吞吐翻倍。

### 11.7 主控前置投喂与自适应超时铁律 (SELF-EVOLVING TOKEN OPTIMIZATION)

- **前置精确投喂（Pre-Seeded Prompts）**：派发修复子智能体前，主智能体或诊断槽位必须先用 `rg -n` / 单跑提取出「精确报错行号 + `Expected`/`Received` diff + 目标 `src/` 函数/行号」直接写入 Prompt，实测可将修复型子智能体从 10–15 分钟压缩至 **3–5 分钟**、节省 60%+ 探索 Token。
- **自适应超时预算（Subtest-Scaled Timeout）**：批量扫盘前先用 `rg -c "test\(" <file>` 统计子测数 `N`，按 `max(20, N * 4)` 秒设置超时（如 33 子测的 `inline-metrics-regressions` 需 `120s`），避免把「慢但全绿」误判为挂起而重复浪费轮次。
- **双端口镜像同步（Binary Sync on Commit）**：每次 Slot B 完成 `bin_19233/main` 构建并提交后，主智能体立即执行 `cp -f bin_19233/main bin_19232/main`，防止 `e2e/harness.ts` 与旧版 `bin_19232` 协议错配产生假失败。
- **大栈白盒运行规范**：凡传递导入 `src/cdp/` 或 `mock_fixtures.cp`（~98 模块）的白盒测试，执行 `cap test -m` 前必须加 `ulimit -s 65536` 防止编译期栈溢出。
- **分级构建加速铁律（Debug `-m -d` vs ReleaseFast `-m -r`）**：日常修复与 E2E 调试**严禁**使用 ReleaseFast `-m -r`（~81s），必须统一使用 `cap build -m -d -o bin_<port> src/main.cp`（实测全量 99 模块仅 **12.70s**，增量单文件仅 **~8.5s**，提速 7–10 倍）；

### 11.8 WPT 双并发执行、JSON 摘要提取与持久化服务规范 (2026-11-24 自进化固化)

- **WPT 并发度提速 (`-j 2`)**：在拥有充足内存的宿主机上运行 `wpt-batched.mjs` 时，优先使用 `-j 2`（双引擎并发，如 `--port 19700` 会自动占用 `19700..19701`），实测 54 个用例耗时从 46s 压缩至 **27s**（提速 41%），且无端口冲突。
- **极简 `--full-out` + Python 失败提取（节省 90% Token）**：运行未知套件摸底时使用 `--full-out /tmp/xxx.json`，严禁直接 `cat` 原始 JSON；统一用 1 行 Python 脚本仅打印 `FAIL` 用例名与前 3 条 `subtest` 断言差值，将单轮上下文消耗从数万 Token 降至 `< 500` Tokens。
- **跨模块同源合批修复**：将共享底层逻辑的 CSS 模块打包在同一 Phase 中修改（如 `canonicalizeCalcToken` 同时覆盖 `css-logical` 与 `css-tables/border-spacing`），单次 `bundle_runtime.py` + `cap build` 同步打通多套件全绿，减少 50% 构建往返。
- **后台持久化 `wpt-serve`**：使用 `IsDaemon: true` 在后台启动 `node test/wpt/scripts/wpt-serve.mjs` 监听 `8000` 端口；主智能体与各只读子智能体均**严禁**再传递 `--start-server`，各自使用隔离 CDP 端口（`19700` / `19710` / `19720`）并发运行，彻底消除重复启停开销。
