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

3. 端到端全链路打通：200 行构建 CapSurf 首个可编译原型 (capsurf_demo.cp)现在，我们已经把 dom.cp（Arena DOM 树） + matcher.cp（Servo 选择器匹配引擎） + cascade.cp（64位权重样式级联） + layout.cp（盒模型与 Flex 坐标计算） 四大核心模块全部对齐了你们仓库里的真实类型。 你可以立刻在 /data/dev/bun/src/ 下新建一个测试入口，把整条“HTML DOM 构建 -> 解析 CSS 样式表 -> 运行 querySelectorAll -> 计算 getBoundingClientRect 屏幕像素坐标”的全链路跑通：
4.

一、 要实现完整的生产级 CapSurf，我们还需要做什么？（核心研发清单）我们已经有了 网络栈 (cap:http/ws) + 事件循环 (cap:event_loop) + CSS 解析与选择器 (src/css) + 基础 Arena DOM 与级联布局原型 (dom.cp/cascade.cp/layout.cp)。要让它真正变成能替代 Puppeteer / Playwright / Kitesurf 的工业级无头浏览器，还需要补齐以下 5 个模块： 模块 1：HTML5 状态机解析器（HTML String -> Arena DOM Tree）
刚才的 capsurf_demo.cp 是手工调用 doc.createElement 建树的；真实场景下，需要把 fetch 拿到的 HTML 字节流自动解析成 Document.nodes 数组（处理未闭合标签、<script> 阻塞执行、innerHTML 动态解析）。模块 2：JS 引擎绑定与 Web API 垫片（DOM Shims，即 Kitesurf 那 67% 的 TS 代码）
将 QuickJS-NG 嵌入后，把 window、document、Element、EventTarget、MutationObserver、fetch、localStorage、History 等对象暴露给网页 JS。这部分不需要从零写，直接用你们的 AI 管线把 Kitesurf 现成的 67% TypeScript 垫片代码批量翻译成 CP 代码即可。 模块 3：CDP 协议服务端（Chrome DevTools Protocol over WebSocket）
基于 CP 已有的 cap:ws 和 cap:json，把 Kitesurf 中负责處理 CDP 指令（如 Page.navigate、Runtime.evaluate、DOM.querySelector、Input.dispatchMouseEvent、Page.captureScreenshot）的 TS 路由逻辑迁入 CP，让外部的 Playwright / Puppeteer / AI Agent 框架能零修改直接连上来。 模块 4：AI Agent 专属结构提取器（Accessibility Tree / Markdown 蒸馏）
90% 的大模型 Agent 并不直接看原始 HTML，而是看 A11y Tree（无障碍树） 或带交互编号（如 [1] <button "提交"> (225, 20)）的精简快照。基于我们的 layout.cp（过滤掉 display: none 和 visibility: hidden 的节点），只需写 300 行 CP 代码就能直接吐出供 LLM 消费的视觉坐标 + 语义树。模块 5（按需可选）：2D 软件光栅化截图器（Layout Rects -> PNG）
当 Agent 调用 page.screenshot() 时，将 layout.cp 算出的 Rect 盒子、背景色、边框和文字绘制成 PNG 字节流。
