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


