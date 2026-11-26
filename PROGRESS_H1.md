# CapSurf 工作进度交接（`PROGRESS.md`）

> 生成时间：2026-11-21 | Git `HEAD`：`dcb00fe`（Phase 212） | 当前状态：**已暂停，等待新 AGY 无缝衔接**  
> 资产盘点：`e2e/*.e2e.test.ts` **392** 个 + `e2e/demo/*.e2e.test.ts` **172** 个（共 **564** 个 E2E）；纯 CP 白盒测试 `test/**/*.cp` **103** 个。

---

## 1. 当前阻塞与未完成项

### 1.1 工作区未提交现场（停机前两路子智能体在研成果）
1. **Slot A（纯 CP 白盒）未验收提交**：
   - 新增未追踪文件：`test/dom/test_mutation_observer_edge.cp`（覆盖 `src/dom/mutation_observer.cp`）。
   - 需补跑 `cpc fix` + `python3 scripts/cpc_brief.py -n 8` + `cap test -m test/dom/test_mutation_observer_edge.cp` 后提交。
2. **Slot B（CDP Screencast 4 连发，端口 `19233` / `bin_19233`）未收尾提交**：
   - 在研改动：`src/cdp/screencast.cp`、`src/cdp/server.cp`、`src/cdp/kitesurf_facade.cp`、`src/cdp/page.cp`、`src/js/shim/window.js`、`src/js/runtime.js`、`src/js/runtime_source.cp`。
   - **关键未完**：此前全量白盒回归曾暴露 `src/cdp/screencast.cp:210` 存在 `*%=` 语法错误（应改为 `h = h *% 1099511628211;`），需确认已修复并跑通：
     - `e2e/demo/screencast-raf-pacing.e2e.test.ts`
     - `e2e/demo/screencast-idle-skip.e2e.test.ts`
     - `e2e/demo/screencast-partial-frames.e2e.test.ts`
     - `e2e/demo/screencast-wheel-coalescing.e2e.test.ts`
3. **`GEMINI.md` 本地微调**：工作区存在 `M GEMINI.md`（2 行变更），提交前确认或一并纳入。

### 1.2 已完成代码修改但待联合 E2E 复测的队列（已提交于 `18f88c2`）
- `e2e/tldraw-app-shell.e2e.test.ts`（6 个子测：`scheduler.postTask`、`indexedDB`、`Path2D`、`getCoalescedEvents`、`_skipSyntheticClick`、`pointer*` 边界事件）
- `e2e/demo/oopif-postmessage.e2e.test.ts`、`e2e/demo/oopif-runtime-evaluate.e2e.test.ts`
- `e2e/demo/page-renderer-parity.e2e.test.ts`、`e2e/demo/screenshot-demo.e2e.test.ts`、`e2e/tty-wheel-grid.e2e.test.ts`
- `e2e/style-churn-memory.e2e.test.ts`、`e2e/text-content-memory.e2e.test.ts`

### 1.3 已精确定位待修复的 CDP / E2E 缺口
1. `e2e/module-registry.e2e.test.ts`：`src/cdp/session.cp:1021-1058` 补齐 `with { type: "json" }` 导入属性与 `type: "css"` 拒绝语义。
2. `e2e/demo/oopif-quads-paint.e2e.test.ts` & `e2e/demo/oopif-prespawn.e2e.test.ts`：`src/cdp/session.cp:3762-3787`（`TL @(25,25)` 8px 偏移）及 `src/net/mock_fixtures.cp`（`/oopif-host-many.html`、`/oopif-child-solid.html`）。
3. `e2e/demo/oopif-detach-teardown.e2e.test.ts` & `e2e/demo/oopif-elements-panel.e2e.test.ts`：`src/cdp/page.cp:1060`（`isDefault: false` + `executionContextDestroyed`）与 `src/cdp/dom.cp:72,470`（`frameId`/`documentURL`/`DOM.getFrameOwner`）。
4. `e2e/detached-node-reclaim.e2e.test.ts` & `e2e/detached-owned-caches.e2e.test.ts`：`src/cdp/session.cp:3271-3313` 的 `Kitesurf.getMemoryStats` 缺少 `domNodes` 与 `wrappers: { demoted, revived, reclaimed }` 字段。
5. **非 CDP 纯 CP 白盒总回归重跑**：Phase 208 跑出 `68/71 PASS`（唯一 3 处失败均因 Slot B 在研的 `screencast.cp:210` `*%=` 语法），待 Slot B 修复 `screencast.cp` 后需重跑确认 `71+/71+`（含 Phase 209–212 新增）全绿。
6. **架构专属（非 QuickJS / 非独立进程，不可直接跑通）用例清单**：`cdp-scene-host`、`cdp-scene-stream`、`wasm-bake-demo`、`inline-wasm-bake`、`wasm-panic-unwind`、`local-tracing`、`isolate-kill`、`isolate-wall-cap`、`navigate-isolate-death`（Boa-Wasm / Cloudflare Workers `env.OTLP_CAPTURE` / RBI）。

---

## 2. 已排除的死胡同（避坑记录）

1. **CP 语法踩坑**：
   - `*%=` 不是合法 CP 运算符（报 `E0100: unexpected token 'Assign'`），包装乘法应写 `h = h *% 1099511628211;`。
   - 错误联合判断 `cp_eq(anyerror, error{...})` 可能返回 `false`，必须改用 `@errorName(e)` 字符串匹配（如 `== "FileTooBig"`）。
   - `src/eval/prefetch_scripts.cp:263` 的 `&body_out` 在 Zig lowering 下表现为 `[]const u8` 而非 `*[]const u8`，白盒测试勿直接调 `prefetchSingle`/`prefetchAll`，待后续专门修复该行。
   - `src/render/style_extractor.cp` 引用旧版 `css.css_values`，在 `cap test` 下无法编译；样式提取白盒应测 `ComputedStyle.extractPaintStyle`。
   - `cpc_brief.py` 对使用 `cap:test` 宏的 `.cp` 文件做独立 `ast-check` 时，会报 5 条未注入 `__cap_test_total`/`__cap_test_failed` 提示，属于正常现象，以 4 项门禁全 `0` + `cap test -m` 通过为准。
2. **文件追加工具坑**：
   - **严禁对 `SESSION.md` / `CHANGELOG.md` / `DEVLOG.md` 使用 `write_file(Append=true)`**（实测偶发异常或格式破坏），必须用 Python `open(path).read().rstrip("\n") + "\n\n" + block + "\n"` 追加写入。
3. **并发与构建坑**：
   - **严禁 Slot A 与 Slot B 同时编译或同时修改 `src/cdp/`**；Slot B 改 `src/cdp/` 时，Slot A 只能做纯 CP 白盒（`test/{dom,css,layout,html,render,net,eval,js,tty}/*.cp`）或纯 JS/TS，且**绝不运行 `cap build`**。
   - **严禁 `pkill -f`**，清理后台服务只按 PID 精确结束。
4. **Prompt 触发安全拦截坑**：
   - 子智能体 Prompt 中避免包含二进制转义魔数（如 `\x89PNG\r\n\x1a\n`）或敏感控制词，使用平实中性工程描述。

---

## 3. 关键约定与约束

1. **严格 2 路并发上限**：最多同时运行 **2 个子智能体**（用户明确要求节约配额）：
   - **Slot A**：纯 CP 白盒测试 / 无 `src/cdp/` 依赖模块（只跑 `cpc fix` + `cpc_brief.py -n 8` + `cap test -m`）。
   - **Slot B**：`src/cdp/` & `src/js/shim/` 修复 + `bin_19233` / 端口 `19233` E2E 验证。
2. **用完即弃**：每次任务完成后立即 `manage_subagents(kill)`，严禁 `send_message` 复用旧上下文。
3. **零锁极速编译与 E2E 命令**：
   - 编译：`ulimit -s 65536 && cap build -m -d -o bin_19233 src/main.cp`（约 6.8s）。
   - 单测 E2E：`CAPSURF_BIN=./bin_19233/main REMOTE_DEBUGGING_PORT=19233 bun test --timeout 15000 <test>`。
   - JS shim 变更后必须先执行 `python3 scripts/bundle_runtime.py` 再编译。
4. **单次原子提交**：每个 Phase 收尾后先更新 `SESSION.md` / `CHANGELOG.md` / `DEVLOG.md`，再精确 `git add` 相关文件执行唯一 1 次 `git commit`。
5. **禁止触碰 `/data/dev/cp/`**：编译器与标准库问题一律按 `/root/.agy/comm/COMM.md` 上报 `cp_agy`。

---

## 4. 下一步行动（Next Step）

新会话启动后，立即并行派发以下 **2 路全新子智能体**：
1. **Slot A**：验收并补全 `test/dom/test_mutation_observer_edge.cp`（运行 `cpc fix`、`cpc_brief.py -n 8`、`cap test -m test/dom/test_mutation_observer_edge.cp`），汇报通过后由主智能体 kill 并提交。
2. **Slot B**：接续 `src/cdp/{screencast,server,kitesurf_facade,page}.cp` 与 `src/js/shim/window.js`：
   - 先修 `src/cdp/screencast.cp:210` `*%=` -> `h = h *% 1099511628211;`；
   - 跑通 `bundle_runtime.py` + `cpc fix` + `cpc_brief.py -n 8` + `cap build -m -d -o bin_19233 src/main.cp`；
   - 在端口 `19233` 跑通 Screencast 4 连发（`screencast-raf-pacing`, `screencast-idle-skip`, `screencast-partial-frames`, `screencast-wheel-coalescing`），并顺带复测 `18f88c2` 队列中的 `tldraw-app-shell` 等 7 个已修复套件。
3. **紧随其后**：Slot B 提交后，Slot A 重跑全量非 CDP `cap test -m`（验证 100+ 白盒全绿），Slot B 依次收尾第 1.3 节列出的 `module-registry` -> `detached-node-reclaim` -> `oopif-*` 4 个精确行号批次。

---

## 5. 经验

1. **双轨隔离分工吞吐极高**：Slot A（纯 CP 白盒）无需 `cap build`，平均 **1~1.5 分钟/文件** 即可完成「写测 + `cpc fix` + 4 门禁 + `cap test -m`」，与 Slot B（`src/cdp/` + `cap build -m -d -o bin_19233`）完全零文件冲突、零端口锁竞争。
2. **集中式小批量 + 精确行号是关键**：把 3~4 个强关联 E2E（共用同一批 `src/cdp/*.cp` 修改）打包给 Slot B、末尾只编译 1 次，可将单次编译压到 **~6.8s**，避免反复全量构建浪费时间和配额。
3. **0ms 自调度定时器防御**：`window.js` 中 `drainOneTask()` 若不加 `isExecutingTask` 守卫，`setTimeout(tick, 0)` 会反复重置 `microtaskDrainBudget` 导致 `ctx.drainJobs()` 空转打满 QuickJS 中断上限（12.8s+ 超时）。

---

## 6. 节省 Token 与高效推进规则

1. **门禁聚合优先**：严禁直接查看 `cpc` 原始全量输出，一律用 `cpc fix <file>` 后接 `python3 scripts/cpc_brief.py -n 8 <file>`（最多 8 条聚焦诊断）。
2. **大文件先定位后分段读**：超过 800 行先用 `rg -n` 提取函数签名与行号，再用 `read_file(StartLine, EndLine)` 精确读取（不超过 150 行），严禁反复整文件读取。
3. **子智能体 Prompt 极简化**：只写「先读 `CP_PITFALLS.md`」+ 明确目标文件/行号/验证命令，不内联大段语法规则。
4. **主智能体零编辑直扫（Batch Sweep）**：对于尚未记录的剩余 E2E 用例（约 40 个），主智能体可用 `timeout 12` 批量直跑一轮，已直接 PASS 的立刻登记提交，只把真实失败的精确断言和行号喂给 Slot B，大幅节省子智能体排查 Token。
5. **不轮询、不猜测**：派发后结束轮次等系统唤醒；所有数字/路径/结果只引用工具实测输出。
