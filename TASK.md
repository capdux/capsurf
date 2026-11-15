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

