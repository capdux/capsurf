
## Phase 154 OOPIF 截图合成、样式表网络事件与 Screencast 并发窗口方案 (`src/cdp/`)
1. **按需光栅化与 OOPIF 合成 (`session.cp`)**：仅在 `!skip_raster` 导航完成时标记 `renderer_active`，在截图阶段对跨源 `<iframe>` 执行子文档解析/定时推进/渲染并解压 IDAT 注入 `canvas_bitmaps`。
2. **Screencast 并发窗口 (`screencast.cp`, `server.cp`)**：解析 `maxInFlight`/`maxFramesInFlight`，用 `inFlight() < max_in_flight` 替换单帧 `hasPendingAck()` 阻塞。

## Phase 155 原生 `fetch`/`setTimeout` 隔离与 `evaluateOnNewDocument` 方案 (`src/js/shim/`, `e2e/harness.ts`)
1. **防篡改原生句柄**：在全局初始化阶段保存 `__capsurf_native_fetch` 与 `__capsurf_native_setTimeout`，内部图片/iframe/脚本加载及过渡快照调度统一走原生通道，不触发页面覆写钩子也不增加页面定时器计数。
