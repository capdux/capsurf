// src/render/style_extractor.cp
import css from "cap:css";
import cascade from "../css/cascade.cp";
import painter from "./painter.cp";

const CssColor = css.css_values.color.CssColor;
const RGBA = css.css_values.color.RGBA;
const BorderSideWidth = css.css_properties.border.BorderSideWidth;
const LengthPercentage = css.css_values.length.LengthPercentage;

/// 将 cap:css 的 CssColor 联合体精确转换为 8-bit RGBA 像素颜色
pub fn resolveCssColor(color: CssColor, current_color: painter.Color): painter.Color {
    return switch (color) {
        // 1. currentColor 关键字：继承元素当前的文本 color 属性
        .current_color => current_color,

        // 2. 标准 RGBA (u8 通道: r, g, b, a) — 涵盖 #hex, rgb(), rgba() 及命名颜色
        .rgba => |rgba| painter.Color{
            r: rgba.r,
            g: rgba.g,
            b: rgba.b,
            a: rgba.a,
        },

        // 3. 浮点色彩空间 (srgb, hsl, hwb, lab, lch, oklab, oklch)
        // 调用 cap:css 内置的 toRGB() 统一折算为 RGBA
        .srgb, .hsl, .hwb, .lab, .lch, .oklab, .oklch => {
            if (rgb_color := color.toRGB()) {
                return resolveCssColor(rgb_color, current_color);
            }
            return painter.Color.rgba(0, 0, 0, 255);
        },

        // 4. light-dark() 现代亮暗色函数（无头默认取 light 分支）
        .light_dark => |ld| resolveCssColor(ld.light.*, current_color),

        // 5. 系统颜色或未知回退为黑色
        * => painter.Color.rgba(0, 0, 0, 255),
    };
}

/// 解析 border-width (thin=1px, medium=3px, thick=5px, length=px)
pub fn resolveBorderWidth(bw: BorderSideWidth): f32 {
    return switch (bw) {
        .thin => 1.0,
        .medium => 3.0,
        .thick => 5.0,
        .length => |len| len.toPx() ?? 0.0,
    };
}

/// 解析 border-radius (支持 px 绝对长度与相对于盒子宽度的百分比)
pub fn resolveBorderRadius(lp: LengthPercentage, box_size: f32): f32 {
    return switch (lp) {
        .dimension => |d| d.toPx() ?? 0.0,
        .percentage => |pct| pct.v * box_size,
        .calc => 0.0,
    };
}

/// 从节点的级联计算结果 ComputedStyle 提取完整光栅化绘制样式 PaintStyle
pub fn extractPaintStyle(
    computed: cascade.ComputedStyle,
    parent_text_color: painter.Color,
    box_width: f32
): painter.PaintStyle {
    // 1. 先解析文本颜色 color（支持继承父节点文本颜色；作为 currentColor 的基准）
    let text_color = parent_text_color;
    if (c_prop := computed.color) {
        text_color = resolveCssColor(c_prop, parent_text_color);
    }

    // 2. 解析背景色 background-color（默认全透明 rgba(0,0,0,0)）
    let bg_color = painter.Color.rgba(0, 0, 0, 0);
    if (bg_prop := computed.background_color) {
        bg_color = resolveCssColor(bg_prop, text_color);
    }

    // 3. 解析边框颜色 border-top-color（若未显式指定则按 CSS 规范回退至 currentColor）
    let border_color = text_color;
    if (bc_prop := computed.border_top_color) {
        border_color = resolveCssColor(bc_prop, text_color);
    }

    // 4. 解析边框宽度（需检查 border-style 不为 none/hidden 才生效）
    let border_width: f32 = 0.0;
    if (bs_prop := computed.border_top_style) {
        if (bs_prop != .none && bs_prop != .hidden) {
            if (bw_prop := computed.border_top_width) {
                border_width = resolveBorderWidth(bw_prop);
            } else {
                border_width = 3.0; // CSS 默认 medium = 3px
            }
        }
    }

    // 5. 解析圆角半径 border-top-left-radius
    let border_radius: f32 = 0.0;
    if (br_prop := computed.border_top_left_radius) {
        border_radius = resolveBorderRadius(br_prop.0, box_width);
    }

    // 6. 若存在 opacity (< 1.0)，对整节点通道 Alpha 进行衰减合成
    if (opacity_val := computed.opacity) {
        const op = std.math.clamp(opacity_val, 0.0, 1.0);
        if (op < 1.0) {
            bg_color.a = @intFromFloat(@as(f32, @floatFromInt(bg_color.a)) * op);
            border_color.a = @intFromFloat(@as(f32, @floatFromInt(border_color.a)) * op);
            text_color.a = @intFromFloat(@as(f32, @floatFromInt(text_color.a)) * op);
        }
    }

    return painter.PaintStyle{
        bg_color: bg_color,
        border_color: border_color,
        text_color: text_color,
        border_width: border_width,
        border_radius: border_radius,
    };
}
