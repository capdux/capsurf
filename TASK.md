请先完整读取当前目录下的 GEMINI.md 架构规范，并检查系统中的 `cap:css` 标准库导出接口（确认 SelectorList、StyleSheet、Property、Display、Size、LengthPercentage 的导入路径与字段）。
确认完毕后，请立即按 GEMINI.md 中的 Milestone 1 开始工作：

1. 编写 src/dom/dom.cp（基于 u32 NodeId 的 Arena DOM 树）；
2. 编写 src/css/matcher.cp、src/css/cascade.cp 与 src/layout/layout.cp（对接 cap:css）；
3. 编写 tests/test_css_layout.cp 并调用 `cpc check` 与 `cap test` 完成自检闭环，直到全部测试通过。

要拼出完整的 CapSurf（支持 DOM + JS + 精确坐标布局 + 截图），你们只需要在现有 CP 资产上补齐极其清晰的 3 个轻量级模块：

1. 补齐一个 500 行的“样式级联器（Style Cascade）”

为什么简单：Bun 的 src/css 是打包器，所以它解析完 rules 和 declaration 后就调用 printer 打印输出了。

你要做的：写一个约 500 行的 CP 模块，遍历 DOM 树，用现成的 selectors/ 匹配节点，按 selectors 算出的权重（Specificity）和继承规则（如 color、font-size 向子节点继承），把 properties/ 里的属性合并成每个 DOM 节点最终的 ComputedStyle 结构体。

2. 迁入轻量布局算法 Taffy（约 1.2 万行纯算法代码 -> CP）

有了每个节点的 ComputedStyle（宽高、Margin、Padding、Flex/Grid 属性），把它们喂给 Flexbox/Grid 布局算法，就能算出每个元素在屏幕上的精确 (x, y, width, height) 像素框（即 getBoundingClientRect()）。

Rust 的 Taffy 库没有任何系统 I/O 依赖，纯粹是数学与树遍历计算。以你们迁 53 万行 Bun 的经验，用 AI 把 Taffy 翻译成纯 CP 代码只需几天时间。

3. 补齐有状态 HTML5 Tree Builder（约 8,000 行）

检查一下你们 /data/dev/bun/src/ 下是否有 html 相关的目录（或者 lol-html 绑定）。由于 Bun 自带的是流式扫描器，只需用 AI 将 Lexbor 的 html/tokenizer 和 html/tree（或者 Kitesurf 的 TS DOM 树层）迁入 CP，就能构建出完整的内存 DOM 树。

三、 架构收益总结：为什么这让 CapSurf 稳操胜券？

```Plaintext
┌────────────────────────────────────────────────────────────────────────────┐
│                   CapSurf 100% 纯血 CP 原生架构拼图                         │
├──────────────────────┬───────────────────────────┬─────────────────────────┤
│ 模块                 │ 代码来源                  │ 当前状态                │
├──────────────────────┼───────────────────────────┼─────────────────────────┤
│ 网络 / HTTP1,2,3 / WS│ cap:http + cap:ws (自 Bun)│ ✅ 100% 已在 CP 实装     │
│ 事件循环 / 定时器    │ cap:event_loop (自 Bun)   │ ✅ 100% 已在 CP 实装     │
│ URL / 路径 / JSON    │ cap:url + cap:json (自Bun)│ ✅ 100% 已在 CP 实装     │
│ CSS3 解析 / 选择器   │ src/css (Lightning+Servo) │ ✅ 100% 已从 Bun 迁入 CP │
│ CDP 协议 + Web APIs  │ Kitesurf (67% TS 代码)    │ ⚡ AI 迁入 CP (约 1~2 周)│
│ HTML5 DOM + 布局计算 │ Lexbor HTML + Taffy 布局  │ ⚡ AI 迁入 CP (约 1~2 周)│
│ JS / Wasm 引擎       │ QuickJS-NG + WAMR         │ 🔌 @c 零成本静态内嵌     │
└──────────────────────┴───────────────────────────┴─────────────────────────┘
```

DOM 树 (dom.cp)：基于连续数组 NodeId (u32) 的 Arena DOM 树，内存立省 40%，零循环引用。

选择器引擎 (matcher.cp + parser.cp + selector.cp)：完整支持 querySelectorAll 与 W3C Selectors Level 4。

样式级联计算 (cascade.cp + style.zig + declaration.zig)：基于 64 位位打包权重，单指令完成 !important 与 Specificity 级联覆盖。

几何盒模型布局 (layout.cp + properties/)：直接算出元素的 (x, y, width, height) 坐标，让 AI Agent 知道每个按钮在哪、是不是可见。

3. 构建 CapSurf 首个可编译测试原型 (tests/test_css_layout.cp)：我们已经把 dom.cp（Arena DOM 树） + matcher.cp（Servo 选择器匹配引擎） + cascade.cp（64位权重样式级联） + layout.cp（盒模型与 Flex 坐标计算） 四大核心模块全部就绪。你可以立刻在 /data/dev/capsurf/tests/ 下新建一个测试入口，把整条“HTML DOM 构建 -> 解析 CSS 样式表 -> 运行 querySelectorAll -> 计算 getBoundingClientRect 屏幕像素坐标”的全链路跑通。
   测试代码跑出来的结果，正是目前所有 AI Agent（如 OpenAI Operator / Claude Computer Use / Browser-Use） 梦寐以求的核心能力：
   不仅能用 querySelectorAll 毫秒级筛选出目标元素；
   还能在不启动龐大的 Chromium GPU 进程、不画一张像素图的情况下，仅用几微秒的纯数学计算，直接告诉 AI Agent：#submit-btn 按钮是可见可点击的，它的精确屏幕坐标在 (x: 225, y: 20, w: 120, h: 44)，而 #secret 被 display: none 隐藏了不可点击！（这已经瞬间超越了无法计算布局坐标的 Lightpanda！）

一、 要实现完整的生产级 CapSurf，我们还需要做什么？（核心研发清单）我们已经有了 网络栈 (cap:http/ws) + 事件循环 (cap:event_loop) + CSS 解析与选择器 (src/css) + 基础 Arena DOM 与级联布局原型 (dom.cp/cascade.cp/layout.cp)。要让它真正变成能替代 Puppeteer / Playwright / Kitesurf 的工业级无头浏览器，还需要补齐以下 5 个模块： 模块 1：HTML5 状态机解析器（HTML String -> Arena DOM Tree）
刚才的 capsurf_demo.cp 是手工调用 doc.createElement 建树的；真实场景下，需要把 fetch 拿到的 HTML 字节流自动解析成 Document.nodes 数组（处理未闭合标签、<script> 阻塞执行、innerHTML 动态解析）。模块 2：JS 引擎绑定与 Web API 垫片（DOM Shims，即 Kitesurf 那 67% 的 TS 代码）
将 QuickJS-NG 嵌入后，把 window、document、Element、EventTarget、MutationObserver、fetch、localStorage、History 等对象暴露给网页 JS。这部分不需要从零写，直接用你们的 AI 管线把 Kitesurf 现成的 67% TypeScript 垫片代码批量翻译成 CP 代码即可。 模块 3：CDP 协议服务端（Chrome DevTools Protocol over WebSocket）
基于 CP 已有的 cap:ws 和 cap:json，把 Kitesurf 中负责處理 CDP 指令（如 Page.navigate、Runtime.evaluate、DOM.querySelector、Input.dispatchMouseEvent、Page.captureScreenshot）的 TS 路由逻辑迁入 CP，让外部的 Playwright / Puppeteer / AI Agent 框架能零修改直接连上来。 模块 4：AI Agent 专属结构提取器（Accessibility Tree / Markdown 蒸馏）
90% 的大模型 Agent 并不直接看原始 HTML，而是看 A11y Tree（无障碍树） 或带交互编号（如 [1] <button "提交"> (225, 20)）的精简快照。基于我们的 layout.cp（过滤掉 display: none 和 visibility: hidden 的节点），只需写 300 行 CP 代码就能直接吐出供 LLM 消费的视觉坐标 + 语义树。模块 5（按需可选）：2D 软件光栅化截图器（Layout Rects -> PNG）
当 Agent 调用 page.screenshot() 时，将 layout.cp 算出的 Rect 盒子、背景色、边框和文字绘制成 PNG 字节流。

## HTML5 解析层（html5ever 与 Lexbor结合，实现最高性能与安全性）

迁移 HTML5 解析层（Tokenizer + TreeBuilder）到原生 CP 代码，以 html5ever 为主蓝本（占比 80%：学习其 TreeSink 架构、枚举模式匹配与树构建算法），以 Lexbor 为辅（占比 20%：仅提取其预先生成的静态查找表与字节流切片技巧）。

### 一、 决定性原因

html5ever 的 TreeSink 架构天生适配我们的 u32 Arena DOM我们在上一轮之所以能把 DOM 树设计成极速、省 40% 内存的 NodeId (u32) 连续数组模型（dom.cp），前提是 HTML 解析器不能越俎代庖去直接操作节点内部的指针。

- 如果从 Lexbor 迁移：
  Lexbor 的 TreeBuilder 代码（如 in_body.c）与它的 C 结构体 lxb_dom_node_t 死死焊在一起。状态机里到处都是 node->parent->last_child = new_node; 这样的裸指针直接赋值。让 AI 一边翻译 HTML5 复杂状态机，一边把几千处 C 指针链表操作改写成 Document.nodes[id] 数组下标操作，AI 极易在复杂的“领养机构算法（Adoption Agency Algorithm）”中改出逻辑 Bug。

- 如果从 html5ever 迁移：

html5ever 在设计之初就规定：TreeBuilder 根本不知道节点长什么样，它只持有一个泛型句柄 Handle（在我们的 CP 代码里，直接定义 type Handle = u32; 即可！）。
当 html5ever 的状态机需要建树或重排节点时，它只调用 TreeSink 接口的十几个方法（如 create_element、append、remove_from_parent）。你只需要把 html5ever 的 tree_builder/mod.rs 原封不动地用 AI 翻译成 CP 的 switch 语句，然后在底层用 100 行代码把这十几个方法对接到 dom.cp 的 nodes 数组上，一次就能跑通，零架构改造风险！

### 二、 语言范式同构

Rust 的 enum + match 就是 CP 的 enum + switchHTML5 规范本质上是一个巨大的双层状态机（词法状态机 + 树插入模式状态机）。

- 我们已经有过成功经验：src/css/selectors/parser.cp（3746 行），原本就是 Mozilla Servo 用 Rust 写的 selectors 库，经由 Zig 迁入 CP。
  你会发现它的 `union(enum)` 和 `switch` 结构在 CP 里极其自然。
- html5ever 也是同一个团队（Mozilla Servo）用完全相同的风格写的！
  在 html5ever 中，Token 定义为清晰的代数数据类型：Rust// html5ever (Rust)

```rust
pub enum Token {
    DoctypeToken(Doctype),
    TagToken(Tag),
    CommentToken(StrTendril),
    CharacterTokens(StrTendril),
    EOFToken,
}
```

用 AI 翻译到 CP 时，完全是 1:1 的机械映射：TypeScript// CapSurf (CP)

```cp
pub enum Token {
    Doctype: Doctype,
    Tag: Tag,
    Comment: []const u8,
    Characters: []const u8,
    Eof,
}
```

而在 TreeBuilder 的 step() 函数中，html5ever 使用极其规整的 `match mode { InsertionMode::InBody => ... }`，你只需要把 match 换成 CP 的 `switch`，把模式分支转换成 CP 语法，转换准确率可以达到 98% 以上。

### 三、 借用安全（Borrow Checker）零冲突

站在 Rust 已经排完雷的肩膀上CP 语言拥有严格的编译期所有权与 NLL 借用检查器（包括我们在第 6.5 节看到的容器迭代期间严禁修改容器的 Loop-Borrow Lock）。

- C 语言的 Lexbor 完全没有所有权概念，它经常在一边遍历 open_elements 栈指针的同时，一边在循环体内直接增删改同一个栈或 DOM 节点。如果直接把 C 代码翻译成 CP，CP 的 sema.rs 借用检查器会报出大量 Borrow conflict 错误，需要人工介入重构控制流。
- 而 html5ever 本身就是用 Rust 写的，它的作者当年已经为了通过 Rust 严苛的 Borrow Checker，把所有会导致别名冲突（Aliasing XOR Mutability）的控制流全部重构成了安全的索引访问或分步操作！因此，从 html5ever 翻译过来的 CP 代码，天生就能丝滑通过 cpc check 的借用安全检查。

通过 html5ever 迁移，对 cp的`所有权和借用检查`的完整性、成熟性有极大帮助。

### 四、 唯一需要避开 html5ever、借鉴 Lexbor 的地方

砍掉 Tendril 和 Rust 宏虽然 html5ever 的主干逻辑完胜，但它有两个 Rust 历史包袱千万不要照搬，这里正是吸收 Lexbor 精华的地方：

#### 1. 用 CP 原生切片 []const u8 彻底替换 html5ever 的 StrTendril：

html5ever 为了支持网络流式分块输入（Chunked Streaming），发明了一个复杂的引用计数字符串类型 Tendril。而在我们的无头浏览器中，cap:http 拿到 HTML 响应体后，内存中已经是一整块连续的 []const u8 缓冲区（只读借用）。
因此，在让 AI 翻译 html5ever 时，直接下达指令：
将所有 StrTendril 全部替换为零拷贝切片 []const u8（仅在遇到 &amp; 等 HTML 实体转义时，才向 Document.allocator 申请一块小内存存放解码后的文本）。这能比原版 html5ever 再快 30%！

#### 2. 用 Lexbor 的静态查找表（或 CP ComptimeEnumMap）替换 html5ever 的编译期宏：

html5ever 的 tree_builder 里用了一点 Rust 的 match_token! 宏来简化标签匹配，且字符实体表（Named Entities）是靠 Cargo build.rs 生成的。
在迁移时：

- 让 AI 直接读取 html5ever 宏展开后（cargo expand） 的纯匹配逻辑，或者直接用 CP 的 switch 正常多分支语法书写；
- 对于标签名（如 "div", "script", "table"）识别，直接使用你们在 parser.cp 里已经用熟的 bun.ComptimeStringMap 或枚举映射，将标签名在 Tokenizer 阶段就转为 1 字节的 HtmlTag 枚举（enum HtmlTag: u8 { Div, Span, Script, ... }），后续 TreeBuilder 全程只比对 1 字节枚举值，速度拉满！

### 五、 实操落地路线：3 步用 AI 拿下 src/html/parser.cp

源码靶点非常集中，你甚至不需要克隆整个 Servo 仓库，只需从 html5ever 仓库（Apache 2.0 / MIT 双协议）中抽出以下 4 个核心文件 喂给AI 迁移管线：

1. tokenizer/states.rs + tokenizer/mod.rs（约 1,500 行 Rust -> 1,000 行 CP）：
   指示 AI 将状态枚举和 step() 循环翻译为 CP，输入改为 input: []const u8 游标，输出直接调用 TreeBuilder 的 process_token()。
2. tree_builder/types.rs + tree_builder/mod.rs（约 3,200 行 Rust -> 2,200 行 CP）：
   指示 AI 将 Handle 固定为 dom.NodeId (u32)，将 self.sink.append(...) 直接内联替换为 this.doc.appendChild(...)。
3. 跑通 WPT html5lib-tests 标准测试集：
   html5ever 仓库自带一套官方 JSON 格式的 html5lib-tests 测试用例（输入一段奇葩畸形的 HTML 字符串，校验生成的 DOM 树结构）。用 CP 的 cap test 跑一遍这套 JSON 测试集，就能确保你们的 cap:html 解析器达到 100% 工业级浏览器标准！

---

### 六、 阶段完成里程碑记录（Milestone Status）

- [x] **Milestone 1: 内存 DOM 原型、选择器引擎、样式级联与几何布局（100% 闭环）**
  - `src/dom/dom.cp`、`src/css/matcher.cp`、`src/css/cascade.cp`、`src/layout/layout.cp`
  - `tests/test_css_layout.cp` (3/3 passed)
- [x] **Milestone 2: HTML5 解析层（Tokenizer + TreeBuilder + Adoption Agency + Foster Parenting）（100% 闭环）**
  - `src/html/tags.cp`、`src/html/entities.cp`、`src/html/tokens.cp`、`src/html/tokenizer.cp`、`src/html/tree_builder.cp`、`src/html/parser.cp`
  - `tests/test_html_tokenizer.cp` (3/3 passed)
  - `tests/test_html_parser.cp` (4/4 passed)
  - 全量回归测试通过（3/3 suites, 10/10 tests passed, 32.3s）
  - 二进制运行 `cap src/main.cp` 闭环验证
- [ ] **Milestone 3: QuickJS-NG 引擎绑定与 DOM API 挂载**
- [ ] **Milestone 4: CDP 协议服务端与双模构建发布**

### 二、 Kitesurf 真实资产规模穿透：上千个文件究竟是什么？

原版 Kitesurf 统计出的 1,238 个 TS/JS 文件中，存在大量脚手架和测试固件。当我们剥离测试后，发现其核心代码极其精炼：
• 测试用例与脚手架（约占 78%，1,000+ 文件，无需迁移）：
• packages/engine/test/：997 个测试文件（包含海量 WPT Web 规范测试矩阵与 Vitest 单元测试桩）。
• bazel/ 与 scripts/：约 30 个 Bazel 构建与补丁脚本。
• 真正需要迁移的生产 TS 核心代码（Production TS/JS）：仅 278 个文件！
• packages/engine/src：99 个核心文件（CDP 协议栈、网络安全过滤、Cookie Jar、WebMCP、AXTree）。
• packages/dom-shim/src：91 个核心文件（完整 W3C Web API 运行时：DOM 树操作、事件系统、MutationObserver、XPath 3.0、HTML 表单、Canvas2D）。
• packages/page-script/src：63 个核心文件（网页注入与执行策略、Scheduler 调度、Fetch/XHR 拦截、反反爬指纹）。
• packages/script-gate/src：11 个核心文件（JS 隔离沙箱与权限门禁）。
• packages/jsrpc-types/src：7 个核心文件（RPC 类型契约定义）。
• packages/worker-script/src：4 个核心文件（Web Worker 线程环境存根）。
• packages/page-renderer/src：3 个核心文件（页面渲染管线桥接）。

### 二、 30 个 Bazel 与 Scripts 脚本是什么？对我们有价值吗？

```text
  深入解构 vendor/kitesurf/bazel/ 与 vendor/kitesurf/scripts/ 后，这批脚本主要分为以下 4 大类：
   类别                          │ 代表文件                             │ 原版用途                              │ 对 CapSurf 的价值评估
  ───────────────────────────────┼──────────────────────────────────────┼───────────────────────────────────────┼───────────────────────────────────────
   1. Bazel 多语言构建脚本       │ bazel/*.bzl, BUILD.bazel             │ Cloudflare                            │ ❌ 无价值（完全废弃）。CapSurf 是纯
                                 │                                      │ 内部单体仓的工程构建规则，用于拼装    │ CP 原生代码，统一由 cap build / cpc
                                 │                                      │ Rust (cargo) + TS (tsc) + WASM (wasm- │ 极速单二进制编译，无需庞大的 Bazel
                                 │                                      │ bindgen)                              │ 规则。
   2. Rust 依赖打补丁脚本        │ scripts/patches/*.patch(20+          │ 原版 Kitesurf 为让外部 Rust           │ ❌ 无价值（完全废弃）。CapSurf
                                 │ 个补丁目录)                          │ 库（blitz, boa_engine, taffy,         │ 底层已由 CP 原生 Arena DOM + QuickJS-
                                 │                                      │ selectors）在 Cloudflare Workers WASM │ NG C-FFI 彻底替代，根本不依赖这些
                                 │                                      │ 限制下勉强运行而打的内存 Hack 补丁    │ Rust crates。
   3. 官方生态测试调度与比对清单 │ scripts/pptr-run.mjsscripts/pw-      │ 调度官方 Puppeteer / Playwright       │ ⭐ 极高参考与复用价值！其中的 pptr-
                                 │ run.mjstests/pptr-                   │ 测试套件，批量运行并比对兼容性通过率  │ expectations.json 记录了哪些 CDP
                                 │ expectations.jsontests/pw-           │                                       │ 命令是自动化必须支持的；我们可以在
                                 │ expectations.json                    │                                       │ Milestone 4 收尾与阶段发布时，用轻量
                                 │                                      │                                       │ TS 脚本驱动它们验证 CapSurf。
   4. 代码规范与开源 DCO 检查    │ check-dco.mjscheck-license-          │ Cloudflare                            │ ❌ 无价值。
                                 │ headers.mjs                          │ 公司内部的代码开源协议合规扫描        │
  ──────
```

## 10. `vendor/kitesurf` 全量文件三级分流与黑名单铁律 (CRITICAL)

为避免子智能体盲目扫描 `vendor/kitesurf` 下上百个无关文件导致 Token 耗尽，对除核心列表外的所有文件严格执行以下三级分流策略：

### 10.1 纳入 `src/js/dom_shim.cp` 内嵌引导 JS 的扩展与 Stub 模块

无需翻译为 CP 原生 C-API，直接精简去类型化后合入内嵌 JS 字符串：

1. **表单与交互控制（必合入，支撑 AI Agent 填表与点击）**：
   - 读取 `dom-shim/src/element/forms/{input.ts,textarea.ts,select.ts,button.ts,form.ts}`、`form-data.ts`、`user-control.ts`，实现表单元素的 `.value`、`.checked`、`.selectedIndex`、`form.submit()` 及模拟真实输入事件流。
   - 读取 `dom-shim/src/element/{html-element.ts,html-classes.ts,resolve.ts,reflect.ts,rect.ts}` 与 `style-declaration.ts`，实现 `HTMLElement` 子类映射、属性反射、`DOMRect` 及 `el.style` 行内样式代理（修改时触发 `__cap.markLayoutDirty()`）。
   - 读取 `dom-shim/src/{parser.ts,traversal.ts}`，提供 `DOMParser`（调 `__cap.parseHtml`）与 `TreeWalker` / `NodeIterator`。
2. **轻量级空壳桩化（Stubs，仅写 5~15 行 JS 防止网页脚本报错）**：
   - `canvas2d.ts` / `element/canvas.ts`：提供 `getContext('2d')` 的轻量 Mock（含 `fillRect`, `measureText`, `toDataURL`），应对前端指纹脚本。
   - `animations.ts` / `transitions.ts`：`Element.prototype.animate` 直接返回已完成的 Promise Stub。
   - `layout/scroll.ts` / `element/dialog.ts` / `element/media.ts` / `range.ts`：维护内存级 `scrollTop`/`scrollLeft` 数值，`video.play()` 返回 `Promise.resolve()`，提供基础 `Range`/`Selection` 空壳。

### 10.2 迁入纯 CP 的辅助协议与网络模块

1. **Playwright / Puppeteer 握手必备域 (`src/cdp/protocol.cp`)**：
   - 参考 `engine/src/cdp/{target.ts,frames.ts,viewport.ts,page-session.ts,network.ts}`，除 `Page`/`DOM`/`Runtime`/`Input`/`Accessibility` 外，必须支持 `Target.setDiscoverTargets`、`Target.getTargets`、`Target.attachToTarget`、`Page.getFrameTree`、`Network.enable`、`Network.setUserAgentOverride`；对 `CSS.enable`、`Log.enable`、`Debugger.enable` 直接返回 `{}` 空成功响应，确保 Playwright 连接握手 100% 顺畅。
2. **Cookie 会话与图片尺寸探测**：
   - 参考 `engine/src/cookies.ts` 实现 `src/net/cookie_jar.cp`（同步 HTTP `Set-Cookie` 与 `document.cookie`）。
   - 参考 `engine/src/eval/image-dims.ts` 实现 `src/render/image_dims.cp`（从图片头部字节直接解析 `naturalWidth`/`naturalHeight` 供布局使用）。
   - 参考 `engine/src/cdp/webmcp.ts` 实现 `src/cdp/webmcp.cp`（将网页表单提取为 AI Agent 工具定义）。

### 10.3 绝对物理黑名单（严禁任何主/子智能体调用 `read_file` 读取！）

`MODULES.md` 和这两个目录的完整文件列表已经把 Kitesurf 的全貌彻底扒开了——**这简直是为我们的 `u32` Arena DOM 量身定做的完美参考库！**

你看 `MODULES.md` 里的一句话：

> _"`registries.ts`: Per-node-id `Map`, `WeakMap`, and `Set` registries shared across modules"_
> _"`mutations/attached.ts`: `isAttachedToDocument(nodeId)` + `safeChildNodeCount(nodeId)`"_

**惊人的架构巧合**：Kitesurf 的 `dom-shim` 底层也是**基于整数 `nodeId`（Node ID）工作的**！它之所以写得这么厚，是因为它在 Cloudflare Workers 里必须隔着一层慢速的 Wasm 边界去调 Rust 的 `blitz-dom`；而在我们的 **CapSurf** 里，QuickJS 和 CP 的 `Document`（`NodeId = u32`）就在同一个原生进程内存空间里！

---

### 一、 精准解剖：哪些文件是“必摘纯金”，哪些直接扔掉？

对照你列出的文件清单，我们可以把 `dom-shim` 和 `engine` 划出一条极其清晰的“黄金切割线”：

| 目录与核心文件                              | 对 CapSurf 的价值 | 最佳落地方式（省 90% 力气） |
| ------------------------------------------- | ----------------- | --------------------------- |
| **1. 用户代理默认样式表（意外之喜！）**<br> |

<br>`engine/src/tty/stylesheet.css`<br>

<br>`engine/src/tty/stylesheet-level2.css`<br>

<br>`engine/src/tty/focus.css` | **白捡的宝贝**：浏览器为什么知道 `<h1>` 字号是 `2em`、`<b>` 是粗体、`<head>` / `<script>` 默认是 `display: none`？全靠浏览器内置的 **UA Stylesheet**！ | **直接拷贝复用**：把这三个 `.css` 文件直接嵌入 `src/css/ua_stylesheet.cp`，在 `cascade.cp` 计算样式时作为第 0 层默认规则喂给 `cap:css`，瞬间解决 HTML 标签默认外观与布局！ |
| **2. DOM 核心包装与事件冒泡（JS 垫片层）**<br>

<br>`dom-shim/src/events/event.ts`<br>

<br>`dom-shim/src/events/dispatch.ts`<br>

<br>`dom-shim/src/node-base.ts`<br>

<br>`dom-shim/src/element/element.ts`<br>

<br>`dom-shim/src/collections.ts`<br>

<br>`dom-shim/src/window.ts`<br>

<br>`dom-shim/src/mutations/observer.ts` | **Web API 灵魂**：实现了完整的 W3C 事件捕获/冒泡（Capturing/Bubbling）、`MutationObserver`、`classList` (`DOMTokenList`)、`HTMLCollection`、`window` / `location` / `history`。 | **融合为内嵌 JS 引导脚本 (`src/js/dom_shim.js`)**：不需要翻译成几万行 CP C-API！把这几个 TS 文件去类型化并精简合并为一个纯 JS 文件，底层通过 `node._nid` (`u32`) 直接调用你在 Milestone 3 写好的 CP 原生函数（`__cap_dom_*`），QuickJS 启动时 1 毫秒内完成预加载！ |
| **3. CDP 协议与 AI Agent 专用域**<br>

<br>`engine/src/cdp/dispatch.ts`<br>

<br>`engine/src/cdp/page.ts`<br>

<br>`engine/src/cdp/dom.ts`<br>

<br>`engine/src/cdp/runtime.ts`<br>

<br>`engine/src/cdp/input.ts`<br>

<br>`engine/src/cdp/accessibility.ts`<br>

<br>`engine/src/cdp/webmcp.ts` | **对外控制核心**：不仅有标准的 `Page` / `DOM` / `Runtime` / `Input` 四大 CDP 域，还有专为 AI Agent 准备的 **`accessibility.ts`（A11y 无障碍树）** 和 **`webmcp.ts`（Web Model Context Protocol 表单工具提取）**！ | **用 `agy` 迁为纯 CP (`src/cdp/*.cp`)**：基于 CP 内置的 `cap:http`、`cap:ws` 和全局 `JSON`，把这 7 个 TS 文件的协议指令分发逻辑翻译成原生 CP 代码。

|
| **4. 冗余包袱（全部跳过不看）**<br>

<br>`otel.ts`, `rbi.ts`, `wasm*.ts`, `oopif-render.ts`, `worker-budget.ts`, `webidl.ts`, `editing/` | Cloudflare 内部遥测（OpenTelemetry）、远程浏览器隔离（RBI）、Wasm 内存分析、富文本编辑器 `execCommand` 补丁。 | **100% 忽略**：严禁让 `agy` 读取或迁移这些文件，立省百万 Tokens！ |

---

### 二、 架构点睛：用“双层架构”把 Milestone 3 升级为完整 DOM + JS 引擎

在 Milestone 3 中，你已经在 `src/js/qjs_engine.cp` 里把 `querySelector`、`getBoundingClientRect` 暴露给了 JS。接下来只需做两步极简扩展，就能把 `dom-shim` 的精华全盘接管：

1. **CP 原生底座层（`src/js/qjs_bridge.cp`）—— 只暴露 15 个基于 `u32 NodeId` 的原子函数**：
   在 QuickJS 的 `globalThis.__cap` 对象上挂载纯整数 ID 驱动的极速 C 函数：

- 树读取：`__cap.getParent(nid): u32`、`__cap.getFirstChild(nid): u32`、`__cap.getNextSibling(nid): u32`、`__cap.getNodeType(nid): u8`、`__cap.getTagName(nid): string`
- 树修改：`__cap.createElement(tag): u32`、`__cap.appendChild(parent_nid, child_nid)`、`__cap.removeChild(parent_nid, child_nid)`、`__cap.setInnerHTML(nid, html_str)`
- 属性与查询：`__cap.getAttr(nid, name)`、`__cap.setAttr(nid, name, val)`、`__cap.querySelectorAll(scope_nid, sel_str): [u32]`、`__cap.getBoundingClientRect(nid): {x, y, width, height}`

2. **JS 垫片层（提取自 `dom-shim` 的 `src/js/bootstrap_dom.cp` 内嵌 JS 字符串）**：
   参考 `dom-shim/src/events/event.ts`、`dispatch.ts`、`node-base.ts` 和 `window.ts`，在 QuickJS 启动时执行一段约 500 行的内嵌引导 JS，在 JS 侧构建标准 `EventTarget -> Node -> Element -> HTMLElement` 原型链（每个实例只存一个 `this._nid = nid`，并用 `Map<u32, Node>` 缓存包装对象）。

- **好处**：网页 JS 调用 `el.addEventListener('click', fn)` 或 `el.dispatchEvent(new MouseEvent('click', { bubbles: true }))` 时，沿着 `__cap.getParent(nid)` 在 QuickJS 内部瞬间完成捕获与冒泡，**无需任何复杂的 C-API 回调引用计数管理，零内存泄漏风险！**

---

### 三、 更新 `GEMINI.md`：追加 Milestone 4、Milestone 5 与 Milestone 6 完整收官蓝图

现在，把下面这段增补内容更新到你的 `/data/dev/capsurf/GEMINI.md` 第 5 节（或者直接替换第 5 节），然后启动 `agy`（使用 **`Flash 3.8 High`**），它就能沿着最省 Token、最精准的路径把 **完整 Web API + UA 默认样式表 + 2D PNG 截图 + CDP/A11y/WebMCP 服务端** 一气呵成写出来：

```bash
cat << 'EOF' >> /data/dev/capsurf/GEMINI.md

---

## 9. 终局收官任务清单（Milestone 4 ~ Milestone 6 实施指令）

> **当前进度确认**：Milestone 1（Arena DOM + `cap:css` 选择器/级联/布局）、Milestone 2（纯 CP HTML5 解析器）、Milestone 3（QuickJS-NG 基础绑定）**均已完工并通过测试**。
> 接下来请严格按以下三个阶段推进，拼出完整的生产级 CapSurf（严禁读取 `vendor/kitesurf` 中的 `otel.ts`、`rbi*`、`wasm*`、`editing/` 等无关文件）：

### Milestone 4：UA 默认样式表 + Kitesurf DOM/Event 完整运行时补齐
1. **内置浏览器默认样式表 (`src/css/ua_stylesheet.cp`)**：
   - 读取 `vendor/kitesurf/packages/engine/src/tty/stylesheet.css`（重点提取 `head, script, style, template, [hidden] { display: none !important; }` 及 `div, p, h1~h6, ul, li, table, button, input` 的默认 `display`/`margin`/`padding` 规则）。
   - 在 `src/css/cascade.cp` 中，将 UA 默认样式表作为最低权重基础层（User-Agent Origin）注入，确保未写 CSS 的 `<script>`/`<head>` 自动隐藏（宽高为 0），块级元素自动换行。
2. **补齐 CP 侧 `u32 NodeId` 原生 DOM 操作桥接 (`src/js/qjs_engine.cp`)**：
   - 在 `src/dom/dom.cp` 中补齐动态修改方法：`removeChild`、`insertBefore`、`setAttribute`、`removeAttribute`、`setTextContent`、`serializeHtml`（支撑 `innerHTML`/`outerHTML`）。
   - 向 QuickJS 全局挂载底层原子桥接对象 `__cap`（全部以 `u32` `NodeId` 交互），并在 DOM 树或属性发生变更时标记 `doc.layout_dirty = true`；当调用 `getBoundingClientRect` 时若 `layout_dirty` 为真，自动重跑 `cascade` + `layout` 实现惰性重排（Lazy Reflow）。
3. **吸收 `dom-shim` 构建完整 W3C 事件流与 Window 环境 (`src/js/dom_shim.cp`)**：
   - 参考 `vendor/kitesurf/packages/dom-shim/src/events/event.ts`、`events/dispatch.ts`、`node-base.ts`、`element/element.ts`、`collections.ts`、`mutations/observer.ts`、`window.ts`。
   - 提炼编写内嵌引导脚本（在 `JSContext` 初始化时自动执行），提供完整的 `EventTarget`、`Event` / `MouseEvent` / `KeyboardEvent` / `InputEvent` / `CustomEvent`（含完整捕获、目标、冒泡三阶段传播）、`classList` (`DOMTokenList`)、`dataset`、`style` 行内样式代理、`MutationObserver`、`localStorage`、`history`、`location` 以及 `<script>` 标签自动顺序执行机制。
   - 编写 `tests/test_dom_events.cp`：验证点击按钮触发父节点事件冒泡、在事件回调中动态修改 DOM 样式与 `classList` 后，再次调用 `getBoundingClientRect()` 坐标实时更新。

### Milestone 5：纯 CP 2D 软件光栅化与 PNG 截图引擎 (`src/render/`)
1. **RGBA 像素画布与几何绘制 (`src/render/painter.cp`)**：
   - 严禁引入重型 C++ Skia 或 Rust Vello！直接用纯 CP 编写轻量级 2D 软件光栅化器 `Canvas { width: u32, height: u32, pixels: [u8] }`（RGBA 4 通道连续数组）。
   - 从 `ComputedStyle` 提取 `background-color`、`color`、`border-color`、`border-width`、`border-radius`（对接 `cap:css` 的颜色与边框属性）。
   - 遍历 `layout.cp` 生成的可见节点 `Rect`（跳过 `visible == false` 或宽高为 0 的节点），实现抗锯齿矩形填充 `fillRect`、圆角矩形 `fillRoundedRect`、四周边框绘制 `strokeBorder` 以及内置 8x16 位图字体/TrueType 文本光栅化 `drawText`。
2. **PNG 图像编码输出 (`src/render/png.cp`)**：
   - 结合 CP 标准库 `cap:pack`（zlib/deflate 压缩）与 CRC32 校验，用不到 120 行纯 CP 代码实现标准 PNG 编码器（写入 PNG Signature、`IHDR`、滤波后的 `IDAT` 压缩像素块、`IEND`），提供 `renderDocumentToPng(doc, viewport_w, viewport_h): [u8]!` 接口。
   - 编写 `tests/test_screenshot.cp`：输入一段包含彩色 Flex 导航栏、卡片、按钮与文字的 HTML+CSS，生成真实 PNG 字节流并校验 PNG 头部签名、尺寸及指定坐标像素颜色值。

### Milestone 6：CDP 协议服务端 + AI Agent 无障碍树 (`src/cdp/`)
1. **迁移 Kitesurf CDP 核心域与 AI 扩展 (`src/cdp/protocol.cp` & `src/cdp/a11y.cp`)**：
   - 参考 `vendor/kitesurf/packages/engine/src/cdp/` 下的 `dispatch.ts`、`page.ts`、`dom.ts`、`runtime.ts`、`input.ts`、`accessibility.ts`。
   - 实现 JSON-RPC 2.0 指令路由器（使用内置一等公民 `JSON`），完整支持：
     - `Page.navigate`、`Page.getLayoutMetrics`、`Page.captureScreenshot`（调用 `renderDocumentToPng` 并转为 Base64 返回）；
     - `Runtime.evaluate`（执行 JS 并返回结果）；
     - `DOM.getDocument`、`DOM.querySelector`、`DOM.querySelectorAll`、`DOM.getBoxModel`；
     - `Input.dispatchMouseEvent`（根据 `(x, y)` 坐标在 `Rect` 数组中做 Hit-Test 命中顶层元素，自动向 QuickJS 派发真实的 `mousedown` -> `mouseup` -> `click` 冒泡事件流）与 `Input.insertText`；
     - `Accessibility.getFullAXTree`（提取可见且可交互的按钮、链接、输入框语义及 `(x, y, w, h)` 坐标，供 AI Agent 极速消费）。
2. **HTTP / WebSocket 守护服务 (`src/cdp/server.cp` & `src/main.cp`)**：
   - 基于 `cap:http` 与 `cap:ws` 实现标准 DevTools 监听端点（`GET /json/version`、`GET /json/list` 及 `ws://.../devtools/page/<id>`）。
   - 编写 `tests/test_cdp_e2e.cp`：模拟 Playwright / AI Agent 发送 CDP 指令序列（`Page.navigate` -> `Runtime.evaluate` -> `Input.dispatchMouseEvent` 点击坐标 -> `Page.captureScreenshot`），完成全链路端到端验证并执行 `cap build -r -f` 与 `cap build -b` 构建验收。
EOF

```

### 四、 给 `agy`（`Flash 3.8 High`）的启动指令

执行完上面那段 `cat << 'EOF' >> /data/dev/capsurf/GEMINI.md` 后，直接在 `agy` 中发送下面这段话即可：

```text
请读取 GEMINI.md 新增的第 9 节（Milestone 4 ~ Milestone 6 终局收官任务清单）。
请按顺序从 Milestone 4 开始推进：
1. 先参考 vendor/kitesurf/packages/engine/src/tty/stylesheet.css 实现 src/css/ua_stylesheet.cp，确保 head/script/style/[hidden] 默认 display: none；
2. 补齐 src/dom/dom.cp 动态修改接口与 src/js/qjs_engine.cp 的 __cap 原生桥接，并参考 vendor/kitesurf/packages/dom-shim/src/{events,node-base.ts,window.ts} 实现完整的事件冒泡与 DOM Shim，跑通 tests/test_dom_events.cp；
3. 随后依次推进 Milestone 5（src/render/painter.cp & png.cp 截图引擎）与 Milestone 6（src/cdp/ 服务端与 A11y 树），每完成一个 Milestone 执行一次 Git 原子提交。

```

追加完这第 10 节后，不要忘了做一次 `git commit` 保存你的 `GEMINI.md` 更新，然后就可以放心地让 `agy` 开工执行 Milestone 4 ~ 6 了！

## 剩余文件

针对 `packages/dom-shim/src` 和 `packages/engine/src` 中剩余的所有文件，为了防止 `agy` 在工作时陷入选择困难或盲目扫描导致 Token 耗尽，我们需要将它们明确划分为“四档处置矩阵”：**T1（核心内嵌 JS）、T2（核心迁 CP）、T3（按需/精简 Stub 桩化）、T4（彻底物理屏蔽）**。

以下是剩余全部文件的逐一归类与精确处置方案：

---

### 一、 `packages/dom-shim/src` 剩余文件全量处置表

`dom-shim` 的本质是在 JS 侧伪造一套符合 W3C 标准的浏览器 DOM API。除了核心节点和事件外，剩余文件按以下三类处理：

#### 1. 纳入 `src/js/dom_shim.cp` 内嵌 JS 的必需模块（精简吸收）

这些文件是现代前端框架（React / Vue / Next.js）和表单交互必调的 API，**不需要用 CP 重写，直接将 TS 剥离类型后合入内嵌引导 JS 即可**：

- **`document.ts` + `index.ts` + `state.ts` + `registries.ts` + `constants.ts**`：
- **处理方式**：保留 `DocumentImpl`、节点类型常量（`ELEMENT_NODE = 1` 等）以及 `Map<u32, Node>` 包装对象缓存池；删掉其中与 Wasm 交互的 `initState`，改为直接调我们的 `__cap.*` 原生桥接函数。

- **`element/html-element.ts` + `element/html-classes.ts` + `element/html-tag.ts` + `element/resolve.ts` + `element/tag-attributes.ts` + `element/reflect.ts**`：
- **处理方式**：合入内嵌 JS。它们负责把 `<input>` 映射为 `HTMLInputElement`、把 `<a>` 映射为 `HTMLAnchorElement`，并实现 HTML 属性与 JS 属性的自动双向反射（如 `el.id` <-> `el.getAttribute('id')`）。

- **`element/forms/*`（共 14 个文件：`input.ts`, `button.ts`, `form.ts`, `select.ts`, `textarea.ts` 等）+ `form-data.ts` + `input-types.ts` + `user-control.ts**`：
- **处理方式**：**重点保留 `input.ts`、`textarea.ts`、`select.ts`、`button.ts`、`form.ts`、`form-data.ts` 和 `user-control.ts**`。因为 AI Agent 自动化操作网页最核心的动作就是**填表（输入账号密码、下拉选择、点击提交）**！`user-control.ts` 里封装了模拟真实用户输入并触发 `input`/`change` 事件的完整逻辑，直接合入内嵌 JS 供 CDP `Input.insertText` 调用。其余冷门表单控件（`meter.ts`, `output.ts`, `fieldset.ts`, `validity.ts`）仅保留空壳类定义。

- **`style-declaration.ts` + `element/rect.ts**`：
- **处理方式**：合入内嵌 JS。`rect.ts` 提供 `DOMRect` 类（包装 `__cap.getBoundingClientRect` 的返回值）；`style-declaration.ts` 提供 `el.style.width = '100px'` 的 `CSSStyleDeclaration` 代理，修改时自动同步更新元素的 `style` 属性字符串并标记 `layout_dirty`。

- **`parser.ts` + `traversal.ts` + `range.ts**`：
- **处理方式**：轻量合入。`parser.ts` 实现 `new DOMParser().parseFromString()`（底层直接调 CP 的 `Document.parseHtml`）；`traversal.ts` 提供 `TreeWalker` 和 `NodeIterator`（很多爬虫与提取脚本会用它遍历文本节点）；`range.ts` 仅保留基础 `Range` / `Selection` 空壳防止前端富文本组件报错。

#### 2. 一行代码“桩化（Stub / Polyfill）”即可的次要模块（省 95% 代码）

这些模块如果完全缺失，部分网页 JS 会报 `ReferenceError` 或 `TypeError` 中断执行；但无头环境下不需要它们的真实视觉效果，**只需在内嵌 JS 里写几行空实现（No-op Stub）**：

- **`canvas2d.ts` + `element/canvas.ts**`：
- 提供 `HTMLCanvasElement.prototype.getContext('2d')`，返回一个包含 `fillRect`, `clearRect`, `fillText`, `measureText: (t) => ({ width: t.length * 8 })`, `toDataURL: () => "data:image/png;base64,"` 的轻量 Mock 对象，专门应付前端指纹检测脚本！

- **`animations.ts` + `transitions.ts**`：
- 提供 `Element.prototype.animate = () => ({ finished: Promise.resolve(), cancel() {}, play() {}, pause() {} })`，让网页动画直接视为瞬间完成。

- **`layout/scroll.ts` + `util/scroll-args.ts` + `element/dialog.ts` + `select-dropdown.ts**`：
- 在 `window` 和 `Element` 上维护简单的 `scrollTop` / `scrollLeft` 数值属性，`scrollTo` / `scrollBy` 直接更新这两个数值；`<dialog>` 的 `showModal()` 直接设置 `open` 属性。

- **`element/aria.ts` + `element/svg.ts` + `element/shadow.ts` + `element/image.ts` + `element/media.ts` + `element/table.ts` + `element/embedded.ts` + `element/load-events.ts**`：
- 仅保留基础属性反射（如 `img.src`, `img.complete = true`），音视频 `<video>`/`<audio>` 的 `play()` 直接返回 `Promise.resolve()`。

- **`xpath.ts`**：
- 若后续 Playwright 需要按 XPath 查找元素再迁入，首期直接保留 `document.evaluate` 简版或延后。

#### 3. 直接借助 CP 标准库或彻底丢弃的 `dom-shim` 文件

- **`text-encoding.ts` + `html-encoding.ts` + `util/strings.ts` + `util/validation.ts**`：UTF-8 编解码与字符串处理直接用 QuickJS 内置或 CP 原生能力。
- **`idl/*` + `webidl.ts` + `css-typed-om*` + `fastpath.ts` + `detached-wrappers.ts` + `record.ts` + `mutations/seq.ts` + `mutations/attached.ts` + `internal-state.ts` + `internals.ts` + `contract.ts` + `trust.ts` + `otel.ts` + `editing/***`：
- **处理方式**：**全部丢弃**！这些全是 Kitesurf 为了跟 Rust Wasm 同步状态写的 op-log 录放器、WeakRef GC 回收器和 WebIDL 校验器，在我们的 CP 原生 Arena 架构下毫无用处。

---

### 二、 `packages/engine/src` 剩余文件全量处置表

`engine` 目录是 Cloudflare Workers 的外层调度器，里面夹杂了大量 Cloudflare 专有业务逻辑，必须严格过滤：

#### 1. 迁入纯 CP 的核心调度与会话模块（并入 `src/cdp/` 与 `src/net/`）

- **`cookies.ts` + `cdp/cookie-jar-registry.ts**`：
- **处理方式**：用 CP 实现一个极简的 `src/net/cookie_jar.cp`（约 100 行），管理当前会话的 `document.cookie` 与 HTTP 请求头中的 `Cookie` / `Set-Cookie` 同步。

- **`cdp/connection.ts` + `cdp/page-session.ts` + `cdp/target.ts` + `cdp/viewport.ts` + `cdp/frames.ts` + `cdp/types.ts**`：
- **处理方式**：并入 `src/cdp/server.cp` 与 `src/cdp/protocol.cp`。负责维护当前打开的 Page 视口宽高（默认 `1280x720`）、Frame ID、Target 信息（回应 Playwright 连接时的 `Target.getTargets` / `Target.attachToTarget` 握手！**这点极其关键，Playwright 连上 CDP 第一件事就是发 `Target.setDiscoverTargets` 和 `Page.getFrameTree**`）。

- **`cdp/network.ts` + `cdp/network-events.ts` + `cdp/fetch.ts**`：
- **处理方式**：在 `src/cdp/protocol.cp` 中实现轻量级 `Network.enable`、`Network.setExtraHTTPHeaders`、`Network.setUserAgentOverride`，当 `cap:http` 发起请求时向 CDP WebSocket 推送 `Network.requestWillBeSent` 和 `Network.loadingFinished` 事件，让 Playwright 的 `page.waitForLoadState('networkidle')` 能正常工作！

- **`cdp/css.ts` + `cdp/log.ts` + `cdp/debugger.ts**`：
- **处理方式**：对 `CSS.enable`、`Log.enable`、`Debugger.enable` 直接返回 `{}` 空成功响应（Ack Stub），确保 DevTools / Playwright 初始化握手不报错。

- **`cdp/webmcp.ts` + `dom-shim/src/model-context.ts**`：
- **处理方式**：**强烈建议保留并迁入 `src/cdp/webmcp.cp**`！这是专为大模型 Agent 自动把网页里的 `<form>` 表单转化为 MCP Tool Call Schema 的神器。

- **`eval/image-dims.ts`**：
- **处理方式**：仅百余行代码，用于从 PNG/JPEG/GIF/WebP 响应头的前几十个字节直接解析出图片的真实宽高（`naturalWidth` / `naturalHeight`），无需解码整张图片就能让 `layout.cp` 算出 `<img>` 标签的正确占位尺寸！可顺手迁为 `src/render/image_dims.cp`。

#### 2. 彻底拉黑屏蔽的 Cloudflare 专有文件（严禁 `agy` 读取）

以下文件 100% 属于 Cloudflare Workers 平台强耦合或远程桌面流媒体逻辑，**直接忽略**：

- **渲染流与 Wasm 桥接**：`cdp/scene-host.ts`, `cdp/scene-stream.ts`, `wasm.ts`, `wasm-modules.ts`, `engine-wasm-probe.ts`, `eval/page-renderer.ts`, `eval/oopif-render.ts`, `eval/font-shaping.ts`, `eval/prefetch-wasm.ts`。
- **Workers 隔离与安全策略**：`rbi.ts`, `rbi-protocol.ts`, `rate-limit.ts`, `request-blocker.ts`, `public-suffix*`, `guards.ts`, `host.ts`, `host-bindings.ts`, `landing.ts`, `embed.ts`, `fixtures-binding.ts`。
- **遥测与打包追踪**：`observability.ts`, `tracing*.ts`, `cdp/diag-spans.ts`, `eval/*-spans.ts`, `eval/*-tail.ts`, `eval/web-bot-auth.ts`, `eval/next-route-chunks.ts`, `eval/learned-scripts.ts`, `eval/speculative-prefetch.ts`, `eval/walk-module-graph.ts`, `eval/script-bundle.ts`。

---

### 三、 一键将“白名单与黑名单路由表”追加进 `GEMINI.md`

为了让 `agy` 对这上百个文件做到心中有数、绝不乱读乱翻耗费 Token，请在终端执行以下命令，将这份精确到文件级的《Kitesurf 模块处置白名单与黑名单》追加到 `/data/dev/capsurf/GEMINI.md` 末尾：

【请按顺序从 Milestone 4 开始推进：】

1. **UA Stylesheet**
   先参考 vendor/kitesurf/packages/engine/src/tty/stylesheet.css
   实现 src/css/ua_stylesheet.cp，确保 head/script/style/[hidden] 默认 display: none；在 `cascade.cp` 计算样式时作为第 0 层默认规则喂给 `cap:css`，瞬间解决 HTML 标签默认外观与布局！

2. **DOM 核心包装与事件冒泡（JS 垫片层）**
   补齐 src/dom/dom.cp
   动态修改接口与 src/js/qjs_engine.cp 的 __cap 原生桥接，并参考 vendor/kitesurf/packages/dom-shim/src/{events,dispatch.ts, node-base.ts,element/element.ts, collections.ts, window.ts, mutations/observer.ts} 实现完整的事件冒泡与 DOM Shim，跑通 tests/test_dom_events.cp；
3. **Web API 灵魂**：`src/js/dom_shim.js`：不需要翻译成几万行 CP C-API！把这几个 TS 文件去类型化并精简合并为一个纯 JS 文件，底层通过 `node._nid` (`u32`) 直接调用你在 Milestone 3 写好的 CP 原生函数（`__cap_dom_*`），QuickJS 启动时 1 毫秒内完成预加载！

4. **CDP 协议与 AI Agent 专用域**：engine/src/cdp/{dispatch.ts, page.ts, dom.ts, runtime.ts, input.ts, accessibility.ts（A11y 无障碍树）, webmcp.ts（Web Model Context Protocol 表单工具提取）}
   基于 CP 内置的 `cap:http`、`cap:ws` 和全局 `JSON`，把这 7 个 TS 文件迁移为 CP 代码。
5. 随后依次推进 Milestone 5（src/render/painter.cp & png.cp 截图引擎）与 Milestone 6（src/cdp/ 服务端与 A11y 树），每完成一个 Milestone 执行一次 Git 原子提交。

---

## 10. `vendor/kitesurf` 全量文件三级分流与黑名单铁律 (CRITICAL)

对除核心列表外的所有文件严格执行以下三级分流策略：

### 10.1 纳入 `src/js/dom_shim.cp` 内嵌引导 JS 的扩展与 Stub 模块

无需翻译为 CP 原生 C-API，直接精简去类型化后合入内嵌 JS 字符串：

1. **表单与交互控制（必合入，支撑 AI Agent 填表与点击）**：
   - 读取 `dom-shim/src/element/forms/{input.ts,textarea.ts,select.ts,button.ts,form.ts}`、`form-data.ts`、`user-control.ts`，实现表单元素的 `.value`、`.checked`、`.selectedIndex`、`form.submit()` 及模拟真实输入事件流。
   - 读取 `dom-shim/src/element/{html-element.ts,html-classes.ts,resolve.ts,reflect.ts,rect.ts}` 与 `style-declaration.ts`，实现 `HTMLElement` 子类映射、属性反射、`DOMRect` 及 `el.style` 行内样式代理（修改时触发 `__cap.markLayoutDirty()`）。
   - 读取 `dom-shim/src/{parser.ts,traversal.ts}`，提供 `DOMParser`（调 `__cap.parseHtml`）与 `TreeWalker` / `NodeIterator`。
2. **轻量级空壳桩化（Stubs，仅写 5~15 行 JS 防止网页脚本报错）**：
   - `canvas2d.ts` / `element/canvas.ts`：提供 `getContext('2d')` 的轻量 Mock（含 `fillRect`, `measureText`, `toDataURL`），应对前端指纹脚本。
   - `animations.ts` / `transitions.ts`：`Element.prototype.animate` 直接返回已完成的 Promise Stub。
   - `layout/scroll.ts` / `element/dialog.ts` / `element/media.ts` / `range.ts`：维护内存级 `scrollTop`/`scrollLeft` 数值，`video.play()` 返回 `Promise.resolve()`，提供基础 `Range`/`Selection` 空壳。

### 10.2 迁入纯 CP 的辅助协议与网络模块

1. `engine/src/` 下：`request-blocker.ts`, `public-suffix*`,
2. **Playwright / Puppeteer 握手必备域 (`src/cdp/protocol.cp`)**：
   - 参考 `engine/src/cdp/{target.ts,frames.ts,viewport.ts,page-session.ts,network.ts}`，除 `Page`/`DOM`/`Runtime`/`Input`/`Accessibility` 外，必须支持 `Target.setDiscoverTargets`、`Target.getTargets`、`Target.attachToTarget`、`Page.getFrameTree`、`Network.enable`、`Network.setUserAgentOverride`；对 `CSS.enable`、`Log.enable`、`Debugger.enable` 直接返回 `{}` 空成功响应，确保 Playwright 连接握手 100% 顺畅。
3. **Cookie 会话与图片尺寸探测**：
   - 参考 `engine/src/cookies.ts` 实现 `src/net/cookie_jar.cp`（同步 HTTP `Set-Cookie` 与 `document.cookie`）。
   - 参考 `engine/src/eval/image-dims.ts` 实现 `src/render/image_dims.cp`（从图片头部字节直接解析 `naturalWidth`/`naturalHeight` 供布局使用）。
   - 参考 `engine/src/cdp/webmcp.ts` 实现 `src/cdp/webmcp.cp`（将网页表单提取为 AI Agent 工具定义）。

### 10.3 绝对物理黑名单（严禁任何主/子智能体调用 `read_file` 读取！）

以下文件属于 Cloudflare Workers 专有平台耦合、Wasm 胶水或遥测代码，**100% 丢弃，严禁读取**：

- `dom-shim/src/` 下：`idl/*`, `webidl.ts`, `css-typed-om*`, `fastpath.ts`, `detached-wrappers.ts`, `record.ts`, `mutations/seq.ts`, `mutations/attached.ts`, `internal-state.ts`, `internals.ts`, `contract.ts`, `trust.ts`, `otel.ts`, `editing/*`。
- `engine/src/` 下：`rbi*`, `wasm*`, `observability.ts`, `tracing*`, `rate-limit.ts`, `request-blocker.ts`, `public-suffix*`, `guards.ts`, `host*.ts`, `landing.ts`, `embed.ts`, `fixtures-binding.ts`, `cdp/scene-*`, `cdp/diag-spans.ts`, `eval/*-spans.ts`, `eval/*-tail.ts`, `eval/oopif-render.ts`, `eval/page-renderer.ts`, `eval/font-shaping.ts`, `eval/web-bot-auth.ts`, `eval/next-route-chunks.ts`, `eval/learned-scripts.ts`, `eval/speculative-prefetch.ts`, `eval/walk-module-graph.ts`, `eval/script-bundle.ts`。
  EOF

```

以下文件属于 Cloudflare Workers 专有平台耦合、Wasm 胶水或遥测代码，**100% 丢弃**：

- `dom-shim/src/` 下：`idl/*`, `webidl.ts`, `css-typed-om*`, `fastpath.ts`, `detached-wrappers.ts`, `record.ts`, `mutations/seq.ts`, `mutations/attached.ts`, `internal-state.ts`, `internals.ts`, `contract.ts`, `trust.ts`, `otel.ts`, `editing/*`。
- `engine/src/` 下：`rbi*`, `wasm*`, `observability.ts`, `tracing*`, `rate-limit.ts`, `guards.ts`, `host*.ts`, `landing.ts`, `embed.ts`, `fixtures-binding.ts`, `cdp/scene-*`, `cdp/diag-spans.ts`, `eval/*-spans.ts`, `eval/*-tail.ts`, `eval/oopif-render.ts`, `eval/page-renderer.ts`, `eval/font-shaping.ts`, `eval/web-bot-auth.ts`, `eval/next-route-chunks.ts`, `eval/learned-scripts.ts`, `eval/speculative-prefetch.ts`, `eval/walk-module-graph.ts`, `eval/script-bundle.ts`。
```

4. 实现超轻量 2D 软件光栅化截图器（src/render/painter.cp）
   既然你明确要求支持截图（Screenshot），且我们已经有了 layout.cp 算出的所有节点精确坐标 Rect { x, y, width, height }，完全不需要引入几百 MB 的 Skia，直接在 CP 里写一个 300~400 行的软件渲染器（Painter） 即可：

画布缓冲区：分配一块 width * height * 4 字节的 RGBA 连续内存数组 pixels: [u8]（如 1280x720 仅占 3.6MB）。

绘制盒模型（按 DOM 树前序 + z-index 遍历）：

从 ComputedStyle 读取 background-color、border-color、border-radius；

编写基础矩形填充（fillRect / fillRoundedRect）与边框绘制函数，把每个可见 NodeId 的 Rect 画到 pixels 上。

文字光栅化：通过 @c 引入单头文件库 stb_truetype.h（仅 1 个 .h 文件，Public Domain 协议），加载一个内置无衬线字体（如 Inter.ttf 或系统字体），将文本节点的字符串渲染到对应坐标上。

编码输出 PNG：利用内置的 cap:pack（zlib 压缩）配合 50 行标准 PNG Chunk 封装代码（或 stb_image_write.h），直接将 pixels 输出为 PNG 字节流并经 Base64 编码后通过 CDP Page.captureScreenshot 返回！
