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
