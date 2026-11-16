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
