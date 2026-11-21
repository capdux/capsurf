// test/css/test_transitions_keyframes.cp
// Pure CP white-box unit tests for CSS Transitions, Animations, EasingFunction (cubic-bezier / steps),
// Time, and @keyframes parsing via cap:css.

import { test, expect_eq } from "cap:test";
import {
    EasingFunction,
    EasingKind,
    StepPosition,
    Time,
    TransitionItem,
    AnimationItem,
    CssKeyframes,
    CssStylesheet,
    CssRuleKind,
} from "cap:css";

fn nearF64(actual: f64, expected: f64, eps: f64): bool {
    const diff = if (actual > expected) (actual - expected) else (expected - actual);
    return diff <= eps;
}

fn main() {
    test("1. EasingFunction parsing, cubic-bezier & steps evaluation, and formatting", () => {
        if (lin := EasingFunction.parse("linear")) {
            expect_eq(lin.kind == EasingKind.Linear, true);
            expect_eq(nearF64(lin.eval(0.0), 0.0, 0.0001), true);
            expect_eq(nearF64(lin.eval(0.25), 0.25, 0.0001), true);
            expect_eq(nearF64(lin.eval(0.75), 0.75, 0.0001), true);
            expect_eq(nearF64(lin.eval(1.0), 1.0, 0.0001), true);
        } else {
            expect_eq(false, true);
        }

        if (ez := EasingFunction.parse("ease")) {
            expect_eq(ez.isEase(), true);
            expect_eq(nearF64(ez.eval(0.0), 0.0, 0.0001), true);
            expect_eq(nearF64(ez.eval(1.0), 1.0, 0.0001), true);
            // At t = 0.5, standard CSS ease is ~0.802
            const mid: f64 = ez.eval(0.5);
            expect_eq(mid > 0.7 && mid < 0.9, true);
        } else {
            expect_eq(false, true);
        }

        if (cb := EasingFunction.parse("cubic-bezier(0.1, 0.7, 1.0, 0.1)")) {
            expect_eq(cb.kind == EasingKind.CubicBezier, true);
            expect_eq(nearF64(cb.eval(0.0), 0.0, 0.0001), true);
            expect_eq(nearF64(cb.eval(1.0), 1.0, 0.0001), true);
            const mid_cb: f64 = cb.eval(0.5);
            expect_eq(mid_cb > 0.0 && mid_cb < 1.0, true);

            let buf: [64]u8 = [0; 64];
            const formatted: []const u8 = cb.format(buf[0..64], false);
            expect_eq(std.mem.startsWith(u8, formatted, "cubic-bezier("), true);
        } else {
            expect_eq(false, true);
        }

        // Invalid x1/x2 outside [0, 1] must return null
        expect_eq(EasingFunction.parse("cubic-bezier(-0.1, 0.0, 1.0, 1.0)") == null, true);
        expect_eq(EasingFunction.parse("cubic-bezier(0.0, 0.0, 1.2, 1.0)") == null, true);
        expect_eq(EasingFunction.parse("unknown-easing") == null, true);

        if (st := EasingFunction.parse("steps(4, start)")) {
            expect_eq(st.kind == EasingKind.Steps, true);
            expect_eq(st.step_count, @as(i32, 4));
            expect_eq(st.step_pos == StepPosition.Start, true);
            expect_eq(nearF64(st.eval(0.0), 0.25, 0.0001), true);
            expect_eq(nearF64(st.eval(0.3), 0.5, 0.0001), true);
            expect_eq(nearF64(st.eval(0.99), 1.0, 0.0001), true);
        } else {
            expect_eq(false, true);
        }

        if (st_none := EasingFunction.parse("steps(3, jump-none)")) {
            expect_eq(nearF64(st_none.eval(0.0), 0.0, 0.0001), true);
            expect_eq(nearF64(st_none.eval(0.4), 0.5, 0.0001), true);
            expect_eq(nearF64(st_none.eval(1.0), 1.0, 0.0001), true);
        } else {
            expect_eq(false, true);
        }

        // jump-none requires count >= 2
        expect_eq(EasingFunction.parse("steps(1, jump-none)") == null, true);
    });

    test("2. TransitionItem & Time parsing and formatting", () => {
        if (t_ms := Time.parse("250ms")) {
            if (t_s := Time.parse("0.25s")) {
                expect_eq(t_ms.eql(t_s), true);
                expect_eq(nearF64(t_ms.toMs(), 250.0, 0.001), true);
                expect_eq(nearF64(t_s.toSeconds(), 0.25, 0.001), true);
            } else {
                expect_eq(false, true);
            }
        } else {
            expect_eq(false, true);
        }

        if (tr1 := TransitionItem.parse("opacity 300ms ease-in 50ms")) {
            expect_eq(std.mem.eql(u8, tr1.property, "opacity"), true);
            expect_eq(nearF64(tr1.duration.toMs(), 300.0, 0.001), true);
            expect_eq(tr1.timing_function.kind == EasingKind.EaseIn, true);
            expect_eq(nearF64(tr1.delay.toMs(), 50.0, 0.001), true);

            let out: [128]u8 = [0; 128];
            const fmt_str: []const u8 = tr1.format(out[0..128], false);
            expect_eq(std.mem.indexOf(u8, fmt_str, "opacity") != null, true);
            expect_eq(std.mem.indexOf(u8, fmt_str, "ease-in") != null, true);
        } else {
            expect_eq(false, true);
        }

        if (tr2 := TransitionItem.parse("transform 0.5s cubic-bezier(0.4, 0, 0.2, 1)")) {
            expect_eq(std.mem.eql(u8, tr2.property, "transform"), true);
            expect_eq(nearF64(tr2.duration.toMs(), 500.0, 0.001), true);
            expect_eq(tr2.timing_function.kind == EasingKind.CubicBezier, true);
            expect_eq(tr2.timing_function.eval(0.5) > 0.5, true);
        } else {
            expect_eq(false, true);
        }

        if (tr3 := TransitionItem.parse("all 150ms")) {
            expect_eq(std.mem.eql(u8, tr3.property, "all"), true);
            expect_eq(nearF64(tr3.duration.toMs(), 150.0, 0.001), true);
            expect_eq(tr3.timing_function.isEase(), true);
            expect_eq(tr3.delay.isZero(), true);
        } else {
            expect_eq(false, true);
        }

        expect_eq(TransitionItem.parse("   ") == null, true);
    });

    test("3. AnimationItem & @keyframes parsing (CssKeyframes + CssStyleSheet)", () => {
        if (anim := AnimationItem.parse("fade-in 1.2s ease-out 100ms infinite alternate forwards running")) {
            expect_eq(std.mem.eql(u8, anim.name, "fade-in"), true);
            expect_eq(nearF64(anim.duration.toMs(), 1200.0, 0.001), true);
            expect_eq(anim.timing_function.kind == EasingKind.EaseOut, true);
            expect_eq(nearF64(anim.delay.toMs(), 100.0, 0.001), true);
            expect_eq(std.mem.eql(u8, anim.iteration_count, "infinite"), true);
            expect_eq(std.mem.eql(u8, anim.direction, "alternate"), true);
            expect_eq(std.mem.eql(u8, anim.fill_mode, "forwards"), true);
            expect_eq(std.mem.eql(u8, anim.play_state, "running"), true);
        } else {
            expect_eq(false, true);
        }

        const kf_src = "@keyframes slideIn { from { opacity: 0; transform: translateX(-20px); } 50% { opacity: 0.5; } to { opacity: 1; transform: translateX(0); } }";
        if (kf := CssKeyframes.parse(kf_src)) {
            expect_eq(std.mem.eql(u8, kf.name, "slideIn"), true);
            expect_eq(kf.step_count, @as(usize, 3));

            expect_eq(nearF64(kf.steps[0].offset_percent, 0.0, 0.001), true);
            expect_eq(kf.steps[0].decl_count, @as(usize, 2));
            expect_eq(std.mem.eql(u8, kf.steps[0].declarations[0].name, "opacity"), true);
            expect_eq(std.mem.eql(u8, kf.steps[0].declarations[0].value, "0"), true);

            expect_eq(nearF64(kf.steps[1].offset_percent, 50.0, 0.001), true);
            expect_eq(kf.steps[1].decl_count, @as(usize, 1));
            expect_eq(std.mem.eql(u8, kf.steps[1].declarations[0].value, "0.5"), true);

            expect_eq(nearF64(kf.steps[2].offset_percent, 100.0, 0.001), true);
            expect_eq(kf.steps[2].decl_count, @as(usize, 2));
            expect_eq(std.mem.eql(u8, kf.steps[2].declarations[1].name, "transform"), true);
        } else {
            expect_eq(false, true);
        }

        expect_eq(CssKeyframes.parse(".not-keyframes { color: red; }") == null, true);

        const sheet: CssStylesheet = CssStylesheet.parse("@keyframes pulse { 0% { opacity: 1; } 100% { opacity: 0; } } .box { animation: pulse 1s linear; }");
        expect_eq(sheet.rule_count >= @as(usize, 2), true);
        expect_eq(sheet.rules[0].kind == CssRuleKind.Keyframes, true);
        expect_eq(sheet.rules[1].kind == CssRuleKind.Style, true);
    });
}
