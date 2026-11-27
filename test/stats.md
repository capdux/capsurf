# CapSurf 测试计划与测试结果统计总表 (`test/stats.md`)

> 更新时间：2026-11-26 14:01（基于 `bin_dev/main` 实测、Puppeteer 全量审计及 WPT `task-38759` 第 60/133 批实时日志）

---

## 一、全局测试资产与预期基线总览

| 测试类别 | 资产路径 | 用例 / 文件规模 | 基线预期分布 (`*-expectations.json`) / 实测进度 | 当前验证方式 |
| :--- | :--- | :--- | :--- | :--- |
| **Unit 白盒原生单测** | `test/unit/**/*.cp` | **138** 个 `.cp` 测试文件 | **全量实测完成**：**50 / 138 PASS**，**88 CompileFailed**（0 运行时失败；主要因旧单测/非模块化导入触发 `E0228`/`E0214`/`E0207`） | `/data/dev/cp/cap/bin/cap test test/unit/` |
| **E2E 黑盒端到端** | `test/e2e/*.e2e.test.ts` | **564** 套件（含 392 常规 + 172 demo） | **全量实测完成**：**474 / 536 PASS (88.4%)**，62 FAIL（49 `rc=1` + 13 `rc=124`），28 SKIP；单测试 1,096 pass / 66 fail | `CAPSURF_BIN=./bin_dev/main python3 scripts/e2e_suite.py` |
| **WPT Web 平台测试** | `test/wpt/` | **11,795** 用例（已登记 **9,725**） | 旧基线 `PASS: 7,224`；全量刷新 `task-38759` 已跑 **60/133 批 (4,789 条)**：`PASS: 2,708`, `FAIL: 1,388`, `TIMEOUT: 691`, `ERROR: 2` | `wpt-batched.mjs -j 8 --update-expectations` |
| **Playwright 兼容测试** | `test/pw/` | **123** 个 spec（**619** 条基线预期） | 分片实测中：`page-wait-for-selector` **47/47 (100%)**，`page-check` **16/16 (100%)**，`page-fill` **36/40 (90%)**，`page-keyboard` **37/44 (84.1%)**，`page-click` **77/102 (75.5%)**，`page-mouse` **11/15 (73.3%)**，`page-goto` **43/61 (70.5%)** | `pw-run.mjs --no-gate --no-retry` |
| **Puppeteer 兼容测试** | `test/pptr/` | **10** 个 spec（221 条实测 + `locator.test`） | **实测完成**：`PASS: 82`, `FAIL: 70`, `TIMEOUT: 58`, `SKIP: 11`（`locator.test` 600s SIGKILL） | `pptr-run.mjs --port 9780 --no-gate` |
| **Panda 黑盒测试集** | `test/panda/`（`.gitignore` 本地） | **517** 项有效页面自测（全量 671 项） | **Phase 970**：`PASS: 224`, `FAIL: 212`, `TIMEOUT: 81` (`tmp/panda_pass20.json`, 由 `204→212→224`, 净增 +12) | `scripts/panda_run.mjs -j 16` |

---

## 二、WPT CSS 各模块实测结果与待推进计划

### 2.1 已达成 100% 全绿的 WPT 子目录 / 专题

| 模块 / 目录 | 实测结果 | 关键修复里程碑 |
| :--- | :--- | :--- |
| `/css/css-text-decor/` | **41/41 PASS (100%)** | Phase 569 |
| `/css/css-text/parsing/` | **86/86 PASS (100%)** | Phase 587（收紧 21 项基线） |
| `/css/css-multicol/` | **32/32 PASS (100%)** | Phase 584 |
| `/css/css-flexbox/getcomputedstyle/` | **74/74 PASS (100%)** | Phase 580 |
| `/css/css-flexbox/parsing/` | **27/27 PASS (100%)** | Phase 580 |
| `/css/css-flexbox/inheritance.html` | **PASS (20/20 subtests)** | Phase 588 摸底确认 |
| `/css/css-sizing/parsing/` | **19/19 PASS (145/145 subtests, 100%)** | Phase 603 摸底确认 |
| `/css/css-tables/parsing/` | **15/15 PASS (37/37 subtests, 100%)** | Phase 595 摸底确认 |
| `/css/css-tables/inheritance.html` | **PASS (10/10 subtests)** | Phase 570 |
| `/css/css-align/parsing/` | **50/50 PASS (100%)** | 历史基线确认 |
| `/css/css-animations/parsing/` | **35/35 PASS (100%)** | Phase 576（含 `animation-computed` 15/15 Phase 610） |
| `/css/css-transitions/parsing/` | **14/14 PASS (100%)** | 历史基线 + Phase 612–613 |
| `/css/css-box/parsing/` | **29/29 PASS (100%)** | 历史基线确认 |
| `/css/css-logical/parsing/` | **54/54 PASS (100%)** | 历史基线确认 |
| `/css/css-position/parsing/` | **22/22 PASS (100%)** | 历史基线确认 |
| `/css/css-position/inheritance.html` | **PASS (12/12 subtests)** | Phase 590 摸底确认 |
| `/css/css-break/parsing/` | **18/18 PASS (100%)** | Phase 572 |
| `/css/selectors/parsing/` | **16/16 PASS (100%)** | 历史基线确认 |
| `/css/css-ruby/parsing/` & `inheritance` | **8/8 PASS + inheritance PASS** | Phase 586 |
| `/css/css-scroll-snap/` | **8/8 PASS (100%)** | 历史基线确认 |
| `/css/css-rhythm/parsing/` | **5/5 PASS (100%)** | 历史基线确认 |
| `/css/css-contain/parsing/` & `inheritance` | **4/4 PASS + inheritance PASS** | Phase 590 确认 |
| `/css/css-content/parsing/` | **4/4 PASS (100%)** | 历史基线确认 |
| `/css/css-will-change/` | **3/3 PASS (100%)** | 历史基线确认 |
| `/css/css-overscroll-behavior/` | **2/2 PASS (100%)** | Phase 575 |
| `/css/css-grid/inheritance.html` | **PASS (20/20 subtests)** | Phase 588 |
| `/css/css-animations/inheritance.html` | **PASS (16/16 subtests)** | Phase 589 |
| `/css/css-transitions/inheritance.html` | **PASS (8/8 subtests)** | Phase 589 |
| `/css/css-lists/inheritance.html` | **PASS (10/10 subtests)** | Phase 592 |
| `/css/css-writing-modes/inheritance.html` | **PASS (10/10 subtests)** | Phase 594（收紧基线） |
| `/css/css-color/inheritance.html` | **PASS (4/4 subtests)** | Phase 589 摸底确认 |

---

### 2.2 全量摸底模块结果与后续修复计划

| WPT 模块 / 过滤前缀 | 最近摸底统计 | 本轮新增已修复 (Phase 588–615) | 后续优先修复计划 |
| :--- | :--- | :--- | :--- |
| `/css/css-transitions/` | **15/20 PASS** -> **18/20 PASS** | `transition-delay-001` (21/21), `transition-duration-001` (21/21), `transition-001` (13/13) | `transition-property-002`（优化超时）、`all-interpolates-same-as-explicit-property`（50% 插值） |
| `/css/css-animations/` | **49/58 PASS** -> **50/58 PASS** | `animation-computed` (15/15) | `style-animation-parsing` / `computed-style-animation-parsing`（`animation-name` 与关键字同名优先级） |
| `/css/css-sizing/` | **29/32 PASS**, 3 FAIL, 0 TIMEOUT | `inheritance-001` PASS | `inheritance-002`（abspos `auto` 宽高 `0px`）、`intrinsic-percent-replaced-{028,033}` |
| `/css/css-tables/` | **16/18 PASS**, 1 FAIL, 1 TIMEOUT | `inheritance` & `parsing/` 16/16 全绿 | `table-row-group-color-inheritance-001`（reftest）、`absolute-tables-001`（防卡死） |
| `/css/css-box/` | **31/33 PASS**, 0 FAIL, 2 TIMEOUT | `inheritance.html` PASS | 排查 2 个 `margin-trim/computed-margin-values/*` 超时 |
| `/css/css-ui/parsing/` | **35/39 PASS**, 3 FAIL, 1 TIMEOUT | `outline-color` 逻辑微调 | `cursor-computed`（gradient）、`canonical-order-outline-sub-properties-001` |
| `/css/css-images/parsing/` | **21/24 PASS** (197/197 subtests) | — | `gradient-interpolation-method-{valid,invalid,computed}` |
| `/css/css-transforms/parsing/` | **21/28 PASS** -> **25/28 PASS** | `transform-computed` (9/9), `webkit-perspective-{valid,invalid}.tentative`, `webkit-transform-valid.tentative` | `transform-valid` (`scale(%)` 转小数)、`scale-parsing-*` (`sign()`) |
| `/css/css-break/` | **163/209 PASS**, 44 FAIL, 2 TIMEOUT | `parsing/` 18/18 全绿 | `/css/css-break/inheritance.html`（补齐 `box-decoration-break`/`break-*`/`orphans`/`widows` 默认值） |
| `/css/css-contain/` | **49/55 recorded PASS**, 1 FAIL, 5 TIMEOUT | `inheritance.html` PASS (2/2) | `content-visibility-auto-first-observation-immediate`、`content-visibility` crash 修复 |
| `/css/css-overflow/parsing/` | **22/32 PASS** -> **25/32 PASS** | `scroll-axis-lock-invalid` (7/7), `scroll-target-group-{invalid,computed}` | `overflow-clip-margin-computed`、`scroll-markers-{invalid,computed}`、`::scroll-button()` |
| `/css/css-backgrounds/parsing/` | **51/74 PASS** -> **52/74 PASS** | `background-size-valid` PASS (`computed` 14/16) | `border-shorthand`、`border-image-shorthand`、`background-invalid`、`webkit-border-radius-valid` |
| `/css/css-shapes/parsing/` | **7/13 PASS** -> **9/13 PASS** | `shape-outside-valid-position` (20/20), `shape-outside-shape-valid` (12/12) | `shape-outside-path-{valid,invalid}`、`shape-outside-shape-invalid` |
| `/css/css-masking/parsing/` | **4/28 PASS** -> **15/28 PASS** | `mask-size-valid`, `mask-repeat-valid`, `mask-type-{computed,invalid}`, `mask-composite-{computed,invalid}`, `clip-rule-{computed,invalid}`, `mask-size-invalid`, `clip-invalid`, `mask-position-{valid,invalid}` | `clip-computed` (`calc`), `mask-image-computed`, `clip-path-shape-parsing` |
| `/css/selectors/` | **41/67 PASS** -> **45/67 PASS** (+4 dir PASS) | `has-invalidation-first-in-sibling-chain`, `has-with-not`, `has-with-is-child-combinator`, `has-pseudoclass-only` | 收紧 4 项 `dir-*` unexpected PASS；`dir-selector-querySelector` (`:dir(ltr, rtl)`)；`not-complex` |
| `/css/css-cascade/` | **31/54 recorded PASS**, 18 FAIL, 4 TIMEOUT, 1 ERROR | `supports-import-parsing` PASS | 收紧 `layer-counter-style-override` PASS；`unset-value-storage`；`important-vs-inline-002` |
| `/css/css-flexbox/` | **95/120 recorded PASS**, 15 FAIL, 10 TIMEOUT | `inheritance.html` PASS (20/20) | `flexbox_computedstyle_flex-shorthand-invalid`、`hittest-*`、`col-wrap-*` |
| `/css/css-position/` | **50/117 recorded PASS**, 55 FAIL, 11 TIMEOUT, 1 ERROR | `parsing/` 22/22, `inheritance` 12/12 | `overlay-button-appearance`、`multicol/static-position`、`position-absolute-center-*` |
| `/css/css-fonts/parsing/` | **41/82 PASS** -> **43/82 PASS** | `font-face-size-adjust` (6/6), `font-stretch-alias` (1/1) | `font-language-override-{valid,computed}`、`font-variation-settings-valid`、`font-size-adjust-valid` |
| `/css/css-syntax/` | **7/24 PASS** -> **9/24 PASS** | `charset-is-not-a-rule`, `trailing-braces` | `whitespace.html` (`\f`), `decimal-points-in-numbers`, `input-preprocessing`, `anb-*` |
| `/css/css-variables/` | **60/194 PASS** -> **63/194 PASS** | `variable-empty-name-reserved`, `variable-exponential-blowup` (OOM->PASS), `missing-closing-nested-fallback` | `css-variable-change-style-{001,002}`、`variable-substitution-basic`、95 项 1008px reftest 同源偏移 |
| `/css/css-conditional/` | **38/135 PASS** -> **39/135 PASS** | `container-type-computed` (5/5), `container-computed` (14/14) | `container-name-computed`, `container-inheritance`, `container-{parsing,name-parsing,type-parsing}`, `CSS-supports-CSSStyleDeclaration` |
| `/css/css-color/` | **14/56 PASS** -> **16/56 PASS** | `color-invalid-hwb` (6/6), `color-invalid-named-color` (184/184, `--timeout 15000`) | `color-computed` (`currentcolor`), `light-dark-currentcolor-in-color`, `opacity-valid` (`%`->number), `color-valid` (`#hex`->`rgb`) |
| `/css/css-display/` | **16/52 PASS**, 32 FAIL, 4 TIMEOUT | — | 收紧 `reading-{flow,order}-valid` unexpected PASS；`textarea-display`、`display-contents-computed-style` |
| `/css/css-lists/` | **46/182 PASS** (testharness **24/24**) | `inheritance` 10/10 | 收紧 2 项 reftest unexpected PASS；`reversed` 计数器与 `display:none/contents` 计数更新 |
| `/css/css-grid/parsing/` | **6/61 PASS** -> **15/61 PASS** | `grid-auto-flow` (3/3), `grid-template-areas` (4/4), `grid-template-{columns,rows}-valid` (2/2) | 收紧 2 项 `repeat-intrinsic-valid.tentative`；`grid-auto-{columns,rows}-valid`（去注释）、`grid-template-node-not-connected` |
| `/css/css-values/minmax-` | **2/19 PASS**, 15 FAIL, 2 TIMEOUT | — | `minmax-number-computed`、`min()`/`max()` 空参数拦截、`calc()` 化简序列化 |

---

## 三、E2E / Playwright / Puppeteer / Unit 测试计划

1. **E2E 冒烟与全量 (`test/e2e/`)**
   - **Smoke 门禁（每阶段必跑）**：`CAPSURF_BIN=./bin_dev/main python3 scripts/e2e_suite.py --smoke` — **3/3 suites, 6/6 tests PASS**（`alert-non-throwing`, `blank-page-document`, `smoke`）。
   - **全量计划**：392 个 `.e2e.test.ts` + `test/e2e/demo/`，在 WPT CSSOM/Selector/Layout 核心收敛后批量回归验收。

2. **Playwright / Puppeteer 兼容套件 (`test/pw/`, `test/pptr/`)**
   - **Playwright**：619 条记录（`358 FAIL`, `261 SKIP`），待 CDP Page/Frame/Input/Network/Evaluate 协议层专项攻坚。
   - **Puppeteer**：79 条记录（`53 FAIL`, `26 SKIP`），随 CDP 兼容层同步推进。

3. **CP 原生白盒单测 (`test/unit/`)**
   - **138** 个 `.cp` 白盒测试文件，保障 DOM Arena、CSS Parser/Cascade/Layout、HTML5 解析与 QuickJS 绑定核心无回归。

---

## 四、2026-11-26 实时审计与基线刷新进度快照

### 4.1 Puppeteer (`test/pptr/`) 分片实测台账（产物统一存于 [tmp/](file:///data/dev/capsurf/tmp)）

- **全量基线参考（10 个 spec，首轮 600s 上限）**：9 个落盘 spec 共 **221** 条：**PASS 82 / FAIL 70 / TIMEOUT 58 / SKIP 11**（`locator.test` 600s SIGKILL）。
- **Phase 937–939 修复后最新实测（`--no-retry`, `CAPSURF_BIN=./bin_dev/main`, 产物 `tmp/pptr*_*.json`）**：
  - `evaluation.test` (**Phase 966**): **42 PASS** / 5 FAIL / 0 TIMEOUT / 5 SKIP（`tmp/pptr966_evaluation.json`, 14 项 `FAIL→PASS`, 余 `RegEx` x2 / `symbols` / `circular` / `createImageBitmap`）
  - `jshandle.test` (**Phase 966**): **24 PASS** / 1 FAIL / 0 TIMEOUT / 0 SKIP（`tmp/pptr966_jshandle2.json`, 5 项 `FAIL→PASS`, `circular objects` 已过, 仅余 `dates`）
  - `click.test` (**Phase 966 已全过不再重测**): **26 PASS** / 0 FAIL / 0 TIMEOUT / 4 SKIP (`tmp/pptr966_click2.json`, `deviceScaleFactor set` 已过)
  - `navigation.test` (**Phase 964**, 0 TIMEOUT): **48 PASS** / 5 FAIL / 0 TIMEOUT / 8 SKIP (`tmp/pptr964_navigation.test.json`, `disposeBrowserContext` 超时已清零)
  - `waittask.test` (**Phase 948**, 0 TIMEOUT): **57 PASS** / 1 FAIL / 0 TIMEOUT / 4 SKIP（由 `15→30→52→55→56→57`, `adoptedStyleSheets` 全过, `tmp/pptr948_wt.json`）
  - `injected.test` (**已全过不再重测**): **8 PASS** / 0 FAIL (`tmp/pptr_injected.json`)
  - `queryselector.test` (**Phase 963 已全过不再重测**): **35 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pptr963_queryselector.test.json`, 由 `1→32→34→35`)
  - `elementhandle.test` (**Phase 966 已全过不再重测**): **48 PASS** / 0 FAIL / 0 TIMEOUT / 2 SKIP (`tmp/pptr966_eh2.json`, 5 项 `touch*` + 2 项 iframe `clickablePoint` 全过)
  - `elementhandle.test` (**Phase 944**): **41 PASS** / 0 FAIL / 7 TIMEOUT / 2 SKIP（由 `13→29→38→41`, `grid.html` 与 `clickablePoint` 通过, `tmp/pptr944_eh.json`）
  - `locator.test` (**Phase 963 已全过不再重测**): **60 PASS** / 0 FAIL / 0 TIMEOUT / 1 SKIP (`tmp/pptr963_locator.test.json`, 由 `0/SIGKILL→57→58→60/61`, `readonly color/range` 560ms 全过)
  - `queryhandler.test` (**已全过不再重测**, Phase 947): **36 PASS** / 0 FAIL / 0 TIMEOUT / 2 SKIP（由 `2→23→33→34→36`，两个 ARIA 超时全过，`tmp/pptr947_qh.json`）
  - `queryhandler.test`: `0→2 PASS` / 11 FAIL（待复测 `Phase 939`）
  - `locator.test`: 120s SIGKILL 未落盘（0 PASS / 7 failing）
- **批量待修根因（6 类，已由 `3cff8856` 定位源码行号，待本轮汇总后统一批量修复）**：
  1. `DOM.resolveNode: No node with given id found`（**54 处 FAIL**）：`document._nid = 0` 未映射非零 `backendNodeId` 哨兵（`src/cdp/dom.cp`, `src/js/dom_binding.cp`）。
  2. `Runtime.callFunctionOn` / `Target.disposeBrowserContext` 阻塞超时（**6 处** + 大量 15s Mocha 超时）：单线程同步等待 `promiseState == 0` 及 `appendChild` 触发布局失效。
  3. `RemoteObject` 序列化与 `subtype` 缺失（**6 处**）：`WeakMap`/`Window`/`Date`/`RegExp`/`Symbol` 及 `navigator.userAgent`。
  4. Bad SSL 报错文案（**2 处**）：`net::ERR_EMPTY_RESPONSE` 需映射为 `net::ERR_CERT_AUTHORITY_INVALID`。
  5. `deviceScaleFactor`（**1 处**）：`src/cdp/emulation.cp` 未同步更新 `window.devicePixelRatio`。

### 4.2 WPT 全量基线刷新 (`task-38759`) 中间统计

- **进度**：**60 / 133 批**（共 11,795 用例，已跑 **4,789** 条）
- **已跑条目实时统计**：**PASS 2,708 (56.5%) / FAIL 1,388 / TIMEOUT 691 / ERROR 2**
- **最近全量 CSS 目录实测 PASS**：
  - `/css/css-ui/`: **828 / 1,077** (`76.9%`)
  - `/css/css-backgrounds/`: **463 / 759**
  - `/css/css-text/`: **368 / 1,813**
  - `/css/css-transforms/`: **273 / 849** (curated 31/31)
  - `/css/css-contain/`: **270 / 569**
  - `/css/css-break/`: **252 / 1,198**
  - `/css/css-sizing/`: **209 / 695**
  - `/css/css-color/`: **207 / 320**（+ Phase 936 4/4 PASS）
  - `/css/css-writing-modes/`: **173 / 470**
  - `/css/filter-effects/`: **142 / 164** (`86.6%`)
  - `/css/css-pseudo/`: **122 / 337**
  - `/css/css-tables/`: **121 / 306**
  - `/css/css-masking/`: **60 / 179** (`33.5%`)

### 4.3 E2E 全量黑盒套件审计汇总（已完成，`adfb8b06`，耗时 914.78s）

- **总体结果**：**474 / 536 套件通过 (88.4%)**，**62 套件失败**（49 项 `rc=1` + 13 项 `rc=124` 120s 超时），28 项跳过（`boa_v8_bridge=14`, `workers_wasm_external=12`, `quickjs_rc_vs_v8_gc=2`）；单测试 **1,096 pass / 66 fail**。
- **62 个失败套件根因归类**：
  - A. **超时 / 卡死（14 项）**：13 项 `120s` 超时 + 1 项 `Page.navigate` `60s` 超时
  - B. **CDP 连接关闭 / Pending 请求丢失（4 项）**：`cors-stylesheets`, `fetch-slot-order`, `link-onload-media`, `resource-type-not-page-settable`
  - C. **布局、定位与几何（15 项）**：`DOM.getBoxModel` / `offsetTop` 差 8px、`float-containment`、`shadow-dom`、`table-caption-sizing` 等
  - D. **CSSOM / 计算样式 / Web Animations（11 项）**：`computed-style-reads`（51 个 camelCase）、`cssom-page-rules`、`web-animations` (6/8) 等
  - E. **DOM 绑定 / WebIDL / 编辑事件（10 项）**：`attributes-live-namednodemap`、`collection-named-shadowing`、`contenteditable-typing` 等
  - F. **表单控件（2 项）**：`forms-form-demo`, `forms-range-demo`
  - G. **脚本加载 / CSP / 网络会话（6 项）**：重复 `load`/`error` 事件、`script-gate`、`redirect-per-hop-gating`、`session-deadlines`

### 4.4 Unit 白盒原生单元测试审计汇总（**138 / 138 100% PASS**）

- **总体结果**：**138 / 138 PASS**（10 大子目录 `css/` 18/18、`cdp/` 28/28、`dom/` 24/24、`eval/` 18/18、`net/` 18/18、`render/` 10/10、`html/` 9/9、`layout/` 5/5、`js/` 4/4、`tty/` 4/4 全部通过 `cap test --modular` 门禁）。

### 4.5 Playwright (`test/pw/`) 分片实测台账（产物统一存于 [tmp/](file:///data/dev/capsurf/tmp)）

- **已完成审计（123 / 123 个 allow-list spec 全量覆盖，0 个未测，`--no-gate --no-retry`, `CAPSURF_BIN=./bin_dev/main`）**：
  - `page-wait-for-selector` (1 & 2, **已全过不再重测**): **47 PASS** / 0 FAIL (`tmp/pw_wfs.json`)
  - `queryselector.spec.ts` (**已全过不再重测**): **17 PASS** / 0 FAIL (`tmp/pw_qs.json`)
  - `page-check` (**已全过不再重测**): **16 PASS** / 0 FAIL (`tmp/pw_check.json`)
  - `page-listeners` (**已全过不再重测**): **3 PASS** / 0 FAIL (`tmp/pw_listeners.json`)
  - `page-add-script-tag` (**已全过不再重测**): **9 PASS** / 0 FAIL (`tmp/pw_add.json`)
  - `page-add-style-tag` (**已全过不再重测**): **4 PASS** / 0 FAIL (`tmp/pw_add.json`)
  - `page-localstorage` (**已全过不再重测**): **9 PASS** / 0 FAIL (`tmp/pw_localstorage.json`)
  - `jshandle-*` (5 specs: `as-element`/`evaluate`/`json-value`/`properties`/`to-string`, **已全过不再重测**): **22 PASS** / 0 FAIL (`tmp/pw943_jshandle.json`)
  - `eval-on-selector*` (3 specs: `eval-on-selector`/`eval-on-selector-all`/`elementhandle-eval-on-selector`, **已全过不再重测**): **41 PASS** / 0 FAIL (`tmp/pw943_evalsel.json`)
  - `locator-*` (8 项**已全过不再重测**: `convenience` 21/21, `element-handle` 5/5, `evaluate` 5/5, `is-visible` 8/8, `list` 1/1, `query` 21/21, `wait-for-function` 10/10, `locator-click` **5/5** (`tmp/pw959_locator-click.json`, 由 `4→5`); 另 `locator-misc-1`: **7 PASS** / 1 FAIL / 5 TIMEOUT; `locator-misc-2`: **6 PASS** / 8 TIMEOUT): 合计 **89 PASS** / 1 FAIL / 13 TIMEOUT (`tmp/pw943_loc*.json`, `tmp/pw959_locator-click.json`)
  - `selectors-text` (**已全过不再重测**): **21 PASS** / 0 FAIL (`tmp/pw943_seltxt.json`)
  - `selectors-misc` (**已全过不再重测**): **19 PASS** / 0 FAIL (`tmp/pw943_selmisc.json`)
  - `selectors-get-by` (**已全过不再重测**): **25 PASS** / 0 FAIL (`tmp/pw943_selgetby.json`)
  - `selectors-register` (**已全过不再重测**): **6 PASS** / 0 FAIL (`tmp/pw943_selreg.json`)
  - `selectors-css` (**已全过不再重测**): **25 PASS** / 0 FAIL / 1 SKIP (`tmp/pw944_selcss.json`, 由 `24→25`)
  - `retarget` (**已全过不再重测**): **15 PASS** / 0 FAIL (`tmp/pw945_retarget.json`, 由 `14→15`)
  - `page-strict` (**已全过不再重测**): **10 PASS** / 0 FAIL (`tmp/pw944_strict.json`, 由 `8→10`)
  - `elementhandle-*` (10 specs, **10 项全量已全过不再重测**: `convenience` 14/14, `click` 8/8, `bounding-box` 7/7, `query-selector` 6/6, `press` 5/5, `select-text` 5/5, `type` 5/5, `scroll-into-view` **8/8+1 SKIP**, `wait-for-element-state` **12/12**, `misc` **10/10** (`tmp/pw970_eh_misc.json`)): 合计 **80 PASS** / 0 FAIL / 0 TIMEOUT / 1 SKIP (`tmp/pw943_eh*.json`, `tmp/pw944_ehbbox.json`, `tmp/pw945_ehclick.json`, `tmp/pw959_elementhandle-*.json`, `tmp/pw966_eh_wait.json`, `tmp/pw970_eh_misc.json`)
  - `expect-to-have-text` (**已全过不再重测**): **28 PASS** / 0 FAIL (`tmp/pw944_exptxt.json`)
  - `expect-to-have-value` (**已全过不再重测**): **11 PASS** / 0 FAIL (`tmp/pw944_expval.json`)
  - `expect-boolean`: **83 PASS** / 1 FAIL (`tmp/pw966_expect_bool.json`, `with frameLocator 2` 已过, 仅余既有 `toBeFocused with shadow elements`)
  - `expect-builtins` (**已全过不再重测**): **278 PASS** / 0 FAIL (`tmp/pw944_expbi.json`)
  - `expect-matcher-result` (**已全过不再重测**): **4 PASS** / 0 FAIL (`tmp/pw944_expmr.json`)
  - `expect-timeout` (**已全过不再重测**): **15 PASS** / 0 FAIL (`tmp/pw944_expto.json`)
  - `expect-misc` (**已全过不再重测**): **59 PASS** / 0 FAIL (`tmp/pw944_expmisc.json`)
  - `page-dispatchevent` (**已全过不再重测**): **16 PASS** / 0 FAIL (`tmp/pw960_page-dispatchevent.json`, 由 `15→16`, `should be atomic` 已过)
  - `page-autowaiting-basic` (**已全过不再重测**): **12 PASS** / 0 FAIL (`tmp/pw948_page-autowaiting-basic.json`, 由 `9→12`)
  - `page-autowaiting-no-hang` (**已全过不再重测**): **9 PASS** / 0 FAIL (`tmp/pw948_page-autowaiting-no-hang.json`, 由 `7→9`)
  - `page-event-console` (**已全过不再重测**): **19 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw950_page-event-console.json`, 由 `16→18→19`)
  - `page-event-pageerror` (**已全过不再重测**): **7 PASS** / 0 FAIL (`tmp/pw966_pageerror.json`, 由 `6→7`, `should fire` 已过)
  - `page-expose-function` (**已全过不再重测**): **18 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw952_page-expose-function.json`, 由 `9→16→17→18`)
  - `page-add-init-script-callback` (**已全过不再重测**): **11 PASS** / 0 FAIL (`tmp/pw948_page-add-init-script-callback.json`, 由 `7→9→11`)
  - `expect-to-have-accessible` (**已全过不再重测**): **14 PASS** / 0 FAIL (`tmp/pw945_expacc.json`)
  - `frame-frame-element` (**已全过不再重测**): **6 PASS** / 0 FAIL (`tmp/pw945_ffel.json`)
  - `frame-evaluate` (**已全过不再重测**): **8 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw970_frame_eval.json`, 由 `3→5→6→8`)
  - `frame-hierarchy` (**已全过不再重测**): **12 PASS** / 0 FAIL (`tmp/pw966_frame_hier.json`, 由 `8→9→11→12`)
  - `locator-frame` (**已全过不再重测**): **9 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw965_locator-frame.json`, 由 `6→8→9`)
  - `selectors-frame` (**已全过不再重测**): **15 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw966_sel_frame.json`, 由 `5→9→13→15`)
  - `page-navigation` (**已全过不再重测**): **3 PASS** / 0 FAIL (`tmp/pw945_pnav.json`)
  - `page-wait-for-load-state` (**已全过不再重测**): **10 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw968_wfload.json`, 由 `6→7→10`)
  - `page-wait-for-navigation` (**已全过不再重测**): **12 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw968_wfnav.json`, 由 `7→10→11→12`)
  - `page-wait-for-request` (**已全过不再重测**): **8 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw950_page-wait-for-request.json`, 由 `7→8`)
  - `page-wait-for-response` (**已全过不再重测**): **8 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw950_page-wait-for-response.json`, 由 `7→8`)
  - `matchers.misc` (**已全过不再重测**): **2 PASS** / 0 FAIL (`tmp/pw948_matchers.misc.json`)
  - `page-wait-for-url` (**已全过不再重测**): **9 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw968_wfurl.json`, 由 `7→9`)
  - `wheel.spec` (**已全过不再重测**): **6 PASS** / 0 FAIL (`tmp/pw945_wheel.json`)
  - `selectors-role` (**已全过不再重测**): **15 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw950_selectors-role.json`, 由 `13→15`)
  - `page-add-init-script` (**已全过不再重测**): **11 PASS** / 0 FAIL (`tmp/pw959_page-add-init-script.json`, 由 `10→11`)
  - `page-add-locator-handler` (**已全过不再重测**): **13 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw966_locator_handler.json`, 由 `1→12→13`)
  - `page-dialog` (**已全过不再重测**): **9 PASS** / 0 FAIL (`tmp/pw950_page-dialog.json`, 由 `7→9`)
  - `page-history` (**已全过不再重测**): **13 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw955_page-history.json`, 由 `10→13`)
  - `page-basic`: **12 PASS** / 4 FAIL (`tmp/pw_basic.json`, `frame({name,url})`/`Chrome/`/`webdriver`)
  - `page-focus` (**已全过不再重测**): **7 PASS** / 0 FAIL (`tmp/pw954_page-focus.json`, 由 `6→7`)
  - `page-emulate-media` (**已全过不再重测**): **12 PASS** / 0 FAIL (`tmp/pw968_media.json`, 由 `9→11→12`)
  - `page-set-content` (**已全过不再重测**): **16 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw970_set_content.json`, 由 `14→15→16`)
  - `page-set-extra-http-headers`: **4 PASS** / 1 FAIL (`tmp/pw_set.json`, `should not duplicate referer header`)
  - `page-set-input-files`: **17 PASS** / 2 FAIL (`tmp/pw960_page-set-input-files.json`, 由 `13→16→17`, 0 TIMEOUT, `should not trim big uploaded files` 已过)
  - `page-wait-for-function` (**已全过不再重测**): **26 PASS** / 0 FAIL / 0 TIMEOUT (`25→26`)
  - `page-fill`: **40 PASS** / 0 FAIL / 0 TIMEOUT (**已全过不再重测**, `tmp/pw943_fill.json`)
  - `page-basic`: **16 PASS** / 0 FAIL (**已全过不再重测**, `tmp/pw943_basic.json`)
  - `page-emulate-media`: **12 PASS** / 0 FAIL (**已全过不再重测**, `tmp/pw943_media.json`)
  - `page-keyboard` (**已全过不再重测**): **44 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw944_kb.json`, 由 `41→43→44`)
  - `page-network-sizes` (**已全过不再重测**): **13 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw952_page-network-sizes.json`, 由 `5→11→12→13`)
  - `page-click` (8 specs): **90 PASS** / 5 FAIL / 5 TIMEOUT / 2 SKIP (`tmp/pw966_page_click.json`, 由 `77→84→85→90`)
  - `page-evaluate` (4 specs): **120 PASS** / 8 FAIL / 3 TIMEOUT (`tmp/pw943_eval.json`, 其中 `page-evaluate.spec.ts` **95/95 全过**, `page-evaluate-handle.spec.ts` **11/11 全过**)
  - `page-screenshot`: **36 PASS** / 7 FAIL / 1 TIMEOUT (`tmp/pw967_page_ss2.json`, 由 `17→19→24→26→28→36`, 所有 `mask` & WebP/JPEG 已过)
  - `page-goto`: **59 PASS** / 2 FAIL / 0 TIMEOUT (`tmp/pw966_page_goto2.json`, 由 `43→46→57→59`, `referrer-policy` & `window.stop()` 已过；另 `tmp/pw970_page_goto.json` 2 项并发导航 `--grep` 复测 PASS)
  - `page-mouse` (**已全过不再重测**): **15 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw959_page-mouse.json`, 由 `11→14→15`)
  - `page-goto`: **43 PASS** / 8 FAIL / 10 TIMEOUT (`tmp/pw_goto.json`, `referer`/COOP/`window.stop()`/异步导航)
  - `page-screenshot`: **17 PASS** / 20 FAIL / 7 TIMEOUT (`tmp/pw_screenshot.json`, `mask [uninitialized]`/WebP&JPEG/animations/`fullPage`)
  - `page-evaluate` (4 specs): **82 PASS** / 11 FAIL / 38 TIMEOUT (`tmp/pw_eval.json`, `callBinding` undefined/过深引用链/导航后报错超时级联)
  - `page-event-load` (**已全过不再重测**): **1 PASS** / 0 FAIL (`tmp/pw_event.json`)
  - `page-event-network` (**已全过不再重测**): **6 PASS** / 0 FAIL (`tmp/pw966_event_network.json`, 由 `5→6`, `RequestFailed @smoke` 已过)
  - `page-event-request`: **9 PASS** / 4 FAIL / 0 TIMEOUT (`tmp/pw970_event_req.json`, 由 `5→6→9`, 仅余 4 项 ServiceWorker)
  - `page-network-idle` (**已全过不再重测**): **10 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw968_net_idle2.json`, 由 `2→3→9→10`)
  - `page-network-request` (**可执行项 26/26 全过不再重测**): **26 PASS** / 0 FAIL / 0 TIMEOUT / 1 SKIP (`tmp/pw968_net_req2.json`, 由 `7→20→25→26`)
  - `page-network-response` (**已全过不再重测**): **21 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw967_net_resp.json`, 由 `17→21`)
  - `page-request-continue` (**已全过不再重测**): **36 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw966_req_continue2.json`, 由 `0→11→18→28→31→32→36`)
  - `page-request-fallback` (**已全过不再重测**): **14 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw965_page-request-fallback.json`, 由 `1→13→14`)
  - `page-request-fulfill` (**已全过不再重测**): **20 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw963_page-request-fulfill.json`, 由 `0→11→18→20`)
  - `page-request-intercept` (**可执行项 13/13 全过不再重测**): **13 PASS** / 0 FAIL / 0 TIMEOUT / 1 SKIP (`tmp/pw967_req_int.json`, 由 `0→11→12→13`)
  - `page-drop` (**已全过不再重测**): **7 PASS** / 0 FAIL (`tmp/pw951_page-drop.json`)
  - `to-match-aria-snapshot` (**已全过不再重测**): **36 PASS** / 0 FAIL (`tmp/pw951_to-match-aria-snapshot.json`)
  - `expect-with-snapshot` (**已全过不再重测**): **14 PASS** / 0 FAIL (`tmp/pw951_expect-with-snapshot.json`)
  - `page-drag` (**预期全跳过不再重测**): 0 PASS / **15 SKIP** (`tmp/pw951_page-drag.json`)
  - `page-select-option` (**已全过不再重测**): **33 PASS** / 0 FAIL (`tmp/pw953_page-select-option.json`, 由 `32→33`)
  - `page-aria-snapshot` (**已全过不再重测**): **38 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw959_page-aria-snapshot.json`, 由 `37→38`)
  - `page-aria-snapshot-ai`: **30 PASS** / 5 FAIL / 1 TIMEOUT (`tmp/pw966_aria_ai.json`, 由 `27→28→29→30`, `<frameset>` `[active]` 已过)
  - `elementhandle-screenshot` (**已全过不再重测**): **11 PASS** / 0 FAIL (`tmp/pw967_eh_ss2.json`, 由 `9→10→11`)
  - `page-filechooser` (**已全过不再重测**): **6 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw966_filechooser.json`, 由 `4→5→6`)
  - `page-route` (**已全过不再重测**): **51 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw966_page-route.json`, 由 `17→18→38→42→47→49→51`)
  - `interception` (**可执行项 7/7 全过不再重测**): **7 PASS** / 1 FAIL (`tmp/pw957_interception.json`, 由 `3→5→6→7`, 仅余 1 项 SW 预期 FAIL)
  - `workers` (**已全过不再重测**): **3 PASS** / 0 FAIL / 0 TIMEOUT (`tmp/pw962_workers.json`, 由 `0→1→3`)
