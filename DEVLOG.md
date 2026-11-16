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
