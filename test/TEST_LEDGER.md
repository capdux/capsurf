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
| 2026-11-22 14:55 | `page-check.spec.ts` | 8/13 PASS (aria role 4 FAIL, 45s截断) | PW Batch 4 |
| 2026-11-22 14:56 | `page-keyboard.spec.ts` | 3/7 PASS (方向键/textInput/microtask 4 FAIL, 45s截断) | PW Batch 4 |
| 2026-11-22 14:57 | `page-mouse.spec.ts` | 0/1 PASS (`detail: 0!=1`, 45s截断) | PW Batch 4 |
| 2026-11-22 14:58 | `page-add-script-tag.spec.ts` | 7/9 PASS (0 TIMEOUT) | PW Batch 5 (Phase 260 生效) |
| 2026-11-22 14:59 | `page-focus.spec.ts` | 1/7 PASS (`activeElement` undefined 6 FAIL) | PW Batch 5 |
| 2026-11-22 15:01 | `page-select-option.spec.ts` | 0/2 TIMEOUT (30s单用例超时, 90s截断) | PW Batch 5 |
| 2026-11-22 15:03 | `page-evaluate.spec.ts` | 20/21 PASS (`[21/95]` reload 阻塞, 90s截断) | PW Batch 5 |
| 2026-11-22 15:05 | `page-set-content.spec.ts` | 9/16 PASS (`XMLSerializer` 5 FAIL, 90s截断) | PW Batch 6 |
| 2026-11-22 15:07 | `page-wait-for-function.spec.ts` | 8/11 PASS (`capsurf:obj:2` 1 FAIL, 90s截断) | PW Batch 6 |
