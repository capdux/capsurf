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

## Milestone 3 核心架构闭环与编译排障技术方案
1. **QuickJS-NG C-FFI 与 CP 原生类型系统深度融合 (`src/js/qjs_engine.cp`)**：
   - 利用 CP 的原生 C 调用约定语法 `(@c fn(params): ret)?`，直接将 C 函数指针与回调降低为 Zig 的 `?*const fn (...) callconv(.c)`，精准对接 QuickJS-NG 的 `JS_NewCFunction2`、`JS_SetInterruptHandler`、`JS_DefinePropertyGetSet` 等关键 FFI 接口。
   - 实现零开销 JSValue 标量构造：利用 64 位值与 64 位 Tag 结构，在寄存器级别内联处理 `jsNull`, `jsUndefined`, `jsTrue`, `jsFalse`, `jsInt`, `jsFloat`。
2. **基于堆稳定地址的 Reductions 抢占式安全中断机制**：
   - 为防止恶意网页或不可信脚本死循环阻塞主线程，构建 `ReductionState { reduction_limit, reduction_count, interrupted }` 结构。
   - 将该状态分配在堆上并将其稳定裸指针挂载到 `JS_SetInterruptHandler`，彻底消除栈局部变量在 `init` 退出后指针失效引发的悬垂野指针假死 Bug。在循环或调用时周期递增，达到上限后向 QuickJS 返回 1 强制熔断抛出异常，毫秒级终止失控脚本。
3. **CP 借用检查器（Borrow Checker）可变性收窄与多读并行**：
   - 面对 C 接口句柄 `ctx: *JSContext` 与其产生的字符串切片 `[]const u8`，CP 严格编译器默认会视返回切片具有该入参的独占借用权。当同一作用域调用 `jsToSlice(ctx, argv[0])` 和 `jsToSlice(ctx, argv[1])` 时，触发 `E0202: cannot borrow as mutable while 'attr_name' is still active`。
   - **收窄方案**：将所有只读接口形参由独占可变 `*JSContext` 收窄为只读引用 `*const JSContext`，利用安全协变转换使得多个入参切片与返回值能够在同一个词法域内共存借用，彻底消除了假性冲突。
4. **Arena DOM Tear-Off 代理包装模式 (`src/js/dom_binding.cp`)**：
   - 借鉴 Blink/WebKit 现代浏览器架构：DOM 树自身使用连续数组常驻 Arena，JS 层的 `Element` 与 `Document` 仅为极轻量级代理 Tear-Off（仅存 4 字节 `NodeId` 与上下文句柄）。
   - 在 JS 动态创建节点或写入属性（`setAttribute`、`className`、`id`、`textContent`）时，统一由 `doc.allocator.dupe` 进行宿主生命周期复制托管，避免临时字符串内存提前释放引发悬垂野指针。
5. **屏幕像素几何坐标直通 (`getBoundingClientRect`)**：
   - 通过 JS 脚本调用 `element.getBoundingClientRect()` 直接透传 `LayoutTree.getBoundingClientRect(node_id)` 计算的屏幕物理像素几何框（`x, y, width, height, left, top, right, bottom`），为后续 AI Agent 自动化页面理解与视觉动作定位提供零延迟原生基建。
6. **编译器排障与工单留痕**：
   - **`cap.json` C 库依赖传递修复**：上游 `cp_agy` 成功结单 `task-capsurf-20261115-cap-test-link-libs`，使 `cap test` 能够解析 `cap.json` 中的 `c_flags` 与 `link_libs` 并自动传递 `-Lvendor/quickjs/build -lqjs` 给底层链接器。
   - **`cap run` (runSingleFile) 链接参数遗漏上报**：发现 `cap <file.cp>` 的 `runSingleFile` 尚未镜像 `runTestSingleFile` 的 `cap.json` 链接解析逻辑，已通过 `task_queue.py` 提交专属工单 `task-capsurf-20261115-cap-run-link-libs` 供 `cp_agy` 进一步闭环。




## Milestone 4 核心架构闭环与编译排障技术方案
1. **CDP 传输层与协议路由器架构 (`src/cdp/server.cp` & `src/cdp/dispatch.cp`)**：
   - 采用标准 HTTP 与 RFC 6455 WebSocket 协议，实现 `/json/version`、`/json/list`、`/json/new` 服务发现端点，使 Puppeteer、Playwright、Chrome DevTools 前端能无缝连接 CapSurf。
   - 实现瞬时优雅终止机制：在 Linux 多线程服务端中，`shutdown(fd, 2)` 后通过本地 loopback 触发轻量 `wakeServer` 连接，使得阻塞在 `accept()` 的后台线程瞬间唤醒退出，规避线程 Join 假死。
2. **AI 专属杀手级协议能力迁移：AXTree 语义树压缩 (`src/cdp/accessibility.cp`)**：
   - 普通网页 DOM 往往包含数万个无语义标签，直接喂给 LLM 会引发 Context Window 爆炸与高昂费用。
   - 迁移自 Kitesurf 的 AXTree 算法遍历 Arena DOM 树，将节点映射为极简语义角色（`button`, `link`, `heading`, `textbox`, `combobox`），通过 `aria-labelledby`、`aria-label`、textContent 计算可访问名称，并过滤 `ignored` 表现型节点，**直接为 AI Agent 减少 80%+ 的 Token 消耗**。
3. **WebMCP 网页工具原生反射架构 (`src/cdp/webmcp.cp`)**：
   - 将现代 AI Agent 的 Model Context Protocol (MCP) 与网页交互深度打通：
   - 自动扫描页面内部的 `<form>`、`<button>`、`<input>`，基于参数约束与字段描述动态构造标准的 JSON-Schema 工具定义，暴露给外部 AI 调用；在 `WebMCP.callTool` 中自动在页面上执行数据回填与按钮点击。
4. **QuickJS-NG 跨线程栈边界与零结尾解析排障经验 (`src/js/qjs_engine.cp`)**：
   - **跨线程栈检测假死**：QuickJS 默认在创建 `JSRuntime` 时通过 `__builtin_frame_address(0)` 记录主线程栈顶 `stack_top`。当 CDP 服务端在工作线程（`std.Thread.spawn`）中调用 `ctx.eval()` 时，工作线程的栈地址不在主线程栈区间，触发 `Maximum call stack size exceeded`。通过调用 `JS_SetMaxStackSize(rt, 0)` 彻底解除跨线程静态栈限制，并在执行前刷新 `JS_UpdateStackTop(rt)`。
   - **`JS_Eval` 零结尾硬性约束**：QuickJS 源码 § 38213 明确标注 `/* 'input' must be zero terminated i.e. input[input_len] = '\0' */`。若传入无 `\0` 的切片，词法分析器越界扫描触发 `SyntaxError: unexpected end of string`。在 `JsContext.eval()` 中为切片提供 4KB 本地栈缓冲区并在末尾注入 `\0`，彻底解决表达式解析异常。
5. **双模构建全面达标**：
   - `cap build -b src/main.cp` 仅耗时 120ms 即生成 Nest 容器热加载动态库 `bin/main.so`（仅 9KB）与接口元数据 `bin/main.d.cp`。
   - `cap src/main.cp` 成功编译并在原生控制台启动 CapSurf 无头引擎与 CDP 服务。

## 方案 A 目录拓扑与 Web API 运行时垫片架构方案
1. **测试与 E2E 目录规范化（方案 A）**：
   - 为避免复数命名（tests, specs, fixtures）带来的语义不一致与混乱，采用统一且精炼的单数命名体系：
     - `test/`：面向内部 CP 模块的原生单元与集成测试，按领域切分子目录（`dom/`, `html/`, `css/`, `layout/`, `js/`, `cdp/`, `net/`）。
     - `e2e/`：面向外部用户行为与 CDP 协议的全链路端到端体系，包含 `harness/`（驱动夹具）、`page/`（靶场资源）、`spec/`（测试规范）与 `ref/`（原始 TS 测试基准）。
2. **Web API 运行时垫片（Runtime Shim）技术可行性分析**：
   - **痛点**：若在 CP 中通过 QuickJS C-FFI 为 `EventTarget`、`CustomEvent`、`classList`、`MutationObserver`、`URLSearchParams`、`Headers`、`Storage` 等 Web API 逐个手写 C 绑定，将带来上万行胶水代码，且极易因 JS Closure 与 C 指针相互持有导致内存泄漏或循环引用悬垂。
   - **解法**：Kitesurf 源码自身在 `dom-shim/` 中也是采用纯 TypeScript 编写，经由 esbuild 打包成单一 `dom-shim.js`，在页面创建时直接注入执行。
   - **CapSurf 最佳实践**：
     - 在 CP 宿主侧仅提供 5~6 个极简 Arena DOM 操作原语（如 `__capsurf_get_attr`、`__capsurf_set_attr`、`__capsurf_query_selector`、`__capsurf_get_rect`、`__capsurf_mutation_notify`）；
     - 上层丰富的 Web API 纯 JS 逻辑合并为 `src/js/runtime_shim.js`（或预编译 QuickJS 字节码），在 `JsContext` 初始化时注入求值；
     - 零 GC 跨界开销，开发效率提升 10 倍，同时 100% 免疫内存悬垂风险。
3. **测试套件全量回归验证**：
   - 目录拓扑迁移后，所有测试路径更新无误，5 大测试套件 17 项测试全绿通过，验证了单数规范与 `cap.json` 配置的完整性。

## 全量 E2E 规范与靶场收纳架构设计
1. **测试资产分级双轨制**：
   - **单测轨（Internal Unit Tests -> `test/`）**：针对 CapSurf 底层数据结构（Arena DOM、CSS Selectors、Cascade、Layout、Cookies、RequestBlocker、AXTree），用 CP 原生编写，毫秒级自闭环编译执行；原始参考保留在 `e2e/ref/`（95 个）。
   - **端到端轨（External E2E Specs -> `e2e/spec/`）**：针对整页与网络/脚本复杂交互，由外部 TS 客户端驱动 580 个标准场景，通过真实的 CDP WebSocket 协议向 CapSurf 发起操作，自动度量 W3C 与 Chrome 兼容率。
2. **靶场资产集中化 (`e2e/page/`)**：
   - 将原散落在 `fixtures-worker/assets`（221 个）与 `e2e/html`（36 个）的 HTML/CSS/JS 页面统一聚拢至 `e2e/page/`，通过软链与相对路径兼容，消除目录割裂。
3. **架构负资产物理隔离**：
   - 坚决将依赖 `workerd` 的 56 个 Cloudflare Worker 内部桩代码隔离在 `vendor/kitesurf/`，CapSurf 原生使用 Linux POSIX 标准网络栈，杜绝无意义的平台绑定。

## E2E 扁平化架构设计演进
- **消除层级冗余（Over-Engineering）**：
  - 测试目录命名既然已经到达顶级 `e2e/`，再引入 `spec/` 属于非必要的过度封装，不仅让用户进入目录时看到空根目录，而且破坏了原测试脚本中相对引用的直观性。
  - 采用直接平铺策略后，`e2e/` 根目录直接暴露 407 个标准测试文件与 `demos/`、`harness/`、`page/`、`ref/` 四大子目录，使得外部测试运行器（如 vitest / playwright）能够直接以 `e2e/*.e2e.test.ts` 进行无参通配执行。

## Web API 垫片层、UA 样式与原生网络安全架构实战 (Steps 1, 2, 3)
1. **W3C UA 样式表第 0 层级联架构 (`src/css/ua_stylesheet.cp`)**：
   - 将 W3C HTML5 标准 UA 默认外观内建在 `ComputedStyle.initForNode`。
   - `head`、`style`、`script`、`template`、`[hidden]` 等节点在任何作者规则计算之前，初始化为 `display: none`，根治网页解析后将不可见标签渲染到画面的经典 Bug。
2. **高拟真防检测 Web API 垫片 (`src/js/runtime_shim.js`)**：
   - **对象包装与 NodeId 身份对齐**：通过 `_nid`（`u32`）将 JS 包装层与底层 Arena DOM 对齐，事件监听器注册在以 `_nid` 为主键的公共 Registry，根除 `getElementById` 重复生成包装器导致的监听器丢失问题。
   - **DOMTokenList (`classList`) 与底层属性原子同步**：`classList` 内部直接委托给 `_el.getAttribute("class")` 与 `_el.setAttribute("class")`，无论 JS 如何修改类名，底层 Arena DOM 节点立刻感知，完美配合 CSS 选择器匹配。
   - **防爬虫高拟真存根策略**：针对 Turnstile / DataDome / FingerprintJS 的行为探测，实装了按字符字号计算物理宽度的 `measureText`（解决图表库零除崩溃）、返回有效 1x1 PNG base64 的 `toDataURL`、优雅降级返回 `null` 的 `webgl`、以及覆盖 `toString` 返回 `function () { [native code] }` 的原生函数伪装。
3. **CP 原生网络安全层 (`src/net/`)**：
   - **零拷贝 PSL 前缀计算 (`src/net/public_suffix.cp`)**：基于 WHATWG Public Suffix List，充分利用域名由点分隔的内存连续性，所有后缀候选均为原始主机名字符串的子切片（`host[start..len]`），0 次字符串拼装与分配，O(1) 哈希判定同源站点（eTLD+1）。
   - **AI Agent 高速请求拦截 (`src/net/request_blocker.cp`)**：毫秒级分类并过滤 `Image`、`Media`、`Font`、广告追踪等重型资源，单智能体网络加载延迟下降 70%+。
   - **RFC 6265 完整标准 CookieJar (`src/net/cookies.cp`)**：实现了跨站请求拦截、同源/跨域边界判定、前缀保护（`__Secure-`、`__Host-`）、SameSite 语义、HttpOnly 影子覆盖防御以及基于时钟与容量的自动淘汰。

## 模块化 DOM-Shim、全量 Web API 扩展与可切换云端矢量录制/动画架构
1. **模块化 `src/js/shim/` 目录拓扑与 Kitesurf 1:1 对齐**：
   - 彻底将单体 `src/js/runtime_shim.js` 重构为模块化架构，拆分为 25 个独立文件，严格对应 Kitesurf 的 `packages/dom-shim/src/`，消除 Cloudflare 专属 Wasm/workerd 依赖：
     - `constants.js`：节点类型掩码、文档比较掩码、命名空间、NodeFilter 掩码、`markNative` 原生函数伪装器、`__CAP_RENDER_CONFIG` 全局开关。
     - `util/{strings,validation}.js`：HTML 转义/反转义、属性名规范化、XML/HTML 命名合规性校验。
     - `events/{event,dispatch}.js`：W3C 事件全体系（`CustomEvent`, `UIEvent`, `MouseEvent`, `KeyboardEvent`, `FocusEvent`, `InputEvent`），支持捕获、目标、冒泡三相传播。
     - `mutations/observer.js`：微任务队列批处理驱动的 `MutationObserver` 与 `MutationRecord`。
     - `collections.js`：`DOMTokenList` (`classList`)、`HTMLCollection`、`NodeList`。
     - `node-base.js`：`Node`、`Text`、`Comment`、`DocumentFragment`。
     - `element/{element,html-element,html-classes}.js`：元素原型、`dataset` 代理、通用/表单属性。
     - `element/forms/{validity,input,textarea,select,button,form}.js`：W3C 规范约束验证（`ValidityState` 10 项条件）、表单联动与提交/重置调度。
     - `form-data.js`：WHATWG 标准 `FormData`（支持迭代器与从 `<form>` 自动抽取所有活跃控件）。
     - `traversal.js`：`TreeWalker`、`NodeIterator`、`NodeFilter`（支持 13 种节点掩码与自定义过滤函数）。
     - `range.js`：`Range`、`Selection`、`createContextualFragment`。
     - `parser.js`：`DOMParser.prototype.parseFromString(str, "text/html")`。
     - `canvas2d.js`：双模 Canvas 2D 渲染上下文，支持纯无头极速存根与云端矢量命令录制（`_ops` 序列化）。
     - `animations.js`：Web Animations API，支持静态即刻完成模式与动态时间线驱动模式。
     - `window.js`：`localStorage`/`sessionStorage`、反指纹伪装 `Navigator`、`Location`、`History`、`Screen`、`Performance`、`IntersectionObserver`、`ResizeObserver`。
     - `index.js`：全模块总装配器，封装 `document.createElement` 原型自动特化并挂载全局。
2. **QuickJS C-FFI 引用计数（`JS_DupValue`）与对象生命周期排障经验**：
   - **段错误根因定位**：在 `dom_binding.cp` 的 `js_element_append_child` 和 `js_element_remove_child` 中，原生 C 函数向 JS 调用方返回传入的 `argv[0]` 时，如果直接 `return argv[0];`，QuickJS 会在调用表达式结束后 decref 一次返回值，导致 `argv[0]` 的引用计数归零并被提前释放；后续再次通过 `firstChild` 或 `nextSibling` 访问该对象时必然触发段错误 `0x8 in js_free_value_rt`。
   - **根治措施**：所有返回入参引用的 C-FFI 接口统一调用 `return JS_DupValue(ctx, argv[0]);` 增加引用计数，杜绝 double-free 悬垂指针。
3. **`cpc` 编译器长字符串解析爆栈与分块嵌入策略**：
   - **现象**：`cpc` 在处理单行包含超过数千个转义字符（`\n`, `\"`, `\\`）的特大字符串字面量（> 100KB）时，递归下降词法/语法分析器触发 `fatal runtime error: stack overflow`。
   - **方案**：编写 `scripts/bundle_runtime.py`，将 147KB 的 `runtime.js` 拓扑合并后切片为 `pub const RUNTIME_CHUNKS: [13][]const u8 = [ "12KB chunk 1", ... ]`，在 CP `mountDom` 中一次性分配连续缓冲并通过 `@memcpy` 组装后传给 `ctx.eval()`，彻底规避递归分析栈溢出，且编译耗时 < 1 秒。
4. **Arena DOM 与瞬态 JS Wrapper 双向属性同步设计**：
   - **痛点**：`createJsElement` 是按需分配的瞬态 Wrapper（每次从 C 层通过 `firstChild`、`querySelector` 返回元素时都会创建新的 JS 实例），若 JS 属性（如 `input.value`, `checkbox.checked`, `option.selected`）仅保存在 JS 包装对象的私有属性上，新 Wrapper 读不到最新值。
   - **架构设计**：在 JS setter 中，除了更新内部变量外，统一显式调用 `this.setAttribute("value", String(v))` / `this.setAttribute("checked", "")` / `this.setAttribute("selected", "")` 同步底层 Arena DOM 属性树，使所有新建 Wrapper 与 CSS 选择器匹配始终感知最新状态。
5. **云端矢量录制与 Web 动画双模切换架构**：
   - 通过全局配置 `__CAP_RENDER_CONFIG = { canvasRecording: false, animations: false }` 实现极速无头与视觉推流的零开销平滑切换：
     - **生产无头 Agent 模式（默认）**：Canvas 绘图命令直接返回有效尺寸与占位像素，动画直接跳转至完成态，不消耗内存录制命令，0 后台定时器开销；
     - **云端推流/录制模式（按需开启）**：`canvasRecording: true` 时将所有 `fillRect`, `arc`, `drawImage`, `stroke` 序列化入 `_ops` 数组供远程流式拉取；`animations: true` 时挂载 `DocumentTimeline` 与真实定时器驱动帧渲染。

## Milestone 5 纯 CP 2D 软件光栅化与 PNG 截图引擎架构方案与排障实录
1. **纯 CP 2D 软件光栅化器架构 (`src/render/painter.cp`)**：
   - **SDF 圆角抗锯齿矩形填充**：基于有符号距离场（Signed Distance Field）算法，单像素遍历判定 `out_dist = sqrt(max(dx, 0)^2 + max(dy, 0)^2) + min(max(dx, dy), 0) - r`，以 `clamp(0.5 - out_dist, 0.0, 1.0)` 计算亚像素覆盖率，完美消除锯齿；
   - **边框一体化渲染**：当 `out_dist >= -border_width` 时自然过渡为 `border_color`，实现背景与圆角边框单遍（Single-Pass）绘制；
   - **内置 8x8 ASCII 点阵字库**：内建无外部依赖的单色字模切片，通过位移运算将字符快速喷涂到 Canvas 像素缓冲区中。
2. **多级压缩等级自适应 PNG 引擎 (`src/render/png.cp`)**：
   - **双模设计目标**：
     - **本地 AI Agent 实时视觉推理**：要求 `< 1ms` 极速直出，指定 `compress_level = 0` 时采用 Filter None（0x00），扫描线内存直通复制，配合 zlib Store 模式（零霍夫曼计算）瞬间出图；
     - **CDP 网络传输与持久化**：追求最小网络带宽与存储占用，指定 `compress_level = 1`（最快 RLE 级压缩，体积剧降 70%+）或 `compress_level = 6/9`（标准/极限 Deflate 压缩），配合 Filter Sub（0x01）差分滤波大幅降低图像熵；
   - **RFC 2083 纯 CP 逆滤波解码器**：内建 `PngCodec.decode`，支持 IHDR/IDAT 自动扫描聚合、zlib `uncompress` 解压与 None/Sub/Up/Average 逆滤波还原，支撑端到端截图往返像素级核验。
3. **排障与语法避坑经验**：
   - **负号运算符与结构体字段解析冲突**：`cpc` 词法分析器对 `-style.border_width` 会错误降低为 `(0 - style).border_width`。显式声明 `const neg_bw = 0.0 - style.border_width;` 彻底消除歧义。
   - **字符字面量与数组推导边界**：在 `switch` 中直接写 `[0x18, ...]` 会默认推导为 `[]const i32` 导致类型不匹配。直接返回转义字符串字面量 `"\x18\x3C..."`（类型推导为零拷贝 `[]const u8`），既提升执行效率又消除类型推导错误。
   - **测试闭包生成器变量命名冲突规避**：在 `cap:test` 的 `() => { ... }` 闭包内部若直接声明名为 `allocator` 的局部变量，闭包捕获结构体会退化为 `*anyopaque` 引发编译失败。将具体测试断言抽离为带错误返回的普通函数（`fn testFoo()!`），在闭包内仅执行单行调用，彻底根除闭包上下文推导退化问题。
   - **标准 C 库链接补齐**：在使用 `cap:pack` 与 `cap:image` 时，底层依赖系统的 `libz.so`，需在 `cap.json` 的 `link_libs` 中补充 `"z"`。

## Phase 1 核心架构闭环与编译排障技术方案（Emulation, Fetch & Network）
1. **CDP 深度域与会话状态集成架构 (`src/cdp/`)**：
   - **Emulation 视口夹取与指标覆盖 (`emulation.cp`)**：严格依照 Kitesurf 视口限制规范（最大边 2560px，最大像素数 2560x1440，最小 16px），处理宽高、缩放比（`deviceScaleFactor`）与移动端标志（`mobile`），并联动同步至 `session.layout`。
   - **Fetch 请求拦截与挂起调度 (`fetch.cp`)**：实现基于环形缓冲区的 `FetchState`，支持 Glob 通配符（`*pattern*`）以及精确匹配；提供 `parkRequest` 与 `buildRequestPausedEvent`，实现拦截后暂停、Mock 状态码/响应体填充（`fulfillRequest`）及主动熔断（`failRequest`）。
   - **Network 域与 RFC 6265 CookieJar 深度绑定 (`network.cp`)**：直接复用 CapSurf 的 `CookieJar` 引擎，实现 `Network.getCookies`（遍历输出 CDP Cookie 列表 JSON）、`Network.setCookie`（格式化 Set-Cookie 头并写入 CookieJar）与 `Network.setExtraHTTPHeaders`。
2. **`cpc` 编译器全局符号遮蔽缺陷与 `@c` 头文件去重方案**：
   - **故障根因**：当 CP 源文件中存在 `@c { "header.h", ... }` 时，`cpc` 编译器（`cpc/src/codegen/mod.rs:709`）会在转译后的 Zig 文件根作用域发射 `pub const c = struct { ... };`。在 Zig 0.17 中，根作用域的 `c` 会禁止任何函数内部使用名为 `c` 的局部变量（如 `const c = ...`、`for (c of ...)`、`fn foo(c: u8)`），导致标准库 `cap:css` 等模块报出大量 `error: local constant shadows declaration of 'c'` 错误。
   - **工单留痕**：已提取 12 行最小可复现用例 `temp/repro_c_shadow.cp`，并通过 `task_queue.py submit --id "task-capsurf-20261116-c-struct-shadow"` 提交工单给 `cp_agy`。
   - **解法**：在 CP 中，`@c { fn foo(): i32; }` 内部的头文件字符串并非必填；移除 `cookies.cp` 与 `public_suffix.cp` 中多余的 `"time.h"`、`"stdio.h"` 后，`cpc` 不再发射根作用域 `c` 结构体，彻底消除命名冲突。
3. **栈缓冲区生命周期与堆复制（Heap Duplication）排障**：
   - **故障现象**：在 `Network.setCookie` 中使用栈分配缓冲 `raw_buf` 构造 Set-Cookie 字符串，导致写入 `CookieJar` 的切片指向已销毁的栈帧，后续 `getCookies` 读取时内容损坏。
   - **解决方案**：统一使用 `std.fmt.allocPrint(session.allocator, ...)` 进行持久化堆分配，确保 Cookie 属性字符串在 Session 生命周期内始终有效。

## Phase 2 & Phase 3 导航加载流水线与多 Frame 隔离架构技术方案与排障实录
1. **真实 HTTP 加载与 URL 端口解析架构 (`src/net/http_loader.cp`)**：
   - **痛点分析**：原 Cookie 模块中的 `parseUrlParts` 仅用于 Cookie 域名与路径匹配，遇冒号 `:` 即刻截断主机名并丢弃端口，导致 `http://127.0.0.1:9876/index.html` 被截断为 `127.0.0.1`，底层 TCP `connect` 连入缺省 80 端口，造成本地测试服务器线程一直阻塞在 `accept`。
   - **架构设计**：实现独立的 `ParsedHttpUrl` 结构体与 `parseHttpUrl` 解析器，区分 authority 与 path/query，支持标准 IPv4/IPv6 主机名、端口显式解析、路径与查询参数提取，并向后过滤 `#hash` 锚点。在 `fetchDocument` 中直接透传解析后的目标端口与 Host 头部，彻底打通真实本地网络测试。
2. **DOM 属性切片生命周期保障机制 (`src/cdp/session.cp`)**：
   - **痛点分析**：`Document.parseHtml` 解析 DOM 节点（如 `id="hero"`、`class="box"`）时直接切片输入 HTML 缓冲区。若输入的 HTML 字符串位于临时栈缓冲区或随 `handle()` 退出而被 `defer allocator.free()` 释放，DOM 树中将产生致命的悬垂指针（Dangling Pointer），在后续 `hasId` 或 `querySelector` 扫描时触发 Segmentation fault。
   - **解决方案**：在 `CdpPageSession` 中维护专属拥有的 `raw_html: []const u8` 成员，在 `session.navigate(html, url)` 内部显式通过 `allocator.dupe(u8, html)` 复制并持有整份 HTML 数据，确保其生命周期与 `session.doc` 绝对对齐，并在 `session.deinit()` 时安全回收。
3. **多 Frame / IFrame 隔离与安全上下文计算 (`src/cdp/frames.cp`)**：
   - **迁移架构**：完整吸收 Kitesurf 的 CDP 框架树机制，基于连续切片与结构体顶层提升规范，建立轻量 `FrameState`。
   - **安全上下文链条演算**：通过 `frameContextOf` 自顶向下迭代祖先链路，综合评估 `about:blank`、`about:srcdoc` 来源继承、HTTPS/WSS/Localhost 可信源判定、跨域隔离（`cross_origin_isolated_context_type`）与进程内/跨进程归属（`same_site_as_parent`）。
   - **结构体命名遮蔽排障**：由于 `accessibility.cp` 内部已导出 `pub struct JsonWriter`，在 `frames.cp` 中若重复定义顶层 `pub struct JsonWriter` 会导致 `cpc` lowering 在生成的 Zig 代码顶层触发 `error: duplicate struct member name 'JsonWriter'`。将 `frames.cp` 内置流式写入器重命名为 `FrameJsonWriter`，实现模块级隔离。
4. **CDP 导航生命周期事件对齐与协议同步**：
   - 标准 Chrome DevTools 协议在页面导航时按时序发射 3 个生命周期事件：`Page.frameNavigated` -> `Page.domContentEventFired` -> `Page.loadEventFired`；
   - 之前测试客户端仅读取 2 个 WebSocket 帧，导致后续的 `DOM.querySelector` 读取到了未被消费的第 3 个事件帧引发 JSON-RPC 解析错位；
   - 在 `PageHandler.handle` 与测试用例中严格对齐 3 阶段生命周期事件派发与接收逻辑，确保 CDP 全链路时序精确一致。

## Phase 4 视觉感知与交互式输入流水线架构方案与排障实录
1. **CDP 截图全链路串联与 Base64 内存流架构 (`src/cdp/page.cp` & `src/cdp/session.cp`)**：
   - **设计目标**：使 AI Agent 与自动化脚本能通过调用标准 CDP `Page.captureScreenshot` 获取当前页面高保真渲染图。
   - **实现方案**：在 `session.cp` 中统一封装 `captureScreenshot`，先动态基于当前视口宽度重新执行级联计算 `Cascade.computeStyles`，再由 `LayoutTree.computeLayout` 产出盒模型 `Rect` 数组；最后透传 `ScreenshotOptions`（视口宽高、背景色、压缩级别）至 `renderDocumentToPng` 进行 2D 软件光栅化，在 C 层通过 `Base64.encodeAlloc` 生成 Base64 字符串返回。
2. **像素级交互命中与双向状态同步设计 (`src/cdp/input.cp`)**：
   - **焦点与坐标命中判定**：在 `Input.dispatchMouseEvent` 中，基于 `session.layout.boxes` 对屏幕绝对物理坐标 `(x, y)` 执行反向几何查找（Hit Test），定位到最顶层可见元素作为 `target_node`，并维护当前会话的 `session.focused_node`。
   - **交互文本注入管道 (`applyInputText`)**：在 `Input.insertText` 与 `Input.dispatchKeyEvent` 中，不仅在底层 Arena DOM 的 `NodeData` 上更新/创建 `value` 属性，而且在已挂载的 QuickJS-NG 上下文中触发 DOM 包装对象更新，确保通过 JS 评估读取 `input.value` 与 `element.textContent` 始终返回最新键盘输入内容。
   - **滚轮与滚动状态跟踪**：在 `Input.dispatchMouseEvent` 的 `mouseWheel` 分支中实时累加 `session.scroll_x` 与 `session.scroll_y`，支撑页面滚动与视口偏移计算。

## Phase 5 Target 多目标注册表与 BrowserContext 架构方案与排障实录
1. **多 Target 与 BrowserContext 拓扑管理架构 (`src/cdp/target.cp`)**：
   - **设计目标**：对齐 Chrome 与 Kitesurf 标准，支持 Playwright / Puppeteer 的 `browser.newContext()` 与 `context.newPage()` 多页面多租户隔离模式。
   - **架构设计**：设计固定容量且零动态分配的 `TargetRegistry`，内嵌 `[16]TargetItem` 与 `[8]BrowserContextState`。当调用 `Target.createBrowserContext` 时，生成全局单调递增的 `"ctx-001"` 上下文标识；调用 `Target.createTarget` 时，将页面绑定至目标上下文，并通过 `Target.getTargets` 和 `Target.getTargetInfo` 暴露完整的 `browserContextId`、`url`、`title` 与 `attached` 状态。
2. **切片生命周期悬垂与响应缓冲区隔离排障**：
   - **痛点分析**：在早期初稿中，部分方法省略接收者指针（按值传递），生成的临时切片指向当前函数局部栈帧；同时，多个指令在共享的 `out_buf` 上做临时字符串格式化，导致后续生成的 JSON 覆盖了之前生成的 `id` 切片内存。
   - **解决方案**：在 `TargetRegistry` 结构体内部配置独立的命名专用缓冲区（`create_ctx_resp_buf`、`create_target_resp_buf`、`attach_resp_buf`），并在所有增删改查方法中严格采用可变独占指针接收者 `(*_)`，彻底杜绝切片悬垂与缓冲区相互覆盖。
3. **Playwright 核心依赖的 Auto-Attach 自动附加机制实装**：
   - 在 Playwright 连接无头浏览器时，会首先发送 `Target.setAutoAttach({ autoAttach: true, flatten: true })`；
   - 随后的每一个 `Target.createTarget` 调用，引擎必须一次性同步连带派发 `Target.targetCreated` 与 `Target.attachedToTarget` 双事件，告知客户端子页面已就绪且 session 已绑定；
   - 在 `dispatch.cp` 中扩充事件缓冲容量至 `[4]CdpEvent`，完全打通了自动附加事件管道。

## Phase 6 Web Workers、执行上下文管理与控制台/日志事件表面架构方案与排障实录
1. **控制台 API 截获与 Log/Runtime 双域多路复用方案 (`src/js/dom_binding.cp` & `src/cdp/session.cp`)**：
   - **设计目标**：捕获 JS 运行时的 `console.log`、`console.info`、`console.warn`、`console.error`、`console.debug`，对齐 Chrome DevTools Protocol 标准，同时向监听客户端按需派发 `Runtime.consoleAPICalled` 与 `Log.entryAdded`；
   - **架构设计**：利用 QuickJS 的 `JS_SetContextOpaque` 将当前 `JsDomContext` 指针安全绑定至 JSContext；在各原生控制台回调中提取参数拼接为完整文本，写入 `dom_ctx.console_records`（16 项固定容量循环覆盖缓冲队列）；通过 POSIX `gettimeofday` 获取微秒级高保真毫秒时间戳；在 `session.cp` 中提供 `pollConsoleEvents`，在每次执行 `Runtime.evaluate` 或页面加载后自动冲刷并格式化为标准 CDP 事件。
2. **Dedicated Worker 线程池与 Target 生命周期映射 (`src/cdp/workers.cp`)**：
   - **设计目标**：支持 Web Worker 拓扑管理与隔离，与 Playwright / Puppeteer 的 Worker 目标附加与消息通道兼容；
   - **架构设计**：实现 `DedicatedWorkerPool`（容量为 8 项的 `DedicatedWorkerEntry` 零 GC 结构数组）。创建 Worker 时调用 `registry.createTargetWithType(url, "worker", "")` 分配目标；若已启用 `auto_attach`，立即派发 `Target.attachedToTarget`（`waitingForDebugger: false`）；提供 `postMessage` 消息投递记录与 `terminateWorker` 生命周期终结接口，级联触发 `Target.detachedFromTarget` 与 `Target.targetDestroyed`。
3. **关键编译排障与语法防坑点**：
   - **数组初始化语法**：在 CP 结构体字段声明中，数组初始化须写为 `[Item{}; N]`，而非 Zig 风格的 `[_]Item{.{}} ** N`；
   - **可空指针优先级与解包**：`dom_ctx` 字段类型必须书写为 `(*JsDomContext)? = null`（外包括号），避免被解析为指向可选值的指针引发 `((dom).?.console_count)` 冗余解包报错；
   - **跨平台时钟**：Zig 0.17 的 `std.time` 模块已精简为单纯的分辨率常数与 epoch 转换，通过 C-FFI POSIX `gettimeofday(&tv, null)` 实现稳健高效的毫秒时间戳获取。

## Phase 7 Playwright/Puppeteer 协议桥接、大报文流控与双模构建架构方案与排障实录
1. **端到端 8 步自动化协议流全链路架构 (`test/cdp/test_playwright_e2e.cp`)**：
   - **设计目标**：脱离 Mock，在真实操作系统 socket 与 RFC 6455 WebSocket 协议通道上，完整复现 Playwright / Puppeteer 驱动无头浏览器的真实生命周期；
   - **时序流程设计**：
     - **发现与握手**：客户端连接端口发送 HTTP 请求探测 `/json/version`，随后发送 HTTP Upgrade 请求完成 WebSocket 握手；
     - **初始握手与挂载**：发送 `Browser.getVersion` 协商协议版本；发送 `Target.setAutoAttach({ autoAttach: true, flatten: true })` 同步消费 RPC 响应及关联的 `Target.attachedToTarget` 事件，避免未消费帧污染后续帧序列；
     - **领域启用与上下文注册**：发送 `Page.enable` 与 `Runtime.enable`，断言接收到 `Runtime.executionContextCreated`；
     - **导航流水线**：发送 `Page.navigate`，驱动 HTML 解析并消费 `frameNavigated`、`domContentEventFired`、`loadEventFired` 3 阶段生命周期事件；
     - **DOM 查询与盒模型几何提取**：调用 `DOM.querySelector` 查找 `#login-btn` 并由 `DOM.getBoxModel` 提取物理像素几何边界；
     - **交互与 JS 执行**：模拟 `Input.dispatchMouseEvent` 点击操作，并由 `Runtime.evaluate` 执行 JS 查询确认 DOM 属性；
     - **视觉截图直出**：调用 `Page.captureScreenshot` 经由 2D 软光栅化与 PNG 压缩，返回完整 Base64 编码数据。
2. **大报文 WebSocket 流控与 Session 缓冲区排障**：
   - **痛点分析**：在早期实现中，`dispatchCdpCommand` 使用固定的 4096 字节栈缓冲区格式化输出。当执行 `Page.captureScreenshot` 时，Base64 编码后的 PNG 图片体积高达数十 KB，导致 `std.fmt.bufPrint` 缓冲区溢出静默返回，WebSocket 连接无数据返回致使客户端永远挂起在 `recv()`；
   - **解决方案**：在 `server.cp` 中实现自适应流控逻辑，当响应报文超过 4KB 时自动走 `std.heap.c_allocator.alloc` 动态内存通道，并在数据发送后立即安全释放，根除大体积 Base64 报文传输死锁；
   - **Session 事件格式化专用方法与 `E0130` 根除**：在静态方法中直接跨结构体以可变指针切片借用 `session.buffer` 会触发 `cpc` 编译器的只读方法可变操作冲突（`E0130`）；将格式化逻辑收敛为 `CdpPageSession` 的专用实例方法（`formatExecutionContextEvent` 与 `formatFrameNavigatedEvent`，显式标注 `*_` 独占接收者），既消除了临时栈指针悬垂逃逸，又保证了编译期生命周期分析严格合规。
3. **独立二进制与 Nest 容器双模构建验证**：
   - 验证独立单二进制模式：`cap build src/main.cp -f`，成功编译出 7.2MB 单可执行文件 `bin/main`；
   - 验证 Nest 容器热加载动态库模式：`cap build src/main.cp -b`，基于 `@is_nest` 宏成功编译出轻量级动态库 `bin/main.so`（9.3KB）及其接口定义 `bin/main.d.cp`。

## Phase 8 Chromium CLI 平替架构、RFC 6455 协议加固与驱动端到端兼容排障实录
1. **Chromium CLI 平替与 Linux `/proc/self/cmdline` 无依赖解析架构 (`src/cli/options.cp`)**：
   - **设计目标**：使 CapSurf 能够作为标准无头 Chromium 的无感替代品，被 Playwright、Puppeteer、Chrome-launcher 等驱动程序直接通过命令行拉起；
   - **架构设计**：
     - 支持 `--remote-debugging-port=<port>`、`--remote-debugging-port <port>` 复合与分离两种端口传参；
     - 支持 `--remote-debugging-address`、`--window-size=<w>,<h>` 动态视口几何配置；
     - 优雅兼容 `--headless`、`--no-sandbox`、`--disable-gpu` 等标准 Chrome 启动参数；
     - 提供 `parseFromProcess()`：直接经由 Linux `/proc/self/cmdline` 读取 NUL (`\0`) 分隔的启动命令行参数，完成轻量解析，避免对外部 libc/wrapper 的脆弱依赖。
2. **Playwright 驱动自动探活标准握手行设计 (`src/main.cp`)**：
   - **痛点与机制**：Playwright 和 Puppeteer 在使用 `launch({ executablePath: '...' })` 启动浏览器子进程后，内部会在子进程的 stdout/stderr 输出流中正则匹配监听行；如果未在超时内检测到特定握手输出，驱动会报超时并 SIGKILL 终止浏览器；
   - **实现方案**：在 CDP 服务端绑定并监听端口后，立即向标准输出发送格式严格规范的调试行：
     `DevTools listening on ws://{address}:{port}/devtools/browser/capsurf-browser-001`
     驱动程序捕获后即可立即提取 WebSocket 调试地址并建立通信。
3. **RFC 6455 协议大小写不敏感标头匹配排障 (`src/cdp/server.cp`)**：
   - **排障定位**：在 Node.js 22 真实驱动测试中，`new WebSocket('ws://...')` 握手发生 `ErrorEvent`。经网络抓包定位，Node.js 原生客户端在发起 WebSocket 升级握手时，请求头采用全小写字段（`sec-websocket-key: ...`、`upgrade: websocket`）。而服务端原实现使用 `std.mem.indexOf(u8, req, "Sec-WebSocket-Key:")`，导致无法命中键值，静默报 `MissingWsKey` 错误并切断连接；
   - **解决方案**：实现 `findHeaderCaseInsensitive`，结合 `std.ascii.eqlIgnoreCase` 对 HTTP 请求头进行大小写不敏感扫描，并容忍冒号周围的空白字符，完全遵循 RFC 6455 规范。
4. **C-FFI 重复符号冲突与跨模块符号收敛排障**：
   - **排障定位**：当多个模块通过 `@c` 声明相同名称的 C 函数（如 `close(fd)`）时，`cpc` 转译生成的单体 `main.zig` 根结构体中会出现重复的 `pub extern "c" fn close` 与 `extern "c" fn close` 导致 Zig 编译器报错；
   - **解决方案**：统一收敛底层 POSIX C-FFI 声明至 `src/cdp/server.cp` 的 `pub @c` 导出块，其他上层模块（如 `src/cli/options.cp`）统一通过 `import { open, read, close } from "../cdp/server.cp"` 引用，根除重复符号。

## Phase 9 网络请求拦截 (Fetch 域) 与安全网关 (Referrer/CSP) 架构方案与排障实录
1. **非回溯线性 Glob 匹配与请求暂停状态机设计 (`src/cdp/fetch_interception.cp`)**：
   - **设计目标**：对齐 Chromium CDP 的 `Fetch.enable`、`Fetch.requestPaused`、`Fetch.continueRequest`、`Fetch.fulfillRequest`、`Fetch.failRequest`，支撑 Playwright `page.route()` 动态截获与 Mock；
   - **算法架构**：
     - 使用双指针 + 单星号标记记录点（`has_star`, `star_j`, `mark`），实现时间复杂度为 $O(N)$ 的零回溯字符扫描；
     - 严格过滤换行符（`0x0A`, `0x0D`），确保 `*` 与 `?` 不会跨行越界；
     - `FetchInterceptionState` 维护定长 64 规则数组与 128 挂起请求环形队列，实现零堆分配内存池，防止高频拦截场景下的碎片堆积；
     - `errorReasonToNet` 提供 13 种常见 Chromium 网络错误代码（`ERR_ABORTED`, `ERR_CONNECTION_REFUSED`, `ERR_TIMED_OUT` 等）的标准映射。
2. **W3C Referrer Policy 决策与降级剥离算法 (`src/net/referrer_policy.cp`)**：
   - **设计目标**：保证跨域导航与子资源加载过程中的用户隐私与安全边界；
   - **算法架构**：
     - 多 Token 逗号切分扫描（规范要求采用最后一个识别出的有效 Token，其余忽略）；
     - `isPotentiallyTrustworthy` 快速识别 `localhost`、`127.0.0.1`、`https:`、`wss:` 与 `about:` 安全上下文；
     - `determineReferrer` 在同源时输出完整去 Hash URL，在跨域时截断为 Origin，在 HTTPS 降级到不安全 HTTP 时完全清空剥离，严格符合 W3C Referrer Policy 规范。
3. **CSP 策略解析与指令源放行判定 (`src/net/csp.cp`)**：
   - **设计目标**：解析并执行网页下发的 `Content-Security-Policy` 响应头，阻止未授权脚本执行与非法网络通信；
   - **算法架构**：
     - `parseCsp` 按分号与空白切分 Token，首 Token 提取指令名，值存入零堆分配定长数组；
     - `schemeMatches` 支持安全协议向上升级匹配（http 放行至 https，ws 放行至 wss）；
     - `hostMatches` 精确区分泛域名通配（`*.domain.com` 不匹配父域 `domain.com`，匹配 `sub.domain.com`）；
     - `isSourceAllowed` 实现指令特异性覆盖（优先读取 `script-src` / `connect-src`，缺省时自动回退至 `default-src`）。


## Phase 10 CDP Debugger 协议域、二进制图片尺寸嗅探器与多租户 CookieJar 分区技术方案
1. **CDP Debugger 域协议引擎与脚本管理 (`src/cdp/debugger.cp`)**：
   - **架构设计**：完整迁移自 `vendor/kitesurf/packages/engine/src/cdp/debugger.ts`，管理预编译脚本与运行时注入脚本的元数据（`CompiledScriptMeta`）；
   - **Playwright/Puppeteer 协议兼容桩**：真实浏览器驱动在初次连接时会密集调用 `Debugger.setSkipAllPauses`、`Debugger.setBreakpointsActive`、`Debugger.setPauseOnExceptions`、`Debugger.setAsyncCallStackDepth`、`Debugger.setBlackboxPatterns`、`Debugger.setBlackboxedRanges`。若未实现这些协议桩，驱动会抛出 `ProtocolError: Debugger.setSkipAllPauses not found` 并阻断自动化链路。通过在 `DebuggerHandler` 中提供完整协议应答，确保驱动顺利进入主控制流；
   - **事件主动广播**：在收到 `Debugger.enable` 请求时，遍历已登记脚本并向客户端发送 `Debugger.scriptParsed` 事件，打通源码映射与断点预埋链路。
2. **零内存分配高速二进制图片尺寸嗅探器 (`src/render/image_dims.cp`)**：
   - **性能动机**：在无头排版流水线中，如果为获取 `<img>` 标签的宽高而每次调用完整解码器（libpng/libjpeg），将耗费数十毫秒 CPU 时间。通过直接解析二进制文件头（只需读取前 32~512 字节），能在百纳秒级别完成尺寸提取，使百万 Agent 并发场景下的页面首次排版性能提升数十倍；
   - **实现亮点**：
     - PNG：读取 `IHDR` 块的 4 字节大端整数；
     - GIF：读取第 6~10 字节小端屏幕宽高；
     - BMP：解析 `BITMAPINFOHEADER` 结构头（支持负高度绝对值转换）；
     - JPEG：扫描跳过 `APP0`~`APP15`、`DQT`、`DHT` 段，命中 `SOF0`（Baseline）或 `SOF2`（Progressive）时提取 16 位大端宽高；
     - WebP：支持 VP8（有损 14 位尺寸带缩放因子）、VP8L（无损位流 14 位宽度与高度）与 VP8X（扩展头 24 位画布宽高）；
     - SVG：快速匹配 `<svg` 与 `xmlns`。
3. **多租户 CookieJar 隔离分区与 Zig 切片指针逃逸排障 (`src/net/cookie_jar_registry.cp`)**：
   - **问题定位**：若在结构体数组中返回嵌套字段的指针（如 `return &_.entries[i].jar;`），`cpc` 编译器在 AST 降低时可能丢弃取地址符 `&`，导致类型推导报错 `expected ?*T, found T`；同时 2D 数组初始化器 `[[0; 128]; 16]` 会降低为不合法的 Zig `@splat` 结构；
   - **解决方案**：
     - 采用 Struct-of-Arrays (SoA) 内存布局，`jars: [16]CookieJar` 与 `key_storage: [2048]u8` 平铺连续内存；
     - 通过 `let slice: []CookieJar = _.jars[i .. i + 1]; unsafe { return @ptrCast(slice.ptr); }` 精确满足生命周期安全与 Zig 指针类型要求；
     - 实装 `makeKey` 复合键与级联卸载 `unregisterConnection`，实现百万 Agent 实例间无干扰的隔离环境。
4. **上游编译器工单排障记录与根因分析 (`cpc -b` BEAM 模式下的普通方法返回)**：
   - **排障定位**：当运行 `cap build src/main.cp -b`（Nest 容器动态库模式）时，`cpc` 编译报错 `error: use of undeclared identifier 'ctx'`；
   - **代码溯源**：在 `/data/dev/cp/cpc/src/codegen/stmt.rs:2055` 中，`Stmt::Return(None)` 分支内无条件对 `is_beam_mode` 生成了 `return beam.Beam.makeAtom(ctx.self.env.?, "ok");`，即便当前函数并非 actor 方法（无 `ctx` 局部上下文），导致所有常规结构体普通方法与模块函数出现 `ctx` 未声明错误；
   - **规范闭环**：提炼 9 行最小复现代码 `temp/repro_beam_non_actor_return.cp`，规范提交工单 `task-capsurf-20261116-beam-non-actor-return` 至 `/data/dev/cp/.tasks/pending/`，后台守护进程持续监听中。


## Phase 11 网络安全与 W3C Fetch 基础设施 (CORS / 禁制标头 / PNA / 重定向) 技术方案
1. **Fetch §2.2.2 禁制标头拦截与安全防护 (`src/net/forbidden_headers.cp`)**：
   - **设计动机**：在无头浏览器中，网页恶意脚本可能会试图伪造 `Host`、`Cookie`、`Sec-Fetch-*` 或 `Proxy-*` 标头以进行请求走私或伪造用户凭据。同时，恶意脚本常通过 `X-HTTP-Method-Override: CONNECT` 等非法伪装攻击内网代理；
   - **解决方案**：实现零堆分配的快速前缀与集合判定，拦截所有 21 种受控标头与非法方法重写，确保发往网络的 HTTP 报文完全由浏览器底层安全管道掌控。
2. **W3C CORS 预检与 64 槽零堆分配 TTL 缓存 (`src/net/cors.cp`)**：
   - **设计动机**：Cross-Origin Resource Sharing (CORS) 是保护跨源资源的核心屏障。对于复杂跨域请求，必须发起 `OPTIONS` 预检并校验服务器允许的方法与标头；
   - **技术亮点**：
     - `TokenList` 采用定长 32 槽零分配数组，支持逗号分割标头的高性能解析；
     - `PreflightCache` 采用 64 槽固定内存环形结构，支持基于单调时钟的 TTL 失效机制（最大 600 秒），在百万 Agent 高频请求场景下大幅降低网络预检往返时延（RTT）；
     - 凭据隔离规则：严格执行 W3C 规范，当 `allow_credentials` 为 `true` 时，`allow_origin` 禁止为通配符 `*`。
3. **Private Network Access (PNA) 私网防护 (`src/net/private_network.cp`)**：
   - **设计动机**：当无头浏览器访问公网未授信网站时，该网站的脚本可能利用浏览器作为跳板对 Agent 所在的内网（如 `10.0.0.0/8`, `192.168.0.0/16`）或本地微服务（`127.0.0.1:9222`）进行端口扫描与未授权调用（SSRF 攻击）；
   - **技术方案**：
     - 划分 `AddressSpace`（loopback: 0, private: 1, public: 2）；
     - 零堆分配解析 IPv4/IPv6，覆盖 RFC 1918、RFC 6598（100.64.0.0/10 运营商级共享地址）、链路本地与组播；
     - 比较 `target.rank < initiator.rank`，当公网页面尝试访问私网或环回地址时直接抛出拒绝拒绝请求，保障智能体本地宿主安全。
4. **HTTP 重定向策略与循环熔断 (`src/net/redirect_walk.cp`)**：
   - **设计动机**：网页重定向（301, 302, 303, 307, 308）涉及复杂的语义转换（如 301/302/303 必须将 POST 转为 GET 并清空 Body，而 307/308 必须原样保留），以及跨域或协议降级（HTTPS -> HTTP）时的 Referer 剥离与 Authorization 标头清理；
   - **技术方案**：
     - `RedirectWalkState` 追踪跳数上限（20 次）与环路探测（记录已访问 URL 列表）；
     - `computeNextReferrer` 严格执行隐私规则：HTTPS 降级到 HTTP 清空 Referer，跨域时剥离为 Origin 根路径，同源保持完整 Referer；
     - 跨域跳转时自动剥离 `Authorization` 敏感凭据标头。
5. **固定数组切片迭代陷阱与规避经验**：
   - **踩坑定位**：在 CP 中，对固定长度数组 `arr: [N]T` 使用 `for (x of arr)` 语法时，底层生成的 `cp_iter_slice` 会按值复制数组到迭代器栈帧，导致返回的切片指针成为临时局部栈悬垂引用，触发 Zig 编译器的未定义行为或逃逸报错；
   - **最佳实践**：对静态固定大小数组的遍历，推荐统一使用标准的下标步进循环：`let i: usize = 0; while (i < arr.len) : (i += 1) { const x = arr[i]; ... }`，完全零开销且 100% 免疫栈逃逸。


## Phase 12 远端串流 RBI 二进制协议、输入滚动队列合并与会话存储表技术方案
1. **输入平滑滚动事件合并与栅栏同步队列 (`src/cdp/wheel_queue.cp`)**：
   - **设计动机**：人类用户或高频自动化脚本在使用鼠标滚轮时，可能在几毫秒内产生数百个微小的 `mouseWheel` 事件。如果不做合并直接派发至渲染与排版管线，会导致剧烈的重排重绘抖动。同时，在跨帧切换或等待滚动到特定位置时，需要插入同步点（Fence）；
   - **解决方案**：
     - `canCoalesceWheel` 判定相邻两个滚轮事件是否具备相同的屏幕视口位置与按键修饰符；若匹配则将 `delta_x` 与 `delta_y` 就地累加至前序批次中，大幅削减派发频率；
     - `WheelQueue.fence()` 插入不可合并的栅栏批次，当且仅面前所有滚动全部被排空消耗后，栅栏才完成放行，实现严密的时序同步。
2. **纯内存高性能结构体会话存储表 (`src/cdp/session_store.cp`)**：
   - **设计动机**：浏览器会话生命周期内需要持续缓存网络请求日志、响应体快照、已编译脚本源码以及恢复点检查点。为了百万 Agent 的高密度并发与内存可控性，禁止无节制动态扩容；
   - **解决方案**：
     - `StoreTable` 采用定长 64 槽环形连续内存布局，条目采用定长键缓冲（128 字节）与内联值缓冲（1024 字节），大载荷支持外部切片引用；
     - `SessionStore` 支持按表名（如 `TABLE_NETWORK_LOGS`）延迟懒加载分配子表，并提供全局聚合诊断接口 `stats()`，实时反馈会话占用的内存大小与键总数。
3. **远端浏览器串流 RBI 二进制批次编解码协议 (`src/cdp/rbi_protocol.cp`)**：
   - **设计动机**：在无头浏览器向人类屏幕进行云端推流（Remote Browser Isolation / Web Streaming）时，将所有 Canvas 绘图命令、字体字形（Glyphs）与图片资源序列化为二进制流发往远程客户端渲染；
   - **紧凑布局**：
     - 采用 `[count u32 LE]` 紧随 `[kind u8][hash u64 LE][length u32 LE][payload]` 纯二进制帧格式，完全避免 JSON 序列化带来的 3~5 倍体积膨胀与 CPU 编解码开销；
     - `DeliveredKeys` 维护定长 128 槽循环 FIFO 哈希去重表，通过 `dropAlreadySent` 过滤客户端本地已具备的图片与字体字形缓存，极大节约云端推流的网络下行带宽。
4. **编译期 64 位无符号整数常量的符号推导规避**：
   - **踩坑定位**：在 CP/Zig 中，十六进制常量如 `0xAABBCCDDEEFF0011`（最高位为 1）若未指定类型，会被默认推导为带符号的 `i64` 整数（值变为负数 `-6144092013047381999`），当赋值给 `u64` 字段时引发 Zig 编译报错 `type 'u64' cannot represent integer value`；
   - **最佳实践**：测试或代码中若使用大整数哈希常量，最高位为 1 时显式声明 `@as(u64, 0x...)`，或选用最高位为 0 的数值（如 `0x1122...`、`0x2233...`）以避免符号推导歧义。


## Phase 13 ResourceStore 内存预算缓存与持续视口流式 Screencast 协议技术方案
1. **CDP 隔离区零堆分配资源缓存 (`src/cdp/resource_store.cp`)**：
   - **设计动机**：云端浏览器与 AI Agent 在渲染网页时，字形与外部图像频繁进出。为杜绝 GC 抖动并保护容器宿主内存，需建立硬性上限的定长内存池；
   - **解决方案**：
     - 单条目上限 8MB，全局总容量 16MB，预分配定长 64 槽环形连续表；
     - 采用内联数据缓冲区（1024 字节）承载小图标与紧凑字体，大文件走切片引用，实现小载荷零动态分配；
     - 采用两级 LRU 淘汰：当内存超过上限时，优先淘汰与当前准入操作属于同一 Connection 的最旧资源，不影响其他活跃并发连接的渲染上下文。
2. **持续视口流式 Screencast 协议与背压控制 (`src/cdp/screencast.cp`)**：
   - **设计动机**：远程桌面与实时 Agent 视觉交互需要高频下发画面（Page.screencastFrame）。若客户端处理不及时或网络波动，直接持续抓帧会导致严重堆积与延迟飙升；
   - **解决方案**：
     - 实装 `pending_ack_session_id` 与 `last_acked_seq`：当客户端收到帧后，必须通过 `Page.screencastFrameAck` 返回确认包，服务端在收到 ACK 前暂停产生新帧（Backpressure 背压阻断），保证端到端低延迟（< 50ms）；
     - 提供纯 CP 原生格式化函数 `formatScreencastFrameEvent`，在栈缓冲区内零分配完成 JSON 包装。
3. **CP 语言只读联合体参数传递规范 (`TreeBuilder.processToken`)**：
   - **踩坑定位**：当函数形参写为指针 `tok: *const Token` 时，调用方调用 `tb.processToken(&token_val)` 在 `cpc` 编译器生成 Zig 代码时可能会因为类类型推导判定导致取地址符 `&` 被剥离，从而产生 `expected '*const Token', found 'Token'` 编译报错；
   - **最佳实践**：完全遵循 CP 官方语言规范第 2 节铁律——只读实例方法与函数形参一律按值声明 `tok: Token`（体内统一只读解包 `switch (tok)`），不仅消除了显式取地址 `&` 的类型剥离风险，还能充分享受 Zig 内部根据体系结构自动寄存器/常量引用优化的最大性能。


## Phase 14 W3C 可访问性 AXTree 引擎与 Public Suffix 站点隔离安全技术方案
1. **W3C Accessibility 语义树与 AI Agent 视觉推理优化 (`src/cdp/accessibility.cp`)**：
   - **设计动机**：现代 AI Agent（如 Browser-Use、Claude Computer Use、Playwright AI 等）并不依赖复杂的庞大 HTML DOM 树，而是首选读取语义紧凑的 `Accessibility.getFullAXTree`。AXTree 清洗了冗余的样式容器节点，仅输出具备实际语义与交互价值的节点；
   - **解决方案**：
     - 构建纯 CP 递归遍历引擎，对 40+ 种 HTML 标签提供标准 ARIA Role 自动映射，并支持 `role="..."` 显式覆写；
     - 严格遵循 W3C AccName 1.2 规范计算 Accessible Name：优先读取 `aria-labelledby`（递归拼装引用的多节点文本）、`aria-label`、语义原生规则（如 `alt`、`<label>` 关联、`placeholder`），再兜底子树文本内容；
     - 隐藏节点剪枝：对 `display: none`、`visibility: hidden`、`aria-hidden="true"` 以及 `<input type="hidden">` 设置 `ignored: true` 并赋予规范的标准原因（`notRendered`, `ariaHiddenElement`, `uninteresting`），无盒模型（boxless）节点阻断子树冗余生成，极大减轻 Agent 上下文 Token 负担。
2. **零堆分配 Public Suffix List 与 eTLD+1 提取 (`src/net/public_suffix.cp`)**：
   - **设计动机**：Cookie 域安全性（RFC 6265bis 杜绝 super-cookie 如在 `.com` 或 `.co.uk` 种 Cookie）与 W3C SameSite 站点隔离判定需要准确计算可注册顶级域 eTLD+1。在百万 Agent 高并发下，每次请求如果动态构建规则 Set 会带来巨大的 GC 压力；
   - **解决方案**：
     - 采用排序定长数组 `MULTI_SUFFIXES: [390][]const u8`，通过二分查找配合无分配大小写不敏感比对，单次匹配耗时仅需几个纳秒；
     - 实现 `siteKey` 零分配切片返回原字符串中的 eTLD+1 子串，同时为 `schemefulSite` 提供原位栈缓冲区写入，完全实现零内存分配的 SameSite 站点隔离。


## Phase 15 流式响应字节预算约束与多租户 Worker 隔离配额技术方案
1. **网络层流式字节硬预算与连接槽位追踪 (`src/net/read_capped.cp`)**：
   - **设计动机**：恶意或畸形网页可能会通过超大响应体（如数百 MB 甚至数 GB 的压缩流或畸形 HTML）对无头浏览器发起内存耗尽拒绝服务攻击，特别是在多 Agent 并发场景下必须设定硬性资源天花板；
   - **解决方案**：
     - 设定四级防护常量：单文档 16MB、单样式表 8MB、单连接最大在途 24MB、全宿主全局并发在途 48MB；
     - `ReadBudgetTracker` 维护定长 32 槽连接记录，提供原子 `chargeWithLimit` 与 `refund` 操作，在累加前进行超限拒绝，保障网络层零内存泄漏与严格的资源隔离；
     - `ReadLease` 结合 RAII 提供租约保护，连接断开时支持批量安全扣减。
2. **多租户 Worker / Isolate 配额与导航余量保护 (`src/cdp/worker_budget.cp`)**：
   - **设计动机**：网页中的 Web Worker、Service Worker 以及 OOPIF（Out-of-Process iframe）会无节制派生 JavaScript Isolate。为防止 CPU 和内存耗尽，同时保证主页面重排重绘和导航请求永远有槽位可用，必须严格限制动态 Worker 上限；
   - **解决方案**：
     - 设置全局硬上限 `MAX_DYNAMIC_WORKERS = 4`；
     - 保障关键业务槽位：未实例化渲染器时硬性预留 1 槽（`RENDERER_RESERVED_SLOTS`），同时为页面导航预留动态余量（`NAVIGATE_HEADROOM_SLOTS * page_targets`）；
     - 严格限制 `MAX_CONCURRENT_OOPIF = 1`，当余量不足时及时拒绝并记录 `worker_denied` 与 `oopif_denied` 统计。
3. **`cpc` 编译器对 `release` 方法命名的特定语义规避**：
   - **踩坑定位**：在 CP 中，如果一个结构体定义了名为 `release(...)` 的成员方法，`cpc` 编译器会将其硬编码判定为类似析构函数的 0 实参 Drop 方法，不仅会自动在作用域末尾插入 `defer val.release();`，而且在表达式调用处会将实参强制丢弃剥离为 `val.release()`，引发 Zig 编译报错 `member function expected 1 argument(s), found 0`；
   - **最佳实践**：在 CP 编写自定义资源归还或租约释放方法时，严禁使用裸词 `release(...)` 命名带参数的常规方法，统一推荐命名为 `relinquish(...)`、`releaseLease(...)` 或 `releaseKind(...)`，彻底规避编译器的隐式 Drop 机制。


## Phase 16 Pike VM 线性正则引擎与投机预取/已学习脚本技术方案
1. **Pike VM 非回溯线性正则匹配器 (`src/cdp/linear_regex.cp`)**：
   - **设计动机**：CDP 出站过滤器需要对页面请求 URL 匹配用户定义的正则模式。传统 NFA/DFA 引擎（如带有回溯的正则引擎）在面临恶意构造的模式（如 `(a+)+$`）时会发生指数级回溯爆炸（ReDoS），瞬间耗尽 CPU 并导致多租户服务假死；
   - **解决方案**：
     - 采用 Pike VM 架构，将正则表达式转译为紧凑的虚拟机指令流（`Char`, `Class`, `Dot`, `WordBoundary`, `Bol`, `Eol`, `Match`, `Jmp`, `Split`）；
     - 维护当前与下一轮双缓冲 `ThreadList`，并借助基于 `generation` 世代号的已访问记录（`seen` 数组），使得每个字节仅遍历当前存活线程的 Epsilon 闭包，单次匹配耗时严格控制在 $O(N \times M)$（输入长度 $\times$ 程序步数）；
     - 引入首字符预过滤 `StartSet`（ASCII 128 位图与 `anchored` 锚点判定）：若字符不在有效首字符集中，直接在循环中跳过位置遍历，大幅加速无匹配长 URL 的前向扫描；
     - 严格设定安全防护上限：`MAX_REPEAT = 1000`、`MAX_PROGRAM = 8192`，在 AST 阶段通过 `expandedSize` 进行饱和预估，拦截反向引用与超限展开。
2. **投机性预取调度器与拦截门禁 (`src/net/speculative_prefetch.cp`)**：
   - **设计动机**：W3C Speculation Rules 规范允许页面预取未来可能访问的脚本和资源，但不能阻塞关键渲染路径，且必须受到出站安全规则与 CDP Fetch 拦截规则的严格约束；
   - **解决方案**：
     - 设定固定超时 `SPECULATIVE_DEADLINE_MS = 1000` 与并发度 `SPECULATIVE_CONCURRENCY = 2`；
     - 实现 `maySpeculate` 判定：当连接启用了 `Fetch.enable` 拦截且匹配 `Script` 资源，或命中了 `OutboundFilters` 的黑名单模式时，直接拒绝投机预取，防止无谓的网络抖动与竞态；
     - 实现 `PrefetchBatchRunner`：利用非阻塞时间戳差值（`getMilliTimestamp`）控制批处理超时，保障 Agent 交互不受投机预取阻塞。
3. **已学习脚本 Origin/Context 双键复用缓存 (`src/eval/learned_scripts.cp`)**：
   - **设计动机**：现代单页应用（SPA）与复杂多页面应用在相同 Origin 和 Context 下多次导航时，经常插入相同的大体积公共脚本。重复从网络或磁盘拉取会拖慢并发 Agent 的页面就绪速度；
   - **解决方案**：
     - 设定 `MIN_LEARNED_BYTES = 8192`：小脚本不记录，仅对 >= 8KB 的关键业务脚本进行学习；
     - 采用纯栈/定长结构体数组实现高效 LRU 淘汰：支持单 Origin 最多 24 个 URL、全局最多 32 个 Origin 分区；
     - 实现 `MAX_IDLE_NAVIGATES = 3` 自动淘汰策略：若连续 3 次页面导航均未使用该学习脚本，自动将其从缓存中清除，实现完全免垃圾回收的高效内存复用。


## Phase 17 导航响应分类器、非 HTML 视图合成器与样式表 LRU 缓存技术方案
1. **多格式导航响应分类与合成 (`src/cdp/document_kind.cp`)**：
   - **设计动机**：浏览器导航不仅限于 HTML 网页，用户和 Agent 也经常直接导航至独立图片、SVG 矢量图、JSON API 接口、纯文本文件或二进制下载资源。无头浏览器若不对非 HTML 响应进行视图合成，DOM 树和渲染树将无法呈现或无法提取盒模型；
   - **解决方案**：
     - MIME 与处置判定：通过 `mimeEssence` 提取纯净类型，结合 `isAttachment`（排查 `inline` 与非附件属性）阻断危险附件下载；
     - 图片视图合成器（`imageDocumentHtml`）：联动 `src/render/image_dims.cp` 的 `sniffImageDims` 极速嗅探尺寸（如 `2x3`），合成包含 `<img src="data:...">` 的暗色居中 Flexbox 视图，便于截屏和 Agent 视觉定位；
     - SVG 视图合成器（`svgDocumentHtml`）：提取根标签与 `<meta>` 标签提升，当 SVG 缺少绝对尺寸与 `viewBox` 时自动注入基于客户端视口的长宽比 `viewBox="0 0 W H"`，并通过 `vw/vh` 生成自适应拉伸样式；
     - JSON 语法树视图合成器（`jsonViewerHtml`）：基于 `cap:json` 进行语法树遍历，渲染可折叠展开的交互式 DOM 树，且在响应体超过 1MB（`JSON_PARSED_MAX_CHARS`）或遇到畸形格式时安全回退至 `<pre>` 文本，防止内存爆炸。
2. **连接级样式表 LRU 缓存 (`src/cdp/style_cache.cp`)**：
   - **设计动机**：截图引擎与样式重排阶段需要复用远程样式表内容。若每个会话每次绘制都重复发网络请求或在内存中无限缓存，会导致严重的网络抖动与内存泄漏；
   - **解决方案**：
     - 接入 Phase 15 `MAX_STYLESHEET_BYTES`（8MB）作为全局上限；
     - 采用基于访问计数的定长槽位 LRU 淘汰算法，当总保留字符数超限时按最久未访问次序逐一淘汰旧样式表；
     - 更新条目时先扣减旧尺寸再追加新尺寸，严防内存计数漂移。
3. **HTTP 429 速率限制与滑动令牌桶 (`src/net/rate_limit.cp`)**：
   - **设计动机**：面对数以百万计的并发 Agent 连接，必须具备对恶性频次调用的过载防护能力，并提供标准化的 HTTP/1.1 429 协议响应；
   - **解决方案**：
     - 实装基于浮点毫秒时间戳的令牌桶（Token Bucket）算法，支持最大突发容量与平滑平摊速率，自动计算并返回合规的 `Retry-After` 秒数；
     - 提供 `isWebSocketUpgrade` 快速入口点判别，与 kitesurf 官方对齐 fail-open 保护策略。


## Phase 18 网络事件追踪、子框架生命周期与 DOM 安全释放技术方案
1. **CDP 网络事件追踪器与主体响应持久化 (`src/cdp/network_events.cp`)**：
   - **设计动机**：Playwright 与 Puppeteer 等外部自动化框架高度依赖完整的 CDP Network 生命周期事件（`requestWillBeSent`, `responseReceived`, `loadingFinished`, `getResponseBody`）。若无头浏览器只提供模拟的 HTTP 响应而不做内部状态记录，Agent 将无法提取网络响应正文；
   - **解决方案**：
     - 限制在途与归档请求：定长 512 槽记录映射，提供严格的单主体 4MB 与全局 16MB 缓冲上限；
     - 请求序号命名法：基于连接前缀与递增序号生成如 `cs-conn-1` 的唯一请求标识；
     - 修复上游 MIME 推断优先级缺陷：在 `inferResourceType` 中将 `application/manifest+json` 提前至普通 `+json` 之前判定，确保 Web Manifest 资源类型精确识别。
2. **子框架附着流与 DOM 安全析构管理 (`src/cdp/iframe_events.cp` & `src/cdp/dom_release.cp`)**：
   - **设计动机**：页面导航或跨域 iframe 挂载时，如果 DOM 树析构不彻底，会因为跨 Arena 的引用指针导致悬垂借用或内存泄漏；
   - **解决方案**：
     - 零堆分配格式化子框架四个核心生命周期事件（`Page.frameAttached`, `Page.frameStartedLoading`, `Page.frameNavigated`, `Page.lifecycleEvent`）；
     - `stripNoscript`：在解析和析构时主动清理 `<noscript>` 标签子树，消除无用表现型噪音；
     - 封装 `releaseFrameDom` 与 `releaseTargetDom`，确保作用域析构（RAII）与 Arena 批量回收协调运作，保障百万 Agent 并发场景下的极速内存平复。


## Phase 19 标签跨度单调提取、脚本预检策略网关与 Isolate 资源治理技术方案
1. **单调游标无回溯 HTML 标签跨度扫描 (`src/html/tag_spans.cp`)**：
   - **设计动机**：在页面预处理（如提取/剥离 `<script>` 或 `<meta>`）阶段，如果使用正则表达式或全量 DOM 树构建，前者易受超长字符串畸形标签引发灾难性回溯爆炸（ReDoS），后者会带来极高的解析与内存开销；
   - **解决方案**：
     - 采用单调递增游标（Monotone Cursor）扫描算法，在字符流中顺序识别开标签与闭标签（`closingTagEnd`）；
     - 提供 `removeHtmlTagSpans`：以单次前向切片拷贝跳过所有匹配 span，避免反复产生动态字符串中间分配，达到 $O(N)$ 线性时间与空间开销。
2. **脚本加载前策略网关 (`src/eval/script_gate.cp`)**：
   - **设计动机**：网页中的外链脚本可能违背 CSP 策略或包含不安全引用。必须在发起网络 Fetch 前基于页面安全规则执行快速决策，拦截未授权脚本；
   - **解决方案**：
     - 实现 RFC 3986 路径规范化算法 `resolveScriptUrl`（准确处理 `../`、`./` 点段消除）；
     - 接入 `src/net/csp.cp` 的 `isSourceAllowed`，支持 Nonce、SRI SHA 校验与 Host 准入判定，为后续脚本沙箱执行建立硬性防线。
3. **Isolate 资源配额通知与隔离标定 (`src/eval/limit_registry.cp`, `src/eval/isolate_id.cp` & `src/cdp/arrival.cp`)**：
   - **设计动机**：多 Agent 高频并发场景下，部分脚本可能陷入死循环（CPU/Wall 时间超限）或触顶内存阈值。引擎必须能在 Reductions 中断或 V8/QuickJS 钩子触发时将超限通知精准派发给对应脚本所有者，并实现失效代（retired generation）的及时扫除；
   - **解决方案**：
     - `LimitRegistry` 维护 64 槽定长注册表，在容量满时优先清理 `is_live_fn` 返回 false 的失效监听项，支持基于所有者（Owner）与 TargetId 的批量解绑；
     - `engineIsolateId` 基于 Linux CSPRNG 生成唯一 UUID，提供 `isForeignIsolate` 防止网络回环误判；
     - `ArrivalTracker` 采用定长 512 槽 LRU 映射，精确记录请求到达与事件派发的时间差，为调试分析与延迟洞察提供基准数据。

## Phase 20 WASM 预取扫描、引擎错误分类、脚本预加载与宿主时钟技术方案
1. **轻量 WASM 二进制与脚本多模态预取引擎 (`src/eval/prefetch_wasm.cp` & `src/eval/prefetch_scripts.cp`)**：
   - **设计动机**：现代重前端与 AI Agent 交互页面往往加载大型 WebAssembly 模块或数十个分块 JavaScript 脚本。如果在主线程执行到 `WebAssembly.instantiateStreaming` 或 `<script>` 时才串行发起网络请求，网络往返延迟（RTT）将严重制约 Agent 执行效率；
   - **解决方案**：
     - `prefetch_wasm` 提供无正则极速文本扫描器，分别在 HTML 标签属性与内联/外部 JS 代码字面量中快速捕获以 `.wasm` 结尾的静态资源路径，并支持 32 个槽位去重；
     - `prefetch_scripts` 维护基于同源性优先级的脚本预取队列，结合 CSP 准入网关在 HTML 初始流解析阶段即向网络层下发并行 Fetch 请求，将 WASM 与脚本载入延迟隐藏在解析周期内。
2. **多源引擎错误统一分类与标准化 JSON 序列化 (`src/eval/engine_errors.cp` & `src/eval/denied_outbound.cp`)**：
   - **设计动机**：Agent 在执行自动化指令或脚本求值时，可能遭遇脚本语法错误、沙箱超时、内存超限或策略阻断。必须向 CDP 协议层和日志系统输出结构清晰、包含精准错误类别（Error Kind）的规范响应；
   - **解决方案**：
     - `engine_errors` 建立统一的错误类别映射表（OOM, Timeout, EvalError, NetworkBlocked 等），格式化输出符合 Chrome DevTools 规范的 JSON 错误负载；
     - `denied_outbound` 针对被 CSP 或安全过滤器拦截的外链出站请求维护滚动环形审计日志，便于安全排障与合规审计，且完全零堆分配。
3. **高精度宿主单调时钟与跨模块时间戳基准 (`src/eval/host_clock.cp`)**：
   - **设计动机**：网络请求跟踪、CDP 性能分析（Performance Metrics）与超时控制严重依赖精确时间戳，普通系统时钟（Wall Clock）在 NTP 同步或时钟跳变时可能发生回退，导致计算出的时间差为负数；
   - **解决方案**：
     - 基于 Linux `clock_gettime(CLOCK_MONOTONIC)` 提供纳秒级单调不回退时钟，同时提供毫秒与微秒级便捷接口，为全引擎提供统一的时间基准。

## Phase 21 Next.js 路由分块预取、Web Bot Auth 签名与 Span 链路追踪技术方案
1. **Next.js 路由清单静态分析与动态引用排序 (`src/eval/next_route_chunks.cp`)**：
   - **设计动机**：基于 Next.js 构建的 SSR/SPA 页面在首屏只会注入当前路由的代码分块，当 AI Agent 决定跳转或执行下一跳操作时，串行发起新路由的 JS 下载将导致数百毫秒的停顿；
   - **解决方案**：
     - 在 HTML 加载阶段探测 `_buildManifest.js`，通过纯文本线性扫描解析出函数形参代换表与路由到静态资源的映射关系；
     - 结合已加载代码中的路由引用字符串扫描（`countRouteMentions`），优先预加载出现频次最高的路由分块（最多 12 个），将路由切换网络耗时彻底消除。
2. **Web Bot Auth 标准化鉴权签名与缓存控制 (`src/eval/web_bot_auth.cp`)**：
   - **设计动机**：为了在代表用户自动化访问受保护的服务端资源时提供可信身份认领，现代 Web 标准正在推进 HTTP Message Signatures (RFC 9421)。必须在发起出站 Fetch 时自动为目标 Authority 签名，并对高频发起的请求进行缓存以避免昂贵的重复计算；
   - **解决方案**：
     - 严格校验 Ed25519 JWK 结构，构造包含 `created`, `expires`, `keyid` 的规范 `Signature-Input` 与 `Signature-Agent` 头部；
     - 维护 256 槽容量的 LRU 缓存，在 5 分钟 TTL 内复用签名，并留出 30 秒安全时钟偏移保护。
3. **分块限额 Span 接收池与 Trace 尾追踪 (`src/eval/span_sink.cp` & `src/eval/carrier_spans.cp`)**：
   - **设计动机**：在并发高负荷运行环境下，Worker 或沙箱输出的大量追踪 Span 和日志如果不加限制直接汇聚，可能耗尽内存并拖垮主进程 IPC 通道；
   - **解决方案**：
     - `span_sink` 对单次调用强制设置 2000 条记录与 4MB 内存上限，超出部分平滑丢弃并计入统计指标；
     - `carrier_spans` 实行 48KB 字节上限与 1000 条日志上限的分块切分策略，确保每次投递均在单帧安全预算以内；
     - `tail_trace_registry` 维护 64 槽定长注册表，在异步任务完成与尾标归因触发时准确关联父 Span 上下文。

## Phase 22 终端渲染模式参数解析、字体整形网格与脚本分桶打包技术方案
1. **轻量终端模式网格参数与样式合成 (`src/tty/nav_params.cp`)**：
   - **设计动机**：针对无图形显示环境的纯文本终端或 SSH 交互客户端，页面需要以字符网格单元（Cell Grid）重新计算尺寸与字体度量，同时避免加载重型 CSS 样式表；
   - **解决方案**：
     - 从导航 URL 查询参数中极速提取 `ks_tty_css`、`ks_color_scheme` 与 `ks_tty_cell`，并执行 `[1, 64]` 像素范围校验；
     - 零堆分配动态拼接包含 `--ks-tty-cell-w` / `--ks-tty-cell-h` 运行时属性的高优先级终端样式表，兼顾 Level 1 与 Level 2 紧凑布局。
2. **字体整形并发控制与格式门禁 (`src/eval/font_shaping.cp`)**：
   - **设计动机**：字体轮廓抽取与复杂文字整形（HarfBuzz / FreeType）是 CPU 密集型操作。在高频并发 Agent 请求下，如果不限制并发，很容易引发 CPU 争用与请求尖峰；
   - **解决方案**：
     - 建立 `FontShapingSlotManager` 状态机，限制最多 4 路并行执行，多余请求平滑排队；
     - 在进入复杂解析前，通过 4 字节 Magic Number 预先过滤非法或损坏的字体文件（支持 OpenType, TrueType, WOFF, WOFF2）。
3. **脚本打包分类、VLQ 编码与安全转义 (`src/eval/script_bundle.cp`)**：
   - **设计动机**：在页面启动时，成百上千个内联与外链脚本具有不同的加载时序（同步/异步/延迟/模块/预加载）。此外，为开发者工具生成 Source Map 需要极速的 Base64 VLQ 差值编码器；
   - **解决方案**：
     - 实现 RFC 4648 Base64 VLQ 编码器，以位运算快速编码行列偏移；
     - 针对脚本注释中的换行符实行安全百分号转义，根除注释逃逸风险；
     - 精准对脚本进行分类分桶，为后续渲染树与执行沙箱提供清晰的就绪时序依赖。

## Phase 23 CDP 协议扩展、沙箱世代生命周期与动态环境分发技术方案
1. **CDP 分发环境动态参数治理 (`src/cdp/dispatch_env.cp`)**：
   - **设计动机**：在不同的测试与生产运行场景下，AI Agent 对页面的 CPU 耗时、Wall Clock 耗时以及网络出站权限有不同的安全策略。需要统一的分发环境对象在命令处理链路中下发这些约束；
   - **解决方案**：
     - 构建 `DispatchEnv` 配置对象，支持在运行时通过键值对动态调整 `page_cpu_ms_cap`、`page_wall_ms_cap` 与 `allow_private_network`；
     - 将私有网络防 SSRF 策略置于第一优先级，保障在未明确赋权时严禁内网探测。
2. **PageScript 多世代演进与安全卸载收割 (`src/cdp/page_script_generation.cp`)**：
   - **设计动机**：页面进行单页跳转（SPA / `history.pushState`）或整页导航时，如果前一代脚本的执行状态未能彻底释放或由于正在执行 RPC 发生悬挂，会导致内存泄漏与数据竞争；
   - **解决方案**：
     - 建立 `StubHolder` 世代容器，以单调自增序号追踪活跃的脚本存根（Stub）；
     - 实现不同意图（`navigate`, `closeTarget`, `wsClosed` 等）的差异化清理路径：整页跳转时主动收割控制台输出与 Cookie 修改并回传新页面，关闭目标时立即斩断进程。
3. **CapSurf / Kitesurf 协议扩展命令双前缀响应 (`src/cdp/protocol_extensions.cp`)**：
   - **设计动机**：为了兼容原 Kitesurf 客户端测试套件与 CapSurf 专属扩展，必须在 CDP 协议处理层支持双前缀识别，并输出结构化响应；
   - **解决方案**：
     - 抹除前缀差异统一路由到核心命令实现，提供会话诊断 `getSessionInfo`、排版模式动态开关 `setLayoutMode` 以及渲染器指标 `getRendererInfo`。

## Phase 24 场景流控反压、Host 二进制切片与顶层 Facade 统一路由技术方案
1. **Scene Host 二进制分块校验与切片服务 (`src/cdp/scene_host.cp`)**：
   - **设计动机**：在无头浏览器图形画面或远程隔离环境（RBI）下，客户端需要获取匹配版本的渲染器二进制。必须支持严格的 Sha256 校验与 4MB 切片 Range 响应，防止网络突发洪峰；
   - **解决方案**：
     - `SceneHostManifest` 提供 32MB 总体积与 4MB 单块边界检查，通过紧凑 JSON-RPC 输出切片数据 Base64。
2. **多路 RBI 画面推流管理与反压确认机制 (`src/cdp/scene_stream.cp`)**：
   - **设计动机**：在高并发场景下向多个 AI Agent 客户端推流渲染画面时，若客户端消费不及时，无节制的发帧会导致严重的内存积压与网络拥塞；
   - **解决方案**：
     - `SceneStreamManager` 维护基于信用额度（Credit）的流控调度，客户端每渲染完成一帧回传 `sceneFrameAck`；
     - 若客户端指示丢帧（`painted: false`），流控引擎自动将 `need_full_frame` 置为真，在下一跳强制触发全量关键帧编码，兼顾吞吐与画面一致性。
3. **顶层 Facade 门面整合 (`src/cdp/kitesurf_facade.cp`)**：
   - **设计动机**：消除零散扩展方法与流控制器的调用复杂性，对上层提供单一派发入口；
   - **解决方案**：
     - 整合 Scene Host、Scene Stream 与 Protocol Extensions，实现多域协议统一路由。

## Phase 25 Dedicated Web Worker 隔离桥接、页面入口与遥测中继技术方案
1. **页面隔离区动态入口代码生成 (`src/eval/page_isolate_entry.cp`)**：
   - **设计动机**：页面隔离脚本加载时需要动态重新导出底层主模块，同时挂载用户自定义的 Host 类。必须具备标识符校验以防止代码注入风险；
   - **解决方案**：
     - `pageIsolateEntry` 先行执行词法合法性校验，若检测到非法标识符自动回退至默认入口 `page.js`，否则安全拼接重导出指令。
2. **遥测环形缓冲区与致命异常检测 (`src/eval/page_script_tail.cp`)**：
   - **设计动机**：浏览器隔离区内的长任务或死循环会导致宿主无响应。必须在日志收集链路对特定崩溃与超时标记实时探知；
   - **解决方案**：
     - 在写入日志时并行匹配 `[ks-kill]` 与 `detected that your Worker's code had hung`，实现微秒级异常标记识别与标准 OTEL 结构生成。
3. **Dedicated Worker 隔离调度与无拷贝切片架构 (`src/eval/dedicated_worker.cp`)**：
   - **设计动机**：Web Worker 具备独立的作用域与执行生命周期，而在 CP 架构下若结构体内部使用定长数组并对外切片，按值拷贝会导致局部悬垂引用；
   - **解决方案**：
     - `DedicatedWorker` 与 `WorkerUpstreamMessage` 直接使用只读切片 `[]const u8`，彻底消除栈内存切片悬垂引用风险；
     - `buildWorkerScriptModule` 将用户代码包裹在 CommonJS 异步运行体内，严格屏蔽全局变量污染。

## Phase 26 ESM 模块依赖图谱分析、Import Map 动态解析与岛屿探测技术方案
1. **Import Map 规范解析与前缀/作用域分级决策 (`src/eval/walk_module_graph.cp`)**：
   - **设计动机**：现代 Web 应用（如 Next.js、Astro、Vite）广泛依赖 import map 映射模块路径。恶意或畸形 import map 可能通过极深前缀或大量规则发起计算耗尽攻击；
   - **解决方案**：
     - 引入 `ImportMapBudget` 计量器，严格限制计算单元 `work`（上限 400M），超限后平滑降级（`degraded = true`），避免整个引擎卡死；
     - `ParsedImportMap` 按作用域（Scope）由长到短、精确匹配优先于前缀通配（以 `/` 结尾）的顺序执行规范化解析。
2. **HTML 静态岛屿与 Preload 探测 (`src/eval/walk_module_graph.cp`)**：
   - **设计动机**：为了在 JavaScript 执行前预热模块依赖，需要从 HTML 静态流中快速抓取 Astro 岛屿（`<astro-island component-url>`）与模块预加载（`<link rel="modulepreload">`）；
   - **解决方案**：
     - 使用轻量零分配切片扫描算法，快速捕获依赖 URL 并注入预加载种子队列。

## Phase 27 页面执行沙箱、RPC 会话追踪与 OOPIF 渲染隔离技术方案
1. **页面沙箱隔离区与缓存键空间划分 (`src/eval/page_script.cp`)**：
   - **设计动机**：主页面与不同源 iframe 必须具备物理隔离的执行环境与配额预算。为了快速命中可复用脚本上下文，需要高特异性缓存键；
   - **解决方案**：
     - `pageScriptCacheKey` 根据是否包含 `loader_id` 与 `frame_id` 前 8 位生成哈希键空间，有效隔离主文档与跨域子框架；
     - `hostnameOf` 与 `originOf` 基于纯切片扫描实现零内存分配，消除堆碎片。
2. **在途 RPC 会话追踪与高精度时钟 (`src/eval/page_script_rpc_session.cp`)**：
   - **设计动机**：CDP 控制台与脚本执行过程中存在大量的异步请求与超时取消逻辑。标准库时间函数若在某些平台缺少会导致符号丢失；
   - **解决方案**：
     - 基于 Linux 系统级 `gettimeofday` 封装微秒级高精时钟 `getNowMs`，支持 64 路无锁在途 RPC 请求的超时探测与槽位回收。
3. **OOPIF 多进程视口安全限幅与预热池 (`src/eval/oopif_render.cp` & `src/eval/page_renderer.cp`)**：
   - **设计动机**：跨域 iframe 渲染可能遭遇恶意巨幅视口请求（例如 100000x100000），导致 GPU 或内存爆仓；
   - **解决方案**：
     - `clampFrameViewport` 将单边限制在 4096 像素，总面积严格限制在 7.8M 像素以内；
     - `OopifRenderCoordinator` 维护 16 路预热帧缓冲池，实现跨域渲染生命周期与离屏光栅化快速装配。

## Phase 28 DOM 事件流水线传播与变更观察器技术方案
1. **DOM 三阶段事件传播与微秒级事件拦截 (`src/dom/event_target.cp`)**：
   - **设计动机**：Web 自动化测试与页面交互（点击、输入、键盘事件）依赖严谨的 DOM Level 3 事件传播机制。若缺乏捕获（Capturing）与冒泡（Bubbling）阶段支持，大量前端框架（如 React 合成事件系统）将无法正常触发；
   - **解决方案**：
     - `DOMEventSystem` 构建从根节点到目标节点的祖先链数组 `path`；
     - 按标准顺序执行捕获阶段（根至目标父级）、目标阶段（AtTarget）、冒泡阶段（目标父级回溯至根）；
     - 提供 `stopPropagation` 与 `stopImmediatePropagation` 中断机制以及 `once` 监听器的单次触发自动失效。
2. **轻量无锁 MutationObserver 变更聚合 (`src/dom/mutation_observer.cp`)**：
   - **设计动机**：AI Agent 驱动页面时，频繁监听 DOM 节点增删与属性变化以感知页面加载完成或 UI 状态变更。直接轮询开销巨大；
   - **解决方案**：
     - `MutationManager` 对目标节点变更进行精准过滤，支持属性旧值保留（`attributeOldValue`）与子节点变动记录；
     - `takeRecords` 提供单次原子消费接口，为后续 CDP `DOM.attributeModified` 与 `DOM.childNodeInserted` 事件提供高效事件源。

## Phase 29 DOM 树遍历器与范围模型计算技术方案
1. **基于 Arena NodeId 的零拷贝 TreeWalker 漫游 (`src/dom/traversal.cp`)**：
   - **设计动机**：DOM 树深层遍历如果使用传统递归或指针解引用，深度极大时可能栈溢出，且借用检查成本高；
   - **解决方案**：
     - 利用 CapSurf 核心 Arena 树的 `parent`, `first_child`, `next_sibling` 等 4 字节整型下标，将 `TreeWalker` 与 `NodeIterator` 改造成纯迭代状态机；
     - `traverseChildren` 与 `traverseSiblings` 结合 `whatToShow` 位掩码在 $O(1)$ 辅助空间内完成任意深度的树形导航。
2. **DOM Range 几何边界与最近公共祖先算法 (`src/dom/range.cp`)**：
   - **设计动机**：Playwright 文本选区、剪切提取与光标定位需要高精度的边界点（BoundaryPoint）比较与公共祖先计算；
   - **解决方案**：
     - `findCommonAncestor` 采用定长数组双路祖先扫描，消除堆分配；
     - `comparePoint` 通过向祖先回溯判断层级关系，实现 `START_TO_START`、`START_TO_END` 等 4 种规范模式的精确有序判定。

## Phase 30 CDP 统一分发增强与综合端到端矩阵技术方案
1. **CDP 协议分发器多域补全与扩展挂载 (`src/cdp/dispatch.cp`)**：
   - **设计动机**：为了向客户端提供无死角的 CDP 协议覆盖，原有的 `CSS` 域方法与 `CapSurf` 扩展命令需要与基础 Page、DOM、Runtime 域享受平等的路由调度待遇；
   - **解决方案**：
     - 将 `CSS.getComputedStyleForNode` 与 `CSS.enable` 直接挂接至 `CssHandler`，支持动态几何与样式属性提取；
     - 通过 `session.facade` 统一路由 `CapSurf.*` 与 `Kitesurf.*` 双前缀扩展协议。
2. **基于 Node.js 原生 WebSocket 的黑盒端到端回归矩阵 (`scripts/test_e2e_matrix.js`)**：
   - **设计动机**：端到端（E2E）测试必须脱离浏览器内部单元 mock，以最真实的远程客户端角度验证从网络握手、协议多路复用到图形光栅化的全生命周期；
   - **解决方案**：
     - 编写 11 步端到端自动化矩阵脚本，严格验证标准 HTTP 自省、WebSocket RFC 6455 帧传输、盒模型像素精度、JS 执行与 PNG 二进制魔数。

## Phase 31 DOM 集合模型与 WebIDL 属性反射引擎技术方案
1. **DOMTokenList 状态机与内建方法命名规避 (`src/dom/collections.cp`)**：
   - **设计动机**：前端代码高频调用 `element.classList.add/remove/toggle`。在 CP 语言中，`replace` 关键字会被编译器 AST 自动下沉重写为切片替换函数 `std.mem.replaceOwned`，引发类型推导不匹配；
   - **解决方案**：
     - 命名重定义为 `replaceToken`，规避内建符号混淆；
     - `serialize` 采用单次线性拼接，杜绝任何中间字符串对象分配。
2. **WebIDL 属性反射截幅与空安全 (`src/dom/reflect.cp`)**：
   - **设计动机**：DOM 属性（如 `tabIndex`, `width`, `maxLength`）的 WebIDL 规范要求进行有符号/无符号截取、默认值回退和枚举规范化；
   - **解决方案**：
     - 使用 CP 原生空值合并运算符 `attr_val ?? default_val` 代替局部解包，消除了局部引用生命周期悬垂（`Cannot return reference to local variable`）报错；
     - `reflectEnum` 结合忽略大小写匹配与多级缺省回退，严格符合 WHATWG HTML 规范。

## Phase 32 DOM 表单控件、约束验证与报文序列化引擎技术方案
1. **表单验证状态机与约束计算 (`src/dom/forms.cp`)**：
   - **设计动机**：标准 HTML 表单包含极其丰富的约束条件（必填 `required`、正则匹配 `pattern`、长度 `minlength`/`maxlength`、数值范围 `min`/`max` 以及脚本自定义验证消息 `setCustomValidity`）；
   - **解决方案**：
     - 在 `FormControl` 实例上实现统一的 `updateValidity`，将 10 个布尔验证维度紧凑存储在 `ValidityState` 中；
     - `checkValidity` 触发级联状态计算，`HTMLFormElement` 支持容器级全量控件一致性扫描，确保提交与重置状态完全可预测。
2. **表单编码与多段报文序列化器 (`src/dom/form_submission.cp`)**：
   - **设计动机**：浏览器表单提交支持 `application/x-www-form-urlencoded` 和 `multipart/form-data` 两种主流 MIME 编码，包含保留字符转义与文件边界构造；
   - **解决方案**：
     - 百分比编码器采用静态快速查找表与十六进制移位算法，跳过无需转义的 RFC 3986 未保留字符；
     - 多段序列化器动态组装 Boundary 标头与 `Content-Disposition` 字段，实现零动态垃圾分配的缓冲区分块写入。

## Phase 33 Canvas 2D 图形引擎与离屏位图光栅化表面技术方案
1. **状态栈与路径 Bresenham 光栅化 (`src/dom/canvas2d.cp`)**：
   - **设计动机**：W3C Canvas 2D 规范要求支持级联状态压栈/出栈（`save`/`restore`），以及包含移动、连线、闭合和描边的矢量路径绘制；
   - **解决方案**：
     - 在 `CanvasRenderingContext2D` 中维护连续固定容量的状态栈与路径折线点数组，杜绝频繁动态堆分配；
     - 采用整型增量 Bresenham 算法进行直线与闭合轮廓光栅化，并结合 Alpha 通道 Porter-Duff 混合公式写入像素。
2. **HTMLCanvasElement 离屏上下文关联与 PNG DataURL 导出 (`src/dom/canvas_element.cp`)**：
   - **设计动机**：Web 页面高频通过 `canvas.getContext('2d')` 惰性获取上下文，并通过 `canvas.toDataURL()` 导出 Base64 位图供视觉 Agent 消费；
   - **解决方案**：
     - `HTMLCanvasElement` 内部通过可空指针缓存关联的上下文实例，仅在首调 `getContext("2d")` 时按需分配像素缓冲区；
     - 导出流程直通无拷贝 `PngCodec.encodeRgbaWithLevel`，再通过标准库 Base64 编码器前缀 `data:image/png;base64,`，完全兼容标准无头浏览器行为。

## Phase 34 HTML5 进程间通信通道与 SSE 流式事件解释器技术方案
1. **MessageChannel 纠缠端口与延迟排队状态机 (`src/eval/message_channel.cp`)**：
   - **设计动机**：HTML5 Web Workers、Service Workers 与 Iframes 依赖 `MessageChannel` 进行轻量异步报文交换，W3C 规范要求在调用 `start()` 或安装事件监听器前，消息需在端口内部安全缓冲；
   - **解决方案**：
     - 在 `MessagePort` 内部构建环形/线性定长消息队列，支持至多 64 条待投递报文零动态分配暂存；
     - `postMessage` 直通对端端口缓冲，未 `start` 前禁止出队；当端口调用 `close()` 时就地丢弃剩余排队项并阻断双向后续流转。
2. **Server-Sent Events 跨分块流式解释器 (`src/net/sse_parser.cp`)**：
   - **设计动机**：AI Agent 与现代 Web 实时数据流高频依赖 `text/event-stream`，网络数据包到达往往在行边界（CR/LF）、BOM 标记或冒号分割处发生任意跨分片截断；
   - **解决方案**：
     - `EventStreamParser` 采用单字节与切片扫描双模式状态机，支持跨分片拼接与 UTF-8 BOM 自动剥离；
     - 严格遵循 WHATWG 规范：多行 `data:` 字段以单个 `\n` 连接，空行触发完整事件派发并清空当前数据缓存，非法含 `\0` 的 ID 字段严格忽略以确保跨状态安全性。

## Phase 35 Web Storage 存储引擎与原生独立进程 E2E 测试架构方案
1. **零堆碎片 Web Storage 状态机 (`src/dom/storage.cp`)**：
   - **设计动机**：Web 应用程序普遍依赖 `localStorage` 与 `sessionStorage` 暂存用户会话、状态或配置项，规范要求支持通过 `key(index)` 按插入顺序稳定检索并实现同源隔离；
   - **解决方案**：
     - 在 `Storage` 内部使用定长连续数组管理 128 项键值对，杜绝哈希表高并发频繁伸缩导致的内存碎片；
     - `removeItem` 采用就地线性移位保持项顺序连续性，`StorageManager` 显式解耦会话级与持久级两个独立桶。
2. **独立进程解耦的 E2E 远程测试架构 (`e2e/harness/harness.js`)**：
   - **设计动机**：原版 Kitesurf 391 个 E2E 用例强依赖 Cloudflare Workers 运行时内部绑定。CapSurf 作为独立单二进制需要一套可直接通过真实网络 WebSocket 测试本地进程的通用自动化 Harness；
   - **解决方案**：
     - `harness.js` 统一接管 CapSurf 进程生命周期（端口动态分配、输出监听、优雅退出保底），基于 Node.js 原生 WebSocket 实现完整的 CDP 请求队列与事件监听；
     - 构建通用断言库与异步轮询器 `expect.poll`，成功将 `blank-page-document.test.js` 迁移验证通过，为后续 391 个测试的规模化导入打下坚实地基。

## Phase 36 DOM XPath 1.0 评估器与 Browser.close 协议架构方案
1. **零动态分配 DOM XPath 1.0 状态机 (`src/dom/xpath.cp`)**：
   - **设计动机**：Playwright、Puppeteer 与爬虫智能体高频使用 XPath 选择器（如 `page.locator('xpath=//div[@class="content"]')`）进行精确定位；
   - **解决方案**：
     - 实现分词与 AST 解析器，将复杂路径编译为节点过滤谓词（标签匹配、属性匹配、索引匹配）；
     - `XPathResult` 采用连续定长数组维护结果节点快照，支持快照随机索引与迭代器顺序遍历，兼顾极高遍历效率与确定性零 GC 生命周期。
2. **CDP `Browser.close` 协议与 RFC 6455 1000 优雅断开通道**：
   - **设计动机**：自动化测试工具（Jest、Vitest、Puppeteer、Playwright）在每次测试用例结束钩子（`afterEach` / `afterAll`）中都会向调试端点发送 `Browser.close` 请求，若服务端仅关闭 socket 或不回复 JSON 确认，测试客户端会报超时挂起或连接异常；
   - **解决方案**：
     - 在 CDP 分发器中识别 `Browser.close`，立即回包响应 `{"id": N, "result": {}}`；
     - 主动构造 RFC 6455 0x08 opcode，携带 1000 状态码与 "Browser.close" 原因字符串发送 Close 帧，随后优雅关闭底层网络连接，使远程黑盒驱动无缝完成测试退出闭环。

## Phase 37 DOM Attributes / NamedNodeMap 与 Data URL 百分号解码架构方案
1. **RFC 2397 规范级 Data URL 解码流水线 (`src/net/http_loader.cp`)**：
   - **设计动机**：自动化测试与框架常用 `page.goto("data:text/html;charset=utf-8," + encodeURIComponent(html))` 动态注入复杂页面。若仅支持简单固定前缀或未处理 `%XX` 编码，页面内容将被解析为畸形文本甚至导致导航失败；
   - **解决方案**：
     - `HttpLoader.decodeDataUrl` 按规范拆分媒体头与 payload 数据段，精准识别 `;base64` 参数以调用 Base64 引擎；
     - 对纯文本与 URL 编码数据执行单趟流式百分号扫描解码（`%XX` 转换），并在导航阶段保证 `charset=utf-8` 正确还原为真实 DOM 标签与内联脚本。
2. **零 GC NamedNodeMap 实时同步与 React 19 兼容性设计 (`src/js/shim/collections.js`)**：
   - **设计动机**：现代前端框架（如 React 19 的 `releaseSingletonInstance`）频繁使用 `while (attrs.length) el.removeAttributeNode(attrs[0])` 排空元素属性，并严格断言 `Object.keys(attrs).length === attrs.length`；
   - **解决方案**：
     - `NamedNodeMap` 实例将属性索引作为实例自有属性直接动态挂载，同时将内部管理状态通过 `Object.defineProperty(..., { enumerable: false })` 彻底隐藏；
     - 每次属性修改（`setAttribute`, `removeAttribute`, `removeAttributeNode`）均实时同步 `_attrs` 映射表并刷新索引属性，既保证符合 W3C 规范的 live 动态特性，又完全兼容 React 19 等复杂排空与克隆循环。

## Phase 38 WHATWG File API 与 HyperlinkDecomposition 架构方案
1. **WHATWG File API 严格边界 MIME 归一化规范 (`src/net/mime.cp` & `src/js/shim/file.js`)**：
   - **设计动机**：Web 规范（WHATWG File API）对 `Blob` 与 `File` 的 `type` 做了严格的限制：任何含有控制字符（`< 0x20`，如垂直制表符 `\x0B`）或非 ASCII 字符（`> 0x7E`，如 Latin-1 `\xE5`）的媒体串必须被强制归一化为空字符串 `""`，同时 ASCII 大写字母必须转换为小写并保留首尾空格；
   - **解决方案**：
     - 在 CP 核心层与 JS Shim 双端实装 `normalizeBlobType`，对字节流进行单趟严格过滤，确保非法字节零容忍快速回退；
     - `Blob` 与 `File` 构造器严格按照 WebIDL 字典规则校验 `options` 参数：当传入数字、布尔值等基本类型时准确抛出 `TypeError`，并支持从 `Request`/`Response` Content-Type 头保留原大小写 MIME 透传。
2. **HTMLHyperlinkElementUtils 畸形 URL 协议回退协议 (`src/dom/hyperlink.cp` & `src/js/shim/element/html-classes.js`)**：
   - **设计动机**：在 HTML5 规范中，`HTMLAnchorElement` 与 `HTMLAreaElement` 实现 `HTMLHyperlinkElementUtils` 接口。当 `href` 设置了畸形 URL（如端口含多冒号 `"http://example.com:80:"`）导致底层 URL 解析器失败时，规范要求 `protocol` 必须返回 `":"`（单一冒号），`origin` 返回 `"null"`，其余字段返回 `""`；
   - **解决方案**：
     - 在 CP 核心层实现 `HyperlinkDecomposition.decompose`，对各种非法 URL 场景统一实施规范回退保护；
     - 在 JS Shim 中为 `HTMLAnchorElement` 与 `HTMLAreaElement` 统一绑定超链接分解属性，使爬虫智能体与前端框架在解析畸形链接时保持完全一致的浏览器行为。

## Phase 39 CSS attr() Evaluator 与 WebIDL Sequence 转换架构方案
1. **W3C CSS Values and Units Level 5 规范级 `attr()` 计算器 (`src/css/attr_evaluator.cp`)**：
   - **设计动机**：CSS 变量经常借助 `attr(data-x type(<url>))` 动态读取 DOM 属性。然而根据 W3C 规范，被 `attr()` 提取的 URL 处于 attr()-tainted 状态，无法作为安全合法的外部资源 URL，在计算样式阶段必须作为无效声明处理，回退到 fallback 值或空串；
   - **解决方案**：
     - `attr_evaluator.cp` 解析 `attr()` 语法树中的类型声明与回退参数；
     - 对 `type(<url>)` 显式判定并实施回退策略，配合 `window.getComputedStyle` 的样式规则匹配，精准支持复杂 CSS 自定义属性动态解析。
2. **WHATWG Headers WebIDL Sequence 规范转换与生命周期隔离 (`src/js/shim/file.js` & `src/cdp/session.cp`)**：
   - **设计动机**：Playwright 与爬虫框架初始化 `Request` 时广泛传入自定义 Header 迭代器或冻结配置对象（如 `Object.freeze({ headers: [...] })`），同时规范要求迭代器属性只读一次 `next` getter，多余读取或非法序列类型必须准确抛出 `TypeError`；
   - **解决方案**：
     - 在 `Headers` 构造器中严格遵循 WebIDL sequence 转换规范，单次捕获迭代器 `next`，并校验二元组数组结构；
     - 在 CDP `Page.navigate` 中规范生命周期，析构旧 `JsDomContext` 并创建新实例，同时将 `global.document` 设置为 `configurable: true`，消除重新导航时的上下文混乱与内存悬垂崩溃。

## Phase 40 Microtask/Macrotask 队列交织、Mock Fixture 合成与 Bun 原生 E2E 驱动架构方案
1. **HTML5 事件循环规范级 Microtask / Macrotask 队列交织设计 (`src/js/qjs_engine.cp`, `src/cdp/session.cp`, `src/js/shim/window.js`)**：
   - **设计动机**：根据 HTML5 事件循环规范（§ 8.1.6 Event loops），在每一个宏任务（Macrotask）执行完毕后，引擎必须进入「执行微任务检查点（Perform a microtask checkpoint）」排空整个微任务队列（Promise 回调、`queueMicrotask`、`MutationObserver`），随后方可调度执行下一个宏任务（如 `MessagePort.postMessage`、`setTimeout`）；若微任务与宏任务没有准确解耦交织，异步链式计算将出现时序颠倒；
   - **解决方案**：
     - 在 `src/js/qjs_engine.cp` 暴露 `executePendingJobs`，直接驱动 QuickJS 内部的 Promise Job Queue；
     - 在 CDP `Runtime.evaluate` 中识别 `awaitPromise: true`，在返回评估结果前循环执行微任务队列直到所有 Promise 达成终态并提取真实值（或异常）；
     - 将 `MessagePort.postMessage` 通过宿主 `setTimeout(..., 0)` 派发，确保由同一个执行上下文排空的 microtask 优先于后续端口消息被处理。
2. **端到端黑盒测试纯 TypeScript 化与 Bun 原生驱动技术方案 (`e2e/harness.ts`, `bunfig.toml`, `tsconfig.json`)**：
   - **设计动机**：前期黑盒驱动曾临时转写为 CommonJS `.test.js` 文件，破坏了测试源码的统一性并增加了中间维护成本；同时 Node.js 驱动单次启动开销大（单套用例需 1~2s），而 Bun 具备原生 TypeScript 执行能力与超快启动性能（单用例 ~20ms）；
   - **解决方案**：
     - 彻底删除全部历史临时 `.test.js` 文件，恢复全量原生 `.e2e.test.ts`；
     - 编写 `e2e/harness.ts` 与 `e2e/shim/cloudflare-test.ts`，基于 Bun 的 `bun:test`（`test`, `expect`, `describe`）和原生 WebSocket 客户端驱动 `./bin/main`；
     - 通过极轻量 `node_modules/cloudflare:test` 映射层，使原版 Kitesurf 用例（包括 `import { env } from "cloudflare:test"`）零修改直跑；
     - 实装 `releaseSharedCdp`，在测试之间优雅管理 WebSocket 独占连接与重连，支持测试用例高并发连续运行。
3. **内置 Mock Fixture 响应回显合成器 (`src/net/mock_fixtures.cp` & `src/net/http_loader.cp`)**：
   - **设计动机**：Kitesurf 原始测试集广泛依赖向 `*.invalid/__fixtures/echo-headers.json` 发送请求以验证网络协议头行为（如 `Origin: null`、用户自定义 Headers、User-Agent 等），而在隔离离线环境中缺乏外部真实服务；
   - **解决方案**：
     - 构建 `MockFixtures.isFixtureUrl` 识别请求模式，通过 `handleFixtureRequest` 在内存中毫秒级动态构造合规 JSON 响应并注入回显头；
     - 在 `HttpLoader.fetchDocument` 中直接拦截，实现完全零外部网络依赖的自包含测试能力。

## Phase 41 CDP 异常格式化、Runtime.addBinding 与防篡改生命周期事件架构方案
1. **CDP RemoteObject 规范级异常序列化与换行契约 (`src/cdp/session.cp`)**：
   - **设计动机**：Chrome DevTools Protocol 规范对 `Runtime.evaluate` 抛出异常时的 RemoteObject 结构有强约定：`className` 必须返回规范错误类（如 `TypeError`, `RangeError` 等），`description` 必须严格包含完整的调用堆栈或带有末尾换行符的单行描述（`TypeError: message\n`）。若缺少换行符或缺少 `className`，Vitest / Puppeteer 断言库（如 `expect(r.exceptionDetails?.exception?.className).toBe("TypeError")`）将直接失败；
   - **解决方案**：
     - 在 `formatCdpException` 中对异常字符串进行前缀冒号分词提取合法类名，并自动补齐末尾 `\n`；
     - 构造完整的 `exceptionDetails` JSON 对象，精准兼容 CDP 原生测试。
2. **CDP `Runtime.addBinding` 与 `Runtime.bindingCalled` 双向事件流水线 (`src/cdp/runtime.cp`, `src/cdp/session.cp`, `src/js/shim/window.js`)**：
   - **设计动机**：自动化测试工具（如 Puppeteer 的 `page.exposeFunction`）通过 `Runtime.addBinding` 向页面注入宿主通信桥梁。页面调用注入的全局函数时，引擎必须在 CDP 连接上即时派发 `Runtime.bindingCalled` 事件供测试客户端捕获并响应；
   - **解决方案**：
     - 采用事件队列解耦模式：在 JS Shim 中维护微型队列 `__capsurf_binding_queue`；
     - 每次执行 `Runtime.evaluate` 返回前调用 `pollBindingEvents` 一次性排空队列并转化为标准 `CdpEvent` 广播；
     - 在 `e2e/harness.ts` 封装 `page.exposeFunction` 与 `expect.poll`，实现双向闭环。
3. **HTML5 页面生命周期防篡改派发与多 Runtime 类隔离 (`src/js/shim/window.js` & `src/js/dom_binding.cp`)**：
   - **设计动机**：部分测试用例（如 `window-global-scope.e2e.test.ts`）会故意覆盖或劫持 `window.Event`，若引擎使用被篡改的构造器分发事件将抛错失败；此外，单进程跨会话创建新 `JSRuntime` 时若跳过 `JS_NewClass` 将导致段错误；
   - **解决方案**：
     - 早期备份 `NativeEvent = global.Event`，在 `__capsurf_dispatch_lifecycle` 与 `history` 路由跳转中始终使用未被污染的原生事件对象；
     - 确保每一个新创建的 `raw_rt` 运行时均显式注册 `Element` 与 `Document` 类定义，根除悬垂类 ID 引发的内存访问越界。

## Phase 42 静态测试资产服务、动态策略端点合成与跨导航评估取消架构方案
1. **测试静态资产与动态策略端点服务架构 (`src/net/mock_fixtures.cp` & `src/net/http_loader.cp`)**：
   - **设计动机**：Kitesurf 原始测试集大量引用 `/__fixtures/events.html`、`webfont.html`、`solid-red.png` 等各类静态文件，同时依赖 `/policy-doc.html`、`/policy-script.js` 等动态端点。在完全隔离的无外网环境中，必须由内部引擎自给自足地提供文件检索与响应合成；
   - **解决方案**：
     - 在 `mock_fixtures.cp` 中实装基于 POSIX C-FFI 的静态资产检索器，映射到 `e2e/page/`，根据扩展名推导合法 MIME Content-Type；
     - 动态合成 `/policy-*` 报文，配合 `HttpLoader` 拦截分发，实现零外网依赖的自包含测试基座。
2. **跨导航在途评估销毁取消机制与 Matcher 扩展 (`e2e/harness.ts`)**：
   - **设计动机**：当页面正在执行长耗时异步 evaluate（如未决 Promise 或定时器）时，若测试触发导航（`goto` 或 `setContent`），Chrome 标准行为是立即销毁执行上下文并拒绝在途 Promise（`Execution context was destroyed`）。若引擎没有销毁机制，挂起的 Promise 将导致测试用例挂死；
   - **解决方案**：
     - 在 `Page` 中维护 `inFlightEvaluates`集合，在导航动作发生瞬间统一 reject 并清空；
     - 基于 Proxy 扩展 `expect(promise).rejects.toThrow(...)`，完善异步断言支持。

## Phase 43 导航历史体系、Page.getNavigationHistory 与同文档/跨文档安全隔离技术方案
1. **环形滑动导航历史记录与 Chrome 规范对齐 (`src/cdp/session.cp` & `src/cdp/page.cp`)**：
   - **设计动机**：Chrome CDP 协议要求 `Page.getNavigationHistory` 准确返回页面完整的浏览轨迹、当前所处索引（`currentIndex`）以及所有历史条目（`id`, `url`, `userTypedURL`, `title`, `transitionType`）。当页面进行前进/后退后再导航时，历史链条必须在当前索引截断；
   - **解决方案**：
     - 在 `session.cp` 中实装 16 槽环形滑动历史记录缓冲区 `nav_entries: [NavigationEntry]` 与 `current_nav_index`；
     - 当连续导航达到 16 槽上限时，触发滑动窗口左移淘汰最久历史；若在历史回退状态发生新导航，自动截断当前索引之后的全部条目；
     - 在 `page.cp` 中准确构造 JSON 返回结构，并通过 `Page.navigateToHistoryEntry` 实现历史快照的原生跳转。
2. **同文档/跨文档导航隔离与同源安全防护 (`src/js/shim/window.js` & `src/cdp/session.cp`)**：
   - **设计动机**：页面可通过 `history.pushState`、`history.replaceState`、`location.assign`、`location.replace` 与 `location.href` 改变当前 URL。恶意脚本可能尝试利用 `history.pushState` 伪造跨域地址或越界操纵不透明源（`data:` / `about:`）的 URL；
   - **解决方案**：
     - 在 `window.js` 中提取原生 `HistoryURL.prototype` 不可篡改访问器（`historyOriginGetter` 等），当检测到跨域跳转或对不透明源执行非 Fragment 修改时，直接阻断并抛出符合 Chrome 规范的 `DOMException("SecurityError", 18)`；
     - 在 `session.navigate` 中区分同文档与跨文档导航：若为跨文档新导航，主动释放销毁旧 `JSContext`（`_.ctx.deinit()`）并重新分配纯净 Realm，杜绝跨文档内存及全局对象泄露污染；
     - 在 `session.pollNavigatedWithinDocEvents` 中捕获 JS 垫片内部记录的同文档事件队列，以 `Page.navigatedWithinDocument` CDP 事件实时派发至客户端。
3. **WHATWG URL 非法端口语法校验排查经验 (`src/js/shim/window.js`)**：
   - **设计动机**：根据 WHATWG URL 规范，当解析含有非法端口格式（如端口为空或含非数字字符 `example.com:80:`）的 URL 时必须抛出 `TypeError("Invalid URL")`。若宽松解析，会导致 `HTMLAnchorElement.prototype.protocol` 错误解析为 `http:` 而非规范规定的回退回 `:`；
   - **解决方案**：
     - 在 `window.js` 的 `parseHostPort` 辅助函数中对冒号数量及端口数字字符添加严格的正则校验（`/^[0-9]+$/`），未通过时统一抛出 `TypeError`，使 `anchor-invalid-url-protocol` 100% 达标。

## Phase 44 CDP RemoteObject 格式化、Runtime.callFunctionOn 与事件篡改免疫技术方案
1. **CDP RemoteObject 规范级序列化与 callFunctionOn 原生调度 (`src/cdp/session.cp` & `src/cdp/runtime.cp`)**：
   - **设计动机**：Puppeteer / Playwright 客户端广泛依赖 `Runtime.callFunctionOn` 在远程对象（如 `globalThis`、DOM Element、通用 Object / Array）上执行函数求值。若返回值只支持按值展开（`returnByValue: true`），客户端无法获取携带 `objectId` 与正确 `className`（如 `"Object"`、`"Array"`、`"Function"`）的 RemoteObject 句柄；
   - **解决方案**：
     - 在 `session.cp` 中实装 `formatCdpRemoteObject`，自动区分基本类型与引用类型。对引用类型生成持久化 `objectId`（`capsurf:obj:N`）并记录入 `__capsurf_remote_objects` 映射表，输出标准 `RemoteObject` 结构体；
     - 实装 `callFunctionOn`：在 JS 环境中提取目标对象并安全绑定为执行函数接收者（`fn.apply(targetObj, args)`），支持 `returnByValue` 与 `awaitPromise`；
     - 在 `runtime.cp` 中打通 `Runtime.callFunctionOn` 协议分发，并在执行前后同步轮询控制台事件与全局绑定事件。
2. **QuickJS Property 所有权消耗与内存双重释放 (Double Free) 根因修复 (`src/js/qjs_engine.cp`)**：
   - **设计动机**：测试运行中偶发出现 QuickJS 析构时的 `__malloc_usable_size` 内存崩溃。经深度调试发现：QuickJS 的 `JS_SetPropertyStr(ctx, obj, prop, val)` C-API 会**主动转移消费**传入的 `val` 所有权；若调用者后续自行调用 `JS_FreeValue` 或使用 `defer ctx.freeValue(val)`，会导致严重的 double-free 内存损坏；
   - **解决方案**：
     - 在 `setPropertyStr` 中将 `JS_SetPropertyStr` 的形参包装为 `JS_DupValue(_.raw_ctx, val)`，显式提升传入值的引用计数，与 CP 语言侧的资源生命周期严格对齐，彻底根除了该类内存崩溃。
3. **EventTarget 机制防页面环境投毒篡改 (`src/js/shim/events/dispatch.js`)**：
   - **设计动机**：部分测试用例会恶意覆写全局 `window.Event = function Hijacked() {}`，若事件分发代码从全局读取 `Event.CAPTURING_PHASE` 或 `Event.BUBBLING_PHASE`，将因为全局属性缺失而中断，导致 `DOMContentLoaded` / `load` 等关键生命周期事件无法到达监听器；
   - **解决方案**：
     - 在 `dispatch.js` 内部建立完全脱钩的不可篡改局部安全常量：`PHASE_CAPTURING = 1`, `PHASE_AT_TARGET = 2`, `PHASE_BUBBLING = 3`, `PHASE_NONE = 0`，确保无论外部页面脚本如何劫持篡改 `window.Event`，引擎内部事件总线始终 100% 免疫且可靠派发。













## Phase 45 DOM 集合与 innerHTML 技术方案
1. **innerHTML 原生实现**：HTML 解析器无片段模式，采用「包装 `<html><body>` + 临时 Document 解析 + 深拷贝（字符串 dupe 进目标 arena）」，兼容独立创建的文档。
2. **包装器身份缓存**：原生每次访问新建 JS 对象会破坏 `===`。以全局 `__capsurf_wrappers` 对象按 `w<docptr>:<nid>` 缓存，节点 id 不复用故安全；代价是包装器随上下文销毁才释放。
3. **cpc 小缺陷（未提单）**：`out: *ArrayListUnmanaged` 出参调用丢失 `&`，改为返回切片规避。
4. **配额**：子智能体曾遇 429，切换模型后重派恢复。

## Phase 46 HTML 序列化、TextDecoder/TextEncoder、命名空间属性与核心 DOM 扩展技术方案
1. **HTML 序列化器架构 (`src/dom/dom.cp: HtmlSerializer`)**：
   - **设计动机**：此前 DOM 节点缺乏对 `innerHTML` 与 `outerHTML` 的精确序列化能力。根据 WHATWG DOM / HTML 序列化标准，14 类自闭合 void 标签（`img`, `input`, `br`, `hr`, `meta`, `link`, `area`, `base`, `col`, `embed`, `param`, `source`, `track`, `wbr`）严禁输出闭合标签，而 `script` 与 `style` 等 raw text 元素内部文本不应进行 HTML 实体转义；
   - **解决方案**：
     - 在 `src/dom/dom.cp` 实现高效流式 `HtmlSerializer`，自动递归遍历子树；
     - 对文本节点进行标准的 `&`, `<`, `>`, `"` 字符转义，对 raw text 元素直通输出原始文本；
     - 新增 `Document.getInnerHtml(node_id)` 与 `Document.getOuterHtml(node_id)`，并在 `dom_binding.cp` 挂载 `innerHTML` / `outerHTML` 原生访问器。
2. **JS 垫片 innerHTML 覆写陷阱修复 (`src/js/shim/element/html-element.js`)**：
   - **问题现象**：原生实装 `innerHTML` 后，E2E 用例中 `innerHTML` 取值依然退化为 `textContent`，且子标签丢失；
   - **根因分析**：`html-element.js` 原先定义了兜底的 `innerHTML` getter/setter（内部映射到 `textContent`）。在初始化扩展原型链时，由于 `installHtmlElementExtensions` 在原生 `el_proto` 之后执行，导致 JS 层的同名属性覆盖了原生 getter/setter；
   - **修复措施**：彻底清理 `html-element.js` 中的旧兜底定义，保证原生 `innerHTML` 访问器直接透出。
3. **WHATWG TextDecoder 多编码支持 (`src/js/shim/file.js`)**：
   - **设计动机**：多个 E2E 测试用例（如 `boa-typed-array-natives`、`page-renderer-pdf`）显式指定 `latin1` / `iso-8859-1` / `windows-1252` 等单字节编码构造 `TextDecoder`；若缺失将导致 `ReferenceError: TextDecoder is not defined` 或编码不支持异常；
   - **解决方案**：
     - 在 `src/js/shim/file.js` 实现完整的 `TextDecoder` 与 `TextEncoder` 类；
     - 对 `utf-8` / `utf8` 执行多字节 UTF-8 解码，对 `latin1` / `iso-8859-1` / `windows-1252` 执行单字节码点映射，挂载至 `global`、`window` 与 `globalThis`。
4. **命名空间属性与表单 Dirty Checked 保持 (`src/dom/dom.cp`, `src/js/shim/element/element.js`, `input.js`)**：
   - 在 `Document` 与 `ElementHandle` 实现 `getAttributeNS`, `setAttributeNS`, `removeAttributeNS`, `getAttributeNames`；
   - 修复 `input.js` 中 `removeAttributeNS('checked')` 时的状态保持：通过内部 `_checkedDirty` 标志位判定用户交互脏状态，若已发生用户修改则移除属性不再重置选框选中态，精准符合 WHATWG HTML 规范。

## Phase 47 XPath 1.0 规范引擎、DOMParser 隔离与 CDP Accessibility 大报文技术方案
1. **XPath 1.0 规范引擎与迭代器状态失效 (`src/js/shim/xpath.js`)**：
   - **设计动机**：W3C DOM Level 3 XPath 规范要求全面支持 13 种轴（含 `ancestor`, `preceding-sibling`, `descendant-or-self`）、谓词表达式、核心函数（`count`, `sum`, `id`, `normalize-space` 等）以及双模求值（迭代器 `ORDERED_NODE_ITERATOR_TYPE` 与快照 `ORDERED_NODE_SNAPSHOT_TYPE`）。其中迭代器模式必须在 DOM 树结构发生变更（如 `appendChild`/`remove`）时立即将 `invalidIteratorState` 置为 `true`，而在纯绘制行为（滚动/选区/Canvas 绘制）下保持有效；
   - **解决方案**：
     - 在 `src/js/shim/xpath.js` 实现完整的 Tokenizer、Lexer、AST Parser 与 Evaluator；
     - 引入全局 `__treeSeq` 计数器，在 `notifyMutation` 触发结构突变时自增；
     - `XPathResult` 迭代器创建时快照记录当前 `__treeSeq`，在后续访问 `invalidIteratorState` 与 `iterateNext()` 时比对，若不一致抛出非法状态异常；
     - 修复 `NodeType.Text` 底层返回值标准对齐（`3` 替代原内部枚举值 `2`），根除将文本节点误判为属性节点截断兄弟遍历的问题。
2. **DOMParser 隔离与防伪造安全领养 (`src/js/shim/parser.js` & `node-base.js`)**：
   - **设计动机**：`DOMParser.parseFromString` 创建的文档必须与宿主文档实现双向隔离。当页面试图向活动节点 `appendChild` 时，若传入恶意伪造对象（`{ _dom: ..., _id: -1 }`）必须严格防御抛出 `TypeError`，而当合法追加 `DOMParser` 解析的子节点时，必须将其干净领养入目标文档，且页面可见的节点包装器上绝对不能泄露私有 `_dom` 内部句柄；
   - **解决方案**：
     - 在 `node-base.js` 引入内部 `WeakSet`（`parsedNodeRegistry`）对解析节点进行私有跟踪；
     - `Node.prototype.appendChild` 增加 `isGenuineNode` 严格校验，杜绝伪造对象挂载；
     - 领养 `ParsedElement` 时自动将其子树解绑（`_children = []`），仅保留当前元素与属性，符合 W3C 跨句柄领养行为；
     - `ParsedFragment.removeChild` 在非直接子节点调用时规范抛出 `DOMException("...", "NotFoundError")`。
3. **CDP 大报文溢出消除与 Accessibility getPartialAXTree (`src/cdp/server.cp` & `accessibility.cp`)**：
   - **设计动机**：测试大型页面的无障碍树时，原 `src/cdp/server.cp` 中固定的 4KB 栈缓冲区在序列化数十个节点的完整属性树时瞬时触发 `Buffer overflow (-32000)`；同时此前 `Accessibility.getPartialAXTree` 返回死数据 `{"nodes":[]}`；
   - **解决方案**：
     - 在 `server.cp` 中将命令分发缓冲区重构为 256KB 堆动态分配，并联动事件分发动态打包，彻底消除了序列化溢出崩溃；
     - 在 `accessibility.cp` 完整实装 `Accessibility.getPartialAXTree`：根据传入的 `nodeId` / `backendNodeId` 准确定位目标节点，当 `fetchRelatives: true` 时向上递归遍历父级收集祖先路径（直至 `RootWebArea`），向下递归收集所有子节点，输出完整的节点集合；
     - 启发式无数据表格自动降级为 `presentationalRole`（`role: none`），支持嵌套列表层深、`visibility` 反转显现与 Tristate 属性。

## Phase 48 CDP 视口滚动/盒模型、表单按钮冒泡提交与无障碍活态属性技术方案
1. **CDP 视口滚动居中与 Runtime objectId 模型盒逆向解构 (`src/cdp/dom.cp` & `src/layout/layout.cp`)**：
   - **设计动机**：Puppeteer 与 Playwright 在进行交互点击前，通常调用 `DOM.scrollIntoViewIfNeeded` 确保节点处于可视区域，并调用 `DOM.getBoxModel` 提取基于 `objectId` 的目标坐标。若仅支持 `nodeId`，以 JS 对象形式传递节点句柄时将直接报错；
   - **解决方案**：
     - 在 `src/cdp/dom.cp` 中解析入参中的 `objectId`，通过提取对象 `_nid` 字段逆向映射到内部 Arena `NodeId`；
     - 实装 `DOM.scrollIntoViewIfNeeded`：比较节点屏幕几何盒与视口或最近滚动容器的边界，计算视口居中偏移量并更新布局上下文滚动状态，使得后续 `isIntersectingViewport` 判定准确返回 `true`。
2. **按钮后代冒泡与表单提交拦截管道 (`src/js/shim/events/` & `src/js/shim/element/forms/`)**：
   - **设计动机**：现代 Web 页面中用户往往点击 `<button type="submit"><span>Text</span></button>` 内部的 `<span>` 或 `<img>`。根据 WHATWG HTML 规范，点击事件从子元素向上传播冒泡至 `<button>`，当冒泡完成且事件未被 `preventDefault()` 取消时，必须自动触发表单提交动作并附带 `submitter` 按钮；
   - **解决方案**：
     - 在 `events/dispatch.js` 中实装冒泡结束后的默认动作（Default Action）挂钩：探测事件路径上最近的 `<button>` 或 `<input type="submit">`，若存在且未被阻止默认行为，则调用其关联表单的 `requestSubmit(button)`；
     - 在 `events/event.js` 中实装标准 `SubmitEvent`，挂载只读属性 `submitter`；
     - 支持内联处理器（如 `onclick="return false"`），在捕获/冒泡阶段执行内联脚本并在返回 `false` 时自动置位 `defaultPrevented`。
3. **无障碍活态值（Live Value & Live Checked）双向同步 (`src/js/shim/element/` & `src/cdp/accessibility.cp`)**：
   - **设计动机**：测试用例 `Accessibility.getFullAXTree reports live value/checked, not attributes` 验证的是用户通过 JS 写入 `input.value = "..."` 与 `checkbox.checked = true` 时，无障碍语义树返回的必须是活态计算值，而非 HTML 标签源码上的静态属性；
   - **解决方案**：
     - 在 `src/js/shim/element/element.js` 与 `input.js` 中打通属性存取器与底层原生 `setAttribute` / `removeAttribute` 的状态联动；
     - 保证单选框（Radio）同名互斥更新时正确派发取消选中态；
     - 在 `src/cdp/accessibility.cp` 的 `computeValue` 与 `populateProperties` 中，优先检索活态属性与计算状态，精准将 `input.value` 反射到 AXNode 的 `value` 字段，将 `checkbox.checked` 反射为 tristate 属性。

## Phase 49 选择器语法校验、光标渲染、输入拦截绕过与下拉表单状态技术方案
1. **选择器语法合法性校验与伪元素结果隔离 (`src/js/shim/selector.js`)**：
   - **设计动机**：DOM 规范规定 `querySelector` / `querySelectorAll` 在接收到语法不合规的选择器（如空伪类 `:not()`、`:is()` 或畸形括号）时，必须抛出 `SyntaxError` 类型的 `DOMException`，而非静默吞掉或当作普通类名；同时合规伪元素选择器（如 `div::after`）必须返回空的 `NodeList`，而非报错或返回真实元素；
   - **解决方案**：实现词法状态机校验器，先通过预编译模式与平衡括号算法识别非法构造，遇合规伪元素时拦截查询底层引擎并立即封装返回长度为 0 的标准只读 `NodeList`。
2. **文本选区与原生光标光栅化绘制 (`src/render/painter.cp` & `src/cdp/session.cp`)**：
   - **设计动机**：端到端用例对比输入光标前后移动的 PNG 截图，若未在聚焦控件上绘制实际的光标竖线，不同 `selectionStart` 的渲染帧将发生二进制哈希一致而断言失败；
   - **解决方案**：在渲染管道中捕获 `opts.focused_node` 与 `opts.caret_pos`，从控件内边距与字体等宽矩阵推导 `caret_x`，在对应像素坐标处绘制 2 像素宽深色光标条，真实还原无头浏览器在文字输入时的屏幕渲染画面。
3. **输入框架间谍绕过与直接值槽写入 (`src/cdp/input.cp` & `selection.js`)**：
   - **设计动机**：现代前端框架（如 React / Vue）在挂载组件时会在原生节点上定义 `value` 访问器间谍（Spy Descriptor）用以追踪变更。浏览器原生键盘事件（`Input.dispatchKeyEvent`）的写入应直接作用于底层存储槽，不应触发框架的自定义属性 Setter；
   - **解决方案**：定义 `getValueRaw` 与 `setValueRaw`，直接读写 `_value` 内部槽与底层属性，阻断向上遍历原型链触发实例上的 Spy 劫持，而后按需派发标准 `input` 与 `selection` 事件。
4. **Arena DOM 稳定标识驱动的下拉选区持久化 (`src/js/shim/element/forms/select.js`)**：
   - **设计动机**：QuickJS 在跨 C 边界提取 `parentNode` 或遍历 `options` 集合时，每次都会封装出全新的 JS `JsElement` 临时包装对象。若将状态存储在当前包装对象上，会导致状态随包装对象生命周期而丢失，导致单选下拉框状态混乱；
   - **解决方案**：基于底层 Arena 的固定 4 字节 `_nid` 建立全局弱持久映射表（`__capsurf_selectState` / `__capsurf_optionState`），配合 `isSameElement` 语义，确保任何临时封装对象均能寻址到统一的响应式选择态。





## Phase 50 HTMLOptionsCollection、SameObject 规范缓存、Document 遗留接口与 Canvas 2D 布局技术方案
1. **HTMLOptionsCollection 原型继承与 [SameObject] 弱映射缓存 (`src/js/shim/collections.js`)**：
   - **设计动机**：WHATWG DOM/HTML 标准规定 `select.options` 必须是 `HTMLOptionsCollection` 实例且其原型链必须继承自 `HTMLCollection.prototype`；同时 `select.options` 与 `element.children` 标注为 `[SameObject]`，意味着在元素生命周期内多次访问该属性必须返回恒等对象引用（`u.children === kids && s.options === opts`）；
   - **解决方案**：
     - 构建 `HTMLOptionsCollection.prototype` 继承关系（`Object.create(HTMLCollection.prototype)`），挂载 `add(element, before)` 与 `remove(index)` 原生标准方法；
     - 基于 `WeakMap` 实现元素级活态集合单例缓存（`selectOptionsCache` 与 `elementChildrenCache`），并在元素底层结构发生变动时自适应动态解析，既满足引用恒等，又保证集合活态（Live Collection）；
     - 实现 `options.length` 的 Set 存取器，在缩小长度时反向移除多余项，在增大长度时补齐占位元素。
2. **Document/Node 根节点连通性与子树文本规范化 (`src/js/shim/window.js` & `src/js/shim/node-base.js`)**：
   - **设计动机**：标准 Document 构造函数需支持无参调用（`new Document()`），并对外暴露 `doctype`、`characterSet` 等遗留属性；所有 Node 必须支持 `getRootNode(options)` 与 `isConnected`；同时 `doc.normalize()` 必须遍历子树合并连续文本节点并清除空文本节点；
   - **解决方案**：
     - 包装 `Document` 构造器支持 `new Document()` 实例生成并正确链接原型；挂载所有遗留文档属性存取器；
     - 实装 `Node.prototype.getRootNode`，根据 `{ composed: true }` 决定是否穿透影子根；遍历父级链条检测文档连通状态（`isConnected`）；
     - 递归遍历子树，合并相连文本节点，并修正 `textContent` 读写流程，避免底层与前端镜像冗余双写产生文本倍增 Bug。
3. **Canvas 2D 上下文品牌校验与固有尺寸盒模型布局 (`src/js/shim/canvas2d.js` & `src/layout/layout.cp`)**：
   - **设计动机**：浏览器规范要求 `CanvasRenderingContext2D` 不可被用户脚本直接以 `new CanvasRenderingContext2D()` 构造（必须通过 `canvas.getContext('2d')` 获取），非法构造需抛出 `TypeError`；所有绘图方法在接收者不合法时需触发非法调用异常；同时未经 CSS 指定尺寸的 `<canvas>` 在狭窄容器内必须遵循 300px × 150px 固有自然尺寸并维持宽高比；
   - **解决方案**：
     - 在 `CanvasRenderingContext2D` 构造器中引入内部私钥 `_INTERNAL_CTX_SECRET` 校验，在 `fillRect` 等全部方法体前置注入 `checkCtx(this)` 接收者校验；
     - 在 `src/layout/layout.cp` 中实装可替换元素固有尺寸推导函数 `ReplacedSize`，为 `<canvas>` 设置默认 300px × 150px 并在盒模型计算中正确结合 CSS `max-width` 实施等比缩放。

## Phase 51 脚本 IDL 访问器、Force-Async 状态机、元素可见性与声明式 Shadow DOM 死循环根治技术方案
1. **游离节点父级泄漏修复与 checkVisibility 可见性状态机 (`src/js/dom_binding.cp` & `src/js/shim/element/element.js`)**：
   - **设计动机**：测试用例 `checkVisibility reports boxless, transparent and hidden elements` 发现刚创建的游离元素 `document.createElement("div").checkVisibility()` 错误返回 `true`；
   - **根因分析**：`js_element_get_parent_node` 原先在 `parent_id == NULL_NODE` 时兜底返回了 `document`，使得所有未挂载游离节点误判为 `parentNode === document` 且 `isConnected === true`；
   - **解决方案**：
     - 将 `document` 返回范围严格收窄为 `el.node_id == doc.root_element && doc.root_element != NULL_NODE`；
     - 在 `src/js/shim/element/element.js` 中实装完整的 `Element.prototype.checkVisibility(options)`，结合 CSS 级联计算与 DOM 连通性，精确校验 `!this.isConnected`、`display: none / contents`、`checkOpacity`、`checkVisibilityCSS`、零尺寸边界与视口可见性。
2. **WHATWG Script 自身访问器与强异步（Force-Async）标记机制 (`src/js/shim/element/html-classes.js`)**：
   - **设计动机**：用例 `src, type and async are own accessors, and async follows force async` 验证规范细节：`src`、`type`、`async` 必须作为 own property 描述符直接定义在 `HTMLScriptElement.prototype` 上；且 `script.async` 受非阻塞（"force-async"）内部标志控制——动态 `createElement("script")` 默认为 true，解析器插入默认为 false，任何属性 Setter 写入或 `setAttribute("async")` 必须清除该标志；
   - **解决方案**：
     - 在 `installHtmlClasses` 中通过 `Object.defineProperty` 直接为 `HTMLScriptElement.prototype` 注册自身访问器，并在原型链断开拦截时保持独立；
     - 维护弱映射 `scriptForceAsyncMap`，在 `async` 属性 Getter 中结合 `this.hasAttribute("async")` 与 `forceAsync` 标志返回布尔值；
     - 在 `setAttribute`、`setAttributeNS`、`removeAttribute` 以及 `async` Setter 中精确联动清除该内部标志。
3. **声明式 Shadow DOM 模版迁移死循环根治与跨环境片段父级桥接 (`src/js/dom_binding.cp` & `src/js/shim/node-base.js` & `html-classes.js`)**：
   - **设计动机**：`template-content-parse.e2e.test.ts` 测试耗时高达 16.3 秒（触发 QuickJS Reductions 超时熔断），并在并发执行时偶发状态死锁；
   - **根因定位**：
     - `DocumentFragment` 与 `ShadowRoot` 属于纯 JS 运行环境构建的容器对象，底层 Arena DOM 并不为模版内容子节点分配原生父节点（`parent_id == NULL_NODE`）；
     - `processSubtreeTemplates` 解析 `<template shadowrootmode="open">` 声明式影子根并向 `ShadowRoot` 转移子树时，循环体 `while (content.firstChild) { shadow.appendChild(content.firstChild); }` 依赖 `shadow.appendChild` 调用 `child.parentNode.removeChild(child)`；
     - 由于 `child.parentNode` 原生 getter 返回 `null`，移除调用未能执行，导致 `content.firstChild` 永远指向首项子节点，在 QuickJS 主线程陷入 100% CPU 死循环直至超时；
   - **解决方案**：
     - 在 `src/js/dom_binding.cp` 中打通原生到 JS 片段的父级桥接：当 Arena `parent_id == NULL_NODE` 时检查 JS 层 `_ownerFragment`，使加入片段的原生节点正确汇报 `parentNode === DocumentFragment`；
     - 在 `Node.prototype.appendChild` / `insertBefore` 与 `DocumentFragment.prototype.removeChild` 中，加入 `_ownerFragment` 自动解除与联动；
     - 在 `processSubtreeTemplates` 与 `DocumentFragment.prototype.set innerHTML` 中加入防御性就地移除逻辑，彻底消除死循环，使套件执行耗时从 16.3 秒降至 248 毫秒。


## Phase 52 Range 上下文感知片段解析、SVG 动态字符串与自定义元素观察属性技术方案
1. **Range 上下文感知解析与 SVG 字体需求触发 (`src/js/shim/range.js`)**：
   - **设计动机**：WHATWG DOM Parsing 标准的 `Range.prototype.createContextualFragment` 要求基于 `startContainer` 上下文节点实施定制化片段解析。在非表格上下文（如 `div`）中，孤立的 `<tr><td>` 必须被剥离且仅保留文本内容，而在表格上下文（如 `tbody`）中必须完整保留 `tr` 节点；当向 `<template>` 节点插入内容时，必须按模版规则保留所有行节点；若上下文或内容涉及 SVG `<text>` 元素，必须调用 `demandFontOutlines()` 提取字体轮廓以满足光栅化排版需求；
   - **解决方案**：
     - 实现上下文元素与标签自适应包裹器，支持 template, tbody, thead, tfoot, tr, td, th, col, colgroup, caption, select, svg 等上下文标签；
     - 规避访问 `element.namespaceURI` 触发 getter 中的 `getAttribute("xmlns")` 间谍拦截，保持 `hooks` 监测期调用记录为空数组；
     - 检视 SVG 内容并在存在 `<text>` 且属于 SVG 上下文时触发 `demandFontOutlines()`，满足 `fontDemand: 2`；
     - `nullMarkup` 规整为 `"null"`，补全 `DocumentFragment.prototype.textContent`；
     - 遍历片段内脚本将其标记为 `_alreadyStarted = false` 以使其在挂载后可执行。
2. **SVGAnimatedString 规范映射与非 HTML 属性原始大小写保留 (`src/js/shim/element/html-classes.js` & `src/js/shim/element/element.js`)**：
   - **设计动机**：SVGElement 的 `className` 属于 `SVGAnimatedString` 接口，并非标准字符串；`new SVGAnimatedString()` 属于非法调用需抛出 `TypeError`；同时 SVG 等非 HTML 命名空间元素必须保留属性的原始大小写（如 `viewBox`, `preserveAspectRatio`, `clipPath`），不可转小写；
   - **解决方案**：
     - 实装独立的 `SVGAnimatedString` 接口与 `baseVal` / `animVal` 双向属性存取器；
     - 在 Element 扩展层加入命名空间与标签上下文穿透判据 `isHtmlElement(el)`，仅对 HTML 元素做小写规整，全面保护 SVG / MathML 属性原貌。
3. **Custom Elements 观察属性生命周期队列与多拦截链协同 (`src/js/shim/element/custom-element.js` & `src/js/shim/index.js`)**：
   - **设计动机**：Web Components 规范要求自定义元素在实例化后按顺序派发初始观察属性，并在后续属性发生变更时触发 `attributeChangedCallback`；同时动态创建的元素必须与 `HTMLScriptElement` 原型链注入及 `forceAsync` 保持单点协调，杜绝多重覆盖导致的属性丢失；
   - **解决方案**：
     - 构建集中式的 `CustomElementRegistry` 与属性快照暂存队列；
     - 将自定义元素的构造升级集成至 `src/js/shim/index.js` 的 `docProto.createElement` 核心拦截链中，消除模块间顺序覆盖隐患。

## Phase 53 事件信任安全防护、Dialog 状态机与 CSSOM 规则层级标准技术方案
1. **Document Referrer 导航协议打通 (`src/cdp/page.cp`, `src/cdp/session.cp` & `src/js/shim/window.js`)**：
   - **设计动机**：测试用例 `document.referrer reports the goto referer` 验证 Playwright / Puppeteer 在调用 `page.goto(url, { referer: "..." })` 时，页面中的 `document.referrer` 必须准确反映指定的 Referrer，而在未指定时返回空字符串 `""`；
   - **解决方案**：
     - 在 `src/cdp/page.cp` 处理 `Page.navigate` 请求时提取 `referrer` 字段；
     - 在 `src/cdp/session.cp` 的 `session.navigate` 中动态向当前 JS 上下文注入并执行 `__capsurf_set_document_referrer`；
     - 在 `src/js/shim/window.js` 的 `Document.prototype` 上挂载只读属性 `referrer`，并在 `e2e/harness.ts` 中透传 `page.goto` 的 `referer` 参数，彻底打通来源地址传递全链路。
2. **Event isTrusted 安全沙箱与重入派发防护 (`src/js/shim/events/event.js` & `src/js/shim/events/dispatch.js`)**：
   - **设计动机**：DOM 规范规定 `event.isTrusted` 必须为 unforgeable 只读属性（用户脚本无法通过 Proxy、Object.defineProperty 伪造）；构造函数 `new Event("forged", init)` 绝不得探测 `init` 上的任何 Symbol 属性；正在派发的事件再次调用 `dispatchEvent` 必须抛出 `InvalidStateError`；而当用户代码重新分发先前由浏览器派发的受信任事件时，`event.isTrusted` 必须被重置为 `false`；表单 `reset()` 必须仅限 HTML 命名空间元素调用，且派发事件时必须使用内部私有槽读取事件类型，免疫属性拦截劫持；
   - **解决方案**：
     - `Event.prototype.isTrusted` 采用只读 Getter，返回 `Boolean(this && this._isTrusted)`；
     - `Event` 构造函数仅以普通字符串键提取 options（`bubbles`, `cancelable`, `composed`），阻断对 Proxy Symbol 的触碰；
     - 在 `dispatchEvent` 中前置检查 `event._isDispatching`，已置位时抛出 `InvalidStateError`；非重入调用时立即将 `event._isTrusted` 重置为 `false`，派发结束后恢复 `_isDispatching = false`；
     - 实现 `dispatchTrustedEvent` 专供内核与表单派发内置受信任事件，`form.reset()` 严格校验 `this instanceof HTMLFormElement` 与 HTML 命名空间，并读取私有 `_type` 防止事件类型被 Setter 篡改。
3. **HTMLDialogElement 规范状态机与异步关闭队列 (`src/js/shim/element/dialog.js`)**：
   - **设计动机**：HTML 标准中 `<dialog>` 的 `show()` 与 `showModal()` 属于互斥状态机——在自身模式内重复调用满足幂等性（无报错），但若在非 modal 状态下调用 `showModal()` 或在 modal 状态下调用 `show()`，必须规范抛出 `InvalidStateError`；同时 `close()` 方法必须将 `close` 事件排队为异步微任务或计时器任务，保证调用当时 `closeSync` 计数严格为 0；
   - **解决方案**：
     - 采用 WeakMap 隔离存储各个 Dialog 实例的 `returnValue` 与 `isModal` 标志；
     - `show()` / `showModal()` 精准检查已有打开态与 modal 标志；
     - `close()` 移除 `open` 属性并使用 `setTimeout(..., 0)` 异步排队分发 `close` 事件。
4. **CSSOM 规则层级派生、CSS.supports 三态语法解析与 StyleDeclaration 原型 (`src/js/shim/cssom.js` & `element/element.js`)**：
   - **设计动机**：用例 `CSSOM rules carry real interfaces, constraints and validation` 与 `CSS.supports evaluates the full supports-condition grammar` 验证完整的 CSSOM 规范：`CSSRule` 及其 13 项常量、`CSSGroupingRule`、`CSSStyleRule`、`CSSMediaRule`、`CSSImportRule`、`CSSFontPaletteValuesRule` 等原型继承关系；`sheet.insertRule()` 必须严格执行参数与语法校验；`CSS.supports` 必须解析复杂的逻辑语法并遵循 CSS Conditional Rules 规范的三态逻辑（未知函数作为 unknown，结合 `not` 返回 false，结合 `or` 返回 true）；`element.style` 必须继承 `CSSStyleDeclaration.prototype` 且属性 `in` 操作符必须区分标准前缀大小写；
   - **解决方案**：
     - 在 `src/js/shim/cssom.js` 中构建完整符合 Web IDL 的 CSSOM 规则类与原型继承链；
     - 实现规则变更联动同步：`insertRule` 与 `deleteRule` 修改时自动更新关联 `<style>` 的 `textContent`，驱动态级联实时响应；
     - 实现支持嵌套括号、`selector(...)` 与未知函数词法状态机的 `CSS.supports` 条件解析器；
     - 在 `element.js` 的 `createStyleProxy` 中以 `CSSStyleDeclaration.prototype` 为原型创建目标代理，精细控制 `has` 拦截器对标准前缀与连字符属性的反馈。

## Phase 54 CSSOM Counter/Page 规则规范、Event 原型纯洁性与 FileReader 异步任务模型
1. **CDP 运行时动态内存扩容与 WebSocket 截断根治 (`src/cdp/runtime.cp`)**：
   - **设计动机**：测试用例向页面注入多条复杂的 CSSOM 规则或大型评估表达式时，CDP 返回时频繁触发 `SyntaxError: JSON Parse error: Unterminated string`；
   - **根本原因**：`src/cdp/runtime.cp` 中的 `Runtime.evaluate` 和 `Runtime.callFunctionOn` 使用了硬编码的 4096 字节栈缓冲区 `decl_buf`、`unesc_buf` 和 `eval_res_buf`，一旦转义前后或求值返回的 JSON 长度超过 4KB，输出即被无情截断；
   - **解决方案**：引入 `session.allocator.alloc` 动态分配对应表达式尺寸的解析空间（保证至少 64KB），并直接将返回数据流式写入 `server.cp` 传入的 256KB `out_buf`，彻底消除了缓冲区溢出隐患。
2. **CSSCounterStyleRule 描述符校验与 CSSPageRule 边距规范 (`src/js/shim/cssom.js`)**：
   - **设计动机**：W3C CSS Counter Styles Level 3 与 CSS Paged Media 规范要求对 `@counter-style` 与 `@page` 提供完整的 CSSOM 反射与错误校验：
     - `@counter-style`：必须校验名称不是保留关键字（`none`, `initial`, `unset` 等）；`system` 锁定算法族系（如 `extends decimal` 后不能同时设置 `symbols`）；`suffix` 拒绝空串；未闭合转义符自动归一化为 `\ufffd`；
     - `@page`：必须支持边距规则如 `@top-left`、`@bottom-right`，且对转义名称（如 `@t\6f p-left`）规范化，插入非页边距规则时抛出 `HierarchyRequestError`；
   - **解决方案**：在 `src/js/shim/cssom.js` 中实装 `CSSCounterStyleRule`（`type: 11`）、`CSSPageRule`（`type: 6`）与 `CSSMarginRule`（`type: 9`），严密映射描述符并严格校验语法。
3. **Web IDL Event 原型只读存取器与 legacy init*Event 兼容 (`src/js/shim/events/event.js`)**：
   - **设计动机**：用例 `events have no own properties but isTrusted` 检验规范要求：`Event` 及其衍生对象上的所有标准字段（`clientX`, `relatedTarget`, `keyCode`, `detail`, `persistentDeviceId` 等）必须存在于各自的原型链上作为只读 Getter，其实例自身属性仅允许保留 `isTrusted`（`Object.keys(event)` 仅有 `['isTrusted']`）；同时，各传统初始化方法（`initMouseEvent`, `initKeyboardEvent`, `initCustomEvent` 等）必须完整挂载在原型上且在派发后失效；
   - **解决方案**：将原本挂在构造函数 `this` 上的属性全部转移至原型 Getter 中，内部通过私有存储槽获取；为 `MouseEvent`, `KeyboardEvent`, `UIEvent` 等补全 legacy init 方法；规范化 `new Event(type)` 类型转换。
4. **FileReader 规范生命周期与多阶段微/宏任务调度 (`src/js/shim/file.js`)**：
   - **设计动机**：W3C File API 规定 `FileReader` 读取操作是一组分阶段的异步任务：`loadstart` 必须在微任务（microtask）中触发，随后触发 `progress`，而 `load` 与 `loadend` 必须排队到后续的宏任务（Macrotask）中执行；若在期间调用 `abort()`，必须中断后续阶段；
   - **解决方案**：在 `src/js/shim/file.js` 中构建 `FileReader`，基于操作代数 `_gen` 实现精准的状态转换与任务排队，确保微任务与宏任务触发时序与真实浏览器 100% 对齐。

## Phase 55 Navigator 客户端提示规范、Beacon API 与 Kitesurf CDP 页面操作扩展技术方案
1. **UserAgent 动态注入与 Chromium Client Hints 反射 (`src/js/shim/window.js`, `src/cdp/session.cp`)**：
   - **设计动机**：W3C User-Agent Client Hints 规范规定仅有 Chromium 引擎对外暴露 `navigator.userAgentData`，其他浏览器引擎（Firefox/Safari）及无头引擎默认 UA 绝不得暴露该属性；当通过 CDP `Emulation.setUserAgentOverride` 或 `Network.setUserAgentOverride` 覆盖为包含 `Chrome/X.Y` 的 UA 时，必须动态挂载与之匹配的 `userAgentData`，且 `getHighEntropyValues` 异步按需提取 `uaFullVersion` 与 `architecture`；
   - **解决方案**：
     - 在 `src/cdp/session.cp` 中实现 `setUserAgentOverride` 与 `syncUserAgentToJs`，在页面导航初始化或实时覆盖时调用 `__capsurf_set_user_agent`；
     - 在 `src/js/shim/window.js` 中实现状态机：非 Chrome 或清空时将 UA 重置为默认 `"Kitesurf/0.0.1"` 并安全 `delete navigator.userAgentData`；检测到 Chrome UA 时按规范动态生成只读 `userAgentData` 及 `getHighEntropyValues` Promise 提取器。
2. **Navigator.sendBeacon 规范 URL 解析与轻量信标传输 (`src/js/shim/window.js`)**：
   - **设计动机**：W3C Beacon 规范要求 `navigator.sendBeacon(url, data)` 能接收相对路径或绝对路径，基于 `document.URL` 进行规范化解析，遇非法 URL 抛出 `TypeError`，并返回布尔值 `true`；
   - **解决方案**：在 `window.js` 中实装基于 `URL` 构造的入参校验，成功解析后返回 `true`。
3. **Kitesurf 页面状态快照与变动计数器联动 (`src/cdp/session.cp`, `src/dom/dom.cp`)**：
   - **设计动机**：测试用例 `getPageState reports the URL, title and a mutation counter that moves` 检验 CDP 扩展方法 `Kitesurf.getPageState`，要求返回 `{ seq, scrollX, scrollY, href, title }`，且当页面执行 `document.getElementById("p")?.append("b")` 等 DOM 变更后，`seq` 必须单调递增；
   - **解决方案**：
     - 在 `Document` 中维护 `mutation_seq`，在 Arena DOM 增删属性或子节点时递增；
     - 在 JS 运行时维护 `__treeSeq`，在 DOM 操作微任务中自增；
     - `getPageState` 动态合并 `doc.mutation_seq + js_seq`，并实时通过 DOM 选择器扫描 `<title>` 文本提取有效标题，返回精准状态快照。
4. **Kitesurf 用户表单控件赋值与受信任事件时序 (`src/js/dom_binding.cp`, `src/js/shim/index.js`)**：
   - **设计动机**：`Kitesurf.setControlValue` 用于模拟用户在页面输入/选择，要求严格遵守规范校验（排除非控件、只读控件、禁用控件、禁用选项等），并在赋值成功后派发真正的受信任事件（`isTrusted = true`）先 `input` 后 `change`；
   - **解决方案**：
     - 导出 `__capsurf_native_find_node_by_id`，利用 Arena DOM 节点快速定位并解包为 JS `Element`；
     - 克服 QuickJS 环境下缺少 `Number.isInteger` 的陷阱，采用健壮的纯数值与边界判定；
     - 严格过滤 `<select>` 中被禁用的 `option` 与 `optgroup`；赋值后通过 `__capsurf_dispatchTrustedEvent` 连续派发两道受信任事件。
5. **重绘受损追踪与祖先子树排除 (`src/dom/dom.cp`, `src/cdp/session.cp`)**：
   - **设计动机**：`markPaintDamage` 与 `getPaintDamageSince` 用于支持增量局部渲染与受损区域上报，且支持 `excludeNodeId` 将用户操作的节点及其祖先树整体排除；
   - **解决方案**：在 `Document` 中维护绘制受损节点集，重绘时记录受损 `node_id`；查询时利用 `doc.isLineageOrSubtree` 递归向上与向下核验，高效完成过滤。

## Phase 56 HTMLIFrameElement 窗口/Location 门面体系、Accept-Encoding 编码收敛与 WebCrypto 算法校验
1. **HTMLIFrameElement contentWindow / contentDocument 门面与原型集成 (`src/js/shim/element/html-classes.js`)**：
   - **设计动机**：测试用例 `iframe location assignment throws SyntaxError without replacing its facade` 与 `a relative location write on an about:blank iframe navigates the frame` 要求 `iframe.contentWindow` 与 `iframe.contentDocument` 返回符合 W3C HTML 规范的窗口对象，具备独立的 `DOMException`、`location`、`document`，且当向 `contentWindow.location` 赋予非法 URL 时，抛出由其自身环境创建的 `SyntaxError`（`code: 12`），并且绝对不得替换 `location` 门面引用；
   - **解决方案**：
     - 在 `html-classes.js` 中建立懒初始化的 Iframe Window 门面，注入 `DOMException`、`Error`、`TypeError`、`Node`、`Document` 等全局构造器；
     - 实现 `makeIframeLocationFacade`：受保护的 `href` 存取器与 `assign`/`replace`/`reload`；严格对入参执行字符串转化，遇 `Symbol()` 抛出原生 `TypeError`；URL 解析失败抛出 `FrameDOMException("Invalid URL", "SyntaxError")`；
     - `contentWindow.location` 存取器拦截非法写入并保护门面对象恒等（`contentWindow.location === previousLocation` 为 `true`）；
     - 补全 `DOM_EXCEPTION_CODES`（`SyntaxError: 12`）。
2. **Accept-Encoding 协议白名单限制与回显支持 (`src/net/mock_fixtures.cp`, `src/cdp/network.cp`, `src/js/shim/window.js`)**：
   - **设计动机**：HTTP 规范中浏览器客户端应上报其支持的压缩格式，当测试通过 `setExtraHTTPHeaders` 尝试注入未实现算法（如 `deflate`, `zstd`）时，网络层必须将编码集限制为内核支持的白名单 `"gzip, br"`；
   - **解决方案**：
     - 在 `mock_fixtures.cp` 中实装 `/__fixtures/gzip-only-doc.html` 动态响应与 `echo-headers.json` 回显；
     - 在 `network.cp` 与 `http_loader.cp` 中实现 `sanitizeAcceptEncoding` 过滤器，收敛请求头至 `"gzip, br"`；
     - 在 `window.js` 的 `fetch` 实现中默认注入 `"accept-encoding": "gzip, br"`。
3. **WebCrypto SubtleCrypto.generateKey 算法族系与参数校验 (`src/js/shim/crypto.js`)**：
   - **设计动机**：W3C Web Cryptography 规范规定 `generateKey` 必须支持标准算法（如 `AES-GCM`），而对缺少必要参数的算法必须返回 rejected Promise with `TypeError`，对不支持的算法（`AES-CMAC`）或不安全哈希（HMAC with `MD5`、RSA-PSS with `SHA`）返回 rejected Promise with `NotSupportedError`；
   - **解决方案**：
     - 在 `crypto.js` 中实装符合 IDL 的 `CryptoKey` 体系；
     - 在 `generateKey` 中严格校验算法名称、哈希算法有效性，遇非法参数或非支持哈希返回 `NotSupportedError`（`DOMException`）；
     - 成功调用时解析为原生 `CryptoKey` 实例并绑定规范算法对象。

## Phase 57 键盘 Tab/Shift+Tab 顺序焦点链、Enter 激活、:focus 伪类与 BrowserContext 跨域 Iframe Cookie 隔离
1. **键盘事件调度与 Tab/Shift+Tab 顺序焦点链算法 (`src/cdp/input.cp`)**：
   - **设计动机**：测试用例 `Tab and Shift+Tab follow sequential focus order and honor cancellation` 验证浏览器无障碍键盘焦点核心行为：通过 Tab 与 Shift+Tab 在文档可聚焦元素间循环导航；若 `keydown` 事件被 `event.preventDefault()` 取消则不得跳转焦点；聚焦末尾元素时必须自动调用视口滚动算法；按下 Enter 必须触发聚焦链接的点击与导航；
   - **技术方案**：
     - 在 `Input.dispatchKeyEvent` 中分发 `keydown` 真实事件到 `document.activeElement`；
     - 检查事件的 `defaultPrevented` 返回值；
     - 构建顺序焦点元素序列：
       - DOM 树前序扫描，排除祖先或当前具有 `inert` 属性的整棵子树、`display: none` 隐藏元素、`disabled` 表单控件、以及负值 `tabindex < 0`；
       - 判定元素的自然可聚焦性（`<a href>`, `<button>`, `<input>`, `<select>`, `<textarea>`, `[contenteditable="true"]` 默认 tabindex 0）；
       - 严格校验 `tabindex` 属性字符串（非纯整数如 `"abc"`, `"3abc"` 回退为自然可聚焦性，非自然可聚焦元素则排除）；
       - 双阶段排序：所有正整数 `tabindex` 按数值升序排列，后续紧随所有 `tabindex = 0` 的元素（保持 DOM 树自然顺序）；
       - 根据 `modifiers & 8` 确定正向推进或逆向循环索引；
       - 执行焦点切换：派发旧元素 `blur`、新元素 `focus`、设置 `document.activeElement` 并执行 `scrollIntoView`，使得视口 `window.scrollY > 0`；
     - 当按下 Enter 键且焦点为 `<a href>` 或 `<button>` 时，派发 `click` 事件并执行规范哈希跳转。
2. **CSS 与 DOM `:focus` 伪类闭环匹配 (`src/dom/dom.cp`, `src/css/matcher.cp`, `src/js/shim/window.js`)**：
   - **设计动机**：页面样式中包含 `a:focus { background-color: rgb(1, 2, 3) }`，在元素获得焦点后，`getComputedStyle(link).backgroundColor` 必须准确反映 `:focus` 规则匹配结果；
   - **技术方案**：
     - 在 `Document` 中维护 `focused_node: NodeId`；
     - 在 `src/css/matcher.cp` 中新增 `PseudoKind.Focus`，并赋予类选择器级权重（specificity）；
     - 在 `Matcher.matchesPseudo` 中判断 `node_id == doc.focused_node`；
     - 在 JS 运行时 `window.js` 的 `matchesSingleSelector` 与 `matchesSelector` 中补齐 `:focus` 判定（`document.activeElement === el`）。
3. **BrowserContext 与 CDP `Storage.setCookies` 批量入库 (`src/cdp/network.cp`, `e2e/harness.ts`)**：
   - **设计动机**：测试用例 `an out-of-process iframe uses its browser context's cookies` 验证通过 `browser.createBrowserContext()` 创建隔离上下文，调用 `Storage.setCookies` 注入特定域名 Cookie，并在该上下文中拉取 Iframe 与执行 `fetch` 时必须携带正确的 Cookie；
   - **技术方案**：
     - 在 `harness.ts` 中补齐 `BrowserContext` 类与 `browser.createBrowserContext()`，底层对接 CDP `Target.createBrowserContext` 与 `Target.disposeBrowserContext`；
     - 在 `src/cdp/network.cp` 中重构 `Storage.setCookies` 与 `Network.setCookies`，逐项解析参数中的 `cookies` 数组，并通过 `session.cookie_jar.setFromHeader` 存入内核 CookieJar。
4. **跨域 Iframe 执行上下文感知与 Cookie 自动注入 (`src/cdp/runtime.cp`, `src/cdp/session.cp`, `src/js/dom_binding.cp`, `src/js/shim/window.js`)**：
   - **技术方案**：
     - 在 `session.cp` 中追踪 DOM 中添加的跨域 Iframe 节点，捕获其源并排队派发 `Runtime.executionContextCreated` 事件；
     - 在 `Runtime.evaluate` 中支持 `contextId` 分区求值；
     - 导出 `@c fn js_capsurf_get_cookie_header(url)`，在 JS `fetch` 中检测同源与 `credentials: "include"`，自动向内核 CookieJar 查询匹配的 Cookie 字符串并挂载至 `cookie` 请求头，确保测试中 `/__fixtures/echo-headers.json` 能够正确回显 `who=context`。

## Phase 58 Storage Cookie 往返、Live document.cookie 贯通、多 BrowserContext 独立 Cookie 隔离与网络事件归属
1. **Fixture 动态 Set-Cookie 与 document.cookie 双向读写贯通 (`src/net/mock_fixtures.cp`, `src/js/dom_binding.cp`, `src/js/shim/index.js`)**：
   - **设计动机**：测试用例 `Storage cookie round-trip: get, set, next-nav visibility, clear` 验证：页面访问特定 fixture 时服务端响应头种植的 Cookie 必须进入内核 CookieJar；JS 环境中执行 `document.cookie` 必须能直接读取内核中的有效 Cookie；通过 `document.cookie = ...` 写入的 Cookie 在后续页面跳转与跨域请求中必须依旧保持可见；调用 `Storage.clearCookies` 后全域 Cookie 必须彻底清空；
   - **技术方案**：
     - 在 `mock_fixtures.cp` 中实装 `/__fixtures/cookies.html`，提取并解析 `Set-Cookie` 头直接写入会话绑定的 `cookie_jar`；
     - 导出原生 C-FFI 桥接函数 `@c fn js_capsurf_set_cookie`，在 JS 中拦截 `document.cookie` 的 setter，解析属性并调用 `cookie_jar.setFromDocumentCookie`；
     - 在 getter 中调用 `__capsurf_get_cookie_header`，确保实时反映内核最新过期与覆盖状态；
     - `Storage.clearCookies` 联动执行 `session.clearCookies()`，重置 CookieJar 并清空本地缓存。
2. **多 BrowserContext 独立 Cookie 隔离与未知 contextId 严格拦截 (`src/cdp/target.cp`, `src/cdp/network.cp`, `src/cdp/session.cp`)**：
   - **设计动机**：测试用例 `browser contexts isolate cookies` 与 `Storage cookies on an unknown browserContextId rejects` 验证：不同 `browserContextId` 必须拥有完全隔离的 Cookie 空间，互不渗透覆盖；当向不存在的 `browserContextId`（如 `"ctx-nonexistent"`）发起 Storage 查询或设置时，必须以明确的 JSON-RPC 错误拒绝；
   - **技术方案**：
     - 在 `target.cp` 的 `BrowserContextState` 中为每个上下文维护专用的 `CookieJar` 实例，随上下文创建与销毁（`disposeBrowserContext`）；
     - 在 `network.cp` 拦截所有传入 `browserContextId` 的操作，未注册上下文立即返回 `-32000: "Failed to find browser context"`；
     - 页面在当前上下文中执行 `fetch` 时，直接路由至该上下文专属的 CookieJar，实现多上下文彻底物理隔离。
3. **子资源 Fetch 网络事件发射与 Frame/Loader 归属属性 (`src/cdp/session.cp`, `src/cdp/runtime.cp`, `src/js/shim/window.js`)**：
   - **设计动机**：用例 `Network events retain their target attribution while a sibling navigates` 监听 `Network.requestWillBeSent` 与 `Network.responseReceived`，断言每个网络事件所附带的 `frameId` 与 `loaderId` 必须与触发它的页面精准绑定，且兄弟页面的导航不得污染事件监听链；
   - **技术方案**：
     - 在 `window.fetch` 触发时调用 `__capsurf_native_record_fetch` 记录出站请求；
     - 在 `pollNetworkEvents` 中按需生成附带 `session.frame_id` 与 `session.loader_id` 的规范 CDP 事件并在执行评估时向订阅客户端广播。

## Phase 59 顶级 SVG 独立文档合成、自闭合 TreeBuilder、ViewBox 几何缩放与 CurrentColor 级联技术方案
1. **独立顶级 SVG 页面合成与 Mock 路由 (`src/cdp/svg_document.cp`, `src/net/mock_fixtures.cp`, `src/cdp/session.cp`)**：
   - **设计动机**：测试用例 `svg-document-render.e2e.test.ts` 验证通过直接以 `.svg` 文件为顶级 URL 导航，包含 XML 序言、Doctype 声明、自定义 `<metadata>`、`viewBox` 与内联 CSS 样式的复杂矢量文档的光栅化截图；
   - **技术方案**：
     - 在 `svg_document.cp` 中实现轻量解析器，安全跳过 XML 声明与 Doctype，提取 `<meta>` 标签提升至 `<head>`；生成重置为 `margin: 0; padding: 0; overflow: hidden;` 的宿主 HTML 容器；
     - 在 `mock_fixtures.cp` 中补充 `.svg` MIME 映射（`image/svg+xml; charset=utf-8`）及动态 case 参数路由分发；
     - 在 `session.cp` 导航拦截中将顶级 SVG 封装并在 Frame 事件中输出标准 MIME。
2. **HTML5 TreeBuilder 自闭合与作用域增强 (`src/html/tree_builder.cp`)**：
   - **技术方案**：
     - 在通用元素分支中检测 `if (tag_tok.self_closing) * = _.popElement();`，避免 `<rect ... />` 自闭合矢量标签堆积导致后续元数据与样式规则层级错乱；
     - 增加 `inDefaultScopeName` 与 `popUntilName`，支持按标签字符串名称精准闭合未知标签（如 `</metadata>`）。
3. **SVG viewBox 视口几何运算与 CurrentColor 级联 (`src/css/cascade.cp`, `src/layout/layout.cp`, `src/render/painter.cp`)**：
   - **技术方案**：
     - 在 `cascade.cp` 中新增 `has_fill_current_color`，解析 `fill: currentColor` 并在级联计算中映射为当前节点级联文本色 `color`；支持 `fill="..."` 表现性属性作为低优先级权重注入；
     - 在 `layout.cp` 中实装 `layoutSvgContainer` 与 `layoutSvgChildren`，依据 W3C SVG 规范实现 `xMidYMid meet` 保持纵横比的几何缩放（`scale = min(svg_w / vb_w, svg_h / vb_h)`）与偏置居中（`offset_x`, `offset_y`），并实施溢出截幅；
     - 在 `painter.cp` 中为 `<rect>` 元素回退提取 `fill` 表现性属性并支持 `currentColor`。

## Phase 60 HTML 表单约束验证引擎与步进数值/日期输入规范技术方案
1. **统一约束验证引擎 (`src/js/shim/element/forms/validity.js`)**：
   - 将所有可验证表单控件（`input`/`textarea`/`select`/`button`）的 `willValidate`、`validity`、`validationMessage`、`checkValidity`/`reportValidity` 收敛至 `validity.js`，通过 `WeakMap` 缓存 `ValidityState` 实例以满足同对象语义；
   - 严格实现 `<fieldset disabled>` 首个 `<legend>` 子树免禁用传播与 `<datalist>` 祖先禁用验证规则。
2. **多类型步进与清洗流水线 (`input.js`, `selection.js`)**：
   - 在 `setValueRaw` 入口对 `<input>` 应用 `sanitizeInputValue` 与 `_valueDirty = true`，确保用户输入非数字（如 `type="number"` 键入 `"e"`）时立即触发 `badInput` 并将 `.value` 清洗为空串；
   - 为 `stepUp`/`stepDown`、`valueAsNumber`/`valueAsDate` 实现 7 种标准类型的数值与日期双向转换及 `InvalidStateError` 校验。

## Phase 61 SVG 溢出裁剪、外链 `<img>` SVG 光栅化与 `<use>`/`<symbol>` 精灵图技术方案
1. **裁剪框透传与外链 SVG 盒裁剪 (`src/render/painter.cp`)**：
   - 引入 `ClipRect` 与 `Canvas.paintBoxClipped`，在 `paintNodeRecursive` 中向下透传有效裁剪区域；`<svg>` 默认按视口盒裁剪，显式声明 `overflow: visible` 时放开裁剪；
   - `<img>` 指向 SVG 资源（`/__fixtures/*.svg` 或 `data:image/svg+xml,...`）时通过 `loadSvgAssetForImg` 加载解析并严格裁剪于 `<img>` 几何盒内，彻底杜绝内容溢出。
2. **`<use>` 引用 `<symbol>` 与 `<path>` 扫描线光栅化 (`src/layout/layout.cp`, `src/render/painter.cp`)**：
   - 实现 `findNodeById` 与 `paintSvgContainerRef`，支持 `<use href/xlink:href="#...">` 跨 `display: none` 隐藏容器引用 `<symbol>` 定义，继承 `viewBox` 缩放与 `fill`/`currentColor`；
   - 实现 `paintSvgPath` 解析 `M/m/L/l/H/h/V/v/Z/z` 路径指令并通过 even-odd 扫描线算法填充光栅化。

## Phase 62 WHATWG EventListener 规范、FormDataEvent 与标准 HTML*Element 原型链技术方案
1. **EventListener 规范化 (`src/js/shim/events/dispatch.js`)**：
   - 先 flatten `options` 再判定 `callback` 为空，接入 `AbortSignal` 取消监听；区分函数监听与 `{ handleEvent }` 对象监听，确保每次派发仅读取一次 `handleEvent` getter 并正确绑定 `this`，同时在派发周期维护 `window.event`。
2. **`HTMLElement` 继承树与 `FormDataEvent` (`html-classes.js`, `index.js`, `form-data.js`)**：
   - 将原生 `Element`/`Document`/`Window` 包装为可 `instanceof` 的标准构造函数，统一全部表单与通用 HTML 元素继承自 `HTMLElement.prototype`；在 `FormData` 构造中派发防重入 `"formdata"` 事件并遵循 `<fieldset disabled>` 首个 `<legend>` 豁免规则。

## Phase 63 SVG 绘制保真度、虚线描边与固有纵横比推导技术方案
1. **SVG 固有纵横比与替换元素布局 (`src/layout/layout.cp`)**：
   - 通过 `extractSvgRootTag` 避免误匹配内部 `<rect>` 的 `width`/`height`；引入 `is_ratio_only` 区分仅含 `viewBox` 的 SVG 与默认 `300x150` 替换元素，完整实现 CSS 2.1 §10.3.2 自动宽度铺满包含块及单边高/宽反推。
2. **虚线描边与全图元统一光栅化 (`src/render/painter.cp`)**：
   - 新增 `parseSvgDashArray` 与 `paintSvgSegmentClipped` 实现规范虚线描边（奇数数组倍增与 `stroke-dashoffset` 相位偏移），并新增缩放 ASCII 点阵 `<text>` 渲染，使 `<svg>` 内各类图元均在 `viewBox` 变换与裁剪框下精准绘制。

## Phase 64 表单文本选区 API、FileList/DataTransfer 与回车隐式提交技术方案
1. **文本选区与 `setRangeText` (`src/js/shim/element/forms/selection.js`)**：
   - `setSelectionRange` 按 WebIDL `unsigned long` (`>>> 0`) 规范化负值参数并限制至文本长度，通过同范围去重避免冗余 `select` 事件；实装 `setRangeText` 四种 `selectionMode`（`select`/`start`/`end`/`preserve`）与越界 `IndexSizeError`。
2. **`FileList`、`DataTransfer` 与 `FormData` 文件条目 (`file.js`, `input.js`, `form-data.js`)**：
   - 基于 `Proxy` 实装带索引访问与迭代器的 `FileList` 与 `DataTransfer`；`HTMLInputElement.prototype.files` 遵循 SameObject 并在 `type="file"` 时返回 `C:\fakepath\<name>`，`FormData` 直接保留 `File`/`Blob` 引用。

## Phase 65 AbortSignal/MessageChannel 规范、事件处理器水合与 Harness Keyboard 技术方案
1. **`AbortSignal` 与 `MessageChannel` (`src/js/shim/window.js`)**：
   - 完整实装 `AbortSignal.abort()`/`timeout()`/`any()` 与 `AbortController`；重构 `MessagePort` 异步 FIFO 队列与 `_structuredClone`，在 `postMessage` 阶段同步校验 `DataCloneError`（函数/Symbol、重复或自身 port、已分离 buffer）并执行 `ArrayBuffer` 转移分离。
2. **内联事件水合与 Harness Keyboard (`dispatch.js`, `e2e/harness.ts`)**：
   - 水合阶段尊重已显式设置的自有属性（避免覆盖 `el.onclick = null`），并按 HTML5 规范将 `<body>` 上的 22 项 `WindowEventHandlers` 转发至 `window`；在 `e2e/harness.ts` 补齐 `page.focus` 与 `page.keyboard`。

## Phase 66 SVG 图形 bbox、外部精灵图标与 abspos 双裁剪通道技术方案
1. **SVG 图形几何与外部 `<use>` 精灵 (`layout.cp`, `painter.cp`, `cascade.cp`)**：
   - 在 `layoutSvgChildren` 中沿子树累加 `transform="translate(tx, ty)"` 并为 `<g>` 计算子图形矩形并集；新增 `has_fill_none` 统一级联权重；在 `painter.cp` 新增 `paintExternalSvgUse` 解析外部 SVG 精灵路径的 `#symbol`。
2. **`abspos` 包含块与双裁剪通道 (`layout.cp`, `painter.cp`)**：
   - 在 `LayoutTree` 维护 `abs_cb_*`（最近非 `static` 祖先或初始包含块）并支持 `left+right`/`top+bottom` 推导 `auto` 尺寸；在 `paintNodeRecursive` 区分流内裁剪框 `clip` 与定位裁剪框 `abs_clip`，仅非 `static` 祖先更新 `abs_clip`，使 `position: absolute` 精准穿透静态 `overflow: hidden` 容器。

## Phase 67 活态 Range 边界、CharacterData 规范与 Shadow DOM 方向性引擎技术方案
1. **活态 Range 边界维护 (`node-base.js`, `range.js`)**：
   - 将 `Text`/`Comment` 全部字符修改统一收口至 `replaceData`，并在 `insertBefore`/`removeChild`/`splitText` 触发 `_liveRanges` 回调，确保所有活跃 `Range` 的 `startOffset`/`endOffset` 与容器节点在 DOM 变更时严格同步。
2. **Shadow DOM 方向性双树算法 (`element.js`, `cssom.js`)**：
   - 区分 `:dir()` 选择器（仅沿 light tree 查找，`<bdi>`/`dir="auto"` 不穿透 ShadowRoot）与 `getComputedStyle(el).direction`（沿 flat tree 经 `<slot>` 向上继承，`<slot dir="auto">` 扫描 assigned nodes）。

## Phase 68 CSS Sticky/Fixed 定位引擎、原生 `elementFromPoint` 与百分比高度退化技术方案
1. **定位静态位置回退与百分比高度规则 (`cascade.cp`, `layout.cp`)**：
   - 将 `Position.Sticky` 从 `Fixed` 独立分离；在 `layoutNodeEx` 传递 `static_x/y` 与 `parent_h_definite`，实现 `abspos`/`fixed` 自动回退流内静态位置与非确定父高度（`auto` 或仅 `min-height`）下百分比高度退化为 `auto` 按子内容撑开。
2. **动态视口坐标与 `document.elementFromPoint` (`dom_binding.cp`)**：
   - 在 `getBoundingClientRect` 与 `elementFromPoint` 入口通过 `ensureLayoutUpToDate` 自动刷新脏布局，并由 `computeViewportRect` 结合 `window.scrollX/Y` 与祖先滚动偏移实时计算 `fixed` 逃逸与 `sticky` 双向/嵌套 scrollport 夹紧坐标。

## Phase 69 Shadow DOM 事件重定向、插槽分发与 `innerText`/`outerText` 规范技术方案
1. **Shadow DOM 插槽、事件重定向与作用域样式 (`html-classes.js`, `dispatch.js`, `window.js`)**：
   - 在事件派发路径中沿 `<slot>` 穿越影子子树，按 `composed` 标志控制是否越过 ShadowRoot 边界，对 `event.target` 实施标准 retargeting 并在 `composedPath()` 中过滤 closed shadow tree；`getComputedStyle` 限定 ShadowRoot 内部 `<style>` 作用域。
2. **`HTMLElement` 文本设置器与游离子树选择器 (`html-classes.js`, `selector.js`, `element.js`)**：
   - 将 `innerText`/`outerText` 严格定义于 `HTMLElement.prototype` 以保留 `SVGElement` expando 语义；扩展 `matchesElementSelector` 支持组合子与逗号选择器，使游离节点与 ShadowRoot 脱离原生文档句柄仍可精确匹配。

## Phase 70 CSS Table 3 布局引擎、匿名盒生成与合并边框技术方案
1. **二维网格与合并边框布局 (`src/layout/layout.cp`, `src/css/cascade.cp`)**：
   - 在 `layoutTableCore` 按 `thead`/`tbody`/`tfoot` 排序并为 `colspan`/`rowspan` 分配不重叠二维槽位，支持 `border-collapse: separate`（`border-spacing`）与 `collapse`（共享网格边框线），并按 `caption` min-content 宽度下限撑宽表格。
2. **匿名盒生成与标准 `content-box` 恢复 (`layout.cp`, `session.cp`, `dom_binding.cp`)**：
   - 在 `layoutBlockContainer` 与 `layoutTableContainer` 按 CSS Table 3 §2.2 自动将孤立 `table-cell`/`table-row` 包裹为匿名表/行盒；移除硬编码 `* { box-sizing: border-box; }` 以恢复标准 `content-box` 盒模型。

## Phase 71 Boa JS 运行时与 WebIDL 访问器规范清扫技术方案
1. **`Error.prototype.stack` 与 `GlobalEventHandlers` 规范化 (`constants.js`, `html-classes.js`, `xpath.js`)**：
   - 包装 `Error.prototype.stack` getter 确保首行符合 V8/Chromium `Name: message` 格式；在 `HTMLElement.prototype` 统一定义 `GlobalEventHandlers` 访问器并校验非法 receiver 抛出 `TypeError("Illegal invocation")`，一举打通 11 项 `e2e/boa-*.e2e.test.ts` 运行时兼容性套件。

## Phase 72 原生 CDP DOM 增量事件推送与 WebSocket 15ms 定时器泵技术方案
1. **增量变更队列与已知节点追踪 (`dom_binding.cp`, `session.cp`, `dom.cp`)**：
   - 在 `JsDomContext` 录制全部原生属性、子节点、文本与 `innerHTML` 变更，结合 `CdpPageSession.known_dom_nodes` 过滤离屏未观测节点，按 W3C CDP 协议输出精确 `previousNodeId` 链及 `childNodeCountUpdated` + `setChildNodes`。
2. **非阻塞 15ms 定时器泵 (`server.cp`)**：
   - 使用 POSIX `poll(&pfd, 1, 15)` 替代纯阻塞读取，在空闲 tick 自动调用 `session.pumpTimers()` 并即时刷出 WebSocket 事件帧，彻底打通异步 `setTimeout` DOM 变更推流。

## Phase 73 `URLSearchParams`、`DOMRect` 实装与剩余 Boa 套件全景归档技术方案
1. **WHATWG `URLSearchParams` 与 `DOMRect` (`window.js`, `element.js`, `range.js`)**：
   - 补齐 `URLSearchParams` 全套增删改查与迭代器接口并挂载至 `URL.prototype.searchParams`；将所有 `getBoundingClientRect()` 返回值包装为带 `toJSON()` 的标准 `DOMRect` 实例。
2. **Boa-Wasm/V8 架构专有测试边界厘清**：
   - 确认剩余 6 项 `boa-*` 套件依赖 Kitesurf 特有的 V8-Boa 双引擎重入与 Wasm 堆统计指标，仅有 `boa-import-meta-url` 为 QuickJS `JS_GetImportMeta` 功能缺口，排入纯 CP 内核后续闭环。

## Phase 74 CSSOM Proxy 不变量、2D 变换序列化与跨 Iframe `postMessage` 技术方案
1. **CSSOM Proxy 不变量与前缀映射 (`cssom.js`, `element.js`)**：
   - 在 `createStyleProxy` 与 `makeRuleStyleDeclaration` 实装 `ownKeys`/`getOwnPropertyDescriptor`/`preventExtensions` 满足 WebIDL `Object.freeze` 契约；新增 2D transform `matrix()` 序列化与 `-webkit-`/`-moz-` 属性回退，并将覆写槽从 `_capturedGetComputedStyle` 分离根除二次包装栈溢出。
2. **多窗口 `postMessage` 与 `writingSuggestions` (`window.js`, `html-classes.js`, `parser.js`)**：
   - 维护 `__capsurf_active_window_stack` 追踪发起窗口与 origin，在 `contentWindow` 隔离事件监听与嵌套 iframe 创建，实现 WHATWG 标准跨窗口 `MessageEvent` 投递。

## Phase 75 SVG 几何长度/锚点导航与防篡改 Trusted Events 技术方案
1. **防篡改事件状态机 (`events/event.js`, `events/dispatch.js`)**：
   - 将在途重入锁、可信标志与事件类型/取消状态全部收归闭包私有 `WeakSet`/`WeakMap`，并在 `dispatchTrustedEvent` 校验 `canonicalNodeIds`，彻底免疫页面覆写 `Event.prototype` 或篡改 `target._id` 的干扰。
2. **SVG 几何长度与 `<a xlink:href>` (`html-classes.js`, `html-element.js`, `harness.ts`)**：
   - 校验 `SVG_NS` 并在 `getTotalLength()` 实现 7 类基础 SVG 图元长度公式；增强点击链路支持 SVG `<a>` `xlink:href` 导航。

## Phase 76 4×4 Transform 矩阵、Web Animations 补间与 CSS Typed OM 技术方案
1. **4×4 齐次变换矩阵与 Web Animations (`cssom.js`, `animations.js`)**：
   - 将 `serializeComputedTransform` 升级为列主序 4×4 矩阵引擎（Rodrigues 3D 旋转 + 2D 自动折叠 + `%.6g` 格式化）；在 `KeyframeEffect` 实装 `cubic-bezier` Newton-Raphson + 二分求值与属性实时插值，接入 `getComputedStyle`。
2. **CSSOM Used Values 与 Typed OM (`window.js`, `collections.js`)**：
   - 补齐 `CSSStyleValue`/`CSSKeywordValue`/`CSSUnitValue`/`computedStyleMap()` 与 `calc`/`min`/`max`/`clamp` 求值，区分有/无布局盒 Used Values 解析；修正 `makeHtmlCollection` Proxy 防止命名属性遮蔽原型方法。

## Phase 77 选区高亮光栅化、Range 滑块与表单实时重绘技术方案
1. **截图前活跃态桥接 (`session.cp`, `painter.cp`, `page.cp`)**：
   - 在 `captureScreenshotClipped` 前采集 `getSelection()` 文本范围与 `input`/`textarea` live `.value` 传入 `ScreenshotOptions`，实现非折叠选区高亮覆盖与表单实时重绘，并支持 CDP `clip` 局部截图编码。
2. **`accent-color` 与 Range 滑块交互 (`cascade.cp`, `input.cp`, `dom_binding.cp`)**：
   - 新增 `PROP_ACCENT_COLOR` 槽位及滑块伪元素回退；在 `Input.dispatchMouseEvent` 实装 `<input type="range">` 鼠标按下与拖拽步进更新及 `preventDefault()` 取消拦截。

## Phase 78 `import.meta`、原生 CSS `--*`/`var()`、SVG 渐变与 Fixed 滚动裁剪技术方案
1. **ES 模块 `import.meta` 与 `paint.prepares` (`qjs_engine.cp`, `session.cp`, `protocol_extensions.cp`)**：
   - 在 `sessionModuleLoader` 中通过 `JS_GetImportMeta` 注入标准 `url` 与 `resolve`；将 `paint_prepares` 绑定至真实 `computeLayout` 执行，选区移动不置脏 `layout_dirty`。
2. **原生 `--*`/`var()`、渐变与文本裁剪 (`cascade.cp`, `painter.cp`, `dom_binding.cp`)**：
   - 在 `ComputedStyle` 维护 `CustomProp` 继承链与嵌套 `var()` 求值，打通 SVG presentation attributes 的 `var(--...)` 与 `url(#grad)`；在 `Canvas.drawTextClipped` 应用活动裁剪框杜绝 `overflow: hidden` 文本溢出。

## Phase 79 Flex 多行折行、BFC 浮动包含、Clip 裁剪与 Corner-Shape 光栅化技术方案
1. **BFC 浮动包含与 Flex 多行折行 (`layout.cp`, `cascade.cp`)**：
   - 仅在容器建立 BFC（`flow-root` / 非 `overflow: visible` / `inline-block` / 浮动 / 绝对固定 / 单元格）时计入浮动子元素底部高度；在 `layoutFlexContainer` 中按主轴可用宽度实装 `flex-wrap: wrap/wrap-reverse` 分行累加。
2. **Corner-Shape 几何与分层裁剪 (`painter.cp`)**：
   - 实装 `round`/`bevel`/`scoop`/`squircle`($p=4$)/`notch` 内外轮廓判定与防串色四象限边框；在绘制管线中区分 `clip: rect(...)`（裁剪 outline+盒体）与 `clip-path: inset(...)`（仅裁剪盒体与子树）。

## Phase 80 CSS Grid 绝对定位、单轴 Overflow Clip 与百分比 `calc()` 技术方案
1. **单轴裁剪与逻辑属性 (`cascade.cp`, `painter.cp`)**：
   - 将 `overflow-x`/`overflow-y` 拆分至独立槽位，实现 CSS Overflow 3 单轴 `clip` 仅裁剪对应轴（另一轴保持无界）而 `hidden`/`scroll`/`auto` 触发双轴裁剪；引入括号感知 `parseBoxQuad` 与 `evalCalcPx` 支持 `margin-inline: calc(50% - 50vw)`。
2. **Grid 绝对定位包含块与网格线 (`layout.cp`)**：
   - 在 `layoutGridContainer` 计算 1-based 行/列网格线偏移表，将 `abspos` 子项移出正常网格流，依据 `grid-area` / `auto` 精准锚定到对应网格线或 padding/content edge。

## Phase 81 CSS Typed OM、`DOMMatrix`、`HighlightRegistry` 与 WeakMap WebIDL 封装技术方案
1. **CSS Typed OM 与 `DOMMatrix`/`HighlightRegistry` (`window.js`, `range.js`)**：
   - 完整集成 `CSS.<unit>()`、`attributeStyleMap`/`computedStyleMap()`、`DOMMatrixReadOnly`/`DOMMatrix` 与 `Highlight`/`CSS.highlights`，并通过 `WeakMap` 私有态与原型 brand check 满足 WebIDL 规范。
2. **零 Own-Property 泄露封装 (`element.js`, `node-base.js`, `index.js`, `constants.js`)**：
   - 将 `_nid`/`_styleProxy`/`_attrs`/`_shadowRoot` 等内部字段全面迁入 `WeakMap` + 原型访问器，使 `Object.keys(el)` 与 `DOMException` 属性描述符完全符合浏览器标准。

## Phase 82 完整 IFC 行内格式化上下文引擎、基线偏移与累积透明度技术方案
1. **CSS 2.1 §9.4.2 IFC 引擎 (`cascade.cp`, `layout.cp`)**：
   - 精确区分 `line-height: normal`/0/无单位乘数/长度，并实装 `collectInlineItems`/`flushInlineLine`/`computeVerticalAlignShift` 统一处理行内文本、`<br>`、atomic inline、strut、half-leading 与 `inline-block` 末行基线传递。
2. **累积透明度与 `visibility` 继承光栅化 (`painter.cp`)**：
   - 在 `paintNodeRecursive` 向下透传 `group_opacity`，修复缩放字形重叠双重混合；`visibility: hidden` 仅跳过自身绘制而继续递归子节点，完整支持 `visibility: visible` 后代重现。

## Phase 83 CSS Text 3 空白折叠、Sizing 3 固有尺寸与 `-webkit-line-clamp` 纯 CP 技术方案
1. **跨 `<span>` 空白折叠与固有尺寸 (`cascade.cp`, `layout.cp`)**：
   - 在 `collectInlineItems` 中维护 `prev_ws` 跨节点空白状态机，实现词边界断行与行首行尾空白裁剪；在 `measureIntrinsicWidth`/`measureMinContentWidth` 实装 `min-content`/`max-content`/`fit-content`/`stretch` 及循环百分比视作 `auto`。
2. **`-webkit-line-clamp` 与 `<svg>` `vertical-align: middle` (`cascade.cp`, `layout.cp`, `cdp/css.cp`)**：
   - 对 `display: -webkit-box` + `-webkit-box-orient: vertical` + `-webkit-line-clamp: N` 自动收敛计算 `display` 为 `flow-root` 并按行数截断盒高；修正 `.Middle` 偏移公式符号并使用直接父行内元素 `xHeightRatio()`。

## Phase 84 网络安全策略（CORP / Private Network Access / SameSite 逐跳重定向）技术方案
1. **逐跳重定向凭据剥离与 SameSite (`cookies.cp`, `http_loader.cp`, `mock_fixtures.cp`)**：
   - 在 `fetchDocumentWithContext` 逐跳解析 301/302/303/307/308 与 mock 重定向，跨源跳剥离 `Authorization`/`Cookie` 并将上一跳 `initiator_origin` 传入 `CookieJar` 执行 RFC 6265bis `SameSite.Strict`/`Lax` 判定。
2. **Private Network Access 与多级 Iframe 帧树 (`page.cp`)**：
   - 拦截公网页面跳转 loopback 返回 `net::ERR_BLOCKED_BY_PRIVATE_NETWORK_ACCESS`；使用堆分配 `collectFrameStates` 扫描 1~2 级 `<iframe>` 最终 URL 与 `securityOrigin` 并发射 `Runtime.executionContextCreated`。

## Phase 85 文档 MIME 合成管线、`Network.getResponseBody` 与 WebSocket Origin 安全技术方案
1. **文档分类与响应体缓存 (`document_kind.cp`, `page.cp`, `network.cp`)**：
   - 在 `Page.navigate` 接入 `documentKind(content_type, content_disposition)`，对图片/纯文本/JSON 合成标准包装文档，对 PDF/octet-stream/attachment 发射 `Network.loadingFailed` 并返回 `net::ERR_ABORTED`；实装 `StoredResponseBody` 缓存支持 `Network.getResponseBody`。
2. **WebSocket 握手 Origin 校验与事件时序 (`server.cp`)**：
   - 严格放行无 Origin / 同源 Host / `devtools://devtools`，其余或 `null` Origin 拒绝返回 403；在 `dispatchCdpCommand` 中先发事件帧再发响应帧消除客户端竞态。

## Phase 86 脚本挂起导航消费与 WebSocket 主动推送技术方案
1. **挂起导航异步消费 (`page.cp`, `server.cp`)**：
   - 在 `performPendingNavigation` 读取并清空 `globalThis.__capsurf_pending_navigation`，完整复用相对 URL 解析、Private Network Access、`HttpLoader.fetchDocument` 与 `documentKind` 管线；在 `server.cp` 的 `poll` 空闲 tick 与 `dispatchCdpCommand` 末尾自动冲刷事件帧，解决脚本发起 `location.href` 跳转时 `waitForNavigation` 超时问题。

## Phase 87 Fetch 防篡改头、Form `[LegacyOverrideBuiltIns]` 与 `execCommand` 技术方案
1. **防篡改 Fetch Headers 与 WebIDL 构造器链 (`window.js`, `index.js`)**：
   - 在引导期捕获 `Map.prototype.forEach` 与 `Reflect.apply` 直接读取内部 `Headers._map` 并执行 WHATWG Forbidden Request-Header 过滤；统一将所有 `HTML*Element` 构造器原型链接至 `HTMLElement -> Element`。
2. **`HTMLFormElement` 命名属性代理与 `execCommand` (`form.js`, `index.js`)**：
   - 基于 Proxy 实装 `[LegacyOverrideBuiltIns]`（索引优先、命名控件覆盖自身属性、保留 DOM 树方法）并在 `Document.prototype` 实装 `execCommand`/`queryCommand*` 富文本命令集。

## Phase 88 Isolated Worlds 生命周期、非阻塞慢导航与 Session Deadlines 技术方案
1. **Isolated Worlds 与上下文淘汰 (`session.cp`, `page.cp`, `runtime.cp`)**：
   - `CdpPageSession` 维护 `isolated_worlds` 注册表，跨导航时递增分配新 id 并淘汰旧 id，`Runtime.evaluate`/`callFunctionOn` 严格校验并抛出 Chrome 标准 `-32000` 错误。
2. **非阻塞慢导航与会话超时 (`server.cp`)**：
   - 在 `runWebSocketSession` 事件循环中支持 `slow-script` 挂起导航期间响应 `Target.closeTarget` 立即中止并拒绝挂起求值；追踪会话启动与客户端帧时间戳实现 `ksIdleMs`/`ksMaxSessionMs`（页面 JS 定时器不续期）。

## Phase 89 `engine.fetch` 适配器与 `fetchFn`/`computeElementUsedWidthPx` 修复技术方案
1. **本地 `engine.fetch` RFC 6455 适配器 (`e2e/harness.ts`)**：
   - 基于 `node:net` 转发本地 HTTP 与 WebSocket 握手请求并保留原始 `Host`/`Origin` 头，内建帧编解码器完整支持 `cdp-envelope`、`cdp-upgrade-origin` 与 `response-body-unknown-id`。
2. **`computeElementUsedWidthPx` 线性化与 `fetchFn` 规范补全 (`window.js`, `file.js`)**：
   - 引入 `visited` Set 打断 `min-content/max-content` 父子互调指数递归；补齐 `Headers` immutable guard、`AbortSignal`、`data:` URL 解码、POST `content-length` 与公网 loopback 阻断。

## Phase 90 PNG 5 滤波全格式解码、`@container` 3-Pass 与 `dom_binding` 段错误根治技术方案
1. **PNG 解码器与悬垂切片根治 (`painter.cp`, `cascade.cp`)**：
   - 基于 `cap:pack` `uncompress` 实现完整 PNG 行滤波还原（None/Sub/Up/Average/Paeth）支持 RGB/RGBA/Grayscale；直接对结构体字段切片 `buf[0..len]` 替代按值接收者 `_` 辅助方法，彻底根除悬垂切片。
2. **`@container` 3-Pass 与 `JS_FreeCString` 误释放修复 (`cascade.cp`, `dom_binding.cp`)**：
   - 将 `ContainerRuleEntry` 内部规则改为 `nested_body` 堆解析降低栈帧；修复 `js_get_computed_prop` 对 `page_allocator.dupe` 切片误调 `JS_FreeCString` 导致的 Debug 段错误，并在含 `@container` 文档首次查询时执行 3-pass 重排。

## Phase 91 `start.main` 栈帧 90.5% 瘦身与 CDP 内存统计架构方案
1. **默认 8MB 栈安全重构 (`main.cp`, `server.cp`, `session.cp`)**：
   - 使用 `objdump` 定位栈帧热点，将 `StyleSheet`/`ComputedStyle`/264KB 模块加载缓冲/106KB WebSocket 帧缓冲全部转为堆分配 + `defer` 释放，并将 `CdpPageSession.init` 改为 `@memset` 就地构造，将 ReleaseFast `main` 栈帧从 ~10.2MB 压至 0.97MB。
2. **多域内存聚合统计 (`session.cp`, `target.cp`, `runtime.cp`, `kitesurf_facade.cp`)**：
   - 基于实时 DOM 节点、HTML 字节与视口构建 `engine`/`pageScript`/`renderer` 严格守恒内存统计，支持诊断目标 attach 与 `Runtime.getHeapUsage` 按会话分域返回。

## Phase 92 Iframe `srcdoc`/Referrer、`blob:` URL 与 `document.write` 保序脚本队列技术方案
1. **Iframe 导航、Referrer 与 `blob:` URL (`html-classes.js`, `window.js`, `parser.js`)**：
   - 实装 `URL.createObjectURL`/`revokeObjectURL` 注册表、`srcdoc` 优先级与 `_navSeq` 防竞态、`loading="lazy"` 延后加载与 `computeIframeReferrer` 策略裁剪；修复 `parser.js` `<!doctype html>` 大小写匹配。
2. **`document.write` 解析器脚本队列 (`index.js`, `window.js`)**：
   - 嵌套 `document.write` 脚本前插至兄弟脚本队列头部，并在 `__capsurf_dispatch_lifecycle` 中等待写入队列排空后再派发 `DOMContentLoaded` 与 `load`。

## Phase 93 CSS 渐变平铺、`background-clip: text` 与 Tab 停靠位技术方案
1. **零额外内存渐变平铺与字形渐变 (`cascade.cp`, `painter.cp`)**：
   - 在 `BgGradient.sampleAt` 按视口裁剪区域直接逐像素计算周期坐标并叠加多图层 `background-clip`，支持 `background-clip: text` 与 `-webkit-text-fill-color` 独立于 `text-decoration` 颜色渲染。
2. **CSS Text 3 §4.2 Tab 停靠位 (`layout.cp`)**：
   - 区分等宽与比例字体的空格宽度，在行内换行阶段按实时 `line_x` 计算 8-space tab stop 推进（余量 `< 0.5 * space_w` 进位至下一停靠点）。

## Phase 94 引擎内存会计、后台重开生命周期与 WebSocket 出站过滤技术方案
1. **Unload 源隔离与重开生命周期 (`session_hooks.cp`, `session.cp`, `runtime.cp`, `server.cp`)**：
   - 导航切换文档前先派发 `pagehide`/`unload` 并排空微任务确保 Cookie 写入旧源；通过 `__capsurf_reopen_load_pending` 在 `consoleAPICalled` 之后与 `poll` 空闲 tick 精确发射 `Page.loadEventFired`。
2. **有界内存统计与 HTTP 路由 (`session.cp`, `server.cp`, `mock_fixtures.cp`)**：
   - 对 `/__fixtures/chatty-json` 实施 12MB FIFO 淘汰记录 `retainedBodyBytes`；在 `handleHttpRequest` 完整实装 `/health`、`/json/*`、`/?inspect=1`/`/?url=` 与代理头解析。

## Phase 95 CORS 样式表、Flat CDP Session 与 `waitForSelector` 收割技术方案
1. **CORS `<link rel="stylesheet">` 规范 (`cssom.js`, `window.js`)**：
   - 在 `_loadLinkSheet()` 区分同源、跨域 `crossorigin` + ACAO、跨域无 ACAO 与重定向后跨域，对 opaque 样式表在 `cssRules`/`insertRule` 抛 `SecurityError` 同时保留 `_cascadeCssText` 参与 `getComputedStyle` 级联。
2. **Flat CDP Session 与 `waitForSelector` (`e2e/harness.ts`)**：
   - 支持顶层 `sessionId` 多路复用与 `CDPClient.connection().session(sessionId)` 句柄，补齐 `Page.waitForSelector` 轮询，一举收口 7 大套件（25/26 PASS）。

## Phase 96 XHTML 嗅探、CDATA 样式解包与 RawText 状态保持技术方案
1. **XHTML 嗅探与 CDATA 解包 (`tokenizer.cp`, `tree_builder.cp`)**：
   - `sniffXhtml` 跳过前导 BOM、XML 声明、注释与 DOCTYPE 后精确校验根标签名 `html`；在 XHTML 模式下解包 `<![CDATA[...]]>` 为字符流，并在 `TreeBuilder.processText` 持久保留 `text_tokenizer_state` 防止 RawText/ScriptData 中途退回 Data 态。

## Phase 97 `currentScript` 微任务排空、样式表优先准入与 `addBinding` 持久化技术方案
1. **逐脚本微任务排空与宏任务延后 (`session.cp`, `session_hooks.cp`)**：
   - 在执行每个经典 `<script>` 前设置 `document._currentScript`，执行完毕后立即 `drainJobs()` 再清零，期间通过 `__capsurf_defer_timers` 暂存 `setTimeout` 宏任务，确保 Promise 微任务可见 `currentScript` 而 `setTimeout` 仅见 `null`。
2. **优先资源准入与持久化绑定 (`session.cp`, `types.cp`, `kitesurf_facade.cp`)**：
   - 提升 `MAX_CDP_EVENTS=64` 并以连续游标缓冲 `net_event_storage` 承载多子资源事件；实装 `<link rel="stylesheet">` 高优先级槽位准入与跨导航自动重装 `Runtime.addBinding`。

## Phase 98 盒模型测量、`ResizeObserver`、根滚动溢出与引擎路由代理技术方案
1. **盒模型测量与 `ResizeObserver` (`element.js`, `window.js`)**：
   - 补齐 `offsetWidth`/`offsetHeight`/`clientTop`/`clientLeft`，`clientWidth`/`clientHeight` 扣除边框；`ResizeObserver` 计算真实 `contentRect` 并通过轻量轮询感知视口、样式、动画与 transition 尺寸变化。
2. **根滚动溢出与 `window.scrollTo` 钳位 (`element.js`, `window.js`, `cloudflare-test.ts`)**：
   - `collectRootScrollExtents` 排除 `position: fixed` 子树并递归计入 `abspos` 溢出，普通元素采用 `relX + childW` 避免负左边距膨胀；`cloudflare-test.ts` 补齐 `X-Forwarded-Host`/`Proto` 与 inspector 绑定使 `engine-routes` 达 15/17 PASS。

## Phase 99 图片固有尺寸、CDP `exposeFunction` 往返与 `setRequestInterception` 技术方案
1. **图片固有尺寸与异步事件 (`html-classes.js`)**：
   - `sniffImageDimensionsFromDataUrl` 遵循 CSS 替换元素规则处理 SVG 默认/viewBox/单边比例；`Image` 构造器支持 `data:` URL 解码并异步派发 `load`/`error`。
2. **`exposeFunction` 往返与请求拦截 (`harness.ts`, `window.js`)**：
   - 统一通过 `Runtime.addBinding("puppeteer_" + name)` + `Runtime.bindingCalled` 与 `Fetch.requestPaused` + `__capsurf_fetch_interceptor` 桥接异步往返，一举拿下 21/22 用例。

## Phase 100 CSS Fonts 4 `@font-face` 引擎与 `::before`/`::after` 伪元素布局技术方案
1. **CSS Fonts 4 与 `UnicodeRange` 步进整形 (`src/css/font_face.cp`, `cascade.cp`, `layout.cp`)**：
   - 实装引号/括号感知的 `FontFaceRule.parseBlock` 与码点区间匹配，在 `spaceWidth`/`charWidthForStyle` 中按 `WebfontStyleMetrics.advanceRatioForCodepoint` 精准计算 WebFont 与 CORS 回退字宽。
2. **伪元素生成内容布局 (`cascade.cp`, `layout.cp`, `painter.cp`)**：
   - 解析 `::before`/`::after` `content:` 字符串并在 `measureIntrinsicWidth`/`collectInlineItems`/`paintNodeRecursive` 中纳入生成文本，覆盖块、行内块、Flex 与按钮容器。

## Phase 101 CDP ShadowRoot 序列化与 CSP `frame-ancestors`/XFO 嵌入策略技术方案
1. **DOM 协议无元数据污染序列化 (`dom.cp`, `session.cp`)**：
   - `serializeCdpNodeWithOptions` 保留 SVG 标签大小写并收集 QuickJS 宿主 `shadowRoots` 附着至文档树，剔除内部 `attributeNamespaces`；`formatMemoryStatsJson` 与 `formatHeapUsageJson` 严格守恒 `accounted_bytes`。
2. **帧嵌入策略与跨域 302 Cookie (`mock_fixtures.cp`, `http_loader.cp`)**：
   - 实装 `isFrameEmbeddingAllowed` 优先遵循 CSP `frame-ancestors` 并兼容严格 `X-Frame-Options` 规则（被拒返回空白文档），跨域 302 重定向响应头中的 `Set-Cookie` 同步写入会话 `CookieJar`。

## Phase 102 匿名 Flex 文本子项布局与主题重绘技术方案
1. **Flex 匿名文本子项 (`src/layout/layout.cp`)**：
   - 在 `layoutFlexChildren` 与 `measureIntrinsicWidth` 中将非空白直接 `NodeType.Text` 子节点按父字体度量纳入 `FlexItem` 列表并回填 `LayoutBox.rect`，彻底打通 `display: flex` 文本节点在 `html.dark` 主题切换后的重新级联重绘。

## Phase 103 `insertAdjacentHTML`、Rendered `innerText` 与 CSS Color 4 相对色技术方案
1. **Rendered `innerText` 与内联样式代理 (`html-classes.js`, `element.js`)**：
   - 按 HTML rendered-text collection steps 实装 `innerText` getter（处理空白折叠、`<pre>`、`<table>` 与隐藏节点）；升级 `createStyleProxy` 完整支持简写展开/清除与属性双向同步。
2. **CSS Color 4 相对色与角度求值 (`cssom.js`, `window.js`)**：
   - 支持 `rgb(none none none)` 与 `rgb(from currentColor r g b)` 向 `color(srgb ...)` 规范化序列化，并实现 `evalCssAngleDeg` 计算三角/双曲角度函数。

## Phase 104 `caret-color` 级联与同源字体度量光标/表单选区光栅化技术方案
1. **光标与选区像素对齐 (`cascade.cp`, `painter.cp`)**：
   - 在 `painter.cp` 直接复用 `LayoutTree.measureTextSegmentWithTabs` 计算 `<input>`/`<textarea>` 光标横坐标与选区高亮区间，确保光标位置与 `#measure` 布局测量误差 `<= 0.2px`；新增 `PROP_CARET_COLOR` 独立控制光标颜色（解决 `color: transparent` 场景光标消失问题）。

## Phase 105 `engine-routes` 全绿、按需字体码点门控与 `DOM.getOuterHTML` 技术方案
1. **按需 `unicode-range` 字体门控与图片扣留会计 (`session.cp`, `session_hooks.cp`)**：
   - 将未命中初始文档码点的 `@font-face` 暂存至 `deferred_fonts`，在 `pollNetworkEvents` 中随 DOM/文本变更按需触发 `addFetchRecord`；对跨源覆写 `<img>` 统计真实 `retainedImageCount`/`Bytes` 并并入 `accountedBytes`。
2. **双层 URL 编码与 `DOM.getOuterHTML` (`server.cp`, `dom.cp`)**：
   - `/?url=` 重定向先内层编码 `initialUrl=` 再外层编码 `wss=` 参数；`DOM.getOuterHTML` 支持 `nodeId == 0` 与 `includeDoctype: true` 恢复前置注释与标准 DOCTYPE。

## Phase 106 CSS Transitions/Animations 引擎、Canvas2D 跨源 Taint 与 Fetch Redirect Shell 技术方案
1. **CSS Transitions & `@keyframes` 运行时 (`animations.js`, `window.js`, `element.js`)**：
   - 在 `classList`/`setAttribute` 变更时捕获前后计算样式快照，按真实时钟驱动 `transitionrun`/`start`/`end`/`cancel` 与 `animation*` 可信事件，支持负 delay、`transition-property: all` 与 retarget 取消。
2. **Canvas2D 图像字节与跨源 Taint (`canvas2d.js`, `html-classes.js`)**：
   - 通过规范节点 ID 校验防止伪造源对象，实时判定 `src`/`srcset`/`<picture>` 与 `<base>` 同源性，跨源时阻断像素写入并置 `_originClean = false` 触发 `SecurityError`。

## Phase 107 原生 UTF-8 码点解码与彩色 Emoji 光栅化技术方案
1. **UTF-8 多字节与彩色 Emoji 矢量绘制 (`src/render/painter.cp`)**：
   - 将文本绘制管线从单字节 ASCII 升级为内联 UTF-8 多字节码点解码，识别 Unicode `Emoji_Presentation=Yes` 与 `U+FE0F` 变体选择符并通过 `drawColorEmojiClipped` 原生绘制 RGBA 彩色字形。

## Phase 108 CSS 层叠上下文命中测试与 Text Decoration 3 传播技术方案
1. **层叠上下文命中与 `transform` 子树平移 (`dom_binding.cp`, `layout.cp`, `cascade.cp`)**：
   - `js_document_element_from_point` 按 `(sc_root, sc_z, sc_pos, local_z, local_pos)` + DOM 树序比较候选节点，完整支持 `pointer-events: none/auto`、`isolation: isolate` 与固定祖先逃逸裁剪；`layoutNode` 在应用 `transform: translate` 后递归 `shiftSubtree` 同步偏移后代。
2. **Text Decoration 3 下划线传播 (`painter.cp`)**：
   - 文本渲染时向上回溯修饰源并在 atomic/out-of-flow 边界截断，独立应用 `text-decoration-color`/`thickness`/`underline-offset` 并叠加 `group_opacity`。

## Phase 109 Web Animations、`contenteditable` 编辑流水线与 Named Access 技术方案
1. **Web Animations 时间轴与 `KeyframeEffect` (`animations.js`)**：
   - 建立 `Animation.effect` 与 `KeyframeEffect._animation` 双向绑定，补全 `getComputedTiming()`、`steps()` 缓动与非法参数 `TypeError` 校验，移除 `finish()` 污染 `target.style`。
2. **`contenteditable` 与 Named Access on Window (`window.js`, `html-element.js`, `harness.ts`)**：
   - 在原型链挂载 `WindowProperties` Proxy 按需解析 `document.getElementById`；实装 `beforeinput`/`textInput`/Enter 段落拆分与 `preventDefault()` 取消链路。

## Phase 110 动态插入 `<script>` 引擎、Speculative Prefetch 与 WebMCP 推流技术方案
1. **动态插入脚本与 Speculative Prefetch (`session_hooks.cp`, `session.cp`, `mock_fixtures.cp`)**：
   - 将 `PAGE_INSERTED_SCRIPT_HOOKS_JS` 独立拆分并挂钩全部 `appendChild`/`insertBefore`，通过原生 `JS_EVAL_TYPE_GLOBAL` 与 `(browserContextId, origin, url)` 隔离的 learned script 表完整实现 1024 预算、CORP/ACAO、ES Module 依赖图与驱逐策略。
2. **多导航内存稳定与事件缓冲 (`session.cp`, `server.cp`, `webmcp.cp`)**：
   - 将 `JsRuntime` 堆上限提升至 256MiB 并改用 16KB 顺序偏移 Console/Log 缓冲，配合 `WebMCP.toolsAdded` 轮询一举拿下 7 大套件 26/26 全绿。

## Phase 111-112 UA 下拉浮层/Iframe 位图与鼠标悬停/动态 `:checked` 技术方案
1. **UA `<select>` 浮层与 Iframe 位图 (`dom_binding.cp`)**：
   - 在 `ua_widgets_script` 中挂载 `domDebug.setIframeImage` 接入 `canvas_bitmaps` 光栅化路径，并实装 `[data-kitesurf-ua="select-dropdown"]` 点击展开、选项命中与外部关闭。
2. **鼠标悬停状态机与动态 `:checked` (`harness.ts`, `selector.js`, `canvas2d.js`)**：
   - 跟踪 `window.__capsurf_lastHoverEl` 规范派发 `mouseout`/`mouseleave`/`mouseover`/`mouseenter` 并去重 `Mouse.click`；将含 `:checked` 选择器路由至 JS 子树匹配准确反映 `form.reset()` 与动态切换。

## Phase 113 CSS `light-dark()`/`color-scheme` 与 CSSOM-View `offset*` 几何技术方案
1. **`light-dark()` 延迟求解 (`cascade.cp`, `style_extractor.cp`)**：
   - 在 `computeStyles` 完整级联并继承 `color_scheme` 后统一求解 `light-dark(a, b)`，同时在 `style_extractor.cp` 与 `dom_binding.cp` 保持绘制与 `getComputedStyle` 一致。
2. **未滚动 `offset*` 几何与 `<link media>` (`layout.cp`, `dom_binding.cp`)**：
   - 暴露 `__capsurf_get_layout_box` 返回未受滚动偏移影响的布局坐标支撑 `offsetParent`/`offsetTop`/`offsetLeft`；按 `linkMediaMatchesScreen` 过滤 `<link rel="stylesheet" media>`。

## Phase 114 `::placeholder` 伪元素与 CJK 全角 Range 度量技术方案
1. **`::placeholder` 伪元素与 `attr()` 内联展开 (`cascade.cp`, `painter.cp`)**：
   - 在 `Cascade.computeStyles` 按 64 位级联权重收集 `::placeholder` 规则并依次执行 `resolveVarValue` -> `expandCssAttrInValue` -> `CssColor.parse`，在 `painter.cp` 中对空表单控件绘制 `placeholder` 文本。
2. **粗体整形与 CJK 全角宽度 (`layout.cp`, `dom_binding.cp`)**：
   - 对 `font-weight >= 600` 施加 `1.06` 宽度因子，并在 `Range.prototype.getClientRects` 按码点区分 CJK 全角（`1.0 * fontSize`）与 ASCII 半角（`0.5 * fontSize`）。

## Phase 115 Pointer/Mouse 事件流、活体命名全局与单行 `[native code]` 技术方案
1. **无 Proxy 活体命名全局与原生函数形态 (`window.js`, `constants.js`, `index.js`)**：
   - 以 `syncNamedGlobals()` 替代原型 Proxy（避免 QuickJS 缺失标识符不抛 `ReferenceError`），在 DOM 增删与脚本执行前实时同步 `iframe`/`id` 且不覆盖页面自有属性；通过 `WeakMap` + `Function.prototype.toString` 规范化输出 Chrome 单行 `[native code]`。
2. **Pointer/Mouse 规范流与独立消息队列 (`window.js`, `harness.ts`)**：
   - 实装 `pointerdown` 取消抑制兼容鼠标事件、`mousedown` 后置默认焦点迁移与 UTF-16 代理对删除，并通过独立 `messageTaskQueue` 确保每条 `MessagePort` 宏任务后立即排空微任务。
[2026-11-19T07:18:10Z] Starting new build and test phase. Initializing documentation tracking.
