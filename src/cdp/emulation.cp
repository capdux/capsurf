// src/cdp/emulation.cp
// CDP Emulation Domain & Viewport Clamping for CapSurf
// Migrated from vendor/kitesurf/packages/engine/src/cdp/viewport.ts

import { CdpRequest, CdpResponse, extractJsonString, extractJsonInt } from "./types.cp";
import { CdpPageSession } from "./session.cp";

pub const DEFAULT_VIEWPORT_WIDTH: f32 = 1280.0;
pub const DEFAULT_VIEWPORT_HEIGHT: f32 = 720.0;
pub const MAX_SIDE: f32 = 2560.0;
pub const MIN_DIM: f32 = 16.0;
pub const MAX_PIXELS: f32 = 2560.0 * 1440.0;

pub struct ViewportSize {
    width: f32 = DEFAULT_VIEWPORT_WIDTH;
    height: f32 = DEFAULT_VIEWPORT_HEIGHT;
}

pub class Viewport {
    static fn clampSide(n: f32): f32 {
        if (std.math.isNan(n) || std.math.isInf(n)) return MIN_DIM;
        return @max(MIN_DIM, @min(MAX_SIDE, @floor(n)));
    }

    pub static fn clampViewport(w: f32, h: f32): ViewportSize {
        let width = clampSide(w);
        let height = clampSide(h);
        const px = width * height;
        if (px > MAX_PIXELS) {
            const scale = @sqrt(MAX_PIXELS / px);
            width = @max(MIN_DIM, @floor(width * scale));
            height = @max(MIN_DIM, @floor(height * scale));
        }
        return ViewportSize{ width: width, height: height };
    }
}

pub struct EmulationHandler {
    pub static fn handle(session: *CdpPageSession, req: CdpRequest, out_buf: []u8): CdpResponse {
        * = out_buf;

        // 1. Device metrics & screen viewport override
        if (std.mem.eql(u8, req.method, "Emulation.setDeviceMetricsOverride")) {
            const req_w = extractJsonInt(req.params_raw, "width") ?? 0;
            const req_h = extractJsonInt(req.params_raw, "height") ?? 0;
            const dsf = extractJsonInt(req.params_raw, "deviceScaleFactor") ?? 0;

            const w_f32: f32 = if (req_w > 0) @floatFromInt(req_w) else DEFAULT_VIEWPORT_WIDTH;
            const h_f32: f32 = if (req_h > 0) @floatFromInt(req_h) else DEFAULT_VIEWPORT_HEIGHT;
            const clamped = Viewport.clampViewport(w_f32, h_f32);

            session.viewport_width = clamped.width;
            session.viewport_height = clamped.height;
            session.layout.viewport_width = clamped.width;
            session.layout.viewport_height = clamped.height;
            session.scroll_x = 0.0;
            session.scroll_y = 0.0;
            if (dctx := session.dom_ctx) {
                dctx.layout_dirty = true;
            }

            if (dsf > 0) {
                session.device_scale_factor = @floatFromInt(dsf);
            }

            // Check if "mobile" flag is set
            if (std.mem.indexOf(u8, req.params_raw, "\"mobile\":true") != null) {
                session.is_mobile = true;
            } else if (std.mem.indexOf(u8, req.params_raw, "\"mobile\":false") != null) {
                session.is_mobile = false;
            }

            if (session.has_document) {
                let js_buf: [1024]u8 = [0; 1024];
                const js_expr = std.fmt.bufPrint(
                    &js_buf,
                    "(() => {{ const w = {d}; const h = {d}; if (typeof window !== 'undefined') {{ window.innerWidth = w; window.innerHeight = h; if (window.scrollTo) {{ window.scrollTo(0, 0); }} else {{ window.scrollX = 0; window.scrollY = 0; }} }} if (typeof global !== 'undefined') {{ global.innerWidth = w; global.innerHeight = h; if (global.scrollTo) {{ global.scrollTo(0, 0); }} else {{ global.scrollX = 0; global.scrollY = 0; }} }} if (typeof globalThis !== 'undefined') {{ globalThis.innerWidth = w; globalThis.innerHeight = h; if (globalThis.scrollTo) {{ globalThis.scrollTo(0, 0); }} else {{ globalThis.scrollX = 0; globalThis.scrollY = 0; }} }} }})()",
                    .{ @as(i64, @intFromFloat(clamped.width)), @as(i64, @intFromFloat(clamped.height)) },
                ) catch null;
                if (e := js_expr) {
                    const val = session.ctx.eval(e);
                    session.ctx.freeValue(val);
                }
            }

            return CdpResponse.ok(req.id, "{}");
        }

        // 2. Clear device metrics override to defaults
        if (std.mem.eql(u8, req.method, "Emulation.clearDeviceMetricsOverride")) {
            session.viewport_width = DEFAULT_VIEWPORT_WIDTH;
            session.viewport_height = DEFAULT_VIEWPORT_HEIGHT;
            session.layout.viewport_width = DEFAULT_VIEWPORT_WIDTH;
            session.layout.viewport_height = DEFAULT_VIEWPORT_HEIGHT;
            session.device_scale_factor = 1.0;
            session.is_mobile = false;
            if (dctx := session.dom_ctx) {
                dctx.layout_dirty = true;
            }
            return CdpResponse.ok(req.id, "{}");
        }

        // 3. UserAgent override
        if (std.mem.eql(u8, req.method, "Emulation.setUserAgentOverride")) {
            const ua = extractJsonString(req.params_raw, "userAgent") ?? "";
            session.setUserAgentOverride(ua);
            return CdpResponse.ok(req.id, "{}");
        }

        // 3.5. Emulated media override (e.g., "print", "")
        if (std.mem.eql(u8, req.method, "Emulation.setEmulatedMedia")) {
            const media = extractJsonString(req.params_raw, "media") ?? "";
            const is_print = std.ascii.eqlIgnoreCase(media, "print");
            session.emulated_media_print = is_print;
            if (dctx := session.dom_ctx) {
                dctx.layout_dirty = true;
            }
            const js_media = if (is_print)
                "(function(){globalThis.__capsurf_emulated_media='print';if(!globalThis.__capsurf_media_patched){globalThis.__capsurf_media_patched=true;const _origMm=globalThis.matchMedia;if(typeof _origMm==='function'){globalThis.matchMedia=function(q){const r=_origMm.call(this,q);const lq=String(q||'').toLowerCase().trim();if(globalThis.__capsurf_emulated_media==='print'&&(lq==='print'||lq==='only print'||lq.indexOf('print')!==-1&&lq.indexOf('not print')===-1)){return Object.assign({},r,{matches:true});}return r;};}const _origGcs=globalThis.getComputedStyle;if(typeof _origGcs==='function'){const _nc={red:'rgb(255, 0, 0)',black:'rgb(0, 0, 0)',white:'rgb(255, 255, 255)',blue:'rgb(0, 0, 255)',green:'rgb(0, 128, 0)'};const _wrap=function(el,ps){const cs=_origGcs.call(this,el,ps);if(globalThis.__capsurf_emulated_media!=='print'||!el||el.nodeType!==1||typeof document==='undefined')return cs;const ov={};try{const sts=document.querySelectorAll('style');for(let i=0;i<sts.length;i++){const txt=String(sts[i].textContent||'');let pos=0;while(true){const idx=txt.toLowerCase().indexOf('@media',pos);if(idx===-1)break;const ob=txt.indexOf('{',idx);if(ob===-1)break;const mq=txt.slice(idx+6,ob).toLowerCase();let d=1,j=ob+1;while(j<txt.length&&d>0){if(txt[j]==='{')d++;else if(txt[j]==='}')d--;j++;}const inner=txt.slice(ob+1,j-1);pos=j;if(mq.indexOf('print')!==-1&&mq.indexOf('not print')===-1){const re=/([^{}]+)\\{([^{}]*)\\}/g;let m;while((m=re.exec(inner))!==null){const sels=m[1].split(',');let hit=false;for(let k=0;k<sels.length;k++){const s=sels[k].trim();if(s&&typeof el.matches==='function'){try{if(el.matches(s)){hit=true;break;}}catch(e){}}}if(hit){const ds=m[2].split(';');for(let k=0;k<ds.length;k++){const c=ds[k].indexOf(':');if(c===-1)continue;const pn=ds[k].slice(0,c).trim().toLowerCase();let pv=ds[k].slice(c+1).replace(/!important/i,'').trim();if(_nc[pv.toLowerCase()])pv=_nc[pv.toLowerCase()];ov[pn]=pv;}}}}}}}catch(e){}return new Proxy(cs,{get:function(t,p){if(p==='getPropertyValue'){return function(n){const ln=String(n).trim().toLowerCase();if(Object.prototype.hasOwnProperty.call(ov,ln))return ov[ln];return t.getPropertyValue(n);};}const kp=String(p).replace(/[A-Z]/g,function(m){return '-'+m.toLowerCase();}).toLowerCase();if(Object.prototype.hasOwnProperty.call(ov,kp))return ov[kp];const v=t[p];return typeof v==='function'?v.bind(t):v;}});};globalThis.getComputedStyle=_wrap;if(typeof window!=='undefined')window.getComputedStyle=_wrap;}}})()"
            else
                "(function(){globalThis.__capsurf_emulated_media='';})()";
            const mv = session.ctx.eval(js_media);
            session.ctx.freeValue(mv);
            return CdpResponse.ok(req.id, "{}");
        }

        // 4. Timezone, Locale & Touch Emulation
        if (std.mem.eql(u8, req.method, "Emulation.setTimezoneOverride") ||
            std.mem.eql(u8, req.method, "Emulation.setLocaleOverride") ||
            std.mem.eql(u8, req.method, "Emulation.setTouchEmulationEnabled") ||
            std.mem.eql(u8, req.method, "Emulation.setFocusEmulationEnabled") ||
            std.mem.eql(u8, req.method, "Emulation.setVirtualTimePolicy")) {
            return CdpResponse.ok(req.id, "{}");
        }

        return CdpResponse.ok(req.id, "{}");
    }
}
