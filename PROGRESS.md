# CapSurf 会话交接文档 (`PROGRESS.md`)

> 生成时间：2026-11-27 08:04 (AEDT) | 当前基线：Playwright 123/123 已全扫、`test/unit/` 138/138 (100%)、当前工作区有一批已通过 `cpc brief` / `node -c` 门禁但**尚未重编进 `./bin_dev/main`** 的代码修改。

---

## 1. 当前阻塞与未完成项

### A. 已修改并过静态门禁，**待编译 `bin_dev` 并复测**的 7 项成果
1. **`src/js/shim/parser.js`** (`ParsedElement.prototype.attributes` getter) -> 已在 `PW965` 先行重编验证通过（`locator-frame` 达成 **9/9 100%**）。
2. **`src/cdp/session.cp` & `src/cdp/page.cp`** (`c037f812`)：
   - `Page.setDocumentContent` 补充 `pollNavigatedWithinDocEvents`；
   - `NavigationEntry.doc_gen` 同文档 `history.back()`/`goBack()` 不再触发全量网络重载；
   - **待复测**：Puppeteer `navigation.test`（48/61 -> 目标 ~50/61）。
3. **之前已编入 `07:53` `bin_dev/main` 但尚未全部复测的模块**：
   - `serializationOptions` / `deepSerializedValue`（`types.cp`, `dom_binding.cp`, `session.cp`, `session_hooks.cp`, `runtime.cp`）—— 在 `pptr964` 跑 `jshandle.test` 仍余 2 项（`dates` 无原型、`circular objects` 报错）、`evaluation.test` 仍余 5 项，需对照上游 `deepSerializedValue` 返回字段微调。
   - `window.stop()` / `Page.stopLoading`（`window.js`, `session.cp`, `page.cp`）—— `pw964_page-goto` 中 `should not resolve goto upon window.stop()` 仍收到 `true`，需检查客户端 `goto` 为何提前 resolve。
   - `Input.dispatchTouchEvent`（`input.cp`, `event.js`, `window.js`, `index.js`）—— `pptr965_elementhandle.test` 中报 `Input.dispatchTouchEvent timed out`，需排查 `input.cp` / `server.cp` 为何未回包。

### B. 尚未修复的阻塞与剩余缺口
1. **Playwright 临门一脚（差 1–3 项即 100% 的 spec）**：
   - `page-route`（**49/51**）：仅余 `should work with encoded server - 2`（TIMEOUT）与 `should not auto-intercept non-preflight OPTIONS with network interception`（FAIL）。
   - `page-add-locator-handler`（**12/13**）：仅余 `should work when owner frame detaches`（`page.$('iframe')` 仍返回非空句柄）。
   - `selectors-frame`（**13/15**）：2 项 FAIL。
   - `page-request-continue`（**31/36**）：5 项 FAIL（主要为 `should override method along with url`、`continue with headers` cookie、跨站重定向头）。
   - `page-event-network`（**5/6**）、`expect-boolean`（**83/84**）、`elementhandle-wait-for-element-state`（**11/12**）、`page-set-input-files`（**17/19**，余 2 项 `200MB.zip` 受 Playwright `isLocal: false` 拦截）、`page-filechooser`（**5/6**）、`frame-hierarchy`（**11/12**）、`page-event-pageerror`（**6/7**，QuickJS `new Error` 列号 `14:15` vs V8 `14:11`）、`page-set-extra-http-headers`（**4/5**）。
2. **Playwright 中大型剩余块**：
   - `page-screenshot`（**26/44**）：原生 `ShadowRoot` + `adopted_css` + `[popover]` 已接入，`mask option` 仍余像素差异；另 `should capture blinking caret in shadow dom` 报 `element is not visible`；WebP/JPEG 基线文件实际为 PNG 魔数需更新。
   - `page-aria-snapshot-ai`（**29/36**, 0 TIMEOUT）、`page-goto`（**57/61**, 0 TIMEOUT）、`page-click`（**85/102**）、`page-evaluate`（**120/131**）、`page-network-request`（**7/27**）、`page-network-idle`（**2/10**）。
3. **Puppeteer 剩余缺口**：
   - `click.test`（**25/26 + 4 SKIP**）：仅余 `deviceScaleFactor set`（`DOM.resolveNode` 跨文档报错，需复测最新 `dom.cp` `contentDocument` `bid >= 1000000`）。
   - `elementhandle.test`（**41/48 + 2 SKIP**）：5 项 `dispatchTouchEvent timed out` + 2 项 `iframe clickablePoint`。
   - `jshandle.test`（23/25）、`evaluation.test`（42/47）、`navigation.test`（48/61）、`waittask.test`（57/62）。
4. **未 Commit 的工作区改动**：当前大量阶段修改尚在工作区未执行 `git commit`，待下一会话先编译验证后统一原子提交。

---

## 2. 已排除的死胡同（避坑记录）

1. **`pw-run.mjs` / `pptr-run.mjs` 位置参数失效**：直接传 `pw-run.mjs page-route` 会忽略参数并重跑全量 **123 个 spec**！必须用 `--spec page-route.spec.ts --results-out tmp/xxx.json --out-dir tmp/xxx`（且不带 `tests/page/` 前缀）。
2. **`MAX_TAB_SESSIONS` 并非 `page-request-fallback` 第 14 次 `newPage()` 断连根因**：真实根因是第 13 个用例发送 `0..255` 二进制 `postData` 时 `escapeJsonString` 直接写入非法 UTF-8 字节导致 WS 客户端断开（已在 `session.cp` & `fetch.cp` 修复并验证 **14/14 100%**）。
3. **`ShadowRoot.prototype.adoptedStyleSheets` 在 `cssom.js` 中定义无效**：因 `bundle_runtime.py` 先打包 `cssom.js` 后打包 `node-base.js`，`cssom.js` 执行时 `global.ShadowRoot` 尚为 `undefined`；必须在 `node-base.js` 定义 `ShadowRoot` 后挂载。
4. **`WorkerCdpEventItem.asCdpEvent()` 省略接收者导致悬垂栈切片**：`fn asCdpEvent(): CdpEvent` 按值拷贝后返回局部栈切片，导致 WS 发送全空格乱码 JSON；必须写 `fn asCdpEvent(*_)`。
5. **`evalInIframe` 尾分号 IIFE 返回 `undefined`**：`(() => { ... })();` 含尾 `;` 时 `compileIframeFn("return (" + expr + ");")` 失败后，直接用 `lastIndexOf(";")` 拆函数体会全部返回 `undefined`；必须先尝试 `return (trimmed)`。
6. **Zig 0.17 标准库移除项**：`std.ascii.indexOfIgnoreCase`、`std.fs.cwd()`、`std.Thread.sleep()`、`std.mem.trimRight()` 均不存在；分别改用 `std.ascii.eqlIgnoreCase` 循环、`std.c.opendir`、`@c { fn usleep }`、`std.mem.trim`。
7. **CP 借用检查 (`E0202`/`E0200`)**：`const x = std.fmt.allocPrint(...) catch ""` 会推导为 `[]const u8` 无法赋给 `([]u8)?`，需用 `if (d := allocPrint(...) catch null)`；`if (ctx := dom_ctx.raw_ctx)` 内调用 `jsToSlice(ctx, val)` 赋给外层切片会报生命周期不足，改为复制到栈缓冲或直接传 `dom_ctx.raw_ctx.?`。

---

## 3. 关键约定与约束（已固化进 `GEMINI.md` 11.14 节）

1. 始终保持 **3 路全新子智能体** 并行推进，严禁复用已完成上下文；主智能体不可空闲等待或陷入长阻塞。
2. **已 100% PASS 的 spec（在 `test/stats.md` 标注 `已全过不再重测`）严禁重复测试**。
3. 所有临时文件、测试报告、日志统一放入 `/data/dev/capsurf/tmp/`（`export TMPDIR=/data/dev/capsurf/tmp`）。
4. 禁止在同一 `--port` 并发运行两个 `pw-run.mjs` / `pptr-run.mjs`；统一使用 `CAPSURF_BIN=./bin_dev/main`。
5. `vendor/browser`（Lightpanda，**AGPL-3.0-only**）只读参考测试场景与架构思路，严禁复制代码；`vendor/kitesurf`（Apache-2.0，已拉取至 `3d2ed9a`）可直接借鉴/迁移修复与测试用例。
6. **3 小时定时审计任务 `task-42302` (`0 */3 * * *`)** 已注册为 Daemon（新会话可按需重新注册或检查状态）。

---

## 4. 下一步行动（Next Step）

新会话启动后，请按以下顺序**立即执行**：
1. **合批编译并冒烟**：
   ```bash
   python3 scripts/bundle_runtime.py && /data/dev/cp/cap/bin/cap build -m -d --out-dir=bin_dev src/main.cp && export TMPDIR=/data/dev/capsurf/tmp && CAPSURF_BIN=./bin_dev/main bun test test/e2e/smoke.e2e.test.ts
   ```
2. **立即验证本轮待验修复（端口 `:9946`，结果落盘 `tmp/pw966_*.json` / `tmp/pptr966_*.json`）**：
   - Puppeteer：`navigation.test`（验证 `c037f812` HistoryAPI 修复）与 `click.test`（确认 `deviceScaleFactor` 是否到 26/26）。
   - 并同步派发 **3 路子智能体**（文件零冲突）：
     - 路 1：排查 `Input.dispatchTouchEvent timed out`（`src/cdp/input.cp` 第 1060-1170 行与 `src/cdp/server.cp`）；
     - 路 2：排查 `page-add-locator-handler::should work when owner frame detaches` 与 `page-route` 剩余 2 项（`encoded server - 2`、`OPTIONS with network interception`）；
     - 路 3：排查 `jshandle.test`（`dates` / `circular objects`）与 `evaluation.test` 的 `deepSerializedValue` 返回细节（`src/js/dom_binding.cp` / `src/cdp/runtime.cp`）。
3. **验证通过后及时更新 `test/stats.md`、`SESSION.md`、`CHANGELOG.md`、`DEVLOG.md` 并执行 1 次原子 `git commit`** 固化当前庞大工作区成果！

---

## 5. 经验总结（供新会话直接沿用）

1. **找准 10x 级共性根因比逐个修测试高效 10 倍**：本次仅凭 `TCP_NODELAY`（消 40ms/包延迟）、`__capsurf_get_root_scroll_size`（`scrollWidth` 从 5.28s 降到 <5ms）、`evalInIframe` 尾分号（一次清零 21 个子 frame 超时）、`FRAME_WINDOW_GLOBALS`（补齐 `getComputedStyle`/`eval`）和 `escapeJsonString` UTF-8 校验，就连续将 `workers`、`locator-frame`、`page-request-fallback`、`page-request-fulfill`、`page-dispatchevent`、`page-aria-snapshot`、`page-mouse`、`page-add-init-script`、`locator-click`、`queryselector.test`、`locator.test` 全部推至 100% PASS。
2. **充分利用 `DEBUG=pw:protocol,pw:browser` 与 `gdb`**：遇到客户端莫名断连或挂起时，不要猜，直接用单测 `--grep` + `DEBUG=pw:protocol` 或 `gdb` 抓一行现场即可 100% 精准定位。
3. **借力已同步的 `vendor/kitesurf` (`3d2ed9a`) 与 `vendor/browser` 测试资产**：Kitesurf 最新提交中的 `12453f2`（节点采纳重键）、`323c036`（微任务排空）、`busy-page-network-idle` 与 Lightpanda `src/browser/tests/cdp/` 用例可直接指导后续攻克 `page-network-idle`、`page-goto` 并发取消与 `page-screenshot` 剩余难关。
