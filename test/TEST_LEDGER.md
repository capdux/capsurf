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
| 2026-11-22 16:18 | `/html/semantics/forms/` | `10/10` | 28 | PASS 22 / FAIL 5 / TIMEOUT 1 (36s) | Batch 45 (`--skip-expected-timeouts`) |
| 2026-11-22 16:19 | `/dom/nodes/` | `9/10` | 25 | PASS 9 / FAIL 14 / TIMEOUT 2 (39s) | Batch 45 (`--skip-expected-timeouts`) |
| 2026-11-22 16:21 | `/dom/nodes/` | `10/10` | 25 | PASS 8 / FAIL 13 / TIMEOUT 4 (90s) | Batch 45 (`--skip-expected-timeouts`) |
| 2026-11-22 16:22 | `/css/css-flexbox/` | `5/5` | 52 | PASS 22 / FAIL 19 / TIMEOUT 11 (90s) | Batch 45 (`--skip-expected-timeouts`) |
| 2026-11-22 16:25 | `/css/css-position/` | `3/5` | 54 | PASS 20 / FAIL 28 / TIMEOUT 6 (90s) | Batch 46 (`--skip-expected-timeouts`) |
| 2026-11-22 16:27 | `/css/css-position/` | `4/5` | 54 | PASS 16 / FAIL 33 / TIMEOUT 5 (90s) | Batch 46 (`--skip-expected-timeouts`) |
| 2026-11-22 16:28 | `/css/css-position/` | `5/5` | 53 | PASS 11 / FAIL 37 / TIMEOUT 5 (90s) | Batch 46 (`--skip-expected-timeouts`) |
| 2026-11-22 16:30 | `/css/css-display/` | `3/5` | 10 | PASS 4 / FAIL 5 / TIMEOUT 1 (90s) | Batch 46 (`--skip-expected-timeouts`) |
| 2026-11-22 16:30 | `/css/css-display/` | `4/5` | 10 | PASS 4 / FAIL 6 / TIMEOUT 0 (8s) | Batch 46 (`--skip-expected-timeouts`) |
| 2026-11-22 16:32 | `/css/css-display/` | `5/5` | 10 | PASS 2 / FAIL 7 / TIMEOUT 1 (18s) | Batch 47 (`--skip-expected-timeouts`) |
| 2026-11-22 16:33 | `/css/css-color/` | `3/5` | 11 | PASS 0 / FAIL 7 / TIMEOUT 4 (90s) | Batch 47 (`--skip-expected-timeouts`) |
| 2026-11-22 16:34 | `/css/css-color/` | `4/5` | 11 | PASS 0 / FAIL 8 / TIMEOUT 3 (63s) | Batch 47 (`--skip-expected-timeouts`) |
| 2026-11-22 16:36 | `/css/css-color/` | `5/5` | 11 | PASS 1 / FAIL 6 / TIMEOUT 4 (81s) | Batch 47 (`--skip-expected-timeouts`) |
| 2026-11-22 16:36 | `/css/css-overflow/` | `3/5` | 7 | PASS 2 / FAIL 5 / TIMEOUT 0 (7s) | Batch 47 (`--skip-expected-timeouts`) |
| 2026-11-22 16:38 | `/css/css-overflow/` | `4/5` | 6 | PASS 0 / FAIL 6 / TIMEOUT 0 (7s) | Batch 48 (`--skip-expected-timeouts`) |
| 2026-11-22 16:38 | `/css/css-overflow/` | `5/5` | 6 | PASS 3 / FAIL 3 / TIMEOUT 0 (7s) | Batch 48 (`--skip-expected-timeouts`) |
| 2026-11-22 16:38 | `/css/css-tables/` | `3/5` | 4 | PASS 0 / FAIL 4 / TIMEOUT 0 (7s) | Batch 48 (`--skip-expected-timeouts`) |
| 2026-11-22 16:38 | `/css/css-tables/` | `4/5` | 3 | PASS 1 / FAIL 2 / TIMEOUT 0 (6s) | Batch 48 (`--skip-expected-timeouts`) |
| 2026-11-22 16:39 | `/css/css-tables/` | `5/5` | 3 | PASS 1 / FAIL 2 / TIMEOUT 0 (6s) | Batch 48 (`--skip-expected-timeouts`) |
| 2026-11-22 16:40 | `/css/css-transforms/` | `3/5` | 6 | PASS 1 / FAIL 5 / TIMEOUT 0 | Batch 49 (`--skip-expected-timeouts`) |
| 2026-11-22 16:41 | `/css/css-transforms/` | `4/5` | 6 | PASS 0 / FAIL 6 / TIMEOUT 0 | Batch 49 (`--skip-expected-timeouts`) |
| 2026-11-22 16:41 | `/css/css-transforms/` | `5/5` | 6 | PASS 1 / FAIL 5 / TIMEOUT 0 | Batch 49 (`--skip-expected-timeouts`) |
| 2026-11-22 16:43 | `/css/css-values/` | `3/5` | 50/70 | PASS 9 / FAIL 26 / TIMEOUT 15 (90s截断) | Batch 49 (`--skip-expected-timeouts`) |
| 2026-11-22 16:44 | `/css/css-values/` | `4/5` | 45/70 | PASS 11 / FAIL 17 / TIMEOUT 17 (90s截断) | Batch 49 (`--skip-expected-timeouts`) |
| 2026-11-22 16:46 | `/css/css-values/` | `5/5` | 22/70 | PASS 6 / FAIL 9 / ERROR 3 / TIMEOUT 4 (90s截断) | Batch 50 (`--skip-expected-timeouts`) |
| 2026-11-22 16:47 | `/css/css-text/` | `1/5` | 25 | PASS 4 / FAIL 19 / TIMEOUT 2 (51s) | Batch 50 (`--skip-expected-timeouts`) |
| 2026-11-22 16:48 | `/css/css-text/` | `2/5` | 25 | PASS 4 / FAIL 20 / TIMEOUT 1 (42s) | Batch 50 (`--skip-expected-timeouts`) |
| 2026-11-22 16:50 | `/css/css-sizing/` | `1/5` | 7 | PASS 0 / FAIL 4 / TIMEOUT 3 (90s截断) | Batch 50 (`--skip-expected-timeouts`) |
| 2026-11-22 16:50 | `/css/css-sizing/` | `2/5` | 7 | PASS 1 / FAIL 3 / TIMEOUT 3 (90s截断) | Batch 50 (`--skip-expected-timeouts`) |
| 2026-11-22 16:52 | `/css/css-text/` | `3/5` | 25 | PASS 7 / FAIL 18 / TIMEOUT 0 (15s) | Batch 51 (`--skip-expected-timeouts`) |
| 2026-11-22 16:52 | `/css/css-text/` | `4/5` | 25 | PASS 6 / FAIL 19 / TIMEOUT 0 (16s) | Batch 51 (`--skip-expected-timeouts`) |
| 2026-11-22 16:53 | `/css/css-text/` | `5/5` | 25 | PASS 3 / FAIL 22 / TIMEOUT 0 (16s) | Batch 51 (`--skip-expected-timeouts`) |
| 2026-11-22 16:54 | `/css/css-sizing/` | `3/5` | 6 | PASS 0 / FAIL 2 / TIMEOUT 4 (75s) | Batch 51 (`--skip-expected-timeouts`) |
| 2026-11-22 16:56 | `/css/css-sizing/` | `4/5` | 6 | PASS 0 / FAIL 2 / TIMEOUT 4 (74s) | Batch 51 (`--skip-expected-timeouts`) |
| 2026-11-22 16:58 | `/css/css-sizing/` | `5/5` | 6 | PASS 0 / FAIL 2 / TIMEOUT 4 (73s) | Batch 52 (`--skip-expected-timeouts`) |
| 2026-11-22 16:59 | `/css/css-backgrounds/` | `1/5` | 23 | PASS 6 / FAIL 16 / TIMEOUT 1 (90s截断) | Batch 52 (`--skip-expected-timeouts`) |
| 2026-11-22 17:01 | `/css/css-backgrounds/` | `2/5` | 23 | PASS 9 / FAIL 13 / TIMEOUT 1 (90s截断) | Batch 52 (`--skip-expected-timeouts`) |
| 2026-11-22 17:02 | `/css/css-backgrounds/` | `3/5` | 23 | PASS 6 / FAIL 16 / TIMEOUT 1 (90s截断) | Batch 52 (`--skip-expected-timeouts`) |
| 2026-11-22 17:03 | `/css/css-backgrounds/` | `4/5` | 23 | PASS 5 / FAIL 18 / TIMEOUT 0 (12s) | Batch 52 (`--skip-expected-timeouts`) |
| 2026-11-22 17:05 | `/css/css-backgrounds/` | `5/5` | 23 | PASS 7 / FAIL 15 / TIMEOUT 1 (90s截断) | Batch 53 (`--skip-expected-timeouts`) |
| 2026-11-22 17:06 | `/css/css-box/` | `1/3` | 11 | PASS 6 / FAIL 5 / TIMEOUT 0 | Batch 53 (`--skip-expected-timeouts`) |
| 2026-11-22 17:06 | `/css/css-box/` | `2/3` | 11 | PASS 1 / FAIL 10 / TIMEOUT 0 | Batch 53 (`--skip-expected-timeouts`) |
| 2026-11-22 17:07 | `/css/css-box/` | `3/3` | 11 | PASS 6 / FAIL 5 / TIMEOUT 0 | Batch 53 (`--skip-expected-timeouts`) |
| 2026-11-22 17:08 | `/css/css-ui/` | `1/5` | 10 | PASS 0 / FAIL 8 / ERROR 1 / TIMEOUT 1 (90s截断) | Batch 53 (`--skip-expected-timeouts`) |
| 2026-11-22 17:08 | `/css/css-ui/` | `2/5` | 10 | PASS 0 / FAIL 10 / TIMEOUT 0 | Batch 54 (`--skip-expected-timeouts`) |
| 2026-11-22 17:09 | `/css/css-ui/` | `3/5` | 9 | PASS 1 / FAIL 8 / TIMEOUT 0 | Batch 54 (`--skip-expected-timeouts`) |
| 2026-11-22 17:10 | `/css/css-ui/` | `4/5` | 9 | PASS 2 / FAIL 6 / TIMEOUT 1 (90s截断) | Batch 54 (`--skip-expected-timeouts`) |
| 2026-11-22 17:11 | `/css/css-ui/` | `5/5` | 9 | PASS 5 / FAIL 4 / TIMEOUT 0 | Batch 54 (`--skip-expected-timeouts`) |
| 2026-11-22 17:13 | `/css/css-flexbox/` | `3/5` | 52 | PASS 21 / FAIL 24 / TIMEOUT 7 (90s截断) | Batch 54 (`--skip-expected-timeouts`) |
| 2026-11-22 17:15 | `/css/css-flexbox/` | `4/5` | 52 | PASS 16 / FAIL 27 / TIMEOUT 9 (90s截断) | Batch 55 (`--skip-expected-timeouts`) |
| 2026-11-22 17:17 | `/css/css-flexbox/` | `5/5` | 52 | PASS 22 / FAIL 19 / TIMEOUT 11 (90s截断) | Batch 55 (`--skip-expected-timeouts`) |
| 2026-11-22 17:19 | `/css/css-position/` | `3/5` | 54 | PASS 20 / FAIL 28 / TIMEOUT 6 (90s截断) | Batch 55 (`--skip-expected-timeouts`) |
| 2026-11-22 17:20 | `/css/css-position/` | `4/5` | 54 | PASS 16 / FAIL 33 / TIMEOUT 5 (90s截断) | Batch 55 (`--skip-expected-timeouts`) |
| 2026-11-22 17:22 | `/css/css-position/` | `5/5` | 53 | PASS 11 / FAIL 38 / TIMEOUT 4 (90s截断) | Batch 55 (`--skip-expected-timeouts`) |
| 2026-11-22 17:24 | `/css/css-display/` | `3/5` | 10 | PASS 4 / FAIL 5 / TIMEOUT 1 (90s截断) | Batch 56 (`--skip-expected-timeouts`) |
| 2026-11-22 17:24 | `/css/css-display/` | `4/5` | 10 | PASS 4 / FAIL 6 / TIMEOUT 0 (8s) | Batch 56 (`--skip-expected-timeouts`) |
| 2026-11-22 17:25 | `/css/css-display/` | `5/5` | 10 | PASS 2 / FAIL 7 / TIMEOUT 1 (18s) | Batch 56 (`--skip-expected-timeouts`) |
| 2026-11-22 17:27 | `/css/css-grid/` | `3/5` | 89/176 | PASS 21 / FAIL 65 / TIMEOUT 3 (90s截断) | Batch 56 (`--skip-expected-timeouts`) |
| 2026-11-22 17:28 | `/css/css-grid/` | `4/5` | 89/176 | PASS 18 / FAIL 66 / TIMEOUT 5 (90s截断) | Batch 56 (`--skip-expected-timeouts`) |
| 2026-11-22 17:30 | `/css/css-grid/` | `5/5` | 132/176 | PASS 31 / FAIL 97 / TIMEOUT 4 (90s截断) | Batch 57 (`--skip-expected-timeouts`) |
| 2026-11-22 17:31 | `/css/css-fonts/` | `3/5` | 24 | PASS 7 / FAIL 16 / TIMEOUT 1 (22s) | Batch 57 (`--skip-expected-timeouts`) |
| 2026-11-22 17:33 | `/css/css-fonts/` | `4/5` | 24 | PASS 4 / FAIL 18 / TIMEOUT 2 (90s截断) | Batch 57 (`--skip-expected-timeouts`) |
| 2026-11-22 17:33 | `/css/css-fonts/` | `5/5` | 24 | PASS 5 / FAIL 19 / TIMEOUT 0 (11s) | Batch 57 (`--skip-expected-timeouts`) |
| 2026-11-22 17:34 | `/css/css-animations/` | `1/5` | 12 | PASS 6 / FAIL 6 / TIMEOUT 0 (39s) | Batch 57 (`--skip-expected-timeouts`) |
| 2026-11-22 17:36 | `/css/css-animations/` | `2/5` | 12 | PASS 2 / FAIL 9 / TIMEOUT 1 (90s截断) | Batch 58 (`--skip-expected-timeouts`) |
| 2026-11-22 17:37 | `/css/css-animations/` | `3/5` | 12 | PASS 4 / FAIL 8 / TIMEOUT 0 (8s) | Batch 58 (`--skip-expected-timeouts`) |
| 2026-11-22 17:37 | `/css/cssom/` | `1/5` | 25 | PASS 9 / FAIL 15 / TIMEOUT 1 (12s) | Batch 58 (`--skip-expected-timeouts`) |
| 2026-11-22 17:38 | `/css/cssom/` | `2/5` | 25 | PASS 13 / FAIL 11 / TIMEOUT 1 (9s) | Batch 58 (`--skip-expected-timeouts`) |
| 2026-11-22 17:38 | `/css/cssom/` | `3/5` | 24 | PASS 9 / FAIL 13 / ERROR 1 / TIMEOUT 1 (18s) | Batch 58 (`--skip-expected-timeouts`) |
| 2026-11-22 17:39 | `/css/selectors/` | `3/5` | 13 | PASS 6 / FAIL 6 / TIMEOUT 1 | Batch 59 (`--skip-expected-timeouts`) |
| 2026-11-22 17:40 | `/css/selectors/` | `4/5` | 13 | PASS 8 / FAIL 5 / TIMEOUT 0 | Batch 59 (`--skip-expected-timeouts`) |
| 2026-11-22 17:41 | `/css/selectors/` | `5/5` | 13 | PASS 6 / FAIL 5 / TIMEOUT 2 | Batch 59 (`--skip-expected-timeouts`) |
| 2026-11-22 17:42 | `/dom/events/` | `2/5` | 26 | PASS 5 / FAIL 14 / ERROR 1 / TIMEOUT 6 (90s截断) | Batch 59 (`--skip-expected-timeouts`) |
| 2026-11-22 17:43 | `/dom/events/` | `3/5` | 26 | PASS 6 / FAIL 12 / ERROR 1 / TIMEOUT 5 / SKIP 2 (90s截断) | Batch 59 (`--skip-expected-timeouts`) |
| 2026-11-22 17:45 | `/css/cssom/` | `4/5` | 24 | PASS 14 / FAIL 8 / ERROR 1 / TIMEOUT 1 (18s) | Batch 60 (`--skip-expected-timeouts`) |
| 2026-11-22 17:45 | `/css/cssom/` | `5/5` | 24 | PASS 12 / FAIL 11 / TIMEOUT 1 (11s) | Batch 60 (`--skip-expected-timeouts`) |
| 2026-11-22 17:47 | `/css/cssom-view/` | `1/5` | 22 | PASS 15 / FAIL 3 / TIMEOUT 4 (90s截断) | Batch 60 (`--skip-expected-timeouts`) |
| 2026-11-22 17:48 | `/css/cssom-view/` | `2/5` | 21 | PASS 8 / FAIL 8 / TIMEOUT 5 (83s) | Batch 60 (`--skip-expected-timeouts`) |
| 2026-11-22 17:49 | `/css/cssom-view/` | `3/5` | 21 | PASS 7 / FAIL 7 / TIMEOUT 6 / CRASH 1 (90s截断) | Batch 60 (`--skip-expected-timeouts`) |
| 2026-11-22 17:50 | `/css/css-animations/` | `4/5` | 11 | PASS 4 / FAIL 7 / TIMEOUT 0 (8s) | Batch 61 (`--skip-expected-timeouts`) |
| 2026-11-22 17:51 | `/css/css-animations/` | `5/5` | 11 | PASS 4 / FAIL 7 / TIMEOUT 0 (7s) | Batch 61 (`--skip-expected-timeouts`) |
| 2026-11-22 17:51 | `/dom/nodes/` | `4/10` | 25 | PASS 9 / FAIL 11 / ERROR 1 / TIMEOUT 4 (24s) | Batch 61 (`--skip-expected-timeouts`) |
| 2026-11-22 17:52 | `/dom/nodes/` | `6/10` | 25 | PASS 10 / FAIL 11 / ERROR 4 / TIMEOUT 0 (30s) | Batch 61 (`--skip-expected-timeouts`) |
| 2026-11-22 17:53 | `/dom/nodes/` | `7/10` | 25 | PASS 10 / FAIL 13 / ERROR 1 / TIMEOUT 1 (27s) | Batch 61 (`--skip-expected-timeouts`) |
| 2026-11-22 17:55 | `/dom/nodes/` | `8/10` | 25 | PASS 9 / FAIL 12 / TIMEOUT 4 (90s截断) | Batch 62 (`--skip-expected-timeouts`) |
| 2026-11-22 17:55 | `/dom/nodes/` | `9/10` | 25 | PASS 9 / FAIL 14 / TIMEOUT 2 (9s) | Batch 62 (`--skip-expected-timeouts`) |
| 2026-11-22 17:57 | `/dom/nodes/` | `10/10` | 25 | PASS 8 / FAIL 14 / TIMEOUT 3 (90s截断) | Batch 62 (`--skip-expected-timeouts`) |
| 2026-11-22 17:58 | `/html/semantics/forms/` | `1/10` | 29 | PASS 22 / FAIL 6 / ERROR 1 / TIMEOUT 0 (18s) | Batch 62 (`--skip-expected-timeouts`) |
| 2026-11-22 17:58 | `/html/semantics/forms/` | `2/10` | 29 | PASS 21 / FAIL 8 / TIMEOUT 0 (12s) | Batch 62 (`--skip-expected-timeouts`) |
| 2026-11-22 17:59 | `/html/semantics/forms/` | `3/10` | 29 | PASS 19 / FAIL 9 / ERROR 1 / TIMEOUT 0 | Batch 63 (`--skip-expected-timeouts`) |
| 2026-11-22 18:00 | `/html/semantics/forms/` | `6/10` | 29 | PASS 21 / FAIL 6 / TIMEOUT 2 | Batch 63 (`--skip-expected-timeouts`) |
| 2026-11-22 18:01 | `/html/semantics/forms/` | `7/10` | 29 | PASS 20 / FAIL 7 / TIMEOUT 2 | Batch 63 (`--skip-expected-timeouts`) |
| 2026-11-22 18:01 | `/html/semantics/forms/` | `8/10` | 29 | PASS 20 / FAIL 7 / ERROR 1 / TIMEOUT 1 | Batch 63 (`--skip-expected-timeouts`) |
| 2026-11-22 18:02 | `/html/semantics/forms/` | `9/10` | 29 | PASS 23 / FAIL 6 / TIMEOUT 0 | Batch 63 (`--skip-expected-timeouts`) |
| 2026-11-22 18:03 | `/html/semantics/forms/` | `10/10` | 28 | PASS 22 / FAIL 5 / TIMEOUT 1 | Batch 64 (`--skip-expected-timeouts`) |
| 2026-11-22 18:04 | `/html/dom/elements/` | `1/5` | 39 | PASS 22 / FAIL 15 / TIMEOUT 2 | Batch 64 (`--skip-expected-timeouts`) |
| 2026-11-22 18:05 | `/html/dom/elements/` | `2/5` | 39 | PASS 19 / FAIL 16 / TIMEOUT 4 | Batch 64 (`--skip-expected-timeouts`) |
| 2026-11-22 18:07 | `/html/dom/elements/` | `3/5` | 39 | PASS 19 / FAIL 14 / TIMEOUT 6 (90s截断) | Batch 64 (`--skip-expected-timeouts`) |
| 2026-11-22 18:07 | `/css/css-break/` | `1/5` | 42 | PASS 30 / FAIL 12 / TIMEOUT 0 | Batch 64 (`--skip-expected-timeouts`) |
| 2026-11-22 18:09 | `/css/css-break/` | `2/5` | 42 | PASS 32 / FAIL 10 / TIMEOUT 0 | Batch 65 (`--skip-expected-timeouts`) |
| 2026-11-22 18:10 | `/css/css-break/` | `3/5` | 42 | PASS 32 / FAIL 9 / TIMEOUT 1 (90s截断) | Batch 65 (`--skip-expected-timeouts`) |
| 2026-11-22 18:11 | `/css/css-contain/` | `1/5` | 14 | PASS 13 / FAIL 1 / TIMEOUT 0 (exit 0, 14/14匹配) | Batch 65 (`--skip-expected-timeouts`) |
| 2026-11-22 18:12 | `/css/css-contain/` | `2/5` | 14 | PASS 12 / FAIL 1 / TIMEOUT 1 (90s截断) | Batch 65 (`--skip-expected-timeouts`) |
| 2026-11-22 18:13 | `/css/css-contain/` | `3/5` | 14 | PASS 10 / FAIL 2 / ERROR 1 / TIMEOUT 1 | Batch 65 (`--skip-expected-timeouts`) |
| 2026-11-22 18:17 | `/html/semantics/embedded-content/` | `3/5`~`5/5` | 40, 40, 40 | PASS 5/40, 10/40, 6/40 (0 CRASH) | Batch 66 (`--skip-expected-timeouts`) |
| 2026-11-22 18:18 | `/fetch/api/basic/` | `2/3`, `3/3` | 10, 10 | PASS 0/10, 2/10 (0 CRASH) | Batch 66 (`--skip-expected-timeouts`) |
| 2026-11-22 18:26 | `/css/selectors/` | `3/5`~`5/5` | 13, 13, 13 | PASS 6/13, 8/13, 6/13 (0 CRASH) | Batch 67 (`--skip-expected-timeouts`) |
| 2026-11-22 18:26 | `/css/cssom-view/` | `1/5`, `2/5` | 22, 21 | PASS 15/22, 8/21 (0 CRASH) | Batch 67 (`--skip-expected-timeouts`) |
| 2026-11-22 18:35 | `/css/cssom-view/` | `3/5` | 21 | PASS 7 / FAIL 7 / TIMEOUT 6 / CRASH 1 | Batch 68 (`--skip-expected-timeouts`) |
| 2026-11-22 18:35 | `/css/css-grid/` | `3/5`~`5/5` (b1/2) | 89, 89, 89 | PASS 21/89, 18/89, 18/89 (0 CRASH) | Batch 68 (`--skip-expected-timeouts`) |
| 2026-11-22 18:35 | `/html/semantics/embedded-content/` | `1/5` | 40 | PASS 6 / FAIL 14 / ERROR 2 / TIMEOUT 18 | Batch 68 (`--skip-expected-timeouts`) |
| 2026-11-22 18:41 | `/html/semantics/embedded-content/` | `2/5` | 40 | PASS 6 / FAIL 14 / ERROR 2 / TIMEOUT 18 (31s) | Batch 69 (`--skip-expected-timeouts`) |
| 2026-11-22 18:41 | `/css/cssom-view/` | `4/5` | 21 | PASS 7 / FAIL 5 / TIMEOUT 9 (0 CRASH) | Batch 69 (`--skip-expected-timeouts`) |
| 2026-11-22 18:41 | `/css/css-box/` | `3/3` | 11 | PASS 6 / FAIL 5 / TIMEOUT 0 (8s) | Batch 69 (`--skip-expected-timeouts`) |
| 2026-11-22 18:41 | `/html/semantics/forms/` | `4/10`, `5/10` | 29, 29 | PASS 21/29, 17/29 (0 CRASH) | Batch 69 (`--skip-expected-timeouts`) |
| 2026-11-22 18:46 | `/css/cssom-view/` | `5/5` | 21 | PASS 10 / FAIL 8 / TIMEOUT 3 (0 CRASH) | Batch 70 (`--skip-expected-timeouts`) |
| 2026-11-22 18:46 | `/html/dom/elements/` | `4/5`, `5/5` | 39, 39 | PASS 23/39, 22/39 (0 CRASH) | Batch 70 (`--skip-expected-timeouts`) |
| 2026-11-22 18:46 | `/css/css-break/` | `4/5`, `5/5` | 42, 41 | PASS 28/42, 29/41 (0 CRASH) | Batch 70 (`--skip-expected-timeouts`) |
| 2026-11-22 18:50 | `/css/css-contain/` | `4/5`, `5/5` | 14, 13 | PASS 11/14, 11/13 (0 CRASH) | Batch 71 (`--skip-expected-timeouts`) |
| 2026-11-22 18:50 | `/dom/events/` | `4/5`, `5/5` | 26, 25 | PASS 6/26, 6/25 (0 CRASH) | Batch 71 (`--skip-expected-timeouts`) |
| 2026-11-22 18:50 | `/dom/nodes/` | `5/10` | 25 | PASS 12 / FAIL 8 / ERROR 2 / TIMEOUT 3 (0 CRASH) | Batch 71 (`--skip-expected-timeouts`) |
| 2026-11-22 18:59 | `/css/css-flexbox/` | `1/5`, `2/5` | 53, 52 | PASS 23/53, 23/52 (0 CRASH) | Batch 72 (`--skip-expected-timeouts`) |
| 2026-11-22 18:59 | `/css/css-position/` | `1/5`, `2/5` | 50, 54 | PASS 15/50, 14/54 (0 CRASH) | Batch 72 (`--skip-expected-timeouts`) |
| 2026-11-22 18:59 | `/css/css-values/` | `1/5` | 36/70 | PASS 3/36 (90s截断, 0 CRASH) | Batch 72 (`--skip-expected-timeouts`) |
| 2026-11-22 19:04 | `/dom/events/` | `1/5`~`3/5` | 26, 26, 26 | PASS 12/26(翻倍), 7/26, 5/26 (0 CRASH) | Batch 73 (`--skip-expected-timeouts`) |
| 2026-11-22 19:04 | `/css/css-fonts/` | `3/5` | 24 | PASS 7 / FAIL 16 / TIMEOUT 1 (0 CRASH) | Batch 73 (`--skip-expected-timeouts`) |
| 2026-11-22 19:04 | `/css/selectors/` | `1/5` | 14 | PASS 6 / FAIL 7 / ERROR 1 / TIMEOUT 0 | Batch 73 (`--skip-expected-timeouts`) |
| 2026-11-22 19:11 | `/dom/nodes/` | `1/10`~`3/10` | 26, 25, 25 | PASS 8/26, 8/25, 9/25 (0 CRASH) | Batch 74 (`--skip-expected-timeouts`) |
| 2026-11-22 19:11 | `/css/selectors/` | `2/5` | 14 | PASS 5/14 (?PASS 2, 0 CRASH) | Batch 74 (`--skip-expected-timeouts`) |
| 2026-11-22 19:11 | `/css/css-fonts/` | `4/5` | 24 | PASS 4/24 (0 CRASH) | Batch 74 (`--skip-expected-timeouts`) |
| 2026-11-22 19:16 | `/dom/events/` | `4/5`, `5/5` | 26, 25 | PASS 10/26, 9/25 (Phase 296提升, 0 CRASH) | Batch 75 (`--skip-expected-timeouts`) |
| 2026-11-22 19:16 | `/css/css-fonts/` | `5/5`, `1/5`, `2/5` | 24, 25, 24 | PASS 5/24, 7/25, 6/24 (0 CRASH) | Batch 75 (`--skip-expected-timeouts`) |
| 2026-11-22 19:23 | `/dom/nodes/` | `4/10`, `6/10`~`9/10` | 25x5 | PASS 9, 10, 10, 9, 9 (0 CRASH) | Batch 76 (`--skip-expected-timeouts`) |
| 2026-11-22 19:27 | `/dom/nodes/` | `10/10` | 25 | PASS 8 / FAIL 13 / TIMEOUT 4 (`Node-isConnected` ?PASS) | Batch 77 (`--skip-expected-timeouts`) |
| 2026-11-22 19:27 | `/dom/collections/` | `1/2`, `2/2` | 6, 5 | PASS 3/6, 3/5 (0 CRASH) | Batch 77 (`--skip-expected-timeouts`) |
| 2026-11-22 19:27 | `/dom/traversal/` | `1/2`, `2/2` | 9, 8 | PASS 3/9, 2/8 (0 CRASH) | Batch 77 (`--skip-expected-timeouts`) |
| 2026-11-22 19:29 | `/css/css-overflow/` | `1/5`~`5/5` | 33 | PASS 6 / FAIL 27 (?PASS 4, 0 CRASH) | Batch 78 (`--skip-expected-timeouts`) |
| 2026-11-22 19:32 | `/css/css-tables/` | `1/5`~`5/5` | 18 | PASS 4 / FAIL 13 / TIMEOUT 1 (0 CRASH) | Batch 79 (`--skip-expected-timeouts`) |
| 2026-11-22 19:34 | `/css/css-transforms/` | `1/5`~`5/5` | 31 | PASS 2 / FAIL 29 / TIMEOUT 0 (0 CRASH) | Batch 80 (`--skip-expected-timeouts`) |
| 2026-11-22 19:41 | `/css/css-color/` | `1/5`~`5/5` | 56 | PASS 2 / FAIL 41 / TIMEOUT 13 (0 CRASH) | Batch 81 (`--skip-expected-timeouts`) |
| 2026-11-22 19:47 | `/css/css-display/` | `1/5`~`5/5` | 52 | PASS 15 / FAIL 34 / TIMEOUT 3 (?PASS 2, 0 CRASH) | Batch 82 (`--skip-expected-timeouts`) |
| 2026-11-22 19:53 | `/css/css-cascade/` | `1/3`~`3/3` | 90 | PASS 36 / FAIL 48 / ERROR 1 / TIMEOUT 5 (?PASS 1, 0 CRASH) | Batch 83 (`--skip-expected-timeouts`) |
| 2026-11-22 19:53 | `/css/css-box/` | `1/3`, `2/3` | 22 | PASS 7 / FAIL 15 (0 CRASH) | Batch 83 (`--skip-expected-timeouts`) |

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
| 2026-11-22 15:29 | `page-set-content.spec.ts` | 13/14 PASS (Phase 265复测全[1..7,9..14]通过) | PW Batch 17 复测 |
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
| 2026-11-22 15:54 | `page-event-console.spec.ts` | 2/3 PASS (emit same log twice已修复, [4/19]卡住90s) | PW Batch 17 复测 |
| 2026-11-22 15:54 | `page-network-request.spec.ts` | 2/21 PASS (90s截断) | PW Batch 12 |
| 2026-11-22 15:57 | `locator-convenience.spec.ts` | 21/21 PASS (20.7s, exit 0) | PW Batch 13 (Phase 266 全绿) |
| 2026-11-22 15:57 | `page-click-timeout-2.spec.ts` | 2/2 PASS (13.9s, exit 0) | PW Batch 13 |
| 2026-11-22 15:58 | `page-click-timeout-3.spec.ts` | 3/4 PASS (waiting 500ms 1 FAIL, 27.6s) | PW Batch 13 |
| 2026-11-22 16:00 | `page-network-response.spec.ts` | 0/10 PASS (`page.goto` null 10 FAIL, 90s截断) | PW Batch 13 |
| 2026-11-22 16:02 | `page-click-react.spec.ts` | 0/3 PASS (1 FAIL + 2 TIMEOUT) | PW Batch 14 |
| 2026-11-22 16:03 | `page-emulate-media.spec.ts` | 3/12 PASS (emulateMedia/matchMedia 9 FAIL) | PW Batch 14 |
| 2026-11-22 16:05 | `elementhandle-bounding-box.spec.ts` | 4/7 PASS (Phase 270 scroll offset修复通过) | PW Batch 18 复测 |
| 2026-11-22 16:06 | `elementhandle-misc.spec.ts` | 5/7 PASS (check全通, 90s截断) | PW Batch 14 |
| 2026-11-22 16:09 | `jshandle-to-string.spec.ts` | 3/6 PASS (sparse/Symbol/RegExp 3 FAIL, 20.9s) | PW Batch 15 |
| 2026-11-22 16:10 | `jshandle-evaluate.spec.ts` | 2/2 PASS (2.1s, exit 0) | PW Batch 15 |
| 2026-11-22 16:10 | `locator-element-handle.spec.ts` | 5/5 PASS (6.2s, exit 0) | PW Batch 15 |
| 2026-11-22 16:11 | `page-add-style-tag.spec.ts` | 1/4 PASS (addStyleTag url 3 FAIL, 21.9s) | PW Batch 15 |
| 2026-11-22 16:12 | `elementhandle-select-text.spec.ts` | 2/5 PASS (getSelection 3 FAIL) | PW Batch 16 |
| 2026-11-22 16:14 | `page-aria-snapshot.spec.ts` | 17/20 PASS (details/pseudo 3 FAIL, 90s截断) | PW Batch 16 |
| 2026-11-22 16:15 | `page-set-input-files.spec.ts` | 9/14 PASS (Phase 272生效, @smoke全通, 90s截断) | PW Batch 18 复测 |
| 2026-11-22 16:16 | `frame-evaluate.spec.ts` | 2/8 PASS (0 deviations, 符合预期) | PW Batch 16 |
| 2026-11-22 16:21 | `eval-on-selector.spec.ts` | 32/32 PASS (35.4s, exit 0) | PW Batch 17 |
| 2026-11-22 16:27 | `locator-evaluate.spec.ts` | 5/5 PASS (5.7s, exit 0) | PW Batch 18 |
| 2026-11-22 16:27 | `locator-list.spec.ts` | 1/1 PASS (2.0s, exit 0) | PW Batch 18 |
| 2026-11-22 16:30 | `elementhandle-click.spec.ts` | 4/8 PASS (Node/ShadowDOM/br/dblclick 4 FAIL, 90s截断) | PW Batch 19 |
| 2026-11-22 16:31 | `elementhandle-scroll-into-view.spec.ts` | 2/8 PASS (6 FAIL, 90s截断) | PW Batch 19 |
| 2026-11-22 16:33 | `frame-hierarchy.spec.ts` | 4/12 PASS (nested/shadow/frameset 8 FAIL, 90s截断) | PW Batch 19 |
| 2026-11-22 16:36 | `page-event-pageerror.spec.ts` | 0/2 TIMEOUT (未触发pageerror, 90s截断) | PW Batch 20 |
| 2026-11-22 16:37 | `page-wait-for-url.spec.ts` | 1/3 PASS (waitForURL load未触发 2 TIMEOUT, 90s截断) | PW Batch 20 |
| 2026-11-22 16:38 | `elementhandle-select-text.spec.ts` | 5/5 PASS (12.2s, exit 0) | PW Batch 20 (Phase 273 全绿) |
| 2026-11-22 16:38 | `locator-click.spec.ts` | 4/5 PASS (Node removed 1 FAIL, 14.1s) | PW Batch 20 (Phase 266+274 复测) |
| 2026-11-22 16:44 | `elementhandle-scroll-into-view.spec.ts` | 5/8 PASS (Phase 275 复测提升 2->5, 90s截断) | PW Batch 21 |
| 2026-11-22 16:44 | `expect-boolean.spec.ts` | 52/54 PASS (empty span/iframe 2 FAIL, 90s截断) | PW Batch 21 |
| 2026-11-22 16:45 | `expect-to-have-text.spec.ts` | 25/28 PASS (hidden innerText/shadow 3 FAIL, 90s截断) | PW Batch 21 |
| 2026-11-22 16:45 | `locator-wait-for-function.spec.ts` | 10/10 PASS (11.1s, exit 0) | PW Batch 21 (全绿) |
| 2026-11-22 16:46 | `expect-to-have-value.spec.ts` | 11/11 PASS (15.7s, exit 0) | PW Batch 22 (全绿) |
| 2026-11-22 16:46 | `expect-to-have-accessible.spec.ts` | 14/14 PASS (16.1s, 含意外PASS 1项) | PW Batch 22 (全绿) |
| 2026-11-22 16:47 | `expect-timeout.spec.ts` | 15/15 PASS (19.5s, exit 0) | PW Batch 22 (全绿) |
| 2026-11-22 16:47 | `frame-frame-element.spec.ts` | 1/6 PASS (DOM.getFrameOwner/shadow 5 FAIL, 36.7s) | PW Batch 22 |
| 2026-11-22 16:51 | `page-event-pageerror.spec.ts` | 2/7 PASS (0 TIMEOUT, Phase 276 复测, 尾部换行/16上限) | PW Batch 23 |
| 2026-11-22 16:51 | `expect-builtins.spec.ts` | 278/278 PASS (6.7s, exit 0) | PW Batch 23 (全绿) |
| 2026-11-22 16:51 | `expect-misc.spec.ts` | 39/41 PASS (toHaveTitle 2 FAIL, `[42/59]` 90s截断) | PW Batch 23 |
| 2026-11-22 16:51 | `expect-matcher-result.spec.ts` | 4/4 PASS (5.5s, exit 0) | PW Batch 23 (全绿) |
| 2026-11-22 16:55 | `expect-to-have-text.spec.ts` | 26/28 PASS (Phase 277 复测 `useInnerText` 已通, 2 FAIL) | PW Batch 24 |
| 2026-11-22 16:56 | `page-add-init-script-callback.spec.ts` | 3/7 PASS (exposeFunctions 4 FAIL, `[8/11]` 90s截断) | PW Batch 24 |
| 2026-11-22 16:56 | `matchers.misc.spec.ts` | 1/2 PASS (outlive navigation 1 FAIL, 重试90s截断) | PW Batch 24 |
| 2026-11-22 16:56 | `interception.spec.ts` | 3/8 PASS (glob/regexp/cache 5 FAIL, 34.1s) | PW Batch 24 |
| 2026-11-22 17:01 | `page-event-pageerror.spec.ts` | 3/7 PASS (Phase 278 复测 `pageErrors` 已通, stack空) | PW Batch 25 |
| 2026-11-22 17:01 | `expect-misc.spec.ts` | 50/51 PASS (Phase 278 复测 `toHaveTitle` 已通, `[52/59]` 90s截断) | PW Batch 25 |
| 2026-11-22 17:01 | `elementhandle-screenshot.spec.ts` | 2/5 PASS (`[6/11]` 90s截断) | PW Batch 25 |
| 2026-11-22 17:01 | `expect-with-snapshot.spec.ts` | 14/14 PASS (11.2s, exit 0) | PW Batch 25 (全绿) |
| 2026-11-22 17:04 | `page-event-pageerror.spec.ts` | 4/7 PASS (Phase 279 复测 `sourceURL` 已通, 3 FAIL) | PW Batch 26 |
| 2026-11-22 17:05 | `page-dialog.spec.ts` | 7/9 PASS (`accept` 回传值 2 FAIL, 含3项意外PASS) | PW Batch 26 |
| 2026-11-22 17:05 | `page-dispatchevent.spec.ts` | 15/16 PASS (`should be atomic` 1 FAIL) | PW Batch 26 |
| 2026-11-22 17:06 | `page-autowaiting-basic.spec.ts` | 4/6 PASS (`[7/12]` 90s截断) | PW Batch 26 |
| 2026-11-22 17:07 | `page-click-during-navigation.spec.ts` | 1/1 PASS (3.7s, exit 0) | PW Batch 27 (全绿) |
| 2026-11-22 17:08 | `page-click-scroll.spec.ts` | 4/8 PASS (2 SKIP, intercepts/viewport 4 FAIL, 47.6s) | PW Batch 27 |
| 2026-11-22 17:09 | `page-click-timeout-4.spec.ts` | 1/3 PASS (stable/fixed-intercept 2 FAIL, 34.1s) | PW Batch 27 |
| 2026-11-22 17:11 | `page-click.spec.ts` | 6/9 PASS (frameset/Node/half-px 3 FAIL, `[10/77]` 90s截断) | PW Batch 27 |
| 2026-11-22 17:15 | `page-drag.spec.ts` | 0/15 SKIP (全部跳过) | PW Batch 28 |
| 2026-11-22 17:16 | `page-drop.spec.ts` | 2/7 PASS (dragover preventDefault 5 FAIL, 40.2s) | PW Batch 28 |
| 2026-11-22 17:16 | `page-add-locator-handler.spec.ts` | 2/6 PASS (`[7/13]` 90s截断) | PW Batch 28 |
| 2026-11-22 17:16 | `page-aria-snapshot-ai.spec.ts` | 2/4 PASS (iframe递归拼接 `[5/36]` 90s截断) | PW Batch 28 |
| 2026-11-22 17:19 | `page-autowaiting-no-hang.spec.ts` | 2/6 PASS (window.stop/open 4 FAIL, `[7/9]` 90s截断) | PW Batch 29 |
| 2026-11-22 17:20 | `page-evaluate-callback.spec.ts` | 1/1 PASS (`[2/22]` exposeFunctions 挂起 90s截断) | PW Batch 29 |
| 2026-11-22 17:21 | `page-evaluate-handle.spec.ts` | 10/11 PASS (userAgent 1 FAIL, 含1项意外PASS, 13.5s) | PW Batch 29 |
| 2026-11-22 17:21 | `page-evaluate-no-stall.spec.ts` | 1/3 PASS (pending nav/no ctx 2 FAIL, 14.4s) | PW Batch 29 |
| 2026-11-22 17:23 | `page-event-load.spec.ts` | 1/1 PASS (2.8s, exit 0) | PW Batch 30 (全绿) |
| 2026-11-22 17:24 | `page-event-network.spec.ts` | 2/6 PASS (goto null/requestfailed 4 FAIL, 27.5s) | PW Batch 30 |
| 2026-11-22 17:25 | `page-event-request.spec.ts` | 2/9 PASS (`[10/13]` 90s截断, fetch/goto/SW) | PW Batch 30 |
| 2026-11-22 17:26 | `page-filechooser.spec.ts` | 4/6 PASS (iframe/upload 2 TIMEOUT, 重试90s截断) | PW Batch 30 |
| 2026-11-22 17:28 | `page-listeners.spec.ts` | 3/3 PASS (3.8s, exit 0) | PW Batch 31 (全绿) |
| 2026-11-22 17:28 | `page-localstorage.spec.ts` | 8/9 PASS (cross-origin scope 1 FAIL, 15.1s, exit 0) | PW Batch 31 |
| 2026-11-22 17:29 | `page-navigation.spec.ts` | 2/3 PASS (cross-origin redirect load 1 TIMEOUT, 39.3s) | PW Batch 31 |
| 2026-11-22 17:31 | `page-network-idle.spec.ts` | 0/3 (`[4/10]` 90s截断, goto null/request) | PW Batch 31 |
| 2026-11-22 17:35 | `locator-click.spec.ts` | 4/5 PASS (`objectId`缺`executionContextId`, Node 1 FAIL) | PW Batch 32 |
| 2026-11-22 17:36 | `expect-misc.spec.ts` | 55/55 PASS (Phase 280 复测 `#FF00FF` 已通, `[56/59]` 90s截断) | PW Batch 32 |
| 2026-11-22 17:36 | `page-network-sizes.spec.ts` | 0/2 TIMEOUT (`[3/13]` 90s截断, request事件未触发) | PW Batch 32 |
| 2026-11-22 17:37 | `page-request-continue.spec.ts` | 0/4 FAIL/TIMEOUT (`[5/36]` 90s截断, ERR_BLOCKED_BY_CLIENT) | PW Batch 32 |
| 2026-11-22 17:39 | `page-drop.spec.ts` | 2/7 PASS (Phase 282 复测, dragover 5 FAIL, 40s) | PW Batch 33 |
| 2026-11-22 17:41 | `page-autowaiting-no-hang.spec.ts` | 6/9 PASS (Phase 282 复测大幅提升 2->6, window.stop已通) | PW Batch 33 |
| 2026-11-22 17:42 | `page-request-fallback.spec.ts` | 1/9 PASS (`[10/14]` 90s截断, ERR_BLOCKED_BY_CLIENT) | PW Batch 33 |
| 2026-11-22 17:43 | `page-request-fulfill.spec.ts` | 0/4 FAIL (`[5/20]` 90s截断, ERR_BLOCKED_BY_CLIENT) | PW Batch 33 |
| 2026-11-22 17:45 | `locator-click.spec.ts` | 4/5 PASS (Phase 283 复测, Node removed 1 FAIL) | PW Batch 34 |
| 2026-11-22 17:46 | `page-strict.spec.ts` | 9/10 PASS (colon tag escape 1 FAIL) | PW Batch 34 |
| 2026-11-22 17:47 | `page-wait-for-load-state.spec.ts` | 6/10 PASS (open/frame/timeout 4 FAIL, 重试90s截断) | PW Batch 34 |
| 2026-11-22 17:48 | `page-wait-for-navigation.spec.ts` | 1/2 PASS (`[3/12]` 90s截断, load未触发) | PW Batch 34 |
| 2026-11-22 17:50 | `page-localstorage.spec.ts` | 9/9 PASS (Phase 284 复测全绿) | PW Batch 35 (全绿) |
| 2026-11-22 17:51 | `page-wait-for-request.spec.ts` | 3/6 PASS (`[6/8]` 90s截断, fetch request未触发) | PW Batch 35 |
| 2026-11-22 17:52 | `page-wait-for-response.spec.ts` | 3/6 PASS (`[6/8]` 90s截断, fetch response未触发) | PW Batch 35 |
| 2026-11-22 17:53 | `page-set-extra-http-headers.spec.ts` | 1/5 PASS (header token/error 4 FAIL) | PW Batch 35 |
| 2026-11-22 17:55 | `page-strict.spec.ts` | 10/10 PASS (Phase 285 复测全绿, 8.4s, exit 0) | PW Batch 36 (全绿) |
| 2026-11-22 17:56 | `queryselector.spec.ts` | 17/17 PASS (18.0s, exit 0) | PW Batch 36 (全绿) |
| 2026-11-22 17:57 | `retarget.spec.ts` | 10/13 PASS (`[14/15]` 90s截断) | PW Batch 36 |
| 2026-11-22 17:58 | `selectors-css.spec.ts` | 17/25 PASS (1 SKIP, 8 FAIL, 重试90s截断) | PW Batch 36 |
| 2026-11-22 18:03 | `page-set-extra-http-headers.spec.ts` | 4/5 PASS (Phase 286 复测大幅提升 1->4, 9.5s) | PW Batch 37 |
| 2026-11-22 18:04 | `selectors-frame.spec.ts` | 4/13 PASS (`[14/15]` 90s截断, ERR_BLOCKED_BY_CLIENT) | PW Batch 37 |
| 2026-11-22 18:04 | `page-route.spec.ts` | 0/5 FAIL (`[6/51]` 90s截断, ERR_BLOCKED_BY_CLIENT) | PW Batch 37 |
| 2026-11-22 18:04 | `page-request-intercept.spec.ts` | 1/6 PASS (`[7/14]` 90s截断, ERR_BLOCKED_BY_CLIENT) | PW Batch 37 |
| 2026-11-22 18:06 | `selectors-css.spec.ts` | 18/25 PASS (Phase 287 复测 `[attr*="..."i]` 已通, 1 SKIP) | PW Batch 38 |
| 2026-11-22 18:07 | `selectors-get-by.spec.ts` | 23/25 PASS (getByLabel 2 FAIL, 39.9s) | PW Batch 38 |
| 2026-11-22 18:09 | `selectors-misc.spec.ts` | 7/9 PASS (`[10/19]` 90s截断, shadow click/above) | PW Batch 38 |
| 2026-11-22 18:09 | `selectors-register.spec.ts` | 6/6 PASS (6.9s, exit 0) | PW Batch 38 (全绿) |
| 2026-11-22 18:16 | `retarget.spec.ts` | 13/15 PASS (shadow label/selectOption 2 FAIL, 重试90s截断) | PW Batch 39 |
| 2026-11-22 18:16 | `selectors-role.spec.ts` | 7/9 PASS (disabled/fieldset 2 FAIL, `[10/15]` 90s截断) | PW Batch 39 |
| 2026-11-22 18:16 | `selectors-text.spec.ts` | 20/21 PASS (`text=lowo` 跨节点 1 FAIL, 52.3s) | PW Batch 39 |
| 2026-11-22 18:16 | `to-match-aria-snapshot.spec.ts` | 31/32 PASS (`details visibility` 1 FAIL, `[33/36]` 90s截断) | PW Batch 39 |
| 2026-11-22 18:25 | `selectors-get-by.spec.ts` | 25/25 PASS (Phase 289 复测全绿, 29.4s, exit 0) | PW Batch 40 (全绿) |
| 2026-11-22 18:25 | `wheel.spec.ts` | 5/6 PASS (scroll when nobody listening 1 TIMEOUT, 49.9s) | PW Batch 40 |
| 2026-11-22 18:25 | `workers.spec.ts` | 0/3 TIMEOUT (worker network 3 TIMEOUT, 90s截断) | PW Batch 40 |
| 2026-11-22 18:25 | `locator-is-visible.spec.ts` | 8/8 PASS (Phase 290 复测全绿, 10.1s, exit 0) | PW Batch 40 (全绿) |
| 2026-11-22 18:31 | `to-match-aria-snapshot.spec.ts` | 35/36 PASS (details visibility 1 FAIL, 0 TIMEOUT) | PW Batch 41 |
| 2026-11-22 18:31 | `page-aria-snapshot.spec.ts` | 25/28 PASS (details/pseudo 3 FAIL, `[29/38]` 90s截断) | PW Batch 41 |
| 2026-11-22 18:31 | `expect-to-have-text.spec.ts` | 26/28 PASS (shadow/not-found 2 FAIL, 56.5s) | PW Batch 41 |
| 2026-11-22 18:31 | `retarget.spec.ts` | 13/15 PASS (shadow label/selectOption 2 FAIL, 1.4m) | PW Batch 41 |
| 2026-11-22 18:36 | `selectors-role.spec.ts` | 13/15 PASS (Phase 291 复测大涨 7->13, 0 TIMEOUT, 1.2m) | PW Batch 42 |
| 2026-11-22 18:36 | `selectors-text.spec.ts` | 20/21 PASS (`text=lowo` 跨节点 1 FAIL, 52.7s) | PW Batch 42 |
| 2026-11-22 18:36 | `selectors-misc.spec.ts` | 7/9 PASS (shadow link/above 2 TIMEOUT, 90s截断) | PW Batch 42 |
| 2026-11-22 18:36 | `page-aria-snapshot-ai.spec.ts` | 14/36 PASS (第14条后服务退出 ECONNREFUSED, 39.9s) | PW Batch 42 更正 |
| 2026-11-22 18:41 | `expect-boolean.spec.ts` | 60/61 PASS (`[62/84]` 90s截断) | PW Batch 43 |
| 2026-11-22 18:45 | `page-dialog.spec.ts` | 7/9 PASS (prompt/confirm 2 FAIL, 18.8s) | PW Batch 44 |
| 2026-11-22 18:45 | `page-dispatchevent.spec.ts` | 15/16 PASS (`should be atomic` 1 FAIL, 30.1s) | PW Batch 44 |
| 2026-11-22 18:45 | `page-evaluate-handle.spec.ts` | 10/11 PASS (userAgent 1 FAIL, 13.0s) | PW Batch 44 |
| 2026-11-22 18:45 | `retarget.spec.ts` | 14/15 PASS (Phase 292 复测提升 13->14, 1.4m) | PW Batch 44 |
| 2026-11-22 18:51 | `page-aria-snapshot.spec.ts` | 25/28 PASS (`[29/38]` 90s截断) | PW Batch 45 |
| 2026-11-22 18:51 | `selectors-role.spec.ts` | 13/15 PASS (Phase 294前旧二进制, 1.2m) | PW Batch 45 |
| 2026-11-22 18:51 | `to-match-aria-snapshot.spec.ts` | 35/36 PASS (1.4m, exit 0) | PW Batch 45 |
| 2026-11-22 18:51 | `page-autowaiting-no-hang.spec.ts` | 5/8 PASS (`[9/9]` 90s截断) | PW Batch 45 |
| 2026-11-22 18:57 | `selectors-role.spec.ts` | 12/15 PASS (disabled 1 TIMEOUT, shadow 2 FAIL, 1.3m) | PW Batch 46 |
| 2026-11-22 18:57 | `expect-to-have-text.spec.ts` | 26/28 PASS (shadow/not-found 2 FAIL, 56.4s) | PW Batch 46 |
| 2026-11-22 18:57 | `selectors-misc.spec.ts` | 7/9 PASS (shadow link/above 2 TIMEOUT, 90s截断) | PW Batch 46 |
| 2026-11-22 18:57 | `page-dispatchevent.spec.ts` | 15/16 PASS (`should be atomic` 1 FAIL, 30.4s) | PW Batch 46 |
| 2026-11-22 19:02 | `locator-misc-1.spec.ts` | 4/8 PASS (Node removed/fill/select 4 FAIL, `[9/13]` 90s截断) | PW Batch 47 |
| 2026-11-22 19:02 | `locator-misc-2.spec.ts` | 4/6 PASS (`[7/14]` 90s截断) | PW Batch 47 |
| 2026-11-22 19:02 | `page-basic.spec.ts` | 12/16 PASS (frame/UA/webdriver 4 FAIL, 36.1s) | PW Batch 47 |
| 2026-11-22 19:02 | `locator-frame.spec.ts` | 6/9 PASS (ERR_BLOCKED_BY_CLIENT 3 FAIL, 26.3s) | PW Batch 47 |
| 2026-11-22 19:06 | `frame-hierarchy.spec.ts` | 4/12 PASS (nested/frameset/shadow 8 FAIL, 59.2s) | PW Batch 48 |
| 2026-11-22 19:06 | `frame-frame-element.spec.ts` | 1/6 PASS (detached/shadow 5 FAIL, 36.9s) | PW Batch 48 |
| 2026-11-22 19:06 | `page-add-style-tag.spec.ts` | 2/4 PASS (url error/CSP 2 FAIL, 15.3s) | PW Batch 48 |
| 2026-11-22 19:06 | `page-wait-for-load-state.spec.ts` | 6/10 PASS (timeout/popup/frame 4 FAIL/TIMEOUT, 1.1m) | PW Batch 48 |
| 2026-11-22 19:10 | `wheel.spec.ts` | 5/6 PASS (1 TIMEOUT, 51.3s) | PW Batch 49 |
| 2026-11-22 19:10 | `selectors-css.spec.ts` | 18/25 PASS (1 SKIP, 1.4m) | PW Batch 49 |
| 2026-11-22 19:10 | `page-click-timeout-3.spec.ts` | 4/4 PASS (全绿, 21.6s, exit 0) | PW Batch 49 (全绿) |
| 2026-11-22 19:10 | `page-click-timeout-4.spec.ts` | 1/3 PASS (stable/intercepts 2 FAIL, 34.8s) | PW Batch 49 |
| 2026-11-22 19:17 | `frame-hierarchy.spec.ts` | 4/12 PASS (`./frame.html` /./已消除, 59.2s) | PW Batch 50 |
| 2026-11-22 19:17 | `page-click-scroll.spec.ts` | 4/8 PASS (2 SKIP, 48.0s) | PW Batch 50 |
| 2026-11-22 19:17 | `page-click.spec.ts` | 4/7 PASS (`[8/77]` 90s截断) | PW Batch 50 |
| 2026-11-22 19:17 | `page-autowaiting-basic.spec.ts` | 4/6 PASS (`[7/12]` 90s截断) | PW Batch 50 |
| 2026-11-22 19:20 | `selectors-css.spec.ts` | 1/25 PASS (第3条后ECONNREFUSED, 35.9s) | PW Batch 51 |
| 2026-11-22 19:20 | `locator-click.spec.ts` | 4/5 PASS (Node removed 1 FAIL, 14.8s) | PW Batch 51 |
| 2026-11-22 19:20 | `elementhandle-click.spec.ts` | 5/8 PASS (Node/Shadow/<br> 3 FAIL/TIMEOUT, 1.0m) | PW Batch 51 |
| 2026-11-22 19:20 | `elementhandle-bounding-box.spec.ts` | 4/7 PASS (nth-of-type/SVG/inline 3 FAIL/TIMEOUT, 1.2m) | PW Batch 51 |
| 2026-11-22 19:23 | `selectors-css.spec.ts` | 20/25 PASS (Phase 300 复测提升, 1 SKIP, 1.2m) | PW Batch 52 |
| 2026-11-22 19:23 | `selectors-get-by.spec.ts` | 25/25 PASS (29.2s, exit 0) | PW Batch 52 (全绿) |
| 2026-11-22 19:23 | `locator-convenience.spec.ts` | 21/21 PASS (21.0s, exit 0) | PW Batch 52 (全绿) |
| 2026-11-22 19:23 | `page-autowaiting-no-hang.spec.ts` | 1/9 PASS (第3条后ECONNREFUSED, 1.1m) | PW Batch 52 更正 |
| 2026-11-22 19:29 | `locator-element-handle.spec.ts` | 5/5 PASS (5.9s, exit 0) | PW Batch 53 (全绿) |
| 2026-11-22 19:29 | `eval-on-selector.spec.ts` | 32/32 PASS (35.9s, exit 0) | PW Batch 53 (全绿) |
| 2026-11-22 19:34 | `locator-click.spec.ts` | 4/5 PASS (Node removed 1 FAIL, 14.4s) | PW Batch 54 |
| 2026-11-22 19:34 | `elementhandle-click.spec.ts` | 5/8 PASS (Node/Shadow/<br> 3 FAIL/TIMEOUT, 59.9s) | PW Batch 54 |
| 2026-11-22 19:34 | `locator-misc-1.spec.ts` | 4/8 PASS (`[9/13]` 90s截断) | PW Batch 54 |
| 2026-11-22 19:34 | `page-click.spec.ts` | 6/9 PASS (`[10/77]` 90s截断) | PW Batch 54 |
| 2026-11-22 19:39 | `selectors-css.spec.ts` | 20/25 PASS (1 SKIP, 1.2m) | PW Batch 55 |
| 2026-11-22 19:39 | `selectors-role.spec.ts` | 12/15 PASS (1.3m) | PW Batch 55 |
| 2026-11-22 19:39 | `page-click-timeout-4.spec.ts` | 1/3 PASS (34.8s) | PW Batch 55 |
| 2026-11-22 19:39 | `retarget.spec.ts` | 14/15 PASS (1.4m) | PW Batch 55 |
| 2026-11-22 19:45 | `locator-misc-1.spec.ts` | 6/8 PASS (Phase 303 复测 fill/clear 全通, `[9/13]` 90s截断) | PW Batch 56 |
| 2026-11-22 19:45 | `selectors-css.spec.ts` | 20/25 PASS (1 SKIP, 1.2m) | PW Batch 56 |
| 2026-11-22 19:45 | `elementhandle-misc.spec.ts` | 5/7 PASS (`[8/10]` 90s截断) | PW Batch 56 |
| 2026-11-22 19:45 | `page-click-timeout-1.spec.ts` | 2/2 PASS (5.5s, exit 0) | PW Batch 56 (全绿) |
| 2026-11-22 19:47 | `page-click-timeout-2.spec.ts` | 2/2 PASS (13.9s, exit 0) | PW Batch 57 (全绿) |
| 2026-11-22 19:47 | `page-event-load.spec.ts` | 1/1 PASS (2.7s, exit 0) | PW Batch 57 (全绿) |
| 2026-11-22 19:47 | `page-listeners.spec.ts` | 3/3 PASS (3.6s, exit 0) | PW Batch 57 (全绿) |
| 2026-11-22 19:47 | `page-event-pageerror.spec.ts` | 4/7 PASS (stack/clear/illegal 3 FAIL, 25.1s) | PW Batch 57 |
| 2026-11-22 19:50 | `expect-to-have-value.spec.ts` | 11/11 PASS (15.7s, exit 0) | PW Batch 58 (全绿) |
| 2026-11-22 19:50 | `expect-to-have-accessible.spec.ts` | 14/14 PASS (16.2s, exit 0) | PW Batch 58 (全绿) |
| 2026-11-22 19:50 | `expect-timeout.spec.ts` | 15/15 PASS (19.7s, exit 0) | PW Batch 58 (全绿) |
| 2026-11-22 19:50 | `expect-with-snapshot.spec.ts` | 14/14 PASS (22.9s, exit 0) | PW Batch 58 (全绿) |
| 2026-11-22 19:51 | `expect-builtins.spec.ts` | 278/278 PASS (6.7s, exit 0) | PW Batch 59 (全绿) |
| 2026-11-22 19:51 | `expect-matcher-result.spec.ts` | 4/4 PASS (5.2s, exit 0) | PW Batch 59 (全绿) |
| 2026-11-22 19:51 | `locator-wait-for-function.spec.ts` | 10/10 PASS (11.4s, exit 0) | PW Batch 59 (全绿) |
| 2026-11-22 19:51 | `queryselector.spec.ts` | 17/17 PASS (18.6s, exit 0) | PW Batch 59 (全绿) |
| 2026-11-22 19:53 | `page-strict.spec.ts` | 10/10 PASS (8.6s, exit 0) | PW Batch 60 (全绿) |
| 2026-11-22 19:53 | `page-localstorage.spec.ts` | 9/9 PASS (10.4s, exit 0) | PW Batch 60 (全绿) |
| 2026-11-22 19:53 | `selectors-register.spec.ts` | 6/6 PASS (5.9s, exit 0) | PW Batch 60 (全绿) |
| 2026-11-22 19:53 | `elementhandle-select-text.spec.ts` | 5/5 PASS (11.3s, exit 0) | PW Batch 60 (全绿) |
| 2026-11-22 19:55 | `page-click-during-navigation.spec.ts` | 1/1 PASS (3.6s, exit 0) | PW Batch 61 (全绿) |
| 2026-11-22 19:55 | `page-set-extra-http-headers.spec.ts` | 4/5 PASS (9.5s) | PW Batch 61 |
| 2026-11-22 19:55 | `page-dialog.spec.ts` | 7/9 PASS (18.5s) | PW Batch 61 |
| 2026-11-22 19:55 | `page-dispatchevent.spec.ts` | 15/16 PASS (26.5s) | PW Batch 61 |
| 2026-11-22 20:00 | `expect-misc.spec.ts` | 57/57 PASS (`[57/59]` 90s截断) | PW Batch 62 |
| 2026-11-22 20:00 | `page-evaluate-handle.spec.ts` | 10/11 PASS (12.9s) | PW Batch 62 |
| 2026-11-22 20:00 | `selectors-text.spec.ts` | 20/21 PASS (`text=lowo` outerHTML 1 FAIL, 51.6s) | PW Batch 62 |
| 2026-11-22 20:00 | `selectors-get-by.spec.ts` | 25/25 PASS (28.4s, exit 0) | PW Batch 62 (全绿) |
| 2026-11-22 20:01 | `/dom/ranges/` (`1/3..3/3`) | 3/12, 3/10, 1/10 PASS (25.1s) | WPT Batch 84 (`bin_dev`) |
| 2026-11-22 20:01 | `/dom/traversal/` (`1/2, 2/2`) | 3/9, 1/8 PASS (15.7s) | WPT Batch 84 (`bin_dev`) |
| 2026-11-22 20:03 | `page-navigation.spec.ts` | 2/3 PASS (38.3s, 1 timeout) | PW Batch 63 |
| 2026-11-22 20:03 | `page-wait-for-url.spec.ts` | 1/9 PASS (90s截断, 2 timeout) | PW Batch 63 |
| 2026-11-22 20:03 | `jshandle-evaluate.spec.ts` | 2/2 PASS (2.5s, exit 0) | PW Batch 63 (全绿) |
| 2026-11-22 20:03 | `jshandle-to-string.spec.ts` | 3/6 PASS (22.1s, match exp) | PW Batch 63 |
| 2026-11-22 20:10 | `selectors-text.spec.ts` | 20/21 PASS (`&nbsp;` 1 FAIL) | PW Batch 64 |
| 2026-11-22 20:10 | `page-wait-for-function.spec.ts` | 9/26 PASS (90s截断, 2 FAIL) | PW Batch 64 |
| 2026-11-22 20:10 | `page-wait-for-selector-1.spec.ts` | 13/15 PASS (90s retry截断) | PW Batch 64 |
| 2026-11-22 20:10 | `page-wait-for-selector-2.spec.ts` | 22/32 PASS (90s截断) | PW Batch 64 |
| 2026-11-22 20:10 | `jshandle-properties.spec.ts` | 7/7 PASS (exit 1 exp dev) | PW Batch 64 (全绿) |
| 2026-11-22 20:16 | `jshandle-as-element.spec.ts` | 4/4 PASS (4.2s, exit 0) | PW Batch 65 (全绿) |
| 2026-11-22 20:16 | `jshandle-json-value.spec.ts` | 3/3 PASS (2.9s, exit 0) | PW Batch 65 (全绿) |
| 2026-11-22 20:16 | `elementhandle-bounding-box.spec.ts` | 4/7 PASS (90s retry截断) | PW Batch 65 |
| 2026-11-22 20:16 | `elementhandle-wait-for-element-state.spec.ts` | 11/12 PASS (24.0s, exit 0) | PW Batch 65 |
| 2026-11-22 20:16 | `page-evaluate.spec.ts` | 20/95 PASS (90s截断) | PW Batch 65 |
| 2026-11-22 20:22 | `selectors-text.spec.ts` | 21/21 PASS (48.8s, 3 exp dev PASS) | PW Batch 66 (全绿) |
| 2026-11-22 20:22 | `jshandle-to-string.spec.ts` | 4/6 PASS (17.2s, `/foo/`+`Symbol` PASS) | PW Batch 66 |
| 2026-11-22 20:22 | `page-wait-for-selector-1.spec.ts` | 14/15 PASS (`removed MutationObserver` PASS) | PW Batch 66 |
| 2026-11-22 20:22 | `elementhandle-click.spec.ts` | 5/8 PASS (90s retry截断) | PW Batch 66 |
| 2026-11-22 20:27 | `page-event-console.spec.ts` | 2/19 PASS (90s截断) | PW Batch 67 |
| 2026-11-22 20:27 | `page-event-pageerror.spec.ts` | 4/7 PASS (3 exp dev PASS) | PW Batch 67 |
| 2026-11-22 20:27 | `page-event-request.spec.ts` | 2/13 PASS (90s截断) | PW Batch 67 |
| 2026-11-22 20:27 | `locator-convenience.spec.ts` | 21/21 PASS (21.7s, exit 0) | PW Batch 67 (全绿) |
| 2026-11-22 20:29 | `locator-frame.spec.ts` | 6/9 PASS (3 `ERR_BLOCKED_BY_CLIENT`) | PW Batch 68 |
| 2026-11-22 20:29 | `locator-query.spec.ts` | 21/21 PASS (exit 0) | PW Batch 68 (全绿) |
| 2026-11-22 20:31 | `locator-element-handle.spec.ts` | 5/5 PASS (6.3s, exit 0) | PW Batch 69 (全绿) |
| 2026-11-22 20:31 | `elementhandle-scroll-into-view.spec.ts` | 5/9 PASS (90s retry截断) | PW Batch 69 |
| 2026-11-22 20:35 | `locator-click.spec.ts` | 5/5 PASS (exit 0) | PW Batch 70 (全绿) |
| 2026-11-22 20:35 | `page-click-timeout-3.spec.ts` | 3/4 PASS | PW Batch 70 |
| 2026-11-22 20:35 | `page-click-timeout-4.spec.ts` | 1/3 PASS | PW Batch 70 |
| 2026-11-22 20:35 | `page-keyboard.spec.ts` | 6/44 PASS (90s截断) | PW Batch 70 |
| 2026-11-22 20:38 | `/css/css-values/` (`2/5..5/5`) | 18/70, 18/70, 22/70, 11/70 PASS (3 `FAIL→PASS`) | WPT Batch 85 (`bin_dev`) |
| 2026-11-22 20:38 | `/css/css-sizing/` (`1/5`) | 0/7 PASS | WPT Batch 85 (`bin_dev`) |
| 2026-11-22 20:42 | `elementhandle-bounding-box.spec.ts` | 4/7 PASS (90s retry截断) | PW Batch 71 |
| 2026-11-22 20:42 | `elementhandle-click.spec.ts` | 6/8 PASS (`Node removed` PASS) | PW Batch 71 |
| 2026-11-22 20:42 | `page-mouse.spec.ts` | 0/15 PASS (`isTrusted=false`, 90s截断) | PW Batch 71 |
| 2026-11-22 20:42 | `page-click-react.spec.ts` | 0/3 PASS (90s retry截断) | PW Batch 71 |
| 2026-11-22 20:48 | `jshandle-to-string.spec.ts` | 5/6 PASS (sparse array PASS) | PW Batch 72 |
| 2026-11-22 20:48 | `page-keyboard.spec.ts` | 21/44 PASS (8 location PASS, 90s截断) | PW Batch 72 |
| 2026-11-22 20:48 | `page-event-console.spec.ts` | 3/19 PASS (`@smoke` PASS, 90s截断) | PW Batch 72 |
| 2026-11-22 20:48 | `page-click.spec.ts` | 10/77 PASS (90s截断) | PW Batch 72 |
| 2026-11-22 20:52 | `page-check.spec.ts` | 12/16 PASS | PW Batch 73 |
| 2026-11-22 20:52 | `page-fill.spec.ts` | 9/40 PASS (90s截断) | PW Batch 73 |
| 2026-11-22 20:52 | `page-focus.spec.ts` | 6/7 PASS (exit 0, match exp) | PW Batch 73 |
| 2026-11-22 20:58 | `/css/css-sizing/` (`2/5..5/5`) | 1/7, 0/6, 0/6, 0/6 PASS | WPT Batch 86 (`bin_dev`) |
| 2026-11-22 20:58 | `/css/css-text/` (`1/5`) | 4/25 PASS | WPT Batch 86 (`bin_dev`) |
| 2026-11-22 20:59 | `page-mouse.spec.ts` | 7/15 PASS (`[1..5,9,10]` PASS, 90s截断) | PW Batch 74 |
| 2026-11-22 20:59 | `page-select-option.spec.ts` | 0/33 PASS (90s截断) | PW Batch 74 |
| 2026-11-22 20:59 | `page-set-content.spec.ts` | 14/16 PASS (90s截断) | PW Batch 74 |
| 2026-11-22 20:59 | `page-history.spec.ts` | 0/13 PASS (90s截断) | PW Batch 74 |
| 2026-11-22 21:01 | `/css/css-text/` (`2/5..5/5`) | 4/25, 7/25, 6/25, 3/25 PASS | WPT Batch 87 (`bin_dev`) |
| 2026-11-22 21:01 | `/css/css-display/` (`1/5`) | 4/11 PASS | WPT Batch 87 (`bin_dev`) |
| 2026-11-22 21:02 | `jshandle-to-string.spec.ts` | 6/6 PASS (3 exp dev PASS) | PW Batch 75 (全绿) |
| 2026-11-22 21:02 | `page-keyboard.spec.ts` | 29/44 PASS (`[1..30]` 29/30, 90s截断) | PW Batch 75 |
| 2026-11-22 21:07 | `page-mouse.spec.ts` | 8/15 PASS (`pointerType` PASS, 90s截断) | PW Batch 76 |
| 2026-11-22 21:07 | `page-expose-function.spec.ts` | 0/18 PASS (90s截断) | PW Batch 76 |
| 2026-11-22 21:07 | `page-add-init-script.spec.ts` | 9/11 PASS | PW Batch 76 |
| 2026-11-22 21:07 | `page-add-script-tag.spec.ts` | 7/9 PASS | PW Batch 76 |
| 2026-11-22 21:12 | `/css/css-display/` (`2/5..5/5`) | 1/11, 3/10, 4/10, 2/10 PASS | WPT Batch 88 (`bin_dev`) |
| 2026-11-22 21:12 | `/css/css-position/` (`1/5`) | 15/54 PASS | WPT Batch 88 (`bin_dev`) |
| 2026-11-22 21:12 | `page-mouse.spec.ts` | 12/15 PASS (`drag select` PASS, 90s截断) | PW Batch 77 |
| 2026-11-22 21:12 | `page-keyboard.spec.ts` | 30/44 PASS (`[1..30]` 30/30全绿, 90s截断) | PW Batch 77 |
| 2026-11-22 21:12 | `page-dialog.spec.ts` | 7/9 PASS (3 exp dev PASS) | PW Batch 77 |
| 2026-11-22 21:12 | `page-dispatchevent.spec.ts` | 15/16 PASS | PW Batch 77 |
| 2026-11-22 21:18 | `page-add-script-tag.spec.ts` | 8/9 PASS (`sourceURL` PASS) | PW Batch 78 |
| 2026-11-22 21:18 | `page-click.spec.ts` | 12/77 PASS (`half-aligned` PASS, 90s截断) | PW Batch 78 |
| 2026-11-22 21:18 | `page-set-input-files.spec.ts` | 9/19 PASS (90s截断) | PW Batch 78 |
| 2026-11-22 21:18 | `page-check.spec.ts` | 12/16 PASS | PW Batch 78 |
| 2026-11-22 21:22 | `page-dispatchevent.spec.ts` | 15/16 PASS | PW Batch 79 |
| 2026-11-22 21:22 | `page-mouse.spec.ts` | 14/15 PASS (`modifiers`+`round down` PASS, 90s截断) | PW Batch 79 |
| 2026-11-22 21:22 | `page-add-style-tag.spec.ts` | 2/4 PASS | PW Batch 79 |
| 2026-11-22 21:22 | `page-set-extra-http-headers.spec.ts` | 4/5 PASS (retry全过) | PW Batch 79 |
| 2026-11-22 21:29 | `page-basic.spec.ts` | 12/16 PASS (`Enter` PASS) | PW Batch 80 |
| 2026-11-22 21:29 | `page-goto.spec.ts` | 7/61 PASS (90s截断) | PW Batch 80 |
| 2026-11-22 21:29 | `page-wait-for-load-state.spec.ts` | 6/10 PASS (90s retry截断) | PW Batch 80 |
| 2026-11-22 21:29 | `page-wait-for-navigation.spec.ts` | 1/12 PASS (90s截断) | PW Batch 80 |
| 2026-11-22 21:34 | `page-add-script-tag.spec.ts` | 9/9 PASS (`sourceURL`+`CSP`全绿) | PW Batch 81 (全绿) |
| 2026-11-22 21:34 | `page-dispatchevent.spec.ts` | 15/16 PASS | PW Batch 81 |
| 2026-11-22 21:34 | `page-mouse.spec.ts` | 13/15 PASS (90s截断) | PW Batch 81 |
| 2026-11-22 21:34 | `locator-misc-1.spec.ts` | 8/13 PASS (90s截断) | PW Batch 81 |
| 2026-11-22 21:35 | `/css/css-position/` (`2/5..5/5`) | 13/54, 17/54, 15/54, 11/53 PASS (5 `FAIL→PASS`) | WPT Batch 89 (`bin_dev`) |
| 2026-11-22 21:35 | `/css/css-flexbox/` (`1/5`) | 24/53 PASS | WPT Batch 89 (`bin_dev`) |
| 2026-11-22 21:39 | `page-add-style-tag.spec.ts` | 3/4 PASS (CSP style URL PASS) | PW Batch 82 |
| 2026-11-22 21:39 | `page-emulate-media.spec.ts` | 3/12 PASS (exit 0, match exp) | PW Batch 82 |
| 2026-11-22 21:39 | `page-screenshot.spec.ts` | 1/44 PASS (90s截断) | PW Batch 82 |
| 2026-11-22 21:39 | `page-network-idle.spec.ts` | 0/10 PASS (90s截断) | PW Batch 82 |
| 2026-11-22 21:43 | `elementhandle-convenience.spec.ts` | 13/14 PASS (29.5s, exit 0) | PW Batch 83 |
| 2026-11-22 21:43 | `elementhandle-misc.spec.ts` | 7/10 PASS (90s截断) | PW Batch 83 |
| 2026-11-22 21:43 | `elementhandle-press.spec.ts` | 5/5 PASS (7.9s, number input PASS) | PW Batch 83 (全绿) |
| 2026-11-22 21:43 | `elementhandle-query-selector.spec.ts` | 6/6 PASS (8.7s, exit 0) | PW Batch 83 (全绿) |
| 2026-11-22 21:48 | `elementhandle-type.spec.ts` | 5/5 PASS (`number input` PASS) | PW Batch 84 (全绿) |
| 2026-11-22 21:48 | `eval-on-selector.spec.ts` | 32/32 PASS (exit 0) | PW Batch 84 (全绿) |
| 2026-11-22 21:48 | `eval-on-selector-all.spec.ts` | 9/9 PASS (2 `* capture` PASS) | PW Batch 84 (全绿) |
| 2026-11-22 21:48 | `frame-evaluate.spec.ts` | 2/8 PASS (exit 0, match exp) | PW Batch 84 |
| 2026-11-22 21:51 | `locator-evaluate.spec.ts` | 5/5 PASS (exit 0) | PW Batch 85 (全绿) |
| 2026-11-22 21:51 | `locator-is-visible.spec.ts` | 8/8 PASS (exit 0) | PW Batch 85 (全绿) |
| 2026-11-22 21:51 | `locator-list.spec.ts` | 1/1 PASS (exit 0) | PW Batch 85 (全绿) |
| 2026-11-22 21:51 | `expect-boolean.spec.ts` | 60/84 PASS (90s截断) | PW Batch 85 |
| 2026-11-22 21:57 | `expect-to-have-text.spec.ts` | 26/28 PASS | PW Batch 86 |
| 2026-11-22 21:57 | `frame-frame-element.spec.ts` | 1/6 PASS | PW Batch 86 |
| 2026-11-22 21:57 | `frame-hierarchy.spec.ts` | 4/12 PASS (90s retry截断) | PW Batch 86 |
| 2026-11-22 21:57 | `interception.spec.ts` | 3/8 PASS | PW Batch 86 |
| 2026-11-22 22:03 | `page-emulate-media.spec.ts` | 12/12 PASS (9 exp FAIL→PASS) | PW Batch 87 (`bin_dev` 全绿) |
| 2026-11-22 22:05 | `selectors-misc.spec.ts` | 7/19 PASS (`#inner-link` shadow dom 超时, 90s截断) | PW Batch 87 |
| 2026-11-22 22:05 | `selectors-role.spec.ts` | 12/15 PASS (`disabled`/`hidden` slot) | PW Batch 87 |
| 2026-11-22 22:05 | `selectors-frame.spec.ts` | 4/15 PASS (`ERR_BLOCKED_BY_CLIENT`, 90s截断) | PW Batch 87 |
| 2026-11-22 22:08 | `locator-misc-2.spec.ts` | 4/14 PASS (90s截断于`[7/14]`) | PW Batch 88 |
| 2026-11-22 22:08 | `page-event-request.spec.ts` | 2/13 PASS (`fetch`未报事件/`route`拦截, 90s截断) | PW Batch 88 |
| 2026-11-22 22:15 | `page-wait-for-selector-1.spec.ts` | 14/15 PASS (`frame detached` 1 fail) | PW Batch 89 |
| 2026-11-22 22:15 | `page-wait-for-selector-2.spec.ts` | 22/32 PASS (`[24/32]` 90s截断) | PW Batch 89 |
| 2026-11-22 22:15 | `page-wait-for-function.spec.ts` | 8/26 PASS (`[11/26]` 90s截断) | PW Batch 89 |
| 2026-11-22 22:15 | `page-evaluate.spec.ts` | 20/95 PASS (`[1..20]`零失败, `[21]` reload 90s截断) | PW Batch 89 |
| 2026-11-22 22:21 | `expect-to-have-text.spec.ts` | 28/28 PASS (35.7s, `in shadow dom`+`#field`修复) | PW Batch 90 (`bin_dev` 全绿) |
| 2026-11-22 22:21 | `selectors-role.spec.ts` | 9/15 PASS (`Shadow1`PASS, `[12/15]`90s截断) | PW Batch 90 |
| 2026-11-22 22:21 | `selectors-misc.spec.ts` | 7/19 PASS (`#inner-link`/`layout`90s截断) | PW Batch 90 |
| 2026-11-22 22:21 | `locator-misc-2.spec.ts` | 5/14 PASS (`lazyload`PASS, `before`PASS/`after>0`, 90s截断) | PW Batch 90 |
| 2026-11-22 22:21 | `/css/css-flexbox/` (`2/5..5/5`) | 26/52, 23/52, 16/52, 25/52 PASS | WPT Batch 90 (`bin_dev`) |
| 2026-11-22 22:21 | `/css/css-grid/` (`1/5`) | 63/177 PASS (11 `FAIL→PASS`) | WPT Batch 90 (`bin_dev`) |
| 2026-11-22 22:26 | `elementhandle-bounding-box.spec.ts` | 4/7 PASS (SVG `#therect` null/90s截断) | PW Batch 91 |
| 2026-11-22 22:26 | `elementhandle-scroll-into-view.spec.ts` | 5/9 PASS (`@smoke` `after>0`/90s截断) | PW Batch 91 |
| 2026-11-22 22:26 | `elementhandle-wait-for-element-state.spec.ts` | 11/12 PASS (23.9s, match exp) | PW Batch 91 |
| 2026-11-22 22:26 | `expect-boolean.spec.ts` | 77/78 PASS (`[78/84]`90s截断, 仅`frameLocator 2`1 fail) | PW Batch 91 |
| 2026-11-22 22:31 | `page-select-option.spec.ts` | 0/33 PASS (`[1/33]`/`[2/33]` 90s截断) | PW Batch 92 |
| 2026-11-22 22:31 | `page-fill.spec.ts` | 35/37 PASS (`[38/40]` 90s截断, 仅2 fail) | PW Batch 92 |
| 2026-11-22 22:31 | `page-check.spec.ts` | 16/16 PASS (30.5s) | PW Batch 92 (`bin_dev` 全绿) |
| 2026-11-22 22:31 | `page-focus.spec.ts` | 7/7 PASS (1 exp FAIL→PASS) | PW Batch 92 (`bin_dev` 全绿) |
| 2026-11-22 22:37 | `page-keyboard.spec.ts` | 30/44 PASS (`[1..30]`零失败, `[31]`PageDown 90s截断) | PW Batch 93 |
| 2026-11-22 22:37 | `page-mouse.spec.ts` | 8/15 PASS (`hover #button-91` 90s截断) | PW Batch 93 |
| 2026-11-22 22:37 | `page-click.spec.ts` | 12/77 PASS (`frameset` 90s截断) | PW Batch 93 |
| 2026-11-22 22:37 | `page-dispatchevent.spec.ts` | 15/16 PASS (29.0s, 仅`should be atomic`) | PW Batch 93 |
| 2026-11-22 22:40 | `page-add-init-script.spec.ts` | 9/11 PASS (CSP/iframe 2 fail) | PW Batch 94 |
| 2026-11-22 22:40 | `page-set-extra-http-headers.spec.ts` | 5/5 PASS (1 exp FAIL→PASS) | PW Batch 94 (`bin_dev` 全绿) |
| 2026-11-22 22:40 | `page-dialog.spec.ts` | 7/9 PASS (3 dismiss PASS, 2 accept fail) | PW Batch 94 |
| 2026-11-22 22:40 | `frame-evaluate.spec.ts` | 2/8 PASS (match exp) | PW Batch 94 |
| 2026-11-22 22:47 | `expect-boolean.spec.ts` | 77/78 PASS (`[78/84]`90s截断) | PW Batch 95 |
| 2026-11-22 22:47 | `elementhandle-bounding-box.spec.ts` | 5/7 PASS (`#therect`PASS) | PW Batch 95 |
| 2026-11-22 22:47 | `elementhandle-scroll-into-view.spec.ts` | 6/8 PASS (`stop moving`PASS, `[8/9]`90s截断) | PW Batch 95 |
| 2026-11-22 22:47 | `page-fill.spec.ts` | 37/38 PASS (`type=number`清空PASS, 仅`·`1 fail) | PW Batch 95 |
| 2026-11-22 22:51 | `/css/css-flexbox/` (`2/5..5/5`) | 26/52, 23/52, 15/52, 25/52 PASS | WPT Batch 90 (`19470`, `bin_dev`) |
| 2026-11-22 22:51 | `/css/css-grid/` (`1/5`) | 63/177 PASS (11 `FAIL→PASS`, 790s) | WPT Batch 90 (`19470`, `bin_dev`) |
| 2026-11-22 22:53 | `selectors-role.spec.ts` | 8/10 PASS (`[10/15]`90s截断) | PW Batch 96 |
| 2026-11-22 22:53 | `selectors-misc.spec.ts` | 7/19 PASS (`#inner-link`/`layout`90s截断) | PW Batch 96 |
| 2026-11-22 22:53 | `locator-misc-2.spec.ts` | 5/14 PASS (`[7/14]`90s截断) | PW Batch 96 |
| 2026-11-22 22:53 | `page-add-init-script.spec.ts` | 10/11 PASS (CSP PASS, 17.8s, 仅iframe 1 fail) | PW Batch 96 |
| 2026-11-22 23:00 | `elementhandle-bounding-box.spec.ts` | 5/7 PASS (90s截断于retry) | PW Batch 97 |
| 2026-11-22 23:00 | `expect-boolean.spec.ts` | 77/78 PASS (`[78/84]`90s截断) | PW Batch 97 |
| 2026-11-22 23:00 | `page-wait-for-load-state.spec.ts` | 6/10 PASS (90s截断于retry) | PW Batch 97 |
| 2026-11-22 23:00 | `page-wait-for-navigation.spec.ts` | 1/12 PASS (`grid.html` 90s截断) | PW Batch 97 |
| 2026-11-22 23:01 | `/css/css-grid/` (`2/5..5/5`) | 48/176, 21/89, 30/137, 43/176 PASS (27 `FAIL→PASS`, 3-5/5 120s截断) | WPT Batch 91 (`19470`, `bin_dev`) |
| 2026-11-22 23:01 | `/css/css-cascade/` (`1/3`) | 10/30 PASS (48s) | WPT Batch 91 (`19470`, `bin_dev`) |
| 2026-11-22 23:05 | `page-add-init-script.spec.ts` | 11/11 PASS (10.2s, 2 exp FAIL→PASS) | PW Batch 98 (`bin_dev` 全绿) |
| 2026-11-22 23:05 | `elementhandle-scroll-into-view.spec.ts` | 6/9 PASS (`visibility:hidden`PASS, 90s截断于retry) | PW Batch 98 |
| 2026-11-22 23:05 | `elementhandle-bounding-box.spec.ts` | 5/7 PASS (90s截断于retry) | PW Batch 98 |
| 2026-11-22 23:05 | `page-dispatchevent.spec.ts` | 15/16 PASS (29.9s) | PW Batch 98 |
| 2026-11-22 23:10 | `expect-boolean.spec.ts` | 77/78 PASS (`[79/84]`90s截断) | PW Batch 99 |
| 2026-11-22 23:10 | `selectors-role.spec.ts` | 10/11 PASS (`[10/15]`PASS, `[12/15]`90s截断) | PW Batch 99 |
| 2026-11-22 23:10 | `page-fill.spec.ts` | 37/38 PASS (`[39/40]`90s截断) | PW Batch 99 |
| 2026-11-22 23:10 | `page-basic.spec.ts` | 12/16 PASS (`page.press`/`frame.press` exp FAIL→PASS) | PW Batch 99 |
| 2026-11-22 23:12 | `/css/css-cascade/` (`2/3`, `3/3`) | 12/30, 14/30 PASS (1 `FAIL→PASS`, 41s/25s) | WPT Batch 92 (`19470`, `bin_dev`) |
| 2026-11-22 23:12 | `/css/css-position/` (`1/5..3/5`) | 17/54, 15/54, 20/54 PASS (4 `FAIL→PASS`) | WPT Batch 92 (`19470`, `bin_dev`) |
| 2026-11-22 23:16 | `elementhandle-bounding-box.spec.ts` | 6/7 PASS (17.3s! `grid.html`+`3244`PASS, 仅y=55 vs 50) | PW Batch 100 |
| 2026-11-22 23:16 | `page-wait-for-navigation.spec.ts` | 1/12 PASS (90s截断于`[4/12]`) | PW Batch 100 |
| 2026-11-22 23:16 | `page-mouse.spec.ts` | 14/15 PASS (仅`#button-91` hover 1 fail, 90s收尾截断) | PW Batch 100 |
| 2026-11-22 23:16 | `elementhandle-scroll-into-view.spec.ts` | 6/9 PASS (56.4s, retry 90s截断) | PW Batch 100 |
| 2026-11-22 23:22 | `page-set-content.spec.ts` | 14/15 PASS (`[8/16] /img.png` 1 fail, `[16/16]`90s截断) | PW Batch 101 |
| 2026-11-22 23:22 | `page-add-style-tag.spec.ts` | 3/4 PASS (CSP PASS, 仅`[3/4]`bad URL) | PW Batch 101 |
| 2026-11-22 23:22 | `page-screenshot.spec.ts` | 1/3 PASS (`[4/44]`90s截断) | PW Batch 101 |
| 2026-11-22 23:22 | `page-goto.spec.ts` | 7/19 PASS (`[20/61]`90s截断, `file://`/`null` response) | PW Batch 101 |
| 2026-11-22 23:32 | `page-mouse.spec.ts` | 13/14 PASS (`[15/15]`90s截断, 仅`should trigger hover state`) | PW Batch 102 |
| 2026-11-22 23:32 | `elementhandle-scroll-into-view.spec.ts` | 6/9 PASS (2 fail / 1 skip, retry 90s截断) | PW Batch 102 |
| 2026-11-22 23:32 | `selectors-misc.spec.ts` | 18/19 PASS (`layout selectors`等11条PASS! 仅`#inner-link`) | PW Batch 102 |
| 2026-11-22 23:32 | `page-wait-for-load-state.spec.ts` | 6/10 PASS (retry 90s截断) | PW Batch 102 |
| 2026-11-22 23:34 | `/css/css-values/` (`4/5..5/5`) | 22/70, 11/70 PASS (2 `FAIL→PASS`, pass 2 180s截断) | WPT Batch 94 (`19470`, `bin_dev`) |
| 2026-11-22 23:34 | `/css/css-color/` (`1/5..3/5`) | 0/12, 1/11, 0/11 PASS | WPT Batch 94 (`19470`, `bin_dev`) |
| 2026-11-22 23:37 | `page-add-style-tag.spec.ts` | 4/4 PASS (8s, CSP+404全绿!) | PW Batch 103 (`bin_dev` 全绿) |
| 2026-11-22 23:37 | `page-set-content.spec.ts` | 15/16 PASS (`[16/16]`90s收尾截断, 仅`/img.png`) | PW Batch 103 |
| 2026-11-22 23:37 | `page-network-idle.spec.ts` | 0/3 PASS (`[4/10]`90s截断) | PW Batch 103 |
| 2026-11-22 23:37 | `page-goto.spec.ts` | 7/19 PASS (`[20/61]`90s截断) | PW Batch 103 |
| 2026-11-22 23:40 | `/css/css-color/` (`4/5..5/5`) | 0/11, 1/11 PASS | WPT Batch 95 (`19470`, `bin_dev`) |
| 2026-11-22 23:40 | `/css/css-backgrounds/` (`1/5..3/5`) | 6/23, 9/23, 6/23 PASS | WPT Batch 95 (`19470`, `bin_dev`) |
| 2026-11-22 23:50 | `page-set-input-files.spec.ts` | 9/19 PASS (`[15/19]`90s截断) | PW Batch 104 |
| 2026-11-22 23:50 | `page-select-option.spec.ts` | 0/33 PASS (`[2/33]`90s截断) | PW Batch 104 |
| 2026-11-22 23:50 | `frame-frame-element.spec.ts` | 1/6 PASS (55s) | PW Batch 104 |
| 2026-11-22 23:50 | `frame-hierarchy.spec.ts` | 4/12 PASS (retry 90s截断) | PW Batch 104 |
| 2026-11-22 23:51 | `/css/css-backgrounds/` (`4/5..5/5`) | 5/23, 7/23 PASS | WPT Batch 96 (`19470`, `bin_dev`) |
| 2026-11-22 23:51 | `/css/css-sizing/` (`1/5..3/5`) | 0/7, 1/7, 0/6 PASS (180s截断) | WPT Batch 96 (`19470`, `bin_dev`) |
| 2026-11-22 23:55 | `page-set-content.spec.ts` | 15/16 PASS (`[16/16]`90s收尾截断, 仅`/img.png`) | PW Batch 105 |
| 2026-11-22 23:55 | `page-mouse.spec.ts` | 14/14 PASS (`[1..14]`零失败! `[15/15]`90s截断) | PW Batch 105 |
| 2026-11-22 23:55 | `selectors-misc.spec.ts` | 18/19 PASS (43.7s, `internal:has=`PASS) | PW Batch 105 |
| 2026-11-22 23:55 | `page-dialog.spec.ts` | 7/9 PASS (18.8s, 3 exp FAIL→PASS) | PW Batch 105 |
| 2026-11-22 23:59 | `/css/css-sizing/` (`4/5..5/5`) | 0/6, 0/6 PASS (180s截断) | WPT Batch 97 (`19470`, `bin_dev`) |
| 2026-11-22 23:59 | `/css/css-text/` (`1/5..3/5`) | 4/25, 4/25, 7/25 PASS (4 `FAIL→PASS`) | WPT Batch 97 (`19470`, `bin_dev`) |
| 2026-11-23 00:01 | `/css/css-text/` (`4/5..5/5`) | 6/25, 3/25 PASS (5 `FAIL→PASS`) | WPT Batch 98 (`19470`, `bin_dev`) |
| 2026-11-23 00:01 | `/css/css-transforms/` (`1/5..3/5`) | 0/7, 0/6, 1/6 PASS | WPT Batch 98 (`19470`, `bin_dev`) |
| 2026-11-23 00:08 | `/css/css-transforms/` (`4/5..5/5`) | 0/6, 1/6 PASS | WPT Batch 99 (`19470`, `bin_dev`) |
| 2026-11-23 00:08 | `/css/css-grid/` (`1/5..3/5`) | 31/165, 39/176, 38/176 PASS (14 `FAIL→PASS`) | WPT Batch 99 (`19470`, `bin_dev`) |
| 2026-11-23 00:11 | `frame-frame-element.spec.ts` | 1/6 PASS (Phase 358/359前启动) | PW Batch 107 |
| 2026-11-23 00:11 | `page-network-response.spec.ts` | 0/11 PASS (`goto`返回`null`, `[12/21]`120s截断) | PW Batch 107 |
| 2026-11-23 00:11 | `page-network-idle.spec.ts` | 0/4 PASS (`[5/10]`120s截断) | PW Batch 107 |
| 2026-11-23 00:11 | `page-goto.spec.ts` | 13/26 PASS (`file://`等PASS! `[27/61]`120s截断) | PW Batch 107 |
| 2026-11-23 00:18 | `selectors-misc.spec.ts` | 18/19 PASS (仅`#inner-link` `TypeError: not a function`) | PW Batch 108 |
| 2026-11-23 00:18 | `frame-frame-element.spec.ts` | 1/6 PASS | PW Batch 108 |
| 2026-11-23 00:18 | `elementhandle-content-frame.spec.ts` | 2/5 PASS (非allow-list直跑) | PW Batch 108 |
| 2026-11-23 00:18 | `elementhandle-owner-frame.spec.ts` | 2/7 PASS (非allow-list直跑) | PW Batch 108 |
| 2026-11-23 00:22 | `/css/css-grid/` (`4/5..5/5`) | 39/176, 43/176 PASS (18 `FAIL→PASS`!) | WPT Batch 100 (`19470`, `bin_dev`) |
| 2026-11-23 00:22 | `/css/css-flexbox/` (`1/5..3/5`) | 26/53, 25/52, 23/52 PASS | WPT Batch 100 (`19470`, `bin_dev`) |
| 2026-11-23 00:26 | `page-network-response.spec.ts` | 4/12 PASS (Phase 360解锁Response! `[13/21]`120s截断) | PW Batch 109 |
| 2026-11-23 00:26 | `page-goto.spec.ts` | 21/27 PASS (Phase 360+361暴涨+8! `[28/61]`120s截断) | PW Batch 109 |
| 2026-11-23 00:26 | `page-network-idle.spec.ts` | 1/4 PASS (`[5/10]`120s截断) | PW Batch 109 |
| 2026-11-23 00:26 | `elementhandle-bounding-box.spec.ts` | 7/7 PASS (11.8s, 6 exp FAIL→PASS) | PW Batch 109 (`bin_dev` 全绿!) |
| 2026-11-23 00:31 | `/css/css-flexbox/` (`4/5..5/5`) | 15/52, 25/52 PASS | WPT Batch 101 (`19470`, `bin_dev`) |
| 2026-11-23 00:31 | `/css/css-tables/` (`1/3..3/3`) | 0/6, 4/6, 0/6 PASS | WPT Batch 101 (`19470`, `bin_dev`) |
| 2026-11-23 00:32 | `selectors-misc.spec.ts` | 19/19 PASS (`internal:has=`+`#inner-link`全绿!) | PW Batch 110 (`bin_dev` 全绿!) |
| 2026-11-23 00:32 | `frame-frame-element.spec.ts` | 1/6 PASS | PW Batch 110 |
| 2026-11-23 00:32 | `expect-boolean.spec.ts` | 83/84 PASS (仅`frameLocator 2` 1 fail!) | PW Batch 110 |
| 2026-11-23 00:32 | `frame-goto.spec.ts` | 0/4 PASS (非allow-list直跑) | PW Batch 110 |
| 2026-11-23 00:38 | `/css/css-overflow/` (`1/3..3/3`) | 2/11, 1/11, 3/11 PASS (4 `FAIL→PASS`) | WPT Batch 102 (`19470`, `bin_dev`) |
| 2026-11-23 00:38 | `/css/css-align/` (`1/2..2/2`) | 7/60, 10/59 PASS | WPT Batch 102 (`19470`, `bin_dev`) |
| 2026-11-23 00:42 | `page-network-response.spec.ts` | 7/12 PASS (headers+statusText全过! `[13/21]`120s截断) | PW Batch 111 |
| 2026-11-23 00:42 | `page-network-idle.spec.ts` | 1/2 PASS (`fetch-a`已达! `[3/10]`120s截断) | PW Batch 111 |
| 2026-11-23 00:42 | `page-goto.spec.ts` | 22/28 PASS (`about:blank`过! `[29/61]`120s截断) | PW Batch 111 |
| 2026-11-23 00:42 | `page-select-option.spec.ts` | 1/3 PASS (`[4/33]`120s截断) | PW Batch 111 |
| 2026-11-23 00:03 | `page-mouse.spec.ts` | 14/14 PASS (`[15/15]`120s截断) | PW Batch 106 |
| 2026-11-23 00:03 | `selectors-misc.spec.ts` | 18/19 PASS (`internal:has=`PASS, 仅`#inner-link`) | PW Batch 106 |
| 2026-11-23 00:03 | `page-select-option.spec.ts` | 4/4 PASS (`[1..4]`全绿! `[5/33]`120s截断) | PW Batch 106 |
| 2026-11-23 00:03 | `frame-frame-element.spec.ts` | 1/6 PASS (`contentFrame` FAIL→PASS) | PW Batch 106 |
| WPT-B103 | `/css/selectors/ 1/3..3/3` + `/dom/nodes/ 1/5..2/5` | 67/168 PASS (4 FAIL→PASS; `/dom/nodes/ 2/5` 尾段串行复测超时 180s) |
| 2026-11-23 00:52 | `expect-boolean.spec.ts` | 83/84 PASS (仅`frameLocator 2`超时, 3 exp FAIL→PASS) | PW Batch 112 |
| 2026-11-23 00:52 | `page-fill.spec.ts` | 38/40 PASS (1报错尾空格 + 1 `load`超时) | PW Batch 112 |
| 2026-11-23 00:52 | `page-event-popup.spec.ts` | 0/13 PASS (`window.open`未定义, 非allow-list) | PW Batch 112 |
| 2026-11-23 00:52 | `network-post-data.spec.ts` | 0/6 PASS (`new Request`+`fetch`未触发`waitForRequest`, 非allow-list) | PW Batch 112 |
| 2026-11-23 00:58 | `page-network-response.spec.ts` | 10/14 PASS (`gzip`+多值头+`statusText`全过! `[15/21]`120s截断) | PW Batch 113 |
| 2026-11-23 00:58 | `frame-frame-element.spec.ts` | 1/6 PASS (`bin_dev` Phase 369前启动) | PW Batch 113 |
| 2026-11-23 00:58 | `page-cache-storage.spec.ts` | 0/1 PASS (`caches is not defined`, 非allow-list) | PW Batch 113 |
| 2026-11-23 00:58 | `page-leaks.spec.ts` | 0/3 PASS (WeakRefs 108/116 > 25, 非allow-list) | PW Batch 113 |
| WPT-B104 | `/dom/nodes/ 3/5..5/5` + `/dom/events/ 1/2..2/2` | 104/279 PASS (3 FAIL→PASS, 0 CRASH) |
| 2026-11-23 01:03 | `page-add-script-tag.spec.ts` | 9/9 PASS (8.3s, 1 exp FAIL→PASS) | PW Batch 114 (`bin_dev` 全绿!) |
| 2026-11-23 01:03 | `page-set-content.spec.ts` | 14/16 PASS (`retry`120s截断) | PW Batch 114 |
| 2026-11-23 01:03 | `frame-frame-element.spec.ts` | 1/6 PASS (Phase 372前启动) | PW Batch 114 |
| 2026-11-23 01:03 | `network-post-data.spec.ts` | 0/6 PASS (13.2s不超时! `postData`仍为空) | PW Batch 114 |
| 2026-11-23 01:10 | `page-network-response.spec.ts` | 14/21 PASS (Phase 371不再超时! +4 PASS) | PW Batch 115 |
| 2026-11-23 01:10 | `page-network-idle.spec.ts` | 1/10 PASS (`:84`超时后断开连接) | PW Batch 115 |
| 2026-11-23 01:10 | `frame-frame-element.spec.ts` | 3/6 PASS (`@smoke`+`contentFrame`+`detached`全过! +2 FAIL→PASS) | PW Batch 115 |
| 2026-11-23 01:10 | `page-cache-storage.spec.ts` | 1/1 PASS (2.2s, 非allow-list) | PW Batch 115 (`bin_dev` 全绿!) |
| WPT-B105 | `/dom/traversal/ 1/2..2/2` + `/dom/ranges/ 1/3..3/3` | 12/53 PASS (0 CRASH, `/dom/ranges/` 尾段串行复测超时 180s) |
| WPT-B106 | `/dom/collections/` + `/dom/lists/` + `/dom/abort/` + `/html/semantics/selectors/` | 12/27 PASS (0 CRASH/0 TIMEOUT; selectors 4/4 matched) |
| 2026-11-23 01:23 | `network-post-data.spec.ts` | 6/6功能全过! (5.7s, 4 pass + 2 upstream `it.fail` 实际 PASS) | PW Batch 116 (`bin_dev` 功能全绿!) |
| 2026-11-23 01:23 | `expect-boolean.spec.ts` | 83/84 PASS (仅`frameLocator 2`超时, 3 exp FAIL→PASS) | PW Batch 116 |
| 2026-11-23 01:23 | `page-mouse.spec.ts` | 14/14 PASS (`[15/15]`120s截断) | PW Batch 116 |
| 2026-11-23 01:23 | `page-history.spec.ts` | 0/3 PASS (`goBack`等待`load`超时, 120s截断) | PW Batch 116 |
| 2026-11-23 01:30 | `page-network-response.spec.ts` | 15/21 PASS (`Response.formData()` PASS! +1 PASS) | PW Batch 117 |
| 2026-11-23 01:30 | `frame-frame-element.spec.ts` | 3/6 PASS (`@smoke`+`contentFrame`+`detached`) | PW Batch 117 |
| 2026-11-23 01:30 | `page-event-popup.spec.ts` | 0/3 PASS (`waitForEvent('popup')`超时, 120s截断) | PW Batch 117 |
| 2026-11-23 01:30 | `page-focus.spec.ts` | 7/7 PASS (1 exp FAIL→PASS, 重试全过) | PW Batch 117 (`bin_dev` 全绿!) |
| WPT-B107 | `/html/dom/ 1/4..4/4` (全量) | 135/267 PASS (50.6%, 0 CRASH, 1 rerun PASS) |
| 2026-11-23 01:33 | `page-emulate-media.spec.ts` | 12/12 PASS (9 exp FAIL→PASS) | PW Batch 118 (`bin_dev` 全绿!) |
| 2026-11-23 01:33 | `page-check.spec.ts` | 16/16 PASS (`EXIT=0`全绿!) | PW Batch 118 (`bin_dev` 全绿!) |
| 2026-11-23 01:33 | `page-click.spec.ts` | 19/22 PASS (`[23/77]`120s截断) | PW Batch 118 |
| WPT-B108 | `/html/semantics/forms/the-{input,select,form}-element/` | 141/172 PASS (82.0%, 0 CRASH, select 49/54) |
| WPT-B109 | `/html/semantics/forms/the-{textarea,button,label,fieldset,option}-element/` | 37/55 PASS (0 CRASH; button 13/16, textarea 12/17) |
| 2026-11-23 01:40 | `page-dispatchevent.spec.ts` | 15/17 PASS (1.0m, 非allow-list) | PW Batch 119 |
| 2026-11-23 01:40 | `page-keyboard.spec.ts` | 34/40 PASS (`[41/44]`120s截断) | PW Batch 119 |
| 2026-11-23 01:40 | `locator-convenience.spec.ts` | 21/21 PASS (`EXIT=0` 21.7s 全绿!) | PW Batch 119 (`bin_dev` 全绿!) |
| 2026-11-23 01:40 | `page-fill.spec.ts` | 38/40 PASS (retry 120s截断) | PW Batch 119 |
| WPT-B110 | `/html/semantics/forms/{constraints,form-submission-0,attributes-common-to-form-controls}/` | 12/25 PASS (0 CRASH/0 TIMEOUT; constraints 10/12, submission 5/5 matched) |
| 2026-11-23 01:47 | `page-history.spec.ts` | 7/13 PASS (0/3→7/13! reload全过, goBack待修currentIndex) | PW Batch 120 |
| 2026-11-23 01:47 | `page-route.spec.ts` | 1/11 PASS (`[12/51]`120s截断) | PW Batch 120 |
| 2026-11-23 01:47 | `page-evaluate.spec.ts` | 20/21 PASS (`[22/95]`120s截断) | PW Batch 120 |
| 2026-11-23 01:47 | `page-wait-for-selector-1.spec.ts` | 15/15 PASS (`EXIT=0` 29.7s 全绿!) | PW Batch 120 (`bin_dev` 全绿!) |
| WPT-B111 | `/html/semantics/{tabular-data,embedded-content/the-iframe-element}/` | 26/163 PASS (1 `ERROR→PASS` lazy-iframe, 0 CRASH) |
| WPT-B112 | `/html/semantics/embedded-content/the-{img,canvas}-element/` | 2/6 PASS (0 CRASH/0 TIMEOUT) |
| WPT-B113 | `/html/semantics/{scripting-1/the-script-element,document-metadata/the-{link,style}-element}/` | 10/68 PASS (2 `FAIL→PASS`, style 4/5, 0 CRASH) |
| 2026-11-23 01:55 | `page-event-popup.spec.ts` | 5/11 PASS (0/3→5/11! `@smoke`等5项全过, `[12/13]`120s截断) | PW Batch 121 |
| 2026-11-23 01:55 | `page-wait-for-selector-2.spec.ts` | 31/32 PASS (`zero-sized` FAIL→PASS!) | PW Batch 121 |
| 2026-11-23 01:55 | `page-wait-for-navigation.spec.ts` | 1/3 PASS (`[4/12]`120s截断) | PW Batch 121 |
| 2026-11-23 01:55 | `page-click.spec.ts` | 23/25 PASS (Phase 382 checkbox通过! `[25/77]`120s截断) | PW Batch 121 |
| WPT-B114 | `/html/webappapis/{timers,microtask-queuing,dynamic-markup-insertion,scripting/events}/` | 17/32 PASS (23/32 matched, events 9/14, 0 CRASH) |
| WPT-B115 | `/html/webappapis/{system-state,atob,structured-clone}/` + `/html/browsers/history/the-history-interface/` | 12/16 PASS (history 7/7 全绿, 0 CRASH/0 TIMEOUT) |
| WPT-B116 | `/html/browsers/{the-window-object,browsing-the-web/navigating-across-documents}/` | 2/9 PASS (9/9 matched `exit=0`, 0 CRASH) |
| 2026-11-23 02:02 | `page-click.spec.ts` | 28/31 PASS (`[14/77]`不卡viewport! 推进至`[31/77]`) | PW Batch 122 |
| 2026-11-23 02:02 | `page-history.spec.ts` | 7/13 PASS (retry 120s截断) | PW Batch 122 |
| 2026-11-23 02:02 | `page-screenshot.spec.ts` | 2/3 PASS (`[4/44]`120s截断) | PW Batch 122 |
| WPT-B117 | `/html/canvas/` (852) + `/html/infrastructure/` + `/html/syntax/parsing/ 1/3..2/3` | 361/882 PASS (canvas **356/852**, **42 FAIL→PASS**, 0 CRASH) |
| 2026-11-23 02:08 | `eval-on-selector.spec.ts` | 26/26 PASS (非allow-list 全绿!) | PW Batch 123 (`bin_dev` 全绿!) |
| 2026-11-23 02:08 | `elementhandle-eval-on-selector.spec.ts` | 6/6 PASS (非allow-list 全绿!) | PW Batch 123 (`bin_dev` 全绿!) |
| 2026-11-23 02:08 | `eval-on-selector-all.spec.ts` | 9/9 PASS (9.2s, 非allow-list 全绿!) | PW Batch 123 (`bin_dev` 全绿!) |
| 2026-11-23 02:08 | `selectors-text.spec.ts` | 21/21 PASS (48.1s, 3 exp FAIL→PASS 全绿!) | PW Batch 123 (`bin_dev` 全绿!) |
| 2026-11-23 02:08 | `page-wait-for-function.spec.ts` | 8/10 PASS (`[11/26]`120s截断) | PW Batch 123 |
| 2026-11-23 02:15 | `page-event-popup.spec.ts` | 10/13 PASS (5/11→10/13! `noopener`+`_blank`全过!) | PW Batch 124 |
| 2026-11-23 02:15 | `selectors-css.spec.ts` | 21/25 PASS (3 exp FAIL→PASS) | PW Batch 124 |
| 2026-11-23 02:15 | `selectors-register.spec.ts` | 6/6 PASS (`EXIT=0` 全绿!) | PW Batch 124 (`bin_dev` 全绿!) |
| 2026-11-23 02:15 | `page-click.spec.ts` | 29/31 PASS (Phase 387 `contextmenu`过! `[31/77]`120s截断) | PW Batch 124 |
| WPT-B118 | `/html/syntax/parsing/ 3/3` + `/svg/` + `/url/ 1/2..2/2` | 37/106 PASS (`/svg/` 31/65 0崩溃0超时) |
| WPT-B119 | `/fetch/api/{headers,request,basic}/` | 4/68 PASS (暴露 `escapeJsonString` `<0x20` 控制字符未转义导致 JSON.parse 崩溃) |
| 2026-11-23 02:21 | `page-wait-for-selector-2.spec.ts` | 32/32 PASS (Phase 389 全绿! 2 exp FAIL→PASS) | PW Batch 125 (`bin_dev` 全绿!) |
| 2026-11-23 02:21 | `page-event-popup.spec.ts` | 11/13 PASS (Phase 391 `a.click()`过! +1 PASS) | PW Batch 125 |
| 2026-11-23 02:21 | `page-click.spec.ts` | 28/30 PASS (`[31/77]`120s截断) | PW Batch 125 |
| WPT-B120 | `/fetch/api/{response,abort,cors}/` | 4/53 PASS (1 `FAIL→PASS` cors-preflight-redirect, 0 CRASH) |
| 2026-11-23 02:25 | `elementhandle-press.spec.ts` | 5/5 PASS (1 exp FAIL→PASS) | PW Batch 126 (`bin_dev` 全绿!) |
| 2026-11-23 02:25 | `page-set-input-files.spec.ts` | 13/19 PASS (7 exp FAIL→PASS) | PW Batch 126 |
| 2026-11-23 02:25 | `page-aria-snapshot.spec.ts` | 36/38 PASS (2 fails: pseudo PUA `\ueab2`, frameset nested `- iframe`) | PW Batch 126 |
| 2026-11-23 02:25 | `page-wait-for-selector-1.spec.ts` | 15/15 PASS (`exit=0`) | PW Batch 126 (`bin_dev` 全绿!) |
| WPT-B121 | `/fetch/api/basic/ 1/2..2/2` + `/xhr/ 1/3..2/3` | 27/99 PASS (`basic 1/2` 2/16 `1 FAIL→PASS`, 0 JSON.parse crash, 0 CRASH) |
| WPT-B122 | `/xhr/ 3/3` + `/workers/ 1/2` | 22/75 PASS (`/xhr/ 3/3` 13/34, `/workers/ 1/2` 9/41, 0 CRASH) |
| 2026-11-23 02:32 | `selectors-css.spec.ts` | 21/25 PASS (3 exp FAIL→PASS) | PW Batch 127 |
| 2026-11-23 02:32 | `page-aria-snapshot.spec.ts` | 36/38 PASS (2 fails: pseudo `\ueab2`, frameset nested) | PW Batch 127 |
| 2026-11-23 02:32 | `locator-misc-1.spec.ts` | 13/13 PASS (3 exp FAIL→PASS) | PW Batch 127 (`bin_dev` 全绿!) |
| 2026-11-23 02:32 | `locator-click.spec.ts` | 5/5 PASS | PW Batch 127 (`bin_dev` 全绿!) |
| 2026-11-23 02:45 | `page-history.spec.ts` | 7/13 PASS (5 FAIL, 1 TIMEOUT) | PW Batch 128 |
| 2026-11-23 02:45 | `page-screenshot.spec.ts` | 15/44 PASS (23 FAIL, 6 TIMEOUT) | PW Batch 128 |
| 2026-11-23 02:45 | `page-keyboard.spec.ts` | 35/44 PASS (6 FAIL, 3 TIMEOUT) | PW Batch 128 |
| 2026-11-23 03:15 | `page-aria-snapshot.spec.ts` | 37/38 PASS (1 fail: pseudo `"\ueab2 hello"` extra space) | PW Batch 129 |
| 2026-11-23 03:15 | `selectors-css.spec.ts` | 24/25 PASS (Phase 395 +3! 1 fail: `root2Shadow.$eval('css:light=#target')`) | PW Batch 129 |
| 2026-11-23 03:15 | `page-focus.spec.ts` | 7/7 PASS (1 exp FAIL→PASS) | PW Batch 129 (`bin_dev` 全绿!) |
| 2026-11-23 03:15 | `page-click.spec.ts` | 37/77 PASS (4 offset fail, 36 timeout) | PW Batch 129 |
| 2026-11-23 04:12 | `page-history.spec.ts` | 10/13 PASS (Phase 397! 0 FAIL, 3 timeout) | PW Batch 130 |
| 2026-11-23 04:12 | `page-keyboard.spec.ts` | 37/44 PASS (Phase 398! 4 fail, 3 timeout) | PW Batch 130 |
| 2026-11-23 04:12 | `page-network-request.spec.ts` | 7/27 PASS (18 fail, 1 skip, 1 timeout) | PW Batch 130 |
| 2026-11-23 04:12 | `page-network-response.spec.ts` | 14/21 PASS (6 fail, 1 timeout) | PW Batch 130 |
| 2026-11-23 04:40 | `selectors-css.spec.ts` | 25/25 PASS (Phase 399! 0 FAIL, 1 skip) | PW Batch 131 (`bin_dev` 全绿!) |
| 2026-11-23 04:40 | `page-network-idle.spec.ts` | 3/10 PASS (1 fail, 6 timeout) | PW Batch 131 |
| 2026-11-23 04:40 | `page-goto.spec.ts` | 41/61 PASS (`file URL`+subframes过! 10 fail, 10 timeout) | PW Batch 131 |
| 2026-11-23 04:40 | `page-evaluate.spec.ts` | 62/95 PASS (4 fail, 29 timeout) | PW Batch 131 |
| WPT-B123 | `/workers/ 2/2` + `/domparsing/` + `/encoding/ 1/2` + `/streams/ 1/2` | 35/663 PASS (`/workers/ 2/2` 6/40, `/domparsing/` 6/33, `/encoding/ 1/2` 22/548, `/streams/ 1/2` 1/42, 0 CRASH) |
| WPT-B124 | `/streams/ 2/2` + `/webstorage/` + `/console/` + `/performance-timeline/` | 10/79 PASS (`/webstorage/` **3/3**, `/console/` **4/4**, `/streams/ 2/2` 1/42, `/performance-timeline/` 2/21, 0 CRASH) |
| WPT-B125 | `/url/ 1/2..2/2` + `/WebCryptoAPI/ 1/2` | 11/84 PASS (`WebCryptoAPI 1/2` 9/66 **9 FAIL→PASS**, `/url/ 1/2` 发现 `Target.targetCreated` JSON 多余 `"}}` 缺陷) |
| 2026-11-23 06:10 | `page-network-response.spec.ts` | 16/21 PASS (Phase 400! `allHeaders`+`set-cookie`过! 初轮 17/21) | PW Batch 132 |
| 2026-11-23 06:10 | `page-wait-for-function.spec.ts` | 9/26 PASS (2 fail, 15 timeout) | PW Batch 132 |
| 2026-11-23 06:10 | `page-wait-for-load-state.spec.ts` | 6/10 PASS (2 fail, 2 timeout) | PW Batch 132 |
| 2026-11-23 06:10 | `page-wait-for-navigation.spec.ts` | 7/12 PASS (0 fail, 5 timeout) | PW Batch 132 |
| 2026-11-23 07:25 | `page-evaluate.spec.ts` | 37/95 PASS (0 FAIL, `reload` 触发 58 timeout) | PW Batch 133 |
| 2026-11-23 07:25 | `page-check.spec.ts` | 16/16 PASS | PW Batch 133 (`bin_dev` 全绿!) |
| 2026-11-23 07:25 | `page-emulate-media.spec.ts` | 12/12 PASS | PW Batch 133 (`bin_dev` 全绿!) |
| 2026-11-23 07:25 | `page-set-content.spec.ts` | 14/16 PASS (0 FAIL, 2 timeout) | PW Batch 133 |
| WPT-B126+127 | `/url/ 1/2` (Phase 403 无JSON崩溃!) + `/html/dom/documents/` + `/css/css-values/ 1/2..2/2` | 111/401 PASS (`/html/dom/documents/` **23/32**, `css-values` **84/350**, **4 FAIL→PASS**, 0 CRASH) |
| WPT-B128 | `/css/selectors/ 1/2..2/2` + `/css/css-cascade/` + `/css/css-sizing/ 1/2` | 67/173 PASS (`selectors` **31/67** **4 FAIL→PASS**, `css-cascade` **36/90** **1 FAIL→PASS**, 0 CRASH) |
| WPT-B129 | `/css/css-flexbox/ 1/4..2/4` + `/css/css-grid/ 1/4` + `/css/css-display/` | 126/404 PASS (`flexbox` **56/131**, `css-grid 1/4` **55/221** **12 FAIL→PASS**, `css-display` **15/52** **2 FAIL→PASS**, 0 CRASH) |
| 2026-11-23 08:17 | `page-keyboard.spec.ts` | 39/44 PASS (Phase 405! copy/cut-paste过! 2 fail, 3 timeout) | PW Batch 134 |
| 2026-11-23 08:17 | `page-wait-for-function.spec.ts` | 9/26 PASS (1 fail, 16 timeout) | PW Batch 134 |
| 2026-11-23 08:17 | `locator-convenience.spec.ts` | 0/21 PASS (21 timeout) | PW Batch 134 |
| 2026-11-23 08:17 | `page-wait-for-selector-2.spec.ts` | 32/32 PASS | PW Batch 134 (`bin_dev` 全绿!) |
| WPT-B130 | `/css/css-flexbox/ 3/4..4/4` + `/css/css-grid/ 2/4..3/4` | 170/570 PASS (`flexbox` **58/130**, `css-grid` **112/440** **20 FAIL→PASS**, 0 CRASH) |
| WPT-B131 | `/css/css-grid/ 4/4` + `/css/css-position/` + `/html/dom/documents/` + `/css/css-sizing/ 2/2` | 139/521 PASS (`grid 4/4` 37/209 **11 FAIL→PASS**, `position` **77/264** **5 FAIL→PASS**, `documents` **24/32**, 0 CRASH) |
| WPT-B126 | `/WebCryptoAPI/ 2/2` + `/FileAPI/` + `/encoding/ 2/2` | 46/670 PASS (`WebCryptoAPI 2/2` 12/66 **11 FAIL→PASS**, `FileAPI` 14/36, `encoding 2/2` 20/568, 0 CRASH) |
| WPT-B132 | `/css/css-backgrounds/ 1/2..2/2` + `/css/css-color/` + `/css/css-box/` | 48/204 PASS (`backgrounds` 33/115, `css-color` 2/56, `css-box` 13/33, 0 CRASH) |
| WPT-B133 | `/html/dom/documents/` + `/url/ 1/2` + `/dom/nodes/ 1/4..2/4` | 106/177 PASS (`documents` 24/32, `url 1/2` 4/19, `nodes 1/4` 38/63, `nodes 2/4` 40/63, 0 CRASH) |
| WPT-B134 | `/css/css-display/` + `/css/css-position/` + `/css/css-overflow/` + `/css/css-text/` | 127/477 PASS (`display` 15/52 **2 FAIL→PASS**, `position` 82/267 **5 FAIL→PASS**, `overflow` 6/33 **4 FAIL→PASS**, `text` 24/125 **9 FAIL→PASS**, 0 CRASH) |
| WPT-B136 | `/css/css-transforms/` + `/css/css-variables/` + `/css/css-ui/` + `/css/css-align/` (89/119) | 84/391 PASS (`transforms` 2/31, `variables` 60/224, `css-ui` 8/47 **2 FAIL→PASS**, `align` 14/89, 0 CRASH) |
| WPT-B135 | `/html/dom/documents/` + `/dom/nodes/ 3/4..4/4` + `/dom/events/` | 120/286 PASS (`documents` 24/32, `nodes 3/4` 24/63, `nodes 4/4` 26/62 **1 FAIL→PASS**, `dom/events` 46/129 **1 FAIL→PASS**, 0 CRASH) |
| WPT-B137 | `/css/css-fonts/` (89/121) + `/css/css-images/` + `/css/css-lists/` + `/html/syntax/` | 50/315 PASS (`fonts` 23/89, `images` 3/26 **1 FAIL→PASS**, `lists` 22/182 **2 FAIL→PASS**, `html/syntax` 2/18, 0 CRASH) |
| WPT-B138 | `/dom/ranges/` (29/36) + `/dom/traversal/` + `/dom/collections/` + `/url/ 2/2` | 21/75 PASS (`ranges` 8/29, `traversal` 5/17, `collections` 6/11, `url 2/2` 2/18, 0 CRASH) |
| WPT-B140 | `/html/webappapis/microtask-queuing/` + `/dom/abort/` + `/html/webappapis/scripting/` | 17/31 PASS (`microtask` 1/3, `abort` 4/7, `scripting` 12/21, 0 CRASH) |
| WPT-B139 | `/html/semantics/document-metadata/` + `/the-img-element/` + `/encoding/ 1/2` (116/633) + `/html/webappapis/timers/` | 20/159 PASS (`metadata` 15/28, `img` 1/5, `encoding 1/2` 0/116, `timers` 4/10, 0 CRASH) |
| WPT-B141 | `/html/dom/documents/` + `/dom/events/` + `/html/webappapis/atob/` + `/html/webappapis/structured-clone/` | 78/164 PASS (`documents` **28/32**, `dom/events` **49/129** **1 FAIL→PASS**, `atob` 0/1, `structured-clone` 1/2, 0 CRASH) |
| WPT-B142 | `/html/semantics/tabular-data/` + `/html/semantics/selectors/` + `/html/browsers/history/` + `/the-form-element/` | 26/48 PASS (`tabular-data` 1/11, `selectors` **3/4 (4/4匹配)**, `history` **11/18**, `the-form-element` **11/15**, 0 CRASH) |
| WPT-B143 | `/html/semantics/scripting-1/` + `/text-level-semantics/` + `/grouping-content/` | 34/108 PASS (`scripting-1` 3/58 **2 FAIL→PASS**, `text-level` **18/23**, `grouping` 13/27, 0 CRASH) |
| WPT-B144 | `/html/semantics/document-metadata/` + `/timers/` + `/fetch/api/headers/` + `/fetch/api/basic/` | 26/81 PASS (`metadata` **17/28**, `timers` 4/10, `headers` 2/12, `fetch/basic` 3/31 **1 FAIL→PASS**, 0 CRASH) |
| WPT-B145 | `/html/semantics/tabular-data/` + `/the-input-element/` + `/the-select-element/` (25/54) + `/the-textarea-element/` | 114/156 PASS (`tabular-data` **7/11**, `input` **84/103**, `select` 9/25, `textarea` **14/17**, 0 CRASH) |
| WPT-B146 | `/fetch/api/request/` + `/fetch/api/response/` + `/html/dom/elements/` (189/195) + `/css/cssom/` | 163/364 PASS (`request` 2/25, `response` 2/28, `dom/elements` **102/189**, `cssom` **57/122** **1 FAIL→PASS**, 0 CRASH) |
| WPT-B147 | `/html/browsers/history/` + `/the-form-element/` + `/the-button-element/` + `/the-label-element/` | 37/56 PASS (`history` **12/18 (17/18匹配)**, `form` 11/15, `button` **13/16**, `label` 1/7, 2 CRASH) |
| WPT-B149 | `dataset` + `/tabular-data/` + `/the-form-element/` + `/dom/ranges/` (29/36) | 33/76 PASS (`dataset` **6/7**, `tabular-data` 7/11, `the-form-element` 11/15, `ranges` 9/29, 0 CRASH) |
| WPT-B148 | `/css-conditional/` + `/cssom-view/` (89/106) + `/css-flexbox/` (78/261) + `/css/selectors/` | 131/369 PASS (`conditional` 39/135, `cssom-view` 40/89, `flexbox` 21/78, `selectors` 31/67 **4 FAIL→PASS**, 0 CRASH) |
| WPT-B150 | `/html/browsers/history/` + `cellIndex` | 14/19 PASS (`history` **13/18 (18/18基线全匹配!)**, `cellIndex` **1/1**, 0 FAIL, 0 CRASH) |
| 2026-11-23 10:24 | `page-history.spec.ts` | 10/13 PASS (`reload data url` FAIL→PASS; 3 timeout) | PW Batch 136 |
| 2026-11-23 10:24 | `page-network-response.spec.ts` | **18/21 PASS** (**8 FAIL→PASS** 含 Brotli/set-cookie/navigated away; 2 fail, 1 timeout) | PW Batch 136 |
| 2026-11-23 10:24 | `page-network-idle.spec.ts` | 2/10 PASS (8 timeout) | PW Batch 136 |
| 2026-11-23 10:24 | `page-event-popup.spec.ts` | 11/13 PASS (2 timeout) | PW Batch 136 |
| WPT-B152 | `the-iframe-element/` (89/152) + `the-canvas-element/` + `/streams/ 1/2` + `/webstorage/` | 26/135 PASS (`iframe` 21/89 **1 ERROR→PASS**, `canvas` **1/1**, `streams 1/2` 1/42, `webstorage` **3/3**, 0 CRASH) |
| WPT-B153 | `/console/` + `/performance-timeline/` + `/user-timing/` + `/FileAPI/` | 35/106 PASS (`console` **4/13 (13/13匹配!)**, `perf-timeline` 2/21, `user-timing` 15/36 **1 FAIL→PASS**, `FileAPI` 14/36, 0 CRASH) |
| WPT-B151 | `/css/css-cascade/` + `/css-sizing/ 1/2` + `/css-values/ 1/2` (89/175) + `/css-grid/ 1/4` (213/221) | 103/408 PASS (`cascade` 36/90 **+1 FAIL→PASS**, `sizing` 0/16, `values` 17/89, `grid 1/4` 50/213 **+9 FAIL→PASS**, 0 CRASH) |
| 2026-11-23 09:56 | `page-evaluate.spec.ts` | 36/95 PASS (59 timeout at teardown) | PW Batch 135 |
| 2026-11-23 09:56 | `page-keyboard.spec.ts` | 39/44 PASS (40/44 on retry; 2 fail, 3 timeout) | PW Batch 135 |
| 2026-11-23 09:56 | `page-aria-snapshot.spec.ts` | 37/38 PASS (1 fail: pseudo codepoint space) | PW Batch 135 |
| 2026-11-23 09:56 | `locator-convenience.spec.ts` | 0/21 PASS (21 timeout at teardown) | PW Batch 135 |
| WPT-B154 | `dataset` + `/streams/ 1/2` + `/WebCryptoAPI/ 1/2..2/2` | 29/181 PASS (`dataset` **7/7全绿!**, `streams 1/2` 1/42, `WebCryptoAPI 1/2` 9/66 **9 FAIL→PASS**, `2/2` 12/66 **11 FAIL→PASS**, 0 CRASH) |
| WPT-B156 | `/streams/ 2/2` + `the-fieldset-element/` + `the-option-element/` | 8/57 PASS (`streams 2/2` 1/42, `fieldset` **6/8**, `option` 1/7, 0 CRASH) |
| WPT-B157 | `/performance-timeline/` + `/FileAPI/` + `document-metadata/` + `the-datalist-element/` | 39/88 PASS (`perf-timeline` **4/21 (19/21匹配!)**, `FileAPI` **15/36** (`Blob-bytes`过!), `metadata` 17/28, `datalist` **3/3全绿!**, 0 CRASH) |
| WPT-B155 | `/css-grid/ 2/4..4/4` (487/660) + `/css-values/ 2/2` | 151/662 PASS (`grid 2/4` 19/89 **+4 FAIL→PASS**, `3/4` 51/220 **+10 FAIL→PASS**, `4/4` 38/178 **+7 FAIL→PASS**, `values 2/2` 43/175 **+3 FAIL→PASS**, 0 CRASH) |
| WPT-B158 | `attributes-common-to-form-controls/` + `form-submission-0/` + `constraints/` + `resetting-a-form/` | 12/29 PASS (`form-submission-0` **5/5匹配**, `constraints` **8/12**, `resetting-a-form` 2/4, 0 CRASH) |
| 2026-11-23 10:56 | `page-screenshot.spec.ts` | 18/44 PASS (20 fail, 6 timeout) | PW Batch 137 |
| 2026-11-23 10:56 | `page-network-request.spec.ts` | 7/27 PASS (16 fail, 3 timeout, 1 skip) | PW Batch 137 |
| 2026-11-23 10:56 | `page-add-init-script.spec.ts` | **11/11 PASS** (`bin_dev` 全绿!) | PW Batch 137 |
| 2026-11-23 10:56 | `page-set-content.spec.ts` | 14/16 PASS (0 fail, 2 timeout) | PW Batch 137 |
| WPT-B160 | `the-output-element/` + `the-meter-element/` + `the-progress-element/` + `/performance-timeline/` | 7/29 PASS (`output` 1/2, `meter` 1/2, `progress` 0/4, `perf-timeline` **5/21 (20/21匹配!)**, 0 CRASH) |
| WPT-B159 | `/css-pseudo/` + `/css-contain/` + `/css-transitions/` + `/css-writing-modes/ 1/2` | 63/109 PASS (`pseudo` 2/10, `contain` **56/69** **+1 FAIL→PASS**, `transitions` 2/20, `writing-modes 1/2` 3/10 **+1 FAIL→PASS**, 2 CRASH) |
| WPT-B161 | `attributes-common-to-form-controls/` + `the-iframe-element/ 2/2` + `the-link-element/` + `the-option-element/` | 23/99 PASS (`form-controls` `7/8`匹配 `formaction`过!, `iframe 2/2` 16/76 **+1 ERROR→PASS**, `link` 5/8 `fetchpriority`过!, `option` 1/7, 0 CRASH) |
| WPT-B162 | `/css-writing-modes/ 2/2` + `/css-animations/` + `/css-shapes/` + `/css-tables/` | 38/156 PASS (`writing-modes 2/2` 2/10, `animations` 20/58, `shapes` 12/70, `tables` 4/18, 0 CRASH) |
| WPT-B163 | `/dom/abort/` + `/dom/collections/` + `/dom/traversal/` + `the-img-element/` | 17/40 PASS (`abort` 4/7, `collections` **7/11** (`domstringmap`过!), `traversal` 5/17, `img` 1/5, 0 CRASH) |
| WPT-B164 | `/css-masking/` (107/179) + `/css-scroll-snap/` + `/css-multicol/` + `/css-break/` | 173/356 PASS (`masking` 16/107 **+1 FAIL→PASS**, `scroll-snap` 0/8, `multicol` 6/32, `css-break` **151/209** **+2 FAIL→PASS**, 0 CRASH) |
| WPT-B165 | `the-link-element/` + `the-fieldset-element/` + `/dom/nodes/ 1/4..2/4` | 60/142 PASS (`link` 5/8, `fieldset` **7/8** (`validationMessage`过!), `nodes 1/4` 25/63 **+1 FAIL→PASS**, `nodes 2/4` 23/63 **+1 FAIL→PASS**, 0 CRASH) |

| WPT-B166 | `/css/css-text-decor/` + `/css/css-will-change/` + `/css/css-overscroll-behavior/` + `/css/css-logical/` | 12/104 PASS (`text-decor` 5/41, `will-change` 1/3, `overscroll` 0/2, `logical` 6/58 (2 timeout), 0 CRASH) |

| WPT-B168 | `the-title-element/` + `the-progress-element/` + `the-meter-element/` + `the-output-element/` | 6/11 PASS (`title` 1/3, `progress` **3/4** (`0/4→3/4`!), `meter` **2/2全绿!**, `output` 0/2, 0 CRASH) |

| WPT-B167 | `the-img-element/` + `/dom/nodes/ 3/4..4/4` + `/css/css-syntax/` | 58/154 PASS (`img` 2/5, `nodes 3/4` 25/63 (`getElementsByClassName-22`过!), `nodes 4/4` 26/62 (`Node-isConnected`过! 1 crash), `css-syntax` 5/24 (23/24匹配!)) |

| 2026-11-23 11:29 | `page-wait-for-navigation.spec.ts` | 7/12 PASS (5 timeout) | PW Batch 138 |
| 2026-11-23 11:29 | `page-wait-for-load-state.spec.ts` | 6/10 PASS (2 fail, 2 timeout) | PW Batch 138 |
| 2026-11-23 11:29 | `page-wait-for-url.spec.ts` | 6/9 PASS (3 timeout) | PW Batch 138 |
| 2026-11-23 11:29 | `page-fill.spec.ts` | 25/40 PASS (1 fail, 14 timeout at CDP connect) | PW Batch 138 |

| WPT-B170 | `the-innertext-and-outertext-properties/` + `the-embed-element/` + `the-object-element/` + `the-video-element/` | 4/21 PASS (`innertext` 4/20 (14 timeout), `object` 0/1 (1/1匹配), 0 CRASH) |

| WPT-B171 | `the-progress-element/` + `the-output-element/` + `the-title-element/` + `/dom/traversal/` | 14/26 PASS (`progress` **4/4全绿!**, `output` **2/2全绿!**, `title` **3/3全绿!**, `traversal` 5/17 (2 timeout), 0 CRASH) |

| WPT-B172 | `the-fieldset-element/` + `the-link-element/` + `the-base-element/` + `the-meta-element/` | 17/25 PASS (`fieldset` **7/8**, `link` **6/8** (`5/8→6/8`, `link-rellist`过!), `base` 2/3, `meta` 2/6, 0 CRASH) |

| WPT-B173 | `/dom/traversal/` + `the-option-element/` + `the-button-element/` + `the-select-element/` | 31/65 PASS (`traversal` **8/17 (`5/17→8/17`, 0 timeout!)**, `option` 1/7, `button` **13/16**, `select` 9/25 (29未跑), 0 CRASH) |

| WPT-B174 | `dynamic-getter` + `outertext-setter` + `Element-hasAttributes` + `Node-isSameNode` | 2/4 PASS (`outertext-setter` **PASS!**, `Element-hasAttributes` **PASS!**, `dynamic-getter` 6/7, `Node-isSameNode` 8/9, 0 CRASH) |

| WPT-B175 | `DocumentFragment-getElementById` + `/dom/collections/` + `/dom/abort/` + `/dom/lists/` | 12/24 PASS (`DocumentFragment-getElementById` **1/1 PASS!**, `collections` 7/11, `abort` 4/7, `lists` 0/5, 0 CRASH) |

| 2026-11-23 12:07 | `page-select-option.spec.ts` | 0/33 PASS (33 timeout at teardown/CDP connect) | PW Batch 139 |
| 2026-11-23 12:07 | `page-check.spec.ts` | **16/16 PASS** (全绿!) | PW Batch 139 |
| 2026-11-23 12:07 | `page-focus.spec.ts` | **7/7 PASS** (**+1 FAIL→PASS**, 全绿!) | PW Batch 139 |

| 2026-11-23 12:12 | `matchers.misc.spec.ts` | **2/2 PASS** (全绿! `123/123` PW清单全覆盖) | PW Batch 140 |
| 2026-11-23 12:12 | `page-mouse.spec.ts` | **14/15 PASS** (**+7 FAIL→PASS**, 1 timeout) | PW Batch 140 |

| WPT-B176 | `/css/css-color/` + `/css/css-variables/` + `/css/css-env/` + `/dom/ranges/ 1/2` | 67/307 PASS (`color` 2/56, `variables` 60/224, `env` 1/9, `ranges 1/2` 4/18 (8 timeout), 0 CRASH) |

| WPT-B177 | `/dom/lists/` + `dynamic-getter` + `Node-isSameNode` + `/dom/events/ 1/2` | 31/72 PASS (`lists` **3/5 (`0/5→3/5`!)**, `dynamic-getter` **1/1 PASS!**, `Node-isSameNode` **1/1 PASS!**, `events 1/2` 26/65 (44/65匹配), 0 CRASH) |

| WPT-B178 | `Range-commonAncestorContainer` + `/dom/events/ 2/2` + `/dom/ranges/ 2/2` + `the-canvas-element/` | 32/84 PASS (`Range-commonAncestorContainer` **1/1 PASS!**, `events 2/2` 23/64 (`Event-dispatch-throwing` **FAIL→PASS!**), `ranges 2/2` 7/18 (7 timeout), `canvas` **1/1 PASS!**, 0 CRASH) |

| WPT-B179 | `/dom/lists/` + `Event-isTrusted` + `the-label-element/` + `the-legend-element/` | 8/15 PASS (`lists` **4/5 (`3/5→4/5`!)**, `Event-isTrusted` **1/1 PASS!**, `label` 1/7, `legend` **2/2全绿!**, 0 CRASH) |

| WPT-B180 | `the-textarea-element/` + `the-form-element/` + `the-input-element/ 1/4` | 44/58 PASS (`textarea` **13/17**, `form` **11/15**, `input 1/4` **20/26**, 0 CRASH) |

| WPT-B181 | `the-input-element/ 2/4..4/4` + `StaticRange-constructor` | 60/78 PASS (`input 2/4` **19/26**, `3/4` **20/26**, `4/4` **20/25**, `StaticRange-constructor` **1/1 PASS!**, 0 CRASH) |

| WPT-B182 | `textarea/cloning-steps` + `input/clone` + `constraints/` + `resetting-a-form/` | 12/18 PASS (`textarea/cloning-steps` **1/1 PASS!**, `input/clone` **1/1 PASS!**, `constraints` 8/12, `resetting` 2/4, 0 CRASH) |

| WPT-B183 | `checkbox` + `email` + `form-elements-nameditem-01` + `form-submission-0/` | 2/8 PASS (`form-submission-0` 2/5 (4/5匹配), `checkbox`/`email`/`nameditem` 各余子测试待查, 0 CRASH) |

| WPT-B184 | `attributes-common-to-form-controls/` + `the-datalist-element/` + `the-img-element/` | 6/16 PASS (`datalist` **3/3全绿!**, `img` 2/5 (4/5匹配), `form-controls` 1/8 (`ReferenceError: tag` 3例待修), 0 CRASH) |

| WPT-B185 | `form-elements-nameditem-01` + `checkbox` + `email` + `/dom/collections/` | 8/14 PASS (`form-elements-nameditem-01` **1/1 PASS!**, `collections` 7/11, 0 CRASH, 0 TIMEOUT) |

| 2026-11-23 13:18 | `page-emulate-media.spec.ts` | **12/12 PASS** (**+9 FAIL→PASS**, 全绿!) | PW Batch 141 |
| 2026-11-23 13:18 | `page-set-input-files.spec.ts` | **13/19 PASS** (**+7 FAIL→PASS**, 6 fail) | PW Batch 141 |
| 2026-11-23 13:18 | `page-expose-function.spec.ts` | 0/18 PASS (18 timeout at teardown/CDP connect) | PW Batch 141 |
| 2026-11-23 13:18 | `page-click.spec.ts` | 37/77 PASS (4 fail, 36 timeout) | PW Batch 141 |

| WPT-B186 | `attributes-common-to-form-controls/` + `the-form-element/` + `the-base-element/` + `the-meta-element/` | 17/32 PASS (`form-controls` 7/8匹配, `the-form-element` **12/15 (`11/15→12/15`)**, `base` 2/3, `meta` 2/6, 0 CRASH) |

| WPT-B187 | `history/` + `documents/` + `dataset` + `/console/` | 52/70 PASS (`history` **18/18匹配**, `documents` 28/32, `dataset` **7/7全绿!**, `console` **13/13匹配**, 0 CRASH) |

| WPT-B188 | `the-base-element/` + `form-action-reflection*` + `dom-tree-accessors/` + `/performance-timeline/` | 34/51 PASS (`base` **3/3全绿!**, `form-action-reflection` **2/2全绿!**, `dom-tree-accessors` **24/25**, `perf-timeline` **20/21匹配!**, 0 CRASH) |

| WPT-B189 | `resource-metadata-management/` + `the-link-element/` + `the-fieldset-element/` + `the-form-element/` | 31/38 PASS (`metadata` 4/7, `link` 6/8, `fieldset` 7/8, `form` **14/15** (`0`FAIL, 1超时), 0 CRASH) |

| WPT-B190 | `dom-tree-accessors/` + `/performance-timeline/` + `/css/css-display/` + `/css/css-position/` | 128/367 PASS (`dom-tree-accessors` **25/25全绿!**, `perf-timeline` 20/21匹配, `css-display` 16/52 (**+2 FAIL→PASS**), `css-position` 82/269 (**+5 FAIL→PASS**), 0 CRASH) |

| 2026-11-23 13:52 | `page-screenshot.spec.ts` | 18/44 PASS (19 fail, 7 timeout) | PW Batch 142 |
| 2026-11-23 13:52 | `page-network-request.spec.ts` | 7/27 PASS (16 fail, 3 timeout, 1 skip) | PW Batch 142 |
| 2026-11-23 13:52 | `page-network-response.spec.ts` | **18/21 PASS** (2 fail, 1 timeout) | PW Batch 142 |
| 2026-11-23 13:52 | `page-aria-snapshot.spec.ts` | **37/38 PASS** (仅1例伪元素空格待修) | PW Batch 142 |

| WPT-B191 | `resource-metadata-management/` + `/css/css-sizing/ 1/2..2/2` + `/css/css-overflow/ 1/2` | 10/56 PASS (`metadata` **5/7 (`4/7→5/7`, `readyState`过!)**, `sizing` 1/32, `overflow 1/2` 4/17 (**+3 FAIL→PASS**), 0 CRASH) |

| WPT-B192 | `the-button-element/` + `/css/css-overflow/ 2/2` + `/css/css-fonts/ 1/2..2/2` | 44/153 PASS (`button` 13/16, `overflow 2/2` 2/16 (**+1 FAIL→PASS**), `fonts 1/2` 16/61, `fonts 2/2` 13/60, 0 CRASH) |

| WPT-B193 | `/css/css-backgrounds/ 1/4..4/4` | 33/115 PASS (`1/4` 9/29, `2/4` 8/29, `3/4` 7/29, `4/4` 9/28, 44/115匹配, 0 CRASH) |

| WPT-B194 | `the-button-element/` + `the-label-element/` + `resource-metadata-management/` + `the-input-element/ 1/4` | 41/56 PASS (`button` **14/16 (`button-labels`过!)**, `input 1/4` **22/26 (`20/26→22/26`)**, `metadata` 5/7, `label` 0/7 (2 crash)) |

| WPT-B195 | `labelable-elements` + `clicking-interactive-content` + `/css/css-text/ 1/4..2/4` | 15/65 PASS (`labelable-elements` **1/1 PASS!**, `css-text 1/4` 7/32 (**+1 FAIL→PASS**), `2/4` 7/31 (**+3 FAIL→PASS**), 0超时, 0 CRASH) |

| WPT-B196 | `button-commandfor` + `input/files` + `/css/css-text/ 3/4..4/4` | 11/64 PASS (`button-commandfor` **1/1 PASS!**, `css-text 3/4` 4/31 (**+1 FAIL→PASS**), `4/4` 6/31 (**+4 FAIL→PASS**), 0超时, 0 CRASH) |

| WPT-B197 | `/css/css-borders/ 1/2..2/2` + `/css/css-box/ 1/2..2/2` | 73/133 PASS (`borders 1/2` 30/50 (**+6 FAIL→PASS**), `2/2` 30/50 (**+7 FAIL→PASS**), `box` 13/33, 0 CRASH) |

| WPT-B198 | `clicking-interactive-content` + `input/files` + `/css/css-transforms/ 1/4..2/4` | 3/18 PASS (`clicking-interactive-content` **1/1 PASS!**, `transforms 1/4..2/4` 2/16, 0超时, 0 CRASH) |

| WPT-B199 | `/css/css-transforms/ 3/4..4/4` + `/css/css-multicol/` + `/css/css-break/ 1/4` | 46/100 PASS (`transforms` 0/15, `multicol` 6/32, `css-break 1/4` **40/53 (`+1 FAIL→PASS`)**, 0超时, 0 CRASH) |

| WPT-B200 | `/css/css-break/ 2/4..4/4` + `/css/css-box/ 1/2` | 118/173 PASS (`break 2/4` 38/52 (**+1 FAIL→PASS**), `3/4` 38/52, `4/4` 35/52, `box 1/2` 7/17 (`float-invalid`过!), 0 CRASH) |

| WPT-B201 | `/css/css-box/ 2/2` + `/css/css-text/ 1/4..2/4` + `/css/css-borders/ 1/2` | 58/129 PASS (`box 2/2` **9/16 (`7→9`)**, `text 1/4` **9/32 (`7→9`)**, `text 2/4` **10/31 (`7→10`, +3 FAIL→PASS)**, `borders 1/2` 30/50 (+6 FAIL→PASS), 0超时, 0 CRASH) |

| WPT-B202 | `/css/css-text/ 3/4..4/4` + `/css/css-break/ 1/4` + `/css/css-box/ 1/2` | 70/132 PASS (`text 3/4` 5/31, `4/4` **12/31 (`6→12`, +4 FAIL→PASS!)**, `break 1/4` **41/53 (+1 FAIL→PASS)**, `box 1/2` **12/17 (`7→12`!)**, 0超时, 0 CRASH) |

### WPT-B203 (port=19700, 172 tests, 143/172 matched)
- `/css/css-box/ --shard 2/2`: 16 total | 10 PASS | 6 FAIL | 0 TIMEOUT | 0 CRASH (`PASS→FAIL`: `float-computed`, `margin-invalid`, `padding-invalid`, `padding-valid`)
- `/css/css-break/ --shard 2/4`: 52 total | 39 PASS | 12 FAIL | 1 TIMEOUT | 0 CRASH (`FAIL→PASS`: `orphans-valid`; `PASS→FAIL`: 8; `PASS→TIMEOUT`: `special-elements-crash`)
- `/css/css-break/ --shard 3/4`: 52 total | 39 PASS | 13 FAIL | 0 TIMEOUT | 0 CRASH (`PASS→FAIL`: 7)
- `/css/css-break/ --shard 4/4`: 52 total | 37 PASS | 15 FAIL | 0 TIMEOUT | 0 CRASH (`PASS→FAIL`: 8)
- Totals: **125 PASS / 46 FAIL / 1 TIMEOUT / 0 CRASH** (27 `PASS→FAIL`, 1 `PASS→TIMEOUT`, 1 `FAIL→PASS`)

### WPT-B204 (port=19700, 104 tests, 39/104 matched)
- `/css/css-cascade/ --shard 1/2`: 45 total | 20 PASS | 24 FAIL | 0 TIMEOUT | 1 ERROR (`all-prop-revert-layer.html`)
- `/css/css-cascade/ --shard 2/2`: 45 total | 16 PASS | 27 FAIL | 2 TIMEOUT | 0 CRASH (`FAIL→PASS`: `layer-counter-style-override.html`)
- `/css/css-color/ --shard 1/4`: 14 total | 1 PASS | 10 FAIL | 3 TIMEOUT | 0 CRASH
- Totals: **37 PASS / 61 FAIL / 5 TIMEOUT / 1 ERROR**

### WPT-B205 (port=19700, 127 tests, 64/127 matched)
- `/css/css-box/parsing/`: 29 total | 19 PASS | 10 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-break/parsing/`: 18 total | 12 PASS | 6 FAIL | 0 TIMEOUT | 0 CRASH (`FAIL→PASS`: `orphans-valid`, `widows-valid`)
- `/css/css-backgrounds/parsing/ --shard 1/2`: 37 total | 7 PASS | 30 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-text/parsing/ --shard 1/2`: 43 total | 13 PASS | 30 FAIL | 0 TIMEOUT | 0 CRASH (`FAIL→PASS`: `text-align-last-valid`)
- Totals: **51 PASS / 76 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B206 (port=19700, 94 tests, 43/94 matched)
- `/css/css-flexbox/parsing/`: 27 total | 11 PASS | 16 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-position/parsing/`: 22 total | 15 PASS | 7 FAIL | 0 TIMEOUT | 0 CRASH (`position` & `z-index` 100% PASS, `top/right/bottom/left` valid/invalid PASS)
- `/css/css-transforms/parsing/ --shard 1/2`: 14 total | 1 PASS | 13 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-multicol/parsing/`: 31 total | 9 PASS | 22 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **36 PASS / 58 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B207 (port=19700, 49 tests, 23/49 matched)
- `/css/css-animations/parsing/`: 35 total | 17 PASS | 18 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-transitions/parsing/`: 14 total | 5 PASS | 9 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **22 PASS / 27 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B208 (port=19700, 69 tests, 55/69 matched)
- `/css/css-box/parsing/`: 29 total | 22 PASS | 7 FAIL | 0 TIMEOUT | 0 CRASH (`clear-computed`, `float-computed`, `visibility-computed` recovered!)
- `/css/css-break/parsing/`: 18 total | 16 PASS | 2 FAIL | 0 TIMEOUT | 0 CRASH (0 unexpected failures! `FAIL→PASS`: `orphans-valid`, `widows-valid`; all `*-computed` recovered!)
- `/css/css-position/parsing/`: 22 total | 15 PASS | 7 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **53 PASS / 16 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B209 (port=19700, 94 tests, 42/94 matched)
- `/css/css-backgrounds/parsing/ --shard 2/2`: 37 total | 10 PASS | 27 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-text/parsing/ --shard 2/2`: 43 total | 23 PASS | 20 FAIL | 0 TIMEOUT | 0 CRASH (`FAIL→PASS`: 7 — `hanging-punctuation-valid`, `text-align-all-valid`, `text-align-valid`, `text-fit-valid`, `text-group-align-valid`, `text-spacing-trim-valid`, `word-space-transform-valid`)
- `/css/css-transforms/parsing/ --shard 2/2`: 14 total | 3 PASS | 11 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **36 PASS / 58 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B210 (port=19700, 113 tests, 33/113 matched)
- `/css/css-fonts/parsing/`: 82 total | 18 PASS | 62 FAIL | 2 TIMEOUT | 0 CRASH
- `/css/css-grid/parsing/ --shard 1/2`: 31 total | 1 PASS | 29 FAIL | 1 TIMEOUT | 0 CRASH (`FAIL→PASS`: `grid-template-rows-repeat-intrinsic-valid.tentative.html`)
- Totals: **19 PASS / 91 FAIL / 3 TIMEOUT / 0 CRASH**

### PW Batch 143 (port=19386, 195 tests)
- `page-click.spec.ts`: 77 total | 39 PASS | 2 FAIL | 36 TIMEOUT
- `page-keyboard.spec.ts`: 44 total | 38 PASS | 3 FAIL | 3 TIMEOUT
- `page-history.spec.ts`: 13 total | 10 PASS | 0 FAIL | 3 TIMEOUT
- `page-goto.spec.ts`: 61 total | 41 PASS | 10 FAIL | 10 TIMEOUT
- Totals: **128 PASS / 15 FAIL / 52 TIMEOUT / 0 SKIP**

### WPT-B211 (port=19700, Phase 469 verification, 63 tests, 37/63 matched)
- `/css/css-flexbox/parsing/`: 27 total | 16 PASS (up from 11!) | 11 FAIL | 0 TIMEOUT | 0 CRASH (`flex-direction-*`, `flex-wrap-*`, `order-*`, `flex-invalid`, `flex-flow-invalid` PASS)
- `/css/css-position/parsing/`: 22 total | 16 PASS (up from 15!) | 6 FAIL | 0 TIMEOUT | 0 CRASH (`inset-invalid` PASS)
- `/css/css-transforms/parsing/ --shard 1/2`: 14 total | 4 PASS (up from 1!) | 10 FAIL | 0 TIMEOUT | 0 CRASH (`transform-box-invalid`, `backface-visibility-computed`, `rotate-parsing-invalid` PASS)
- Totals: **36 PASS / 27 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B212 (port=19700, 80 tests, 24/80 matched)
- `/css/css-grid/parsing/ --shard 2/2`: 30 total | 3 PASS | 26 FAIL | 1 TIMEOUT | 0 CRASH (`FAIL→PASS`: `grid-template-columns-repeat-intrinsic-valid.tentative.html`)
- `/css/css-align/parsing/`: 50 total | 16 PASS | 34 FAIL | 0 TIMEOUT | 0 CRASH (all `*-gap-invalid` + `gap-valid/shorthand` + `align/justify-computed` PASS)
- Totals: **19 PASS / 60 FAIL / 1 TIMEOUT / 0 CRASH**

### WPT-B213 (port=19700, 96 tests, 47/96 matched)
- `/css/css-fonts/parsing/`: 82 total | 29 PASS (up from 18!) | 51 FAIL | 2 TIMEOUT | 0 CRASH
- `/css/css-transforms/parsing/ --shard 2/2`: 14 total | 7 PASS (up from 3!) | 7 FAIL | 0 TIMEOUT | 0 CRASH (`perspective-invalid`, `perspective-origin-invalid`, `translate-parsing-invalid`, `transform-origin-invalid` PASS)
- Totals: **36 PASS / 58 FAIL / 2 TIMEOUT / 0 CRASH**

### WPT-B214 (port=19700, 56 tests, 15/56 matched)
- `/css/css-sizing/parsing/`: 19 total | 0 PASS | 4 FAIL | 15 TIMEOUT | 0 CRASH
- `/css/css-display/parsing/`: 5 total | 1 PASS | 4 FAIL | 0 TIMEOUT | 0 CRASH (`5/5 matched expectations`)
- `/css/css-overflow/parsing/`: 32 total | 8 PASS | 24 FAIL | 0 TIMEOUT | 0 CRASH (`FAIL→PASS`: `block-ellipsis-valid`, `continue-valid`, `scroll-markers-valid`, `scroll-target-group-valid`)
- Totals: **9 PASS / 32 FAIL / 15 TIMEOUT / 0 CRASH**

### WPT-B215 (port=19700, 73 tests, 35/73 matched)
- `/css/css-align/parsing/`: 50 total | 25 PASS (up from 16!) | 25 FAIL | 0 TIMEOUT | 0 CRASH (all `*-invalid` PASS!)
- `/css/css-lists/parsing/`: 23 total | 8 PASS | 15 FAIL | 0 TIMEOUT | 0 CRASH (`counter-*-invalid` & `list-style-position-*` PASS!)
- Totals: **33 PASS / 40 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B216 (port=19700, 70 tests, 43/70 matched)
- `/css/css-text-decor/parsing/`: 31 total | 9 PASS | 22 FAIL | 0 TIMEOUT | 0 CRASH (`text-decoration-style-*`, `text-decoration-skip-ink-*`, `text-decoration-color-valid/invalid` PASS)
- `/css/css-ui/parsing/`: 39 total | 19 PASS | 19 FAIL | 1 TIMEOUT | 0 CRASH (`box-sizing-*`, `resize-*`, `user-select-*`, `outline-style-valid/computed`, `outline-width/color/offset-invalid` PASS)
- Totals: **28 PASS / 41 FAIL / 1 TIMEOUT / 0 CRASH**

### WPT-B217 (port=19700, 63 tests, 29/63 matched)
- `/css/css-overflow/parsing/`: 32 total | 7 PASS | 25 FAIL | 0 TIMEOUT | 0 CRASH (`scrollbar-gutter-invalid`, `webkit-line-clamp-invalid`, `text-overflow-invalid` PASS!)
- `/css/css-text-decor/parsing/`: 31 total | 11 PASS (up from 9!) | 20 FAIL | 0 TIMEOUT | 0 CRASH (`text-decoration-line-invalid`, `text-underline-position-invalid` PASS!)
- Totals: **18 PASS / 45 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B218 (port=19700, 70 tests, 53/70 matched)
- `/css/css-ui/parsing/`: 39 total | 21 PASS (up from 19!) | 17 FAIL | 1 TIMEOUT | 0 CRASH (`field-sizing-invalid`, `outline-invalid` PASS!)
- `/css/css-multicol/parsing/`: 31 total | 16 PASS (up from 9!) | 15 FAIL | 0 TIMEOUT | 0 CRASH (`column-fill/rule/rule-style/rule-width/span-invalid`, `column-rule-style/color-computed` PASS!)
- Totals: **37 PASS / 32 FAIL / 1 TIMEOUT / 0 CRASH**

### WPT-B219 (port=19700, Phase 477 verification, 70 tests, 48/70 matched)
- `/css/css-text-decor/parsing/`: 31 total | 12 PASS (up from 11!) | 19 FAIL | 0 TIMEOUT | 0 CRASH (`text-emphasis-position-invalid` PASS!)
- `/css/css-ui/parsing/`: 39 total | 21 PASS | 17 FAIL | 1 TIMEOUT | 0 CRASH
- Totals: **33 PASS / 36 FAIL / 1 TIMEOUT / 0 CRASH**

### PW Batch 144 (port=19386, 168 tests)
- `page-evaluate.spec.ts`: 95 total | 37 PASS | 0 FAIL | 58 TIMEOUT
- `page-wait-for-selector-1.spec.ts`: 15 total | **15 PASS (100%)** | 0 FAIL | 0 TIMEOUT
- `page-wait-for-selector-2.spec.ts`: 32 total | **32 PASS (100%, +2 unexpected passes!)** | 0 FAIL | 0 TIMEOUT
- `page-wait-for-function.spec.ts`: 26 total | 5 PASS | 2 FAIL | 19 TIMEOUT
- Totals: **89 PASS / 2 FAIL / 77 TIMEOUT / 0 SKIP**

### WPT-B220 (port=19700, Phase 478 verification, 74 tests, 51/74 matched)
- `/css/css-text-decor/parsing/`: 31 total | 13 PASS (up from 12!) | 18 FAIL | 0 TIMEOUT | 0 CRASH (`text-decoration-invalid` PASS!)
- `/css/css-ui/parsing/`: 39 total | 22 PASS (up from 21!) | 16 FAIL | 1 TIMEOUT | 0 CRASH (`outline-style-invalid` PASS!)
- `/css/css-grid/parsing/grid-template-areas`: 4 total | 1 PASS (`grid-template-areas-invalid`) | 3 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **36 PASS / 37 FAIL / 1 TIMEOUT / 0 CRASH**

### WPT-B221 (port=19700, 26 tests, 13/26 matched)
- `/css/css-lists/parsing/`: 23 total | 12 PASS (up from 8!) | 11 FAIL | 0 TIMEOUT | 0 CRASH (8/8 `*-invalid.html` PASS!)
- `/css/css-grid/parsing/grid-auto-flow`: 3 total | 1 PASS (`grid-auto-flow-invalid`) | 2 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **13 PASS / 13 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B222 (port=19700, 33 tests, 2/33 matched)
- `/css/css-sizing/parsing/`: 19 total | 0 PASS | 4 FAIL | 15 TIMEOUT | 0 CRASH
- `/css/css-color/ --shard 2/4`: 14 total | 2 PASS | 8 FAIL | 4 TIMEOUT | 0 CRASH
- Totals: **2 PASS / 12 FAIL / 19 TIMEOUT / 0 CRASH**

### PW Batch 145 (port=19386, 77 tests)
- `page-network-request.spec.ts`: 27 total | 7 PASS | 16 FAIL | 3 TIMEOUT | 1 SKIP
- `page-network-response.spec.ts`: 21 total | 18 PASS | 2 FAIL | 1 TIMEOUT
- `page-add-init-script.spec.ts`: 11 total | **11 PASS (100%)** | 0 FAIL | 0 TIMEOUT
- `page-expose-function.spec.ts`: 18 total | 0 PASS | 0 FAIL | 18 TIMEOUT
- Totals: **36 PASS / 18 FAIL / 22 TIMEOUT / 1 SKIP**

### WPT-B223 (port=19700, Phase 483 verification, 55 tests, 49/55 matched)
- `/css/css-box/parsing/`: 29 total | 23 PASS (up from 22!) | 6 FAIL | 0 TIMEOUT | 0 CRASH (`margin-computed` PASS!)
- `/css/css-position/parsing/`: 22 total | **21 PASS (95.5%!)** | 1 FAIL | 0 TIMEOUT | 0 CRASH (`top/right/bottom/left-computed` & `inset-computed` PASS!)
- `/css/css-backgrounds/parsing/border-width`: 4 total | **3 PASS** | 1 FAIL | 0 TIMEOUT | 0 CRASH (`border-width-computed` PASS!)
- Totals: **47 PASS / 8 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B224 (port=19700, Phase 485 verification, 89 tests, 44/89 matched)
- `/css/css-fonts/parsing/font-weight`: 3 total | 1 PASS | 2 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-text/parsing/ --shard 1/2`: 43 total | **20 PASS** | 23 FAIL | 0 TIMEOUT | 0 CRASH (`FAIL→PASS`: 4 — `text-align-last-computed`, `text-align-last-valid`, `text-indent-computed`, `word-break-computed`)
- `/css/css-text/parsing/ --shard 2/2`: 43 total | **24 PASS** | 19 FAIL | 0 TIMEOUT | 0 CRASH (`FAIL→PASS`: 7)
- Totals: **45 PASS / 44 FAIL / 0 TIMEOUT / 0 CRASH**

### PW Batch 146 (port=19386, 31 tests)
- `page-set-content.spec.ts`: 16 total | **14 PASS** | 0 FAIL | 2 TIMEOUT
- `page-set-extra-http-headers.spec.ts`: 5 total | **4 PASS** | 1 FAIL | 0 TIMEOUT
- `page-wait-for-load-state.spec.ts`: 10 total | **6 PASS** | 2 FAIL | 2 TIMEOUT
- Totals: **24 PASS / 3 FAIL / 4 TIMEOUT / 0 SKIP**

### WPT-B225 (port=19700, Phase 486 verification, 28 tests, 14/28 matched)
- `/css/css-transforms/parsing/ --shard 1/2`: 14 total | 4 PASS | 10 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-transforms/parsing/ --shard 2/2`: 14 total | **9 PASS (up from 7!)** | 5 FAIL | 0 TIMEOUT | 0 CRASH (`rotate-parsing-computed`, `scale-parsing-invalid`, 7/7 non-prefixed `*-invalid` PASS!)
- Totals: **13 PASS / 15 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B226 (port=19700, 74 tests, 36/74 matched)
- `/css/css-backgrounds/parsing/ --shard 1/2`: 37 total | **12 PASS (up from 7!)** | 25 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-backgrounds/parsing/ --shard 2/2`: 37 total | **13 PASS (up from 10!)** | 24 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **25 PASS / 49 FAIL / 0 TIMEOUT / 0 CRASH**

### PW Batch 147 (port=19386, 34 tests)
- `page-set-input-files.spec.ts`: 19 total | **13 PASS (+7 unexpected passes!)** | 6 FAIL | 0 TIMEOUT
- `page-mouse.spec.ts`: 15 total | **14 PASS (+7 unexpected passes!)** | 0 FAIL | 1 TIMEOUT
- Totals: **27 PASS / 6 FAIL / 1 TIMEOUT / 0 SKIP**

### WPT-B227 (port=19700, 28 tests, 5/28 matched)
- `/css/css-color/ --shard 3/4`: 14 total | 2 PASS | 10 FAIL | 2 TIMEOUT | 0 CRASH
- `/css/css-color/ --shard 4/4`: 14 total | 2 PASS | 9 FAIL | 3 TIMEOUT | 0 CRASH
- Totals: **4 PASS / 19 FAIL / 5 TIMEOUT / 0 CRASH**

### WPT-B228 (port=19700, 224 tests, 77/224 matched)
- `/css/css-variables/ --shard 1/2`: 112 total | 31 PASS | 81 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-variables/ --shard 2/2`: 112 total | 29 PASS | 79 FAIL | 2 TIMEOUT | 2 ERROR
- Totals: **60 PASS / 160 FAIL / 2 TIMEOUT / 2 ERROR**

### WPT-B229 (port=19700, Phase 489 verification, 56 tests, 36/56 matched)
- `/css/css-backgrounds/parsing/border-radius`: 3 total | **2 PASS** (`border-radius-invalid`, `border-radius-computed`!) | 1 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-color/parsing/opacity`: 3 total | 1 PASS | 2 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-align/parsing/`: 50 total | **31 PASS (up from 25!)** | 19 FAIL | 0 TIMEOUT | 0 CRASH (`gap-*`, `grid-gap-*`, `*-gap-computed`, `justify-content-*` PASS!)
- Totals: **34 PASS / 22 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B230 (port=19700, Phase 488 verification, 28 tests, 16/28 matched)
- `/css/css-transforms/parsing/ --shard 1/2`: 14 total | **6 PASS (up from 4!)** | 8 FAIL | 0 TIMEOUT | 0 CRASH (`perspective-origin-computed`, `transform-origin-computed` PASS!)
- `/css/css-transforms/parsing/ --shard 2/2`: 14 total | **9 PASS** | 5 FAIL | 0 TIMEOUT | 0 CRASH (`9/10` non-prefixed `*-invalid` + `rotate-parsing-computed` PASS!)
- Totals: **15 PASS / 13 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B231 (port=19700, Phase 490 verification, 39 tests, 22/39 matched)
- `/css/css-flexbox/parsing/`: 27 total | **17 PASS (up from 16!)** | 10 FAIL | 0 TIMEOUT | 0 CRASH (`flex-flow-computed` PASS!)
- `/css/css-align/parsing/place-`: 12 total | 3 PASS | 9 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **20 PASS / 19 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B232 (port=19700, 93 tests, 43/93 matched)
- `/css/css-overflow/parsing/overflow`: 5 total | 1 PASS | 4 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-values/ --shard 1/4`: 88 total | 19 PASS | 60 FAIL | 9 TIMEOUT | 0 CRASH (`FAIL→PASS`: `random-in-keyframe.html`)
- Totals: **20 PASS / 64 FAIL / 9 TIMEOUT / 0 CRASH**

### PW Batch 148 (port=19386, 96 tests)
- `page-fill.spec.ts`: 40 total | 25 PASS | 1 FAIL | 14 TIMEOUT
- `page-focus.spec.ts`: 7 total | **6 PASS (+1 unexpected pass!)** | 1 FAIL | 0 TIMEOUT
- `page-select-option.spec.ts`: 33 total | 0 PASS | 0 FAIL | 33 TIMEOUT
- `page-check.spec.ts`: 16 total | **16 PASS (100%)** | 0 FAIL | 0 TIMEOUT
- Totals: **47 PASS / 2 FAIL / 47 TIMEOUT / 0 SKIP**

### WPT-B233 (port=19700, 175 tests, 95/175 matched)
- `/css/css-values/ --shard 2/4`: 88 total | 25 PASS (`FAIL→PASS`: `random-item-valid`, `various-values-important`) | 52 FAIL | 11 TIMEOUT | 0 CRASH
- `/css/css-values/ --shard 3/4`: 87 total | 25 PASS | 51 FAIL | 10 TIMEOUT | 1 ERROR
- Totals: **50 PASS / 103 FAIL / 21 TIMEOUT / 1 ERROR**

### PW Batch 149 (port=19386, 39 tests)
- `page-emulate-media.spec.ts`: 12 total | **12 PASS (100%, +9 unexpected passes!)** | 0 FAIL | 0 TIMEOUT
- `page-event-console.spec.ts`: 19 total | 3 PASS | 0 FAIL | 16 TIMEOUT
- `page-event-pageerror.spec.ts`: 7 total | 4 PASS | 3 FAIL | 0 TIMEOUT
- `page-event-load.spec.ts`: 1 total | **1 PASS (100%)** | 0 FAIL | 0 TIMEOUT
- Totals: **20 PASS / 3 FAIL / 16 TIMEOUT / 0 SKIP**

### WPT-B234 (port=19700, Phase 498 verification, 114 tests, 58/114 matched)
- `/css/css-values/ --shard 4/4`: 87 total | 16 PASS | 63 FAIL | 8 TIMEOUT | 0 CRASH
- `/css/css-flexbox/parsing/`: 27 total | **19 PASS (up from 17!)** | 8 FAIL | 0 TIMEOUT | 0 CRASH (`flex-direction-*`, `flex-grow-*`, `flex-shrink-*`, `flex-wrap-*`, `order-*` 100% PASS!)
- Totals: **35 PASS / 71 FAIL / 8 TIMEOUT / 0 CRASH**

### WPT-B235 (port=19700, Phase 500 verification, 100 tests, 75/100 matched)
- `/css/css-position/parsing/`: **22 / 22 PASS (100% ALL GREEN!)** | 0 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-align/parsing/`: **35 PASS (up from 31!)** | 15 FAIL | 0 TIMEOUT | 0 CRASH (`gap`/`row-gap`/`column-gap`/`grid-*-gap` & all `*-invalid` 100% PASS!)
- `/css/css-transforms/parsing/`: 28 total | 15 PASS | 13 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **72 PASS / 28 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B236 (port=19700, Phase 500 verification, 57 tests)
- `/css/css-lists/parsing/`: 23 total | 12 PASS | 11 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-text-decor/parsing/`: 31 total | **15 PASS (up from 13!)** | 16 FAIL | 0 TIMEOUT | 0 CRASH (`text-decoration-line-*`, `text-underline-position-valid` PASS!)
- `/css/css-grid/parsing/grid-auto-flow`: 3 total | 1 PASS | 2 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **28 PASS / 29 FAIL / 0 TIMEOUT / 0 CRASH**

### PW Batch 150 (port=19386, 78 tests)
- `page-listeners.spec.ts`: **3 / 3 PASS (100%)** | 0 FAIL | 0 TIMEOUT | 0 SKIP
- `page-dispatchevent.spec.ts`: **15 / 16 PASS** | 1 FAIL | 0 TIMEOUT | 0 SKIP
- `page-drag.spec.ts`: 0 PASS | 0 FAIL | 0 TIMEOUT | 15 SKIP
- `page-screenshot.spec.ts`: 18 PASS | 21 FAIL | 5 TIMEOUT | 0 SKIP
- Totals: **36 PASS / 22 FAIL / 5 TIMEOUT / 15 SKIP**

### WPT-B237 (port=19700, 117 tests)
- `/css/css-backgrounds/parsing/`: 74 total | 8 PASS | 66 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-color/parsing/`: 43 total | 1 PASS | 34 FAIL | 8 TIMEOUT | 0 CRASH
- Totals: **9 PASS / 100 FAIL / 8 TIMEOUT / 0 CRASH**

### PW Targeted Verification (Phase 504, port=19388, 7 tests)
- `page-event-pageerror.spec.ts`: **6 / 7 PASS (+4 unexpected passes vs baseline)** | 1 FAIL (`should fire` stack col/eval) | 0 TIMEOUT | 0 SKIP

### WPT-B238 (port=19700, Phase 504 verification, 85 tests)
- `/css/css-lists/parsing/`: 23 total | 12 PASS | 11 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-text-decor/parsing/`: 31 total | 15 PASS | 16 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-grid/parsing/ --shard 1/2`: 31 total | 3 PASS (`FAIL→PASS`: `grid-template-rows-repeat-intrinsic-valid.tentative.html`) | 27 FAIL | 1 TIMEOUT | 0 CRASH
- Totals: **30 PASS / 54 FAIL / 1 TIMEOUT / 0 CRASH**

### WPT-B239 (port=19700, Phase 505 verification, 54 tests)
- `/css/css-lists/parsing/`: 23 total | **13 PASS (up from 12, `list-style-type-valid` green!)** | 10 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-text-decor/parsing/`: 31 total | **18 PASS (up from 15, `text-emphasis-position-*` & `text-underline-position-computed` green!)** | 13 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **31 PASS / 23 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B240 (port=19700, Phase 506 verification, 81 tests)
- `/css/css-text-decor/parsing/`: 31 total | **22 PASS (up from 18, `text-decoration-skip-spaces-*` & `inset-invalid` green!)** | 9 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-align/parsing/`: 50 total | 35 PASS | 15 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **57 PASS / 24 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B241 (port=19700, default ./bin/main baseline, 60 tests)
- `/css/css-multicol/parsing/`: 31 total | 6 PASS | 25 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-box/parsing/`: 29 total | 11 PASS | 18 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **17 PASS / 43 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B242 (port=19700, Phase 507 verification, 54 tests)
- `/css/css-lists/parsing/`: 23 total | 13 PASS | 10 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-text-decor/parsing/`: 31 total | 22 PASS | 9 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **35 PASS / 19 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B243 (port=19700, Phase 508 verification, 77 tests)
- `/css/css-align/parsing/`: 50 total | 35 PASS | 15 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-flexbox/parsing/`: 27 total | 19 PASS | 8 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **54 PASS / 23 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B244 (port=19700, Phase 509 verification, 131 tests)
- `/css/css-lists/parsing/`: 23 total | **14 PASS (`list-style-shorthand.sub` green!)** | 9 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-text-decor/parsing/`: 31 total | **24 PASS (`text-decoration-shorthand` & `text-emphasis-style-computed` green!)** | 7 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-align/parsing/`: 50 total | 35 PASS | 15 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-flexbox/parsing/`: 27 total | **21 PASS (`flex-flow-shorthand` & `flex-flow-valid` green!)** | 6 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **94 PASS / 37 FAIL / 0 TIMEOUT / 0 CRASH**

### PW Batch 151 (port=19386, 56 tests)
- `page-wait-for-load-state.spec.ts`: 10 total | 6 PASS | 2 FAIL | 2 TIMEOUT
- `page-wait-for-function.spec.ts`: 26 total | 4 PASS | 3 FAIL | 19 TIMEOUT
- `page-wait-for-navigation.spec.ts`: 12 total | 7 PASS | 0 FAIL | 5 TIMEOUT
- `page-wait-for-request.spec.ts`: 8 total | 7 PASS | 0 FAIL | 1 TIMEOUT
- Totals: **24 PASS / 5 FAIL / 27 TIMEOUT / 0 SKIP**

### WPT-B245 (port=19700, Phase 509 verification, 60 tests)
- `/css/css-box/parsing/`: 29 total | **23 PASS (up from 22!)** | 6 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-multicol/parsing/`: 31 total | **18 PASS (up from 9!)** | 13 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **41 PASS / 19 FAIL / 0 TIMEOUT / 0 CRASH**

### PW Batch 152 (port=19386, Phase 509 verification, 76 tests)
- `page-event-pageerror.spec.ts`: **6 / 7 PASS** | 1 FAIL | 0 TIMEOUT | 0 SKIP
- `page-set-input-files.spec.ts`: **13 / 19 PASS** | 6 FAIL | 0 TIMEOUT | 0 SKIP
- `page-aria-snapshot.spec.ts`: **37 / 38 PASS** | 1 FAIL (`should include pseudo codepoints`) | 0 TIMEOUT | 0 SKIP
- `page-emulate-media.spec.ts`: **12 / 12 PASS (100%)** | 0 FAIL | 0 TIMEOUT | 0 SKIP
- Totals: **68 PASS / 8 FAIL / 0 TIMEOUT / 0 SKIP**

### WPT-B246 (port=19700, Phase 510 verification, 81 tests)
- `/css/css-flexbox/parsing/`: 27 total | 21 PASS (`flex-shorthand` 36/40, `flex-computed` 9/14) | 6 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-text-decor/parsing/`: 31 total | 24 PASS (`text-emphasis-style-computed-vertical-lr` 7/9) | 7 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-lists/parsing/`: 23 total | 14 PASS (`counter-reset-valid` 15/16, `counter-{increment,set}-valid` 9/10) | 9 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **59 PASS / 22 FAIL / 0 TIMEOUT / 0 CRASH**

### PW Batch 153 (port=19386, Phase 510 verification, 40 tests)
- `page-add-init-script.spec.ts`: **11 / 11 PASS (100%, +2 unexpected passes!)** | 0 FAIL | 0 TIMEOUT | 0 SKIP
- `page-add-script-tag.spec.ts`: **9 / 9 PASS (100%, +1 unexpected pass!)** | 0 FAIL | 0 TIMEOUT | 0 SKIP
- `page-add-style-tag.spec.ts`: **4 / 4 PASS (100%, +1 unexpected pass!)** | 0 FAIL | 0 TIMEOUT | 0 SKIP
- `page-basic.spec.ts`: **12 / 16 PASS (+1 unexpected pass!)** | 4 FAIL | 0 TIMEOUT | 0 SKIP
- Totals: **36 PASS / 4 FAIL / 0 TIMEOUT / 0 SKIP**

### WPT-B247 (port=19700, Phase 510 verification, 49 tests)
- `/css/css-transitions/parsing/`: 14 total | 5 PASS | 9 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-animations/parsing/`: 35 total | 17 PASS | 18 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **22 PASS / 27 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B248 (port=19700, Phase 510 verification, 28 tests)
- `/css/css-transforms/parsing/`: 28 total | 15 PASS | 13 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **15 PASS / 13 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B249 (port=19700, Phase 511 verification, 50 tests)
- `/css/css-lists/parsing/`: 23 total | **17 PASS (up from 14, `counter-{reset,increment,set}-valid` green!)** | 6 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-flexbox/parsing/`: 27 total | **22 PASS (up from 21, `flex-shorthand` 40/40 green!)** | 5 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **39 PASS / 11 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B250 (port=19700, Phase 511 verification, 51 tests)
- `/css/css-overflow/parsing/`: 32 total | 11 PASS (`FAIL→PASS`: `scroll-markers-valid`, `scroll-target-group-valid`) | 21 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-sizing/parsing/`: 19 total | 0 PASS | 4 FAIL | 15 TIMEOUT | 0 CRASH
- Totals: **11 PASS / 25 FAIL / 15 TIMEOUT / 0 CRASH**

### WPT-B251 (port=19700, Phase 512 verification, 58 tests)
- `/css/css-text-decor/parsing/`: 31 total | **25 PASS (up from 24, `text-decoration-valid` green!)** | 6 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-flexbox/parsing/`: 27 total | **23 PASS (up from 22, `flex-basis-invalid` green!)** | 4 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **48 PASS / 10 FAIL / 0 TIMEOUT / 0 CRASH**

### PW Batch 154 (port=19386, Phase 511 verification, 76 tests)
- `page-check.spec.ts`: **16 / 16 PASS (100%)** | 0 FAIL | 0 TIMEOUT | 0 SKIP
- `page-focus.spec.ts`: **6 / 7 PASS (+1 unexpected pass!)** | 1 FAIL (`should emit blur event`) | 0 TIMEOUT | 0 SKIP
- `page-fill.spec.ts`: 25 PASS | 1 FAIL | 14 TIMEOUT | 0 SKIP
- `page-history.spec.ts`: **10 / 13 PASS (+1 unexpected pass!)** | 0 FAIL | 3 TIMEOUT | 0 SKIP
- Totals: **57 PASS / 2 FAIL / 17 TIMEOUT / 0 SKIP**

### WPT-B252 (port=19700, Phase 513 verification, 37 tests)
- `/css/css-overflow/parsing/`: 32 total | **16 PASS (up from 11, `block-ellipsis-*`, `continue-*`, `max-lines-invalid` green!)** | 16 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-display/parsing/`: 5 total | 1 PASS | 4 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **17 PASS / 20 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B253 (port=19700, Phase 513 verification, 84 tests)
- `/css/css-ui/parsing/`: 39 total | 25 PASS | 13 FAIL | 1 TIMEOUT | 0 CRASH
- `/css/css-borders/tentative/parsing/`: 45 total | **13 PASS (+13 FAIL→PASS!)** | 32 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **38 PASS / 45 FAIL / 1 TIMEOUT / 0 CRASH**

### WPT-B254 (port=19700, Phase 513 verification, 40 tests)
- `/css/css-break/parsing/`: 18 total | **16 PASS (+2 FAIL→PASS, 0 regressions!)** | 2 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-position/parsing/`: **22 / 22 PASS (100% ALL GREEN!)** | 0 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **38 PASS / 2 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B255 (port=19700, Phase 514 verification, 70 tests)
- `/css/css-ui/parsing/`: 39 total | **27 PASS (up from 25, `outline-shorthand` & `outline-valid` green!, 38/39 matched)** | 12 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-multicol/parsing/`: 31 total | **19 PASS (up from 18, `column-rule-shorthand` green!)** | 12 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **46 PASS / 24 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B256 (port=19700, Phase 514 verification, 79 tests)
- `/css/css-align/parsing/`: 50 total | 35 PASS | 15 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-box/parsing/`: 29 total | 23 PASS | 6 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **58 PASS / 21 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B257 (port=19700, Phase 515 verification, 70 tests)
- `/css/css-ui/parsing/`: 39 total | **28 PASS (up from 27, `webkit-appearance-alias` green!, all expected PASS green!)** | 10 FAIL | 1 TIMEOUT | 0 CRASH
- `/css/css-multicol/parsing/`: 31 total | **21 PASS (up from 19, `column-rule-width-valid` & `column-count-computed` green!)** | 10 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **49 PASS / 20 FAIL / 1 TIMEOUT / 0 CRASH**

### WPT-B258 (port=19700, Phase 515 verification, 46 tests)
- `/css/css-break/parsing/`: **18 / 18 PASS (100% ALL GREEN!, +4 FAIL→PASS)** | 0 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-transforms/parsing/`: 28 total | 15 PASS | 13 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **33 PASS / 13 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B259 (port=19700, Phase 516 verification, 70 tests)
- `/css/css-multicol/parsing/`: 31 total | **24 PASS (up from 21, `column-{fill,span,rule-width}-computed` green!, 30/31 matched)** | 7 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-ui/parsing/`: 39 total | 28 PASS | 10 FAIL | 1 TIMEOUT | 0 CRASH
- Totals: **52 PASS / 17 FAIL / 1 TIMEOUT / 0 CRASH**

### WPT-B260 (port=19700, Phase 516 verification, 114 tests)
- `/css/css-transforms/parsing/`: 28 total | 15 PASS | 13 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-text/parsing/`: 86 total | **45 PASS (+12 FAIL→PASS!)** | 41 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **60 PASS / 54 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B261 (port=19700, Phase 517 verification, 59 tests)
- `/css/css-multicol/parsing/`: 31 total | **25 PASS (31/31 matched, 0 unexpected!, `columns-invalid` green!)** | 6 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-transforms/parsing/`: 28 total | **16 PASS (up from 15, `rotate-parsing-valid` green!)** | 12 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **41 PASS / 18 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B262 (port=19700, Phase 518 verification, 114 tests)
- `/css/css-transforms/parsing/`: 28 total | **17 PASS (up from 16, `transform-box-computed` green!)** | 11 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-text/parsing/`: 86 total | **47 PASS (up from 45, `line-break-computed` & `word-wrap-computed` green!)** | 39 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **64 PASS / 50 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B263 (port=19700, Phase 518 verification, 119 tests)
- `/css/css-backgrounds/parsing/`: 74 total | **29 PASS (up from 8!)** | 45 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-borders/tentative/parsing/`: 45 total | **13 PASS (+13 FAIL→PASS!)** | 32 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **42 PASS / 77 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B264 (port=19700, Phase 519 verification, 49 tests)
- `/css/css-animations/parsing/`: 35 total | 17 PASS | 18 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-transitions/parsing/`: 14 total | 5 PASS | 9 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **22 PASS / 27 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B265 (port=19700, Phase 519 verification, 91 tests)
- `/css/css-text/parsing/`: 86 total | **52 PASS (up from 47, `text-wrap*` & `white-space-collapse-computed` green!)** | 34 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-display/parsing/`: 5 total | 1 PASS | 4 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **53 PASS / 38 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B266 (port=19700, Phase 520 verification, 49 tests)
- `/css/css-transitions/parsing/`: 14 total | **8 PASS (up from 5, `transition-property-valid` & `transition-{delay,duration}-computed` green!)** | 6 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-animations/parsing/`: 35 total | 17 PASS | 18 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **25 PASS / 24 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B267 (port=19700, Phase 520 verification, 61 tests)
- `/css/css-grid/parsing/ --shard 1/2`: 31 total | 3 PASS (`FAIL→PASS`: `grid-template-rows-repeat-intrinsic-valid.tentative`) | 27 FAIL | 1 TIMEOUT | 0 CRASH
- `/css/css-grid/parsing/ --shard 2/2`: 30 total | 3 PASS (`FAIL→PASS`: `grid-template-columns-repeat-intrinsic-valid.tentative`) | 26 FAIL | 1 TIMEOUT | 0 CRASH
- Totals: **6 PASS / 53 FAIL / 2 TIMEOUT / 0 CRASH**

### PW Batch 155 (port=19386, Phase 517 verification, 176 tests)
- `page-evaluate.spec.ts`: 35 PASS | 1 FAIL | 59 TIMEOUT | 0 SKIP
- `page-network-response.spec.ts`: **18 / 21 PASS** | 2 FAIL | 1 TIMEOUT | 0 SKIP
- `page-keyboard.spec.ts`: **38 / 44 PASS** | 3 FAIL | 3 TIMEOUT | 0 SKIP
- `page-dispatchevent.spec.ts`: **15 / 16 PASS** | 1 FAIL | 0 TIMEOUT | 0 SKIP
- Totals: **106 PASS / 7 FAIL / 63 TIMEOUT / 0 SKIP**

### WPT-B268 (port=19700, Phase 521 verification, 49 tests)
- `/css/css-transitions/parsing/`: 14 total | **10 PASS (up from 8, `transition-property-invalid` & `transition-valid` green!)** | 4 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-animations/parsing/`: 35 total | 17 PASS | 18 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **27 PASS / 22 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B269 (port=19700, Phase 521 verification, 82 tests)
- `/css/css-fonts/parsing/ --shard 1/2`: 41 total | 16 PASS | 24 FAIL | 1 TIMEOUT | 0 CRASH
- `/css/css-fonts/parsing/ --shard 2/2`: 41 total | 18 PASS | 22 FAIL | 1 TIMEOUT | 0 CRASH
- Totals: **34 PASS / 46 FAIL / 2 TIMEOUT / 0 CRASH**

### WPT-B270 (port=19700, Phase 522 verification, 49 tests)
- `/css/css-transitions/parsing/`: 14 total | **12 PASS (up from 10, `transition-shorthand` & `transition-timing-function-invalid` green!)** | 2 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-animations/parsing/`: 35 total | **18 PASS (up from 17, `animation-timing-function-invalid` green!)** | 17 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **30 PASS / 19 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B271 (port=19700, Phase 522 verification, 90 tests)
- `/css/css-cascade/ --shard 1/2`: 45 total | 20 PASS | 24 FAIL | 0 TIMEOUT | 1 ERROR
- `/css/css-cascade/ --shard 2/2`: 45 total | 17 PASS (`FAIL→PASS`: `layer-counter-style-override`) | 26 FAIL | 2 TIMEOUT | 0 CRASH
- Totals: **37 PASS / 50 FAIL / 2 TIMEOUT / 1 ERROR**

### WPT-B272 (port=19700, Phase 523 verification, 100 tests)
- `/css/css-transitions/parsing/`: 14 total | **13 PASS (`transition-computed` & `transition-invalid` green!, only `transition-valid` left!)** | 1 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-text/parsing/`: 86 total | 51 PASS | 35 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **64 PASS / 36 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B273 (port=19700, Phase 523 verification, 73 tests)
- `/css/css-lists/parsing/`: 23 total | 17 PASS | 6 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-align/parsing/`: 50 total | 35 PASS | 15 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **52 PASS / 21 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B274 (port=19700, Phase 524 verification, 49 tests)
- `/css/css-transitions/parsing/`: **14 / 14 PASS (100% ALL GREEN!)** | 0 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-animations/parsing/`: 35 total | 18 PASS | 17 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **32 PASS / 17 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B275 (port=19700, Phase 524 verification, 58 tests)
- `/css/css-text-decor/parsing/`: 31 total | 25 PASS | 6 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-flexbox/parsing/`: 27 total | 23 PASS | 4 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **48 PASS / 10 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B276 (port=19700, Phase 525 verification, 121 tests)
- `/css/css-animations/parsing/`: 35 total | **22 PASS (up from 18, `animation-composition-*` & `animation-name-{valid,invalid}` green!)** | 13 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-text/parsing/`: 86 total | 52 PASS | 34 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **74 PASS / 47 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B277 (port=19700, Phase 525 verification, 162 tests)
- `/css/css-values/ --shard 1/4`: 88 total | 19 PASS (`FAIL→PASS`: `random-in-keyframe`) | 60 FAIL | 9 TIMEOUT | 0 CRASH
- `/css/css-backgrounds/parsing/`: 74 total | 29 PASS | 45 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **48 PASS / 105 FAIL / 9 TIMEOUT / 0 CRASH**

### WPT-B278 (port=19700, Phase 526 verification, 121 tests)
- `/css/css-animations/parsing/`: 35 total | 22 PASS | 13 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-text/parsing/`: 86 total | **54 PASS (up from 52, `text-wrap-computed` & `text-wrap-pretty` green!)** | 32 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **76 PASS / 45 FAIL / 0 TIMEOUT / 0 CRASH**

### PW Batch 156 (port=19386, Phase 521 verification, 198 tests)
- `page-dialog.spec.ts`: **7 / 9 PASS** | 2 FAIL (`prompt`/`confirm` accept return) | 0 TIMEOUT | 0 SKIP
- `page-goto.spec.ts`: 41 PASS | 10 FAIL | 10 TIMEOUT | 0 SKIP
- `page-click.spec.ts`: 39 PASS | 2 FAIL | 36 TIMEOUT | 0 SKIP
- `page-route.spec.ts`: 5 PASS | 40 FAIL | 6 TIMEOUT | 0 SKIP
- Totals: **92 PASS / 54 FAIL / 52 TIMEOUT / 0 SKIP**

### WPT-B279 (port=19700, Phase 527 verification, 175 tests)
- `/css/css-values/ --shard 2/4`: 88 total | 23 PASS (`FAIL→PASS`: `random-item-valid`, `various-values-important`) | 52 FAIL | 11 TIMEOUT | 0 CRASH
- `/css/css-values/ --shard 3/4`: 87 total | 26 PASS | 50 FAIL | 10 TIMEOUT | 1 ERROR
- Totals: **49 PASS / 102 FAIL / 21 TIMEOUT / 1 ERROR**

### WPT-B280 (port=19700, Phase 528 verification, 121 tests)
- `/css/css-text/parsing/`: 86 total | **58 PASS (up from 54, `text-transform-*`, `text-align-computed`, `white-space-shorthand` green!)** | 28 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-animations/parsing/`: 35 total | 22 PASS | 13 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **80 PASS / 41 FAIL / 0 TIMEOUT / 0 CRASH**

### WPT-B281 (port=19700, Phase 529 verification, 49 tests)
- `/css/css-animations/parsing/`: 35 total | **25 PASS (up from 22, `animation-{valid,invalid,computed}` green!)** | 10 FAIL | 0 TIMEOUT | 0 CRASH
- `/css/css-transitions/parsing/`: **14 / 14 PASS (100% ALL GREEN!)** | 0 FAIL | 0 TIMEOUT | 0 CRASH
- Totals: **39 PASS / 10 FAIL / 0 TIMEOUT / 0 CRASH**

### PW Batch 157 (bin_dev Phase 529, 2026-11-23)
- **Specs (`page-network-idle.spec.ts`, `page-network-request.spec.ts`, `page-listeners.spec.ts`, `page-emulate-media.spec.ts`)**: `52` total (`24 PASS / 16 FAIL / 11 TIMEOUT / 1 SKIP`, `page-emulate-media.spec.ts` **12/12 PASS** with 9 unexpected passes; `page-listeners.spec.ts` **3/3 PASS**).

### WPT-B282 (bin_dev Phase 529, 2026-11-23)
- **Filter `/css/css-text/parsing/`**: `86` files (`58 PASS / 28 FAIL / 0 TIMEOUT / 0 ERROR`, `53` matched expectations, `13` unexpected PASS, `20` unexpected FAIL). `white-space-shorthand-text-wrap.html` 3/6; `word-break-valid.html` 5/6; `letter-spacing-{valid,computed}` 8/9, 7/9; `word-spacing-{valid,computed}` 8/9, 7/9; `hyphenate-character-{valid,computed}` 4/5, 4/5; `tab-size-computed` 8/10.

### WPT-B283 (bin_dev Phase 530, 2026-11-23)
- **Filter `/css/css-animations/parsing/`**: `35` files (`31 PASS / 4 FAIL / 0 TIMEOUT / 0 ERROR`, `30` matched expectations, `1` unexpected PASS: `animation-shorthand.html` FAIL->PASS, `4` unexpected FAIL: `animation-name-computed` 26/27, `animation-range-end-computed` 23/29, `animation-range-end-valid` 19/24, `animation-range-start-valid` 24/26).

### WPT-B284 (bin_dev Phase 531, 2026-11-23)
- **Filter `/css/css-animations/parsing/`**: `35` files (**35 PASS / 0 FAIL / 0 TIMEOUT / 0 ERROR**, `34` matched expectations, `1` unexpected PASS: `animation-shorthand.html`; **100% ALL GREEN**).

### WPT-B285 (bin_dev Phase 531, 2026-11-23)
- **Filter `/css/css-text-decor/parsing/`**: `31` files (`25 PASS / 6 FAIL / 0 TIMEOUT / 0 ERROR`, `26` matched expectations, `2` unexpected PASS: `text-decoration-skip-spaces-{computed,valid}`, `3` unexpected FAIL: `text-decoration-color-computed` 2/3, `text-decoration-computed` 8/14, `text-emphasis-computed` 0/7).

### WPT-B286 (bin_dev Phase 532, 2026-11-23)
- **Filter `/css/css-text/parsing/`**: `86` files (**71 PASS / 15 FAIL / 0 TIMEOUT / 0 ERROR**, jumped from 58/86 to **71/86**, `66` matched expectations, `13` unexpected PASS, `7` unexpected FAIL). `letter-spacing`, `word-spacing`, `tab-size`, `word-break`, `white-space-shorthand-text-wrap`, `hyphenate-character`, `hanging-punctuation-invalid`, `text-justify-computed-legacy` all green; `hyphenate-limit-chars-computed` 10/11.

### WPT-B287 (bin_dev pre-533, 2026-11-23)
- **Filter `/css/css-text-decor/parsing/`**: `31` files (`25 PASS / 6 FAIL / 0 TIMEOUT / 0 ERROR`), `text-decoration-computed` improved `8/14 -> 11/14`; remaining 6 issues (`rgb()` serialization, vertical `filled/open`, `text-decoration-color: inherit`, `text-decoration-inset` negative/%) targeted in Phase 533 & 534.

### PW Batch 158 (bin_dev Phase 530, 2026-11-23)
- **Specs (`page-focus.spec.ts` 6/7, `page-aria-snapshot.spec.ts` 37/38, `page-screenshot.spec.ts` 18/44, `page-mouse.spec.ts` 14/15)**: `104` total (**75 PASS / 23 FAIL / 6 TIMEOUT / 0 SKIP**; +12 unexpected PASS including 7 in `page-mouse` and 4 in `page-screenshot`).

### WPT-B288 (bin_dev Phase 533, 2026-11-23)
- **Filter `/css/css-text-decor/parsing/`**: `31` files (**28 PASS / 3 FAIL / 0 TIMEOUT / 0 ERROR**, `27` matched expectations, `3` unexpected PASS: `text-decoration-skip-spaces-{computed,valid}`, `text-emphasis-style-computed-vertical-lr`; `text-decoration-computed` 14/14 and `text-emphasis-computed` 7/7 all green!).

### WPT-B289 (bin_dev Phase 534, 2026-11-23)
- **Filter `/css/css-text-decor/parsing/`**: `31` files (**30 PASS / 1 FAIL / 0 TIMEOUT / 0 ERROR**, `0` unexpected FAIL, `4` unexpected PASS: `text-decoration-inset-valid` 16/16, `text-decoration-skip-spaces-{computed,valid}`, `text-emphasis-style-computed-vertical-lr`; `text-decoration-color-computed` 3/3 all green!). Only `text-decoration-inset-computed` (9/16) remained, fixed in Phase 535.

### WPT-B290 (bin_dev Phase 535, 2026-11-23)
- **Filter `/css/css-text-decor/parsing/`**: `31` files (**31 PASS / 0 FAIL / 0 TIMEOUT / 0 ERROR**, `26` matched expectations, `5` unexpected PASS; **100% ALL GREEN**!).

### WPT-B291 (bin_dev Phase 536, 2026-11-23)
- **Filter `/css/css-text/parsing/`**: `86` files (**75 PASS / 11 FAIL / 0 TIMEOUT / 0 ERROR**, `70` matched expectations, `13` unexpected PASS, `3` unexpected FAIL). `hyphenate-limit-chars-computed` 11/11, `text-fit-invalid`, `text-group-align-invalid`, `word-space-transform-{invalid,computed}` all green!

### WPT-B292 (bin_dev Phase 537, 2026-11-23)
- **Filter `/css/css-text/parsing/`**: `86` files (**81 PASS / 5 FAIL / 0 TIMEOUT / 0 ERROR**, `0` unexpected FAIL, `16` unexpected PASS). Remaining `text-fit-computed` & `word-space-transform-valid` & `hanging-punctuation-valid` fixed in Phase 538; `text-spacing: auto` serialization fixed in Phase 539.

### WPT-B293 (bin_dev Phase 539, 2026-11-23)
- **Filter `/css/css-text/parsing/`**: `86` files (**86 PASS / 0 FAIL / 0 TIMEOUT / 0 ERROR**, `65` matched expectations, `21` unexpected PASS; **100% ALL GREEN**!).

### WPT-B294 (bin_dev Phase 539, 2026-11-23)
- **Filter `/css/css-flexbox/parsing/`**: `27` files (`23 PASS / 4 FAIL / 0 TIMEOUT / 0 ERROR`, `24` matched expectations, `3` unexpected FAIL: `flex-basis-computed` 10/12, `flex-computed` 10/14, `flex-valid` 12/16, `webkit-aliases` 0/12). Fixed in Phase 540.

### WPT-B295 (bin_dev Phase 540, 2026-11-23)
- **Filter `/css/css-flexbox/parsing/`**: `27` files (`24 PASS / 3 FAIL / 0 TIMEOUT / 0 ERROR`), `flex-basis-computed` 12/12 all green, `flex-valid` 15/16, `flex-computed` 13/14, `webkit-aliases` jumped `0/12 -> 10/12`.

### WPT-B296 (bin_dev Phase 541, 2026-11-23)
- **Filter `/css/css-flexbox/parsing/`**: `27` files (**26 PASS / 1 FAIL / 0 TIMEOUT / 0 ERROR**, `27/27` matched expectations, `0` unexpected FAIL; `flex-valid` & `flex-computed` & `flex-basis-computed` all green, only `webkit-aliases` 10/12 expected FAIL).

### WPT-B298 (bin_dev Phase 542, 2026-11-23)
- **Filter `/css/css-lists/parsing/`**: `23` files (**21 PASS / 2 FAIL / 0 TIMEOUT / 0 ERROR**; `counter-{reset,increment,set}-computed` & `list-style-computed` all green!). Only `list-style-valid` (22/24) and `list-style-image-computed` (3/11) remained, fixed in Phase 543.

### WPT-B299 (bin_dev Phase 543, 2026-11-23)
- **Filter `/css/css-lists/parsing/`**: `23` files (**22 PASS / 1 FAIL / 0 TIMEOUT / 0 ERROR**; `list-style-valid` 24/24 all green!). Only `list-style-image-computed.sub.html` (8/11) remained due to unclosed trailing `)` in 3 WPT gradient inputs.

### WPT-B300 (bin_dev Phase 543, 2026-11-23)
- **Filter `/css/css-ui/parsing/`**: `39` files (`28 PASS / 10 FAIL / 1 TIMEOUT / 0 ERROR`, `38/39` matched expectations); `outline-color-{valid,computed}` 1/2 & 2/3, `outline-offset-{valid,computed}` 4/5 & 6/7, `cursor-computed` 36/39, `interactivity` 6/9.

### WPT-B302 (bin_dev Phase 544, 2026-11-23)
- **Filter `/css/css-ui/parsing/`**: `39` files (**31 PASS / 8 FAIL / 0 TIMEOUT / 0 ERROR**, `3` unexpected PASS: `interactivity`, `outline-offset-{valid,computed}`; `interactivity-computed` 40/46). `outline-color` & `caret-color` fixed in Phase 545.

### WPT-B303 (bin_dev Phase 545, 2026-11-23)
- **Filter `/css/css-ui/parsing/`**: `39` files (**33 PASS / 5 FAIL / 1 TIMEOUT / 0 ERROR**, `5` unexpected PASS: `caret-color-valid`, `interactivity`, `outline-color-valid`, `outline-offset-{valid,computed}`; `caret-color-computed` 7/12, `outline-color-computed` 2/3, `cursor-computed` 36/39).

### WPT-B304 (bin_dev Phase 545, 2026-11-23)
- **Filter `/css/css-lists/parsing/`**: `23` files (**23 PASS / 0 FAIL / 0 TIMEOUT / 0 ERROR**, `23/23` matched expectations; **100% ALL GREEN**!).

### WPT-B305 (bin_dev Phase 546, 2026-11-23)
- **Filter `/css/css-ui/parsing/`**: `39` files (**34 PASS / 4 FAIL / 1 TIMEOUT / 0 ERROR**, `7` unexpected PASS: `caret-color-{valid,computed}`, `cursor-valid`, `interactivity`, `outline-color-valid`, `outline-offset-{valid,computed}`; `cursor-computed` 37/39, `outline-color-computed` 2/3, `cursor-invalid` 6/10).

### WPT-B306 (bin_dev Phase 547, 2026-11-23)
- **Filter `/css/css-ui/parsing/`**: `39` files (**35 PASS / 3 FAIL / 1 TIMEOUT / 0 ERROR**, `38/39` matched expectations); `cursor-invalid.html` 10/10 (100% PASS), `cursor-valid.html` 10/10 (100% PASS), `caret-color-{valid,computed}` PASS, `outline-*` (8 files PASS); `interactivity-computed.html` TIMEOUT.

### PW-B160 (bin_dev Phase 547, 2026-11-23)
- **Spec `page-keyboard.spec.ts`**: `44` tests (**38 PASS / 3 FAIL / 3 TIMEOUT**), 20 expected FAIL promoted to PASS (`should move with the arrow keys`, `insertText`, `key events`, `selectAll`, `copy-pasting`, etc.).

### WPT-B307 (bin_dev Phase 548, 2026-11-23)
- **Filter `/css/css-overflow/parsing/`**: `32` files (**21 PASS / 11 FAIL / 0 TIMEOUT / 0 ERROR**, `32/32` matched expectations, **100% MATCHED**!); `block-ellipsis-*` PASS, `continue-*` PASS, `line-clamp-*` PASS, `max-lines-*` PASS, `overflow-clip-margin.html` PASS, `overflow-{valid,computed,invalid}` PASS, `scrollbar-gutter-*` PASS, `text-overflow-*` PASS, `webkit-line-clamp-*` PASS.

### WPT-B308 (bin_dev Phase 549, 2026-11-24)
- **HTML5 Tokenizer Crash/Hang Fix (`src/html/tokenizer.cp`)**: 修复 `State.Data` 与 `State.RcData` 中遇到未转义 `&`（如 `<title>CSS Intrinsic & Extrinsic Sizing: ...</title>`）且 `decodeEntity` 返回 `null` 时死循环悬停崩溃的严重缺陷。
- **Filter `/css/css-contain/parsing/`**: `4` files (**4 PASS / 0 FAIL / 0 TIMEOUT / 0 ERROR**, `4/4` matched expectations, **100% ALL GREEN**!); `contain-computed-children.html`, `contain-computed.html`, `contain-invalid.html`, `contain-valid.html` 全绿通过。
- **Filter `/css/css-contain/content-visibility/`**: `4` files (**4 PASS / 0 FAIL / 0 TIMEOUT / 0 ERROR**, **100% ALL GREEN**!); `content-visibility-computed.html`, `content-visibility-invalid.html`, `content-visibility-valid.html`, `inheritance.html` 全绿通过。
- **Filter `/css/css-sizing/contain-intrinsic-size/parsing/`**: `3` files (**3 PASS / 0 FAIL / 0 TIMEOUT / 0 ERROR**, `3/3` matched expectations, **100% ALL GREEN**!); `contain-intrinsic-size-computed.html`, `contain-intrinsic-size-invalid.html`, `contain-intrinsic-size-valid.html` 全绿通过。
- **Filter `/css/css-sizing/parsing/`**: `19` files (**19 PASS / 0 FAIL / 0 TIMEOUT / 0 ERROR**, `19/19` matched expectations, **100% ALL GREEN**!); `box-sizing-*`, `height-*`, `width-*`, `min-width-*`, `max-width-*`, `min-height-*`, `max-height-*` 全绿通过。
- **Filter `/css/css-sizing/aspect-ratio/parsing/`**: `3` files (**3 PASS / 0 FAIL / 0 TIMEOUT / 0 ERROR**, **100% ALL GREEN**!); `aspect-ratio-computed.html`, `aspect-ratio-invalid.html`, `aspect-ratio-valid.html` 全绿通过。

### WPT-B309 (bin_dev Phase 550, 2026-11-24)
- **Filter `/css/css-scroll-anchoring/parsing/`**: `3` files (**3 PASS / 0 FAIL / 0 TIMEOUT / 0 ERROR**, **100% ALL GREEN**!); `overflow-anchor-computed.html`, `overflow-anchor-invalid.html`, `overflow-anchor-valid.html` 全绿通过。
- **Filter `/css/css-shapes/parsing/`**: `6` files (**6 PASS / 0 FAIL / 0 TIMEOUT / 0 ERROR**, **100% ALL GREEN**!); `shape-image-threshold-computed.html`, `shape-image-threshold-invalid.html`, `shape-image-threshold-valid.html`, `shape-margin-computed.html`, `shape-margin-invalid.html`, `shape-margin-valid.html` 全绿通过。
- **Filter `/css/css-scroll-snap/parsing/`**: `9` files (**9 PASS / 0 FAIL / 0 TIMEOUT / 0 ERROR**, **100% ALL GREEN**!); `scroll-snap-type-computed.html`, `scroll-snap-type-invalid.html`, `scroll-snap-type-valid.html`, `scroll-snap-align-computed.html`, `scroll-snap-align-invalid.html`, `scroll-snap-align-valid.html`, `scroll-snap-stop-computed.html`, `scroll-snap-stop-invalid.html`, `scroll-snap-stop-valid.html` 全绿通过。
- **累计**: 18/18 对应用例 100% 全部通过。

### WPT-B310 (bin_dev Phase 551, 2026-11-24)
- **Filter `/css/css-scroll-snap/parsing/` (scroll-margin & scroll-padding 全系)**: `16` files (**16 PASS / 0 FAIL / 0 TIMEOUT / 0 ERROR**, **100% ALL GREEN**!);
  - `scroll-margin-block-inline-computed.html` (PASS), `scroll-margin-block-inline-invalid.html` (PASS), `scroll-margin-block-inline-shorthand.html` (PASS), `scroll-margin-block-inline-valid.html` (PASS)
  - `scroll-margin-computed.html` (PASS), `scroll-margin-invalid.html` (PASS), `scroll-margin-shorthand.html` (PASS), `scroll-margin-valid.html` (PASS)
  - `scroll-padding-block-inline-computed.html` (PASS), `scroll-padding-block-inline-invalid.html` (PASS), `scroll-padding-block-inline-shorthand.html` (PASS), `scroll-padding-block-inline-valid.html` (PASS)
  - `scroll-padding-computed.html` (PASS), `scroll-padding-invalid.html` (PASS), `scroll-padding-shorthand.html` (PASS), `scroll-padding-valid.html` (PASS)
- **累计**: 16/16 用例 100% 全部通过。

### WPT-B311 / B312 / B313 / B314 摸底与 Phase 552 全绿闭环 (bin_dev Phase 552, 2026-11-24)
- **Filter `/css/css-logical/parsing/`**: `54` files (**54 PASS / 0 FAIL / 0 TIMEOUT / 0 ERROR**, **100% ALL GREEN**!); 从摸底 `23/54` 跃升至 `54/54` 全绿（覆盖 `block-size`/`inline-size`/`min-*/max-*`/`inset*`/`margin-block*`/`padding-block*`/`border-block*`/`border-inline*` 全系长手、简写与计算值）。
- **Filter `/css/css-tables/parsing/`**: `15` files (**15 PASS / 0 FAIL / 0 TIMEOUT / 0 ERROR**, **100% ALL GREEN**!); `border-spacing-{valid,computed}` 转绿。
- **子智能体只读摸底记录（待后续 Phase 攻坚）**：
  - `/css/css-masking/parsing/`: `28` files (`4 PASS / 24 FAIL / 0 TIMEOUT`)
  - `/css/selectors/parsing/`: `16` files (`6 PASS / 10 FAIL / 0 TIMEOUT`)
  - `/css/css-images/parsing/`: `24` files (`7 PASS / 15 FAIL / 2 TIMEOUT`, `image-resolution-valid` unexpected PASS)
  - `/css/css-gaps/parsing/`: `15` files (`3 PASS / 12 FAIL / 0 TIMEOUT`)
  - `/intersection-observer/`: `117/149` ran (`45 PASS / 61 FAIL / 11 TIMEOUT`, batch 2 因 `3d-transform-occlusion` 重启上限截断)

### WPT-B315 ~ B318 摸底与 Phase 553 选择器解析 16/16 全绿闭环 (bin_dev Phase 553, 2026-11-24)
- **Filter `/css/selectors/parsing/`**: `16` files (**16 PASS / 0 FAIL / 0 TIMEOUT / 0 ERROR**, **100% ALL GREEN**!); 从摸底 `6/16` 跃升至 `16/16` 全绿（`invalid-pseudos`, `parse-attribute`, `parse-has`, `parse-has-disallow-nesting-has-inside-has`, `parse-is`, `parse-not`, `parse-slotted`, `parse-state`, `parse-where` 等全面通过）。
- **子智能体只读摸底记录（待后续 Phase 攻坚）**：
  - `/css/css-variables/`: `192/224` ran (`60 PASS / 126 FAIL / 5 TIMEOUT / 1 ERROR`)
  - `/css/css-pseudo/parsing/`: `5` files (`0 PASS / 5 FAIL / 0 TIMEOUT`, `5/5` matched expectations)
  - `/wasm/` (`--shard 1/3`): `32/119` ran (`0 PASS / 27 FAIL / 3 TIMEOUT / 2 ERROR`)
  - `/domparsing/`: `33` files (`6 PASS / 24 FAIL / 2 TIMEOUT / 1 SKIP`)
  - `/domxpath/`: `28` files (`20 PASS / 7 FAIL / 1 ERROR / 0 TIMEOUT`)








### WPT-B319 ~ B331 摸底与 Phase 554 `/domxpath/` 28/28 全绿闭环 (bin_dev Phase 554, 2026-11-24)
- **Filter `/domxpath/`**: `28` files (**28 PASS / 0 FAIL / 0 TIMEOUT / 0 ERROR**, **100% ALL GREEN**!); 修复 `Attr.prototype.lookupNamespaceURI`、`Node.prototype.lookupNamespaceURI`（`DocumentType`/`DocumentFragment` 返回 `null`，`xml`/`xmlns` 仅限 Element）、`resolvePrefix`（`ErrorEvent` 上报 + `NAMESPACE_ERR`）、`asciiLowercase` 与 SVG/MathML 大小写保留、`lang()` 属性大小写、`createHTMLDocument().body` 动态 getter、`ParsedElement.prototype.outerHTML` 与 `serializeXmlNode` 死循环、以及 `src/html/tree_builder.cp` `processInCell` 遇到 `</table>`/`</tbody>`/`</tr>` 结束标签未退出单元格状态导致后续同级表格嵌套的缺陷。
- **E2E Smoke**: `CAPSURF_BIN=./bin_dev/main python3 scripts/e2e_suite.py --smoke` **3/3 suites, 6/6 tests PASS**。
- **子智能体只读摸底记录**：
  - `/css/css-animations/parsing/`: `35` files (**35/35 PASS, 100% ALL GREEN**, `animation-shorthand.html` unexpected PASS)
  - `/css/css-transitions/parsing/`: `14` files (**14/14 PASS, 100% ALL GREEN**)
  - `/css/css-break/parsing/`: `18` files (**18/18 PASS, 100% ALL GREEN**, 4 unexpected PASS: `orphans-computed/valid`, `widows-computed/valid`)
  - `/css/css-sizing/parsing/`: `19` files (**19/19 PASS**)
  - `/css/css-position/parsing/`: `22` files (**22/22 PASS**)
  - `/css/css-will-change/parsing/`: `3` files (`2 PASS / 1 FAIL`: `will-change-invalid.html`)
  - `/css/css-align/parsing/`: `50` files (`35 PASS / 15 FAIL`)
  - `/css/css-multicol/parsing/`: `31` files (`25 PASS / 6 FAIL`)
  - `/css/css-box/parsing/`: `29` files (`24 PASS / 5 FAIL`)
  - `/css/css-display/parsing/`: `5` files (`1 PASS / 3 FAIL / 1 TIMEOUT`)
  - `/css/css-backgrounds/parsing/`: `74` files (`29 PASS / 45 FAIL`)
  - `/css/css-color/parsing/`: `43` files (`10 PASS / 33 FAIL`)
  - `/custom-elements/`: `17` files (`10 PASS / 7 FAIL`); `/shadow-dom/` (`1/2`): `60` files (`18 PASS`)
  - `/user-timing/`: `36` files (`17 PASS`); `/hr-time/`: `14` files (`4 PASS`); `/webidl/`: `43` files (`8 PASS`)
  - `/console/`: `13` files (`4 PASS`); `/url/`: `18` files (`0 PASS`); `/urlpattern/`: `5` files (`0 PASS`)

### WPT-B332 ~ B341 摸底与 Phase 555 `/css/css-will-change/parsing/` 3/3 + `/css/css-align/parsing/` 50/50 全绿闭环 (bin_dev Phase 555, 2026-11-24)
- **Filter `/css/css-will-change/parsing/`**: `3` files (**3 PASS / 0 FAIL / 0 TIMEOUT**, **100% ALL GREEN**!); 实现 `will-change` 严格文法校验（拦截 `auto` 混用及 `none`/`all`/`will-change`/CSS 全局关键字，放行 `--var` 前缀 `<custom-ident>`）。
- **Filter `/css/css-align/parsing/`**: `50` files (**50 PASS / 0 FAIL / 0 TIMEOUT**, **100% ALL GREEN**!); 实现 `first baseline -> baseline`、`<pos> legacy -> legacy <pos>`、`place-content` 单基线值补全 `start`、以及 `justify-items: legacy` 沿父链继承解析；同步收紧 `test/wpt/wpt-expectations.json`（`justify-items-computed.html`, `place-items-computed.html`）。
- **E2E Smoke**: `CAPSURF_BIN=./bin_dev/main python3 scripts/e2e_suite.py --smoke` **3/3 suites, 6/6 tests PASS**。
- **子智能体只读摸底记录**：
  - `/css/css-text/parsing/`: `86` files (**86/86 PASS, 100% ALL GREEN**, 21 unexpected PASS)
  - `/css/css-text-decor/parsing/`: `31` files (**31/31 PASS, 100% ALL GREEN**, 5 unexpected PASS)
  - `/css/css-lists/parsing/`: `23` files (**23/23 PASS, 100% ALL GREEN**)
  - `/css/css-flexbox/parsing/`: `27` files (`26 PASS / 1 FAIL`: `webkit-aliases.html`)
  - `/css/css-writing-modes/parsing/`: `15` files (`13 PASS / 2 FAIL`: `text-combine-upright-invalid`, `text-orientation-invalid`)
  - `/css/css-shapes/parsing/`: `13` files (`6 PASS / 7 FAIL`)
  - `/css/css-ruby/parsing/`: `8` files (`3 PASS / 5 FAIL`)
  - `/css/css-counter-styles/`: `11` files (`0 PASS / 11 FAIL`)
  - `/css/css-grid/parsing/`: `61` files (`6 PASS / 51 FAIL / 4 TIMEOUT`)

### WPT-B342 ~ B355 摸底与 Phase 556 `/css/css-writing-modes/parsing/` 15/15 + `/css/css-flexbox/parsing/` 27/27 全绿闭环 (bin_dev Phase 556, 2026-11-24)
- **Filter `/css/css-writing-modes/parsing/`**: `15` files (**15 PASS / 0 FAIL / 0 TIMEOUT**, **100% ALL GREEN**!); 新增 `text-combine-upright`（`none | all`）与 `text-orientation`（`mixed | upright | sideways`）严格校验；收紧 `writing-mode-computed.html`、`writing-mode-valid.html` 基线为 `PASS`。
- **Filter `/css/css-flexbox/parsing/`**: `27` files (**27 PASS / 0 FAIL / 0 TIMEOUT**, **100% ALL GREEN**!); 在 `expandCssShorthand` 与 `normalizeCssSpecifiedValue` 中跳过含 `var(...)` 的待替换简写值（修复 `-webkit-flex` / `-webkit-flex-flow` 遗留别名测试 `webkit-aliases.html`）；收紧 `webkit-aliases.html` 基线为 `PASS`。
- **E2E Smoke**: `CAPSURF_BIN=./bin_dev/main python3 scripts/e2e_suite.py --smoke` **3/3 suites, 6/6 tests PASS**。
- **子智能体只读摸底记录**：
  - `/css/css-contain/parsing/`: `4` files (**4/4 PASS, 100% ALL GREEN**)
  - `/css/css-scroll-anchoring/`: `1` file (**1/1 PASS, 100% ALL GREEN**)
  - `/css/css-content/parsing/`: `4` files (`1 PASS / 3 FAIL`)
  - `/css/css-cascade/parsing/`: `5` files (`2 PASS / 3 FAIL`)
  - `/css/css-view-transitions/parsing/`: `9` files (`3 PASS / 5 FAIL / 1 TIMEOUT`)
  - `/css/css-anchor-position/parsing/`: `10` files (`0 PASS / 10 FAIL`)
  - `/css/css-overscroll-behavior/parsing/`: `1` file (`0 PASS / 1 FAIL`)
  - `/css/css-page/parsing/`: `11` files (`5 PASS / 6 FAIL`)
  - `/css/css-rhythm/parsing/`: `5` files (`0 PASS / 5 FAIL`)

### WPT-B356 ~ B366 摸底与 Phase 557 `/css/css-overscroll-behavior/parsing/` 1/1 + `/css/css-content/parsing/` 4/4 全绿闭环 (bin_dev Phase 557, 2026-11-24)
- **Filter `/css/css-overscroll-behavior/parsing/`**: `1` file (**1 PASS / 0 FAIL / 0 TIMEOUT**, **100% ALL GREEN**!); 实现 `overscroll-behavior`（`[ contain | none | auto | chain ]{1,2}`）与长手属性严格校验。
- **Filter `/css/css-content/parsing/`**: `4` files (**4 PASS / 0 FAIL / 0 TIMEOUT**, **100% ALL GREEN**!); 严格校验 `content` 的 `/` alt-text 语义、`attr()`/`counter()`/`counters()`/`symbols()` 参数约束，规范化 `counter(..., decimal)`/`symbols(symbolic ...)`，修复 `window.js` 中多令牌字符串误转义问题，并将 `evalGradientComputed` 与 `resolveContentGradients` 提升至 `display: none` 提前返回分支之前以支持无布局盒元素上的渐变计算值（含 `to top right -> to right top` 与未闭合括号容错）；收紧 `content-counter-valid.html` 基线为 `PASS`。
- **E2E Smoke**: `CAPSURF_BIN=./bin_dev/main python3 scripts/e2e_suite.py --smoke` **3/3 suites, 6/6 tests PASS**。
- **子智能体只读摸底记录**：
  - `/css/css-logical/`: `58` files (`54 PASS / 1 FAIL / 3 TIMEOUT`)
  - `/css/css-sizing/`: `32` files (`27 PASS / 4 FAIL / 1 TIMEOUT`)
  - `/css/css-ui/`: `47` files (`35 PASS / 10 FAIL / 2 TIMEOUT`)
  - `/css/css-transitions/`: `20` files (`14 PASS / 5 FAIL / 1 TIMEOUT`)
  - `/css/css-properties-values-api/`: `14` files (`8 PASS / 6 FAIL`)
  - `/css/css-pseudo/`: `10` files (`3 PASS / 7 FAIL`)
  - `/css/css-syntax/`: `24` files (`7 PASS / 17 FAIL`)
  - `/css/css-borders/tentative/parsing/`: `45` files (`14 PASS / 31 FAIL`)
  - `/css/css-values/` (`1/3`, `2/3`, `3/3`): `23/59`, `18/62`, `19/63` PASS

### WPT-B369 ~ B370 摸底与 Phase 558 `/css/css-ruby/parsing/` 8/8 + `/css/css-rhythm/parsing/` 5/5 全绿闭环 (bin_dev Phase 558, 2026-11-24)
- **Filter `/css/css-ruby/parsing/`**: `8` files (**8 PASS / 0 FAIL / 0 TIMEOUT**, **100% ALL GREEN**!); 实现 `ruby-align`、`ruby-merge`、`ruby-overhang`（含 `none -> spaces` 归一化）与 `ruby-position` 严格校验；收紧 `ruby-*-valid.html` 4 项基线为 `PASS`。
- **Filter `/css/css-rhythm/parsing/`**: `5` files (**5 PASS / 0 FAIL / 0 TIMEOUT**, **100% ALL GREEN**!); 实现 `block-step-align`、`block-step-insert`、`block-step-round`、`block-step-size` 及简写 `block-step` 严格文法校验。
- **E2E Smoke**: `CAPSURF_BIN=./bin_dev/main python3 scripts/e2e_suite.py --smoke` **3/3 suites, 6/6 tests PASS**。
- **子智能体只读摸底记录**：
  - `/css/css-multicol/`: `32` files (`25 PASS / 7 FAIL`)
  - `/css/css-tables/`: `18` files (`15 PASS / 2 FAIL / 1 TIMEOUT`)

### WPT-B367 ~ B368 摸底与 Phase 559 `/css/css-box/parsing/` 29/29 全绿闭环 (bin_dev Phase 559, 2026-11-24)
- **Filter `/css/css-box/parsing/`**: `29` files (**29 PASS / 0 FAIL / 0 TIMEOUT**, **100% ALL GREEN**!); 为 `ZERO_PX_PROPS` 接入 `canonicalizeCalcToken`（修复 `padding-right: calc(2em + 3%) -> calc(3% + 2em)`），拦截未知 `margin-*`/`padding-*` 伪属性（`margin-bottom-left`、`padding-bottom-left`），并实现 `margin-trim`（`none | block | block-start | block-end`、双值折叠 `block` 与 `STANDARD_COMPUTED_LONGHANDS` 默认值）。
- **E2E Smoke**: `CAPSURF_BIN=./bin_dev/main python3 scripts/e2e_suite.py --smoke` **3/3 suites, 6/6 tests PASS**。
- **子智能体只读摸底记录**：
  - `/css/css-animations/` (partial): `16/58` ran (`8 PASS / 5 FAIL / 3 TIMEOUT`)
  - `/css/css-backgrounds/` (partial): `50/115` ran (`22 PASS / 22 FAIL / 5 TIMEOUT / 1 ERROR`)

### WPT-B367 ~ B368 摸底与 Phase 559 `/css/css-box/parsing/` 29/29 全绿闭环 (bin_dev Phase 559, 2026-11-24)
- **Filter `/css/css-box/parsing/`**: `29` files (**29 PASS / 0 FAIL / 0 TIMEOUT**, **100% ALL GREEN**!); 为 `ZERO_PX_PROPS` 接入 `canonicalizeCalcToken`（修复 `padding-right: calc(2em + 3%) -> calc(3% + 2em)`），拦截未知 `margin-*`/`padding-*` 属性名（`margin-bottom-left`、`padding-bottom-left`），并实现 `margin-trim`（`none | block | block-start | block-end`、双值折叠 `block` 与 `STANDARD_COMPUTED_LONGHANDS` 默认值）；收紧 `margin-trim.html`、`margin-trim-computed.html` 基线为 `PASS`。
- **E2E Smoke**: `CAPSURF_BIN=./bin_dev/main python3 scripts/e2e_suite.py --smoke` **3/3 suites, 6/6 tests PASS**。
- **子智能体只读摸底记录**：
  - `/css/css-animations/` (partial): `16/58` ran (`8 PASS / 5 FAIL / 3 TIMEOUT`)
  - `/css/css-backgrounds/` (partial): `50/115` ran (`22 PASS / 22 FAIL / 5 TIMEOUT / 1 ERROR`)

### WPT-B371 ~ B380 摸底与 Phase 560 `/css/css-multicol/parsing/` 31/31 全绿闭环 (bin_dev Phase 560, 2026-11-24)
- **Filter `/css/css-multicol/parsing/`**: `31` files (**31 PASS / 0 FAIL / 0 TIMEOUT**, **100% ALL GREEN**!);
  - `selector.js`: 放行 `::column`、`::scroll-marker` 及合法链式伪元素组合（`::before::column`、`::column::scroll-marker`、`::before::column::scroll-marker`），拦截 `::column:hover`、`::column::before`/`::first-letter`/`::first-line`。
  - `cssom.js`: 实现 `columns` 简写展开与序列化（含 `0` 宽度 -> `0px` 与 `/ <column-height>` 语法）及 `column-rule` 默认序列化修复（`medium`）。
  - `window.js`: 打通 `::column` 伪元素匹配、伪元素 `inherit` 原生元素回溯、`applyUsedValuesToComputedMap` 伪元素 `scroll-margin-*` `em -> px` 求值、`nonNegLenProps` 新增 `column-height`、`column-rule`/`columns` 计算值以及 `scrollSnapMargin* -> scrollMargin*` 代理映射；收紧 6 项基线为 `PASS`。
- **E2E Smoke**: `CAPSURF_BIN=./bin_dev/main python3 scripts/e2e_suite.py --smoke` **3/3 suites, 6/6 tests PASS**。
- **子智能体只读摸底记录**：
  - `/css/css-will-change/`: `3` files (**3/3 PASS, 100% ALL GREEN**)
  - `/css/css-scroll-snap/`: `8` files (`7 PASS / 1 FAIL`)
  - `/css/css-writing-modes/`: `20` files (`15 PASS / 5 FAIL`)
  - `/css/css-break/`: `209` files (`163 PASS / 44 FAIL / 2 TIMEOUT`)
  - `/css/css-overflow/`: `33` files (`22 PASS / 11 FAIL`)
  - `/css/css-flexbox/` (partial): `120/261` ran (`85 PASS / 25 FAIL / 10 TIMEOUT`)
  - `/css/css-align/` (partial): `35/119` ran (`23 PASS / 4 FAIL / 8 TIMEOUT`)
  - `/css/css-display/`: `52` files (`15 PASS / 31 FAIL / 6 TIMEOUT`)
  - `/css/css-shapes/`: `70` files (`20 PASS / 47 FAIL / 3 TIMEOUT`)
  - `/css/css-masking/`: `94` files (`15 PASS / 75 FAIL / 4 TIMEOUT`)

### Phase 561 `/css/css-scroll-snap/` 8/8 全绿闭环 & Writing Modes 校验增强 (bin_dev Phase 561, 2026-11-24)
- **Filter `/css/css-scroll-snap/`**: `8` files (**8 PASS / 0 FAIL / 0 TIMEOUT**, **100% ALL GREEN**!); 新增 `scroll-initial-target`（`none | nearest`）严格校验并加入 `STANDARD_CSS_PROPERTIES`。
- **Writing Modes (`text-combine-upright`, `text-orientation`) & `content`**: 将 `text-combine-upright`、`text-orientation`、`writing-mode` 加入 `INHERITED_CSS_PROPS`，补全 `getComputedStyle` 默认值（`"none"` / `"mixed"` / `"normal"`），并在 `collectRawComputedMap` 过滤非法内联声明使 `text-combine-upright-parsing-invalid-001/002.html` 转绿（`/css/css-writing-modes/` 17/20 PASS）。
- **E2E Smoke**: `CAPSURF_BIN=./bin_dev/main python3 scripts/e2e_suite.py --smoke` **3/3 suites, 6/6 tests PASS**。

### Phase 562 `/css/css-gaps/parsing/gap-decorations-important.html` & `/css/css-pseudo/parsing/highlight-pseudos.html` 转绿与选择器和解 (bin_dev Phase 562, 2026-11-24)
- **Filter `/css/css-gaps/parsing/gap-decorations-important.html`**: **1/1 PASS**; 在 `element.js` `parseStyleMap` 对 `column-rule`/`columns` 启用 `setEntryWithShorthand`，使内联 `!important` 简写展开至长手属性。
- **Selectors & Pseudo**: `selector.js` 支持 `*::pseudo -> ::pseudo` 归一化及 `::file-selector-button` 用户交互伪类组合；`/css/selectors/parsing/` **16/16 PASS**、`/css/css-pseudo/parsing/highlight-pseudos.html` **PASS**（基线收紧为 `PASS`）。
- **E2E Smoke**: `CAPSURF_BIN=./bin_dev/main python3 scripts/e2e_suite.py --smoke` **3/3 suites, 6/6 tests PASS**。

### Phase 563 `/css/css-images/parsing/` (`image-orientation`, `image-resolution`, `image()`, `object-fit`) 强化 & 3小时自进化固化 (bin_dev Phase 563, 2026-11-24)
- **Filter `/css/css-images/parsing/`**:
  - `image-orientation-{computed,invalid,valid}.html`: **3/3 PASS (100% ALL GREEN)**（校验 `from-image | none`，并将 `image-orientation` 纳入 `STANDARD_COMPUTED_LONGHANDS` 默认值 `"from-image"`）。
  - `image-function-{invalid,computed}.html`: **2/2 PASS**（校验 `image(<color>)` 及计算期颜色归一化）。
  - `image-resolution-invalid.html`: **PASS**（校验 `[ from-image || <resolution> ] && snap?`）。
  - `object-fit-computed.html`: **PASS**（支持 `[ contain | cover ] || scale-down` 双值语法与 `scale-down` 后置归一化，基线收紧为 `PASS`）。
- **E2E Smoke**: `CAPSURF_BIN=./bin_dev/main python3 scripts/e2e_suite.py --smoke` **3/3 suites, 6/6 tests PASS**。

### WPT-B381 ~ B383 摸底与 Phase 564 `object-position` 全绿闭环 (bin_dev Phase 564, 2026-11-24)
- **Filter `/css/css-images/parsing/object-position-*`**: **3/3 PASS (100% ALL GREEN)**（`object-position-valid.html`, `object-position-invalid.html`, `object-position-computed.html` 全绿）；`cssom.js` 实现 1 值补齐 `center` 与 4 值水平边优先规范序，`window.js` 实现 `object-position` 计算期 `%`/`px`/`calc(100% - px)` 精确解析。
- **E2E Smoke**: `CAPSURF_BIN=./bin_dev/main python3 scripts/e2e_suite.py --smoke` **3/3 suites, 6/6 tests PASS**。
- **子智能体只读摸底记录**：
  - `/css/css-position/`: `117` files (`50 PASS / 55 FAIL / 11 TIMEOUT / 1 ERROR`)
  - `/css/css-grid/` (`1/3`, `2/3`): `74/282`, `58/282` PASS

### Phase 565 `gradient-position` 全绿闭环 & WPT-B384 摸底 (bin_dev Phase 565, 2026-11-24)
- **Filter `/css/css-images/parsing/gradient-position`**: **3/3 PASS (100% ALL GREEN)**（`gradient-position-invalid.html`, `gradient-position-valid.html`, `gradient-position-computed.html` 全绿）；`cssom.js` 提取 `isValidCssPositionTokens` 校验 `(radial|conic)-gradient` 的 `at <position>` 并规范化指定值序，`window.js` 提取 `resolveGradientAtPos` 统一解析 `radial-gradient`/`conic-gradient` 计算期 `at <pos>` 并省略默认 `at 50% 50%`。
- **Filter `/css/css-images/parsing/object-position`**: **3/3 PASS** 保持全绿。
- **E2E Smoke**: `CAPSURF_BIN=./bin_dev/main python3 scripts/e2e_suite.py --smoke` **3/3 suites, 6/6 tests PASS**。
- **子智能体只读摸底记录**：
  - `/css/css-grid/` (`3/3`): `57/278` PASS

### Phase 566 `/css/css-images/parsing/` 剩余用例全绿收官 & WPT-B386/B387/B390 摸底 (bin_dev Phase 566, 2026-11-24)
- **Filter `/css/css-images/parsing/conic-gradient-calc-angle-percentage`**: **2/2 PASS (100% ALL GREEN)**（`calc(deg + %)` 接入 `canonicalizeCalcToken` 规范排序，同时拦截 `linear/radial-gradient` 混用 `%` 与角度、`conic-gradient(from calc(...%))` 及无单位混算）。
- **Filter `/css/css-images/parsing/image-resolution`**: **2/2 PASS (100% ALL GREEN)**（放行负 `<resolution>` 如 `-8dpcm`）。
- **Filter `/css/css-images/parsing/object-fit`**: **3/3 PASS (100% ALL GREEN)**（`contain scale-down` / `scale-down contain` 折叠序列化为 `"scale-down"`）。
- **Expectations Tightened**: `conic-gradient-calc-angle-percentage-valid.html`, `image-resolution-valid.html`, `object-fit-valid.html` 收紧为 `PASS`。
- **E2E Smoke**: `CAPSURF_BIN=./bin_dev/main python3 scripts/e2e_suite.py --smoke` **3/3 suites, 6/6 tests PASS**。
- **子智能体只读摸底记录**：
  - `/css/css-fonts/`: `121` files (`49 PASS / 69 FAIL / 3 TIMEOUT`)
  - `/css/css-text/`: `125` files (`90 PASS / 34 FAIL / 1 TIMEOUT`)
  - `/css/css-backgrounds/` (`partial`, 34 recorded): `18 PASS / 12 FAIL / 4 TIMEOUT` (`animations/border-*-width-composition.html`)

### Phase 567 `/css/css-transforms/` (`perspective-origin`, `transform-origin`, `translate`, `evaluation-validation`) 全绿闭环 & WPT-B391~B397 摸底 (bin_dev Phase 567, 2026-11-24)
- **Filter `/css/css-transforms/parsing/`**:
  - `perspective-origin-{invalid,valid,computed}.html`: **3/3 PASS (100% ALL GREEN)**（1 值 `<position>` 补齐 `center` 规范序列化）。
  - `transform-origin-{invalid,valid,computed}.html`: **3/3 PASS (100% ALL GREEN)**（1 值补齐 `center` 且严格拒绝 4 令牌位置）。
  - `translate-parsing-{invalid,computed,valid}.html`: **3/3 PASS (100% ALL GREEN)**（`translate` 指定值接入 `canonicalizeCalcToken`）。
- **Filter `/css/css-transforms/css-transform-style-evaluation-validation`**: **PASS (13/13)**（`transform` 函数参数拒绝不匹配右括号如 `rotate(30deg)|rotateX(60deg)`）。
- **E2E Smoke**: `CAPSURF_BIN=./bin_dev/main python3 scripts/e2e_suite.py --smoke` **3/3 suites, 6/6 tests PASS**。
- **子智能体只读摸底记录**：
  - `/css/css-transforms/` (`b391` initial): `31` files (`16 PASS / 14 FAIL / 1 TIMEOUT`) -> Phase 567 转绿 6 项至 `22/31 PASS`
  - `/css/css-color/` (`b385`): `56` files (`12 PASS / 34 FAIL / 10 TIMEOUT`)
  - `/css/css-lists/` (`b392`): `182` files (`45 PASS / 137 FAIL / 0 TIMEOUT`)
  - `/css/css-counter-styles/` (`b393`): `11` files (`0 PASS / 11 FAIL / 0 TIMEOUT`)
  - `/css/css-page/` (`b394`): `23` files (`8 PASS / 15 FAIL / 0 TIMEOUT`)
  - `/css/css-pseudo/` (`b395`): `10` files (`4 PASS / 6 FAIL / 0 TIMEOUT`)
  - `/css/css-masking/parsing/` (`b396`): `28` files (`4 PASS / 24 FAIL / 0 TIMEOUT`)
  - `/css/css-shapes/parsing/` (`b397`): `13` files (`6 PASS / 7 FAIL / 0 TIMEOUT`)

### Phase 568 `view-transition-*` 全绿 & `text-decor/parsing` 31/31 基线收紧 & WPT-B400~B406 摸底 (bin_dev Phase 568, 2026-11-24)
- **Filter `/css/css-view-transitions/parsing/view-transition-`**: **7/7 PASS (100% ALL GREEN)**（`cssom.js` 注册并严格校验 `view-transition-{name,class,group}` `<custom-ident>`；`window.js` 纳入 `STANDARD_COMPUTED_LONGHANDS`，`revert`/`revert-layer` 回退初始值、`maTch-element -> match-element`）。
- **Filter `/css/css-text-decor/parsing/`**: **31/31 PASS (100% ALL GREEN)**（收紧 `text-decoration-inset-{computed,valid}`, `text-decoration-skip-spaces-{computed,valid}`, `text-emphasis-style-computed-vertical-lr` 共 5 项基线为 `PASS`）。
- **E2E Smoke**: `CAPSURF_BIN=./bin_dev/main python3 scripts/e2e_suite.py --smoke` **3/3 suites, 6/6 tests PASS**。
- **子智能体只读摸底记录**：
  - `/css/css-text-decor/` (`b402`): `41` files (`34 PASS / 7 FAIL`)
  - `/css/css-nesting/` (`b401`): `45` files (`11 PASS / 34 FAIL`)
  - `/css/css-contain/parsing/` (`b404`): **4/4 PASS (100% ALL GREEN)**
  - `/css/css-sizing/parsing/` (`b406`): **19/19 PASS (100% ALL GREEN, 155/155 subtests)**
