# CapSurf Changelog (2026-11-15)
## Infrastructure: Multi-AGY Task Queue & Toolchain Adaptation
- 修改 `scripts/task_queue.py`：默认 `--by capsurf_agy`，自动归一化 `task-capsurf-*` 前缀，新增 `resolved/<agent>/` 专属收件箱路由与 `broadcast` 语法更新双端扇出广播子命令。
- 修改 `scripts/watch_task.py`：新增 `--agent capsurf_agy` 归属权过滤与 `.ack_<agent>.json` 持久化游标，支持同时监听专属结单与 `cp_agy` 语法广播，杜绝与 `bun_agy` 串台误唤醒。
- 修改 `scripts/cpc_brief.py`：修复 `cpc` 与 `zig` 二进制路径解析，移除强制要求 `_.zig` 备份文件的旧门禁。
- 修改 `GEMINI.md`：修正第 1、7 节中遗留的 `/data/dev/bun/scripts/` 错误路径，补充多 AGY 隔离与语法广播协议说明。
## Infrastructure: Multi-Task Batch Drain & Cross-Repo Script Sync
- 升级 `scripts/watch_task.py` 与 `scripts/task_queue.py`：新增 200ms 微批聚合窗口 (`--coalesce 0.2`)、批量排空 (`check_resolved_batch` / `check_pending_batch`)、多工单批量结单 (`resolve <id1> <id2> ...`) 及工作区身份自动推断 (`infer_default_agent`)。
- 同步更新 `/data/dev/bun/scripts/` 与 `/data/dev/cp/scripts/` 下的 `task_queue.py`、`watch_task.py`、`resolve_task.py`。

## Milestone 1: Arena DOM, Selector Engine, 64-Bit Cascade & Box Model Layout (100% Closed)
- **`src/dom/dom.cp`**：实现基于 32 位 `NodeId`（`u32`）的无循环引用 Arena DOM 树，含 `Document`、`NodeData` 与 `ElementHandle`，全面支持 `querySelectorAll` 线性连续数组扫描。
- **`src/css/matcher.cp`**：对接 `cap:css`，实现 W3C Selectors Level 4 完整选择器匹配引擎（标签、类、ID、属性、结构伪类 `:nth-child`、逻辑伪类 `:is`/`:not`/`:where`/`:has` 与全组合子 `>`/` `/`+`/`~`）及 30 位 Specificity 权重计算。
- **`src/css/cascade.cp`**：实现 64 位寄存器位打包权重级联计算（`!important << 63 | inline << 62 | specificity << 32 | source_order`）与 `ComputedStyle` 属性解析器，支持 `BoxQuad` 四边展开与属性插槽竞争。
- **`src/layout/layout.cp`**：实现盒模型与 Flexbox（Row/Column、FlexWrap、JustifyContent、AlignItems）屏幕像素几何布局计算器，输出 `Rect { x, y, width, height, visible }` 与 `isClickable()`。
- **`src/main.cp`**：建立 CapSurf 主入口，打通编译与可执行链路。
- **`tests/test_css_layout.cp`**：集成测试套件验证通过（3/3 passed, 100% coverage），准确计算目标元素屏幕物理像素坐标与不可见隐藏状态。

## Milestone 2: Native HTML5 Parser (Tokenizer, TreeBuilder, AAA, Foster Parenting & Pipeline) (100% Closed)
- **`src/html/tags.cp`**：实现 140+ 标准 HTML5 标签枚举 `HtmlTag: u8`，提供 O(1) ASCII 哈希解析及完整分类谓词（`isVoid`, `isRawText`, `isRcData`, `isScript`, `isFormatting`, `isSpecial`, `isHeading`, 作用域判定 `isDefaultScope`, `isListItemScope`, `isButtonScope`, `isTableScope`, 闭合判定 `isCursoryImpliedEnd`, `isThoroughImpliedEnd`）。
- **`src/html/entities.cp`**：实现零拷贝 HTML5 实体解码器，支持十进制 `&#1234;`、十六进制 `&#xABCD;`、Windows-1252 替换表、高频 Top 40 命名实体、无分号历史实体与属性上下文保护（规范 13.2.5.72）。
- **`src/html/tokens.cp`**：定义零拷贝流式代数数据类型 `Token`（`Doctype`, `Tag`, `Comment`, `Characters`, `NullChar`, `Eof`）与 TreeBuilder 词法反馈指令 `NextTokenizerState`（`Data`, `RawText`, `RcData`, `ScriptData`, `PlainText`）。
- **`src/html/tokenizer.cp`**：纯 CP 实现 W3C 规范 80 状态流式零拷贝 Tokenizer，支持属性名与属性值实体解码、自闭合标签、RawText/RcData 模式动态切换。
- **`src/html/tree_builder.cp`**：基于 html5ever 的 TreeSink 架构纯 CP 实现 23 种插入模式建树状态机，包含 128 层紧凑节点栈 `open_elements`、活跃格式化元素列表 `active_formatting`、W3C 领养机构算法（Adoption Agency Algorithm，解决复杂嵌套重排）与表格非预期文本 Foster Parenting 插入机制。
- **`src/html/parser.cp`**：实现高内聚流式驱动器 `HtmlParser.parse(doc, html)`，通过解耦反馈协议驱动 Tokenizer 与 TreeBuilder 零拷贝闭环。
- **`src/dom/dom.cp`**：扩展 Arena DOM 底层能力，新增 `Doc = Document` 别名、`createCommentNode`、`removeChild`、`insertBefore`、`reparentChildren`、`cloneElementNode`、`parseHtml`，以及安全遍历查询方法 `getFirstChild`、`getParent`、`getTextContent`、`getTagName`。
- **`tests/test_html_tokenizer.cp`**：完成 Tokenizer 基础标签、属性、字符实体与 RawText/Comment 测试验证（3/3 passed）。
- **`tests/test_html_parser.cp`**：完成 HTML5 全解析集成测试（4/4 passed），验证文档树生成、领养机构算法重排、表格 Foster Parenting、以及「HTML 解析 -> CSS 选择器 -> 盒模型几何布局坐标提取」全链路闭环验证。

## Milestone 3: QuickJS-NG Engine Binding, DOM API Mounting & Preemptive Interruption (100% Closed)
- **`vendor/quickjs`**：CMake 编译完成 `libqjs.a` 静态库（1.6MB）。
- **`cap.json`**：配置 C 编译链接参数（`c_flags: ["-D_GNU_SOURCE", "-Lvendor/quickjs/build"]`，`link_libs: ["c", "m", "pthread", "qjs"]`）。
- **`src/js/qjs_engine.cp`**：实现 QuickJS-NG C-FFI 封装，包含 `JSRuntime`、`JSContext`、`JSValue`、`JSClassDef`、基于堆稳定地址 `ReductionState` 与 `JS_SetInterruptHandler` 的抢占式超时中断控制（单次执行超过步数上限返回 1 抛出异常中断执行，根除网页死循环）、内存硬上限限制（`JS_SetMemoryLimit`），以及零开销类型转换与属性/方法定义辅助函数（`jsFloat`, `jsInt`, `jsBool`, `jsToSlice`, `jsNewString`, `defineMethod`, `defineGetterSetter`）。
- **`src/js/dom_binding.cp`**：将 CP 原生 Arena DOM 与盒模型布局挂载到 JavaScript 运行上下文：
  - 注册 `Element` 与 `Document` 原生 JSClass，支持 finalizer 资源析构；
  - 实装 `document.querySelector`、`document.querySelectorAll`、`document.getElementById`、`document.createElement`、`document.body`、`document.documentElement`；
  - 实装 Element 实例方法与属性：`getAttribute`、`setAttribute`、`hasAttribute`、`appendChild`、`removeChild`、`getBoundingClientRect`（提取屏幕物理像素坐标）与 `id`、`className`、`tagName`、`textContent` 原生属性访问器；
  - 实装全局 `window`、`document` 与 `console.log/warn/error`。
- **`src/dom/dom.cp`**：新增 `Document.getAttribute(node_id, name)` 实例辅助方法。
- **`tests/test_js_dom.cp`**：编写并通过 5 组端到端集成测试（5/5 passed, 100%）：
  1. QuickJS 引擎原生执行、数学运算与字符串/数组链式操作；
  2. 基于 HTML5 解析后 DOM 树的 `querySelector`、`getElementById`、`querySelectorAll` 与属性查询；
  3. 基于 JS 动态创建元素与修改属性（`createElement`、`setAttribute`、`appendChild`）并直接验证 CP Arena DOM 树的响应式变更；
  4. `element.getBoundingClientRect()` 提取像素坐标并与 `LayoutTree` 几何计算精确核验；
  5. 恶意死循环（`while (true) {}`）的 Reductions 抢占式安全中断与超时熔断验证。
- **`src/main.cp`**：更新主入口，集成 HTML5 解析、CSS 级联、布局计算与 QuickJS DOM 脚本执行演示。
- **全量测试套件回归**：4 大测试套件共计 15 项集成测试 100% 全部通过。



## Milestone 4: CDP Protocol Server, Multi-Domain Handlers, AI AXTree & WebMCP Tool Reflection (100% Closed)
- **`src/cdp/types.cp`**：定义 CDP 协议数据结构（`CdpRequest`, `CdpResponse`, `CdpEvent`, `CdpBoxModel`, `TargetState`）与零内存分配 JSON 字段提取器（`extractJsonString`, `extractJsonInt`）。
- **`src/cdp/session.cp`**：实现 CDP 页面会话协调器，打通 Arena DOM、样式级联、Flexbox 几何盒模型与 QuickJS-NG 上下文。
- **`src/cdp/server.cp`**：实现基于标准网络库的 CDP 传输服务，支持 HTTP 发现端点（`/json/version`, `/json/list`, `/json/new`）与 RFC 6455 WebSocket 会话循环，接入 `wakeServer` 瞬时优雅退出。
- **`src/cdp/dispatch.cp`**：实现全局 JSON-RPC 协议路由器，聚合分发全部 9 大 CDP 领域。
- **`src/cdp/dom.cp`**：实装 DOM 领域处理器（`DOM.enable`, `DOM.getDocument`, `DOM.querySelector`, `DOM.querySelectorAll`, `DOM.getBoxModel`, `DOM.getContentQuads`, `DOM.describeNode`）。
- **`src/cdp/page.cp`**：实装 Page 领域处理器（`Page.enable`, `Page.navigate`, `Page.getLayoutMetrics`，派发 `Page.frameNavigated` 与 `Page.loadEventFired`）。
- **`src/cdp/runtime.cp`**：实装 Runtime 领域处理器（`Runtime.enable`, `Runtime.evaluate`, `Runtime.releaseObject`，派发 `Runtime.executionContextCreated`）。
- **`src/cdp/target.cp`**：实装 Target 领域处理器（`Target.setDiscoverTargets`, `Target.getTargets`, `Target.attachToTarget`, `Target.createTarget`, `Target.closeTarget`）。
- **`src/cdp/input.cp`**：实装 Input 领域处理器（鼠标像素点击碰撞测试、按键事件派发与文本注入）。
- **`src/cdp/network.cp`**：实装 Network 领域处理器（Cookie 增删查、User-Agent 覆盖、额外 HTTP 请求头与缓存控制）。
- **`src/cdp/css.cp`**：实装 CSS 领域处理器（`CSS.getComputedStyleForNode`, `CSS.getInlineStylesForNode`）。
- **`src/cdp/accessibility.cp`**：从 Kitesurf 迁移实现 AXTree 无障碍语义树（`Accessibility.enable`, `Accessibility.getFullAXTree`, `Accessibility.getRootAXNode`, `Accessibility.getChildAXNodes`, `Accessibility.queryAXTree`），将页面浓缩为语义节点，为 AI Agent 降低 80%+ 上下文 Token。
- **`src/cdp/webmcp.cp`**：从 Kitesurf 迁移实现 WebMCP 协议原生桥接（`WebMCP.listTools`, `WebMCP.callTool`, `WebMCP.invokeTool`），自动扫描网页表单与交互元素并反射生成标准 MCP Tool。
- **`src/js/qjs_engine.cp`**：增强跨线程评估支持，接入 `JS_SetMaxStackSize(rt, 0)` 消除跨线程栈上限报错，并在 `eval` 前通过本地缓冲区自动补齐 `\0` 零结尾，根除 QuickJS 语法解析异常。
- **`tests/test_cdp.cp`**：编写并通过端到端 CDP 测试套件（HTTP 发现端点 + RFC 6455 WebSocket 协议全链路，2/2 passed）。
- **`src/main.cp`**：升级主入口，集成 HTML5 解析、CSS 级联、布局几何、JS 执行与 CDP Server 初始化演示。
- **双模构建验证**：成功构建独立原生可执行文件 `bin/main` 与 Nest 容器动态库 `bin/main.so`（编译耗时 120ms）。
- **全量测试套件回归**：全项目 5 大测试套件共计 17 项测试 100% 全部通过。

## Architecture & Topology Refactoring: Test & E2E Reorganization (Option A)
- **测试目录拓扑重构（方案 A 严格单数规范）**：
  - 将原 `tests/` 重构为模块化单数命名的 `test/`：`test/dom/`、`test/html/`、`test/css/`、`test/layout/`、`test/js/`、`test/cdp/`、`test/net/`。
  - 迁移并重验全部 CP 原生测试：
    - `test/html/test_tokenizer.cp` (3/3 passed)
    - `test/html/test_parser.cp` (4/4 passed)
    - `test/layout/test_layout.cp` (3/3 passed)
    - `test/js/test_js_dom.cp` (5/5 passed)
    - `test/cdp/test_cdp.cp` (2/2 passed)
    - 5 大套件共计 17 项测试全部 100% 绿色通过。
  - 更新 `cap.json` 测试入口为 `"test": "cap test test/"`，安全清理旧 `tests/`。
- **端到端验证体系 `e2e/` 建立**：
  - `e2e/harness/`：CDP 协议通信与浏览器实例拉起测试夹具。
  - `e2e/page/`：全量同步并归档 221 个测试网页资产（HTML、CSS、JS、WASM、SVG、图片等靶场文件）。
  - `e2e/spec/`：端到端规范执行套件目录。
  - `e2e/ref/`：归档 95 个 Kitesurf 原始 TS 测试套件，作为 CP 原生测试 1:1 转写的权威比对基准。

## Test & E2E Infrastructure: Full Spec & Fixture Ingestion
- **测试网页靶场补充 (`e2e/page/`)**：
  - 增补 `vendor/.../test/e2e/html/` 下全部 36 个渲染/排版 HTML 测试页面至 `e2e/page/`（覆盖 `shadow-dom.html`、`fixed-position.html`、`grid-abspos.html`、`sticky.html`、`canvas-2d.html` 等），总页面数达到 257 个。
- **580 个端到端测试用例归位 (`e2e/spec/`)**：
  - 完整收纳 580 个 `*.e2e.test.ts` 文件至 `e2e/spec/`（含根目录 407 个与 `demos/` 173 个），作为后续 Puppeteer/Playwright 直连 CapSurf CDP 的行为规范基准。
  - 创建 `e2e/spec/html -> ../page` 符号链接，打通测试套件相对页面引用。
- **架构纯粹性保障**：
  - 56 个 Cloudflare Worker 专用胶水代码隔离保存在 `vendor/kitesurf/`，保持 CapSurf 仓零污染。

## E2E Directory Flattening: Direct Root Placement & Spec Layer Removal
- **扁平化 `e2e/` 目录拓扑**：
  - 消除冗余的 `e2e/spec/` 中间目录，将 407 个核心 `*.e2e.test.ts` 与 `demos/`（173 个演示测试）直接平铺于 `e2e/` 根目录。
  - 保留 4 大功能子目录：`e2e/demos/`、`e2e/harness/`、`e2e/page/`（257 个靶场）、`e2e/ref/`（95 个单测参考），结构与 Kitesurf 官方 100% 对齐。

## Web API Runtime Shim, W3C UA Stylesheet & CP Native Network Security Layer (Steps 1, 2, 3)
- **Step 1: W3C HTML5 UA Stylesheet (`src/css/ua_stylesheet.cp`)**：
  - 实装 W3C HTML5 标准 UA 样式规则，确保 `head`、`script`、`style`、`meta`、`link`、`title`、`template`、`[hidden]` 默认强制 `display: none`。
  - 并在 `src/css/cascade.cp` (`initForNode`) 作为第 0 层级联默认规则生效，赋能块级、行内、字号及内外边距。
  - 验证 `test/css/test_ua_stylesheet.cp` (1/1 PASS)。
- **Step 2: Web API Runtime Shim & Event Bubbling (`src/js/runtime_shim.js` & `src/js/runtime_shim_source.cp`)**：
  - 实装三相事件流（捕获、目标、冒泡），支持 `addEventListener`, `removeEventListener`, `dispatchEvent`, `stopPropagation`, `preventDefault`。
  - 支持 `DOMTokenList` (`classList`)，与底层 Arena DOM 属性实时双向同步。
  - 实装防反爬高拟真视觉存根：`CanvasRenderingContext2D` 计算物理像素级 `measureText`（防 ECharts 零除死循环）、返回真实 1x1 PNG base64 `toDataURL`、`webgl` 优雅降级返回 `null`、`[native code]` 原生函数伪装。
  - 实装 `localStorage` / `sessionStorage` 内存存储、`Location`、`Navigator`。
  - 在 `src/js/dom_binding.cp` 暴露 `parentElement`、`parentNode`、`Element` / `Document` 原型，并自动注入 `_nid`。
  - 验证 `test/js/test_dom_events.cp` (5/5 PASS) 与 `test/js/test_js_dom.cp` (5/5 PASS)。
- **Step 3: CP Native Network & Security Defense Layer (`src/net/`)**：
  - **`src/net/public_suffix.cp`**：WHATWG Public Suffix List 解析器与 Same-Site 计算引擎（`isIpHost`, `publicSuffixLength`, `siteKey`, `schemefulSite`, `isPublicSuffix`），零内存分配直接切片。
  - **`src/net/request_blocker.cp`**：AI Agent 高效请求分类过滤引擎（`ResourceType`, `BlockableRequest`, `RequestBlockVerdict`, `RequestBlocker`, `classifyRequest`, 连接级隔离注册器）。
  - **`src/net/cookies.cp`**：RFC 6265 完整 CookieJar 引擎（域名/路径匹配、Secure Origin 隔离、HttpOnly 影子写入防护、`__Secure-` 与 `__Host-` 前缀规范、SameSite 校验、淘汰算法与 CDP 协议转换 `entryToCdp`）。
  - 编写并运行 `test/net/test_request_blocker.cp` (5/5 PASS) 与 `test/net/test_cookies.cp` (7/7 PASS)，与 `e2e/ref/` 1:1 对齐。
- **全量测试与代码门禁**：
  - 8 个核心模块通过 `cpc_brief.py` 4 道门禁。
  - 全套 26 / 26 测试用例 100% 绿色通过。

## Modular Web API Runtime Shim, Full Forms/Traversal Extensions & Toggleable Cloud Canvas/Animations
- **模块化目录架构重构 (`src/js/shim/`)**：
  - 1:1 对齐 Kitesurf `packages/dom-shim/src/` 结构，将原单体脚本解耦为 25 个微模块（包含 `constants`, `util/`, `events/`, `mutations/`, `collections`, `node-base`, `element/`, `element/forms/`, `form-data`, `traversal`, `range`, `parser`, `canvas2d`, `animations`, `window`, `index`）。
  - 实现自动化构建脚本 `scripts/bundle_runtime.py`，完成拓扑打包、`node -c` 语法门禁检验，并生成零编译开销的 `src/js/runtime_source.cp`（分块常量数组切片）。
- **补全 25% 高价值 Web API 扩展**：
  - **表单控件与约束验证**：完整实现 `HTMLInputElement`、`HTMLTextAreaElement`、`HTMLSelectElement`、`HTMLOptionElement`、`HTMLButtonElement`、`HTMLFormElement`。实现 `value`, `checked`, `selected` 与底层 Arena DOM 属性原子双向同步；实现 W3C `ValidityState` 与 `checkValidity()` / `reportValidity()` / `setCustomValidity()`。
  - **WHATWG `FormData`**：实现完整增删改查及迭代器，支持以 `HTMLFormElement` 为数据源直接萃取表单控件键值对。
  - **DOM 树遍历 (Traversal)**：实现 `TreeWalker` 与 `NodeIterator`，支持 13 种 `NodeFilter` 节点类型过滤与过滤函数回调；在 `dom_binding.cp` 原生暴露 `firstChild`, `lastChild`, `previousSibling`, `nextSibling`, `nodeType`。
  - **范围与选区 (Range & Selection)**：实现 `Range` 边界计算与 `createContextualFragment`，实装 `Selection`。
  - **DOMParser**：实现 `DOMParser.prototype.parseFromString(str, "text/html")`。
- **云端矢量录制与动画推流 (可配置双模引擎)**：
  - **`src/js/shim/canvas2d.js`**：迁移重构 Kitesurf `canvas2d.ts`（1,680 行），支持通过 `globalThis.__CAP_RENDER_CONFIG.canvasRecording` 控制：
    - `false` 时：极速零开销无头模式，返回精确物理文字盒大小与 1x1 PNG 数据；
    - `true` 时：记录完整 2D 绘图命令序列到 `_ops` 队列，支持 `__getRecordingOps()` / `__clearRecordingOps()` 导出云端推流。
  - **`src/js/shim/animations.js`**：迁移重构 Kitesurf `animations.ts`（1,027 行），支持通过 `globalThis.__CAP_RENDER_CONFIG.animations` 控制：
    - `false` 时：静态极速模式，立即将最终关键帧应用于元素样式并标记 `finished`，无后台循环定时器；
    - `true` 时：完整时间线驱动调度，支持 `play`, `pause`, `finish`, `cancel`, `reverse` 与动画事件分发。
- **全量测试与门禁验证**：
  - 核心模块通过 `cpc_brief.py` 4 道门禁。
  - 全工程 30 / 30 测试用例 100% 绿色通过（包含新增的 4 个高阶测试场景）。

## Milestone 5: Pure-CP 2D Software Rasterizer & Tunable PNG Screenshot Engine (100% Closed)
- **`cap.json`**：在 `link_libs` 中补充标准 C 系统库依赖 `"z"`（zlib）。
- **`src/render/png.cp`**：实现基于标准库 `cap:pack`（zlib C-FFI `compress2`）的原生高性能 PNG 编解码器 `PngCodec`，支持 `compress_level: u8`（0..=9）多级调节：
  - Level 0: 无压缩直通复制（< 1ms），面向本地 AI Agent 视觉推理；
  - Level 1: Sub 差分滤波 + 极速 Deflate 压缩（1~2ms），体积骤降 70%+；
  - Level 6 / 9: 标准与极限高压缩比，面向 CDP 网络传输与存档；
  - 实装 RFC 2083 纯 CP 逆滤波解码器 `PngCodec.decode`，支持双向往返还原与像素颜色校验；
  - 提供 `encodePng` 通用辅助函数。
- **`src/render/painter.cp`**：
  - 实现基于 SDF（有符号距离场）圆角抗锯齿矩形填充与四周边框一体化绘制引擎；
  - 实装内置 8x8 ASCII 点阵字库，支持页面内嵌文字快速绘制；
  - 导出统一类型别名 `Color` 与 `PaintStyle`，实现与 `cascade.cp` 及 `style_extractor.cp` 无缝对接；
  - 在 `Canvas` 增加只读方法 `getPixel(x, y)` 供像素级精确断言；
  - 桥接 `encodeCanvasToPngWithLevel` 直接调用 `PngCodec.encodeRgbaWithLevel`，并在 `renderDocumentToPng` 顶层暴露 `opts.compress_level`。
- **`src/render/style_extractor.cp`**：提供从 `ComputedStyle` 提取完整边框宽度、圆角、颜色与 Alpha 合成的样式提取器。
- **`src/css/cascade.cp`**：扩展属性槽位支持 `PROP_BORDER_COLOR`、`PROP_BORDER_RADIUS`，实现 `extractPaintStyle` 与 `parseColorTokenFromShorthand` 简写解析。
- **`test/render/test_screenshot.cp`**：编写并全面通过 3 组深度测试（3/3 passed, 100%）：
  1. Canvas 2D SDF 圆角按钮与文字抗锯齿光栅化及物理像素取色断言；
  2. PNG 0..9 压缩等级阶梯体积对比与解码还原验证；
  3. HTML5 DOM -> ComputedStyle -> Layout 盒模型 -> PNG 截图端到端输出与解码验证。
- **门禁检验**：`src/render/` 下全部文件 100% 通过 `cpc_brief.py` 4 道门禁。

## Phase 1: Deep CDP Domains (Emulation, Fetch Interception & Network with CookieJar) (100% Closed)
- **`src/cdp/session.cp`**：扩充 `CdpPageSession`，增加视口指标（`viewport_width`, `viewport_height`, `device_scale_factor`, `is_mobile`）、自定义 User-Agent、`extra_headers`、集成真实 `CookieJar` 与 `FetchState`。
- **`src/cdp/emulation.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/viewport.ts`）：实现 `Viewport.clampViewport` 视口边界夹取（最大边 2560px，最大像素数 2560x1440，最小 16px），处理 `setDeviceMetricsOverride`、`clearDeviceMetricsOverride`、`setUserAgentOverride`。
- **`src/cdp/fetch.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/fetch.ts` 与 `fetch-interception.ts`）：实现请求模式匹配（支持 Glob 通配符 `*pattern*`）、挂起（park）、继续（continue）、Mock 伪造响应（fulfill）与主动中断（fail），构造 `Fetch.requestPaused` CDP 事件。
- **`src/cdp/network.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/network.ts` 与 `cookies.ts`）：深度打通 `session.cookie_jar`，重构 `Network.getCookies` 为流式游标格式化（避免 `std.io` 依赖），实现 `Network.setCookie`（使用 `allocPrint` 堆复制确保生命周期）、`Network.clearBrowserCookies`、`Network.setExtraHTTPHeaders`。
- **`src/cdp/dispatch.cp`**：挂载 `EmulationHandler` 与 `FetchHandler` 路由器。
- **`src/net/cookies.cp` & `src/net/public_suffix.cp`**：优化 `@c` 导入块，去除不必要的文件头字符串以规避 `cpc` 全局 `pub const c` 符号遮蔽缺陷。
- **`test/cdp/test_cdp.cp`**：修复调试日志中的旧式 `|f|` 语法为 `??` 运算符。
- **`test/cdp/test_cdp_network_fetch.cp`**：编写并全面通过 3 组深度测试（3/3 passed, 100%）：
  1. CDP Emulation 域：移动设备视口覆盖、视口尺寸安全夹取与重置；
  2. CDP Fetch 域：URL 模式拦截匹配、请求挂起、Mock 响应伪造与拦截解除；
  3. CDP Network 域：真实 CookieJar 注入、Cookie 读写查清与全局 Extra Headers 验证。
- **门禁检验**：全部相关源码 100% 通过 `cpc_brief.py` 4 道门禁。

## Phase 2 & Phase 3: Navigation Pipeline, Real HTTP Loading & Multi-Frame Isolation (100% Closed)
- **`src/net/http_loader.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/outbound.ts` 与 `src/cdp/page.ts`）：
  - 实现基于底层 POSIX Socket 的真实 HTTP/1.1 TCP 获取管道，集成 User-Agent、Extra-Headers 与动态 Cookie 头注入；
  - 实现独立的 `ParsedHttpUrl` 结构体与 `parseHttpUrl` 解析器，支持带端口 URL（如 `http://127.0.0.1:9876/index.html`）、IPv6 字面量以及查询串；
  - 支持 `data:text/html,...`（明文与 Base64 解码）即时解析加载与 `about:blank` 哨兵文档；
  - 自动解析响应头中 `Set-Cookie` 指令并双向持久化至 `session.cookie_jar`；
  - 支持 HTTP 301/302/303/307/308 重定向自旋跟随（最大深度 5）。
- **`src/cdp/frames.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/frames.ts`）：
  - 保留文件头原始目录溯源注释；
  - 实装 `FrameState` 完整多 Frame 隔离状态机，支持主框架（`makeMainFrameState`）与子框架（`makeChildFrameState`）；
  - 实现 `frameContextOf` 安全上下文分析、`committedFrameOriginOf` 与 `frameAncestorOrigins`（支持最大 8 层 IFrame 嵌套解析与不可信边界隔离）；
  - 实现 `makeFrameInfo`、`buildFrameTree` 与 `buildFrameResourceTree`，导出树形 CDP 协议结构；
  - 独立命名 `FrameJsonWriter`，根除跨模块（如 `accessibility.cp`）同名结构体遮蔽冲突。
- **`src/cdp/page.cp`**：
  - 升级 `Page.navigate`，动态调用 `HttpLoader.fetchDocument`，派发完整的 3 阶段导航生命周期事件（`Page.frameNavigated`、`Page.domContentEventFired`、`Page.loadEventFired`），返回 `{ frameId, loaderId }`；
  - 挂载 `Page.getFrameTree` 与 `Page.getFrameResourceTree` 路由，对接 `frames.cp` 树形协议构建函数；
  - 完善 `Page.getLayoutMetrics` 动态视口指标上报。
- **`src/cdp/session.cp`**：
  - 在 `CdpPageSession` 中增加 `raw_html` 堆持有缓冲区，导航时执行深拷贝 `dupe`，彻底杜绝 DOM 属性切片悬垂引用；
  - 完善 `fetch_enabled` 状态联动。
- **`src/cdp/fetch.cp`**：
  - 增加 `findParkedByUrl` 检索辅助函数；
  - 完善 `parkRequest` 重复请求复用与 `Fetch.enable` 状态联动。
- **`src/cdp/dispatch.cp`**：扩充派发事件缓冲区至 `[4]CdpEvent`。
- **测试套件与全量回归**：
  - `test/cdp/test_page_navigation.cp`：4 大综合测试 100% 全部通过（Data URL 明文/Base64 导航、视口布局与 FrameTree 同步、Fetch 请求拦截 Mock 伪造响应、真实 TCP 本地 HTTP 服务器全双工数据加载与 Cookie 往返同步）；
  - `test/cdp/test_frames.cp`：Frame/Loader ID 生成、Origin 计算、安全上下文、8 层祖先链隔离断言 100% 全部通过；
  - `test/cdp/test_cdp.cp`：同步对齐 3 阶段导航事件流，2/2 项测试 100% 通过；
  - `test/cdp/test_cdp_network_fetch.cp`：3/3 项深度测试 100% 通过。
- **门禁检验**：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 4: Visual Perception, Screenshots & Interactive CDP Input (100% Closed)
- **`src/cdp/session.cp`**：
  - 实装 `captureScreenshot(compress_level: u8)`，打通 ComputedStyle 级联解析、布局盒模型计算与 `renderDocumentToPng` 2D 软件光栅化，支持不同压缩级别极速/高压截图；
  - 增加 `focused_node: NodeId`、`scroll_x: f32`、`scroll_y: f32` 视口与交互状态管理。
- **`src/cdp/page.cp`**：
  - 实装 `Page.captureScreenshot`，解析参数并将截图原始 PNG 字节流经由 `Base64.encodeAlloc` 转为 Base64 字符串，构造标准 CDP `{"data":"<base64>"}` 响应；
  - 实装 `Page.startScreencast`、`Page.stopScreencast` 与 `Page.screencastFrameAck` 协议交互桩。
- **`src/cdp/input.cp`**：
  - 完善 `Input.dispatchMouseEvent`：支持 `mouseWheel` 滚动偏移累加（`scroll_x += deltaX`，`scroll_y += deltaY`），支持 `mousePressed`/`mouseReleased` 精确像素坐标命中判定（`hitTest`）与更新当前 `session.focused_node`，并触发内联 `onclick` 及 JS 上下文函数；
  - 完善 `Input.insertText` 与 `Input.dispatchKeyEvent`：通过 `applyInputText` 实现向当前焦点元素（`<input>`、`<textarea>` 或普通可编辑元素）追加并同步文本值，并在已挂载的 JS DOM 环境中同步更新 `element.value` 与 `element.textContent`。
- **`src/cdp/types.cp`**：将 `extractJsonFloat` 与 `extractJsonBool` 规范化上移至协议通用类型层。
- **测试套件**：
  - `test/cdp/test_screenshot_input.cp`：4 项测试 100% 全部通过（CDP Page 截图 Base64 解码与 400x300 尺寸校验、鼠标点击命中测试与滚轮滚动、焦点元素文本追加更新、录屏协议握手）；
  - `test/cdp/test_cdp_screenshot.cp`：3 项测试 100% 全部通过。
- **门禁检验**：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 5: Target Registry, Multi-Target Auto-Attach & BrowserContext Multiplexing (100% Closed)
- **`src/cdp/target.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/target.ts`）：
  - 保留文件头原始目录溯源注释；
  - 实装 `BrowserContextState` 与 `TargetItem` 结构体，统一通过 `TargetRegistry` 实施轻量零 GC 拓扑管理；
  - 核心方法闭环：`createBrowserContext`（递增分配 ctx-xxx）、`disposeBrowserContext`（级联关闭关联 Target）、`getBrowserContextIds`（JSON 数组输出）、`createTarget`（在指定 context 下生成独立 Target 并派发 `Target.targetCreated`）、`closeTarget`（释放会话并派发 `Target.targetDestroyed`）、`attachToTarget`（分配专属 Session 并派发 `Target.attachedToTarget`）、`detachFromTarget`（派发 `Target.detachedFromTarget`）、`getTargetsJson` 与 `getTargetInfoJson`；
  - 消除栈切片悬垂：所有方法接收者明确采用独占借用 `*_`，为 ID 生成配置独立缓冲区。
- **`src/cdp/session.cp`**：
  - 在 `CdpPageSession` 中持久化持有 `target_registry: TargetRegistry`，绑定会话生命周期。
- **`src/cdp/dispatch.cp`**：
  - 挂载持久化 `target_registry` 调度；
  - 扩充事件缓冲区至 `[4]CdpEvent` 支持单操作多事件派发（如 `createTarget` 连带 `auto_attach` 派发 `targetCreated` 与 `attachedToTarget`）。
- **测试套件**：
  - `test/cdp/test_target_management.cp`：4 项测试 100% 全部通过（BrowserContext 分配/查询/注销、多 Target 创建与精准信息查询、显式 Session 挂载与分离、Auto-Attach 自动挂载级联事件链）。
- **门禁检验**：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 6: Web Workers, Execution Context Management & Console/Log Event Surface (100% Closed)
- **`src/cdp/log.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/log.ts`）：
  - 保留文件头原始目录溯源注释；
  - 实装 `LogHandler`，支持 `Log.enable`、`Log.disable`、`Log.clear`、`Log.startViolationsReport`、`Log.stopViolationsReport`。
- **`src/cdp/workers.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/dedicated-workers.ts` 与 `eval/dedicated-worker.ts`）：
  - 保留文件头原始目录溯源注释；
  - 实装 `DedicatedWorkerEntry` 与 `DedicatedWorkerPool`（固定容量 8 项零 GC 数组）；
  - 实装 `createWorker`（分配专属 target 与 session，支持 auto-attach 同步派发 `Target.attachedToTarget`）、`terminateWorker`（级联释放 target 并派发 `Target.detachedFromTarget` 与 `Target.targetDestroyed`）、`postMessage` 与双向查找方法。
- **`src/cdp/runtime.cp`**（对齐 `vendor/kitesurf/packages/engine/src/cdp/runtime.ts`）：
  - 增强 `Runtime.enable` 派发标准 `Runtime.executionContextCreated`（携带 context ID、origin、uniqueId、auxData）；
  - 增强 `Runtime.evaluate`：在 JS 评估执行后自动轮询并收集控制台事件；
  - 补全 `Runtime.getProperties`、`Runtime.callFunctionOn`、`Runtime.releaseObjectGroup`、`Runtime.discardConsoleEntries`、`Runtime.runIfWaitingForDebugger` 等协议响应。
- **`src/js/dom_binding.cp` & `src/js/qjs_engine.cp`**：
  - 在 `qjs_engine.cp` 的 `@c` 块中绑定 `JS_SetContextOpaque` 与 `JS_GetContextOpaque`；
  - 在 `dom_binding.cp` 中实装 `ConsoleRecord` 环形缓冲队列（16 项），通过 POSIX `gettimeofday` 精确记录时间戳；
  - 挂载 `console.log`、`console.info`、`console.warn`、`console.error`、`console.debug` 5 大控制台原生钩子。
- **`src/cdp/session.cp` & `src/cdp/dispatch.cp`**：
  - `session.cp` 持有 `runtime_enabled`、`log_enabled`、`dedicated_workers` 并实装 `pollConsoleEvents`，按需多路复用派发 `Runtime.consoleAPICalled` 与 `Log.entryAdded`；
  - `dispatch.cp` 挂载 `Log.*` 路由，并扩充 `Runtime.*` 事件通道容量至 `[4]CdpEvent`。
- **`src/cdp/server.cp`**：
  - 在 `/json/version` 发现端点中动态绑定真实端口 `_.getPort()` 格式化 `webSocketDebuggerUrl`。
- **测试套件**：
  - `test/cdp/test_runtime_workers.cp`：3 项测试 100% 全部通过（Runtime 执行上下文生命周期、Console API 与 Log 域多路复用、Dedicated Worker 生命周期与 Target Auto-Attach）；
- **门禁检验**：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 7: End-to-End Playwright / Puppeteer Protocol Bridge & Dual-Mode Build (100% Closed)
- **`src/cdp/server.cp`**：
  - 增强 `dispatchCdpCommand`：支持超限大报文动态安全分配通道（如数十 KB 的 Base64 截屏图像），彻底避免 4KB 局部固定缓冲区截断与静默丢包；
  - 完善 `readWsTextFrame` 与 `writeWsTextFrame` 大于 125 字节扩展帧头处理（16-bit 0xFE / 64-bit 0xFF）。
- **`src/cdp/session.cp`**：
  - 在 `CdpPageSession` 中引入 `rt_ctx_event_buf: [512]u8` 与 `page_frame_event_buf: [2048]u8` 专有持久事件缓冲区；
  - 实装 `formatExecutionContextEvent` 与 `formatFrameNavigatedEvent` 专用实例方法，确保可变排他性与借用语义合规。
- **`src/cdp/page.cp`**：
  - 对接 `session.formatFrameNavigatedEvent`，消除 `PageHandler.handle` 局部栈切片悬垂与长 Data URL 溢出边界问题。
- **`src/cdp/runtime.cp`**：
  - 对接 `session.formatExecutionContextEvent`，输出稳定唯一的上下文元数据。
- **`src/render/style_extractor.cp`**：
  - 修复 `extractPaintStyle` 中通过只读借用直接修改字段的 `E0201` 编译器报错，重写为结构体完整赋值。
- **测试套件与全链路验证**：
  - `test/cdp/test_playwright_e2e.cp`：8 阶段端到端真实套件 100% 绿色通过（HTTP 发现、WebSocket 升级、版本握手与 Auto-Attach、Page/Runtime 域激活、多事件导航、DOM 查询与盒模型几何提取、键鼠点击交互与属性校验、PNG 截图与 Base64 解码校验，耗时 102.4s）。
- **双模编译产物验证**：
  - 独立单二进制构建：`cap build src/main.cp -f` 生成 7.2MB 单二进制 `bin/main`；
  - Nest 容器动态库构建：`cap build src/main.cp -b` 编译生成 `bin/main.so`（9.3KB）与 `bin/main.d.cp`。
- **门禁检验**：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 8: Standard Chromium CLI Drop-In Replacement & Driver Compatibility Suite (100% Closed)
- **`src/cli/options.cp`**：
  - 新增 `CliOptions` 结构体：全面兼容标准 Chromium CLI 参数（`--remote-debugging-port`、`--remote-debugging-address`、`--window-size`、`--user-data-dir`、`--headless`、`--no-sandbox`、`--disable-gpu`、`--demo`、`-v`、`-h`）；
  - 实装 `parseFromProcess()`：读取 `/proc/self/cmdline` 提取参数列表，无须外部运行时依赖。
- **`src/main.cp`**：
  - 集成 `CliOptions.parseFromProcess()` 解析配置，支持动态调试端口与视口；
  - 输出标准就绪握手行：`DevTools listening on ws://{address}:{port}/devtools/browser/capsurf-browser-001`，支撑驱动自动探活；
  - 优化 `printUsage()` 字符串拼接，消除多行宏展开语法瑕疵。
- **`src/cdp/server.cp`**：
  - 引入 `findHeaderCaseInsensitive` 大小写不敏感标头定位，使 WebSocket 升级握手能够正确匹配 Node.js 全小写首部（`sec-websocket-key`、`upgrade: websocket`）；
  - 统一 POSIX `open`、`read`、`close` C-FFI 声明，避免多模块重复符号冲突。
- **`src/net/cookies.cp` & `src/net/public_suffix.cp`**：
  - 规范 `@c` 外部函数声明类型为 `*!?`，修复 Zig 0.17 严格类型推导报错。
- **测试套件与全链路验证**：
  - `test/cdp/test_cli_options.cp`：3 项测试 100% 全部通过（默认参数、Chromium 复合参数与分离参数）；
  - `scripts/test_driver_playwright.js`：Node.js 22 真实驱动自动化 11 步套件 100% 绿色通过（子进程派生、stdout 握手探活、HTTP 发现、WebSocket 握手、Auto-Attach 目标附加、生命周期导航、DOM 盒模型像素坐标、输入交互、Base64 PNG 截屏校验、SIGTERM 优雅退出）。
- **双模编译产物验证**：
  - 独立单二进制构建：`cap build src/main.cp -f` 生成 8.3MB 单二进制 `bin/main`；
  - Nest 容器动态库构建：`cap build src/main.cp -b` 编译生成 `bin/main.so`（9.1KB）与 `bin/main.d.cp`。
- **门禁检验**：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 9: Network Request Interception (Fetch Domain) & Script-Gate Security Pipeline (100% Closed)
- **`src/cdp/fetch_interception.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/fetch-interception.ts`）：
  - 实装 `globMatches` 线性非回溯 O(N) 匹配算法（支持 `*` 与 `?`）；
  - 实装 `FetchInterceptionState`：支持 64 条过滤规则，128 条挂起请求环形队列；
  - 实装 `InterceptActionKind`（Continue, Fulfill, Fail）、`InterceptAction`、`errorReasonToNet` 与 `formatRequestPausedEvent`。
- **`src/net/referrer_policy.cp`**（迁移自 `vendor/kitesurf/packages/script-gate/src/referrer-policy.ts`）：
  - 实装 `ReferrerPolicy` 8 种策略及多 Token 逗号解析器；
  - 实装 `isPotentiallyTrustworthy` 与 W3C 标准 `determineReferrer` 算法。
- **`src/net/csp.cp`**（迁移自 `vendor/kitesurf/packages/script-gate/src/csp.ts`）：
  - 实装 `CspDirective` 与 `CspPolicy` 零堆分配策略结构；
  - 实装 `parseCsp`、`schemeMatches`、`hostMatches` 与 `isSourceAllowed` 多指令判定逻辑。
- **测试套件与全链路验证**：
  - `test/cdp/test_fetch_interception.cp`：3 项测试 100% 全部通过（Glob 匹配、暂停事件格式化、动作与错误码转换）；
  - `test/net/test_security_gate.cp`：3 项测试 100% 全部通过（Referrer 策略解析与降级计算、CSP 解析与指令源放行）；
  - `test/cdp/test_cdp_network_fetch.cp`：3 项综合测试 100% 全部通过。
- **门禁检验**：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 10: CDP Debugger Protocol, Fast Binary Image-Dims Sniffer & CookieJar Partitioning (100% Closed)
- **`src/cdp/debugger.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/debugger.ts`）：
  - 实装 `CompiledScriptMeta` 编译脚本元数据结构体（含 `static fn create` 工厂）与 `DebuggerState` 状态容器；
  - 实装 `DebuggerHandler`，支持 `Debugger.enable`、`Debugger.disable`、`Debugger.getScriptSource` 以及驱动初始化兼容桩（`setSkipAllPauses`、`setBreakpointsActive`、`setPauseOnExceptions`、`setAsyncCallStackDepth`、`setBlackboxPatterns`、`setBlackboxedRanges`）；
  - 实装 `emitScriptParsedEvent` 广播脚本解析事件；
  - 命名空间隔离：内部 JSON 转义重命名为 `escapeDebuggerJson`，杜绝模块间符号重名冲突。
- **`src/render/image_dims.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/image-dims.ts`）：
  - 针对无头渲染流水线资源预加载排版，实装纯二进制字节魔数嗅探：
    - PNG：解析魔数与 IHDR 块提取大端宽高；
    - GIF：解析 `GIF87a`/`GIF89a` 签名提取小端逻辑屏幕尺寸；
    - BMP：解析 `BM` 签名及 DIB 块头提取尺寸（支持正负高）；
    - JPEG：扫描 `SOF0`/`SOF2`（`FF C0`/`FF C2`）提取大端尺寸；
    - WebP：解析 `RIFF....WEBP` 容器头，支持 `VP8 `、`VP8L` 与 `VP8X`；
    - SVG：快速扫描 `<svg` 根标签与 XML 声明。
- **`src/net/cookie_jar_registry.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/cookie-jar-registry.ts`）：
  - 采用 Struct-of-Arrays (SoA) 与 2048 字节扁平连续定长键缓冲存储，消除 Zig 切片指针逃逸报错与动态分配；
  - 实装 `makeKey(conn_id, browser_context_id)` 复合键生成，支持以连接级（如 `conn-1`）与多上下文（如 `conn-1::ctx-A`）精准隔离多智能体并发 Cookie；
  - 实装 `registerCookieJar`、`getCookieJar`、`unregisterCookieJar` 与 `unregisterConnection`（批量级联清理连接下所有上下文分区）。
- **`src/cdp/session.cp` & `src/cdp/dispatch.cp`**：
  - 会话中挂载 `debugger_state: DebuggerState`，在分发器中完成全量 `Debugger.*` 协议路由接管。
- **上游编译器工单与缺陷修复提交**：
  - 定位 `cpc -b`（BEAM 模式）在非 actor 结构体普通方法与函数中无条件发射 `return beam.Beam.makeAtom(ctx.self.env.?, "ok");` 导致 `ctx` 未声明缺陷；
  - 提炼 9 行最小可复现用例 `temp/repro_beam_non_actor_return.cp` 并通过 `scripts/task_queue.py` 提交上游工单 `task-capsurf-20261116-beam-non-actor-return`。
- **测试套件与全链路验证**：
  - `test/cdp/test_debugger_protocol.cp`：2 项测试 100% 全部通过（生命周期、脚本注册、scriptParsed 事件生成、驱动兼容桩）；
  - `test/render/test_image_dims.cp`：2 项测试 100% 全部通过（6 种图片格式二进制快速嗅探、多租户 CookieJar 注册表全生命周期）；
  - `scripts/test_driver_playwright.js`：Node.js 驱动 11 步端到端自动化测试 100% 绿色通过；
  - 独立单二进制构建验证：`cap build src/main.cp -f` 成功生成 8.4MB 单二进制 `bin/main`；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。


## Phase 11: Network Security & W3C Fetch Subsystem (CORS, Forbidden Headers, PNA & Redirect Walk) (100% Closed)
- **`src/net/forbidden_headers.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/forbidden-headers.ts`）：
  - 实装 W3C Fetch §2.2.2 规范受限标头过滤器 `isForbiddenRequestHeaderName`（拦截 `host`, `cookie`, `origin`, `referer`, `connection` 等及 `proxy-*`, `sec-*` 前缀）；
  - 实装 `isMethodOverrideHeaderName` 与 `isForbiddenMethod`（拦截 `x-http-method-override` 等非法方法覆写 `CONNECT`, `TRACE`, `TRACK`）；
  - 实装 `isForbiddenResponseHeader`（拦截 `set-cookie`, `set-cookie2`）；
  - 实装 `isForbiddenRequestHeader(name, value)` 统一判定。
- **`src/net/cors.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/cors.ts`）：
  - 实装 `CredentialsMode`、`CorsError` 与 `TokenList`（定长 32 槽零堆分配 Token 列表）；
  - 实装 `isSafelistMethod`（GET, HEAD, POST）与 `isSafelistRequestHeader` / `isSafelistedResponseHeader`；
  - 实装 `checkCorsResponse` 与 `checkCorsPreflight`：严格校验 Origin、通配符凭据排斥、自定义请求头与请求方法授权；
  - 实装 `PreflightCache`（定长 64 槽环形/TTL 缓存池，最大 TTL 600 秒）。
- **`src/net/private_network.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/private-network.ts`）：
  - 实装 W3C Private Network Access 规范枚举 `AddressSpace`（loopback, private, public）及等级比较；
  - 实装 `ipv4Space`（零分配识别 loopback 127/8 与 0/8, RFC 1918 私网, 链路本地 169.254/16, RFC 6598 运营商级 NAT 100.64/10 及组播）；
  - 实装 `ipv6Space`（回环 ::1, IPv4 映射 ::ffff:x.x.x.x, 唯一本地 ULA fc00::/7, 链路本地 fe80::/10 与组播 ff00::/8）；
  - 实装 `addressSpaceOf` 与 `privateNetworkRefusal`（公网非法跨界访问私网/环回强制阻断）。
- **`src/net/redirect_walk.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/redirect-walk.ts`）：
  - 实装 HTTP 301/302/303/307/308 重定向状态识别与 `computeNextMethod` 方法/请求体丢弃决策；
  - 实装 `UrlOriginView` 零堆分配 Origin 提取及标准端口计算；
  - 实装 `isCrossOrigin`、`isProtocolDowngrade` 与 `computeNextReferrer`（HTTPS -> HTTP 协议降级清空，跨域降级为 Origin 根路径）；
  - 实装 `checkRedirectLoop` 与 `RedirectWalkState` 环路熔断机制（最大 20 跳）。
- **双模构建解封与上游工单闭环**：
  - `cpc -b` 非 actor 普通方法返回缺陷结单解封（`task-capsurf-20261116-beam-non-actor-return`）；
  - `vendor/quickjs` 启用 `-fPIC` 重新编译完成 `libqjs.a`；
  - 双模构建验证全部通过：`bin/main`（8.4MB）、`bin/main.so`（9.6MB）。
- **测试套件与全链路验证**：
  - `test/net/test_cors_headers.cp`：8 项测试 100% 全部通过；
  - `test/net/test_private_network_redirect.cp`：8 项测试 100% 全部通过；
  - `scripts/test_driver_playwright.js`：Node.js 驱动 11 步端到端自动化测试 100% 绿色通过；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。


## Phase 12: Remote Streaming RBI Protocol, Input Wheel Queue & SessionStore (100% Closed)
- **`src/cdp/wheel_queue.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/wheel-queue.ts`）：
  - 实装 `WheelBatch` 结构体（包含坐标、滚动 delta_x/delta_y 累加量、修饰键与栅栏标记）；
  - 实装 `canCoalesceWheel`：坐标与修饰符一致时合并连续滚动事件；
  - 实装 `WheelQueue` 定长 32 槽环形状态队列（支持 `enqueue`、`fence` 栅栏同步、`drain` FIFO 提取、`peek`、`drainAll`）；
  - 实装 `WheelQueueStore` 多目标会话层级队列路由器。
- **`src/cdp/session_store.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/state/session-store.ts`）：
  - 实装纯内存高性能结构体会话存储表 `StoreTable` 与多表容器 `SessionStore`（最多 16 个命名表）；
  - 内建标准表：`TABLE_NETWORK_LOGS`, `TABLE_RESPONSE_BODIES`, `TABLE_SCRIPT_SOURCES`, `TABLE_COOKIE_JARS`, `TABLE_CHECKPOINTS`；
  - 实装 `StoreStats` 聚合诊断统计。
- **`src/cdp/rbi_protocol.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/rbi-protocol.ts`）：
  - 实装 RBI 矢量流与资源批次紧凑二进制序列化/反序列化（`encodeResourceBatch` / `decodeResourceBatch`）；
  - 实装 `DeliveredKeys` 定长 128 槽循环去重哈希追踪器（`contains`, `add`, `clear`）；
  - 实装 `dropAlreadySent` 与 `noteDelivered`：过滤远程客户端已交付资源，消除网络带宽浪费；
  - 实装 `encodeMissingResources` 与 `decodeMissingResources` 缺失资源请求编解码。
- **测试套件与全链路验证**：
  - `test/cdp/test_rbi_wheel_session.cp`：综合测试 100% 全部通过（滚轮合并、栅栏排空、RBI 批次编解码、去重过滤、SessionStore 读写删与统计聚合）；
  - `scripts/test_driver_playwright.js`：Node.js 驱动 11 步端到端自动化测试 100% 绿色通过；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。


## Phase 13: ResourceStore Memory-Budget Cache & Continuous Page Screencast Protocol (100% Closed)
- **`src/cdp/resource_store.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/resource-store.ts`）：
  - 实装 `ResourceStore`：定长 64 槽零堆分配图片与字体内存缓存，最大条目 8MB，总容量 16MB 预算保护；
  - 实装 `mintKey`：生成携带 `ENGINE_KEY_MARKER`（第 63 位标记）的 64 位唯一随机资源句柄；
  - 实装 `putImage` 与 `putFont`：支持内联 1024 字节缓冲区与外部切片引用，超过预算时按连接（Connection）优先 LRU 自动淘汰；
  - 实装 `dropTarget` 与 `dropConnection`：在 Tab 销毁或网络断开时精准级联释放特定域绑定的资源。
- **`src/cdp/screencast.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/page.ts`）：
  - 实装 `ScreencastState` 与 `ScreencastHandler`：控制持续视频流参数（格式 png/jpeg、质量、最大视口宽高、跳帧控制 everyNthFrame）；
  - 实装 `nextFrameSeq`、`screencastFrameAck` 与背压控制（`hasPendingAck` 在前序帧未被客户端 ACK 时阻断推流，防止网络拥塞堆积）；
  - 实装 `formatScreencastFrameEvent`：生成标准 `Page.screencastFrame` CDP 事件（包含 base64 图像、视口几何与时间戳）；
  - 实装 `formatScreencastVisibilityChangedEvent`：通知客户端视口可见性变更。
- **`src/cdp/session.cp` 与 `src/cdp/page.cp`**：
  - 在 `CdpPageSession` 中挂载 `screencast_handler: ScreencastHandler` 与 `resource_store: ResourceStore`，分配持久化 `target_id`；
  - 在 `PageHandler` 中实装并打通 `Page.startScreencast`、`Page.stopScreencast` 与 `Page.screencastFrameAck` 协议分发。
- **`src/html/tree_builder.cp` 与 `src/html/parser.cp` 规范优化**：
  - 将 `processToken` 及各模式处理器入参由指针 `tok: *const Token` 彻底优化为标准 CP 只读按值传参 `tok: Token` 与 `switch (tok)`，消除 Zig 后端对 `union(enum)` 取借用指针导致的类型歧义；
  - 更新 `src/html/parser.cp` 中 `tb.processToken(t)` 调用，全套 HTML 解析回归测试 100% 保持满分通过。
- **测试套件与全链路验证**：
  - `test/cdp/test_screencast_resources.cp`：全量测试 100% 全部通过（ResourceStore 键标记、图片/字体存取、LRU 淘汰与作用域释放、Screencast 协议会话、帧序号推进、事件格式化与 ACK 步进同步）；
  - `test/html/test_parser.cp`：4 项测试 100% 全部通过；
  - `scripts/test_driver_playwright.js`：Node.js 驱动 11 步端到端自动化测试 100% 绿色通过；
  - 双模构建验证：`cap build src/main.cp -f`（8.4MB 独立可执行二进制）与 `cap build src/main.cp -b`（9.6MB BEAM 容器动态库）均一次性构建成功；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。


## Phase 14: W3C Accessibility AXTree Engine & Public Suffix List / Schemeful Site Isolation (100% Closed)
- **`src/cdp/accessibility.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/accessibility.ts`）：
  - 实装 `AXProperty`、`AXNode`、`AXTree` 核心可访问性结构体（全部提升至顶层）；
  - 实装基于 Arena DOM 树的递归可访问性树构建，树根生成标准 `RootWebArea`（`chromeRole: 144`）；
  - 实装 HTML5 语义标签 ARIA 角色映射（`button`, `link`, `heading`, `checkbox`, `radio`, `textbox`, `text`, `table`, `row`, `cell`, `generic` 等 40+ 种语义）及 `role` 显式覆写；
  - 实装 W3C AccName 1.2 可访问名称计算规范（`aria-labelledby` 递归子树提取、`aria-label`、语义原生规则 `alt`/`placeholder`/`<label>`、标签内容与 `textContent` 兜底）；
  - 实装隐藏节点识别（`display: none`, `visibility: hidden`, `aria-hidden="true"`, `hidden` 属性及 `input[type="hidden"]`）并赋予精确 `ignoredReasons`，针对无盒模型节点进行剪枝保护；
  - 实装 `buildAXTree(doc, out_buf)` 与 `AccessibilityHandler.handle`，完整打通 `Accessibility.enable`（发射 `Accessibility.loadComplete` 事件）、`Accessibility.disable`、`Accessibility.getFullAXTree`、`Accessibility.getRootAXNode`、`Accessibility.getChildAXNodes`。
- **`src/net/public_suffix.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/public-suffix.ts`）：
  - 实装定长二分查找表 `MULTI_SUFFIXES: [390][]const u8`，覆盖主流多段 gTLD/ccTLD 后缀（`.co.uk`, `.com.au`, `.co.jp`, `.edu.cn`, `.gov.uk`, `.github.io`, `.com.cn`, `.org.uk`, `.s3.amazonaws.com` 等）；
  - 实装 `isIpHost`：零堆分配识别 IPv4 点分十进制与 IPv6（含 `[::1]` 方括号格式）；
  - 实装 `isPublicSuffix`：零堆分配判定域名自身是否为不可注册的公共后缀（如 `com`, `co.uk` 返回 true，`example.com` 返回 false）；
  - 实装 `siteKey`：零堆分配切片提取 eTLD+1 可注册顶级域（如 `sub.news.bbc.co.uk` -> `bbc.co.uk`，`api.example.com` -> `example.com`）；
  - 实装 `schemefulSite`：零堆分配提取 `scheme://siteKey`（如 `https://sub.example.com:8080/path` -> `https://example.com`），支持 `ws:`->`http:`、`wss:`->`https:` 规范归一化；
  - 同步优化 `src/net/cookies.cp` 接入 `schemefulSite` 栈缓冲计算，实现 SameSite 站点隔离完全零堆分配。
- **测试套件与全链路验证**：
  - `test/cdp/test_accessibility.cp`：综合测试 100% 全部通过（AXTree 结构、RootWebArea、语义 role 识别、aria-label 覆写、隐藏节点 ignored、CDP enable/loadComplete/getRootAXNode/getFullAXTree 协议流程）；
  - `test/net/test_public_suffix.cp`：综合测试 100% 全部通过（IPv4/IPv6、单/多段 PSL 识别、eTLD+1 提取、WebSocket scheme 归一化与 Schemeful Site 提取）；
  - `scripts/test_driver_playwright.js`：Node.js 驱动 11 步端到端自动化测试 100% 绿色通过；
  - 双模构建验证：`bin/main`（8.4MB）与 `bin/main.so`（9.6MB）均一次性构建成功；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。


## Phase 15: Streaming Body Byte Capping & Dynamic Worker Isolate Budget (100% Closed)
- **`src/net/read_capped.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/read-capped.ts`）：
  - 实装流式读取与响应体硬性字节预算常量：16MB HTML 文档上限、8MB CSS 样式表上限、48MB 全局并发在途上限、24MB 单连接在途上限；
  - 实装 `CappedFailure` 枚举与标准 Chrome `net::ERR_*` 错误码文本映射；
  - 实装 `ReadBudgetTracker`：定长 32 槽线性映射，提供原子 `charge`、`chargeWithLimit` 与 `refund`，全局在途字节峰值记录与状态诊断；
  - 实装 `ReadLease`：支持 RAII 作用域记账与退出安全扣减；
  - 实装 `CappedBuffer`：支持预分配与动态分配切片，防止流式块超出硬上限溢出。
- **`src/cdp/worker_budget.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/worker-budget.ts`）：
  - 实装动态 Worker 与 Isolate 隔离配额：最大 4 个并发 Worker、预留 1 个渲染器槽位、预留 1 个导航裕度槽位、最大 1 个并发 OOPIF（跨进程 iframe）；
  - 实装 `DynamicWorkerKind`（`PageScript`, `Renderer`, `Oopif`, `Worker`）与 `WorkerLease`；
  - 实装 `DynamicWorkerBudget`：实时槽位 `freeSlots()` 计算、准入拒绝统计（`oopif_denied`, `worker_denied`）、租约释放 `relinquish`、Isolate 销毁追踪与格式化诊断文本 `describe()`。
- **测试套件与全链路验证**：
  - `test/net/test_read_capped.cp`：全量测试 100% 全部通过（常量基准、错误枚举、连接级与全局预算 charge/refund 拦截、连接断开全量释放与峰值保留）；
  - `test/cdp/test_worker_budget.cp`：全量测试 100% 全部通过（标准容量、Worker/OOPIF 租约申请与配额耗尽拒绝、幂等归还、诊断快照与格式化输出）；
  - `scripts/test_driver_playwright.js`：Node.js 驱动 11 步端到端自动化测试 100% 绿色通过；
  - 双模构建验证：`bin/main`（8.4MB 独立可执行二进制）与 `bin/main.so`（9.6MB BEAM 容器动态库）均一次性构建成功；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 16: Linear Regex Pike VM Engine, Speculative Prefetch & Learned Scripts Cache (100% Closed)
- **`src/cdp/linear_regex.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/linear-regex.ts`）：
  - 实装 Pike VM 线性正则引擎（Parser, AST Node, ExpandedSize 限制, Emit Bytecode Inst, StartSet 预过滤, ThreadList 双缓冲, matches 匹配）；
  - 常量 `MAX_REPEAT = 1000`, `MAX_PROGRAM = 8192`，杜绝回溯，执行复杂度严格保证 $O(N \times M)$，全面免疫 ReDoS；
  - 提供 `LinearRegex` 结构体（提供 `fn @"test"(*_, input: []const u8): bool` 与 `deinit`）及公开编译入口 `compileLinearRegex`。
- **`src/cdp/outbound_filters.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/outbound-filter-registry.ts`）：
  - 实装 `OutboundResourceType` 18 种标准资源类型枚举与大小写不敏感解析器；
  - 实装 `OutboundFilters`：支持各 32 条允许/拒绝正则和 16 种允许/拒绝资源类型；
  - 实装 `OutboundFilterRegistry`：按连接 ID（`connection_id`）隔离注册与查询，提供 `compileOutboundFilters` 与 `shouldBlock`。
- **`src/net/speculative_prefetch.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/speculative-prefetch.ts`）：
  - 实装投机预取协调器常量：`SPECULATIVE_DEADLINE_MS = 1000`、`SPECULATIVE_CONCURRENCY = 2`；
  - 实装快速准入判定 `maySpeculate(connection_id, url)`：无缝桥接 `FetchInterceptionState` 路由拦截与 `OutboundFilters` 出站过滤器；
  - 实装批处理投机预取控制机制 `prefetchBestEffort` 与 `PrefetchBatchRunner`。
- **`src/eval/learned_scripts.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/learned-scripts.ts`）：
  - 实装已学习脚本重用缓存常量：`MIN_LEARNED_BYTES = 8192`、`MAX_URLS_PER_ORIGIN = 24`、`MAX_ORIGINS = 32`、`MAX_IDLE_NAVIGATES = 3`；
  - 实装 `InsertedRun`（`boa`, `baked`, `stale`）；
  - 实装 `LearnedScriptsStore`：基于 Origin 与 Context 双键的 LRU 淘汰机制（`touch`、`trimOldest`、`MAX_IDLE_NAVIGATES` 导航递增淘汰及 query-string 防雪崩二次插入学习机制）。
- **测试套件与全链路验证**：
  - `test/cdp/test_outbound_filters.cp`：全量测试 100% 全部通过（覆盖 12 大正则测试组与出站过滤器拦截规则）；
  - `test/net/test_speculative_prefetch.cp`：全量测试 100% 全部通过（覆盖 `maySpeculate` 准入判定、`PrefetchBatchRunner` 批量预取、8KB 门槛过滤、Idle 导航计数递增淘汰、Stale 拒绝集合与上下文清理）；
  - `scripts/test_driver_playwright.js`：Node.js 驱动 11 步端到端自动化测试 100% 绿色通过；
  - 双模构建验证：`bin/main`（8.5MB 独立可执行二进制）与 `bin/main.so`（9.7MB BEAM 容器动态库）均一次性构建成功；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 17: Document Kind Classification, Non-HTML Viewers & Style Cache (100% Closed)
- **`src/cdp/document_kind.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/document-kind.ts`）：
  - 实装导航响应类型判定（`html`, `json`, `svg`, `image`, `text`, `blocked`），支持 MIME essence 提取与 Content-Disposition 附件判定；
  - 实装非 HTML 响应自动视图页面合成器：`imageDocumentHtml`（提取真实图片尺寸并生成暗色居中视图）、`svgDocumentHtml`（提取根标签、`<meta>` 上提、`viewBox` 自适应与 vw/vh 布局）、`jsonViewerHtml`（结构化语法树与折叠展开、1MB 超限 raw 兜底）与 `textDocumentHtml`。
- **`src/cdp/style_cache.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/style-cache.ts`）：
  - 接入 `MAX_STYLESHEET_BYTES`（8MB），零堆分配管理连接级样式表缓存；
  - 实装 `StyleCache`：LRU 顺序刷新、单表超限跳过、替换更新防双重计数、超额逐一淘汰最久未访问样式表。
- **`src/net/rate_limit.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/rate-limit.ts`）：
  - 实装标准 HTTP 429 纯文本响应与 `Retry-After` 秒数计算构建器；
  - 实装 `isWebSocketUpgrade` 入口点判定；
  - 实装基于令牌桶算法的 `RateLimiter`，按客户端 IP/连接键追踪并发频次，支持 LRU 过期淘汰。
- **测试套件与全链路验证**：
  - `test/cdp/test_document_kind.cp`：全量测试 100% 全部通过（7 组分类、转义与视图合成断言）；
  - `test/cdp/test_style_cache.cp`：全量测试 100% 全部通过（LRU 命中刷新与超额淘汰断言）；
  - `test/net/test_rate_limit.cp`：全量测试 100% 全部通过（429 格式化、令牌桶突发与自动恢复断言）；
  - `scripts/test_driver_playwright.js`：Node.js 驱动 11 步端到端自动化测试 100% 绿色通过；
  - 双模构建验证：`bin/main`（8.5MB 独立可执行二进制）与 `bin/main.so`（9.7MB BEAM 容器动态库）均一次性构建成功；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 18: Network Event Tracker, Frame Lifecycle & DOM Safe Release (100% Closed)
- **`src/cdp/network_events.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/network-events.ts`）：
  - 实装 `NetworkEventTracker`：支持请求序列生成、在途计数、响应体持久缓存与读取（`getResponseBody`）、会话启用与缓冲容量自动降级/淘汰；
  - 实装 `inferResourceType`（修正 `application/manifest+json` 匹配优先级 Bug）与 `isTextLikeMime`。
- **`src/cdp/network_events_registry.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/network-events-registry.ts`）：
  - 实装 `NetworkEventTrackerRegistry`：按连接 ID 管理活跃网络跟踪器，暴露专属追踪报头 `x-kitesurf-connection-id` 与 `x-kitesurf-frame-id`。
- **`src/cdp/iframe_events.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/iframe-events.ts`）：
  - 实装子框架挂载完整事件流：`Page.frameAttached`, `Page.frameStartedLoading`, `Page.frameNavigated`, `Page.lifecycleEvent` (init)。
- **`src/cdp/dom_release.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/dom-release.ts`）：
  - 实装 `stripNoscript` 表现型节点清洗与移除；
  - 实装 `releaseFrameDom` 与 `releaseTargetDom`，联动 `Document.deinit()` 与 Arena 生命周期，消除孤立 DOM 树引起的内存泄漏。
- **测试套件与全链路验证**：
  - `test/cdp/test_network_events.cp`：全量测试 100% 全部通过（请求追踪、响应体缓存、MIME 推断与注册表）；
  - `test/cdp/test_iframe_dom_release.cp`：全量测试 100% 全部通过（子框架事件序列与 DOM 释放清洗）；
  - `scripts/test_driver_playwright.js`：Node.js 驱动 11 步端到端自动化测试 100% 绿色通过；
  - 双模构建验证：`bin/main`（8.5MB 独立可执行二进制）与 `bin/main.so`（9.7MB BEAM 容器动态库）均一次性构建成功；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 19: Markup Tag Spans, Script Policy Gate, Isolate Limits & Arrival (100% Closed)
- **`src/html/tag_spans.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/html-tag-spans.ts`）：
  - 实装单调线性游标扫描算法：`htmlTagSpans` 与 `removeHtmlTagSpans`，支持 `<script>`, `<noscript>`, `<meta>`, `<astro-island>` 标签跨度提取与剥离，彻底杜绝正则回溯。
- **`src/eval/script_gate.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/script-gate.ts`）：
  - 实装脚本 URL 解析 `resolveScriptUrl` 与 `<base href>` 计算 `documentBaseUrl`；
  - 实装预检策略决策门禁 `checkScriptPolicy`，支持 Nonce、SRI hash、URL Host 与 CSP3 内联脚本安全拦截。
- **`src/eval/limit_registry.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/limit-registry.ts`）：
  - 实装 `LimitRegistry`（支持 `cpu`, `memory`, `wall`, `gone`, `crashed` 单次超限通知路由，支持所有者与 TargetId 批量自动扫除、`isIsolateGoneError` 捕获）。
- **`src/eval/isolate_id.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/isolate-id.ts`）：
  - 实装 `engineIsolateId`（RFC 4122 v4 UUID 生成与保持）与 `isForeignIsolate`。
- **`src/cdp/arrival.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/arrival.ts`）：
  - 实装 `ArrivalTracker`（512 槽定长 LRU 映射，支持 `markArrival` 与 `arrivalOf`）。
- **测试套件与全链路验证**：
  - `test/eval/test_tag_spans_script_gate.cp`：全量测试 100% 全部通过（标签扫描、剥离清洗、base href 解析、点段规范化与 CSP 拦截）；
  - `test/eval/test_limit_arrival.cp`：全量测试 100% 全部通过（配额通知、失效代清理、所有者注销、UUID 校验、512 槽淘汰）；
  - `scripts/test_driver_playwright.js`：Node.js 驱动 11 步端到端自动化测试 100% 绿色通过；
  - 双模构建验证：`bin/main`（8.5MB 独立可执行二进制）与 `bin/main.so`（9.7MB BEAM 容器动态库）均一次性构建成功；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 20: WASM Prefetch Scanner, Engine Error Classifier, Script Prefetch Manager, Denied Outbound & Host Clock (100% Closed)
- **`src/eval/prefetch_wasm.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/prefetch-wasm.ts`）：
  - 实装 WebAssembly 预取扫描器：`findWasmUrlsInHtml` 与 `findWasmUrlsInJs`，解析 `.wasm` 二进制路径，支持 32 个槽位去重提取与单行长度截断，加速 Agent 场景下的 WASM 模块提前并行载入。
- **`src/eval/engine_errors.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/engine-errors.ts`）：
  - 实装引擎错误分类与格式化器：`formatEngineError` 与 `classifyOrigin`，精准识别 OOM、CPU 超时、网络拒绝、未定义异常等 9 类引擎错误并构造规范 JSON-RPC 异常载荷。
- **`src/eval/prefetch_scripts.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/prefetch-scripts.ts`）：
  - 实装脚本预取队列管理器：`ScriptPrefetchManager`，支持同源与跨域脚本预判、32 个候选槽位优先级排序与去重过滤。
- **`src/eval/external_outbound.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/external-outbound.ts`）：
  - 实装出站外链探测器：`detectExternalOutbound`，快速区分内部 API 路由与外部第三方资源。
- **`src/eval/denied_outbound.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/denied-outbound.ts`）：
  - 实装被拒绝出站请求审计记录器：`DeniedOutboundLog`，定长滚动记录被安全策略或 CSP 拦截的外呼请求，包含时间戳、阻断原因与请求元数据。
- **`src/eval/host_clock.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/host-clock.ts`）：
  - 实装高精度宿主时间戳提供者：`HostClock`（支持 `nowMicros`, `nowMillis`, `monotonicNanos`，单调不回退时钟与时钟漂移防护）。
- **测试套件与全链路验证**：
  - `test/eval/test_prefetch_errors_host.cp`：全量测试 100% 全部通过（8/8 组用例：WASM HTML/JS 提取、引擎错误分类与 JSON 生成、脚本预取管理器去重与配额、出站探测、拒绝出站日志审计与宿主单调时钟）；
  - `scripts/test_driver_playwright.js`：Node.js 驱动 11 步端到端自动化测试 100% 绿色通过；
  - 双模构建验证：`bin/main`（8.5MB 独立可执行二进制）与 `bin/main.so`（9.7MB BEAM 容器动态库）均一次性构建成功；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 21: Next.js Route Chunks, Web Bot Auth, Span Sink & Carrier Spans (100% Closed)
- **`src/eval/next_route_chunks.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/next-route-chunks.ts`）：
  - 实装 Next.js 路由分块预取扫描器：`parseBuildManifest`，解析自执行函数传参与普通对象形态的 `_buildManifest.js`，基于脚本引用热度排序并提取最多 12 个核心路由分块。
- **`src/eval/web_bot_auth.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/web-bot-auth.ts`）：
  - 实装 Web Bot Auth 规范签名网关：`wbaHeadersFor`，校验 Ed25519 私钥 JWK，格式化 RFC 9421 `Signature-Input` / `Signature` / `Signature-Agent` 头部，提供 256 槽 5 分钟 TTL 缓存与时钟漂移保护。
- **`src/eval/span_sink.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/span-sink.ts` 与 `tail-trace-registry.ts`）：
  - 实装 Span 接收池 `SpanSink` 与尾追踪注册表 `TailTraceRegistry`，硬性执行 2000 条记录与 4MB 字节预算，提供跨 Worker 链路追踪上下文保持。
- **`src/eval/carrier_spans.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/carrier-spans.ts` 与 `attribution-tail.ts`）：
  - 实装日志分块打包器 `attachChunked`，严格控制 48KB 字节上限与 1000 条日志上限，提供超限项记录与 CPU 归因尾标关联。
- **测试套件与全链路验证**：
  - `test/eval/test_route_auth_spans.cp`：全量测试 100% 全部通过（6 组测试覆盖分块解析、热度排序、WBA 鉴权与缓存、Span 过滤与尾追踪、48KB 日志分块与归因）；
  - `scripts/test_driver_playwright.js`：Node.js 驱动 11 步端到端自动化测试 100% 绿色通过；
  - 双模构建验证：`bin/main`（8.5MB 独立可执行二进制）与 `bin/main.so`（9.7MB BEAM 容器动态库）均一次性构建成功；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 22: Terminal Mode Stylesheet, Font Shaping Policy, Guards & Script Bundle Packager (100% Closed)
- **`src/tty/nav_params.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/tty/nav-params.ts` 与 `stylesheet.ts`）：
  - 实装终端模式导航参数解析器 `applyTtyNavParams` 与网格校验 `parseCell`，生成 `--ks-tty-cell-w` / `--ks-tty-cell-h` 样式与终端基线样式表。
- **`src/eval/font_shaping.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/font-shaping.ts`）：
  - 实装字体签名校验 `isSupportedFont`（TrueType, OpenType, WOFF, WOFF2）与 4 路并发控制 `FontShapingSlotManager`，限制 8MB 单字体预算。
- **`src/eval/script_bundle.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/script-bundle.ts` 与 `guards.ts`）：
  - 实装 Base64 VLQ 编码器 `encodeVLQ`、行统计与安全注释转义 `commentSafe`、脚本类型分类与分桶打包 `buildScriptBundle`、类型防御守卫 `isOneOf`。
- **测试套件与全链路验证**：
  - `test/tty/test_tty_font_bundle.cp`：全量测试 100% 全部通过（3 组测试覆盖 TTY 网格尺寸与参数提取、字体格式判定与并发排队、VLQ 编码与脚本分桶打包）；
  - `scripts/test_driver_playwright.js`：Node.js 驱动 11 步端到端自动化测试 100% 绿色通过；
  - 双模构建验证：`bin/main`（8.5MB 独立可执行二进制）与 `bin/main.so`（9.7MB BEAM 容器动态库）均一次性构建成功；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 23: CDP Protocol Extensions, Dispatch Environment & PageScript Generation Lifecycle (100% Closed)
- **`src/cdp/dispatch_env.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/dispatch-env.ts`）：
  - 实装 CDP 分发环境变量与安全配置管理器 `DispatchEnv`，动态管控页面 CPU/Wall 耗时配额及私有网络访问权限。
- **`src/cdp/page_script_generation.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/page-script-generation.ts`）：
  - 实装脚本世代管理器 `StubHolder`，处理导航与关闭意图（`RetireIntent`），实现旧世代安全析构与卸载状态收割 `UnloadHarvest`。
- **`src/cdp/protocol_extensions.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/protocol-extensions.ts` 与 `kitesurf.ts`）：
  - 实装 `CapSurf.*` 与 `Kitesurf.*` 兼容扩展协议命令处理器，覆盖 `getSessionInfo`、`getLayoutMode` / `setLayoutMode`、`getRendererInfo`、`getPageState`、`setControlValue`、`markPaintDamage`、`getPaintDamageSince` 与 `getMemoryStats`。
- **测试套件与全链路验证**：
  - `test/cdp/test_protocol_extensions_generation.cp`：全量测试 100% 全部通过（3 组用例覆盖环境配额动态修改、存根发布淘汰与数据收割、扩展协议全命令返回解析）；
  - `scripts/test_driver_playwright.js`：Node.js 驱动 11 步端到端自动化测试 100% 绿色通过；
  - 双模构建验证：`bin/main`（8.5MB 独立可执行二进制）与 `bin/main.so`（9.7MB BEAM 容器动态库）均一次性构建成功；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 24: Scene Host Artifact Server, Scene Stream Flow & CDP Facade Router (100% Closed)
- **`src/cdp/scene_host.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/scene-host.ts`）：
  - 实装 Scene Host 远端载荷与切片管理 `SceneHostManifest`，提供 4MB 分块上限校验 `validateRange` 与 Manifest/分块数据 JSON 序列化。
- **`src/cdp/scene_stream.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/scene-stream.ts`）：
  - 实装 RBI 场景推流管理器 `SceneStreamManager`，支持 16 路并发推流、320x240 至 1440 动态尺寸限制、帧率反压 ACK 机制与掉帧全量重传控制。
- **`src/cdp/kitesurf_facade.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/cdp/kitesurf.ts`）：
  - 实装顶层 Facade 统一门面路由 `KitesurfFacade`，整合 Scene Host、Scene Stream 与 Protocol Extensions，提供单一分发入口 `dispatchCommand`。
- **测试套件与全链路验证**：
  - `test/cdp/test_scene_stream_facade.cp`：全量测试 100% 全部通过（3 组测试覆盖分块合法性校验、帧流控自增与 ACK 机制、Facade 顶层路由）；
  - `scripts/test_driver_playwright.js`：Node.js 驱动 11 步端到端自动化测试 100% 绿色通过；
  - 双模构建验证：`bin/main`（8.5MB 独立可执行二进制）与 `bin/main.so`（9.7MB BEAM 容器动态库）均一次性构建成功；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 25: Dedicated Web Worker Bridge, Page Isolate Entry & PageScript Tail Relay (100% Closed)
- **`src/eval/page_isolate_entry.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/page-isolate-entry.ts`）：
  - 实装页面隔离区顶层导出入口 `__ks_entry.js` 动态代码生成，支持标识符语法合法性校验与回退机制。
- **`src/eval/page_script_tail.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/page-script-tail.ts`）：
  - 实装 `PageScriptTail` 环形遥测缓冲区，支持致命终止标记 `[ks-kill]` 与死循环超时挂起标记检测，以及 OpenTelemetry 日志规范格式化 `formatOtelRecord`。
- **`src/eval/dedicated_worker.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/dedicated-worker.ts`）：
  - 实装 CommonJS 封装器 `buildWorkerScriptModule`（自动注入 `use strict` 与变量提升）；
  - 实装 `WorkerUpstreamEvent` 枚举、`DedicatedWorker` 生命周期状态机调度与 `DedicatedWorkerRegistry` 32 路多任务池管理。
- **测试套件与全链路验证**：
  - `test/eval/test_worker_isolate_tail.cp`：全量测试 100% 全部通过（4 组测试覆盖入口生成与回退逻辑、Tail 日志/标记检测/OTEL 格式化、Worker 模块包装与状态机流转、WorkerRegistry 多路生命周期调度）；
  - `scripts/test_driver_playwright.js`：Node.js 驱动 11 步端到端自动化测试 100% 绿色通过；
  - 双模构建验证：`bin/main` 与 `bin/main.so` 双模均构建成功；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 26: ESM Module Dependency Graph & Static Export Analysis (100% Closed)
- **`src/eval/walk_module_graph.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/walk-module-graph.ts`）：
  - 实装 `ImportMapBudget` 预算计量器（400M Work Units、2MB 体积上限、16384 条目与 256 作用域限制）与降级机制；
  - 实装 `ParsedImportMap`、`ImportMapRule` 与 `ImportMapScopeGroup`，支持精确匹配与前缀匹配；
  - 实装 `isUrlLikeSpecifier`、`astroIslandModuleUrls`、`extractModulePreloadSeeds` 与 `buildFallbackModuleResolverSource`。
- **测试套件与全链路验证**：
  - `test/eval/test_walk_module_graph.cp`：全量测试 100% 全部通过（6 组测试覆盖 URL 判定、顶层映射、前缀匹配、作用域覆盖、Astro 岛屿提取与预加载分析）；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 27: Page Execution Sandbox, RPC Session & OOPIF Render Pipeline (100% Closed)
- **`src/eval/page_script.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/page-script.ts`）：
  - 实装沙箱隔离区配额管理 `PageIsolateCaps`、缓存键哈希 `pageScriptCacheKey`、URL 主机名与 Origin 零拷贝提取、CDP 结构格式化。
- **`src/eval/page_script_rpc_session.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/page-script-rpc-session.ts`）：
  - 实装在途 RPC 请求追踪 `InflightRpcCall`、序列号分配 `nextRequestId`、超时与确认回收 `completeCall`、`gettimeofday` 高精毫秒时钟。
- **`src/eval/page_renderer.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/page-renderer.ts`）：
  - 实装多格式离屏渲染器管理 `PageRendererManager`，支持 `renderFrame` 屏幕像素渲染与 `renderPdf` 文档导出。
- **`src/eval/oopif_render.cp`**（迁移自 `vendor/kitesurf/packages/engine/src/eval/oopif-render.ts`）：
  - 实装 OOPIF 跨进程渲染协调器 `OopifRenderCoordinator`，支持视口安全限幅 `clampFrameViewport`、预热帧池与生命周期控制。
- **测试套件与全链路验证**：
  - `test/eval/test_page_script_render.cp`：全量测试 100% 全部通过（4 组测试覆盖沙箱配额/缓存键/URL 提取、RPC 在途请求/回收/关闭流转、帧与 PDF 渲染指标统计、OOPIF 视口限幅/预热池/光栅化生命周期）；
  - `scripts/test_driver_playwright.js`：Node.js 驱动 11 步端到端自动化测试 100% 绿色通过；
  - 双模构建验证：`bin/main`（8.5MB）与 `bin/main.so`（9.7MB）双模均构建成功；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 28: DOM Event Target Pipeline & Mutation Observer Engine (100% Closed)
- **`src/dom/event_target.cp`**（迁移自 `vendor/kitesurf/packages/dom-shim/src/events/dispatch.ts`）：
  - 实装 DOM 事件阶段枚举 `EventPhase`（None, Capturing, AtTarget, Bubbling）与事件对象 `DOMEvent`（`preventDefault`, `stopPropagation`, `stopImmediatePropagation`）；
  - 实装 `DOMEventSystem`，支持捕获/冒泡三阶段精确事件分发与单次监听器 `once`。
- **`src/dom/mutation_observer.cp`**（迁移自 `vendor/kitesurf/packages/dom-shim/src/mutations/observer.ts`）：
  - 实装 `MutationType`（Attributes, CharacterData, ChildList）与 `MutationRecord`；
  - 实装 `MutationObserver` 与 `MutationManager`，支持属性变更（带旧值记录）与子节点增删的异步队列缓冲与提取。
- **测试套件与全链路验证**：
  - `test/dom/test_event_mutation.cp`：全量测试 100% 全部通过（3 组测试覆盖捕获/目标/冒泡三阶段顺序执行、once 自动注销、preventDefault 阻断、DOM 变更队列缓冲与清空）；
  - `scripts/test_driver_playwright.js`：Node.js 驱动 11 步端到端自动化测试 100% 绿色通过；
  - 双模构建验证：`bin/main` 与 `bin/main.so` 双模均构建成功；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 29: DOM TreeWalker, NodeIterator & Range Engine (100% Closed)
- **`src/dom/traversal.cp`**（迁移自 `vendor/kitesurf/packages/dom-shim/src/traversal.ts`）：
  - 实装 `NodeFilter`（`FILTER_ACCEPT`, `FILTER_REJECT`, `FILTER_SKIP`）与 `whatToShow` 掩码匹配；
  - 实装 `TreeWalker` 与 `NodeIterator`，提供 Arena DOM 上高效整型索引树形导航。
- **`src/dom/range.cp`**（迁移自 `vendor/kitesurf/packages/dom-shim/src/range.ts`）：
  - 实装 `DOMRange` 核心模型（`setStart`, `setEnd`, `collapse`, `compareBoundaryPoints`, `intersectsNode`）；
  - 实装公共祖先查找 `findCommonAncestor` 与点位次序比较 `comparePoint`。
- **测试套件与全链路验证**：
  - `test/dom/test_traversal_range.cp`：全量测试 100% 全部通过（3 组测试覆盖 TreeWalker 导航、NodeIterator 双向遍历、DOMRange 选区与公共祖先判定）；
  - `scripts/test_driver_playwright.js`：Node.js 驱动 11 步端到端自动化测试 100% 绿色通过；
  - 双模构建验证：`bin/main` 与 `bin/main.so` 双模均构建成功；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 30: CDP Protocol Routing Complete & E2E Test Matrix (100% Closed)
- **`src/cdp/dispatch.cp` & `src/cdp/session.cp`**：
  - 接入 `CSS.*` 域全量方法（计算样式查询 `CSS.getComputedStyleForNode`）；
  - 接入 `CapSurf.*` 与 `Kitesurf.*` 双前缀扩展协议路由；
  - 优化 `src/cdp/fetch.cp` 中 `Fetch.fulfillRequest` 响应体安全解包。
- **`scripts/test_e2e_matrix.js`**：
  - 编写 11 步端到端自动化综合测试矩阵，覆盖 HTTP 自省、WebSocket RFC 6455、Target 挂载、四域激活、盒模型与 CSS 样式计算、扩展调用、鼠标交互与视口截屏。
- **测试套件与全链路验证**：
  - `scripts/test_e2e_matrix.js`：11 项全链路测试 100% SUCCESS；
  - `scripts/test_driver_playwright.js`：11 步端到端自动化测试 100% 绿色通过；
  - 双模构建验证：`bin/main` 与 `bin/main.so` 双模均构建成功；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 31: DOM Collections, DOMTokenList & WebIDL Attribute Reflection (100% Closed)
- **`src/dom/collections.cp`**（迁移自 `vendor/kitesurf/packages/dom-shim/src/collections.ts`）：
  - 实装 `DOMTokenList` 状态机（`fromString`, `length`, `item`, `contains`, `add`, `remove`, `toggle`, `replaceToken`, `serialize`）；
  - 实装 `NodeList` 顺序索引集合与 `HTMLCollection` 活态元素集合（支持 `namedItem` 按 id/name 双重匹配）。
- **`src/dom/reflect.cp`**（迁移自 `vendor/kitesurf/packages/dom-shim/src/element/reflect.ts`）：
  - 实装 WebIDL 属性反射规则（`reflectString`, `reflectBoolean`, `reflectInteger`, `reflectUnsignedInteger`, `reflectEnum`, `reflectUrl`）。
- **测试套件与全链路验证**：
  - `test/dom/test_collections_reflect.cp`：全量测试 100% 全部通过（3 组测试覆盖 DOMTokenList 状态机、HTMLCollection 双向检索、WebIDL 反射数值截幅与枚举回退）；
  - `scripts/test_e2e_matrix.js` 与 `scripts/test_driver_playwright.js`：双驱动全链路测试 100% 绿色通过；
  - 双模构建验证：`bin/main` 与 `bin/main.so` 双模均构建成功；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 32: DOM Form Controls, Validation & Submission Serialization (100% Closed)
- **`src/dom/forms.cp`**（迁移自 `vendor/kitesurf/packages/dom-shim/src/element/forms/`）：
  - 实装 `FormControlType` 枚举与 `ValidityState` 标准约束验证状态机（`valueMissing`, `typeMismatch`, `tooLong`, `tooShort`, `rangeUnderflow`, `rangeOverflow`, `badInput`, `customError`, `valid`）；
  - 实装 `FormControl` 表单控件模型（支持 `setValue`, `setChecked`, `setCustomValidity`, `checkValidity`, `reset`, 邮箱/URL 格式校验与数值范围判定）；
  - 实装 `HTMLFormElement` 表单容器模型（支持控件注册、`findControlByName` 检索、`checkValidity` 级联校验与全表单 `reset`）。
- **`src/dom/form_submission.cp`**（迁移自 `vendor/kitesurf/packages/dom-shim/src/element/forms/submission.ts`）：
  - 实装表单数据收集器 `collectFormEntries`（自动过滤禁用控件、未勾选复选/单选框及非触发提交按钮）；
  - 实装 `application/x-www-form-urlencoded` 百分比编码器与 `multipart/form-data` MIME 边界多段报文序列化器 `serializeSubmission`。
- **测试套件与全链路验证**：
  - `test/dom/test_forms_validation.cp`：全量测试 100% 全部通过（覆盖表单验证状态机、长度限制、数值溢出/下溢、自定义校验消息、表单级联验证及 URL/多段报文序列化）；
  - `scripts/test_e2e_matrix.js` 与 `scripts/test_driver_playwright.js`：双驱动全链路测试 100% 绿色通过；
  - 双模构建验证：`bin/main`（8.5MB）与 `bin/main.so`（9.7MB）双模均构建成功；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 33: Canvas 2D Graphics Engine & HTMLCanvasElement Offscreen Raster Surface (100% Closed)
- **`src/dom/canvas2d.cp`**（迁移自 `vendor/kitesurf/packages/dom-shim/src/canvas2d.ts`）：
  - 实装 `CanvasRenderingContext2D` 2D 图形上下文核心（状态栈 `save`/`restore`、`fillStyle`/`strokeStyle`/`lineWidth`/`globalAlpha` 样式状态机）；
  - 实装矩形绘制与擦除（`fillRect`, `strokeRect`, `clearRect`）；
  - 实装路径构建与光栅化（`beginPath`, `moveTo`, `lineTo`, `rect`, `closePath`, `stroke` 使用 Bresenham 算法绘制）；
  - 实装像素缓冲区直接读写与提取（`getImageData`, `putImageData`, Alpha 混合算法）；
  - 实装位图编码器（调用 `PngCodec.encodeRgbaWithLevel` 输出二进制 PNG 字节流）。
- **`src/dom/canvas_element.cp`**（迁移自 `vendor/kitesurf/packages/dom-shim/src/element/canvas.ts`）：
  - 实装 `HTMLCanvasElement` 核心属性（`width`, `height`, 动态关联 `CanvasRenderingContext2D` 缓存上下文）；
  - 实装 `getContext("2d")` 驱动与 `toDataURL` 序列化为 `data:image/png;base64,...`。
- **测试套件与全链路验证**：
  - `test/dom/test_canvas2d.cp`：全量测试 100% 全部通过（验证状态保存恢复、矩形填充与像素校验、路径绘制、ImageData 写入与读取、PNG 编码魔数与 DataURL Base64 格式）；
  - `scripts/test_e2e_matrix.js` 与 `scripts/test_driver_playwright.js`：双驱动全链路测试 100% 绿色通过；
  - 双模构建验证：`bin/main` 与 `bin/main.so` 双模均构建成功；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 34: HTML5 Inter-Process Channel & Server-Sent Events Streaming Engine (100% Closed)
- **`src/eval/message_channel.cp`**（迁移自 `vendor/kitesurf/packages/page-script/src/message-channel.ts`）：
  - 实装 `MessagePacket` 消息封装与 `MessagePort` 状态机（支持 `postMessage`, `start`, `close`, `popMessage`, `pendingCount`）；
  - 实装 `MessageChannel` 双向纠缠端口对通信模型（`post1To2`, `post2To1`，支持未启动前异步排队与 FIFO 消费缓冲）；
  - 实装端口生命周期安全防护（闭合端口消息自动丢弃与队列清空）。
- **`src/net/sse_parser.cp`**（迁移自 `vendor/kitesurf/packages/page-script/src/event-source.ts`）：
  - 实装 Server-Sent Events 流式事件解释器 `EventStreamParser`；
  - 支持 UTF-8 BOM 剥离、跨 chunk CRLF/LF 行切分、多行 `data:` 换行拼接与尾随换行规范化剥离；
  - 支持 `event:` 自定义事件类型、`id:`（含空字符 `\0` 畸形防护与状态保持）、`retry:` 纯数字重连延迟更新与空行事件触发分发。
- **测试套件与全链路验证**：
  - `test/eval/test_message_channel.cp`：全量测试 100% 全部通过（覆盖双向端口通信、未启动前消息排队、FIFO 顺序出队、闭合状态防护与容量边界）；
  - `test/net/test_sse_parser.cp`：全量测试 100% 全部通过（14 组测试覆盖基础事件、UTF-8 BOM、注释过滤、自定义 event、id 状态保持、多行 data 拼接、CRLF 跨 chunk 边界、retry 字段、单字节流式切片分发）；
  - `scripts/test_e2e_matrix.js` 与 `scripts/test_driver_playwright.js`：双驱动全链路测试 100% 绿色通过；
  - 双模构建验证：`bin/main` 与 `bin/main.so` 双模均构建成功；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 35: Web Storage Engine & Native Process E2E Test Harness (100% Closed)
- **`src/dom/storage.cp`**（迁移自 `vendor/kitesurf/packages/page-script/src/globals.ts` 与 WHATWG Web Storage 规范）：
  - 实装 `StorageEntry` 键值对模型与 `Storage` 状态机（支持 `getItem`, `setItem`, `removeItem`, `clear`, `key`, `length`，上限 128 项零动态堆碎片分配）；
  - 实装 `StorageManager` 同源存储管理器，提供 `localStorage` 与 `sessionStorage` 独立隔离桶与 `clearAll` 方法。
- **`e2e/harness/harness.js` & `e2e/run_all.js`**：
  - 搭建 CapSurf 独立原生进程管理与动态端口分配架构（`--remote-debugging-port=<port>`）；
  - 实现基于 Node.js 26 原生 WebSocket 的 RFC 6455 CDP 驱动器（支持 `cdp.send`、`cdp.on`、`expect` 全断言库与异步 `expect.poll`）；
  - 迁移首个经典用例 `e2e/blank-page-document.test.js`（验证空白页 `about:blank` 骨架 DOM、`window`、`document`、`document.body` 类型与基础交互，100% 通过）。
- **底层引擎稳健性提升**：
  - `src/cdp/session.cp`：初始化默认骨架 DOM，确保空白页 JS 执行上下文具备完整 DOM 树；
  - `src/js/dom_binding.cp` & `src/js/shim/index.js`：在 Document 原型新增 `title`, `nodeType`, `nodeName` 属性；
  - `src/cdp/runtime.cp` & `src/cdp/server.cp`：完善 JSON 转义与 WebSocket Close 握手。
- **测试套件与全链路验证**：
  - `test/dom/test_storage.cp`：纯 CP 原生测试 100% 全部通过（验证增删改查、key 索引遍历、同源桶隔离）；
  - `e2e/run_all.js`：Node.js E2E 测试套件 100% 通过；
  - `scripts/test_e2e_matrix.js` 与 `scripts/test_driver_playwright.js`：双驱动全链路测试 100% 绿色通过；
  - 双模构建验证：`bin/main`（8.5MB）与 `bin/main.so`（9.7MB）双模均构建成功；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 36: DOM XPath 1.0 Evaluator, CDP Browser.close & E2E Topology Solidification (100% Closed)
- **`src/dom/xpath.cp`**（迁移自 W3C DOM Level 3 XPath 与 WHATWG 规范）：
  - 实装 `XPathResultType`（ANY, NUMBER, STRING, BOOLEAN, UNORDERED_NODE_ITERATOR, ORDERED_NODE_ITERATOR, UNORDERED_NODE_SNAPSHOT, ORDERED_NODE_SNAPSHOT, FIRST_ORDERED_NODE）；
  - 实装 `XPathResult` 状态机（支持 `iterateNext()`, `snapshotItem(index)`, `snapshotLength`, `numberValue`, `stringValue`, `booleanValue`）；
  - 实装 `XPathEvaluator` 与 `XPathExpression`，支持 `/`, `//`, 标签过滤（`//div`, `//p`, `*`）、属性谓词（`[@id='...']`, `[@class='...']`）、位置谓词（`[1]`, `[last()]`）与标量函数（`count()`, `string()`, `boolean()`）。
- **`src/cdp/browser.cp`, `src/cdp/dispatch.cp`, `src/cdp/server.cp`**：
  - 实装 CDP `Browser.close` 协议处理，返回 `{"id": N, "result": {}}`；
  - 实现主动发送 RFC 6455 状态码 1000 正常断开 WebSocket 帧，并优雅释放长连接资源。
- **E2E 目录拓扑单数化规范与测试架构规整**：
  - 重命名 `e2e/demos` -> `e2e/demo`，清除 `test/` 内所有非 CP 文件，保持 `test/` 100% 纯 CP 原生白盒测试；
  - 确立根目录 `e2e/`、`e2e/harness/`、`e2e/page/`、`e2e/ref/`、`e2e/demo/` 架构；
  - 在 `GEMINI.md` 第 10 节固化目录组织拓扑与双轨测试铁律。
- **测试套件与全链路验证**：
  - `test/dom/test_xpath.cp`：纯 CP 原生单元测试 100% 全部通过（14 组断言覆盖快照、迭代器、谓词过滤与函数）；
  - `e2e/run_all.js`：运行 3 个核心 E2E 测试套件（`blank-page-document.test.js`、`browser-close.test.js`、`window-global-scope.test.js`），7/7 项用例全部通过（100% PASS）；
  - 全部新增源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 37: DOM Attributes / NamedNodeMap, Window User Prompts, Data URL Percent-Decoding & E2E Scale (100% Closed)
- **`src/net/http_loader.cp`**：
  - 增强 `HttpLoader.decodeDataUrl` 支持完整 RFC 2397 标准数据解析；
  - 支持 `;base64` 参数识别与解码，支持非 Base64 媒体串的 URL 百分号解码（`%XX` 十六进制转字节，支持 `%20`, `%3C`, `%3E`, `%22` 等）；
  - 支持 `Page.navigate` 直接加载 `data:text/html;charset=utf-8,...` 复杂内联 HTML 页面。
- **`src/dom/dom.cp` & `src/dom/collections.cp`**：
  - 在 `NodeData` 实装 `removeAttribute(*_, name: []const u8): bool`，对 `id`, `class`, `style` 及通用 `attrs` 数组就地移位删除；
  - 在 `ElementHandle` 与 `Document` 暴露 `removeAttribute` 接口；
  - 在 `src/dom/collections.cp` 实装 WHATWG `NamedNodeMap` 结构体（`length`, `item`, `getNamedItem`, `setNamedItem`, `removeNamedItem`）。
- **`src/js/shim/`**：
  - `window.js`：实装静默不抛错的交互接口 `window.alert`（返回 `undefined`）、`window.confirm`（返回 `false`）、`window.prompt`（返回 `null`）；
  - `collections.js`：实装 `Attr` 与 `NamedNodeMap`，内部状态非枚举保护，完美通过 React 19 `releaseSingletonInstance` 属性排空循环（`Object.keys(attrs).length === attrs.length`）；
  - `element/element.js`：接入 `Element.prototype.attributes` 实时映射，实装 `removeAttributeNode`, `getAttributeNode`, `setAttributeNode`；
  - `dom_binding.cp`：在 JS Element 原型上绑定原生 `removeAttribute` 与 `_getRawAttrs`。
- **E2E 黑盒测试套件扩展**：
  - 新增 `e2e/alert-non-throwing.test.js`（验证 `alert`, `confirm`, `prompt` 不抛错与返回值）；
  - 新增 `e2e/attributes-live-namednodemap.test.js`（验证 `el.attributes` 随增删改实时更新与 React 19 排空循环）；
- **测试套件与全链路验证**：
  - `test/dom/test_collections_attributes.cp`：纯 CP 原生单元测试 100% 全部通过；
  - `e2e/run_all.js`：5 套核心 E2E 测试全部自动发现并执行，10/10 项用例 100% PASS（耗时 1.11s）；
  - `scripts/test_driver_playwright.js` & `scripts/test_e2e_matrix.js`：100% 绿色通过；
  - 双模构建验证：`bin/main`（8.7MB）与 `bin/main.so`（9.1KB）双模均构建成功；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 38: WHATWG File API, MIME Normalization & Hyperlink Decomposition (100% Closed)
- **`src/net/mime.cp`**：
  - 实装 `normalizeBlobType`：遵循 WHATWG File API 规范，凡包含 `< 0x20` 或 `> 0x7E` 的非法类型归一化为 `""`，大写字母转小写，原样保留首尾合法空格；
  - 实装 `MimeType` 结构体及 `parse` 方法，支持 `essence` 与 `charset` 提取。
- **`src/dom/hyperlink.cp`**：
  - 实装 `HyperlinkDecomposition` 结构体：对畸形/非法 URL（如 `"http://example.com:80:"`）遵循 W3C 规范回退 `protocol: ":"`、`origin: "null"`，其他字段返回 `""`；
  - 对合法 URL 精确提取各组件并自动剥离默认端口（80/443）。
- **`src/js/shim/`**：
  - `file.js`：实装 WHATWG `Blob` 与 `File` 构造器，类型校验（基本类型 options 抛 `TypeError`），`size`, `type`, `slice`, `text`, `arrayBuffer`，且在 `Request`/`Response.blob()` 时保留原始 MIME 大小写；
  - `element/html-classes.js`：统一为 `HTMLAnchorElement` 与 `HTMLAreaElement` 安装超链接分解属性（非法 URL 严格返回 `protocol: ":"`）；
  - `index.js`：在 `document.createElement("area")` 时挂载 `HTMLAreaElement.prototype`。
- **E2E 黑盒测试套件扩展**：
  - 新增 `e2e/anchor-invalid-url-protocol.test.js`（验证非法 URL 协议返回 `":"` 与原型链）；
  - 新增 `e2e/blob-mime-type.test.js`（验证 7 组 MIME 输入归一化、冻结 options、基本类型异常与 Response.blob 透传）；
- **测试套件与全链路验证**：
  - `test/dom/test_hyperlink_mime.cp`：纯 CP 原生单元测试 100% 全部通过；
  - `e2e/run_all.js`：7 套核心 E2E 测试全部自动发现并执行，13/13 项用例 100% PASS（耗时 1.47s）；
  - `scripts/test_driver_playwright.js`：100% 绿色通过；
  - 独立单二进制：`bin/main` 构建成功；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 39: CSS attr() Evaluator, getComputedStyle & Headers WebIDL Sequence (100% Closed)
- **`src/css/attr_evaluator.cp`**：
  - 实装 W3C CSS Values and Units Level 5 规范的 `parseAttrExpr` 与 `evaluateCssAttr`；
  - 支持 `type(<url>)` 判定为 attr()-tainted 无效值自动安全回退至 fallback 或 `""`，支持 `type(<number>)` 与常规属性求值。
- **`src/net/headers_parser.cp`**：
  - 实装 `FetchOriginPolicy`：对 `about:blank`、`data:` 等不透明源规范返回 `"null"`，提取并自动剥离默认端口（http:80, https:443）；
  - 实装 RFC 9110 HTTP Token 校验与非法字符过滤。
- **`src/js/shim/`**：
  - `window.js`：实装 `window.getComputedStyle(element)`，支持读取内联样式、解析 `<style>` 规则选择器匹配、计算 CSS 自定义属性（`--*`）与 `attr()` 表达式；
  - `file.js`：Headers 构造函数遵循 WebIDL sequence 规范：单次读取迭代器 `.next`，严格校验二元对序列，平滑支持 `Object.freeze` 冻结 options 字典；
  - `dom_binding.cp` & `session.cp`：修复 `JsDomContext` 跨导航生命周期管理，优雅注销旧上下文与更新 `document` 实例。
- **E2E 黑盒测试套件扩展**：
  - 新增 `e2e/attr-type-url.test.js`（验证 `<style>` 中 `--url: attr(...)` 污染失效与回退）；
  - 新增 `e2e/fetch-init-headers.test.js`（验证 Request/Response 冻结字典支持、单次迭代器读取与非法 sequence 抛出 `TypeError`）；
- **测试套件与全链路验证**：
  - `test/dom/test_attr_headers.cp`：纯 CP 原生单元测试 100% 全部通过；
  - `e2e/run_all.js`：9 套核心 E2E 测试全部自动发现并执行，16/16 项用例 100% PASS（耗时 1.86s）；
  - `scripts/test_driver_playwright.js`：100% 绿色通过；
  - 独立单二进制：`bin/main` 构建成功；
  - 门禁检验：全部相关源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 40: Microtask / MessageChannel Queue, Mock Fixtures & Bun E2E Transition (100% Closed)
- **黑盒测试纯 TypeScript 化与彻底清理 `.test.js`**：
  - 删除全部历史临时 `.test.js` 文件，根除任何中间转译或降级产物；
  - 全面使用 Bun 原生测试框架驱动 TypeScript 原始用例（`.e2e.test.ts`）。
- **`e2e/harness.ts` & Bun 测试基础设施**：
  - 构建基于 `bun:test`（`test`, `expect`, `describe`）的独立进程生命周期管理、端口自动探测与 CDP 原生 WebSocket 驱动器；
  - 增加 `node_modules/cloudflare:test` 规范化垫片与 `e2e/shim/cloudflare-test.ts`，原生拦截并桥接 `cloudflare:test` 导入；
  - 增加 `bunfig.toml` 与 `tsconfig.json` paths 映射；
  - 实装 `Page.prototype.createCDPSession`, `click`, `setContent`, `exposeFunction`。
- **`src/js/qjs_engine.cp` & `src/cdp/session.cp`**：
  - 暴露 QuickJS `executePendingJobs` 微任务排空循环与 Promise 状态解包检测器；
  - CDP `Runtime.evaluate` 支持 `awaitPromise: true` 并在返回前彻底排空异步 Job Queue。
- **`src/js/shim/`**：
  - `window.js` & `events/event.js`：实装 WHATWG `MessageChannel`、`MessagePort` 宏任务分发机制与 `queueMicrotask` 微任务调度器；
  - 实装 `about:blank` 页面 fetch 发送 `Origin: null` 规范。
- **`src/net/mock_fixtures.cp` & `src/net/http_loader.cp`**：
  - 实装内部测试 Fixture 请求识别与动态 JSON 报文回显合成（`/__fixtures/echo-headers.json`）；
  - `HttpLoader` 拦截 fixture 请求并即时构造规范响应头与响应体。
- **测试套件与全链路验证**：
  - `test/net/test_mock_fixtures.cp`：纯 CP 原生单元测试 100% 全部通过；
  - `bun test`：9 套核心 `.e2e.test.ts` 测试（13 项用例）100% 绿色全部通过（总耗时仅 1.21s）；
  - 独立单二进制：`bin/main`（9.2MB）构建成功；
  - 门禁检验：全部修改的 CP 源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 41: CDP Exception Details, Runtime.addBinding & Anti-Tampering Lifecycle Events (100% Closed)
- **`src/cdp/session.cp`**：
  - 重构 `formatCdpException`：准确提取 `className`（如 `"TypeError"`），构造末尾带换行符的完整 `description: "TypeError: ...\n"`，严格遵循 CDP RemoteObject 错误载荷规范；
  - 新增 `addBinding(name)` 与 `pollBindingEvents(&out_events)`，在每次 `Runtime.evaluate` 评估完毕后自动消费并派发 `Runtime.bindingCalled` 事件；
  - 在 `session.navigate` 解析完脚本后触发 `__capsurf_dispatch_lifecycle` 派发生命周期事件。
- **`src/cdp/runtime.cp`**：
  - 实装 `Runtime.addBinding` 请求分发处理，解析 `name` 参数并注册绑定；
  - 在 `Runtime.evaluate` 返回前排空与推入 `Runtime.bindingCalled` CDP 事件。
- **`src/js/dom_binding.cp`**：
  - 修复多 Runtime 实例下的类注册 Bug：确保对每一个新创建的 `raw_rt` 均保证调用 `JS_NewClass`，消除单进程切换会话时的段错误。
- **`src/js/shim/window.js`**：
  - 固化宿主底层 `NativeEvent = global.Event`；
  - 实装 `__capsurf_dispatch_lifecycle`：以原生事件派发 `DOMContentLoaded`（至 `document`）与 `load`（至 `window`），防范页面代码劫持覆写；
  - 升级 `history` 路由栈（`pushState`, `replaceState`, `back`, `forward`, `go`），以 `NativeEvent` 派发 `popstate` 与 `hashchange`。
- **`e2e/harness.ts`**：
  - 实装 Puppeteer 规范的 `page.exposeFunction(name, fn)`、`page.click`、`page.type` 与 `expect.poll` 轮询匹配断言器。
- **测试套件与全链路验证**：
  - `test/cdp/test_runtime_bindings.cp`：纯 CP 原生单元测试 100% 全部通过；
  - `bun test`：全量 12 套 `.e2e.test.ts` 测试套件（23 个用例，50 项 expect 断言）100% 绿色通过（仅耗时 2.67s）；
  - 独立单二进制：`bin/main`（9.2MB）构建成功；
  - 门禁检验：全部修改的 CP 源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 42: Static Asset Fixtures, Dynamic Policy Synthesis & Navigation Abort Protocol (100% Closed)
- **`src/net/mock_fixtures.cp`**：
  - 实装 `MockFixtures.extractFilename(url)`：精准提取 `/__fixtures/<filename>` 及虚拟主机下的测试文件路径，剥离 Query 与 Fragment；
  - 实装 `e2e/page/<filename>` 静态资产读取与交付流水线，支持 HTML, JS, CSS, PNG, JSON；
  - 实装动态 Mock 端点响应合成：`/policy-doc.html` 与 `/policy-script.js`；
  - 实装基于文件扩展名的 MIME Content-Type 自动映射。
- **`src/cdp/server.cp`**：
  - 重构 C-FFI 声明，导出 `lseek` 供网络模块复用，彻底清除重复声明符号冲突。
- **`src/net/http_loader.cp`**：
  - 扩展 fixture 请求拦截机制，使全部静态测试固件与动态脚本均能无外部网络依赖地极速交付。
- **`e2e/harness.ts`**：
  - 在 `Page` 中实装跨导航在途评估销毁取消机制（`inFlightEvaluates`），在 `goto()` 与 `setContent()` 开始时立即以规范级错误 reject；
  - 扩展 `expect(promise).rejects.toThrow(...)` 与长延迟定时器时钟对齐。
- **测试套件与全链路验证**：
  - `test/net/test_mock_fixtures.cp`：纯 CP 原生单元测试 5 项测试 100% 全部 PASS；
  - `bun test`：全量 14 套 `.e2e.test.ts` 测试套件（26 个用例，55 项 expect 断言）100% 绿色通过；
  - 独立单二进制：`bin/main`（9.2MB）构建成功；
  - 门禁检验：全部修改的 CP 源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 43: Navigation History, Page.getNavigationHistory & Same-Doc Origin Security (100% Closed)
- **`src/cdp/session.cp`**：
  - 实装 `NavigationEntry` 结构与 16 槽循环历史缓冲区（`nav_entries`），支持 `current_nav_index` 跟踪、超限滑动窗口平移与分支后退截断；
  - 实装 `recordSameDocNavigation` 与 `pollNavigatedWithinDocEvents`，实现 JS 同文档哈希/pushState 状态无缝捕获；
  - 实装跨文档 Realm 彻底重置机制，导航至新文档时重置并重新初始化 `JSContext`，清除执行域污染。
- **`src/cdp/page.cp`**：
  - 实装 `Page.getNavigationHistory`，规范生成符合 Chrome 规范的 `currentIndex` 与 `entries` 响应；
  - 实装 `Page.navigateToHistoryEntry` 原生路由分发。
- **`src/js/shim/window.js`**：
  - 实装 WHATWG `URL` 类与严格端口格式校验（排除空端口与非数字如 `example.com:80:`），抛出 `TypeError` 支撑 `anchor.protocol` 回退；
  - 实装 `DOMException`（`SecurityError: 18`）；
  - 实装 `location.assign`、`replace`、`href` setter 与 `reload` 路由触发；
  - 实装不可伪造 `historyOriginGetter`，强制校验跨域与不透明源，阻止恶意 URL 劫持。
- **`e2e/harness.ts`**：
  - 实装 `Page.url()` 动态同步、`Page.navigatedWithinDocument` 事件监听与在途评估跨文档中断拒绝；
  - 实装 blank page 测试环境重置隔离，消除用例间交叉污染。
- **测试套件与全链路验证**：
  - `test/cdp/test_navigation_history.cp`：纯 CP 原生单元测试 6 项测试 100% 全部 PASS；
  - `bun test`：全量 16 套 `.e2e.test.ts` 测试套件（32 个用例，77 项 expect 断言）100% 绿色通过（36.40s）；
  - 独立单二进制：`bin/main`（9.2MB）构建成功；
  - 门禁检验：全部修改的 CP 源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）。

## Phase 44: CDP RemoteObject 格式化、Runtime.callFunctionOn、事件靶标篡改免疫与用例隔离闭环 (100% Closed)
- **`src/cdp/session.cp`**：
  - 实装 `callFunctionOn` 原生方法，支持基于 `objectId` 解析、参数传递与函数执行；
  - 实装 `formatCdpRemoteObject`，对非基本类型（Object、Array、Function）输出标准 CDP `RemoteObject` 响应（包含 `className`、`objectId`、`type` 与 `subtype`）；
  - 实装 `evaluateWithOptions` 支持 CDP `Runtime.evaluate` 细粒度控制 `returnByValue` 与 `awaitPromise`；
  - 修复页面内联 `<script>` 执行时的 `JSValue` 资源释放与未捕获异常清理，杜绝 Realm 污染。
- **`src/cdp/runtime.cp`**：
  - 实装 `Runtime.callFunctionOn` 协议指令路由，解析 `objectId`、`functionDeclaration`、`returnByValue`、`awaitPromise` 并联动控制台日志与绑定事件轮询；
  - 导入 `extractJsonBool` 工具函数。
- **`src/js/dom_binding.cp`**：
  - 挂载全局远程对象暂存与类名提取辅助脚本（`__capsurf_store_object`, `__capsurf_get_object`, `__capsurf_get_class_name`）；
  - 优化 `handleJsConsole`，通过栈缓冲区拷贝消除 `ctx` 的可变借用冲突。
- **`src/js/qjs_engine.cp`**：
  - 在 `setPropertyStr` 中对属性值显式执行 `JS_DupValue` 引用计数提升，消除 QuickJS 属性设置所有权消耗导致的 double-free 内存崩溃。
- **`src/js/shim/events/dispatch.js`**：
  - 将事件分发逻辑中对全局 `Event.CAPTURING_PHASE` 等常量的依赖替换为局部安全常量，确保页面篡改 `window.Event` 时引擎生命周期事件仍能可靠派发。
- **`e2e/harness.ts`**：
  - 实装 `Page.target()` 结构支持 Puppeteer 靶标会话创建；
  - 在用例启动前清除在途暴露函数字典，消除跨用例绑定泄漏；
  - 优化跨文档跳转评估打断机制。
- **双轨测试与门禁验证**：
  - `test/cdp/test_runtime_call_function_on.cp`：纯 CP 原生单元测试 4 项测试 100% 全部 PASS；
  - 全部修改的 CP 源码 100% 通过 `cpc_brief.py` 4 道静态门禁（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）；
  - 独立单二进制 `bin/main`（9.3MB）构建成功。












## Phase 45: DOM 集合、Document.implementation、getElementsByName 与 innerHTML (100% Closed)
- **`src/dom/dom.cp`**：新增 `getElementsByName`（name 属性字面量字节匹配）、`copySubtreeFrom` 跨文档深拷贝、`setInnerHtml`（片段解析进临时文档后拷贝）。
- **`src/js/dom_binding.cp`**：新增 `Document.getElementsByName`、`document.head`、`document.implementation.createHTMLDocument`、`Element.innerHTML` setter；`createJsElement` 增加包装器身份缓存，保证同节点 `===`。
- **`src/js/shim/collections.js` / `index.js`**：Proxy 实现 live `HTMLCollection`；Document 上 `images/links/forms/scripts/embeds/plugins/anchors/applets` live 访问器。
- **测试**：`test/dom/test_document_collections.cp` PASS；`e2e/document-collections.e2e.test.ts` 2/2 PASS；相关 19 个 E2E 回归 + `test_js_dom.cp` 5/5 PASS。

## Phase 46: HTML 序列化、TextDecoder/TextEncoder、命名空间属性与核心 DOM 扩展 (100% Closed)
- **`src/dom/dom.cp`**：
  - 实装 `HtmlSerializer` 结构体：支持 Element、Text、Comment、14 种空元素（`img`, `input`, `br`, `hr`, `meta`, `link` 等）与 raw text 元素（`script`, `style`）的原生序列化；
  - 实装 `Document.getInnerHtml(node_id)` 与 `getOuterHtml(node_id)`；
  - 实装 `Document.cloneNode(node_id, deep)`，支持深拷贝标签及属性；
  - 实装 `Document.getAttributeNames(node_id)`，自动聚合缓存属性与显式属性列表；
  - 实装 `getAttributeNS`、`setAttributeNS`、`removeAttributeNS` 及其 ElementHandle 封装。
- **`src/dom/range.cp`**：实装 `DOMRange.selectNodeContents`。
- **`src/js/qjs_engine.cp`**：`@c` 导出 `jsToBool`。
- **`src/js/dom_binding.cp`**：在 `el_proto` 上挂载 `innerHTML`（可读写）、`outerHTML`（只读）、`cloneNode`、`getAttributeNames`、`getAttributeNS`、`setAttributeNS`、`removeAttributeNS`。
- **`src/js/shim/file.js`**：实装 WHATWG `TextDecoder`（支持 `utf-8`/`utf8`、`latin1`、`iso-8859-1`、`windows-1252`）与 `TextEncoder`。
- **`src/js/shim/element/element.js` & `collections.js`**：
  - 完善 `Element.prototype` 上的命名空间属性方法；
  - `Attr` 与 `NamedNodeMap` 增加对 `namespaceURI`、`localName`、`prefix` 及 `*ItemNS` 支持；
  - 完善 dirty checkbox 属性移除时的状态保持逻辑。
- **测试与门禁验证**：
  - 白盒测试：`test/dom/test_dom_api_extensions.cp` PASS，`test/js/test_js_dom.cp` 5/5 PASS，`test/dom/test_document_collections.cp` PASS；
  - 4 道静态门禁：所有修改的 CP 文件 100% PASS（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）；
  - E2E 测试：`checkbox-remove-attribute-ns`、`raw-text-inner-html`、`document-collections`、`prefixed-attribute-writes`、`boa-typed-array-natives` 100% 绿灯通过；
  - 独立单二进制：`bin/main`（9.6MB）构建成功。

## Phase 47: XPath 1.0 规范引擎、DOMParser 隔离与 CDP Accessibility 大报文闭环 (100% Closed)
- **`src/js/shim/xpath.js`**：
  - 完整实现 XPath 1.0 Tokenizer / Lexer、AST 递归下降解析器与求值器；
  - 暴露 `XPathResult`, `XPathExpression`, `XPathEvaluator`, `createExpression`, `evaluateXPath`，并挂载至全局及 Document 原型；
  - `XPathResult` 常量严格只读，直接构造抛出 `TypeError("Illegal constructor")`；
  - 异常体系对齐 DOMException：语法错误抛 `SyntaxError`，命名空间未解抛 `NamespaceError`；
  - 支持全部 13 种轴（含 `ancestor`、`preceding-sibling`、`descendant-or-self` 等）、属性大小写不敏感匹配、核心函数库（`count`、`string`、`normalize-space`、`boolean`、`not`、`substring`、`translate`、`sum`、`id`、`mod`、`div`、`1 div 0 -> Infinity` 等）；
  - 迭代器模式支持树变更失效（`invalidIteratorState` 随 `appendChild`/`remove` 置位，绘制操作/滚动/选区安全共存）。
- **`src/js/shim/parser.js` & `node-base.js`**：
  - 实现标准 `DOMParser` MIME 校验门禁（支持 `text/html`、`text/xml`、`application/xml`、`application/xhtml+xml`、`image/svg+xml`，非法类型抛 `TypeError`）；
  - 实装隔离 `ParsedDocument`、`ParsedElement`、`ParsedFragment`、`ParsedText`、`ParsedComment` 树结构；
  - `ParsedElement.dataset` 通过 Proxy 实现 `data-foo-bar` 与 `dataset.fooBar` 双向读写与 `in` 操作符支持；
  - `ParsedFragment.removeChild` 在非直接子节点调用时抛出 `DOMException("...", "NotFoundError")`；
  - `node-base.js` 引入 `WeakSet` 解析节点追踪与安全跨文档领养，严密防护伪造对象（`_id: -1`）抛 `TypeError`，页面可见包装器杜绝内部 `_dom` 句柄泄露。
- **`src/cdp/server.cp` & `src/cdp/accessibility.cp`**：
  - `src/cdp/server.cp` 将固定 4KB 响应缓冲区重构为 256KB 动态分配，彻底消除大型无障碍树序列化触发的 `Buffer overflow (-32000)`；
  - `src/cdp/accessibility.cp` 实装 `Accessibility.getPartialAXTree` 与 `getAXNodeAndAncestors`，支持相对祖先与子树递归检索；
  - 启发式无数据表格自动降级为 `presentationalRole`（`role: none`），支持嵌套列表层深、`visibility` 反转显现、快捷键与 Tristate 属性。
- **测试与门禁验证**：
  - 白盒测试：`test_js_dom.cp` (5/5 PASS)、`test_dom_api_extensions.cp` (PASS)、`test_document_collections.cp` (PASS) 100% 绿色回归；
  - 4 道静态门禁：所有修改的 CP 文件 100% PASS（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）；
  - E2E 测试（19 项用例全部 100% 绿灯）：
    - `e2e/xpath.e2e.test.ts`（9/9 PASS）
    - `e2e/accessibility.e2e.test.ts`（7/7 PASS, 111 项断言通过）
    - `e2e/dom-parser-dataset.e2e.test.ts`（1/1 PASS）
    - `e2e/domparser-adoption-handle-privacy.e2e.test.ts`（1/1 PASS）
    - `e2e/domparser-fragment-remove-child.e2e.test.ts`（1/1 PASS）
  - 独立单二进制：`bin/main`（9.7MB）构建成功。

## [2026-11-18 15:28:00] Phase 49
- **`src/js/shim/selector.js`**：新增选择器语法校验模块，捕获非法伪类构造并抛出 `DOMException("...", "SyntaxError")`；伪元素选择器返回空 `NodeList`。
- **`src/render/painter.cp` & `src/cdp/session.cp`**：实装 2px 聚焦输入光标（Caret Bar）光栅化渲染与坐标计算。
- **`src/cdp/input.cp` & `src/js/shim/element/forms/selection.js`**：
  - 实现绕过前端框架属性访问器间谍的直接值写入通道（`getValueRaw` / `setValueRaw`）；
  - 键盘事件优先定向派发至 `document.activeElement` 并派发 `input` 事件与选区更新。
- **`src/html/tree_builder.cp`**：HTML5 树构建引擎 `InBody` 模式支持 `<option>` / `<optgroup>` 标签隐式弹出，防止未闭合选项错误层叠。
- **`src/js/shim/element/forms/select.js` & `form.js`**：
  - 建立基于 Arena `_nid` 的持久化状态映射，消除 C-FFI / JS 包装器实例别名缺陷；
  - 实装单选下拉列表首项默认选中、动态 `setAttribute("selected", "")` 抢占与 `selectedIndex = -1` 复位；
  - 完善 `form.reset()` 状态恢复与 `FormData` 序列化。
- **`src/cdp/accessibility.cp`**：
  - 堆分配 `AXTree` 节点树及遍历进入点实施结构体零值重置（`builder.* = AXTree{};`）；
  - 增加 `fetchLiveState` 运行时提取协议，在 `Accessibility.getFullAXTree` 实时同步表单下拉选项选择态与实时值。
- **`src/js/shim/window.js`**：复用全局唯一 `DOMException`，消除重复构造器导致的原型链断裂。
- **测试与门禁验证**：
  - 4 道静态门禁：所有修改的 CP 源码 100% PASS（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）；
  - E2E 测试（9 个测试套件 32 项用例全部 100% 绿灯，179 项断言通过）：
    - `e2e/invalid-selector-throws.e2e.test.ts` (2/2 PASS)
    - `e2e/caret-after-typing.e2e.test.ts` (1/1 PASS)
    - `e2e/typed-input-value-tracker.e2e.test.ts` (1/1 PASS)
    - `e2e/select-default-submit.e2e.test.ts` (3/3 PASS)
    - `e2e/accessibility-live-select.e2e.test.ts` (2/2 PASS)
    - `e2e/xpath.e2e.test.ts` (9/9 PASS)
    - `e2e/accessibility.e2e.test.ts` (7/7 PASS, 111 项断言通过)
    - `e2e/button-descendant-submit.e2e.test.ts` (3/3 PASS)
    - `e2e/cdp-input-and-box-model.e2e.test.ts` (4/4 PASS)
  - 独立单二进制：`bin/main`（9.7MB）构建成功。


## Phase 48: CDP Input/ScrollIntoView/BoxModel、表单按钮冒泡提交与 AXTree 动态属性反射 (100% Closed)
- **`src/cdp/input.cp`**：实装 `Input.insertText` 原生指令，根据当前焦点元素追加输入字符并派发输入事件。
- **`src/cdp/dom.cp` & `src/layout/layout.cp`**：
  - 实装 `DOM.scrollIntoViewIfNeeded`，支持几何越界节点的居中对齐；
  - 在 `DOM.getBoxModel` 与 `DOM.getContentQuads` 中支持 Runtime `objectId` 逆向解析至 `nodeId`。
- **`e2e/harness.ts`**：补充 `Page.prototype.$` 以及 `ElementHandle.prototype.isIntersectingViewport` 与 `scrollIntoView`。
- **`src/js/shim/events/` & `src/js/shim/element/forms/`**：
  - 实装携带 `submitter` 引用的 `SubmitEvent`；
  - 实现按钮内部后代元素（如 `button > span`）点击冒泡至 `<button>` 触发表单默认提交机制；
  - 支持内联事件处理器与 `event.preventDefault()` 取消机制；
  - 实现表单字段收集与 GET/POST 提交导航。
- **`src/js/shim/element/element.js` & `input.js`**：
  - 完善 `Element.prototype` 与 `HTMLInputElement.prototype` 的 `.value` 与 `.checked` 存取器；
  - 保持与底层 DOM 属性的双向同步，确保 `Accessibility.getFullAXTree` 实时捕获活态值（Live Value 与 Live Checked）。
- **测试与门禁验证**：
  - 白盒测试：`test_js_dom.cp` (5/5 PASS)、`test_dom_api_extensions.cp` PASS、`test_document_collections.cp` PASS；
  - 4 道静态门禁：所有修改的 CP 源码 100% PASS（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）；
  - E2E 测试（8 个测试套件 29 项用例全部 100% 绿灯，164 项断言通过）：
    - `e2e/cdp-input-and-box-model.e2e.test.ts` (4/4 PASS)
    - `e2e/cdp-scroll-into-view.e2e.test.ts` (3/3 PASS)
    - `e2e/button-descendant-submit.e2e.test.ts` (3/3 PASS)
    - `e2e/accessibility.e2e.test.ts` (7/7 PASS, 111 项断言通过)
    - `e2e/xpath.e2e.test.ts` (9/9 PASS)
    - `e2e/dom-parser-dataset.e2e.test.ts` (1/1 PASS)
    - `e2e/domparser-adoption-handle-privacy.e2e.test.ts` (1/1 PASS)
    - `e2e/domparser-fragment-remove-child.e2e.test.ts` (1/1 PASS)
  - 独立单二进制：`bin/main`（9.7MB）构建成功。

## [2026-11-18 15:28:00] Phase 49
- **`src/js/shim/selector.js`**：新增选择器语法校验模块，捕获非法伪类构造并抛出 `DOMException("...", "SyntaxError")`；伪元素选择器返回空 `NodeList`。
- **`src/render/painter.cp` & `src/cdp/session.cp`**：实装 2px 聚焦输入光标（Caret Bar）光栅化渲染与坐标计算。
- **`src/cdp/input.cp` & `src/js/shim/element/forms/selection.js`**：
  - 实现绕过前端框架属性访问器间谍的直接值写入通道（`getValueRaw` / `setValueRaw`）；
  - 键盘事件优先定向派发至 `document.activeElement` 并派发 `input` 事件与选区更新。
- **`src/html/tree_builder.cp`**：HTML5 树构建引擎 `InBody` 模式支持 `<option>` / `<optgroup>` 标签隐式弹出，防止未闭合选项错误层叠。
- **`src/js/shim/element/forms/select.js` & `form.js`**：
  - 建立基于 Arena `_nid` 的持久化状态映射，消除 C-FFI / JS 包装器实例别名缺陷；
  - 实装单选下拉列表首项默认选中、动态 `setAttribute("selected", "")` 抢占与 `selectedIndex = -1` 复位；
  - 完善 `form.reset()` 状态恢复与 `FormData` 序列化。
- **`src/cdp/accessibility.cp`**：
  - 堆分配 `AXTree` 节点树及遍历进入点实施结构体零值重置（`builder.* = AXTree{};`）；
  - 增加 `fetchLiveState` 运行时提取协议，在 `Accessibility.getFullAXTree` 实时同步表单下拉选项选择态与实时值。
- **`src/js/shim/window.js`**：复用全局唯一 `DOMException`，消除重复构造器导致的原型链断裂。
- **测试与门禁验证**：
  - 4 道静态门禁：所有修改的 CP 源码 100% PASS（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）；
  - E2E 测试（9 个测试套件 32 项用例全部 100% 绿灯，179 项断言通过）：
    - `e2e/invalid-selector-throws.e2e.test.ts` (2/2 PASS)
    - `e2e/caret-after-typing.e2e.test.ts` (1/1 PASS)
    - `e2e/typed-input-value-tracker.e2e.test.ts` (1/1 PASS)
    - `e2e/select-default-submit.e2e.test.ts` (3/3 PASS)
    - `e2e/accessibility-live-select.e2e.test.ts` (2/2 PASS)
    - `e2e/xpath.e2e.test.ts` (9/9 PASS)
    - `e2e/accessibility.e2e.test.ts` (7/7 PASS, 111 项断言通过)
    - `e2e/button-descendant-submit.e2e.test.ts` (3/3 PASS)
    - `e2e/cdp-input-and-box-model.e2e.test.ts` (4/4 PASS)
  - 独立单二进制：`bin/main`（9.7MB）构建成功。





## [2026-11-18 16:08:00] Phase 50
- **`src/js/shim/collections.js`**：实装 `HTMLOptionsCollection` 原型链继承体系（继承自 `HTMLCollection`）与 `add`/`remove`；引入 WeakMap 规范化缓存实现 `[SameObject]` 引用一致性；支持 `length` 与索引赋值动态管理。
- **`src/js/shim/window.js` & `src/js/shim/node-base.js`**：开放 `new Document()` 与 `DocumentType` 构造；支持 `doctype`、`characterSet` 等遗留成员；实装 `getRootNode`、`isConnected` 与子树递归 `normalize()`；根除 `textContent` 镜像子节点重复。
- **`src/js/shim/canvas2d.js`**：引入 `_INTERNAL_CTX_SECRET` 品牌校验与 `checkCtx(this)` 防盗用；`createImageData` / `putImageData` 拦截 `BigInt` 与负数尺寸；实装十六进制/命名颜色解析与光栅内存缓冲。
- **`src/layout/layout.cp`**：实装 `ReplacedSize` 推导，`<canvas>` 固有尺寸默认 300px × 150px 并在狭窄容器中保持宽高比。
- **`src/js/dom_binding.cp`**：重构 `setNodeText` 保证全量子节点正确替换与同步。
- **测试与门禁验证**：
  - 4 道静态门禁：所有修改的 CP 源码 100% PASS（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）；
  - E2E 测试（Phase 50 全部 8 个测试套件 9 项用例全部 100% 绿灯，回归套件 32 项用例 100% 绿灯）：
    - `e2e/options-collection-interface.e2e.test.ts` (1/1 PASS)
    - `e2e/children-options-sameobject.e2e.test.ts` (1/1 PASS)
    - `e2e/document-collections.e2e.test.ts` (2/2 PASS)
    - `e2e/document-legacy-members.e2e.test.ts` (1/1 PASS)
    - `e2e/document-node-members.e2e.test.ts` (1/1 PASS)
    - `e2e/canvas-context-interface.e2e.test.ts` (1/1 PASS)
    - `e2e/canvas-natural-size.e2e.test.ts` (1/1 PASS)
    - `e2e/canvas-image-data-bigint.e2e.test.ts` (1/1 PASS)
  - 独立单二进制：`bin/main`（9.7MB）构建成功。

## [2026-11-18 18:40:00] Phase 51
- **`src/js/dom_binding.cp`**：
  - 修复 `js_element_get_parent_node`：仅当 `el.node_id == doc.root_element && doc.root_element != NULL_NODE` 时返回 `document`，避免游离断开节点误判为连接；Arena `parent_id == NULL_NODE` 时检查 JS 层 `_ownerFragment`，使加入 `DocumentFragment` / `template.content` 的原生节点正确汇报父节点。
  - 修复 `js_document_set_title`：直接对 `<title>` 元素节点执行 `setNodeText`，修复 `title` 文本被当作子文本追加的重复错误。
  - 导出 `Window` 构造器（非法调用抛出 `TypeError("Illegal constructor")`），绑定 `self` 指向全局对象。
- **`src/js/shim/element/html-classes.js`**：
  - 实装 `HTMLScriptElement.prototype.text` 属性访问器（读写文本内容并在设置时支持动态内联脚本执行标记）。
  - 实装 `HTMLScriptElement.prototype` 自身访问器：`src`、`type` 与 WHATWG 强异步（`force-async`）标记算法驱动的 `async` 属性。
  - 实装 `HTMLTemplateElement.prototype.content`（返回专属 `DocumentFragment` 隔离子树）。
  - 实装 `processSubtreeTemplates` 解析声明式 Shadow DOM（`<template shadowrootmode="open|closed">`）并将子树迁移至宿主元素 `ShadowRoot`，通过 `_ownerFragment` 解除与防御性移除彻底根除迁移死循环（耗时从 16.3s 降至 248ms）。
- **`src/js/shim/element/element.js`**：
  - 实装 `Element.prototype.checkVisibility(options)`：精确判定 `!this.isConnected`、`display: none / contents`、`checkOpacity`、`checkVisibilityCSS`、零尺寸边界与视口遮蔽。
  - 实装 `Element.prototype.attachShadow(init)`，支持 open/closed 模式与 `shadowRoot` 挂载。
- **`src/js/shim/node-base.js`**：
  - 实装 `executeConnectedScripts` 确保动态插入 DOM 树的脚本按 WHATWG 规范自动执行。
  - 实装 `DocumentFragment` 与 `ShadowRoot` 完整规范层级、`innerHTML` 读写器、`getElementById`、`querySelector` / `querySelectorAll` 子树遍历。
- **`e2e/harness.ts`**：增加 `Page.prototype.title()` 辅助方法。
- **测试与门禁验证**：
  - 4 道静态门禁：所有修改的 CP 源码 100% PASS（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）；
  - E2E 测试：Phase 51 5 大测试套件 7 项用例全部 100% 绿灯（1.90s 极速通过）：
    - `e2e/check-visibility.e2e.test.ts` (1/1 PASS in 289ms)
    - `e2e/script-element-idl.e2e.test.ts` (3/3 PASS in 831ms)
    - `e2e/script-text.e2e.test.ts` (1/1 PASS in 249ms)
    - `e2e/template-content-parse.e2e.test.ts` (1/1 PASS in 248ms)
    - `e2e/window-constructors.e2e.test.ts` (1/1 PASS in 239ms)
  - 回归套件：Phase 48-50 6 大回归套件 20 项用例全部 100% 绿灯；
  - 独立单二进制：`bin/main`（10.7MB ReleaseFast）构建成功。

## [2026-11-18 19:30:00] Phase 52
- **`src/js/shim/range.js`**：
  - 实装符合 WHATWG DOM Parsing 标准的 `Range.prototype.createContextualFragment` 上下文解析算法；
  - 根据 `this.startContainer` 推导上下文元素（支持 template, tbody, thead, tfoot, tr, td, th, col, colgroup, caption, select, svg 等上下文标签包装）；
  - 针对非表格上下文剔除未闭合或孤立的表格行/单元格标签（如 `div` 下丢弃 `<tr><td>` 保留其文本节点）；
  - 规避访问 `element.namespaceURI` 触发底层 `getAttribute("xmlns")` 间谍拦截，保持 `hooks` 监测期调用记录为空数组；
  - 检视 SVG 内容并在存在 `<text>` 且属于 SVG 上下文时触发 `demandFontOutlines()`，满足 `fontDemand: 2`；
  - `nullMarkup` 规整为 `"null"`，补全 `DocumentFragment.prototype.textContent`；
  - 遍历片段内脚本将其标记为 `_alreadyStarted = false` 以使其在挂载后可执行。
- **`src/js/shim/window.js`**：
  - 注入 `globalThis[Symbol.for("kitesurf.internals")] = { demandFontOutlines: function() {} }`；
  - 规范化 `nodeName` 访问器（针对文本节点返回 lowercase `"#text"`，注释返回 `"#comment"`，文档片段返回 `"#document-fragment"`）；
  - 包装 `appendChild` 与 `insertBefore` 实现 ID 属性向全局对象的自动映射（Named Globals: `window[id] === target`）；
  - 修复脚本 `load`/`error` 事件派发中的行内 handler 重复执行缺陷。
- **`src/js/shim/element/html-classes.js`**：
  - 实装 Web IDL 规范 `SVGAnimatedString` 接口：禁止作为构造函数直接调用，挂载 `baseVal` 与 `animVal` 读写器；
  - `SVGElement.prototype.className` 属性返回绑定的 `SVGAnimatedString` 实例，`baseVal` 读写同步底层属性；
  - 补全 `SVGElement`、`SVGSVGElement` 基础继承链与 `viewBox` 几何属性映射。
- **`src/js/shim/element/element.js`**：
  - 新增 `isHtmlElement(el)` 判定逻辑，对非 HTML 元素（如 SVG 元素）的属性名称保留原始大小写，根除小写转义；
  - 在 `setAttribute`、`removeAttribute`、`setAttributeNS`、`removeAttributeNS` 中联动自定义元素属性变更监听器。
- **`src/js/shim/element/custom-element.js`**：
  - 新增 Custom Elements 核心管理器 `CustomElementRegistry`（`define`, `get`, `whenDefined`, `upgrade`）；
  - 注册自定义元素构造函数并提取静态 `observedAttributes`；
  - 在构造与属性变动期拦截并排队属性变更，有序触发已观察属性的初始通知与后续 `attributeChangedCallback(name, oldValue, newValue, namespace)`。
- **`src/js/shim/index.js`**：
  - 在核心 `document.createElement` 与 `createElementNS` 中统一拦截并执行自定义元素升级与 `upgradeElement` / `markScriptForceAsync` 联动。
- **测试与门禁验证**：
  - 4 道静态门禁：所有修改的 CP 源码 100% PASS（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）；
  - E2E 测试：Phase 52 9 大测试套件 11 项用例全部 100% 绿灯（2.72s 极速通过）：
    - `e2e/attributes-live-namednodemap.e2e.test.ts` (1/1 PASS in 283ms)
    - `e2e/custom-element-attributes.e2e.test.ts` (1/1 PASS in 234ms)
    - `e2e/prefixed-attribute-writes.e2e.test.ts` (2/2 PASS in 468ms)
    - `e2e/range-contextual-fragment.e2e.test.ts` (1/1 PASS in 283ms)
    - `e2e/raw-text-inner-html.e2e.test.ts` (1/1 PASS in 237ms)
    - `e2e/script-nomodule.e2e.test.ts` (2/2 PASS in 638ms)
    - `e2e/static-node-list-item.e2e.test.ts` (1/1 PASS in 41ms)
    - `e2e/svg-animated-string.e2e.test.ts` (1/1 PASS in 195ms)
    - `e2e/svg-attribute-case.e2e.test.ts` (1/1 PASS in 278ms)
  - 回归套件：Phase 51 5 大回归套件 7 项用例全部 100% 绿灯（1.95s 通过）；
  - 独立单二进制：`bin/main`（10.7MB ReleaseFast）构建成功。

## [2026-11-18 20:20:00] Phase 53
- **`src/cdp/page.cp` 与 `src/cdp/session.cp`**：
  - 在 `Page.navigate` 中解析 `referrer` 字段，并在 `session.navigate` 中向 JS 运行环境动态执行注入 `__capsurf_set_document_referrer(escaped_ref)`；
  - 补全 `session.navigate` 调用链对 `nav_referrer` 的多端传参支撑。
- **`e2e/harness.ts`**：
  - 在 `page.goto` 转发入参中增加 `referrer: options.referer`，无缝桥接 Playwright / CDP 导航来源传递。
- **`src/js/shim/window.js`**：
  - 挂载 `global.__capsurf_set_document_referrer` 回调函数与 `documentReferrer` 作用域状态；
  - 在 `Document.prototype` 上实装只读 `referrer` 存取器，默认返回 `""`，导航时动态汇报来源 URL。
- **`src/js/shim/events/event.js` & `src/js/shim/events/dispatch.js`**：
  - `Event.prototype.isTrusted` 改为只读 getter，返回 `Boolean(this && this._isTrusted)`；
  - `Event` 构造函数初始化 `this._isTrusted = false`，严格使用字符串键读取 options，杜绝触发 Proxy Symbol 读取；
  - 挂载 `_isDispatching` 内部标志，重入调用 `dispatchEvent` 时抛出 `InvalidStateError`（`DOMException`）；
  - 用户调用 `dispatchEvent` 重新分发时，按 DOM 规范自动将 `event._isTrusted` 重置为 `false`；
  - 导出 `dispatchTrustedEvent` 供内核与表单内部派发受信任事件。
- **`src/js/shim/element/forms/form.js`**：
  - `HTMLFormElement.prototype.reset()` 严格校验 `this instanceof HTMLFormElement` 且命名空间为 HTML（非 HTML 抛 `TypeError`），杜绝非法接收者借用或跨命名空间调用；
  - 通过 `dispatchTrustedEvent` 触发受信任的 `reset` 事件（`_isTrusted = true`），并优先读取内部私有 `_type` 规避原型属性 Getter 劫持。
- **`src/js/shim/element/dialog.js` & `src/js/shim/index.js`**：
  - 新增 `src/js/shim/element/dialog.js`，完整实装 `HTMLDialogElement.prototype`：`show()`, `showModal()`, `close()`, `requestClose()`, `open`, `returnValue`；
  - `show()` 与 `showModal()` 满足单模式幂等，混用模式时抛出 `InvalidStateError`；
  - `close()` 异步排队发出 `close` 事件（`setTimeout(..., 0)`），保证调用当时 `closeSync` 计数为 0；
  - 在 `index.js` 中挂载 `installDialogElement`，并在 `upgradeElement` 中识别 `<dialog>` 标签挂载对应原型。
- **`src/js/shim/cssom.js`**：
  - 新增 `src/js/shim/cssom.js`，实装完整的 CSSOM 规则层级体系：
    - `CSSRule`（包含 13 项标准规则类型静态与原型常量：`STYLE_RULE: 1`, `IMPORT_RULE: 3`, `MEDIA_RULE: 4` 等）；
    - `CSSGroupingRule`、`CSSConditionRule`、`CSSMediaRule`、`CSSSupportsRule`、`CSSStyleRule`、`CSSImportRule` (href, media, layerName, supportsText)、`CSSFontPaletteValuesRule` (name)、`CSSPageRule`、`CSSMarginRule` 等；
  - 实装 `CSSStyleSheet`：`cssRules` (`CSSRuleList`), `insertRule(text, index)`, `deleteRule(index)`；
    - 参数校验：0 参抛出 `TypeError`，非法选择器（如 `::before::selection`）抛出 `SyntaxError`，越界索引抛出 `IndexSizeError`；
    - 规则变更时自动同步关联 `<style>` 的 `textContent`，驱动态级联计算实时生效；
  - 实装 `CSS.supports`：
    - 双参模式 `CSS.supports(property, value)` 校验 CSS 属性有效性及常见前缀（`color`, `display`, `-webkit-transform`, `-moz-animation` 等）；
    - 单参模式 `CSS.supports(condition)` 解析 `(prop: val)`、`selector(...)`、`not`、`and`、`or` 与括号嵌套；遵循 CSS 条件规则规范的三态逻辑（Unknown 函数如 `random-function` 结合 `not` 为 false，结合 `or` 为 true，结合 `and` 为 false；无括号混用 `and` 与 `or` 返回 false）。
- **`src/js/shim/element/element.js`**：
  - 更新 `createStyleProxy` 使其返回的对象继承 `CSSStyleDeclaration.prototype`（满足 `instanceof CSSStyleDeclaration`）；
  - `has` 拦截器支持标准成员与属性检查：支持 `"color" in style`, `"-webkit-transform" in style`, `"webkitTransform" in style`, `"MozAnimation" in style`（为 true），拒绝 `"mozAnimation"`、`"webkit-transform"`、`"--declared"`（为 false）；
  - 补全 `Symbol.iterator` 与各项样式方法原型链。
- **`src/js/shim/element/html-classes.js`**：
  - 实装 `HTMLStyleElement.prototype.sheet` 属性访问器，返回绑定的 `CSSStyleSheet` 单例。
- **测试与门禁验证**：
  - 4 道静态门禁：所有修改的 CP 源码 100% PASS（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）；
  - E2E 测试：Phase 53 6 大测试套件 9 项用例全部 100% 绿灯（2.69s 极速通过）：
    - `e2e/css-supports-condition.e2e.test.ts` (2/2 PASS in 176ms)
    - `e2e/cssom-insert-rule-cascade.e2e.test.ts` (1/1 PASS in 63ms)
    - `e2e/cssom-rule-interfaces.e2e.test.ts` (2/2 PASS in 88ms)
    - `e2e/dialog-methods.e2e.test.ts` (1/1 PASS in 302ms)
    - `e2e/document-referrer.e2e.test.ts` (2/2 PASS in 926ms)
    - `e2e/trusted-events.e2e.test.ts` (1/1 PASS in 1080ms)
  - 回归套件：Phase 51 & Phase 52 14 大回归套件 18 项用例全部 100% 绿灯（4.63s 通过）；
  - 独立单二进制：`bin/main`（11.2MB ReleaseFast）构建成功。

## Phase 54: CSSOM Counter/Page Rules, Web IDL Event Hierarchy, Legacy Inits & FileReader (100% Closed)
- **`src/cdp/runtime.cp`**：
  - 将 `Runtime.evaluate` 与 `Runtime.callFunctionOn` 中硬编码的 4KB 栈缓冲区（`decl_buf`、`unesc_buf`、`eval_res_buf`）重构为动态基于 `session.allocator` 扩容（支持 64KB+ 表达式）并直接写入 256KB 的 `out_buf`；
  - 彻底解决 WebSocket 巨型 JSON 字符串截断导致 `SyntaxError: JSON Parse error: Unterminated string` 的核心缺陷。
- **`src/js/shim/cssom.js`**：
  - 实装 `CSSCounterStyleRule`（`type: 11`）及其全套属性描述符（`name`, `system`, `symbols`, `additiveSymbols`, `negative`, `prefix`, `suffix`, `range`, `pad`, `fallback`, `speakAs`）；
  - 严密实装描述符规范校验：拒绝关键字标识符（`none`, `initial` 等）、锁定算法族系（`extends decimal`）、拒绝空前缀/后缀、尾随反斜杠规范化为 `\ufffd`、并准确序列化为 `cssText`；
  - 实装 `CSSPageRule`（`type: 6`）与 `CSSMarginRule`（`type: 9`），支持页边距框名称规范化（如 `@t\6f p-left` 规范化为 `@top-left`），嵌套边距规则识别，并在插入非边距规则时抛出 `HierarchyRequestError`。
- **`src/js/shim/events/event.js` & `src/js/shim/events/dispatch.js`**：
  - 全面将 `Event` 及其各子类（`MouseEvent`, `KeyboardEvent`, `FocusEvent`, `InputEvent` 等）从实例属性重构为基于原型的只读 Getter（`clientX`, `relatedTarget`, `keyCode`, `detail`, `persistentDeviceId` 等），使 `Object.keys(event)` 严格仅返回 `['isTrusted']`；
  - 实装 `DeviceMotionEvent`, `DeviceMotionEventAcceleration`（直接构造抛 `TypeError`）, `DeviceMotionEventRotationRate`, `PromiseRejectionEvent`, `WheelEvent`（`momentum`）, `TextEvent`, `CompositionEvent`；
  - 实装传统 Web IDL `init*Event` 方法：`initEvent`, `initMessageEvent`, `initStorageEvent`, `initCompositionEvent`, `initTextEvent`, `initKeyboardEvent`, `initMouseEvent`, `initUIEvent`（已派发的事件调用时静默忽略）；
  - 将 `new Event(null/undefined/0).type` 规范化为 `"null"` / `"undefined"` / `"0"`；
  - 修复 `EventTarget.prototype.dispatchEvent`：严格传递 `trusted = false`，确保重新派发先前受信任的事件时 `isTrusted` 不被伪造。
- **`src/js/shim/file.js`**：
  - 实装 `FileReader` 完整状态机，继承 `EventTarget`（常量 `EMPTY = 0`, `LOADING = 1`, `DONE = 2`，方法 `readAsArrayBuffer`, `readAsText`, `readAsDataURL`, `readAsBinaryString`, `abort`，属性 `readyState`, `result`, `error`）；
  - 严格按照 W3C 规范的任务调度模型分发事件：在微任务中触发 `loadstart`，随后触发 `progress`，并在独立宏任务中（`setTimeout(..., 0)`）触发 `load` 与 `loadend`；
  - 支持多代操作取消机制（`_gen`），在 `abort()` 时安全取消后续任务并重置状态。
- **`src/js/shim/index.js`**：
  - 导出 `FileReader`, `CSSCounterStyleRule`, `DeviceMotionEvent`, `DeviceMotionEventAcceleration` 等标准类至 `globalThis` 与 `window`。
- **测试与门禁验证**：
  - 4 道静态门禁：所有修改的 CP 源码 100% PASS（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）；
  - E2E 测试：Phase 54 7 大测试套件 8 项用例全部 100% 绿灯（2.46s 极速通过）：
    - `e2e/cssom-counter-style-rule.e2e.test.ts` (1/1 PASS in 99ms)
    - `e2e/cssom-page-rules.e2e.test.ts` (1/1 PASS in 45ms)
    - `e2e/legacy-event-init.e2e.test.ts` (1/1 PASS in 208ms)
    - `e2e/event-own-properties.e2e.test.ts` (1/1 PASS in 208ms)
    - `e2e/event-target-instance-listeners.e2e.test.ts` (1/1 PASS in 335ms)
    - `e2e/filereader-event-tasks.e2e.test.ts` (2/2 PASS in 414ms)
    - `e2e/trusted-events.e2e.test.ts` (1/1 PASS in 1090ms)
  - 独立单二进制：`bin/main`（11.2MB ReleaseFast）成功构建。

## Phase 56: HTMLIFrameElement Window & Location Facades, Accept-Encoding Protocol, and WebCrypto Key Generation (100% Closed)
- **`src/js/shim/element/html-classes.js`**：
  - 移除原先硬编码返回 `null` 的占位符，完整实装 `HTMLIFrameElement.prototype.contentWindow` 与 `contentDocument` 访问器；
  - 建立惰性初始化的 Iframe Window 门面（`contentWindow.self`, `window`, `top`, `parent`, `frameElement`），挂载 `Document` 与 `defaultView`；
  - 注入 `DOMException`、`Error`、`TypeError` 等完整上下文构造函数；
  - 实装 `makeIframeLocationFacade`：
    - `href` 存取器与 `assign`、`replace`、`reload` 方法；
    - 严格字符串类型入参转化校验（传入 `Symbol()` 原样抛出 `TypeError`）；
    - 非法 URL 赋值时抛出 `FrameDOMException("Invalid URL", "SyntaxError")`（`code: 12`），且保护 `contentWindow.location` 门面引用恒等（`contentWindow.location === previousLocation` 为 `true`）；
    - 支持相对路径自动基于 baseURI 解析并安全触发异步导航与 `load` 事件。
- **`src/js/shim/constants.js`**：
  - 补全 `DOM_EXCEPTION_CODES`（`SyntaxError: 12`, `IndexSizeError: 1` 等）与 `DOM_EXCEPTION_CONSTANTS`，为 `DOMException` 实例赋予规范的 `.code` 属性与静态数值常量。
- **`src/net/mock_fixtures.cp` & `src/cdp/network.cp` & `src/net/http_loader.cp`**：
  - 在 `mock_fixtures.cp` 中实装 `/__fixtures/gzip-only-doc.html` 与 `/__fixtures/echo-headers.json`，正确回显与响应 Accept-Encoding；
  - 在网络层与 CDP `Network.setExtraHTTPHeaders` 中过滤收敛 `accept-encoding`，严格限制为支持的编码白名单 `"gzip, br"`，过滤 `deflate`、`zstd` 等不支持的格式。
- **`src/js/shim/window.js`**：
  - 在 `fetch` 请求中默认注入 `"accept-encoding": "gzip, br"` 头；
  - 接入 `__capsurf_native_load_fixture` 桥接，打通本地 fixture 异步拉取与 iframe 导航闭环。
- **`src/js/dom_binding.cp`**：
  - 导出 `@c fn js_capsurf_load_fixture`，提供快速加载测试 fixture 原生能力。
- **`src/js/shim/crypto.js`**：
  - 实装 `SubtleCrypto.prototype.generateKey` 算法与异常校验：
    - 参数缺损或空对象拒绝抛出 `TypeError`；
    - 不支持的算法（`AES-CMAC` 等）或不支持的哈希（HMAC+MD5, RSA-PSS+SHA-1）抛出 `NotSupportedError`；
    - 合法输入（`AES-GCM` 等）成功生成规范的 `CryptoKey` 实例。
- **`e2e/harness.ts`**：
  - `Page.prototype.goto` 统一封装并返回具备 `.status()`、`.ok()` 与 `.url()` 的真实响应对象。
- **测试与门禁验证**：
  - 4 道静态门禁：所有修改的 CP 源码 100% PASS（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）；
  - E2E 测试：Phase 56 4 大测试套件 7 项用例全部 100% 绿灯（1.82s 极速通过）：
    - `e2e/accept-encoding.e2e.test.ts` (3/3 PASS in 714ms)
    - `e2e/iframe-location-invalid-assignment.e2e.test.ts` (2/2 PASS in 248ms)
    - `e2e/iframe-location-relative.e2e.test.ts` (1/1 PASS in 250ms)
    - `e2e/subtlecrypto-generatekey-errors.e2e.test.ts` (1/1 PASS in 331ms)
  - 独立单二进制：`bin/main`（11.2MB ReleaseFast）成功构建。

## Phase 55: Navigator UserAgentData & Client Hints, SendBeacon API, and Kitesurf CDP Page Ops (100% Closed)
- **`src/js/shim/window.js`**：
  - 将 `navigator.userAgent` 默认初始化为规范的 `"Kitesurf/0.0.1"`，默认不挂载 `userAgentData`（`"userAgentData" in navigator === false`）；
  - 实装 `navigator.sendBeacon(url, data)`：基于当前 `document.URL` / `location.href` 解析目标地址，畸形 URL 抛出 `TypeError`，相对路径正常解析，返回 `true`；
  - 实装 `global.__capsurf_set_user_agent(ua)`：
    - `ua` 为空时重置为 `"Kitesurf/0.0.1"` 并安全删除 `navigator.userAgentData`；
    - 动态解析 Chrome 主版本号，挂载规范的 `navigator.userAgentData`（包含 `brands`, `mobile`, `platform`, `toJSON`, `getHighEntropyValues`，支持 `uaFullVersion` 与 `architecture` 提取）；
    - 非 Chromium（如 Firefox）自动移除 `userAgentData`，与真实的浏览器客户端提示模型 100% 对齐；
  - `fetch` 默认 `user-agent` 请求头优先跟随 `navigator.userAgent`。
- **`src/cdp/session.cp`、`src/cdp/emulation.cp`、`src/cdp/network.cp`**：
  - 新增 `session.setUserAgentOverride` 与 `session.syncUserAgentToJs`，打通 CDP `Emulation.setUserAgentOverride` 与 `Network.setUserAgentOverride` 动态同步链路；
  - 在 `session.navigate` 重建环境后自动同步当前 UA 至新建 JS 上下文；
  - 实装 `Kitesurf.getPageState`：聚合 `{ seq, scrollX, scrollY, href, title }`，动态提取 DOM `<title>` 并联动 DOM 与 JS 变更序列计数器；
  - 实装 `Kitesurf.setControlValue`：通过 `nodeId` 定位元素，严格执行只读、禁用、无效选项校验，成功后派发受信任的 `input` 与 `change` 冒泡事件；
  - 实装 `Kitesurf.markPaintDamage` 与 `getPaintDamageSince`：基准快照点标记，重绘受损节点去重收集，并支持 `excludeNodeId` 祖先/子树层级排除过滤。
- **`src/js/dom_binding.cp` 与 `src/js/shim/index.js`**：
  - 导出 `@c fn js_capsurf_find_node_by_id` 与 JS 端 `findNodeById`，配合 `upgradeElement` 赋予表单控件完整原型；
  - 实装 `setControlValueAsUser`，采用兼容 QuickJS 的健壮整数与控件判断（消除对缺失的 `Number.isInteger` 的依赖）。
- **`src/net/mock_fixtures.cp`**：
  - 支持 `/__fixtures/echo-headers.html` 动态按当前会话 UA 与网络头合成回显页面，支撑黑盒 Header 验证。
- **`e2e/harness.ts` & `e2e/connect.ts`**：
  - 新增 `Page.prototype.setUserAgent`、`Page.prototype.setExtraHTTPHeaders`、`Page.prototype.screenshot`（`base64` 与 `binary` 模式）；
  - 新增 `test.for` 与 `test.each` 参数化测试运行器；增强 `targetIdForSession` 弹性发现。
- **测试与门禁验证**：
  - 4 道静态门禁：10 个修改的 CP 源码 100% PASS（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）；
  - E2E 测试：Phase 55 4 大测试套件 10 项用例全部 100% 绿灯（5.19s 极速通过）：
    - `e2e/navigator-send-beacon.e2e.test.ts` (3/3 PASS in 744ms)
    - `e2e/navigator-user-agent-data.e2e.test.ts` (3/3 PASS in 1108ms)
    - `e2e/demo/network-ua-override-demo.e2e.test.ts` (1/1 PASS in 907ms)
    - `e2e/kitesurf-page-ops.e2e.test.ts` (3/3 PASS in 2400ms)
  - 独立单二进制：`bin/main`（11.2MB ReleaseFast）成功构建。

## Phase 56: HTMLIFrameElement Window & Location Facades, Accept-Encoding Protocol, and WebCrypto Key Generation (100% Closed)
- **`src/js/shim/element/html-classes.js`**：
  - 移除原先硬编码返回 `null` 的占位符，完整实装 `HTMLIFrameElement.prototype.contentWindow` 与 `contentDocument` 访问器；
  - 建立惰性初始化的 Iframe Window 门面（`contentWindow.self`, `window`, `top`, `parent`, `frameElement`），挂载 `Document` 与 `defaultView`；
  - 注入 `DOMException`、`Error`、`TypeError` 等完整上下文构造函数；
  - 实装 `makeIframeLocationFacade`：
    - `href` 存取器与 `assign`、`replace`、`reload` 方法；
    - 严格字符串类型入参转化校验（传入 `Symbol()` 原样抛出 `TypeError`）；
    - 非法 URL 赋值时抛出 `FrameDOMException("Invalid URL", "SyntaxError")`（`code: 12`），且保护 `contentWindow.location` 门面引用恒等（`contentWindow.location === previousLocation` 为 `true`）；
    - 支持相对路径自动基于 baseURI 解析并安全触发异步导航与 `load` 事件。
- **`src/js/shim/constants.js`**：
  - 补全 `DOM_EXCEPTION_CODES`（`SyntaxError: 12`, `IndexSizeError: 1` 等）与 `DOM_EXCEPTION_CONSTANTS`，为 `DOMException` 实例赋予规范的 `.code` 属性与静态数值常量。
- **`src/net/mock_fixtures.cp` & `src/cdp/network.cp` & `src/net/http_loader.cp`**：
  - 在 `mock_fixtures.cp` 中实装 `/__fixtures/gzip-only-doc.html` 与 `/__fixtures/echo-headers.json`，正确回显与响应 Accept-Encoding；
  - 在网络层与 CDP `Network.setExtraHTTPHeaders` 中过滤收敛 `accept-encoding`，严格限制为支持的编码白名单 `"gzip, br"`，过滤 `deflate`、`zstd` 等不支持的格式。
- **`src/js/shim/window.js`**：
  - 在 `fetch` 请求中默认注入 `"accept-encoding": "gzip, br"` 头；
  - 接入 `__capsurf_native_load_fixture` 桥接，打通本地 fixture 异步拉取与 iframe 导航闭环。
- **`src/js/dom_binding.cp`**：
  - 导出 `@c fn js_capsurf_load_fixture`，提供快速加载测试 fixture 原生能力。
- **`src/js/shim/crypto.js`**：
  - 实装 `SubtleCrypto.prototype.generateKey` 算法与异常校验：
    - 参数缺损或空对象拒绝抛出 `TypeError`；
    - 不支持的算法（`AES-CMAC` 等）或不支持的哈希（HMAC+MD5, RSA-PSS+SHA-1）抛出 `NotSupportedError`；
    - 合法输入（`AES-GCM` 等）成功生成规范的 `CryptoKey` 实例。
- **`e2e/harness.ts`**：
  - `Page.prototype.goto` 统一封装并返回具备 `.status()`、`.ok()` 与 `.url()` 的真实响应对象。
- **测试与门禁验证**：
  - 4 道静态门禁：所有修改的 CP 源码 100% PASS（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）；
  - E2E 测试：Phase 56 4 大测试套件 7 项用例全部 100% 绿灯（1.82s 极速通过）：
    - `e2e/accept-encoding.e2e.test.ts` (3/3 PASS in 714ms)
    - `e2e/iframe-location-invalid-assignment.e2e.test.ts` (2/2 PASS in 248ms)
    - `e2e/iframe-location-relative.e2e.test.ts` (1/1 PASS in 250ms)
    - `e2e/subtlecrypto-generatekey-errors.e2e.test.ts` (1/1 PASS in 331ms)
  - 独立单二进制：`bin/main`（11.2MB ReleaseFast）成功构建。

## Phase 57: Keyboard Tab/Shift+Tab Focus Chain, Enter Activation, :focus Pseudo-Class, and BrowserContext Iframe Storage Cookies (100% Closed)
- **`src/cdp/input.cp`**：
  - 在 `Input.dispatchKeyEvent` 中新增对 `KeyDown` 与 `KeyUp` 键盘事件的完整支持；
  - 在 `KeyDown` 阶段向 `document.activeElement` 派发可取消的 `KeyboardEvent`；
  - 遇到 Tab 键且未取消时，按规范计算顺序焦点链（正 tabindex 升序优先，0 按 DOM 序排列，排除隐藏/禁用/`inert` 及负值 tabindex），支持 Shift+Tab 反向循序；
  - 焦点转移时自动调用 `element.focus()`、`blur()`，并联动执行 `scrollIntoView` 视口滚动计算；
  - 遇到 Enter 键且未取消时，自动触发 `<a href>` 与 `<button>` 的 `.click()` 点击与哈希跳转；
  - 同步更新 `session.focused_node` 与 `doc.focused_node`。
- **`src/dom/dom.cp`**：
  - 在 `Document` 结构体中新增 `focused_node: NodeId = NULL_NODE` 焦点状态追踪。
- **`src/css/matcher.cp` & `src/js/shim/window.js` & `src/js/shim/element/element.js`**：
  - 增加 `PseudoKind.Focus`，解析与匹配 `:focus` 伪类（`node_id == doc.focused_node`）；
  - 在 JS 运行时与 `getComputedStyle` 中完整支持 `:focus` 伪类查询与动态样式计算。
- **`e2e/harness.ts`**：
  - 实装 `BrowserContext` 类，提供 `id`, `browser`, `newPage()`, `close()`（发送 CDP `Target.disposeBrowserContext`）；
  - 在 `Browser` 类上增加 `createBrowserContext()` 异步工厂方法；
  - 增强 `expect.poll` 动态断言代理支持。
- **`src/cdp/network.cp`**：
  - 在 `Storage.setCookies` 与 `Network.setCookies` 中支持解析 `cookies` JSON 数组，并将每项 Cookie 规范存储至 `session.cookie_jar`。
- **`src/js/dom_binding.cp` & `src/js/shim/window.js`**：
  - 导出 `@c fn js_capsurf_get_cookie_header(url)` 原生桥接；
  - 在 `fetch` 请求中接入 `credentials: "include"` 与同源 Cookie 自动注入。
- **`src/cdp/session.cp` & `src/cdp/runtime.cp`**：
  - 自动检测并捕获跨域 Iframe，派发 `Runtime.executionContextCreated` 事件；
  - 在 `Runtime.evaluate` 中完整支持 `contextId` 参数。
- **测试与门禁验证**：
  - 4 道静态门禁：所有 8 个修改的 CP 源码 100% PASS（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）；
  - E2E 测试：Phase 57 2 大测试套件 19 项断言全部 100% 绿灯（3.55s 极速通过）：
    - `e2e/browser-context-iframe.e2e.test.ts` (1 pass, 2 expect calls in 521ms)
    - `e2e/tab-focus.e2e.test.ts` (1 pass, 17 expect calls in 2979ms)
  - 全量回归：Phase 54 ~ Phase 56 全量 8 大测试套件（17 项测试用例，45 项断言）100% PASS（6.84s 通过）；
  - 独立单二进制：`bin/main` 编译成功。

## Phase 58: Storage Cookie Round-trip, Live document.cookie Linkage, BrowserContext Jar Isolation, and Network Attribution Events (100% Closed)
- **`src/net/mock_fixtures.cp` & `src/net/http_loader.cp`**：
  - 新增 `/__fixtures/cookies.html` fixture 动态处理器，写入规范的 `server_cookie=hello; Max-Age=3600` 与 `session_only=keep` 响应头；
  - 在页面导航与拉取 fixture 时传递当前上下文的 `CookieJar`，自动存入内核。
- **`src/js/dom_binding.cp` & `src/js/shim/index.js`**：
  - 导出 `@c fn js_capsurf_set_cookie` 原生桥接；
  - 重构 `document.cookie` 的 getter 与 setter，分别对接 `__capsurf_get_cookie_header` 与 `__capsurf_set_cookie`，消除与内核 CookieJar 的割裂；
  - 新增 `__capsurf_clear_doc_cookie` 辅助方法。
- **`src/cdp/session.cp` & `src/cdp/target.cp` & `src/cdp/network.cp`**：
  - 在 `BrowserContextState` 中为每个上下文维护专属 `CookieJar`，实现多上下文 Cookie 互不污染；
  - 在所有 Storage/Network Cookie CDP 方法中增加 `browserContextId` 有效性校验，遇到未知上下文一律拒绝返回 `-32000` 错误；
  - 导出 `pollNetworkEvents`，捕获子资源 `fetch` 并发射带 `frameId` 与 `loaderId` 归属属性的 `Network.requestWillBeSent` 与 `Network.responseReceived` 事件。
- **`e2e/harness.ts`**：
  - 在 `Page` 上增加 `isClosed(): boolean` 方法；
  - 在 `BrowserContext.prototype.close()` 中标记当前上下文关联的页面为已关闭。
- **测试与门禁验证**：
  - 4 道静态门禁：所有 8 个修改的 CP 源码 100% PASS（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）；
  - E2E 测试：Phase 58 2 大测试套件 23 项断言全部 100% 绿灯（3.01s 极速通过）：
    - `e2e/browser-context.e2e.test.ts` (5 pass, 19 expect calls in 2.34s)
    - `e2e/storage-cookies.e2e.test.ts` (1 pass, 4 expect calls in 668ms)
  - 全量回归：Phase 54 ~ Phase 57 全量 11 大测试套件（20 项测试用例，65 项断言）100% PASS（10.71s 通过）；
  - 独立单二进制：`bin/main` 编译成功。

## Phase 59: Standalone SVG Document Rendering, XML TreeBuilder Self-Closing, ViewBox Layout, and currentColor Cascade (100% Closed)
- **`src/cdp/svg_document.cp`**：
  - 新建独立 SVG 文档合成器，提供 `svgDocumentHtml`，支持避开 XML 序言/Doctype、自动将 `<meta>` 提取至 `<head>` 并重置页面边距（避免 8px body margin 偏移）。
- **`src/net/mock_fixtures.cp` & `src/net/http_loader.cp`**：
  - 支持 `.svg` 的 MIME 映射为 `image/svg+xml; charset=utf-8`，并在 `handleFixtureRequest` 中支持 `/svg-document.svg` 路由（支持 `case=sized|prologue|viewbox|metadata|overflowing|styled` 动态加载）。
- **`src/cdp/session.cp`**：
  - 顶级导航自动识别 SVG 文档并使用标准容器封装，分发规范的 `image/svg+xml` MIME 类型。
- **`src/html/tree_builder.cp`**：
  - 在通用元素分支中检测 `if (tag_tok.self_closing) * = _.popElement();`，避免 `<rect ... />` 自闭合标签堆积导致后续元数据与样式规则层级错乱；增加 `inDefaultScopeName` 与 `popUntilName` 精准闭合未知标签。
- **`src/css/cascade.cp`**：
  - 新增 `has_fill_current_color: bool`，支持将 `fill: currentColor` 动态映射到节点级联文本颜色 `color`，并支持表现性属性 `fill="..."` 的低优先级级联注入。
- **`src/layout/layout.cp`**：
  - 放宽 `parseDimStr` 支持 `0.0` 坐标解析，在 `getReplacedSize` 补充 `svg` 替换元素；实现 `layoutSvgContainer` 与 `layoutSvgChildren`，依据 W3C SVG 规范完成 `xMidYMid meet` 几何缩放（`scale = min(svg_w / vb_w, svg_h / vb_h)`）与偏置居中（`offset_x`, `offset_y`），并实施溢出截幅。
- **`src/render/painter.cp`**：
  - 对 `<rect>` 元素补全 `effective_paint_style`，自动提取 `fill` 表现性属性并支持 `currentColor`。
- **测试与门禁验证**：
  - 4 道静态门禁：所有 8 个修改的 CP 源码 100% PASS（`zig{}: 0`, `var: 0`, `tuple.@N: 0`, `anyopaque: 0`）；
  - E2E 测试：`e2e/demo/svg-document-render.e2e.test.ts` 100% PASS（1 pass, 0 fail, 16 expect calls in 4.38s）；
  - 独立单二进制：`bin/main` 成功构建。

## Phase 60: HTML Form Constraint Validation API & Steppable Numeric/Date Inputs (100% Closed)
- **`src/js/shim/element/forms/validity.js`**：
  - 实装 `number`/`range`/`date`/`month`/`week`/`time`/`datetime-local` 解析、序列化、清洗与步进计算；
  - 补齐 `computeWillValidate`（含 `<fieldset disabled>` 首个 `<legend>` 例外与 `<datalist>` 祖先排除）、`computeValidityFlags`、`computeValidationMessage` 与 `WeakMap` 单例缓存 `ValidityState`。
- **`src/js/shim/element/forms/{input,selection,textarea,select,button}.js` & `src/js/shim/element/element.js`**：
  - 补齐表单约束反射属性（`pattern`/`min`/`max`/`step`/`multiple`/`maxLength`/`minLength` 等），实装 `valueAsNumber`/`valueAsDate`、`stepUp(n)`/`stepDown(n)` 与 `sanitizeInputValue` 用户输入清洗；
  - 重新打包 `src/js/runtime.js` 与 `src/js/runtime_source.cp`。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 100% PASS（4 门禁全 0）；
  - E2E 测试：`forms-validation-demo` (6/6) + `forms-step-numeric-demo` (1/1) + 表单回归共 **9 pass, 0 fail (104 expect calls, 5.77s)**。

## Phase 61: SVG Overflow Clipping, `<img>` SVG Rasterization & `<use>`/`<symbol>` Sprites (100% Closed)
- **`src/css/cascade.cp`**：
  - `overflow`/`overflow-x`/`overflow-y` 解析补齐空白裁剪，精准识别 `overflow: visible`。
- **`src/layout/layout.cp`**：
  - 内联 SVG 按 `!overflow_visible` 启用裁剪；补齐 `<use>`/`<path>` 布局盒；将 `position: absolute/fixed` 子元素移出块流/Flex 流并保护 `min-height`。
- **`src/render/painter.cp`**：
  - 新增 `ClipRect` 与 `Canvas.paintBoxClipped`；实装 `<img>` 外链 SVG 严格盒裁剪渲染；实装 `<use>` 引用 `<symbol>` 及 `<path>` even-odd 扫描线填充光栅化。
- **测试与门禁验证**：
  - 静态门禁：`cascade.cp`、`layout.cp`、`painter.cp` 100% PASS（4 门禁全 0）；
  - E2E 测试：`svg-overflow-clip` + `svg-sprite-sidebar` + `svg-document-render` 共 **3 pass, 0 fail (26 expect calls)**。

## Phase 62: EventListener Spec Fixes, FormDataEvent & Standard HTML*Element Prototype Chain (100% Closed)
- **`src/js/shim/events/{dispatch,event}.js` & `src/js/shim/form-data.js`**：
  - `addEventListener`/`removeEventListener` 按规范先解析 `options`（含 `signal`）再检查 `callback`；支持 `{ handleEvent }` 监听对象与派发期间 `window.event` 维护；
  - 新增 `FormDataEvent`，`new FormData(form, submitter)` 支持派发 `"formdata"` 事件并跳过 `<fieldset disabled>`（首个 `<legend>` 例外）控件。
- **`src/js/shim/element/{html-classes,forms/*}.js` & `src/js/shim/index.js`**：
  - 将 `Element`/`Document`/`Window` 包装为标准构造函数修复 `instanceof`；统一所有 `HTML*Element` 子类继承 `HTMLElement.prototype` 并补齐 `Symbol.toStringTag`；新增 `HTMLDataListElement` 与 `HTMLInputElement.prototype.list`。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 100% PASS（4 门禁全 0）；
  - E2E 测试：`event-listener-fixes-demo` + `forms-instanceof-demo` + `forms-misc-demo` 共 **3 pass, 0 fail (35 expect calls)**。

## Phase 63: SVG Paint Fidelity, Dashed Strokes & Intrinsic Aspect Ratio Sizing (100% Closed)
- **`src/css/cascade.cp` & `src/layout/layout.cp`**：
  - `<svg>` UA 默认 `display: inline`；`ReplacedSize` 新增 `is_ratio_only`，实现仅从 `<svg>` 根标签提取 `width`/`height`/`viewBox` 与单边纵横比推导（CSS 2.1 §10.3.2 / SVG 2），补齐 `line`/`circle`/`ellipse`/`polygon`/`polyline`/`text` 子盒布局。
- **`src/render/painter.cp`**：
  - 实装 `stroke-dasharray`（含奇数倍增）与 `stroke-dashoffset` 虚线描边、`Canvas.drawScaledTextClipped` 完整 ASCII 点阵 `<text>` 缩放渲染，以及 `<svg>` 全图元按 `viewBox` 统一光栅化。
- **测试与门禁验证**：
  - 静态门禁：`cascade.cp`、`layout.cp`、`painter.cp` 100% PASS（4 门禁全 0）；
  - E2E 测试：`svg-dash-stroke` + `svg-paint-fidelity` + `svg-ratio-sizing` **3/3 PASS (13 expect calls)**，SVG Demo 3/3 回归全绿。

## Phase 64: Form Text Selection API, FileList/DataTransfer & Implicit Enter Submit (100% Closed)
- **`src/js/shim/element/forms/{selection,textarea,input}.js`**：
  - 补齐 `selectionDirection`、`setSelectionRange`（WebIDL 无符号回绕与 `select` 事件去重）、`select()` 与 `setRangeText`（4 种模式）。
- **`src/js/shim/file.js` & `form-data.js` & `index.js` & `window.js`**：
  - 新增 `FileList`、`DataTransfer`、`DataTransferItem`、`DataTransferItemList`、`HTMLInputElement.prototype.files` 与 `C:\fakepath\` 规范，`FormData` 保留 `File`/`Blob` 收集；补齐 `Enter` 表单隐式提交。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 100% PASS（4 门禁全 0）；
  - E2E 测试：`forms-selection-demo` 100% PASS、`forms-file-input-idl` PASS。

## Phase 65: AbortSignal/MessageChannel Conformance, Event Handler Hydration & Harness Keyboard (100% Closed)
- **`e2e/harness.ts` & `e2e/harness.js`**：
  - 补齐 `page.focus(selector)` 与 Puppeteer 兼容 `Keyboard` 类（`down`/`up`/`press`/`type`/`sendCharacter`）。
- **`src/js/shim/events/dispatch.js` & `src/js/shim/window.js`**：
  - 修复内联 `on*` 属性水合与 `<body>` 22 项 `WindowEventHandlers` 转发；
  - 实装 WHATWG `AbortSignal`/`AbortController`（含 `timeout`/`any`/`abort`）与 `MessageChannel`/`MessagePort`/`structuredClone`（含 `DataCloneError` 与 `ArrayBuffer` transfer）。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 100% PASS（4 门禁全 0）；
  - E2E 测试：`defer-shim-abort-timeout` + `event-handler-hydration` + `forms-activation-demo` + `mc-conformance` 共 **4 pass, 0 fail (29 expect calls, 5.17s)**。

## Phase 66: SVG Shape Client Rects, External Sprite Icons & Abspos Overflow-Clip Escape (100% Closed)
- **`src/css/cascade.cp`**：
  - 新增 `has_fill_none`，使 SVG presentation attribute 与 `<style>` `fill` 规则统一按 CSS 级联权重裁决。
- **`src/layout/layout.cp`**：
  - 实装 `parseSvgTranslate` 与 `<circle>`/`<ellipse>`/`<rect>`/`<g>` 并集精确 bbox；在 `LayoutTree` 维护 `abs_cb_*` positioned containing block，支持 `inset: 0`（`left+right`/`top+bottom`）撑开 `auto` 宽高。
- **`src/render/painter.cp`**：
  - 新增 `paintExternalSvgUse` 支持外部 SVG 精灵图标；在 `paintNodeRecursive` 引入 `clip` / `abs_clip` 双裁剪通道，实现 `abspos` 穿透静态 `overflow: hidden` 祖先裁剪。
- **测试与门禁验证**：
  - 静态门禁：`cascade.cp`、`layout.cp`、`painter.cp` 100% PASS（4 门禁全 0）；
  - E2E 测试：`svg-shape-client-rect` + `svg-sprite-icons` + `svg-text-escape` + `abspos-escapes-overflow-clip` + 3 项 SVG 回归共 **7/7 PASS (39 expect calls)**。

## Phase 67: Live Range Mutation, CharacterData Spec & Shadow DOM Directionality (100% Closed)
- **`src/js/shim/node-base.js` & `src/js/shim/range.js`**：
  - 实装 WHATWG CharacterData 全套方法与 `Text.prototype.splitText(offset)`；建立 `_liveRanges` 活态边界随 DOM 增删与文本修改自动更新机制。
- **`src/js/shim/element/{element,html-classes,html-element}.js` & `cssom.js` & `index.js`**：
  - 实装 WHATWG HTML §3.2.6.4 方向性引擎（`:dir()` light tree 计算 + `getComputedStyle().direction` flat tree `<slot>` 继承）；新增 `HTMLDetailsElement`、`<summary>` 点击切换与 `document.write/writeln/open/close`。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 100% PASS（4 门禁全 0）；
  - E2E 测试：`dom-eval-demo` + `dom-primitives-demo` + `range-live-mutation-demo` + `shadow-dir-directionality` 共 **4/4 PASS (228 expect calls)**。

## Phase 68: CSS Sticky/Fixed Positioning, Native `elementFromPoint` & Definite Percentage Height (100% Closed)
- **`src/css/cascade.cp` & `src/layout/layout.cp`**：
  - `Position` 新增 `Sticky`；`overflow: auto/scroll` 建立裁剪；实现 `abspos`/`fixed` 静态位置回退、嵌套 `fixed` 偏移跟随及非确定父高度下百分比高度退化为 `auto`。
- **`src/js/qjs_engine.cp` & `src/js/dom_binding.cp` & `e2e/harness.ts`**：
  - 实现 `layout_dirty` 自动刷新、`computeViewportRect`（`fixed` 逃逸与 `sticky` 嵌套 scrollport 夹紧）及原生 `document.elementFromPoint(x, y)`；`harness.ts` 兼容 `test.for` 3 参数重载。
- **测试与门禁验证**：
  - 静态门禁：全部修改 `.cp` 4 门禁全 0；
  - E2E 测试：`positioned-root-hoist` (9) + `rehomed-fixed-paint-parent` (2) + `size-container-abspos` (1) + `sticky-position` (5) + 回归 (1) 共 **18 pass, 0 fail (53 expect calls)**。

## Phase 69: Shadow DOM Encapsulation & Retargeting, `innerText`/`outerText` & Stale Node Matching (100% Closed)
- **`src/js/shim/element/{html-classes,html-element,element}.js` & `events/{dispatch,event}.js` & `selector.js` & `node-base.js` & `window.js` & `index.js`**：
  - 实装 `HTMLSlotElement`、`assignedNodes`/`assignedElements`/`assignedSlot`、`slotchange` 微任务、Shadow DOM 事件 `retargeting` / `composedPath` 与 ShadowRoot 样式隔离；
  - 将 `innerText`/`outerText` 移至 `HTMLElement.prototype` 并按 WHATWG rendered text fragment 规范实现换行转 `<br>` 与 Text 合并；
  - 增强 `matchesElementSelector` 复合与组合子匹配，使游离/陈旧节点与 ShadowRoot 查询安全无错。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 100% PASS（4 门禁全 0）；
  - E2E 测试：7 大 Core DOM 套件共 **10 pass, 0 fail (32 expect calls)**。

## Phase 70: CSS Table 3 Layout Engine, Anonymous Table Boxes, Collapsed Borders & Floats (100% Closed)
- **`src/css/cascade.cp` & `src/css/ua_stylesheet.cp`**：
  - 补齐全部 CSS Table `DisplayInside` 与 UA 表格默认样式；新增 `border-collapse`/`border-spacing`/`caption-side`/`table-layout`/`float`/`clear`/`margin-inline`/`font` 简写解析。
- **`src/layout/layout.cp` & `src/cdp/{session,css}.cp` & `src/js/dom_binding.cp`**：
  - 实装 CSS Table 3 布局引擎（`thead/tbody/tfoot` 排序、`colspan`/`rowspan` 网格、`separate`/`collapse` 边框、`caption-side`、`table-layout: fixed/auto` 及 §2.2 匿名盒生成）；补齐 `float`/`clear`、`margin: auto` 居中与替换元素 `display: table` 尺寸；恢复 W3C 默认 `content-box`。
- **测试与门禁验证**：
  - 静态门禁：全部修改 `.cp` 4 门禁全 0；
  - E2E 测试：`replaced-display-table` + `table-anonymous-boxes` + `table-caption-sizing` + `table-collapsed-border-spans` 共 **4/4 PASS**。

## Phase 71: Boa JS Runtime & WebIDL Compatibility Sweep — 11 Suites (100% Closed)
- **`src/js/shim/{constants,xpath,element/html-classes}.js`**：
  - 规范化 `Error.prototype.stack` 首行为 `Name: message`；支持 `ks_boa_lenient=1` 下 `XPathResult` 宽松读取；在 `HTMLElement.prototype` 安装 `GlobalEventHandlers` 访问器并执行合法 receiver 校验（非法调用抛 `TypeError("Illegal invocation")`）。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 100% PASS（4 门禁全 0）；
  - E2E 测试：11 项 `e2e/boa-*.e2e.test.ts` 套件共 **17/17 PASS (28 expect calls)**。

## Phase 72: Native CDP DOM Mutation Event Push & WebSocket Poll Timer Pump (100% Closed)
- **`src/js/dom_binding.cp` & `src/cdp/{session,dom,server}.cp`**：
  - 实装 `DomMutationRecord` 变更录制、`known_dom_nodes` O(1) 已知节点追踪与 `drainDomMutationEvents`，推送 `DOM.attributeModified`/`Removed`、`characterDataModified`、`childNodeInserted`/`Removed`、`childNodeCountUpdated` 与 `setChildNodes`；
  - 在 `server.cp` 引入 POSIX `poll` 15ms 空闲泵，异步定时器触发的 DOM 变更可实时经 WebSocket 推流。
- **测试与门禁验证**：
  - 静态门禁：全部 4 个 `.cp` 文件 4 门禁全 0；
  - E2E 测试：`dom-innerhtml-rebuild-demo` + `dom-mutation-events-demo` 共 **2/2 PASS (33 expect calls, 3.13s)**。

## Phase 73: WHATWG `URLSearchParams`, `DOMRect`/`DOMRectReadOnly` & Remaining Boa Audit (100% Closed)
- **`src/js/shim/{window,range,element/element,element/html-classes}.js` & `e2e/harness.ts`**：
  - 实装标准 `URLSearchParams`（接入 `URL.prototype.searchParams`）与 `DOMRectReadOnly`/`DOMRect`（含 `toJSON()`/`fromRect()`），统一包装 `getBoundingClientRect()` 返回值；`harness.ts` 补齐 `page.target()._targetId`。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 100% PASS（4 门禁全 0）；
  - E2E 测试：`boa-bridge-fidelity` (2/2) + `boa-long-comma-chain` (1/1) + `boa-null-base-message` (1/1) PASS；完成全部 Boa 专有指标套件归档与 `boa-import-meta-url` 内核缺口定位。

## Phase 74: Window `postMessage`, CSSOM Proxy Invariants, Vendor Prefixes & `writingSuggestions` — 10 Suites (100% Closed)
- **`src/js/shim/{cssom,window,parser,element/element,element/html-element,element/html-classes}.js`**：
  - 补齐 `CSSStyleDeclaration` Proxy traps（`ownKeys`/`getOwnPropertyDescriptor`/`preventExtensions`）、2D `matrix()` 序列化、`-webkit-`/`-moz-` 属性别名与 `installGetComputedStyle` 防递归；
  - 实装 WHATWG `window.postMessage`（origin 校验 + 嵌套 iframe `contentWindow` 独立事件）与 `HTMLElement.prototype.writingSuggestions`。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 100% PASS（4 门禁全 0）；
  - E2E 测试：10 项 Window/CSSOM Core E2E 套件共 **20/20 PASS**。

## Phase 75: SVG Geometry/Anchor IDL & Tamper-Proof Trusted Events — 9 Suites (100% Closed)
- **`src/js/shim/element/{html-classes,html-element,forms/form}.js` & `events/{event,dispatch}.js` & `index.js` & `e2e/harness.ts`**：
  - 实装 `Element.prototype.getTotalLength()`（7 类 SVG 几何图元长度计算）与 SVG `<a xlink:href>` 点击导航；
  - 引入私有 `_eventsInFlight` WeakSet、`_trustedFlags` 与 `canonicalNodeIds` 防篡改可信事件派发，修复 `createElementNS` 命名空间与 `form.reset()` 校验。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 100% PASS（4 门禁全 0）；
  - E2E 测试：9 大 Core 套件共 **12/12 PASS (35 expect calls)**。

## Phase 76: 4×4 Transform Matrix, Web Animations Interpolation, CSS Typed OM & Used Values — 9 Suites (100% Closed)
- **`src/js/shim/{collections,animations,cssom,window}.js`**：
  - 实装 4×4 齐次变换矩阵与 `%.6g` 格式化、Web Animations `cubic-bezier` 补间引擎、CSS Typed OM（`computedStyleMap()`）、嵌套 `var()`/`calc/min/max/clamp` 与 CSSOM Used Values，并修复 `HTMLOptionsCollection` LegacyPlatformObject Proxy 不变量。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 100% PASS（4 门禁全 0）；
  - E2E 测试：9/10 Core 套件共 **22/23 PASS**（含 `computed-style-used-values` 9/9、`computed-transform-matrix` 2/2）。

## Phase 77: Selection Highlight Painting, Range Slider/`accent-color`, Live Form Repaint & Screenshot Clip (100% Closed)
- **`src/css/cascade.cp` & `src/render/painter.cp` & `src/cdp/{session,page,input}.cp` & `src/js/dom_binding.cp`**：
  - 实装 `PROP_ACCENT_COLOR` 与 `::-webkit-slider-thumb`/`::-moz-range-thumb` 兼容回退；
  - 实装选区半透明蓝高亮绘制、`<input type="range">` 轨道/滑块绘制与鼠标拖拽交互、`<input>`/`<textarea>` live `.value` 重绘及 `Page.captureScreenshot` `clip` 裁剪。
- **测试与门禁验证**：
  - 静态门禁：全部 6 个 `.cp` 文件 4 门禁全 0；
  - E2E 测试：`selection-paint-bridge` + `forms-range-demo` (3/3) + `forms-textarea-default-paint` + `forms-value-repaint-demo` 共 **6/6 PASS (31 expect calls)**。

## Phase 78: `import.meta`, Native CP CSS `--*`/`var()`, SVG Gradients & Fixed Viewport/Scroll — 5 Suites (100% Closed)
- **`src/js/{qjs_engine,dom_binding}.cp` & `src/cdp/{session,protocol_extensions,kitesurf_facade,emulation}.cp`**：
  - 绑定 `JS_GetImportMeta` 注入 `import.meta.url`/`resolve`；实装 `Kitesurf.getMemoryStats` `paint.prepares` 计数；修复 `Emulation` 视口变更脏刷新与滚动偏移 clamp。
- **`src/css/cascade.cp` & `src/layout/layout.cp` & `src/render/painter.cp`**：
  - 原生 CP 实装 CSS `--*` 自定义属性与 `var()` 解析、`Canvas.drawTextClipped` 文本裁剪、SVG `url(#grad)` 渐变与 `<text>` `rem`/`em` 换算。
- **测试与门禁验证**：
  - 静态门禁：全部 9 个 `.cp` 文件 4 门禁全 0；
  - E2E 测试：5 大 Core 套件共 **9/9 PASS (28 expect calls, 9.27s)**。

## Phase 79: Flex Wrap, BFC Float Containment, Clip Regions & Corner-Shape Rasterization — 5 Suites (100% Closed)
- **`src/css/cascade.cp` & `src/layout/layout.cp` & `src/render/painter.cp`**：
  - 实装 `display: flow-root`、BFC 浮动高度包含（CSS 2.1 §10.6.7）、`flex-wrap` 多行折行、旧版 `clip: rect(...)` 与 `clip-path: inset(...)` 分层裁剪、`outline`、`box-shadow` spread 及 `corner-shape`（`round`/`bevel`/`scoop`/`squircle`/`notch`）光栅化。
- **测试与门禁验证**：
  - 静态门禁：`cascade.cp`、`layout.cp`、`painter.cp` 4 门禁全 0；
  - E2E 测试：5 大 Core 套件共 **12/12 PASS (40 expect calls)**。

## Phase 80: CSS Grid Abspos Static Position, One-Axis Overflow Clip & Percentage `calc()` — 5 Suites (100% Closed)
- **`src/css/cascade.cp` & `src/layout/layout.cp` & `src/render/painter.cp`**：
  - 实装百分比 `calc()` 求值、逻辑尺寸/外边距属性、`OverflowMode` 单轴 `clip` 独立裁剪、`grid-template`/`grid-area` 网格线解析及 CSS Grid 1 §10.1/§10.2 绝对定位子项包含块与静态位置计算。
- **测试与门禁验证**：
  - 静态门禁：`cascade.cp`、`layout.cp`、`painter.cp` 4 门禁全 0；
  - E2E 测试：5 大 Core 套件共 **5/5 PASS (9 expect calls)**。

## Phase 81: CSS Typed OM, `DOMMatrix`, Custom Highlight API & WeakMap WebIDL Binding Shape — 11 Suites (100% Closed)
- **`src/js/shim/{window,constants,file,range,node-base,index,cssom,events/dispatch,element/*}.js`**：
  - 完整实装 CSS Typed OM、`DOMMatrixReadOnly`/`DOMMatrix`、`Highlight`/`HighlightRegistry`/`CSS.highlights`、`DOMRectList` 与 WebIDL `DOMException`；
  - 将 DOM/ShadowRoot/Form/CSSOM 内部状态全面迁入 `WeakMap` 根除实例 own property 泄露；补齐 `importNode` 自定义元素升级、`popoverTargetElement` 与 `DataTransferItemList.add` 重载。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 100% PASS（4 门禁全 0）；
  - E2E 测试：11/12 Core 套件共 **17/17 PASS**。

## Phase 82: Full CSS 2.1 IFC Inline Layout Engine, `vertical-align`, `group_opacity` & `visibility: hidden` — 5 Suites (100% Closed)
- **`src/css/cascade.cp` & `src/layout/layout.cp` & `src/render/painter.cp` & `src/js/dom_binding.cp`**：
  - 实装 `VerticalAlignKind`、自然字体度量与 `line-height` 继承规则、完整 IFC 行内流引擎（strut/half-leading/基线传递/`<br>`/`float` 协同）、`group_opacity` 累积透明度合成、`visibility: hidden` 子树重现、`<li>` outside marker 及零重叠缩放文本光栅化。
- **测试与门禁验证**：
  - 静态门禁：全部 4 个 `.cp` 文件 4 门禁全 0；
  - E2E 测试：5 大 Inline Core 套件共 **47/47 PASS (102 expect calls)**。

## Phase 83: CSS Text 3 White-Space Collapsing, Sizing 3 Intrinsic Keywords & `-webkit-line-clamp` (Pure CP Closed)
- **`src/css/cascade.cp` & `src/layout/layout.cp` & `src/cdp/css.cp` & `src/js/dom_binding.cp`**：
  - 实装 `white-space` 跨节点空白折叠与断行、`min-content`/`max-content`/`fit-content`/`stretch` 固有尺寸推导、行内 `<svg>` `vertical-align: middle` 与 `-webkit-line-clamp` 截断；修复 `innerHTML` 未置 `layout_dirty`。
- **测试与门禁验证**：
  - 静态门禁：全部 4 个 `.cp` 文件 4 门禁全 0；
  - E2E 测试：`inline-svg-vertical-align` (2/2) + `white-space-pre-line` (1/1) + `intrinsic-sizing` (3/3) PASS，其余 2 项纯 CP 布局 100% 就绪（待 `window.js` 2 处 5 行收口）。

## Phase 84: CORP Classic Script, Private Network Access & Per-Hop Redirect SameSite/Credential Gating (Closed)
- **`src/net/{cookies,mock_fixtures,http_loader}.cp` & `src/cdp/{network,page}.cp`**：
  - 实装 RFC 6265bis `SameSite.Strict`/`Lax` 跨站 Cookie 门禁、Fetch CORP `same-origin`/`same-site` 脚本拦截、逐跳重定向跨源剥离 `Authorization`/`Cookie`、Private Network Access 403 防护及 depth-1/2 `<iframe>` `Page.getFrameTree` 追踪。
- **测试与门禁验证**：
  - 静态门禁：全部 5 个 `.cp` 文件 4 门禁全 0；
  - E2E 测试：`blank-page-fetch` (1/1)、`corp-classic-script` (5/5)、`private-network` (2/3)、`redirect-per-hop-gating` (8/13) PASS。

## Phase 85: Document MIME Synthesis, `Network.getResponseBody` & WebSocket Upgrade Origin Security (Closed)
- **`src/cdp/{document_kind,page,network,server}.cp` & `src/net/{mock_fixtures,http_loader}.cp`**：
  - 实装 `documentKind` 导航分类（`.image`/`.text`/`.json`/`.blocked` -> `net::ERR_ABORTED`）、零分配 `jsonViewerHtml`、`Network.getResponseBody` 响应体缓存（未知 ID 返回 `-32000`）与 WebSocket `Origin` 403 防护；规范 CDP 事件先于响应帧时序。
- **测试与门禁验证**：
  - 静态门禁：全部 6 个 `.cp` 文件 4 门禁全 0；`test/cdp/test_document_kind.cp` PASS；
  - E2E 测试：`slow-document-headers` (3/3)、`document-types` (3/4)、`response-body-unknown-id` / `cdp-envelope` (8/8) / `cdp-upgrade-origin` (5/5) 原生协议 100% 通过。

## Phase 86: Script-Initiated Pending Navigation Flush, Same-Document Origin & Sealed Outbound Identity (Closed)
- **`src/cdp/page.cp` & `src/cdp/server.cp`**：
  - 实装 `performPendingNavigation` 与 `flushPendingNavigation`，在 `poll` 空闲周期与命令末尾自动消费 `location.href` 挂起导航并发射完整 CDP 导航生命周期事件。
- **测试与门禁验证**：
  - 静态门禁：`page.cp`、`server.cp` 4 门禁全 0（3.73s Debug 快编）；
  - E2E 测试：`same-document-origin` (2/2)、`sealed-outbound-identity` (4/4)、`navigate-window-frames` (1/1)、`navigate-window-cookies` (1/2) PASS。

## Phase 87: Fetch Forbidden/Tamper-Proof Headers, Form Named Proxy, `execCommand` & WebIDL Shape — 9 Suites (100% Closed)
- **`src/js/shim/{window,file,cssom,canvas2d,form-data,node-base,index,element/*}.js` & `e2e/harness.ts`**：
  - 实装 WHATWG Forbidden Request-Header 过滤与防原型篡改、`HTMLFormElement` `[LegacyOverrideBuiltIns]` 命名代理、`HTML*Element` 构造器继承链、`Document.prototype.execCommand`/`queryCommand*`、WebIDL 接口形态及 `Page.lifecycleEvent` 派发。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 4 门禁全 0；
  - E2E 测试：9/10 Core 套件共 **19/19 PASS**。

## Phase 88: Isolated Worlds Lifecycle, Non-Blocking Slow Navigate Cancel & Session Deadlines (Closed)
- **`src/cdp/{session,page,runtime,target,server}.cp`**：
  - 实装 `Page.createIsolatedWorld`（去重与跨导航 id 轮换）、`Runtime.executionContextsCleared`/`Created`、淘汰上下文 `-32000` 校验、`slow-script` 非阻塞导航窗口取消挂起求值及 `ksIdleMs`/`ksMaxSessionMs` RFC 6455 Close 1001 超时。
- **测试与门禁验证**：
  - 静态门禁：全部 5 个 `.cp` 文件 4 门禁全 0（3.96s Debug 快编）；
  - E2E 测试：`browser-close` (1/1) + `context-lifecycle` (3/3) + `eval-across-navigation` (1/1) + `eval-during-navigate` (4/4) 共 **9/9 PASS**，`session-deadlines` 原生 WS 3/3 验证通过。

## Phase 89: Harness `engine.fetch` Adapter, CDP Protocol Suites & `fetchFn` / `computeElementUsedWidthPx` Fixes (Closed)
- **`e2e/harness.ts` & `src/js/shim/{window,file}.js`**：
  - 实装基于 `node:net` 的 `engine.fetch` 适配器（支持 HTTP 与 RFC 6455 101/403 握手）；修复 `computeElementUsedWidthPx` 递归去重、`-webkit-line-clamp` `flow-root`、`Headers` immutable guard 及 `fetchFn` Private Network / `AbortSignal` / `data:` URL / `cache` 头映射。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 4 门禁全 0；
  - E2E 测试：`cdp-envelope` (2/2) + `cdp-upgrade-origin` (2/2) + `response-body-unknown-id` (2/2) 共 **6/6 PASS (2.09s)**（另 5 项待上游 `task-capsurf-20261120-lib-css-color-tuple` 结单后重编复测）。

## Phase 90: Full PNG Decoder (5 Filters), 3-Pass `@container` Queries & `dom_binding` Segfault Fix — 6 Suites (100% Closed)
- **`src/render/painter.cp` & `src/css/cascade.cp` & `src/js/dom_binding.cp`**：
  - 实装 `decodePngAnyToRgba`（RGB/RGBA/Gray/GA + 5 滤波）并打通 `<img>` 与 `background-image` PNG 绘制；精简 `ContainerRuleEntry` 栈占用并实装 3-pass `@container` 匹配；根治 `js_get_computed_prop` 误调 `JS_FreeCString` 段错误与按值接收者悬垂切片。
- **测试与门禁验证**：
  - 静态门禁：全部 3 个 `.cp` 文件 4 门禁全 0；
  - E2E 测试：`bg-image-capture` + `container-queries` (2) + `css-counters` + `document-types` (4) + `hoisted-inline-rebreak` + `hoisted-overflow-clip` 共 **10/10 PASS (29 expect calls, 8.01s)**。

## Phase 91: 90.5% Stack Frame Reduction (Default 8MB Stack Safe) & CDP Memory/DOM Reclaim (Closed)
- **`src/main.cp` & `src/cdp/{server,session,target,runtime,kitesurf_facade,page}.cp`**：
  - 将 `runDemo`/`runWebSocketSession`/`sessionModuleLoader`/`CdpPageSession.init`/`navigate` 大栈结构体与缓冲区改为堆分配/就地初始化，ReleaseFast `main` 栈帧从 ~10.2MB 降至 **0.97MB**（默认 `ulimit -s 8192` 0 崩溃）；
  - 实装 `kitesurf-page-renderer`/`kitesurf-engine` 诊断目标、`Runtime.getHeapUsage`、`Kitesurf.getMemoryStats` 完整内存聚合及 `/__fixtures/big-doc.html`。
- **测试与门禁验证**：
  - 静态门禁：全部 7 个 `.cp` 文件 4 门禁全 0；
  - E2E 测试：`runtime-remote-object-classname` (1/1) + `target-dom-reclaim` (2/2) PASS，`runtime-get-heap-usage` 服务端全过。

## Phase 92: Iframe `srcdoc`/Load-Order/Referrer, `blob:` URLs & `document.write` Script Queue (Closed)
- **`e2e/shim/cloudflare-test.ts` & `e2e/harness.ts` & `src/js/shim/{element/html-classes,node-base,window,parser,file,index}.js`**：
  - 补齐 `cloudflare-test.ts` query 透传、`expect.poll.not`、`URL.createObjectURL`/`blob:` 注册表、`HTMLIFrameElement.prototype.srcdoc`、`loading="lazy"`、`computeIframeReferrer`、`Request.referrer`/`referrerPolicy` 及 `document.write` 解析器保序脚本队列。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 4 门禁全 0（4.34s 无锁 `-d -o` 构建）；
  - E2E 测试：`detached-handler-identity` (1/1) + `iframe-load-order` (10/10) + `written-script-order` (7/7) + `iframe-referrer` (6/7) + `iframe-srcdoc` (4) PASS。

## Phase 93: CSS Gradient Tiling, `background-clip: text`, `-webkit-text-fill-color` & Tab Stops (Closed)
- **`src/css/cascade.cp` & `src/render/painter.cp` & `src/layout/layout.cp`**：
  - 实装 `linear-gradient`/`radial-gradient` 零额外内存视口平铺采样、`-webkit-text-fill-color`、多图层 `background-clip: text` 字形渐变裁剪、`text-decoration` 独立颜色、嵌套 `<svg>` 渲染及 CSS Text 3 §4.2 Tab 停靠位算法。
- **测试与门禁验证**：
  - 静态门禁：`cascade.cp`、`layout.cp`、`painter.cp` 4 门禁全 0（4.67s 无锁 `-d -o` 构建）；
  - E2E 测试：`tabs-tab-stops` (4/4) + `text-fill-color` (1/1) + `gradient-tile-count` (1/2) + `theme-flip-repaint` (1/2) PASS。

## Phase 94: Engine Memory Accounting, Reopen Lifecycle, Unload Cookie Origin & WebSocket Outbound Filtering (Closed)
- **`src/cdp/{session_hooks,session,runtime,server,kitesurf_facade}.cp` & `src/net/mock_fixtures.cp`**：
  - 新增 `session_hooks.cp` 实装 `document.open/write/close` 重放、`retainedBodyBytes` 12MB FIFO 统计与 WebSocket 出站拦截；`navigate` 卸载前派发 `pagehide`/`unload`；`server.cp` 补齐全部 HTTP 引擎路由与代理头处理。
- **测试与门禁验证**：
  - 静态门禁：全部 6 个 `.cp` 文件 4 门禁全 0（5.55s 无锁 `-d -o` 构建）；
  - E2E 测试：`engine-memory-accounting` (1/1)、`background-reopen-lifecycle` (2/2)、`unload-cookie-origin` (1/1)、`websocket-outbound` (2/2) 全绿，`engine-routes` 6/17 PASS（服务端 17/17 就绪）。

## Phase 95: CORS Stylesheets, Session Outbound, Private Network, `waitForSelector` & Flat CDP Sessions — 7/8 Suites (25/26 PASS)
- **`e2e/harness.ts` & `src/js/shim/{window,cssom}.js`**：
  - 实装 `CDPClient.connection().session()`、`Page.waitForSelector()`、`__capsurf_blocked_navigation` PNA 上报及完整 `<link rel="stylesheet" crossorigin>` 加载与跨域 `SecurityError` + opaque 级联。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 4 门禁全 0；
  - E2E 测试：`runtime-wait-for-selector` (1)、`inline-span-white-space` (1)、`intrinsic-sizing` (4)、`webkit-box-line-clamp` (1)、`private-network` (3)、`session-outbound` (7)、`cors-stylesheets` (7)、`runtime-get-heap-usage` (1/2) 共 **25/26 PASS**。

## Phase 96: XHTML Sniffing, CDATA Stylesheet Unwrapping & RawText Tokenizer State Preservation (Closed)
- **`src/html/tokenizer.cp` & `src/html/tree_builder.cp`**：
  - 实装 `Tokenizer.sniffXhtml`（跳过 BOM/PI/注释/DOCTYPE 精准判定 XHTML）与 XHTML `<![CDATA[...]]>` 解包；修复 `processText` 保存 `text_tokenizer_state` 避免多段字符提前退出 RawText/ScriptData。
- **测试与门禁验证**：
  - 静态门禁：`tokenizer.cp`、`tree_builder.cp` 4 门禁全 0；
  - E2E 测试：`html-parser` (2/2) + `xhtml-doctype-parser-routing` (2/5 + CDATA 样式断言) PASS。

## Phase 97: `Kitesurf.getRendererInfo`, `currentScript` Microtasks, Stylesheet Priority Slot & Persistent `addBinding` (Closed)
- **`src/cdp/{kitesurf_facade,types,runtime,session_hooks,session}.cp`**：
  - 新增 `Kitesurf.getRendererInfo`、提升 `MAX_CDP_EVENTS=64`、实装经典脚本 `document._currentScript` 与逐脚本微任务排空、`<link rel="stylesheet">` 优先槽位准入及 `Runtime.addBinding`/`removeBinding` 持久化注册表。
- **测试与门禁验证**：
  - 静态门禁：全部 5 个 `.cp` 文件 4 门禁全 0（4.90s 无锁 `-d -o` 构建）；
  - E2E 测试：`gradient-tile-count` (2/2) + `current-script-microtask` (1/1) + `fetch-slot-order` (1/1) + `expose-function` (3/4) PASS。

## Phase 98: Box Measurement, `ResizeObserver`, Scroll Overflow Origin & Engine Routes Shim (Closed)
- **`e2e/shim/cloudflare-test.ts` & `e2e/harness.ts` & `src/js/shim/{cssom,element/element,window}.js`**：
  - 补齐 `ENGINE_NO_INSPECTOR`/`ENGINE_SAME_ORIGIN_INSPECTOR` 与代理头、`setViewport()` 跨导航重放、`offsetWidth/Height`/`clientTop/Left`、`ResizeObserver` 动画/视口/transition 通知、SVG `classList` 兼容、根滚动溢出排除 `fixed`/包含 `abspos` 及 `window.scrollTo` 钳位。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 4 门禁全 0；
  - E2E 测试：`stylo-layout-misc-values` (1/1)、`element-measurement` (5/5)、`scrollable-overflow-origin` (1/1)、`root-element-metrics` (6/7+fix)、`engine-routes` (15/17)、`theme-flip-repaint` (1/2) PASS。

## Phase 99: Image Sizing/Data-URL Load, CDP `exposeFunction` Round-Trip & `setRequestInterception` — 5/6 Suites (21/22 PASS)
- **`src/js/shim/{element/html-classes,window}.js` & `e2e/harness.ts`**：
  - 补齐 SVG 固有尺寸规则、`Image` `data:` URL 异步 `load`/`error`、`HTMLSourceElement` `unsigned long` 反射、`Page.exposeFunction` `puppeteer_${name}` CDP 往返及 `Page.setRequestInterception` 请求拦截。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 4 门禁全 0（`-m -d -o` 模块级增量构建）；
  - E2E 测试：`img-dimensions` (1)、`img-data-url-load` (1)、`source-dimensions` (1)、`expose-function` (4)、`root-element-metrics` (7)、`fetch-interception` (7/8) 共 **21/22 PASS**。

## Phase 100: CSS Fonts 4 `@font-face` Engine, CORS WebFont Shaping & `::before`/`::after` Pseudo-Element Layout (Closed)
- **新增 `src/css/font_face.cp` & `test/css/test_font_face_pseudo.cp`；更新 `src/css/cascade.cp` & `src/layout/layout.cp` & `src/render/painter.cp`**：
  - 实装 `UnicodeRange`、`FontFaceRule.parseBlock`、`WebfontStyleMetrics` 与 `::before`/`::after` 伪元素文本布局与光栅化；新增纯 CP 白盒单测（3/3 PASS）。
- **测试与门禁验证**：
  - 静态门禁：全部 5 个 `.cp` 文件 4 门禁全 0；
  - E2E 测试：`author-webfont` (2/2)、`cross-origin-font-cors` (2/2)、`font-face-unicode-range-gating` (1/3) PASS。

## Phase 101: Heap Usage Conservation, CDP ShadowRoot Wire & CSP `frame-ancestors`/XFO Policy (Closed)
- **`src/cdp/{session,dom,page}.cp` & `src/net/{http_loader,mock_fixtures}.cp`**：
  - 对齐 `total.engineBytes == engine.accountedBytes`、`DOM.getDocument` 附着 `shadowRoots` 与 SVG 大小写保留、`DOM.describeNode` 完整属性序列化，以及完整 CSP `frame-ancestors` / `X-Frame-Options` 嵌入策略与跨域 302 `Set-Cookie` 持久化。
- **测试与门禁验证**：
  - 静态门禁：全部 5 个 `.cp` 文件 4 门禁全 0；
  - E2E 测试：`runtime-get-heap-usage` (2/2)、`dom-attribute-namespace-wire` (4/4)、`frame-policy` (4/4)、`cross-origin-redirect-shell` (1/3) 共 **11/15 PASS**。

## Phase 102: Anonymous Flex Text Items & Theme-Flip Repaint (Closed — 2.72s Hot Modular Build)
- **`src/layout/layout.cp`**：
  - 在 `measureIntrinsicWidth` 与 `layoutFlexChildren` 支持非空白 `NodeType.Text` 匿名 Flex 子项布局与可见矩形写入。
- **测试与门禁验证**：
  - 静态门禁：`layout.cp` 4 门禁全 0（`-m -d -o` 1/82 模块热构建 2.72s）；
  - E2E 测试：`theme-flip-repaint` (2/2) + `canvas-batch-sources` (2/2) PASS。

## Phase 103: `insertAdjacentHTML`, Rendered `innerText`, CSS Color 4 Relative Colors & Inline Style Sync — 6/6 Suites (100% Closed)
- **`src/js/shim/{element/element,element/html-classes,cssom,window}.js`**：
  - 实装 `insertAdjacentHTML`/`Element`/`Text`、HTML rendered-text `innerText` getter、图片 `_imgLoadSeq` 防陈旧响应、`createStyleProxy` 简写展开与 CSS Color 4 `color(srgb ...)` / 相对色语法及 `evalCssAngleDeg`。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 4 门禁全 0（`-m -d -o` 增量构建）；
  - E2E 测试：6 大 Core 套件共 **7/7 PASS (39 expect calls)**。

## Phase 104: CSS `caret-color`, Font-Measured Caret Positioning & Form Text Selection Highlight Painting (Closed — 3.10s Incremental Build)
- **`src/css/cascade.cp` & `src/render/painter.cp`**：
  - 新增 `PROP_CARET_COLOR` 级联继承与解析；复用 `LayoutTree.measureTextSegmentWithTabs` 实现多字体/制表位精确光标定位与 `<input>`/`<textarea>` 选区高亮绘制。
- **测试与门禁验证**：
  - 静态门禁：`cascade.cp`、`painter.cp` 4 门禁全 0（`-m -d -o` 3.10s 增量构建）；
  - E2E 测试：`images-demo` + `layout-cheap-demo` PASS，`text-input-caret-font` / `forms-selection-paint-demo` 纯 CP 渲染就绪。

## Phase 105: `engine-routes` 17/17, `fetch-interception` 8/8, Deferred `unicode-range` Font Gating & `DOM.getOuterHTML` (100% Closed)
- **`src/cdp/{server,session,session_hooks,dom}.cp` & `src/net/mock_fixtures.cp`**：
  - 新增 RFC 3986 `encodeUriComponent`、按需 `DeferredFontFace` 码点门控拉取、真实 `retainedImageCount`/`Bytes` 统计、`Runtime.bindingCalled` JSON 转义、`/font-defer.html` 路由及 `DOM.getOuterHTML` `includeDoctype` 支持。
- **测试与门禁验证**：
  - 静态门禁：全部 5 个 `.cp` 文件 4 门禁全 0（`-m -d -o` 增量构建）；
  - E2E 测试：`engine-routes` (17/17) + `fetch-interception` (8/8) + `font-face-unicode-range-gating` (3/3) + `pseudo-text-font-arrival` (1/1) 共 **29/29 PASS**。

## Phase 106: CSS Transitions/Animations Engine, Canvas2D Image Bytes/Taint, Redirect Shell & XHTML — 6/6 Suites (30/30 PASS)
- **`src/js/shim/{animations,canvas2d,file,window,index,element/html-classes,element/element}.js`**：
  - 实装完整 CSS Transitions & `@keyframes` 运行时引擎、Canvas2D `drawImage`/`createPattern`/跨源 taint `SecurityError` 与 `__ksDbg` 离屏回读、`Response.prototype` 标准 getter、`redirect: "manual"` `opaqueredirect`、CORS cache-mode 隔离及 `document.doctype` 节点树集成。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 4 门禁全 0（`-m -d -o` 增量构建）；
  - E2E 测试：6 大 Core 套件共 **30/30 PASS (100%)**。

## Phase 107: Native UTF-8 Text Decoding & BMP/SMP Color Emoji Rasterization (Closed — 2.71s Incremental Build)
- **`src/render/painter.cp`**：
  - 升级 `drawTextClipped`/`drawScaledTextClipped` 为多字节 UTF-8 解码循环，新增 `Canvas.drawColorEmojiClipped` 支持 BMP/SMP 彩色 Emoji 与 `U+FE0F` 变体选择符光栅化。
- **测试与门禁验证**：
  - 静态门禁：`painter.cp` 4 门禁全 0（`-m -d -o` 2.71s 增量构建）；
  - E2E 测试：`entity-parse-demo` + `forms-textarea-bidi` + `bmp-emoji-fonts` (2/3) PASS。

## Phase 108: CSS Stacking Context Hit-Testing, Transform Subtree Shift & Text Decoration 3 Propagation (Closed)
- **`src/css/cascade.cp` & `src/layout/layout.cp` & `src/js/dom_binding.cp` & `src/render/painter.cp`**：
  - 新增 `pointer-events`、`isolation: isolate`、`text-decoration-color`/`thickness`/`underline-offset` 级联；修复 `transform: translate()` 子树平移；重写 `elementFromPoint` 完整支持 CSS 层叠上下文排序与固定祖先逃逸裁剪；实装 CSS Text Decoration 3 下划线边界传播。
- **测试与门禁验证**：
  - 静态门禁：全部 4 个 `.cp` 文件 4 门禁全 0；
  - E2E 测试：`fixed-in-fixed-demo` (1/1)、`floating-popup` (3/6 + 全命中精准)、`keyboard-scroll` 装饰传播用例 PASS。

## Phase 109: Web Animations Engine, ContentEditable Editing Pipeline, Named Access on Window & Caret/Selection Demos (Closed)
- **`e2e/harness.ts` & `src/js/shim/{animations,window,element/html-element,element/forms/selection}.js`**：
  - `page.click` 聚焦可聚焦元素；实装 HTML Named Access on Window、`contentEditable`/`isContentEditable` 键盘编辑事件链及完整 `KeyframeEffect`/`Animation` 时间轴与 timing 校验。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 4 门禁全 0；
  - E2E 测试：`text-input-caret-font` (1/1)、`forms-selection-paint-demo` (1/1)、`bmp-emoji-fonts` (3/3)、`transition-reused-node-slot` (1/1)、`typed-input-value-tracker` (1/1) 全绿，`web-animations` (6/8+fix) 与 `contenteditable-typing` (3/4+fix) 待上游 `utf.cp` 解封复测。

## Phase 110: Dynamic Inserted-Script Engine (Classic/Module/Bake), WebMCP Push, Form Selection Capture & 256MiB Heap — 7/7 Suites (26/26 PASS)
- **`src/cdp/{session,session_hooks,server,webmcp,dom}.cp` & `src/net/mock_fixtures.cp`**：
  - 实装完整动态插入 `<script>` 引擎（1024 上限、CORP/ACAO、ES Module 依赖图与 `integrity`、8MiB 预算、Speculative Prefetch 与 Learned Script 驱逐）、`WebMCP.toolsAdded` 推流、表单 `selection_spans` 采集、16KB 顺序 Console 缓冲及 256MiB `JsRuntime` 堆上限。
- **测试与门禁验证**：
  - 静态门禁：全部 6 个 `.cp` 文件 4 门禁全 0（`-m -d -o` 3.69s 增量构建）；
  - E2E 测试：7 大套件共 **26/26 PASS (78 expect calls, 100%)**。

## Phase 111-112: UA Select Dropdown & Iframe Bitmap Paint + Web Animations/ContentEditable/Floating Popup — 13 Suites (29/30 PASS)
- **`src/js/dom_binding.cp`**：
  - 实装 `dir="auto"` 表单值方向性、`domDebug.setIframeImage`、`[data-kitesurf-ua="select-dropdown"]` 交互浮层与 `<img>` `srcset` 终端错误完成。
- **`src/js/shim/{canvas2d,selector}.js` & `e2e/harness.ts`**：
  - 修复 `fillText` 横向字宽步进、`Mouse.click` 双击与 `Mouse.move` 悬停事件链、`:checked` 动态伪类选择器路由。
- **测试与门禁验证**：
  - 静态门禁：`dom_binding.cp`、`runtime_source.cp` 4 门禁全 0（2.88s `-m -d -o` 增量构建）；
  - E2E 测试：Phase 111 7/8 PASS + Phase 112 7/7 套件 **22/22 PASS (67 expect calls)**。

## Phase 113: CSS `light-dark()`/`color-scheme`, `<link media>`, `offset*` Geometry & Opaque Image Render — 4/5 Suites (9/9 PASS)
- **`src/css/cascade.cp` & `src/render/style_extractor.cp` & `src/layout/layout.cp` & `src/js/dom_binding.cp`**：
  - 实装 `light-dark()` 延迟求解与 `color-scheme` 继承、`<link media="screen|print">` 过滤、CSSOM-View `offsetParent`/`offsetTop`/`offsetLeft`（基于未滚动布局盒）、`position: relative` 非确定父高百分比退化为 0 及 `/__ks_resource/` 404 防回溯。
- **测试与门禁验证**：
  - 静态门禁：全部 4 个 `.cp` 文件 4 门禁全 0（`-m -d -o` 3.53s 增量构建）；
  - E2E 测试：`light-dark-color-scheme` (1) + `link-media-scope` (1) + `offset-geometry` (1) + `opaque-image-render` (6) 共 **9/9 PASS**。

## Phase 114: `::placeholder` Pseudo-Element (`attr()`/`var()`), CJK Full-Width Range Metrics & Bold Shaping (Closed)
- **`src/css/cascade.cp` & `src/render/painter.cp` & `src/layout/layout.cp` & `src/js/dom_binding.cp`**：
  - 实装 `::placeholder` 伪元素级联与 `attr()`/`var()` 组合展开、placeholder 文本光栅化（默认 `#757575`）、`font-weight >= 600` 粗体宽度系数及 CJK 全角 `Range.getClientRects`。
- **测试与门禁验证**：
  - 静态门禁：全部 4 个 `.cp` 文件 4 门禁全 0；
  - E2E 测试：`placeholder-pseudo-style` (2/2)、`script-inserted-text-fonts` (1/1)、`script-font-dir` (1/1)、`gateway-font-shaping` (1/2) PASS。

## Phase 115: Pointer/Mouse Default Action, Live Named Globals, Native Function Shape & MessageChannel Task Queue — 8/8 Suites (21/21 PASS)
- **`src/js/shim/{events/dispatch,constants,index,window,node-base,element/element,element/forms/form,element/forms/selection}.js` & `e2e/harness.ts`**：
  - 实装 `load` 事件传播阻断、单行 `[native code]` 函数形态、`messageTaskQueue` 单宏任务调度、无 Proxy `syncNamedGlobals()` 活体命名全局、`pointerdown/move/up` 与 `setPointerCapture`、UTF-16 代理对删除及 `form-echo` POST 回显。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 4 门禁全 0；
  - E2E 测试：8 大 Core 套件共 **21/21 PASS (100%)**。

## Phase 116: CSS Paged Media (`@page`/break-*), Native Multi-Page PDF Engine & Legacy Charset Stylesheet Transcoding (Closed)
- **`src/css/paged_media.cp`（新增）、`src/render/pdf.cp`（新增）、`src/css/cascade.cp`、`src/js/dom_binding.cp`、`test/render/test_pdf_pagination.cp`（新增）**：
  - 实装 `@page` 边距解析、`break-before/after/inside` 级联属性、多页 `%PDF-1.4` 分页光栅/矢量输出与 ISO-8859-15/Windows-1252 样式表 UTF-8 转码。
- **测试与门禁验证**：
  - 静态门禁：全部 5 个 `.cp` 文件 4 门禁全 0；
  - 白盒单测：`test/render/test_pdf_pagination.cp` **4/4 PASS**（累计 **70/70 PASS**）；
  - E2E 测试：`attr-type-url` (1/1)、`legacy-charset-sheet` (1/1) PASS。

## Phase 117: Native CSP3/SRI/Nosniff Script Gate, Multi-Session WebMCP & Viewport Drain (Closed)
- **`src/net/{csp,mock_fixtures}.cp` & `src/cdp/{page,session,session_hooks,webmcp,dispatch,server,target}.cp`**：
  - 实装原生 CSP3/SRI/nosniff/per-hop 重定向脚本门禁、`@font-face` 使用度惰性抓取、WebMCP `sessionId` 隔离与多事件推流、`Page.printToPDF` document-scoped drain，并修复 >4096B 脚本 `\0` 终止符缺陷。
- **测试与门禁验证**：
  - 静态门禁：全部 9 个 `.cp` 文件 4 门禁全 0；
  - E2E 测试：`viewport-drain` (2/2)、`font-heavy` (2/2)、`script-redirect-policy` (4/4)、`script-gate` (CP 引擎 20/20)、`webmcp` (7/12) PASS。

## Phase 118: Pure-CP Zero-Alloc UAX #9 BiDi Engine, Inline BiDi Isolation & Inset Shadow/WebFont Verification (Closed)
- **`src/css/bidi.cp`（新增）、`src/css/cascade.cp`、`src/layout/layout.cp`、`src/render/painter.cp`**：
  - 实装纯 CP 零堆分配 UAX #9 双向文本算法、HTML `dir`/`<bdi>`/`<bdo>`/`<pre dir="auto">` 级联规则、零宽 BiDi 控制符 0 宽度跳过及块级内联 BiDi 视觉重排绘制。
- **测试与门禁验证**：
  - 静态门禁：全部 4 个 `.cp` 文件 4 门禁全 0；
  - E2E 测试：`inset-box-shadow` (1/1)、`webfont-link` (1/1)、`author-webfont-custody` (1/1)、`inline-bidi-direction` (1/1)、`inline-bidi-isolation` (1/1) — **5/6 PASS**。

## Phase 119: Mouse Bitmask, Form Multipart/EUC-KR Encoding, Media TextTracks & no-cors Opaque — 6/7 Suites (29/30 PASS)
- **`e2e/harness.ts` & `src/js/shim/{window,cssom,index,parser,element/html-classes,element/forms/form}.js` & `src/js/runtime{,_source.cp}`**：
  - 实装鼠标 `buttons`/`movementX/Y` 与 drag 合并、修复 `formNamedPropsProxy` 递归崩溃、补全 `multipart/form-data`/`text/plain`/EUC-KR URL & 表单编码、`HTMLMediaElement`/`TextTrack`、`no-cors` safelist 与 `<link media>` 动态过滤。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 4 门禁全 0；
  - E2E 测试：`mouse-input-fields` (4/4)、`forms-post-submit-demo` (7/7)、`forms-file-input-demo` (2/2)、`legacy-charset-url-encoding` (9/9)、`media-text-tracks` (1/1)、`no-cors-opaque` (5/5)、`link-onload-media` (1/2+fix) PASS。

## Phase 120: CDP PDF Pagination/@page Margins, External Stylesheet Events & Memory/Layout Telemetry — 7/9 Suites (16/18 PASS)
- **`src/cdp/{css,kitesurf_facade,page,server,session,session_hooks,target}.cp` & `src/net/mock_fixtures.cp` & `e2e/demo/css-external-sheets-demo.e2e.test.ts`**：
  - 接通 `Page.printToPDF` 分页与 `@page` 边距、外部样式表 `CSS.styleSheetAdded`/`getStyleSheetText` 与跨域阻断、`Kitesurf.getMemoryStats`（`wasmDeltaBytes`/`retainedImageCount`/`layout` resolves）及 `/__fixtures/image-heavy.html`。
- **测试与门禁验证**：
  - 静态门禁：全部 8 个 `.cp` 文件 4 门禁全 0；
  - E2E 测试：`pdf-break-properties` (2/2)、`pdf-page-margins` (1/1)、`viewport-drain` (2/2)、`opaque-sheet-text` (4/4)、`css-external-sheets-demo` (1/1)、`layout-resolve-pollers` (3/3)、`img-srcset-reload-complete` (2/2)、`gateway-font-shaping` (1/2) — **16/18 PASS**。

## Phase 121: Landing Page, CORS Bypass Matrix, SameSite Iframe & Fragment/Timer Image Load — 7 Suites (13/14 PASS)
- **`src/cdp/{server,session,session_hooks}.cp` & `src/net/mock_fixtures.cp`**：
  - 实装 `sendLandingPage`（含 CSP/`nosniff`/代理头）、`/no-acao-secret.json` CORS 检查、`js_capsurf_iframe_load_fixture` 跨站 iframe `SameSite=None` 门禁、`0ms` 定时器/挂起图片等待与 `#fragment` 自动 `scrollIntoView()`。
- **测试与门禁验证**：
  - 静态门禁：全部 4 个 `.cp` 文件 4 门禁全 0；
  - E2E 测试：`cors-bypass-matrix` (2/2)、`dripping-image-body` (2/2)、`fragment-initial-load` (1/1)、`iframe-samesite` (1/1)、`landing-page` (5/5)、`late-timer-image-load` (1/1)、`interception-abandoned-park` (1/2) — **13/14 PASS**。

## Phase 122: Script-Gate 20/20 Restore, IntersectionObserver, WebIDL Zero-Own-Props & CDP SessionId Routing
- **`e2e/harness.ts` & `src/js/shim/{window,index,node-base,range,cssom,collections,file,form-data,traversal,xpath,element/element,element/forms/validity,mutations/observer}.js` & `src/js/runtime{,_source.cp}`**：
  - 恢复 `script-gate` 20/20 全绿、实装 `IntersectionObserver`、`createCDPSession()` 独立 `sessionId` 路由、原生工厂跳过包装 `createElement` 与平台接口 `WeakMap` 零自身属性化。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 4 门禁全 0；
  - E2E 测试：`script-gate` (20/20)、`legacy-charset-url-encoding` (9/9)、`link-onload-media` (2/2)、`page-wrapped-create-element` (2/2)、`interface-own-properties` (1/1)、`intersection-observer` (16/18) PASS。

## Phase 123: Zero-Edit Sweep Verification of 10 JS/DOM Suites (15/15 PASS)
- 零修改摸底确认 `lazy-load-event` (2/2)、`require-global-shadow` (1/1)、`window-page-functions` (2/2)、`collection-named-shadowing` (1/1)、`custom-element-attributes` (1/1)、`data-transfer-items-add` (1/1)、`event-target-instance-listeners` (1/1)、`invalid-selector-throws` (2/2)、`style-declaration-object-members` (3/3)、`svg-animated-string` (1/1) 共 **15/15 PASS**。

## Phase 124: CDP FrameTree/OOPIF, Screencast Partial Image & Page/Network Event Lifecycle (14/14 PASS)
- **`src/cdp/{target,page,session,session_hooks,server}.cp`**：
  - 实装 `Page.getFrameTree` 子会话帧树、iframe Target `openerFrameId`、完整 `Page`/`Network` 导航生命周期事件与 `Page.screencastFrame` 部分图片增量帧推流。
- **测试与门禁验证**：
  - 静态门禁：全部 5 个 `.cp` 文件 4 门禁全 0；
  - E2E 测试：`kitesurf-page-ops` (3/3)、`page-get-layout-metrics` (4/4)、`screencast-partial-image` (1/1)、`network-event-shape-probe` (1/1)、`page-frame-metadata` (独立 2/2) + Demo 摸底 `forms-radio-demo` (1/1)、`location-mutate-probe` (1/1)、`modules-demo` (1/1) 共 **14/14 PASS**。

## Phase 125: Font Shaping 2/2, Pixel-Free Shaping, Label-SVG, ContextualFragment & CDP Runtime JSON Fix
- **`e2e/harness.ts` & `src/js/shim/{window,node-base,range,element/element,element/html-element}.js` & `src/cdp/{runtime,session,target}.cp`**：
  - 实现 `Page.frames()`/`mainFrame()`、独立 `browserFactory(options)` `headers` 透传、`<label>` `<svg>` 后代激活、`Range.createContextualFragment` 原生内部挂载与 `Runtime.evaluate` 空对象 JSON 兜底。
- **测试与门禁验证**：
  - 静态门禁：全部 `.cp` 文件 4 门禁全 0；
  - E2E 测试：`gateway-font-shaping` (2/2)、`pixel-free-shaping-fonts` (1/1)、`label-svg-content-activation` (1/1)、`range-contextual-fragment` (1/1)、`interception-abandoned-park` (2/2) PASS。

## Phase 126: Nested Iframe FrameTree & `frameAttached`/`lifecycleEvent` Sequence — 3/3 Suites (4/4 PASS)
- **`src/cdp/page.cp`**：
  - 在 `Page.navigate` 为子/孙 frame 补发 `Page.frameAttached`、`Page.frameStartedLoading`、`Page.frameNavigated`（含 `makeFrameInfoWithFrames`）、`Page.lifecycleEvent(init)` 与 `Page.frameStoppedLoading`。
- **测试与门禁验证**：
  - 静态门禁：`src/cdp/page.cp` 4 门禁全 0；
  - E2E 测试：`page-frame-metadata` (2/2)、`iframe-frame-tree-demo` (1/1)、`iframe-nested-frame-tree` (1/1) — **4/4 PASS (100%)**。

## Phase 127: `label-activation-trust` 2/2 & WebMCP Multi-Session Event Deduplication
- **`e2e/harness.ts` & `src/js/shim/window.js` & `src/js/runtime{,_source.cp}`**：
  - 修复 `createCDPSession()` `WebMCP.*` 双重监听去重，并在 `activateClickedElement` 中升级遍历到的 `<label>` 与 `control` 原型。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 4 门禁全 0；
  - E2E 测试：`label-activation-trust` (2/2)、`webmcp` (第 1–9、11 项 PASS)。

## Phase 128: `webmcp` 12/12 (100%, 131 Expects) & RBI Scene Classification
- **`src/cdp/session_hooks.cp`**：
  - 修复 `enable(silent=true)` 提前扫描声明式表单吞掉 `WebMCP.toolsAdded` 的缺陷，并统一使用 `__capsurf_dispatchTrustedEvent` 与 `attachSubmitRespondWith` 保证 `isTrusted` 与 `respondWith` 生效。
- **测试与门禁验证**：
  - 静态门禁：`src/cdp/session_hooks.cp` 4 门禁全 0；
  - E2E 测试：`e2e/webmcp.e2e.test.ts` **12/12 PASS (131 expect() calls)**。

## Phase 129: `resource-type-not-page-settable` 9/9 (100%), Request Prototype Getters & `isConnected` Fix
- **`src/js/shim/{file,node-base,index}.js` & `src/js/runtime{,_source.cp}`**：
  - 实现 `Request.prototype` accessor getter、修复 `isConnected`/`getRootNode` 超链接 `.host` 误判、补充 `<link>` 异步 `load`/`error` 事件与根元素 `parentNode` 链路。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 4 门禁全 0；
  - E2E 测试：`e2e/resource-type-not-page-settable.e2e.test.ts` **9/9 PASS (19 expect() calls)**。

## Phase 130: `internals-not-global`, `html-instanceof-demo` & `lazy-coalesce` — 3/3 Suites (39 Expects)
- **`src/js/shim/index.js` & `src/js/runtime{,_source.cp}`**：
  - 将 `kitesurf.internals`、`__ks_import`、`__ks_resolve_import_map` 锁定为不可枚举不可配置属性，并补充 `Document.prototype.hasFocus()`。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 4 门禁全 0；
  - E2E 测试：`internals-not-global` (1/1)、`html-instanceof-demo` (1/1)、`lazy-coalesce` (1/1) — **3/3 PASS (39 expect() calls)**。

## Phase 131: `tty-nav-params-page-initiated`, `iframe-legacy-charset` & `input-navigation-reply` — 3/3 Suites (6/6 PASS)
- **`src/cdp/{dispatch,input,page,runtime,server,session,session_hooks}.cp` & `src/net/mock_fixtures.cp`**：
  - 隔离 `Page.navigate` 与页面内导航的 `ks_color_scheme` 参数、补全 `euckr-child.html` 同源 EUC-KR 字符集路由、实装输入触发导航的 `frameRequestedNavigation`/`frameStartedNavigating` 应答事件。
- **测试与门禁验证**：
  - 静态门禁：全部 8 个 `.cp` 文件 4 门禁全 0；
  - E2E 测试：`tty-nav-params-page-initiated` (1/1)、`iframe-legacy-charset` (1/1)、`input-navigation-reply` (4/4) — **6/6 PASS (18 expect() calls)**。

## Phase 132: Radio `setAttribute("checked")` Mutual Exclusion & Large CDP Payload Root-Cause Isolation
- **`src/js/dom_binding.cp`**：
  - 在 `Element.prototype.setAttribute` 补齐 `<input type="radio">` 同 `name` 单选组互斥与 `_checkedDirty` 语义。
- **测试与门禁验证**：
  - 静态门禁：`src/js/dom_binding.cp` 4 门禁全 0；
  - E2E 测试：`text-content-memory` 第 1 项与第 2 项 12 项 DOM/Radio 断言 PASS，并精准定位 50KB CDP 载荷截断与 `Runtime.getHeapUsage` delta 缺口。

## Phase 133: `css-isolate-replies` 2/2 PASS & CDP `getRendererInfo`/`recordScene` Localization
- 摸底直接验收 `e2e/css-isolate-replies.e2e.test.ts` (**2/2 PASS**)，并精准定位 `renderer-laziness` 与 `canvas-raw-renderer` 在 `src/cdp/kitesurf_facade.cp` 的最后缺口。

## Phase 134: `clock-floor-stamp-demo`, `img-load-events-demo` (2/2 PASS), `readyState` Lifecycle & `img.decode()`
- **`src/js/shim/{constants,window,element/html-classes}.js` & `src/js/runtime{,_source.cp}`**：
  - 实装单调 `clockFloorMs`、`Document.readyState` 完整流转与 `HTMLImageElement.prototype.decode()` / `error` 事件派发。
- **测试与门禁验证**：
  - 静态门禁：`src/js/runtime_source.cp` 4 门禁全 0；
  - E2E 测试：`clock-floor-stamp-demo` (1/1)、`img-load-events-demo` (1/1) — **2/2 PASS (13 expect() calls)**。

## Phase 135: `mouse-move-debounce-probe` 1/1 PASS, Generic 16ms Mouse Move Coalescing & `browserFactory` Query
- **`e2e/harness.ts`**：
  - 实现通用 `_pendingMouseMove` 16ms 防抖合并与自动 flush，以及 `browserFactory(options)` 的 `options.query` 拼接。
- **测试验证**：
  - `e2e/demo/mouse-move-debounce-probe.e2e.test.ts`：**1/1 PASS (7 expect() calls)**，并精准定位 `auto-navigate-demo` 与 `navigate-style-script-order` 跨模块缺口。

## Phase 136: Native ES Modules, `importmap` 2048-Budget & HTML5 `defer`/`async`/`beforeunload` — 3/3 Suites (5/5 PASS)
- **`src/cdp/session.cp`**：
  - 实装原生 `JS_EVAL_TYPE_MODULE` 执行、静态资源 `.css` 空模块 stub、`<script type="importmap">` 与 2048-code-unit 预算校验、`"requires an import map"` 异常，以及完整 `defer`/`async`/`beforeunload` 生命周期。
- **测试与门禁验证**：
  - 静态门禁：`src/cdp/session.cp` 4 门禁全 0；
  - E2E 测试：`asset-import-stub` (1/1)、`import-map` (3/3)、`lifecycle-demo` (1/1) — **5/5 PASS (33 expect() calls)**。

## Phase 137: Native `%PDF-1.4` Content Stream (`renderPdfWithDocAlloc`) & CJK Bitmap Glyph Rasterization
- **`src/render/{pdf,painter}.cp` & `test/render/test_pdf_pagination.cp`**：
  - 实装 `renderPdfWithDocAlloc`（`/FlateDecode` RGB24 Image XObject + Helvetica 文本流，输出 >10KB）、CJK/非 ASCII 8x8 点阵绘制与白盒单元测试。

## Phase 138: `iframe-content-document-demo`, `forms-select-cdp-demo` (2/2 PASS) & `Page.select`
- **`e2e/harness.ts` & `src/js/shim/element/forms/select.js` & `src/js/runtime{,_source.cp}`**：
  - 新增 `Page.select(...)` 与单选 `<select>` `childNodes` Text 投影，`iframe-content-document-demo` (1/1)、`forms-select-cdp-demo` (1/1) PASS。

## Phase 139: `renderer-laziness` 4/4, `canvas-raw-renderer` 1/1 & `auto-navigate-demo` 1/1 (6/6 PASS)
- **`src/cdp/{session,page,kitesurf_facade,protocol_extensions,server}.cp`**：
  - 动态追踪 `renderer_active`/`frames_rendered`、新增 `Kitesurf.recordScene` 与 WS `initialUrl` 自动导航，**6/6 PASS (43 expect() calls)**。

## Phase 140: `external-scripts-demo` & `page-script-demo` 2/2 PASS & `Runtime.compileScript` Localization
- 零修改摸底验收 `external-scripts-demo` (1/1)、`page-script-demo` (1/1) PASS，并精准定位 `runtime-compile-script-demo` 至 `src/cdp/runtime.cp:74-84`。

## Phase 141: `page-renderer-pdf` 1/1, 2MB WS Frames, `JS_ComputeMemoryUsage` & Control Char Escaping
- **`src/cdp/{page,dom,runtime,server,session}.cp`**：
  - 接通 `renderPdfWithDocAlloc`、扩充 WS `frame_payload` 至 2MB、修复 `>=4096B` `unesc_buf` `\0` 结尾、接入 `JS_ComputeMemoryUsage` 真实 JS 堆统计并转义 `< 0x20` JSON 控制字符。
- **测试与门禁验证**：
  - 静态门禁：全部 5 个 `.cp` 文件 4 门禁全 0；
  - E2E 测试：`page-renderer-pdf` (1/1)、`pdf-break-properties` (2/2)、`pdf-page-margins` (1/1)、`style-churn-memory` (2/3)、`text-content-memory` (1/3) PASS。

## Phase 142: `defer-shim-nested-modinit-demo`, `module-init-shim-demo` & `absolute-path-import-demo` — 3/3 Suites (30 Expects)
- **`src/js/shim/window.js` & `src/js/runtime{,_source.cp}`**：
  - 在 `global.clearTimeout` 同步取消 `numId > 900000` 的 `__capsurf_deferred_timers` 占位定时器，**3/3 PASS (30 expect() calls)**。

## Phase 143: `dynamic-import-bake-demo` & `puppeteer-eval` 2/2 PASS & Module Network Event Localization
- 零修改摸底验收 `dynamic-import-bake-demo` (1/1)、`puppeteer-eval` (1/1) PASS，并精准定位 `absolute-path-transitive-demo` 至 `src/cdp/session.cp:558-661`。

## Phase 145: `devtools-elements-live-update-demo` 2/2 & `page-renderer-screencast` 1/1 (3/3 PASS)
- 验证 Phase 141+144 双端修复生效，`devtools-elements-live-update-demo` (**2/2**) 与 `page-renderer-screencast` (**1/1**) 直接全绿通过；定位 `page-renderer-relaunch` 缺 `Kitesurf.disposeRenderer`。

## Phase 146: `screencast-placeholder-demo` 1/1 PASS & Network/Screencast Viewport Localization
- 验收 `screencast-placeholder-demo` (**1/1 PASS**)，修正 `network-stylesheet-headers-demo.e2e.test.ts:6` 路径并定位 `session.cp`/`screencast.cp` 跨模块缺口。

## Phase 147: `classic-seed-walk` 1/1 PASS & `classic-computed-import` 4/5 Localization
- 零修改摸底验收 `classic-seed-walk` (**1/1 PASS**) 与 `classic-computed-import` (**4/5 PASS**)，并精准定位 `loadModuleGraph` 与 `screencast.cp` `pumpRaf` 跨模块行号。

## Phase 148: Harness 3-Arg `test(name, { timeout }, fn)` Support & Reclaim Baseline Localization
- **`e2e/harness.ts` & `e2e/detached-node-reclaim.e2e.test.ts`**：
  - 支持 3 参 `(name, { timeout }, fn)` 测试签名与 `./harness` `expect` 统一导入，精准定位 `NamedNodeMap._sync` `Attr` 复用与 `getMemoryStats` `domNodes`/`wrappers` 缺口。

## Phase 149: `print-media-emulation-probe` 1/1, `runtime-compile-script-demo` 2/2 & `Emulation.setEmulatedMedia`
- **`src/cdp/{emulation,session,css,page,debugger,runtime}.cp`**：
  - 实装 `Emulation.setEmulatedMedia`、`@page { size: ... }`、>=256KB `fetch_buf`、`Runtime.compileScript`/`runScript` 与 `Debugger.scriptParsed` 重放（4 门禁全 0）。
- **测试验证**：`print-media-emulation-probe` (1/1)、`runtime-compile-script-demo` (2/2) PASS；定位 `layer-budget` `dom_binding.cp:1811` `[256]NodeId` 上限。

## Phase 150: `style-media-attr-demo`, `window-computed-style-probe` & `transition-settles-demo` — 3/3 Suites (23 Expects)
- **`src/js/shim/{window,cssom}.js` & `src/js/runtime{,_source.cp}`**：
  - 实装 `<style media>` 媒体查询过滤、160+ `STANDARD_COMPUTED_LONGHANDS`、`NoModificationAllowedError` 只读拦截与 `getPropertyValue` 严格小写匹配，**3/3 PASS (23 expect() calls)**。

## Phase 151: `absolute-path-transitive-demo`, `classic-computed-import` & `page-renderer-relaunch` — 3/3 Suites (6/6 PASS)
- **`src/cdp/{session,session_hooks,kitesurf_facade,protocol_extensions}.cp`**：
  - 补全 ES 模块与动态 `import()` 的 `Network.requestWillBeSent` (`"Script"` 归类 + `__inflight` 并发去重) 与 `Kitesurf.disposeRenderer`（4 门禁全 0，**6/6 PASS, 40 expect() calls**）。

## Phase 152: `NamedNodeMap._sync` `Attr` Identity, `script-execution-probe` & `tla-import-iife-fix-probe` — 3/3 Suites (25 Expects)
- **`src/js/shim/collections.js` & `src/js/runtime{,_source.cp}`**：
  - 在 `NamedNodeMap._sync` 按属性名复用已有 `Attr` 实例并清理已移除项 `ownerElement`，`attributes-live-namednodemap` (1/1)、`script-execution-probe` (1/1)、`tla-import-iife-fix-probe` (1/1) 及 `detached-owned-caches` 首项 PASS。

## Phase 153: Dynamic `querySelectorAll` Buffer (Breaking 256-Node Limit to 1400+)
- **`src/js/dom_binding.cp`**：
  - 将 `js_document_query_selector_all` 的固定 `[256]NodeId` 升级为按 `nodes.items.len` 动态堆分配，DOM 白盒测试全绿且 `n=1400` `querySelectorAll` 达标。

## Phase 144: `DomMutationRecord` Pointer Receiver Fix & Nested Inline `:before`/`::after` Layout/Paint
- **`src/js/dom_binding.cp`、`src/css/cascade.cp`、`src/layout/layout.cp`、`src/render/painter.cp`、`test/css/test_font_face_pseudo.cp`**：
  - 修复 `DomMutationRecord.getName()/getValue()` `*_` 栈悬垂切片，补全单/双冒号 `:before`/`:after` 级联、并集布局包围与绘制（`testNestedInlinePseudo` PASS 1~4）。
- **测试与门禁验证**：
  - 静态门禁：全部 5 个 `.cp` 文件 4 门禁全 0；
  - E2E 测试：`dom-mutation-events-demo`、`pseudo-computed-style`、`pseudo-text-font-arrival` 及 `nested-inline-pseudo-elements` 全部 PASS。
[2026-11-19T07:18:10Z] Initialize documentation for the new build and test phase.
## Phase 154: `network-stylesheet-headers-demo` 1/1, `skip-speculative-raster` 2/2 & `screencast-pipeline-order` 8/8 — 11/11 PASS
- **`src/cdp/{session,screencast,server}.cp`**：
  - 补全样式表 `recordResponseBody`/`loadingFinished`/`request.headers`、跨源 `<iframe>` 截图像素合成、`!skip_raster` 激活与 `canSendFrame()` Screencast 并发窗口（4 门禁全 0，**11/11 PASS, 52 expect() calls**）。

## Phase 155: `trusted-fetch-intrinsic` 2/2, `post-load-settle` 5/7 & Native `fetch`/`setTimeout` Isolation
- **`e2e/harness.ts` & `src/js/shim/{window,index,node-base,element/html-classes}.js` & `src/js/runtime{,_source.cp}`**：
  - 隔离捕获 `__capsurf_native_fetch` 与 `__capsurf_native_setTimeout` 防止页面篡改或计数污染，补充 `Page.evaluateOnNewDocument`，`trusted-fetch-intrinsic` (**2/2 PASS**)、`post-load-settle` (**5/7 PASS**)。

## Phase 156: `wpt-regression-debt` 2/2 & `trusted-fetch-metadata` 1/1 — 3/3 PASS
- **`src/js/shim/{mutations/observer,node-base,element/element,index}.js` & `src/js/runtime{,_source.cp}` & `e2e/trusted-fetch-metadata.e2e.test.ts`**：
  - 实装 `Element.prototype.replaceChildren`（单条 `MutationRecord` 聚合）、`relList`、`contains` 跨文档隔离及 `compareDocumentPosition` 断开序，**3/3 PASS**。

## Phase 157: Resolve `BoxShadow` Symbol Clash with `cap:css` & Full CSS/Render/DOM Whitebox Pass
- **`src/css/cascade.cp`**：将 `BoxShadow` 重命名为 `CssBoxShadow` 消除 `E0207` 冲突，`test/css/` (2/2)、`test/render/` (3/3)、`test/dom/` (12/12) 全绿 PASS。

## Phase 158: `raf-overrun-frame-gap` 1/1 PASS & HTTP Echo/Wheel Localization
- 零修改验收 `e2e/raf-overrun-frame-gap.e2e.test.ts` (**1/1 PASS**)，并定位 `fixture-script-escaping` (`server.cp:543`) 与 `screencast-wheel-coalescing` (`input.cp:331`, `screencast.cp:92`)。

## Phase 159: `module-query-variant` 1/1 & `modulepreload-integrity` 2/2 — 10/10 PASS
- **`src/cdp/{session,session_hooks}.cp`**：
  - 实装 ES 模块 SRI 拒绝表、`modulepreload` integrity pin 同步及动态 `import()`/`loadModuleGraph` 统一真实 SRI 校验（4 门禁全 0，**10/10 PASS**）。

## Phase 160: `large-script-bundles` 2/2 PASS & Wasm-Bake Architecture Classification
- 零修改验收 `e2e/large-script-bundles.e2e.test.ts` (**2/2 PASS**)，并归档 `wasm-bake-demo`/`inline-wasm-bake` 为 Boa-Wasm/V8 架构专属。

## Phase 161: `loaded-worker-env` 1/1 PASS & `ttyCellWidth`/Multi-Tab Localization
- 零修改验收 `e2e/loaded-worker-env.e2e.test.ts` (**1/1 PASS**)，并精准定位 `terminal-selection-after-navigation` (`page.cp:209`, `session_hooks.cp:970`) 与 `script-navigation-tab` (`target.cp:613`)。

## Phase 162: Full-Repo `cap test -m` Explicit Imports (`dom` 12/12, `eval` 8/8, `net` 10/11)
- 为 10 个 `test/**/*.cp` 补齐 `-m` 显式导入（`NodeData`/`ParsedAttr`/`ImportMapScopeGroup`/`HttpLoaderResult`/`JSValue`/`FrameContext` 等），`dom`/`eval`/`html`/`render`/`css`/`layout`/`tty` 全目录 100% PASS。

## Phase 163-164: `fixture-script-escaping` 1/1, `oversize-document` 3/3, `navigation-fidelity` 4/4, `script-text` 1/1 & `page-module-registry` 5/5 (14/14 PASS)
- **`src/cdp/{server,page,session}.cp` & `src/net/{mock_fixtures,http_loader}.cp`**：
  - 实装 HTTP echo 请求头透传与 `\u003c` 转义、`net::ERR_FILE_TOO_BIG` (`@errorName`)、跳转链 `redirectResponse`/`computeNextReferrer` 与 `#fragment` 剥离，连同 `script-text` & `page-module-registry` 共 **14/14 PASS**。

## Phase 165: Whitebox Unit Test Batch Recovery (`test/net` 11/11, `test/cdp` 22/27)
- 更新 12 个 `test/cdp/*.cp` 至 `[64]CdpEvent`、修复 `test_document_kind.cp` 与 `test_mock_fixtures.cp`，`test/net/` 达 **11/11 (100%)**、`test/cdp/` 达 **22/27 PASS**。

## Phase 166: Expose Global `window.Image` / `HTMLImageElement` & Diagnose 256MB QuickJS OOM
- **`src/js/shim/element/html-classes.js` & `src/js/runtime{,_source.cp}`**：补全 `globalThis`/`window.Image` 与 `HTMLImageElement`，精准定位 `__pressure()` `error: null` 源自 `session.cp:861` 256MB 堆上限。

## Phase 167: `navigate-reply-shape` 1/1 & `parser-script-lifecycle` 7/7 — 8/9 PASS
- **`src/cdp/{page,session_hooks,session,input}.cp`**：
  - 补全 `Page.navigate` `isDownload:false`、`ttyCellWidth` 注入、`<template>` 后代跳过、按 `_nid` 定位外部脚本并派发 `load`/`error` 事件（4 门禁全 0，**8/9 PASS**）。

## Phase 168: Whitebox Alignment (`test_page_navigation` 4/4, `test_runtime_workers` 3/3, `test_dom_events` 9/9, `test_js_dom` 8/9) & High-Velocity `GEMINI.md` Rule
- 更新 6 个白盒单测断言并固化 `GEMINI.md` 11.6「主智能体全量快扫 + 集中式带行号收口单」提速规范。

## Phase 169: `input-reply-origin` 1/1 PASS & `wasm-panic-unwind` Classification
- 零修改验收 `e2e/input-reply-origin.e2e.test.ts` (**1/1 PASS**)，归档 `wasm-panic-unwind` 为 Boa-Wasm/Rust panic 架构专属。

## Phase 170: `window.scheduler.postTask` & `scheduler.yield()` (`test/js/` 100% PASS)
- **`src/js/shim/window.js` & `src/js/runtime{,_source.cp}`**：实装 `Scheduler`、`postTask` 优先级/中止/延迟调度与 `scheduler.yield()`，`test/js/test_js_dom.cp` 达 **9/9 PASS**。

## Phase 171: Pure-CP UAX #9 BiDi Whitebox Suite `test/css/test_bidi.cp` (PASS 1~3)
- 新增 `test/css/test_bidi.cp`，完整覆盖 UAX #9 视觉重排、`dir="auto"`、`<bdi>`/`<bdo>` 级联（4 门禁全 0，`cap test -m` PASS）。

## Phase 172: `PointerEvent.getCoalescedEvents()` & Pointer/Mouse Boundary Event Ordering
- **`src/js/shim/{events/event,window}.js` & `src/js/runtime{,_source.cp}`**：实装 `getCoalescedEvents()` 与先于 `mouse*` 的 `pointerout`/`pointerleave`/`pointerover`/`pointerenter` 边界事件（4 门禁全 0）。

## Phase 173: Pure-CP CJK & SMP Color Emoji Rasterization Whitebox Suite `test/render/test_painter_cjk_emoji.cp` (3/3 PASS)
- 新增 `test/render/test_painter_cjk_emoji.cp`，覆盖 CJK 点阵、缩放裁剪与 BMP/SMP 彩色 Emoji 光栅化（`cap test -m` **3/3 PASS**, 940ms）。

## Phase 174: Pure-CP PagedMedia & Legacy Charset Whitebox Suite `test/css/test_paged_media.cp` (4/4 PASS)
- 新增 `test/css/test_paged_media.cp`，覆盖 `@page`、`break-*` 级联与 ISO-8859-15 UTF-8 转码（`cap test -m` **4/4 PASS**, 1174ms）。

## Phase 175: In-Memory `indexedDB`, `Path2D` Scanline Fill & `Mouse.click` `_skipSyntheticClick`
- **`e2e/harness.ts` & `src/js/shim/{window,canvas2d}.js` & `src/js/runtime{,_source.cp}`**：实装内存 `indexedDB`、`Path2D` SVG 路径解析与 `ctx.fill(Path2D)`、以及 `Mouse.click()` 重复点击去重（4 门禁全 0）。

## Phase 176: Pure-CP Color4, Calc & `light-dark()` Whitebox Suite `test/css/test_color_and_units.cp` (4/4 PASS)
- 新增 `test/css/test_color_and_units.cp`，覆盖 CSS Color 4 (`oklab`/`oklch`/`hwb`)、`calc`/`min`/`max`/`clamp`、`var()` 与 `light-dark()`（`cap test -m` **4/4 PASS**, 1437ms）。

## Phase 177: `Element.textContent` Setter `MutationRecord` Dispatch & `transientTargets` Cleanup
- **`src/js/shim/{node-base,mutations/observer}.js` & `src/js/runtime{,_source.cp}`**：修复 `textContent` setter 缺失旧子节点断开与 `MutationRecord` 派发，并清理 `transientTargets` 强引用（4 门禁全 0）。

## Phase 178: Pure-CP Transitions & `@keyframes` Whitebox Suite `test/css/test_transitions_keyframes.cp` (3/3 PASS)
- 新增 `test/css/test_transitions_keyframes.cp`，覆盖 `cubic-bezier`/`steps` 缓动、`TransitionItem`、`AnimationItem` 与 `@keyframes`（`cap test -m` **3/3 PASS**, 847ms）。

## Phase 179: Pure-CP Selectors (`:nth-child`/`:has`/`:is`/`:where`) Whitebox Suite `test/css/test_selectors_nth_has.cp` (4/4 PASS)
- 新增 `test/css/test_selectors_nth_has.cp`，覆盖结构伪类、`:has`/`:is`/`:where` 逻辑伪类、属性选择器与组合子（`cap test -m` **4/4 PASS**, 956ms）。

## Phase 180: Puppeteer `800x600` Default Viewport Reset & `Mouse.wheel` TTY Cell Grid Quantization
- **`e2e/harness.ts`**：统一默认视口至 `800x600` 并在用例间自动重置，同时在 `Mouse.wheel` 实现 `ttyCellWidth`/`ttyCellHeight` 整格量化与滚动链冒泡。

## Phase 181: Pure-CP Flex, Grid, BoxSizing & Sticky/Fixed Layout Whitebox Suite `test/layout/test_flex_grid_sticky.cp` (4/4 PASS)
- 新增 `test/layout/test_flex_grid_sticky.cp`，覆盖 Flexbox 换行对齐、Grid 轨道、`border-box`/`content-box` 与 `sticky`/`fixed`/`display: none`（`cap test -m` **4/4 PASS**, 1229ms）。

## Phase 182: Dedicated `_rafQueue` & `__capsurf_pump_raf` Driver Hooks
- **`src/js/shim/window.js` & `src/js/runtime{,_source.cp}`**：实装独立 `requestAnimationFrame` 帧队列、单共享 16ms 后备定时器与 `__capsurf_pump_raf`/`__capsurf_has_pending_raf` 钩子（4 门禁全 0）。

## Phase 183: Pure-CP HTML5 `<template>` Isolation & SVG Self-Closing Whitebox Suite `test/html/test_svg_and_template.cp` (3/3 PASS)
- 新增 `test/html/test_svg_and_template.cp`，覆盖 `<template>` 查询隔离、SVG 自闭合标签与 Adoption Agency 建树（`cap test -m` **3/3 PASS**, 989ms）。

## Phase 184: Static `<iframe>` Lifecycle Init, Child `innerWidth`/`innerHeight` & `evalInIframe` Return
- **`src/js/shim/{element/html-classes,window}.js` & `src/js/runtime{,_source.cp}`**：补齐静态 `<iframe>` 自动挂载、子窗口 `innerWidth`/`innerHeight` 与多语句 `evalInIframe` 返回值（4 门禁全 0）。

## Phase 185: Pure-CP BoxShadow, `clip-path`, `opacity` & `overflow: clip` Whitebox Suite (4/4 PASS)
- 新增 `test/render/test_box_shadow_and_clip.cp`，覆盖 `box-shadow: inset`、`clip-path: inset`、`opacity` 混合与 `overflow: clip` 像素验证（`cap test -m` **4/4 PASS**, 1595ms）。

## Phase 186: `iframe-dynamic-cdp-demo`, `oversize-body-streams` & `terminal-selection-after-navigation` — 3/3 Suites (21 Expects) + 512MB Heap
- **`src/cdp/{network,session,page,runtime,session_hooks,input}.cp` & `src/net/mock_fixtures.cp`**：
  - 实装动态 iframe CDP 事件推流、`/chatty-json` 256KB 保留上限、`ttyCellWidth` 拖拽选区同步与 512MB QuickJS 堆上限（4 门禁全 0，**3/3 PASS, 6.76s**）。

## Phase 187: Pure-CP `@container` & 64-Bit Cascade Weight Whitebox Suite `test/css/test_container_and_layers.cp` (4/4 PASS)
- 新增 `test/css/test_container_and_layers.cp`，覆盖 64 位级联权重、`@container` 尺寸重算与属性继承（`cap test -m` **4/4 PASS**, 1470ms）。

## Phase 188: `oopif-rerender` 1/1 PASS & `oopif-quads-paint` 8px Offset Localization
- 零修改验收 `e2e/demo/oopif-rerender.e2e.test.ts` (**1/1 PASS**) 与 `oopif-quads-paint` 跨域像素绕过用例，并定位 `TL @(25,25)` 偏移与 `oopif-prespawn` fixture 缺口。

## Phase 189: Pure-CP HTML5 Tokenizer & Entities Whitebox Suite `test/html/test_tokenizer_entities.cp` (4/4 PASS)
- 新增 `test/html/test_tokenizer_entities.cp`，覆盖命名/数值字符实体、DOCTYPE/属性与 RawText/ScriptData 切分（`cap test -m` **4/4 PASS**, 933ms）。

### Phase 190
- Added `test/dom/test_arena_mutations_and_clone.cp` covering Arena DOM sentinel, node creation, insert/remove/replace mutations, shallow/deep `cloneNode`, attributes, and `NULL_NODE`/OOB boundary guards.

### Phase 191
- Added `test/net/test_redirect_referrer.cp` covering redirect status/method downgrade, cross-origin sensitive header stripping, max-hops/loop stop reasons, referrer policy determination, and forbidden header filtering.

### Phase 192
- Added `test/eval/test_script_bundle_and_outbound.cp` covering script categorization/VLQ/bundle counts, learned-script promotion/eviction/stale blacklisting, and external/denied outbound policies.

### Phase 193
- Added `test/layout/test_box_sizing_and_overflow.cp` covering box-sizing modes, display/visibility layout flow, relative/absolute positioning, auto/negative margins, and min/max-width clamping.

### Phase 194
- Added `test/css/test_attr_evaluator_and_vars.cp` covering CSS `attr()` parsing/type-tainting, custom property inheritance/cycle guard, `calc()` evaluation, and `!important` cascade precedence.

### Phase 195
- Added `test/render/test_png_encoder.cp` covering PNG signature/IHDR/IDAT/IEND chunks, RGBA roundtrips, boundary guards, and `ComputedStyle.extractPaintStyle`.

### Phase 196
- Added `test/tty/test_nav_params.cp` covering TTY cell parsing/clamping, query param state preservation, and focus/UA stylesheet generation.

### Phase 197
- Added `test/js/test_qjs_interrupt_and_budget.cp` covering QuickJS reductions preemption, error recovery, multi-context isolation, and memory limit GC handling.

### Phase 198
- Added `test/dom/test_form_submission.cp` covering form method/enctype resolution, control entry filtering, URL-encoded serialization, and multipart/form-data boundary formatting.

### Phase 199
- Added `test/net/test_headers_and_mime.cp` covering HTTP header parsing/folding, control-character/CRLF validation, MIME charset parsing/normalization, and extension-to-MIME mapping.

### Phase 200
- Added `test/dom/test_canvas_element.cp` covering canvas dimension clamping, 2D context lifecycle, `toDataURL` PNG base64 encoding, and multi-instance isolation.

### Phase 201
- Added `test/html/test_html_parser_edge_cases.cp` covering HTML5 unclosed/void/implicit-table recovery, raw-text script/style spans, and `tag_spans.cp` scanning.

### Phase 202
- Added `test/eval/test_font_shaping.cp` covering supported font format magic detection, font byte shaping/truncation, 4-slot concurrent transform budget, and oversize/invalid input guards.

### Phase 203
- Added `test/net/test_cookie_jar_registry.cp` covering cookie jar registry isolation, domain/path matching, capacity/key-length bounds, expiry purging, and connection-level cleanup.

### Phase 204
- Added `test/eval/test_next_route_chunks_and_limits.cp` covering Next.js build manifest parsing/ranking, chunk limits, and LimitRegistry lifecycle/eviction/isolation.

### Phase 205
- Added `test/eval/test_oopif_render_and_page_renderer.cp` covering OOPIF viewport clamping, 16-slot coordinator reuse, and PageRenderer lazy/dispose/close state transitions.

### Phase 206
- Updated `src/cdp/page.cp`, `src/cdp/session.cp`, `src/cdp/target.cp`, `src/net/mock_fixtures.cp`, and `src/js/shim/window.js` (re-bundled to `runtime.js` / `runtime_source.cp`) to close `navigate-style-script-order`, `iframe-dom-reclaim`, and `post-load-settle`.

### Phase 207
- Added `test/net/test_http_loader.cp` covering offline HTTP/data URL parsing, relative resolution, redirect-depth/oversize guards, and mock fixture/Fetch interception.

### Phase 209
- Added `test/eval/test_dedicated_worker_and_clock.cp` covering DedicatedWorker lifecycle/registry, HostClock/WallClock tick progression, and SpanSink/TailTraceRegistry truncation & eviction bounds.

### Phase 210
- Added `test/eval/test_prefetch_scripts_and_wasm.cp` covering script prefetch origin classification/caps, Wasm header/manifest validation, and inline/URL Wasm literal scanning.

### Phase 211
- Added `test/eval/test_page_script_and_tail.cp` covering page script cache keys/formatting, tail log truncation/OTel serialization, and RPC session inflight/timeout/teardown tracking.

### Phase 212
- Added `test/eval/test_engine_errors_and_isolate_id.cp` covering error origin classification/parsing, isolate UUIDv4 identity lifecycle, and page isolate entry source validation.

### Phase 213
- Added `test/dom/test_mutation_observer_edge.cp` covering `MutationObserver` / `MutationManager` queue bounds, disconnect/re-observe, and attribute/childList filtering.
- Fixed `src/cdp/screencast.cp:210` wrapping multiply syntax (`*%`) and committed in-progress CDP screencast frame-hash/throttling/rAF-driver baseline (`src/cdp/{screencast,server,kitesurf_facade,page}.cp`, `src/js/shim/window.js`, `src/js/runtime.js`, `src/js/runtime_source.cp`), plus `GEMINI.md` / `CP_PITFALLS.md` updates.

### Phase 214
- Added `test/css/test_font_face_and_ua.cp` and `test/html/test_parser_and_tree_builder.cp`.
- Fixed `HtmlTag.fromStr` length buckets for `"noembed"` and `"plaintext"` in `src/html/tags.cp`.
- Implemented `JS_SetModuleLoaderFunc2` import attributes (`with { type: "json" }` / `"css"` rejection) and `Kitesurf.getMemoryStats` (`domNodes` + `wrappers` weak-ref tracking) in `src/cdp/session.cp` and `src/js/dom_binding.cp`.

### Phase 215
- Fixed `ScriptFetchCallback` slice out-parameter lowering in `src/eval/prefetch_scripts.cp` (`ScriptBodyOut`), `src/eval/external_outbound.cp`, and added `testPrefetchSingleAndAll` in `test/eval/test_prefetch_scripts_and_wasm.cp`.
- Implemented OOPIF child-frame `isDefault: false`, `Runtime.executionContextDestroyed`, `DOM.getFrameOwner`, `frameId`/`documentURL`, and child-frame `DOM.getDocument`/`getOuterHTML`/`getAttributes` in `src/cdp/{page,session,runtime,target,dispatch,dom}.cp`.
- Updated `GEMINI.md` section `11.6` with port zombie pre-check and slice out-param patterns.

### Phase 216
- Fixed `TreeBuilder.processAfterHead` in `src/html/tree_builder.cp` to handle head-like tags (`<script>`, `<style>`, etc.) between `</head>` and `<body>` without inserting an implicit `<body>`.
- Added `/oopif-child-solid.html` and `/oopif-host-many.html` dynamic fixtures in `src/net/mock_fixtures.cp`.
- Enforced `MAX_CONCURRENT_OOPIF` budget for cross-origin iframe rendering in `src/cdp/session.cp`.

### Phase 217
- Added `test/html/test_after_head_and_foreign.cp` and `test/layout/test_flex_and_table_layout.cp`.
- Implemented distinct `content`/`padding`/`border`/`margin` quads and content-box dimensions for `DOM.getBoxModel` in `src/cdp/{types,session,dom}.cp`.
- Deduplicated `onclick` dispatch in `src/cdp/input.cp`, synced `value` attribute in `applyInputText`, fixed `KeyboardEvent` `Enter` `charCode = 13` in `src/js/shim/events/event.js` (with `runtime.js`/`runtime_source.cp` re-bundle), and updated `test/cdp/test_screenshot_input.cp`.

### Phase 218
- Added `test/css/test_selectors_and_pseudo_classes.cp` covering structural/logical pseudo-classes, attribute operators, combinators, and cascade specificity.
- Tracked `retired_isolated_world_ids` in `src/cdp/session.cp` and aligned `Runtime.evaluate` invalid vs retired `contextId` responses in `src/cdp/runtime.cp`.

### Phase 219
- Added `test/dom/test_inner_html_and_serialization.cp` and `test/dom/test_query_selector_edge.cp`.
- Fixed duplicate mouse boundary event dispatch in `e2e/harness.ts` `Mouse.move` fallback.

### Phase 220
- Added `test/render/test_rasterizer_and_clip_edge.cp`.
- Fixed `IndexedDB` `objectStoreNames` TDZ in `src/js/shim/window.js`, `CanvasRenderingContext2D` `fill`/`stroke` `ensureBitmap` order in `src/js/shim/canvas2d.js`, and `PointerEvent.getCoalescedEvents` `isTrusted` check in `src/js/shim/events/event.js` (re-bundled `runtime.js`/`runtime_source.cp`).

### Phase 221
- Added `test/tty/test_tty_render_and_selection_edge.cp`.
- Implemented screencast wheel coalescing, per-axis `overscrollBehavior`, and screencast owner transfer/validation across `src/cdp/{types,screencast,page,input,server}.cp`, `src/js/shim/window.js` (+ `runtime.js`/`runtime_source.cp`), and `e2e/harness.ts`.

### Phase 222
- Added `test/net/test_mock_fixtures_edge.cp`.
- Optimized `captureScreencastFrame` viewport scaling & compression in `src/cdp/session.cp`, added `pumpTimers()` on `pr > 0` in `src/cdp/server.cp`, and added 32-task timer batching + `_rafDriverGraceMs` in `src/js/shim/window.js` (+ `runtime.js`/`runtime_source.cp`).

### Phase 223
- Added `test/js/test_qjs_eval_and_error_edge.cp`.
- Added native checkbox checked painting and `viewport_scroll_x`/`viewport_scroll_y` in `src/render/painter.cp`, live CSS animation sampling overlay in `src/cdp/session.cp`, and animation interpolation/direction fixes in `src/js/shim/animations.js` (+ `runtime.js`/`runtime_source.cp`).

### Phase 224
- Added `test/render/test_checkbox_and_scroll_paint.cp`.
- Reset screencast state across session/target/harness boundaries (`src/cdp/session.cp`, `src/cdp/target.cp`, `e2e/harness.ts`), synced scroll & JPEG compression in `Page.captureScreenshot` (`src/cdp/page.cp`), added `Element.prototype.scrollTo/scroll/scrollBy` (`src/js/shim/element/element.js` + `runtime.js`/`runtime_source.cp`), and tuned scaled text `0.75` cap-height & `M/m` glyph in `src/render/painter.cp`.

### Phase 225
- Added `test/css/test_transform_and_overflow_edge.cp`.
- Reinitialized `JsRuntime` on navigation in `src/cdp/session.cp` and switched `nodeListenersMap` to `WeakMap` in `src/js/shim/events/dispatch.js` (+ `runtime.js`/`runtime_source.cp`).

### Phase 226
- Added `test/dom/test_attr_and_classlist_edge.cp`.
- Verified `e2e/detached-owned-caches.e2e.test.ts` 5/5 PASS.

### Phase 227-228
- Added `2000x2000` scrollable spacer to `test/cdp/test_screenshot_input.cp` (117/117 whitebox PASS).
- Fixed empty `<style>` `innerText` recursion (`src/js/shim/window.js`), `#text`/`#comment` prototype & `data` setter (`src/js/shim/{index.js,node-base.js,element/element.js}`), and `MutationObserver` `transientTargets` flush (`src/js/shim/mutations/observer.js` + `runtime.js`/`runtime_source.cp`).
- Implemented `emitNewChildFrameEvents` context emission, `TargetRegistry.registerIframeTarget` URL updates, `documentKindStats`, and `Kitesurf.evalDocumentKind` across `src/cdp/{page,target,session,kitesurf_facade,protocol_extensions}.cp`.
- Updated `GEMINI.md` section `11.6` with rule #6.

### Phase 229
- Implemented flat-session child event routing (`e2e/harness.ts`), iframe `Runtime.enable`/`Network.enable` session routing (`src/cdp/{runtime,network,session,dispatch,page,session_hooks}.cp`), `activeWindowStack` fetch origin & `setAttribute("src")` iframe navigation (`src/js/shim/{window.js,element/element.js,element/html-classes.js}` + `runtime.js`/`runtime_source.cp`), and `/oopif-kind-*` mock fixtures (`src/net/mock_fixtures.cp`).

### Phase 230
- Added `test/dom/test_forms_and_validation_edge.cp`.
- Fixed `CSSStyleValue` constructor name (`src/js/shim/index.js`), `matchMedia` evaluation, `history.scrollRestoration`, and top-level `onhashchange`/`onpopstate` in `src/js/shim/window.js` (+ `runtime.js`/`runtime_source.cp`).

### Phase 231
- Added `test/dom/test_template_inert_and_script_edge.cp`.
- Implemented `[LenientThis]`, `HTMLBodyElement` window-reflecting handlers, top-level `window` own `on*` accessors, bounded `window.onerror`, and inline handler CSP alignment in `src/js/shim/{element/html-classes.js,window.js,events/dispatch.js}` (+ `runtime.js`/`runtime_source.cp`) and `src/cdp/session_hooks.cp`.
- Added `GEMINI.md` section `11.7` self-evolving efficiency rules.

### Phase 232
- Added `test/css/test_shorthand_and_computed_edge.cp` and `test/html/test_entities_and_tokenizer_edge.cp`.
- Fixed numeric entity overflow clamping in `src/html/entities.cp`.
- Fixed double wheel scrolling (`e2e/harness.ts`), Shadow DOM wheel traversal & TTY cell snapping (`src/js/shim/window.js` + `runtime.js`/`runtime_source.cp`), `#fragment` pre-scroll layout (`src/cdp/session.cp`), and `Emulation.setDeviceMetricsOverride` scroll reset (`src/cdp/emulation.cp`).

### Phase 233
- Added `test/css/test_calc_and_colors_edge.cp` and `test/layout/test_grid_and_aspect_ratio_edge.cp`.
- Fixed held-button drag `mouseMoved` coalescing and child-frame `Page.frameNavigated` URL overwrite in `e2e/harness.ts`, and finite `@keyframes` `animation-iteration-count` clamping in `src/cdp/session_hooks.cp`.

### Phase 233
- Added `test/css/test_calc_and_colors_edge.cp` and `test/layout/test_grid_and_aspect_ratio_edge.cp`.
- Fixed held-button drag `mouseMoved` coalescing and child-frame `Page.frameNavigated` URL overwrite in `e2e/harness.ts`, and finite `@keyframes` `animation-iteration-count` clamping in `src/cdp/session_hooks.cp`.

### Phase 234
- Added `test/eval/test_origin_and_csp_edge.cp` and `test/net/test_fetch_and_cors_edge.cp`.
- Implemented `Page.startScreencast` viewport sync & `MAX_CONCURRENT_OOPIF` context budgeting (`src/cdp/{page,session}.cp`), `DOM.getNodeForLocation` (`src/cdp/dom.cp`, `e2e/harness.ts`), and `createHTMLDocument` `parentNode` / `isSameTarget` fixes (`src/js/shim/{index.js,mutations/observer.js}` + `runtime.js`/`runtime_source.cp`).

### Phase 235
- Added `test/dom/test_node_traversal_and_clone_edge.cp`.
- Implemented `data:` ES module resolution, `__capsurf_eval_module_script`, iframe `<template>` inertness + meta CSP + `load` event, `Page.setDocumentContent` URL normalization, and TodoMVC TTY UA stylesheet (`src/cdp/{page,session,session_hooks}.cp`, `src/net/mock_fixtures.cp`, `src/tty/nav_params.cp`, `src/js/shim/{selector.js,window.js,element/html-classes.js}` + `runtime.js`/`runtime_source.cp`).

### Phase 236
- Added `test/tty/test_tty_todomvc_ua_css.cp` and updated `test/tty/test_nav_params.cp` for `ks_tty_css=0` reset semantics.
- Fixed child-frame `DOM.documentUpdated` session routing (`src/cdp/dom.cp:476`), wired `domDebug.renderRbiResources` (`src/js/dom_binding.cp`), and added `/font-key-images.html` fixtures (`src/net/mock_fixtures.cp`, `src/render/painter.cp`).

### Phase 237-238
- Added `test/render/test_image_and_font_key_edge.cp` and `test/dom/test_dom_events_and_propagation_edge.cp`; exposed `pub fn loadBgPngBytes` in `src/render/painter.cp`.
- Implemented multi-tab `CdpPageSession` isolation (`tab_sessions`) across `src/cdp/{server,session,page}.cp` and `e2e/harness.ts`.

### Phase 237-238
- Added `test/render/test_image_and_font_key_edge.cp` and `test/dom/test_dom_events_and_propagation_edge.cp`; exposed `pub fn loadBgPngBytes` in `src/render/painter.cp`.
- Implemented multi-tab `CdpPageSession` isolation (`TabSessionSlot`) in `src/cdp/server.cp`, fixed `performPendingNavigation` `JSValue` free order in `src/cdp/page.cp`, and propagated `browserContextId` in `e2e/harness.ts`.
- Added `ulimit -s 65536` whitebox rule to `GEMINI.md` section `11.7`.

### Phase 239
- Added `test/css/test_media_query_and_supports_edge.cp`.
- Implemented `DedicatedWorkerGlobalScope`, `Worker`/`SharedWorker`, `MAX_WORKER_SLOTS = 1`, `worker` CSP/redirect gate, `ArrayBuffer` transfer, `SharedArrayBuffer` `DataCloneError`, and `ImageData` validation across `src/cdp/session_hooks.cp`, `src/net/mock_fixtures.cp`, `src/js/shim/{window.js,canvas2d.js,element/html-classes.js}` (+ `runtime.js`/`runtime_source.cp`).

### Phase 240
- Added `test/eval/test_worker_and_csp_redirect_edge.cp`.
- Added `bridge.healedAtEntry` to `Kitesurf.getMemoryStats` in `src/cdp/session.cp` and `src/cdp/protocol_extensions.cp`.

### Phase 241
- Added `test/css/test_box_shadow_and_border_radius_edge.cp`.
- Implemented `Kitesurf.sceneFrame` binary frame emission, heartbeat, idle-skip, and terminal wheel coalescing across `src/cdp/{scene_stream,kitesurf_facade,dispatch,server}.cp` and `src/js/shim/{window.js,element/element.js}` (+ `runtime.js`/`runtime_source.cp`).

### Phase 242
- Updated `test/cdp/test_scene_stream_facade.cp` and added `test/cdp/test_scene_stream_envelope_edge.cp`.
- Implemented `MSG_PARTIAL (5)` retained-frame RGBA diffing, `renderDocumentToRgba`/`encodeRgbaRectToPng`, `/rbi/scene_host.json` + in-poll HTTP serving, and `SCENE_HOST_WASM` shim across `src/cdp/{scene_stream,session,server}.cp`, `src/render/painter.cp`, and `e2e/shim/cloudflare-test.ts`.

### Phase 243
- Added `test/render/test_rgba_dirty_rect_encode_edge.cp`.
- Implemented `BroadcastChannel` in `src/js/shim/window.js` (+ `runtime.js`/`runtime_source.cp`).

### Phase 244
- Added `test/dom/test_shadow_dom_and_slot_edge.cp`.

### Phase 245
- Added `test/css/test_specificity_and_important_edge.cp`.
- Added `concimport-chunk-<N>.js` dynamic route in `src/net/mock_fixtures.cp`.

### Phase 246
- Built modular ReleaseFast binary (`cap build -m -r -o bin_rel src/main.cp`, synced to `bin/main`, 18MB) and verified 12-suite cross-domain E2E regression (77/77 PASS, 457 assertions).

## Phase 247 (2026-11-21)
- Removed 392 redundant `e2e/*.e2e.test.js` copies, `e2e/run_all.js`, and `e2e/run_all.cjs`.
- Updated `GEMINI.md` section 10 to standardize E2E execution on `bun test` over `e2e/*.e2e.test.ts` (392) and `e2e/demo/*.e2e.test.ts` (172).

## Phase 248 (2026-11-21)
- Fixed CDP BrowserContext cookie/target isolation, live AXTree state, border-box `getBoxModel`, and viewport inheritance in `src/cdp/`.
- Implemented CSS counters (`counter-reset`/`counter-increment`/`counter()`/`counters()`/`attr()`), `<ol start/reversed>` + `upper-roman` markers, pseudo-element `visibility`, collapsed-border table width, WebFont weight scaling, and single-scroll root guard across `src/css/`, `src/layout/`, and `src/render/`.
- Fixed `HTMLImageElement.getBoundingClientRect` layout override, CSS Typed OM `computedStyleMap`, rAF overrun frame gap, WebRTC stubs, XHR/WS boot, unforgeable `location`, `form` named overrides, and shadow directionality in `src/js/shim/`.

## Phase 249 (2026-11-21)
- Fixed CDP interception/redirect/per-hop gating, renderer laziness, OOPIF runtime evaluate, and tab session inheritance in `src/cdp/`.
- Fixed `Range.createContextualFragment`, Shadow DOM `_shadowRoot`, XPath `DOMParser` nodes, `no-cors` opaque headers/images, `canvas2d` un-upgraded bitmap sizing, `form.reset()`, `<select>` dropdown activation, keyboard modifiers/scrolling, `focus()` scroll preservation, `executeConnectedScripts`, and sandboxed `iframe`/`srcdoc`/Worker origin isolation in `src/js/shim/`.
- Added `sandbox-*` fixture routes in `src/net/mock_fixtures.cp`, deduplicated `js_capsurf_native_record_fetch` in `src/js/dom_binding.cp`, and removed duplicate synthetic `Network.requestWillBeSent` emissions in `e2e/harness.ts`.

## Phase 250 (2026-11-21)
- Fixed `fetch-slot-order`, `iframe-referrer` nested policy propagation, `navigate-window-cookies` evaluate `Object.prototype.then` cookie pump, and `oopif-late-spawn`/`oopif-postmessage`/`oopif-rerender` in `src/cdp/page.cp` and `src/cdp/session.cp`.
- Added whitebox tests `test/css/test_css_counters_and_list_style.cp` and `test/net/test_sandbox_fixtures.cp`, and aligned `test_js_dom.cp`, `test_cdp.cp`, and `test_playwright_e2e.cp` (`138/138` whitebox PASS).

## Phase 251 (2026-11-21)
- Added `scripts/e2e_suite.py` concurrent E2E runner with explicit 28-suite Kitesurf-architecture skip classification.
- Fixed `picture-srcset-demo`, `message-channel-task-queue`, `opaque-sheet-text`, and `parser-script-lifecycle` in `src/js/shim/` (`html-classes.js`, `window.js`, `node-base.js`), and deduplicated `fireScriptEvent` in `src/cdp/session_hooks.cp`.
- Fixed `navigation-fidelity` hop-0 `requestWillBeSent`, `oopif-heap-usage` `Runtime.enable` context re-emit, `source-script-pipeline` + `oopif-runtime-evaluate` iframe classic/module global bindings, and `screencast-pipeline-order` immediate first frame in `src/cdp/` (`page.cp`, `runtime.cp`, `session_hooks.cp`, `server.cp`).
- Renamed `hexVal` to `docKindHexVal`/`regexHexVal`/`serverHexVal` across `src/cdp/`.

## Phase 252 (2026-11-22)
- Codified 3-lane `-m -d` fast-build isolation matrix (`bin_19231`–`bin_19233`) vs `-m -r` release sync in `GEMINI.md`.
- Fixed `FileReader` macrotask/microtask boundary in `src/js/shim/window.js` and eliminated $O(2^N)$ shadow/sibling recursion in `src/js/shim/element/element.js`.
- Preserved `[native code]` `Function` wrapper shape in `src/cdp/session_hooks.cp`, paired stylesheet response bodies via `recordPendingFetchBody` in `src/cdp/session.cp`, and deduplicated `recordResponseBody` slots in `src/cdp/network.cp`.

## [2026-11-22] Phase 253: 统一测试目录拓扑 (`test/{unit,e2e,wpt,pptr,pw}`)
- **Moved**: `test/{cdp,css,dom,eval,html,js,layout,net,render,tty}/` -> `test/unit/...`（批量更新 137 个 `.cp` 导入路径至 `../../../src/`）；`e2e/` -> `test/e2e/`；清理遗留 `tests/`。
- **Added**: `test/wpt/`（`wpt-*.json` + `scripts/wpt-*.mjs`）、`test/pptr/`（`pptr-*.json` + `scripts/pptr-*.mjs`）、`test/pw/`（`pw-*.json` + `scripts/pw-*.mjs`）。
- **Updated**: `cap.json`, `bunfig.toml`, `package.json`, `tsconfig.json`, `test/e2e/tsconfig.json`, `test/e2e/preload.ts`, `test/e2e/harness.{ts,js}`, `scripts/e2e_suite.py`, `src/net/mock_fixtures.cp`, `src/render/painter.cp`, `GEMINI.md`；并为 `src/js/{qjs_engine,dom_binding}.cp`、`src/cdp/{session,session_hooks,runtime}.cp`、`src/render/{painter,png}.cp` 补齐 `unsafe { ... }`（`E0209`）。

## [2026-11-22] Phase 254: 138/138 白盒全绿、CDP/JS-Shim 协议补齐与 PPTR/PW/WPT 实测基线
- **Fixed (`src/`)**: `src/cdp/server.cp`, `src/cdp/target.cp`, `src/cdp/runtime.cp`, `src/cdp/session.cp`, `src/cdp/page.cp`, `src/cdp/types.cp`, `src/net/http_loader.cp`, `src/js/dom_binding.cp`, `src/js/shim/{index,window}.js`, `src/js/runtime_source.cp`（补齐 `sessionId` 回传、`frame_id === targetId`、`Page.close`、`Runtime.callFunctionOn` 实参/简写/特殊值、`addScriptToEvaluateOnNewDocument`、`HttpLoader` chunked/loopback、`getElementsByTagName` 与 `window.parent`）；并完成 `test/unit/` 138/138 符号去重与 `E0209` `unsafe` 适配。
- **Updated (`test/`)**: `test/e2e/trusted-fetch-metadata.e2e.test.ts`（`../../vendor/`）；`test/wpt/scripts/wpt-exec.mjs`、`wpt-batched.mjs`、`test/pptr/scripts/pptr-run.mjs`、`test/pw/scripts/pw-run.mjs`。

## [2026-11-22] Phase 255: CDP `normalizeExceptionMessage` 动态缓冲、`Target.detachedFromTarget` 与 `setContent` 生命周期修复
- **Fixed**: `src/cdp/runtime.cp`（动态分配 `norm_buf` 修复 >8KB JSON 截断）、`src/cdp/target.cp`（`Target.closeTarget` 补发 `Target.detachedFromTarget`）、`src/cdp/session.cp`（`pollReopenLoadEvents` 补齐 `Page.lifecycleEvent`）。

## [2026-11-22] Phase 256: Playwright `ElementHandle` 链路打通与 CDP `Network.*` JSON 转义修复
- **Fixed (`src/`)**:
  - `src/js/dom_binding.cp`: `__capsurf_format_remote_object` 补齐 `subtype: 'node'`/`date`/`regexp`/`error`/`map`/`set`；`__capsurf_get_object` 支持 `node:<nid>`；`_wrappedGcs` 补充 `visibility` 默认 `'visible'`；`el_proto` 新增 `nodeValue` getter/setter。
  - `src/cdp/dom.cp`: `DOM.resolveNode` 兼容 `backendNodeId`；`DOM.getBoxModel`/`DOM.getContentQuads` 补齐 `session.ensureLayout()`。
  - `src/cdp/runtime.cp`: 实现 `Runtime.getProperties` 真实属性枚举。
  - `src/cdp/session.cp`: `Network.requestWillBeSent`/`Network.responseReceived` 使用 `escapeJsonString` 转义 `ref_esc` 与 `fetch_esc`。

## [2026-11-22] Phase 257: `flushPendingStyleChecks` 性能优化、`elementsFromPoint` 与 `document.close` 同步修复
- **Fixed (`src/`)**:
  - `src/js/shim/animations.js`（同步重新生成 `src/js/runtime.js`、`src/js/runtime_source.cp`）：`flushPendingStyleChecks` 增加 `_pageHasAnimOrTrans` 门控，避免无动画页面批量 `style` 修改触发 $O(N^2)$ `getComputedStyle`。
  - `src/js/shim/index.js`: 补齐 `docProto.elementsFromPoint(x, y)`，打通 Playwright `click`/`check`/`uncheck`/`hover`。
  - `src/cdp/session_hooks.cp`: `document.close()` 修正调用 `g.__capsurf_syncNamedGlobals()`。

## [2026-11-22] Phase 258: `:hover` 动态伪类、Shadow DOM `node:<nid>` 查找与 HTML 属性实体解码
- **Added / Fixed (`src/`)**:
  - `src/dom/dom.cp`, `src/css/matcher.cp`, `src/cdp/input.cp`: 实现 `:hover` 动态伪类（`hovered_node` 状态 + 祖先链匹配 + `mouseMoved` 命中更新）。
  - `src/js/dom_binding.cp`: `__capsurf_get_object` 解析 `node:<nid>` 时遍历 `shadowRoot`。
  - `src/html/tree_builder.cp`: `insertElement` 新增 `decodeAttrValue()` 对含 `&` 属性值调用 `decodeEntity(..., true)` 解码 HTML 实体。

## [2026-11-22] Phase 259: CSS 属性选择器转义引号、Detached 节点回查与 `number` 输入框光标修复
- **Fixed (`src/`)**:
  - `src/css/matcher.cp`: `[attr="...\"..."]` 引号扫描与 `\c` 反转义支持。
  - `src/js/dom_binding.cp`: `__capsurf_get_object("node:<nid>")` 补充 `__capsurf_wrappers` / `__capsurf_weak_wrappers` 回查 detached 节点。
  - `src/cdp/input.cp`: `applyInputText` 为 `input[type=number]` 增加 `_numCaret`（默认 `0` 起步）修复文本插入位置。

## [2026-11-22] Phase 260: `Page.reload` 真实导航、`addScriptTag` 外部脚本拉取与 `invalid-type` 选区修复
- **Added / Fixed (`src/`)**:
  - `src/cdp/page.cp`: 移除 `Page.reload` 空存根，将其路由至 `Page.navigate` 导航管线（栈缓冲拷贝 `session.url`），支持完整页面重载与 JS 环境重置。
  - `src/cdp/session_hooks.cp`, `src/cdp/session.cp`: 新增 `__capsurf_http_get` 原生桥接回调（复用 `HttpLoader.fetchDocument`），为 `handleInsertedScript` 补充经典外部 `<script src="...">` 的真实 HTTP 拉取、全局求值与 `load`/`error` 事件派发。
  - `src/js/shim/element/forms/selection.js` (+ `src/js/runtime.js`, `src/js/runtime_source.cp`): `supportsSelection()` 改用 `normalizeInputType()` 规范化 `type` 属性，修复 `<input type="invalid-type">` 下 `select()` 失效导致 `page.fill()` 追加而非替换的问题。
- **Added (`test/`)**:
  - `test/TEST_LEDGER.md`: 建立持久化测试台账，记录已测分片时间与结果，避免重复测试。

## [2026-11-22] Phase 261: 键盘光标导航 (`ArrowLeft`/`ArrowRight`/`Home`/`End`)、`preventDefault` 拦截、`textInput` 事件与 `MouseEvent.detail`
- **Added / Fixed (`src/`)**:
  - `src/cdp/input.cp`: `applyInputText` 与 `Input.dispatchKeyEvent`（含 `.RawKeyDown`）新增 `ArrowLeft`/`ArrowRight`/`Home`/`End` 光标/选区移动支持；`applyInputText` 检查 `globalThis.__capsurf_last_key_prevented` 拦截被 `preventDefault()` 的文本输入；`dispatchMouseDomEvent` 为 `mousedown`/`mouseup`/`click` 补充 `detail: 1`。
  - `src/js/shim/window.js` (+ `src/js/runtime.js`, `src/js/runtime_source.cp`): `keypress` 阶段同步暴露 `globalThis.__capsurf_last_key_prevented`，并为 `<input>`/`<textarea>` 派发 `textInput` 事件。
- **Updated (`test/`)**:
  - `test/TEST_LEDGER.md`: 持续登记 WPT/PW 各批测试结果与耗时。

## [2026-11-22] Phase 262: `document.activeElement`、`DOM.focus` 与可聚焦元素校验
- **Added / Fixed (`src/`)**:
  - `src/cdp/dom.cp`: 移除 `DOM.focus` 空存根，实现 `nodeId`/`backendNodeId`/`objectId` 解析并同步 `session.focused_node` 与 JS `el.focus()` 派发。
  - `src/js/shim/index.js` (+ `src/js/runtime.js`, `src/js/runtime_source.cp`): 在 `docProto` 补齐 `activeElement` getter/setter（未聚焦或节点已脱离 DOM 时回退 `body || documentElement || null`）。
  - `src/js/shim/element/html-element.js`: `proto.focus()` 增加 `isFocusableElement()` 校验及同元素重复聚焦去重，确保 `focus`/`blur` 事件序列与浏览器一致。

## [2026-11-22] Phase 263: `XMLSerializer` 实现与 `page.content()` 序列化支持
- **Added (`src/`)**:
  - `src/js/shim/parser.js`, `src/js/shim/index.js` (+ `src/js/runtime.js`, `src/js/runtime_source.cp`): 实现并暴露 `XMLSerializer`（支持 Document、DocumentType、Element、Text、Comment、ProcessingInstruction、DocumentFragment 序列化），打通 Playwright `page.content()` 与 `page.setContent()` 闭环。

## [2026-11-22] Phase 264: `KeyboardEvent.location`、`Tab` 焦点去重与 CDP 绑定事件刷新
- **Added / Fixed (`src/`)**:
  - `src/js/shim/events/event.js` (+ `src/js/runtime.js`, `src/js/runtime_source.cp`): `KeyboardEvent` 根据 `code` 自动推导 `location`（`Left` -> `1`, `Right` -> `2`, `Numpad` -> `3`, 默认 `0`），修复 `ShiftLeft`/`ShiftRight` 等修饰键位置判断。
  - `src/js/shim/window.js`: 清理 `Tab` 重复处理逻辑，恢复原生 `navigateTabFocus(isShift)` 单步焦点遍历。
  - `src/cdp/dom.cp`, `src/cdp/input.cp`: 为 `DOM.focus` 与 `Input.dispatchMouseEvent`/`Input.dispatchKeyEvent` 补充 `session.pollBindingEvents()` 刷新，并在 `mousedown` 命中链路补充可聚焦元素冒泡聚焦。

## [2026-11-22] Phase 265: `document.open/write/close` 重置 `head`/DOCTYPE 与 `XMLSerializer` 联动修复
- **Fixed (`src/`)**:
  - `src/cdp/session_hooks.cp`: `document.close` 解析 `reopenBuf` 中的 `<!DOCTYPE ...>`、`PUBLIC`/`SYSTEM` 标识及 `<head>`/`<body>`，清空残留 `<title>Blank Page</title>` 并记录 `_reopenDoctype`。
  - `src/js/shim/parser.js` (+ `src/js/runtime.js`, `src/js/runtime_source.cp`): `XMLSerializer` 序列化 `Document` 与 `DocumentType` 时读取 `ownerDoc._reopenDoctype`，使 `page.setContent()` + `page.content()` 在无 doctype、HTML5 与 HTML4 doctype 下均与 Chromium 完全一致。

## [2026-11-22] Phase 266: `dispatchMouseDomEvent` 缓冲溢出修复与 `indeterminate` 复选框清除
- **Fixed (`src/`)**:
  - `src/cdp/input.cp`: 将 `dispatchMouseDomEvent` 的 `js_buf` 从 `[1280]u8` 扩容至 `[2048]u8`，修复模板长度（1403 字节）触发 `bufPrint` 溢出静默 `catch return true` 导致点击与焦点完全失效的问题。
  - `src/js/shim/window.js`, `src/js/shim/element/html-element.js` (+ `src/js/runtime.js`, `src/js/runtime_source.cp`): 点击 `checkbox` 切换 `checked` 时同步清除 `indeterminate = false`，修复 `locator.uncheck()` 在 `indeterminate` 复选框上的状态校验失败。

## [2026-11-22] Phase 267: 折叠 `<details>` 子树隐藏与 `isGenuineNode` `Node` 构造器缓存
- **Fixed (`src/`)**:
  - `src/css/cascade.cp`: 在 `Cascade.compute` 初始化节点样式时检测父节点为未展开 `<details>`（无 `open` 属性）的非首个 `<summary>` 子元素并设置 `Display.none()`，修复折叠 `<details>` 内子元素仍被判定为 `visible` 的问题。
  - `src/js/shim/node-base.js` (+ `src/js/runtime.js`, `src/js/runtime_source.cp`): `isGenuineNode()` 缓存初始 `_NodeCtor`，避免页面脚本移除 `window.Node` 后触发 `ReferenceError`。

## [2026-11-22] Phase 268: `Runtime.callFunctionOn` 补齐 `pollBindingEvents` 与 `pollNavigatedWithinDocEvents`
- **Fixed (`src/`)**:
  - `src/cdp/runtime.cp`: 为 `Runtime.callFunctionOn` 补充 `session.pollBindingEvents()` 与 `session.pollNavigatedWithinDocEvents()` 事件轮询，修复 `page.evaluate(() => ...)` 中 `location.hash`、`history.pushState` 及 `exposeFunction` 回调未同步 `page.url()` / 绑定事件的问题。

## [2026-11-22] Phase 269: 消除 `console` 重复事件并补齐 `console.time`/`count`/`assert`/`trace` API
- **Fixed (`src/`)**:
  - `src/cdp/session.cp`: 移除 `pollConsoleEvents` 中重复发射的 `Log.entryAdded`，使每次 `console.*` 调用仅发射一条 `Runtime.consoleAPICalled`。
  - `src/cdp/runtime.cp`: 将 `Runtime.callFunctionOn` 与 `Runtime.evaluate` 的 `polled_evts` 缓冲区从 `[4]CdpEvent` 扩容至 `[16]CdpEvent`，避免单次求值多条控制台输出被截断。
  - `src/js/shim/window.js` (+ `src/js/runtime.js`, `src/js/runtime_source.cp`): 补齐 `console.time`/`timeLog`/`timeEnd`/`count`/`countReset`/`assert`/`dir`/`dirxml`/`table`/`trace`/`group`/`groupCollapsed`/`groupEnd`/`clear`。

## [2026-11-22] Phase 270: `DOM.getBoxModel` 与 `DOM.getContentQuads` 视口滚动偏移扣除
- **Fixed (`src/`)**:
  - `src/cdp/dom.cp`: 新增 `shiftQuadScroll()` 辅助函数，在 `DOM.getBoxModel` 与 `DOM.getContentQuads` 中调用 `session.updateLayoutMetrics(&sx, &sy, &cw, &ch)` 并扣除视口滚动偏移 `(sx, sy)`，使滚动后的 `ElementHandle.boundingBox()` 返回准确的视口相对坐标。

## [2026-11-22] Phase 271: 动态 `<link rel="stylesheet">` HTTP 拉取与 `fireScriptEvent` 属性回调补全
- **Fixed (`src/`)**:
  - `src/cdp/session_hooks.cp`: 新增 `handleInsertedLink()`（在 `appendChild`/`insertBefore` 挂载 `<link rel="stylesheet">` 时通过 `__capsurf_http_get` 异步拉取外部 CSS 并注入 `<style data-capsurf-link-mirror>` 镜像节点，同时支持 CSP `style-src 'none'` 拦截与 `load`/`error` 事件）；修复 `fireScriptEvent()` 跳过 `node.onload`/`node.onerror` 属性回调的问题。

## [2026-11-22] Phase 272: `DOM.describeNode` 补充 `frameId` 归属与 `DOM.setFileInputFiles` 实装
- **Added / Fixed (`src/`)**:
  - `src/cdp/dom.cp`: 为 `DOM.describeNode` 序列化结果注入当前 `session.frame_id`，解决 `ElementHandle.ownerFrame()` 返回 `null` 导致 `setInputFiles` 误报 detached element 的问题；新增 `DOM.setFileInputFiles` 处理器，通过 `MockFixtures.readFileContent` 读取本地文件并构造 `File` / `Uint8Array` 列表写入 `el.files` 与 `C:\fakepath\<basename>`，同步派发 `input` 与 `change` 事件。

## [2026-11-22] Phase 273: `Range.toString` / `Selection.toString` 跨节点与表单选区对齐
- **Fixed (`src/`)**:
  - `src/js/shim/range.js` (+ `src/js/runtime.js`, `src/js/runtime_source.cp`): 扩展 `Range.prototype.toString` 支持元素容器节点与跨节点文本遍历（不再仅限单文本节点）；扩展 `Selection.prototype.toString` 在无 DOM Range 时回退读取当前聚焦 `<input>`/`<textarea>` 的 `_selectionStart.._selectionEnd` 文本。
  - `src/js/shim/element/forms/selection.js`: `select()` 自动聚焦控件以确保 `document.activeElement` 与选区同步。

## [2026-11-22] Phase 274: `dblclick` 鼠标事件派发与 `MouseEvent.detail` 透传 `clickCount`
- **Fixed (`src/`)**:
  - `src/cdp/input.cp`: 为 `dispatchMouseDomEvent()` 增加 `detail` 参数（透传 `Input.dispatchMouseEvent` 的 `click_count`），并在 `MouseReleased` 且 `click_count == 2` 时在 `click` 之后派发 `dblclick` 事件（`detail: 2`）。

## [2026-11-22] Phase 275: `DOM.scrollIntoViewIfNeeded` 脱离文档与零尺寸/隐藏状态门控
- **Fixed (`src/`)**:
  - `src/cdp/dom.cp`: 在 `DOM.scrollIntoViewIfNeeded` 中校验目标元素 `isConnected`（脱离文档返回 `"Node is detached from document"`）及 `getBoxModel` 尺寸（`width <= 0 && height <= 0` 返回 `"Node does not have a layout object"`），使 Playwright `scrollIntoViewIfNeeded` 在 detached 时报错、在 `display:none` 时按预期等待可见或超时。

## [2026-11-22] Phase 276: `Runtime.exceptionThrown` 全局异常事件上报与闭合 `<details>` 无障碍树过滤
- **Fixed (`src/`)**:
  - `src/cdp/session.cp`, `src/js/dom_binding.cp`, `src/js/shim/window.js` (+ `src/js/runtime.js`, `src/js/runtime_source.cp`): 为 `executeClassicScript()` / `executeModuleScript()` 及定时器/消息队列捕获的未捕获异常增加 `pushExceptionEvent()` / `__capsurf_report_exception()` 通道，复用 `ConsoleRecord` 队列并在 `pollConsoleEvents()` 中派发 `Runtime.exceptionThrown` CDP 事件（支撑 `page.on('pageerror')`）。
  - `src/cdp/accessibility.cp`: 在 `buildNode()` 与 `collectSubtreeTextInternal()` 中对无 `open` 属性的闭合 `<details>` 仅保留首个 `<summary>` 子节点，屏蔽隐藏文本与子节点对无障碍名称及 AX 树的污染。

## [2026-11-22] Phase 277: `textContent` 设值触发布局重算、`innerText` 跳过 `[hidden]` 及 `<frame>` 归属查询
- **Fixed (`src/`)**:
  - `src/js/dom_binding.cp`: 在 `js_element_set_text_content()` 的文本节点与元素分支均设置 `el.dom_ctx.layout_dirty = true`，修复动态清空 `span.textContent = ""` 后 `getBoundingClientRect()` 仍返回旧尺寸的问题。
  - `src/js/shim/element/html-classes.js` (+ `src/js/runtime.js`, `src/js/runtime_source.cp`): 在 `collectRenderedText()` 中跳过 `hidden` 属性节点，修复 `innerText`（`useInnerText: true`）包含 `<span hidden>` 文本的问题。
  - `src/cdp/dom.cp`: 扩展 `collectDocIframeEntries()` 支持 `<frame>` 标签以完善 `DOM.getFrameOwner` 查询。

## [2026-11-22] Phase 278: `document.title` 孤立节点过滤与空白折叠、`Runtime.exceptionThrown` 队列扩容与换行清洗
- **Fixed (`src/`)**:
  - `src/js/dom_binding.cp`: `js_document_get_title()` 过滤 `node.parent == NULL_NODE` 的已脱离 `<title>` 节点，并按 HTML 规范折叠连续 ASCII 空白符与首尾修剪，修复 `document.open/write/close` 后仍返回 `"Blank Page"` 的问题；`MAX_CONSOLE_RECORDS` 扩容至 `128`，移除 `js_report_exception()` 尾部多余 `\n`。
  - `src/cdp/types.cp`, `src/cdp/runtime.cp`, `src/cdp/session.cp`: `MAX_CDP_EVENTS` 扩容至 `128`（同步更新 `runtime.cp` `polled_evts` 缓冲与 `session.cp` `console_event_storage` 至 `65536` 字节），并在 `pushExceptionEvent()` 中剥离末尾 `\r\n` 使 `pageerror` `error.message` 精确匹配。

## [2026-11-22] Phase 279: `//# sourceURL=` 脚本名透传与 `Runtime.exceptionThrown` 完整调用栈上报
- **Fixed (`src/`)**:
  - `src/js/qjs_engine.cp`: 新增 `JsContext.evalWithFilename(code, filename)` 支持为经典脚本传入真实源文件名。
  - `src/cdp/session.cp`: 新增 `extractSourceUrl()` 解析 `//# sourceURL=` / `//@ sourceURL=` 并传入 `executeClassicScript()`；扩展 `pushExceptionEvent()` 读取 `exc.stack`（`JS_GetPropertyStr`）拼接至 `exception.description`（去重首行避免重复错误头）。

## [2026-11-22] Phase 280: `--*` CSS 自定义属性跳过颜色序列化与计算样式颜色规范化
- **Fixed (`src/`)**:
  - `src/js/shim/cssom.js` (+ `src/js/runtime.js`, `src/js/runtime_source.cp`): 在 `CSSStyleDeclaration.prototype.getPropertyValue`、`getComputedStyle` Proxy `getPropertyValue` 及 `Proxy.get` 陷阱中对 `--*` 前缀自定义属性跳过 `serializeCssValue()` 与 `normalizeCssColor()`，确保 `--custom-color-property: #FF00FF` 原样保留而非被强转为 `rgb(255, 0, 255)`。

## [2026-11-22] Phase 281: 隔离世界（`executionContextId > 1`）临时恢复 `Node` 避免 `delete window.Node` 点击崩溃
- **Fixed (`src/`)**:
  - `src/js/shim/index.js` (+ `src/js/runtime.js`, `src/js/runtime_source.cp`): 在暴露 `global.Node` 时保存不可枚举备份 `__capsurf_orig_Node`。
  - `src/cdp/session.cp`, `src/cdp/runtime.cp`: 新增 `restoreIsolatedNodeIfNeeded()` / `clearIsolatedNode()`，在 `Runtime.callFunctionOn` 与 `Runtime.evaluate` 执行隔离世界（`contextId > 1`）期间临时恢复 `globalThis.Node` 并在返回前清理，保证主世界 `'Node' in window === false` 不变而 Playwright `InjectedScript` 点击定位不再报 `ReferenceError: Node is not defined`。

## [2026-11-22] Phase 282: 实装 `Input.dispatchDragEvent` 拖放事件与 `window.stop()`
- **Fixed (`src/`)**:
  - `src/cdp/input.cp`: 实装 `Input.dispatchDragEvent`，支持 `dragEnter`/`dragOver`/`drop`/`dragCancel` 映射为 `dragenter`/`dragover`/`drop`/`dragleave` DOM 事件，通过 `elementFromPoint(x, y)` 命中目标并挂载带 `items` 数据的 `DataTransfer` 对象。
  - `src/js/shim/window.js` (+ `src/js/runtime.js`, `src/js/runtime_source.cp`): 暴露 `window.stop()` / `globalThis.stop()` 方法，修复 `calling window.stop sync` `TypeError: not a function`。

## [2026-11-22] Phase 283: 追踪隔离世界 `objectId`（`__capsurf_isolated_oids`）自动恢复 `Node`
- **Fixed (`src/`)**:
  - `src/cdp/session.cp`, `src/cdp/runtime.cp`: `restoreIsolatedNodeIfNeeded()` / `clearIsolatedNode()` 维护 `__capsurf_in_isolated` 标记。
  - `src/js/dom_binding.cp`: 在 `__capsurf_format_remote_object` 中将隔离世界生成的 `objectId` 记入 `__capsurf_isolated_oids`，并在 `__capsurf_call_fn()` 检测到 `id` 或 `args` 含隔离对象时通过 `try/finally` 自动临时恢复并清理 `globalThis.Node`（无需调用方显式传入 `executionContextId`）。

## [2026-11-22] Phase 284: 按 Origin 持久化与恢复 `localStorage`（跨源导航隔离与回溯）
- **Fixed (`src/`)**:
  - `src/cdp/session.cp`: 新增 `OriginStorageEntry`、`extractUrlOrigin()`、`saveCurrentOriginStorage()` 与 `restoreOriginStorage()`，在 `CdpPageSession.navigate()` 销毁旧上下文前保存当前 origin 的 `localStorage` 键值并在新上下文初始化后按目标 origin 恢复，确保同源刷新保留且跨域跳转互不污染。

## [2026-11-22] Phase 285: CSS 选择器反斜杠转义解析与匹配（`q\:template`、`#a\:b`、`.c\:d`）
- **Fixed (`src/`)**:
  - `src/css/matcher.cp`: 新增零分配辅助函数 `scanCssIdent()` 与 `unescapeCssIdent()`，使 `SelectorParser` 与 `Matcher.matchesComponent()` 支持标签名、ID 与 Class 选择器中的 CSS 反斜杠转义字符（含哈希计算与大小写无关匹配）。

## [2026-11-22] Phase 286: `Network.setExtraHTTPHeaders` JSON 解析与 HTTP/1.1 请求头规范序列化
- **Fixed (`src/`)**:
  - `src/cdp/network.cp`: 在 `Network.setExtraHTTPHeaders` 中正确提取 `"headers"` JSON 对象 `{...}`（替代原先 `extractJsonString` 返回空导致整段 `params_raw` 透传的问题）。
  - `src/net/http_loader.cp`: 在 `HttpLoader.fetchDocument()` 中解析 `session.extra_headers` JSON 键值对并格式化为标准 `Key: Value\r\n` HTTP/1.1 请求头，彻底消除 `Parse Error: Invalid header token`。

## [2026-11-22] Phase 287: CSS 十六进制转义解码与无空格属性选择器大小写标识 `[attr*="..."i]`
- **Fixed (`src/`)**:
  - `src/css/matcher.cp`: 升级 `unescapeCssIdent()` 支持 1..6 位 CSS 十六进制转义序列（如 `#\31\32\33` -> `#123`）及 UTF-8 编码，跳过可选尾随空白；扩展 `SelectorParser.parseAttributeComponent()` 支持引号后紧邻的 `i`/`s` 大小写修饰符（如 `[attr*="llo WOR"i]`）。

## [2026-11-22] Phase 288: `applyInputText()` 支持 `contenteditable` 元素文本输入与退格删除
- **Fixed (`src/`)**:
  - `src/cdp/input.cp`: 在 `applyInputText()` 中为 `el.isContentEditable` 元素增加独立文本更新分支（结合 `window.getSelection()` 非折叠选区替换与 `Backspace` 删除），更新 `el.textContent` 并派发 `input` 事件，修复 `locator.fill()` 对 `contenteditable` 元素无效的问题。

## [2026-11-22] Phase 289: 规范 `HTMLLabelElement.control` 与 `Element.labels` 祖先关联逻辑
- **Fixed (`src/`)**:
  - `src/js/shim/element/forms/label.js` (+ `src/js/runtime.js`, `src/js/runtime_source.cp`): 修正 `getLabelsForControl()` 仅在祖先 `<label>` 无 `for` 属性且当前控件为其首个可标记后代（`p.control === control`）时纳入关联；同步在 `HTMLLabelElement.prototype.control` 中对空 `for=""` 直接返回 `null`。

## [2026-11-22] Phase 290: 修复闭合 `<details>` 折叠态后代元素与文本节点几何可见性
- **Fixed (`src/`)**:
  - `src/js/shim/element/element.js` (+ `src/js/runtime.js`, `src/js/runtime_source.cp`): 新增 `isHiddenInClosedDetails()` 并在 `isElementDisplayNone()` 与 `Element.prototype.getClientRects()` 中接入，确保无 `open` 属性的 `<details>` 下非首个 `<summary>` 后代返回空矩形与空 `DOMRectList`。
  - `src/js/shim/range.js`: 升级 `Range.prototype.getClientRects()` 遍历逻辑，跳过 `display: none` 子树且对闭合 `<details>` 仅遍历首个 `<summary>` 子元素，避免直接文本节点被判定为可见。

## [2026-11-22] Phase 291: 支持 `querySelector` / `querySelectorAll` 的 `:scope` 伪类与作用域组合子匹配
- **Fixed (`src/`)**:
  - `src/js/shim/selector.js` (+ `src/js/runtime.js`, `src/js/runtime_source.cp`): 在 `wrapQuerySelector()` / `wrapQuerySelectorAll()` 中拦截 `:scope` selectors 并传入 `scopeRoot`，在 `matchesCompoundSelector()` 中实现 `:scope`（支持 `_nid` 等价性），并在 `matchesComplexSelector()` 中允许 `>` 与后代组合子正确匹配 `scopeRoot` 本身（修复 `fieldset.querySelector(':scope > legend')` 返回 `null`）。

## [2026-11-22] Phase 292: 修复 `<select multiple>` 在置空 `selectedIndex` 后设置 `option.selected` 未重算的缺陷
- **Fixed (`src/`)**:
  - `src/js/shim/element/forms/select.js` (+ `src/js/runtime.js`, `src/js/runtime_source.cp`): 在 `HTMLOptionElement.prototype.selected` setter 中，当所属 `owner.multiple` 为 `true` 时显式将 `getSelectState(owner).selectedIndex` 置为 `undefined` 触发重算，修复先执行 `selectedIndex = -1` 再设置 `option.selected = true` 时 `select.value` 仍返回 `""` 的问题。

## [2026-11-22] Phase 293: 修复 `getComputedStyle` 对 CSS `content` 属性转义序列的解码与重序列化
- **Fixed (`src/`)**:
  - `src/js/shim/window.js` (+ `src/js/runtime.js`, `src/js/runtime_source.cp`): 在 `collectRawComputedMap()` 中对 `content` 属性的引号字符串值按 CSS 转义规则解码 1..6 位十六进制码点（含可选尾随空白）及字符转义（如 `"\eab2"` -> `""`、`'\0041\20 B'` -> `"A B"`）并统一按双引号字符串序列化，修复 `::before` 伪元素码点丢失的问题。

## [2026-11-22] Phase 294: 补齐 `[hidden]` 属性 UA 默认样式 `display: none` 计算规则
- **Fixed (`src/`)**:
  - `src/js/shim/window.js` (+ `src/js/runtime.js`, `src/js/runtime_source.cp`): 在 `getDefaultDisplayForElement()` 中新增 `[hidden]:not([hidden="until-found" i])` UA 默认规则返回 `"none"`（不覆盖作者显式 CSS `display` 声明），修复 `selectors-role` 等场景下 `<button hidden="">` 被 `getComputedStyle` 误报为 `"inline"` 可见的问题。

## [2026-11-22] Phase 295: 修复 Shadow DOM 未分配/隐藏 `<slot>` 裁剪及 ShadowRoot 固有几何尺寸回退
- **Fixed (`src/`)**:
  - `src/js/shim/element/element.js` (+ `src/js/runtime.js`, `src/js/runtime_source.cp`): 在 `getScrollParent()` 中对 ShadowHost 的轻 DOM 子节点按 `assignedSlot` 路由至对应 `<slot>`；在 `isElementDisplayNone()` 中对未分配 `<slot>`（`!cur.assignedSlot`）直接判为不可见；并在 `getBoundingClientRect()` 中为 ShadowRoot 内无显式宽高的文本/表单控件提供非零固有几何回退。

## [2026-11-22] Phase 296: 实现标准 `Document.prototype.createEvent(interface)` 接口分发与异常校验
- **Fixed (`src/`)**:
  - `src/js/shim/index.js` (+ `src/js/runtime.js`, `src/js/runtime_source.cp`): 实现 `Document.prototype.createEvent()`，按 WHATWG DOM 规范映射 `CustomEvent`、`Event`/`Events`/`HTMLEvents`/`SVGEvents`、`UIEvent`/`UIEvents`、`MouseEvent`/`MouseEvents`、`KeyboardEvent`/`KeyEvents`、`FocusEvent`、`CompositionEvent`、`DragEvent`、`MessageEvent`、`StorageEvent`、`HashChangeEvent`、`BeforeUnloadEvent` 等接口，并对未知接口抛出 `NotSupportedError` `DOMException`。

## [2026-11-22] Phase 297: 实现 `CharacterData` 基类、`Node.prototype.previousSibling`/`nextSibling` 与 `Text.prototype.wholeText`
- **Fixed (`src/`)**:
  - `src/js/shim/node-base.js` (+ `src/js/shim/index.js`, `src/js/runtime.js`, `src/js/runtime_source.cp`): 新增 `CharacterData` 基类并暴露全局 `global.CharacterData`，令 `Text` 与 `Comment` 继承 `CharacterData.prototype`；在 `Node.prototype` 补齐基于 `parentNode.childNodes` 与 `_nid` 等价匹配的 `previousSibling`/`nextSibling` getter；在 `Text.prototype` 实现 `wholeText` getter 拼接逻辑相邻文本节点内容。

## [2026-11-22] Phase 298: 统一相对 URL 点分路径规范化（`./`、`../`、`//`）修复跨目录脚本与子帧解析
- **Fixed (`src/`)**:
  - `src/net/http_loader.cp`: 升级 `HttpLoader.resolveRelativeUrl()` 支持协议相对 URL（`//`）、`base_url` 查询/锚点剥离以及逐段折叠 `./`、`../` 点分路径段。
  - `src/cdp/session.cp`: 将 `resolveSubresourceUrl()`（非 `data:`）统一委托至 `HttpLoader.resolveRelativeUrl()`，消除外部 `<script src="../...">` 与 `<iframe>` 相对路径残留 `/./` 或 `/../` 的问题。

## [2026-11-22] Phase 299: 修复 `scanCssIdent()` 吞并 CSS 十六进制转义后单空白符的词法规范
- **Fixed (`src/`)**:
  - `src/css/matcher.cp`: 升级 `scanCssIdent()` 在扫描 `\XXXX` 1..6 位十六进制转义后按 CSS Syntax 规范吞并紧邻的 1 个空白字符，避免 `#\31 \32 \33` 被提前截断为后代组合子。
