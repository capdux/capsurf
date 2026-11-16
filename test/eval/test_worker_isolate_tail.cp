import { pageIsolateEntry, ENTRY_MODULE } from "../../src/eval/page_isolate_entry.cp";
import { PageScriptTail, TailLogRecord, formatOtelRecord, ISOLATE_KILL_MARKER, HUNG_INVOCATION_TEXT } from "../../src/eval/page_script_tail.cp";
import {
    WorkerUpstreamEvent,
    WorkerState,
    WorkerUpstreamMessage,
    SpawnDedicatedWorkerOptions,
    DedicatedWorker,
    DedicatedWorkerRegistry,
    hasUseStrict,
    buildWorkerScriptModule,
    upstreamEventToString,
    parseUpstreamEvent,
} from "../../src/eval/dedicated_worker.cp";

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("assertion failed");
    }
}

pub fn main() {
    // 1. Test pageIsolateEntry
    {
        let buf: [512]u8 = [0; 512];
        let res = pageIsolateEntry("HostApp", "app_worker.js", &buf);
        assert(std.mem.eql(u8, res, ENTRY_MODULE), "should return __ks_entry.js");
        let expected = "export * from \"page.js\";\nexport { default } from \"page.js\";\nexport { HostApp } from \"app_worker.js\";\n";
        assert(std.mem.startsWith(u8, buf[0..expected.len], expected), "entry file body matches");

        // Invalid identifier fallback
        let fallback = pageIsolateEntry("123Invalid!", "app.js", &buf);
        assert(std.mem.eql(u8, fallback, "page.js"), "invalid identifier should fallback to page.js");
    }

    // 2. Test PageScriptTail
    {
        let tail: PageScriptTail = PageScriptTail{};
        tail.addLog("info", "Worker initialized successfully", 1000);
        tail.addLog("debug", "Processing subtask 42", 1005);
        assert(tail.record_count == 2, "record count is 2");
        assert(!tail.kill_requested, "no kill marker yet");
        assert(!tail.hung_detected, "no hung marker yet");

        // Test marker detection
        tail.addLog("error", "Engine fatal: [ks-kill] force terminating isolate", 1010);
        assert(tail.kill_requested, "kill marker should be detected");

        tail.addLog("warn", "Worker warning: detected that your Worker's code had hung in loop", 1015);
        assert(tail.hung_detected, "hung worker should be detected");

        // Test OTEL formatting
        let otel_buf: [512]u8 = [0; 512];
        let otel_str = formatOtelRecord("INFO", "Telemetry event", 1700000000, &otel_buf);
        assert(otel_str.len > 0, "otel record should not be empty");
        assert(std.mem.indexOf(u8, otel_str, "\"severityText\":\"INFO\"") != null, "otel severity matches");
        assert(std.mem.indexOf(u8, otel_str, "\"body\":{\"stringValue\":\"Telemetry event\"}") != null, "otel body matches");
    }

    // 3. Test dedicated_worker
    {
        // Event strings
        assert(std.mem.eql(u8, upstreamEventToString(.WorkerMessage), "worker.message"), "message ev string");
        assert(std.mem.eql(u8, upstreamEventToString(.WorkerError), "worker.error"), "error ev string");
        assert(std.mem.eql(u8, upstreamEventToString(.WorkerClosed), "worker.closed"), "closed ev string");

        let p_ev = parseUpstreamEvent("worker.message");
        assert(p_ev != null && p_ev.? == .WorkerMessage, "parse worker.message");

        // Module packaging
        let code1 = "console.log('worker start');";
        let mod_buf: [1024]u8 = [0; 1024];
        let wrapped1 = buildWorkerScriptModule(code1, &mod_buf);
        assert(std.mem.indexOf(u8, wrapped1, "module.exports.run = async function () {") != null, "has run wrapper");
        assert(std.mem.indexOf(u8, wrapped1, "console.log('worker start');") != null, "contains original source");

        // Strict mode detection
        let strict_code = "\"use strict\";\nlet x = 10;";
        assert(hasUseStrict(strict_code), "should detect use strict");
        let non_strict_code = "var a = 1;";
        assert(!hasUseStrict(non_strict_code), "should not detect use strict");

        // Worker lifecycle
        let worker: DedicatedWorker = DedicatedWorker{};
        worker.init("worker-session-1", "worker-thread", 100);
        assert(std.mem.eql(u8, worker.getId(), "worker-session-1"), "worker id matches");
        assert(std.mem.eql(u8, worker.getLabel(), "worker-thread"), "worker label matches");
        assert(!worker.deliverMessage("test payload"), "deliverMessage should fail in Init state");

        assert(worker.start(), "start worker");
        assert(worker.deliverMessage("ping"), "deliverMessage should succeed in Running state");
        assert(worker.messages_sent == 1, "messages_sent count incremented");

        * = worker.recordUpstream(.WorkerMessage);
        assert(worker.messages_received == 1, "messages_received count incremented");

        * = worker.recordUpstream(.WorkerError);
        assert(worker.errors_count == 1, "errors_count incremented");

        worker.terminate("Page closed", false);
        assert(!worker.deliverMessage("ping after close"), "deliverMessage should fail after termination");
    }

    // 4. Test DedicatedWorkerRegistry
    {
        let registry: DedicatedWorkerRegistry = DedicatedWorkerRegistry{};
        assert(registry.activeCount() == 0, "initial active count is 0");

        let opts: SpawnDedicatedWorkerOptions = SpawnDedicatedWorkerOptions{
            connection_id: "conn-99",
            label: "parser-worker",
            cpu_ms: 200,
        };

        let w_idx_opt = registry.spawn(opts);
        assert(w_idx_opt != null, "spawn should succeed");
        let w_idx = w_idx_opt.?;
        assert(registry.activeCount() == 1, "active count is 1");

        let w_ptr = registry.getWorker(w_idx);
        assert(w_ptr != null, "getWorker returns valid pointer");
        assert(std.mem.eql(u8, w_ptr.?.getId(), "conn-99"), "worker ID matches spawned options");

        assert(registry.terminateWorker(w_idx, "done", false), "terminate worker");
        assert(registry.activeCount() == 0, "active count is 0 after termination");
        assert(registry.getWorker(w_idx) == null, "terminated worker slot is freed");
    }

    std.debug.print("test_worker_isolate_tail: ALL ASSERTIONS PASSED\n", .{});
}
