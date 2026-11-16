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


