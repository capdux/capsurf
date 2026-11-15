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

