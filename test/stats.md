# CapSurf 测试计划与测试结果统计总表 (`test/stats.md`)

> 更新时间：2026-11-24（基于本地真实基线文件与 `bin_dev/main` 实测汇总）

---

## 一、全局测试资产与预期基线总览

| 测试类别 | 资产路径 | 用例 / 文件规模 | 基线预期分布 (`*-expectations.json`) | 当前验证方式 |
| :--- | :--- | :--- | :--- | :--- |
| **Unit 白盒原生单测** | `test/unit/**/*.cp` | **138** 个 `.cp` 测试文件 | 原生白盒（需全绿） | `cap test` / `cpc brief` |
| **E2E 黑盒端到端** | `test/e2e/*.e2e.test.ts` | **392** 个 `.e2e.test.ts` 套件 | Smoke 3/3 必绿门禁 | `CAPSURF_BIN=./bin_dev/main python3 scripts/e2e_suite.py` |
| **WPT Web 平台测试** | `test/wpt/` | **9,725** 条基线预期 | `PASS: 7095`, `FAIL: 2256`, `TIMEOUT: 320`, `SKIP: 31`, `ERROR: 23` | `node test/wpt/scripts/wpt-batched.mjs -j 2` |
| **Playwright 兼容测试** | `test/pw/` | **619** 条基线预期 | `FAIL: 358`, `SKIP: 261` | Playwright CDP Runner |
| **Puppeteer 兼容测试** | `test/pptr/` | **79** 条基线预期 | `FAIL: 53`, `SKIP: 26` | Puppeteer CDP Runner |

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
