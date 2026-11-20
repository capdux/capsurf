import {
    ParsedImportMap,
    ImportMapBudget,
    ImportMapRule,
    ImportMapScopeGroup,
    isUrlLikeSpecifier,
    astroIslandModuleUrls,
    extractModulePreloadSeeds,
    buildFallbackModuleResolverSource,
} from "../../src/eval/walk_module_graph.cp";

fn assert(cond: bool, msg: []const u8) {
    if (!cond) {
        std.debug.print("Assertion failed: {s}\n", .{msg});
        @panic("assertion failed");
    }
}

pub fn main() {
    // 1. Test isUrlLikeSpecifier
    {
        assert(isUrlLikeSpecifier("/root/mod.js"), "absolute path is url-like");
        assert(isUrlLikeSpecifier("./local.js"), "./ path is url-like");
        assert(isUrlLikeSpecifier("../parent.js"), "../ path is url-like");
        assert(isUrlLikeSpecifier("https://cdn.esm.sh/react"), "https:// is url-like");
        assert(isUrlLikeSpecifier("http://localhost:3000/app.js"), "http:// is url-like");
        assert(!isUrlLikeSpecifier("react"), "bare specifier is not url-like");
        assert(!isUrlLikeSpecifier("@lodash/es"), "scoped bare specifier is not url-like");
    }

    // 2. Test ParsedImportMap: imports and prefix rules
    {
        let map: ParsedImportMap = ParsedImportMap{};
        assert(map.addImport("react", "https://esm.sh/react@18", false), "add react import");
        assert(map.addImport("lodash/", "https://esm.sh/lodash-es/", false), "add lodash/ prefix import");
        assert(map.addImport("blocked-mod", "", true), "add blocked module");

        let budget: ImportMapBudget = ImportMapBudget{};
        let buf: [512]u8 = [0; 512];

        // Exact match
        let r1 = map.resolve("react", "https://example.com/index.js", &budget, &buf);
        assert(r1 != null, "react should resolve");
        assert(std.mem.eql(u8, r1.?, "https://esm.sh/react@18"), "react resolved url matches");

        // Prefix match
        let r2 = map.resolve("lodash/map.js", "https://example.com/index.js", &budget, &buf);
        assert(r2 != null, "lodash/map.js should resolve via prefix");
        assert(std.mem.eql(u8, r2.?, "https://esm.sh/lodash-es/map.js"), "lodash resolved url matches");

        // Blocked module
        let r3 = map.resolve("blocked-mod", "https://example.com/index.js", &budget, &buf);
        assert(r3 == null, "blocked module should return null");

        // Bare specifier not in map
        let r4 = map.resolve("unknown-pkg", "https://example.com/index.js", &budget, &buf);
        assert(r4 == null, "unmapped bare specifier should return null");

        // URL-like specifier fallback
        let r5 = map.resolve("./components/button.js", "https://example.com/index.js", &budget, &buf);
        assert(r5 != null, "relative specifier should resolve to itself");
        assert(std.mem.eql(u8, r5.?, "./components/button.js"), "relative specifier matches");
    }

    // 3. Test ParsedImportMap Scopes
    {
        let map: ParsedImportMap = ParsedImportMap{};
        assert(map.addImport("utils", "https://cdn.com/utils-global.js", false), "global utils");

        let scope_opt = map.addScope("https://example.com/admin/");
        assert(scope_opt != null, "add admin scope");
        let scope = scope_opt.?;
        assert(scope.addRule("utils", "https://cdn.com/utils-admin.js", false), "admin utils");

        let budget: ImportMapBudget = ImportMapBudget{};
        let buf_scope: [512]u8 = [0; 512];

        // Resolve from inside admin scope
        let r_admin = map.resolve("utils", "https://example.com/admin/dashboard.js", &budget, &buf_scope);
        assert(r_admin != null, "admin utils should resolve");
        assert(std.mem.eql(u8, r_admin.?, "https://cdn.com/utils-admin.js"), "admin utils overrides global");

        // Resolve from outside admin scope
        let r_public = map.resolve("utils", "https://example.com/public/index.js", &budget, &buf_scope);
        assert(r_public != null, "public utils should resolve");
        assert(std.mem.eql(u8, r_public.?, "https://cdn.com/utils-global.js"), "public utils uses global mapping");
    }

    // 4. Test Astro Island URLs Extraction
    {
        let html = "<div><astro-island component-url=\"/_astro/Hero.123.js\" props=\"{}\"></astro-island>" ++
                   "<span>Text</span><astro-island component-url=\"/_astro/Nav.456.js\"></astro-island></div>";
        let urls: [16][]const u8 = [""; 16];
        let count = astroIslandModuleUrls(html, &urls);
        assert(count == 2, "extracted 2 astro islands");
        assert(std.mem.eql(u8, urls[0], "/_astro/Hero.123.js"), "hero url matches");
        assert(std.mem.eql(u8, urls[1], "/_astro/Nav.456.js"), "nav url matches");
    }

    // 5. Test Module Preload Seeds
    {
        let html = "<head><link rel=\"stylesheet\" href=\"style.css\">" ++
                   "<link rel=\"modulepreload\" href=\"/assets/vendor.js\">" ++
                   "<link rel=\"modulepreload\" href=\"/assets/main.js\"></head>";
        let urls_preload: [16][]const u8 = [""; 16];
        let count = extractModulePreloadSeeds(html, &urls_preload);
        assert(count == 2, "extracted 2 modulepreloads");
        assert(std.mem.eql(u8, urls_preload[0], "/assets/vendor.js"), "vendor modulepreload matches");
        assert(std.mem.eql(u8, urls_preload[1], "/assets/main.js"), "main modulepreload matches");
    }

    // 6. Test Fallback Module Resolver Source
    {
        let buf_resolver: [1024]u8 = [0; 1024];
        let src = buildFallbackModuleResolverSource(&buf_resolver);
        assert(src.len > 0, "fallback resolver source is not empty");
        assert(std.mem.indexOf(u8, src, "export function resolveImportMap") != null, "has resolveImportMap export");
    }

    std.debug.print("test_walk_module_graph: ALL ASSERTIONS PASSED\n", .{});
}
