# CapSurf 测试进度台账 (`TEST_LEDGER.md`)

用于持久化记录已完成测试的分片、测试时间与结果摘要，避免多轮/多子智能体重复测试。

## WPT 已测分片记录

| 测试时间 | 目录 / Filter | 分片 | 用例数 | 结果摘要 | 备注 |
| :--- | :--- | :--- | :---: | :--- | :--- |
| 2026-11-22 14:14 | `/css/css-break/` | `4/5`, `5/5` | 42, 41 | PASS 26/42, 29/41 | Phase 258 |
| 2026-11-22 14:14 | `/css/css-contain/` | `4/5`, `5/5` | 14, 13 | PASS 11/14, 11/13 | Phase 258 |
| 2026-11-22 14:14 | `/dom/nodes/` | `5/10` | 25 | PASS 24/25 | Phase 258 |
| 2026-11-22 14:14 | `/html/semantics/forms/` | `4/10`, `5/10` | 29, 29 | PASS 21/29, 17/29 | Phase 258 |
| 2026-11-22 14:14 | `/html/dom/elements/` | `4/5`, `5/5` | 39, 39 | PASS 21/39, 20/39 | Phase 258 |
| 2026-11-22 14:39 | `/css/css-animations/` | `4/5`, `5/5` | 10/11 | PASS 3/10, 3/10 | Batch 30 (60s截断) |
| 2026-11-22 14:42 | `/css/cssom/` | `4/5`, `5/5` | 10/24 | PASS 5/10, 4/10 | Batch 30 (60s截断) |
| 2026-11-22 14:43 | `/css/cssom-view/` | `4/5` | 10/21 | PASS 4/10 | Batch 30 (60s截断) |
| 2026-11-22 14:46 | `/css/cssom-view/` | `5/5` | 21 | PASS 11 / FAIL 8 / TIMEOUT 2 | Batch 31 |
| 2026-11-22 14:48 | `/dom/events/` | `4/5`, `5/5` | 26, 25 | PASS 7/26, 7/25 | Batch 31 |
| 2026-11-22 14:50 | `/html/semantics/embedded-content/` | `1/5`, `2/5` | 40, 40 | CDP JSON 崩溃 (前15/14条中断) | Batch 31 |
| 2026-11-22 14:51 | `/css/selectors/` | `1/5`, `2/5` | 14, 14 | PASS 6/14 (12s), PASS 5/14 (27s) | Batch 32 (`--skip-expected-timeouts`) |
| 2026-11-22 14:53 | `/css/css-flexbox/` | `1/5` | 53 | PASS 24 / FAIL 21 / TIMEOUT 8 (90s) | Batch 32 (`--skip-expected-timeouts`) |
| 2026-11-22 14:53 | `/dom/collections/` | `1/2` | 6 | PASS 3 / FAIL 3 / TIMEOUT 0 (7s) | Batch 32 (`--skip-expected-timeouts`) |
| 2026-11-22 14:54 | `/fetch/api/basic/` | `1/3` | 11 | PASS 1 / FAIL 8 / TIMEOUT 2 (25s) | Batch 32 (`--skip-expected-timeouts`) |
| 2026-11-22 14:57 | `/css/css-values/` | `1/5`, `2/5` | 23/70, 34/70 | PASS 2/23, 6/34 (90s截断) | Batch 33 (`--skip-expected-timeouts`) |
| 2026-11-22 14:59 | `/css/css-grid/` | `1/5` | 89/177 | PASS 17 / FAIL 66 / TIMEOUT 6 (90s截断) | Batch 33 (`--skip-expected-timeouts`) |
| 2026-11-22 15:01 | `/dom/traversal/` | `1/2` | 9 | PASS 2 / FAIL 6 / TIMEOUT 1 (18s) | Batch 33 (`--skip-expected-timeouts`) |
| 2026-11-22 15:01 | `/dom/ranges/` | `1/3` | 12 | PASS 2 / FAIL 5 / TIMEOUT 3 / SKIP 2 (90s) | Batch 33 (`--skip-expected-timeouts`) |
| 2026-11-22 15:03 | `/css/css-flexbox/` | `2/5` | 52 | PASS 23 / FAIL 19 / TIMEOUT 10 (90s截断) | Batch 34 (`--skip-expected-timeouts`) |
| 2026-11-22 15:05 | `/css/css-grid/` | `2/5` | 89/176 | PASS 19 / FAIL 65 / TIMEOUT 5 (90s截断) | Batch 34 (`--skip-expected-timeouts`) |
| 2026-11-22 15:05 | `/dom/collections/` | `2/2` | 5 | PASS 3 / FAIL 2 / TIMEOUT 0 (7s) | Batch 34 (`--skip-expected-timeouts`) |
| 2026-11-22 15:06 | `/dom/traversal/` | `2/2` | 8 | PASS 2 / FAIL 5 / TIMEOUT 1 (18s) | Batch 34 (`--skip-expected-timeouts`) |
| 2026-11-22 15:07 | `/dom/ranges/` | `2/3` | 12 | PASS 3 / FAIL 3 / TIMEOUT 4 / SKIP 2 (55s) | Batch 34 (`--skip-expected-timeouts`) |
| 2026-11-22 15:27 | `/dom/ranges/` | `3/3` | 12 | PASS 1 / FAIL 4 / TIMEOUT 6 / SKIP 1 (90s) | Batch 36 (`--skip-expected-timeouts`) |
| 2026-11-22 15:29 | `/css/css-position/` | `1/5` | 49/54 | PASS 15 / FAIL 28 / TIMEOUT 5 / ERROR 1 (90s) | Batch 36 (`--skip-expected-timeouts`) |
| 2026-11-22 15:29 | `/css/css-display/` | `1/5` | 11 | PASS 4 / FAIL 7 / TIMEOUT 0 (18.5s) | Batch 36 (`--skip-expected-timeouts`) |
| 2026-11-22 15:30 | `/css/css-color/` | `1/5` | 12 | PASS 0 / FAIL 11 / TIMEOUT 1 (41.5s) | Batch 36 (`--skip-expected-timeouts`) |
| 2026-11-22 15:30 | `/css/css-overflow/` | `1/5` | 7 | PASS 0 / FAIL 7 / TIMEOUT 0 (10.4s) | Batch 36 (`--skip-expected-timeouts`) |
| 2026-11-22 15:32 | `/css/css-position/` | `2/5` | 54 | PASS 13 / FAIL 32 / TIMEOUT 9 (90s) | Batch 37 (`--skip-expected-timeouts`) |
| 2026-11-22 15:34 | `/css/css-display/` | `2/5` | 11 | PASS 2 / FAIL 8 / TIMEOUT 1 (90s) | Batch 37 (`--skip-expected-timeouts`) |
| 2026-11-22 15:35 | `/css/css-color/` | `2/5` | 11 | PASS 1 / FAIL 9 / TIMEOUT 1 (76.7s) | Batch 37 (`--skip-expected-timeouts`) |
| 2026-11-22 15:36 | `/css/css-overflow/` | `2/5` | 7 | PASS 1 / FAIL 6 / TIMEOUT 0 (9.9s) | Batch 37 (`--skip-expected-timeouts`) |
| 2026-11-22 15:36 | `/css/css-cascade/` | `1/3` | 30 | PASS 10 / FAIL 19 / TIMEOUT 1 (22.4s) | Batch 37 (`--skip-expected-timeouts`) |
| 2026-11-22 15:37 | `/css/css-cascade/` | `2/3` | 30 | PASS 12 / FAIL 18 / TIMEOUT 0 (12s) | Batch 38 (`--skip-expected-timeouts`) |
| 2026-11-22 15:38 | `/css/css-cascade/` | `3/3` | 30 | PASS 14 / FAIL 15 / TIMEOUT 1 (79s) | Batch 38 (`--skip-expected-timeouts`) |
| 2026-11-22 15:39 | `/css/css-lists/` | `1/3` | 61 | PASS 5 / FAIL 56 / TIMEOUT 0 (22s) | Batch 38 (`--skip-expected-timeouts`) |
| 2026-11-22 15:40 | `/css/css-tables/` | `1/5` | 4 | PASS 1 / FAIL 2 / TIMEOUT 1 (65s) | Batch 38 (`--skip-expected-timeouts`) |
| 2026-11-22 15:41 | `/css/css-transforms/` | `1/5` | 7 | PASS 0 / FAIL 6 / TIMEOUT 1 (19s) | Batch 38 (`--skip-expected-timeouts`) |
| 2026-11-22 15:42 | `/css/css-lists/` | `2/3` | 61 | PASS 11 / FAIL 50 / TIMEOUT 0 (21s) | Batch 39 (`--skip-expected-timeouts`) |
| 2026-11-22 15:42 | `/css/css-lists/` | `3/3` | 60 | PASS 6 / FAIL 54 / TIMEOUT 0 (20s) | Batch 39 (`--skip-expected-timeouts`) |
| 2026-11-22 15:42 | `/css/css-tables/` | `2/5` | 4 | PASS 1 / FAIL 3 / TIMEOUT 0 (6s) | Batch 39 (`--skip-expected-timeouts`) |
| 2026-11-22 15:43 | `/css/css-transforms/` | `2/5` | 6 | PASS 0 / FAIL 6 / TIMEOUT 0 (7s) | Batch 39 (`--skip-expected-timeouts`) |
| 2026-11-22 15:44 | `/css/css-variables/` | `1/3` | 75 | PASS 19 / FAIL 56 / TIMEOUT 0 (19s) | Batch 39 (`--skip-expected-timeouts`) |
| 2026-11-22 15:46 | `/css/css-variables/` | `2/3` | 75 | PASS 20 / FAIL 52 / TIMEOUT 3 (90s) | Batch 40 (`--skip-expected-timeouts`) |
| 2026-11-22 15:47 | `/css/css-variables/` | `3/3` | 74 | PASS 21 / FAIL 51 / TIMEOUT 2 (40s) | Batch 40 (`--skip-expected-timeouts`) |
| 2026-11-22 15:48 | `/css/css-fonts/` | `1/5` | 25 | PASS 7 / FAIL 15 / TIMEOUT 3 (90s) | Batch 40 (`--skip-expected-timeouts`) |
| 2026-11-22 15:49 | `/css/css-fonts/` | `2/5` | 24 | PASS 6 / FAIL 17 / TIMEOUT 1 (42s) | Batch 40 (`--skip-expected-timeouts`) |
| 2026-11-22 15:51 | `/dom/nodes/` | `1/10` | 26 | PASS 8 / FAIL 13 / TIMEOUT 4 / SKIP 1 (90s) | Batch 40 (`--skip-expected-timeouts`) |
| 2026-11-22 15:53 | `/dom/nodes/` | `2/10` | 25 | PASS 8 / FAIL 12 / TIMEOUT 5 (90s) | Batch 41 (`--skip-expected-timeouts`) |
| 2026-11-22 15:53 | `/dom/nodes/` | `3/10` | 25 | PASS 8 / FAIL 13 / TIMEOUT 4 (26s) | Batch 41 (`--skip-expected-timeouts`) |
| 2026-11-22 15:54 | `/dom/events/` | `1/5` | 26 | PASS 6 / FAIL 12 / TIMEOUT 8 (28s) | Batch 41 (`--skip-expected-timeouts`) |
| 2026-11-22 15:56 | `/dom/events/` | `2/5` | 26 | PASS 4 / FAIL 15 / TIMEOUT 6 / ERROR 1 (90s) | Batch 41 (`--skip-expected-timeouts`) |
| 2026-11-22 15:57 | `/html/dom/elements/` | `1/5` | 39 | PASS 22 / FAIL 15 / TIMEOUT 2 (53s) | Batch 41 (`--skip-expected-timeouts`) |
| 2026-11-22 15:59 | `/html/dom/elements/` | `2/5` | 39 | PASS 18 / FAIL 16 / TIMEOUT 5 (80s) | Batch 42 (`--skip-expected-timeouts`) |
| 2026-11-22 16:00 | `/html/dom/elements/` | `3/5` | 39 | PASS 18 / FAIL 15 / TIMEOUT 6 (90s) | Batch 42 (`--skip-expected-timeouts`) |
| 2026-11-22 16:01 | `/html/semantics/forms/` | `1/10` | 29 | PASS 21 / FAIL 7 / TIMEOUT 1 (20s) | Batch 42 (`--skip-expected-timeouts`) |
| 2026-11-22 16:02 | `/html/semantics/forms/` | `2/10` | 29 | PASS 21 / FAIL 7 / TIMEOUT 1 (22s) | Batch 42 (`--skip-expected-timeouts`) |
| 2026-11-22 16:03 | `/dom/events/` | `3/5` | 26 | PASS 2 / FAIL 14 / TIMEOUT 8 / SKIP 2 (90s) | Batch 42 (`--skip-expected-timeouts`) |
| 2026-11-22 16:04 | `/html/semantics/forms/` | `3/10` | 29 | PASS 19 / FAIL 9 / TIMEOUT 1 (22s) | Batch 43 (`--skip-expected-timeouts`) |
| 2026-11-22 16:05 | `/html/semantics/forms/` | `6/10` | 29 | PASS 18 / FAIL 7 / TIMEOUT 4 (41s) | Batch 43 (`--skip-expected-timeouts`) |
| 2026-11-22 16:06 | `/dom/nodes/` | `4/10` | 25 | PASS 9 / FAIL 9 / TIMEOUT 7 (41s) | Batch 43 (`--skip-expected-timeouts`) |
| 2026-11-22 16:07 | `/dom/nodes/` | `6/10` | 25 | PASS 10 / FAIL 10 / TIMEOUT 4 / ERROR 1 (47s) | Batch 43 (`--skip-expected-timeouts`) |
| 2026-11-22 16:09 | `/css/css-flexbox/` | `3/5` | 52 | PASS 21 / FAIL 24 / TIMEOUT 7 (90s) | Batch 43 (`--skip-expected-timeouts`) |
| 2026-11-22 16:11 | `/html/semantics/forms/` | `7/10` | 29 | PASS 19 / FAIL 6 / TIMEOUT 4 (57s) | Batch 44 (`--skip-expected-timeouts`) |
| 2026-11-22 16:12 | `/html/semantics/forms/` | `8/10` | 29 | PASS 19 / FAIL 6 / TIMEOUT 3 / ERROR 1 (54s) | Batch 44 (`--skip-expected-timeouts`) |
| 2026-11-22 16:13 | `/dom/nodes/` | `7/10` | 25 | PASS 10 / FAIL 12 / TIMEOUT 3 (46s) | Batch 44 (`--skip-expected-timeouts`) |
| 2026-11-22 16:14 | `/dom/nodes/` | `8/10` | 25 | PASS 9 / FAIL 12 / TIMEOUT 4 (90s) | Batch 44 (`--skip-expected-timeouts`) |
| 2026-11-22 16:16 | `/css/css-flexbox/` | `4/5` | 52 | PASS 15 / FAIL 28 / TIMEOUT 9 (90s) | Batch 44 (`--skip-expected-timeouts`) |
| 2026-11-22 16:17 | `/html/semantics/forms/` | `9/10` | 29 | PASS 23 / FAIL 6 / TIMEOUT 0 (29s) | Batch 45 (`--skip-expected-timeouts`) |

## Playwright (PW) 已测 Spec 记录

| 测试时间 | Spec | 结果 | 备注 |
| :--- | :--- | :--- | :--- |
| 2026-11-22 | `jshandle-as-element.spec.ts` | 4/4 PASS | Phase 256 |
| 2026-11-22 | `elementhandle-query-selector.spec.ts` | 6/6 PASS | Phase 256 |
| 2026-11-22 | `elementhandle-eval-on-selector.spec.ts` | 6/6 PASS | Phase 256 |
| 2026-11-22 | `jshandle-properties.spec.ts` | 7/7 PASS | Phase 259 |
| 2026-11-22 | `jshandle-json-value.spec.ts` | 3/3 PASS | Phase 259 |
| 2026-11-22 | `eval-on-selector-all.spec.ts` | 9/9 PASS | Phase 259 |
| 2026-11-22 | `elementhandle-convenience.spec.ts` | 13/14 PASS | Phase 258 |
| 2026-11-22 | `elementhandle-press.spec.ts` | 4/5 PASS | Phase 258 |
| 2026-11-22 | `elementhandle-type.spec.ts` | 4/5 PASS | Phase 259 |
| 2026-11-22 | `elementhandle-wait-for-element-state.spec.ts` | 8/12 PASS | Phase 259 |
| 2026-11-22 | `page-add-init-script.spec.ts` | 9/11 PASS | Phase 259 |
| 2026-11-22 | `page-fill.spec.ts` | 8/9 PASS | 待复测 invalid-type 修复 |
| 2026-11-22 14:55 | `page-check.spec.ts` | 10/16 PASS (aria role 4 + trial 2 FAIL) | PW Batch 15 复测 |
| 2026-11-22 14:56 | `page-keyboard.spec.ts` | 3/7 PASS (方向键/textInput/microtask 4 FAIL, 45s截断) | PW Batch 4 |
| 2026-11-22 14:57 | `page-mouse.spec.ts` | 0/1 PASS (`detail: 0!=1`, 45s截断) | PW Batch 4 |
| 2026-11-22 14:58 | `page-add-script-tag.spec.ts` | 7/9 PASS (0 TIMEOUT) | PW Batch 5 (Phase 260 生效) |
| 2026-11-22 14:59 | `page-focus.spec.ts` | 1/7 PASS (`activeElement` undefined 6 FAIL) | PW Batch 5 |
| 2026-11-22 15:01 | `page-select-option.spec.ts` | 0/2 TIMEOUT (30s单用例超时, 90s截断) | PW Batch 5 |
| 2026-11-22 15:03 | `page-evaluate.spec.ts` | 20/21 PASS (`[21/95]` reload 阻塞, 90s截断) | PW Batch 5 |
| 2026-11-22 15:05 | `page-set-content.spec.ts` | 9/16 PASS (`XMLSerializer` 5 FAIL, 90s截断) | PW Batch 6 |
| 2026-11-22 15:07 | `page-wait-for-function.spec.ts` | 8/11 PASS (`capsurf:obj:2` 1 FAIL, 90s截断) | PW Batch 6 |
| 2026-11-22 15:27 | `page-focus.spec.ts` | 2/7 PASS (4期望偏差) | PW Batch 8 |
| 2026-11-22 15:29 | `page-set-content.spec.ts` | 9/15 PASS (XMLSerializer已通, 余title/doctype 5 FAIL) | PW Batch 8 |
| 2026-11-22 15:31 | `page-keyboard.spec.ts` | 6/16 PASS (location 9 FAIL, 90s截断) | PW Batch 8 |
| 2026-11-22 15:32 | `page-screenshot.spec.ts` | 0/2 TIMEOUT (30s单用例超时, 90s截断) | PW Batch 8 |
| 2026-11-22 15:35 | `page-wait-for-selector-1.spec.ts` | 11/14 PASS (MutationObserver/shadow 2 FAIL + 1 TIMEOUT) | PW Batch 9 |
| 2026-11-22 15:36 | `page-wait-for-selector-2.spec.ts` | 22/23 PASS (0 FAIL, 1 TIMEOUT) | PW Batch 9 |
| 2026-11-22 15:38 | `page-goto.spec.ts` | 7/18 PASS (file:// + response null 11 FAIL, 90s截断) | PW Batch 9 |
| 2026-11-22 15:40 | `page-history.spec.ts` | 0/3 PASS (goBack TIMEOUT / pushState FAIL, 90s截断) | PW Batch 9 |
| 2026-11-22 15:41 | `locator-query.spec.ts` | 21/21 PASS (19.8s) | PW Batch 10 |
| 2026-11-22 15:41 | `locator-convenience.spec.ts` | 20/21 PASS (indeterminate uncheck 1 FAIL, 26.2s) | PW Batch 10 |
| 2026-11-22 15:42 | `locator-click.spec.ts` | 2/5 PASS (3 FAIL, 26.0s) | PW Batch 10 |
| 2026-11-22 15:44 | `page-expose-function.spec.ts` | 0/1 TIMEOUT (卡在[1/18], 90s截断) | PW Batch 10 |
| 2026-11-22 15:46 | `locator-misc-1.spec.ts` | 3/8 PASS (buffer修复前, 90s截断) | PW Batch 11 |
| 2026-11-22 15:48 | `locator-misc-2.spec.ts` | 3/6 PASS (90s截断) | PW Batch 11 |
| 2026-11-22 15:49 | `locator-is-visible.spec.ts` | 7/8 PASS (details hidden 1 FAIL) | PW Batch 11 |
| 2026-11-22 15:50 | `page-basic.spec.ts` | 11/16 PASS (location.hash/frame/UA/webdriver 5 FAIL) | PW Batch 11 |
| 2026-11-22 15:52 | `locator-frame.spec.ts` | 6/9 PASS (`ERR_BLOCKED_BY_CLIENT` 3 FAIL) | PW Batch 12 |
| 2026-11-22 15:52 | `page-click-timeout-1.spec.ts` | 2/2 PASS (12.9s) | PW Batch 12 |
| 2026-11-22 15:54 | `page-event-console.spec.ts` | 1/9 PASS (90s截断) | PW Batch 12 |
| 2026-11-22 15:54 | `page-network-request.spec.ts` | 2/21 PASS (90s截断) | PW Batch 12 |
| 2026-11-22 15:57 | `locator-convenience.spec.ts` | 21/21 PASS (20.7s, exit 0) | PW Batch 13 (Phase 266 全绿) |
| 2026-11-22 15:57 | `page-click-timeout-2.spec.ts` | 2/2 PASS (13.9s, exit 0) | PW Batch 13 |
| 2026-11-22 15:58 | `page-click-timeout-3.spec.ts` | 3/4 PASS (waiting 500ms 1 FAIL, 27.6s) | PW Batch 13 |
| 2026-11-22 16:00 | `page-network-response.spec.ts` | 0/10 PASS (`page.goto` null 10 FAIL, 90s截断) | PW Batch 13 |
| 2026-11-22 16:02 | `page-click-react.spec.ts` | 0/3 PASS (1 FAIL + 2 TIMEOUT) | PW Batch 14 |
| 2026-11-22 16:03 | `page-emulate-media.spec.ts` | 3/12 PASS (emulateMedia/matchMedia 9 FAIL) | PW Batch 14 |
| 2026-11-22 16:05 | `elementhandle-bounding-box.spec.ts` | 3/7 PASS (frame/force layout PASS, scroll offset等4 FAIL/TIMEOUT) | PW Batch 14 |
| 2026-11-22 16:06 | `elementhandle-misc.spec.ts` | 5/7 PASS (check全通, 90s截断) | PW Batch 14 |
| 2026-11-22 16:09 | `jshandle-to-string.spec.ts` | 3/6 PASS (sparse/Symbol/RegExp 3 FAIL, 20.9s) | PW Batch 15 |
| 2026-11-22 16:10 | `jshandle-evaluate.spec.ts` | 2/2 PASS (2.1s, exit 0) | PW Batch 15 |
| 2026-11-22 16:10 | `locator-element-handle.spec.ts` | 5/5 PASS (6.2s, exit 0) | PW Batch 15 |
| 2026-11-22 16:11 | `page-add-style-tag.spec.ts` | 1/4 PASS (addStyleTag url 3 FAIL, 21.9s) | PW Batch 15 |
| 2026-11-22 16:12 | `elementhandle-select-text.spec.ts` | 2/5 PASS (getSelection 3 FAIL) | PW Batch 16 |
| 2026-11-22 16:14 | `page-aria-snapshot.spec.ts` | 17/20 PASS (details/pseudo 3 FAIL, 90s截断) | PW Batch 16 |
| 2026-11-22 16:15 | `page-set-input-files.spec.ts` | 0/8 PASS (ownerFrame null 7 FAIL, 90s截断) | PW Batch 16 |
| 2026-11-22 16:16 | `frame-evaluate.spec.ts` | 2/8 PASS (0 deviations, 符合预期) | PW Batch 16 |
