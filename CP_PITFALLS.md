# CP 迁移踩坑速查与权威契约手册 (CP_PITFALLS.md)

> **子智能体唯一必读速查指南（约 4KB）**。参照 `/data/dev/cp/cp_agent_ref.md` 最新权威契约与 Bun 重构实战经验提炼。**子智能体严禁全文读取 68KB 的 `cp_agent_ref.md`！**

---

## A. CP 权威语法核心契约与 Top 29 禁手速查

1. **方法接收者与可见性**：
   - 彻底废除 `self`/`this`（`E0121`）。方法体内访问当前实例统一用 `_`；
   - 只读方法**省略首参** `fn foo(): T`（形参写 `(_)` 报 `E0123`）；可变修改首参写 `*_`（如 `fn set(*_, val: i32)`）；消耗所有权首参写 `move _` 或 `own _`；
   - **`struct` / `enum` 内部成员与方法默认全部 `pub`**（内部直接写 `fn` / `static fn`，**严禁**写冗余 `pub`）；**`class` 成员默认 `priv`**（公开成员/方法才加 `pub`）；`static class`/`static struct` 严禁实例化与声明 `init`/`~init`/`extends`。
2. **变量声明、形参契约、直接赋值三态与五态解构（2026-11-21 升级）**：
   - 变量声明仅用 `const`（只读）、`let`（可变）、`@tls let`（彻底废除 `var`/`let mut`/`threadlocal var`）；抛弃返回值统一写 `* = expr;`（严禁 `_ = expr;` / `let _ = expr;`，`E0120`）；
   - **形参契约**：只读 `v: T`；安全可变 `v: *T`（编译器自动推导值拷贝或可变借用；**牢记 `*T` 是借用，绝非裸指针**）；所有权转移 `move v: T`；跨线程独占所有权 `own v: T`（自动析构，可作结构成员）；**绝对禁手**：严禁 `*v: T`、`mut v: T`、`let v: T`、`v: move T`（`E0104`/`E0125`）；
   - **直接赋值三态统一**：无修饰默认只读（POD 值拷贝；RAII 资源自动只读借用，**零析构**）；`*x` 安全可变借用（**零析构**）；`move x` / `own x` 显式转移所有权（原变量即刻失效并移交析构；`new`/工厂返回值天然拥有所有权无需 `move`）；
   - **五态对象解构 & 容器/成员分离借用**：
     - 只读 `const { a, b } = obj;`、别名 `const { id, name: userName } = obj;`、剩余打包 `const { id, ...rest } = obj;`；
     - 整体可变借用 `const { a, b } = *obj;`（与任意子成员互斥）；
     - 局部成员分离可变借用 `const { *a: ma, *b: mb, c } = obj;`（指向不同成员合法并行修改，直指原内存）；
     - 整体转移 `const { a } = move obj;` / 单字段转移 `const { move a: ma, b } = obj;`（仅转移出的 RAII 字段接管析构）；
     - 数组解构 `const [head, *, tail, ...rest] = arr;`（忽略元素用 `*`，严禁用 `_`）。
3. **`class` / `struct` / `ref struct`、`init` 构造与 `~init()` / `drop(x)` 唯一析构体系（2026-11-21 重大变更）**：
   - **`struct` vs `class` vs `ref struct`**：
     - `struct`：纯栈分配、无需 `new`、默认 `pub`、无 `extends`；无 `~init()` 为 POD 值拷贝，有 `~init()` 为栈 Move-Only RAII；字面量写 `Point{ x: 1, y: 2 }`（严禁 Zig `.x = 1` / `.{}`）；
     - `class`：堆分配、必须 `new Class(...)`、默认 `priv`、支持 `extends`/`super`/`Box<dyn Trait>` 与私有 `#field`；
     - 普通 `struct`/`class` 无 `<'a>` 生命周期：非 Copy 成员必须持有所有权（`move`/`own`/`Shared<T>`，FFI 可用 `*T!`）；需要暂存栈借用字段必须声明 **`ref struct`**（严格栈绑定，严禁逃逸 `spawn`/`await`/`return`）；
   - **构造与唯一析构（彻底将 `release` 退回普通方法）**：
     - **`init(...)`**：`class` 构造函数（含字段类未手写时自动合成全字段构造器；子类调用 `super.init(...)`）；
     - **`~init() { ... }`**：**`struct` 与 `class` 唯一合法析构函数**（无参数、无返回值、隐式独占可变接收者 `_`），离开作用域自动触发确定性析构，具**双重幂等保护**（调用侧 `__x_moved` + 非 C-ABI 实例 `__cp_dropped`）；
     - **`x.~init()`**：直接调用特殊析构函数（双重幂等保护，多次调用或随后离开作用域均绝不 Double-Free，但不在编译期作废标识符 `x`）；
     - **`drop(x)`**（或 `drop(move x)`）：编译器内置提前释放原语，立即触发一次 `~init()` **并在编译期立即消耗 `x` 所有权**（后续再访问 `x` 直接编译报错防 Use-After-Drop，离开作用域跳过 `defer`）；
     - **`release` 不再是析构函数**：无论无参 `release(*_)` 还是多参 `release(*_, slot)`，`release` 一律为**普通业务方法**，永不注入自动析构、永不截断实参，彻底杜绝命名冲突！
     - **零误伤**：裸指针 `*T!` / `*const T!` / `**T!`、安全借用 `*T` / `&T`、字段读取 `const h = _.handlers;` 永不触发自动析构。
4. **返回类型、纯后缀类型、四层转换法则与字符串**：
   - 有返回值**必须加冒号** `): ReturnType`（`E0116`）；`void` 返回值全场景一律**省略 `: void`**（含 `@c` 声明，`E0122`；`*void` 指针保留）；
   - 纯后缀体系：可选 `T?`（严禁 `?T`，`E0115`）；错误联合 `T!` / `T ! Err`（严禁 `!T`）；三态 `T? ! Err`；废除 `any`（`E0119`），泛型单态化用 `auto`；
   - **四层转换**：无损隐式拓宽；有损/数值强转用 `u32(x)`；同宽/指针强转用 `as`（如 `p as *T!`、`p as usize`，严禁 `"123" as i32`，字符串解析用 `i32.parse(s)?`）；物理位重解释用 `@bitCast`；
   - **字符串与字面量**：`string`（`[]const u8`）、`cstring`（`[:0]const u8`）、`cstr`（`[*:0]const u8`）；模板插值必须反引号 `` `hi ${x}` ``；空切片写 `[]`（严禁 `&.{}`），匿名对象写 `{ a: 1 }`，元组写 `[1, 2]`。
5. **控制流解包、守卫三剑客、循环与运算符**：
   - 废除 Zig 管道捕获 `|capture|`（`E0118`）与 `if (let v = opt)`（`E0126`）；
   - Walrus 解包：只读 `if (v := opt)` / `while (v := it.next())`；可变指针原地修改 `if (*v := opt)` / `while (*v := it.next())`；
   - 循环：只读 `for (v of arr)`、可变借用 `for (*v of arr)`、所有权转移 `for (move v of arr)`（`Loop-Borrow Lock`：遍历受检容器期间严禁增删/重赋值容器）；步进 `while (i < len) : (i += 1)`；带标签循环与跳出 `loop: for (...) { ... break :loop; }`；
   - 错误与空值链：可选链 `?.`、强制解包 `.?`、空值合并 `??` / `??=`、错误传播 `?` / `? default` / `catch e =>`、`defer` / `errdefer`；支持 `?? break :label` / `?? continue :label` 以及环绕/饱和复合赋值（`+%=`, `-%=`, `*%=`, `+|=`, `-|=`, `*|=` , `<<|=`）。
6. **C-FFI、指针安全、`&v` 取址、内置类与并发边界**：
   - C 符号声明用 `@c { fn foo(a: i32): i32; struct Bar {..} }` / `pub @c {..}`（块内严禁重复 `pub`/`extern`）；布局装饰器用 `@c` / `@packed` / `@align(N)`；
   - **裸指针与 `&v` 取址互操作**：裸指针必须带尾置 `!`（`*T!` / `*const T!` / `**T!` / `**const T!` / `*!` / `*!?` / `[*]T!` / `[*:0]T!`，`[*c]T` 仅限 FFI 边界）；**`&v` 回收为纯取址运算符**，支持 `const p: *T! = &v;` / `let p: *const T! = &v;`（亦兼容 `*v!`）；裸指针解引用 `p.*`、切片 `p[0..len]` 与强转必须在 `unsafe { ... }` 内（`E0209`），结构体单级指针字段访问自动解引用写 `p.field`（严禁 `p.*.field`）；
   - 全局 `std.*`、`builtin` 免 `import` 直接用（严禁 `const std = @import("std");` 与为调用 std 写 `zig {}`）；运行时宏用 `@is_nest`（替代 `@is_beam`）；
   - `JSON`（`JSON.stringify`/`JSON.parse<T>`）与 `RegExp`（`/pat/flags` / `new RegExp(...)`）为原生一等公民无需 `import`（严禁 `@json_parse`）；跨 `spawn`/`await` 严禁捕获局部借用，必须 `move`/`own` 或 `Shared<T>`（`E0305`）。

---

## B. Bun 资产迁移实战避坑清单 (Lowering Pitfalls)

1. **范围模式严禁使用三点 `...`，必须严格写为两点 `..`**：
   - 踩坑：在 `switch (r) { 0...0x7F => ... }` 中写 `...` 会导致 `cpc` 词法解析错位，发生 AST 语句静默丢失。
   - 规范：全场景严格写为两点 `0..0x7F`、`0x80..0x7FF`、`0..=10`。
2. **标量错误匹配严禁使用 `switch (e)`**：
   - 踩坑：在 `catch |e|` 后对错误标量使用 `switch (e)` 会触发生成未声明的 `cp_has_ok_val` / `cp_is_null_val` 导致编译失败。
   - 规范：改用语句级 `if (e == error.A) ... else if (e == error.B) ...`，或确定不失败时直接 `catch unreachable`。
3. **实例方法禁止声明为 `static fn`**：
   - 踩坑：可变接收者方法若写为 `static fn foo(*_, ...)`，`static` 会从参数列表剔除接收者，但在体内仍将 `_` lower 为 `self`，导致 `undeclared identifier 'self'`。
   - 规范：实例方法一律写 `pub fn foo(*_, ...)` 或省略只读接收者 `pub fn foo(...)`。
4. **数组字面量类型推导陷阱（`E0207`）**：
   - 踩坑：`[ bytes[0], 0, 0, 0 ]` 中的字面量 `0` 会被默认推导为 `i32`，导致生成类型为 `Tuple([U8, I32, I32, I32])` 报类型不匹配。
   - 规范：在函数或模块顶层声明 `const z: u8 = 0;`，数组写为 `[ bytes[0], z, z, z ]`；或者使用 `@as([4]u8, [ bytes[0], 0, 0, 0 ])`。
5. **空切片与字符串类型别名**：
   - 踩坑：写 `const string = []const u8;` 会误声明局部数组变量遮蔽原生类型 `u8`；写 `return &[_]u8{};` 无法推导数组长度。
   - 规范：类型别名写 `type string = []const u8;`；返回空切片写 `return "";`（自动推导为 `[]const u8`）或 `let empty: [0]T = []; return empty[0..0];`。
6. **CP 内置关键字命名冲突**：
   - 踩坑：`codepoint`、`err`、`in`、`as`、`let`、`type`、`import`、`arena`、`from` 等为 CP 内置关键字。若用作变量名或字段名会导致词法断裂。
   - 规范：错误捕获命名为 `e`；Unicode 码点命名为 `cp`；结构体字段或标识符必须转义为 `@"in"`, `@"arena"`, `@"type"`。
7. **`@root struct` 与辅助结构体组织铁律**：
   - 规范：原 Zig 文件若为根结构体，文件级辅助结构体/枚举必须提升到模块顶层（位于 `@root struct Name { ... }` 外部），`@root struct` 内仅放置根字段与自身方法，确保内部 `_`/`*_` 正常绑定且辅助类型被完整 CP lowering。
8. **结构体嵌套提升与分号结尾**：
   - 规范：`struct`/`enum` 内部嵌套的二级子结构体（`pub const Sub = struct {..};`）会被跳过 CP lowering，必须提升至文件顶层声明；所有 `pub struct Foo { ... };` 声明尾部必须带分号 `;`。
9. **`?? return` 语法非法**：
   - 规范：`??` 是表达式运算符，`return` 是语句。严禁 `val ?? return null;`，必须写为 `if (v := opt) { ... } else return null;` 或 `let v = opt; if (v == null) return null;`。

---

## C. 子智能体 Token 极简流水线与 4 项硬门禁

1. **Step 1 机械预转换**：
   ```bash
   python3 scripts/zig2cp_prep.py <file1.zig> <file2.zig>
   ```
   自动创建 `<stem>_.zig` 原始备份（若已存在严禁覆盖），并生成完成 80% 语法的 `<stem>.cp` 初稿。
2. **Step 2 编译器内置修复**：
   ```bash
   /data/dev/cp/cap/bin/cpc fix <file.cp>
   ```
   自动就地修复 `E0101`..`E0104`, `E0115`..`E0123`, `E0126`（如自动修正 `?T` 为 `T?`，修正 `_ =` 为 `* =` 等）。
3. **Step 3 聚合去重诊断验收（严禁查看原始报错）**：
   ```bash
   python3 scripts/cpc_brief.py -n 8 <file.cp>
   ```
   **严禁**直接运行不带过滤的 `cpc` 打印 40~70KB 原始报错！`cpc_brief.py` 自动合并级联错误并完成 4 项硬门禁检查。
4. **Step 4 精确修补与熔断**：
   - 用 `edit_file` 精准修补。同一类错误在整文件一次性修完再复编，严禁逐行微调试错；
   - 单文件修改步数 $\le 50$ 步，超限判定为 `BLOCKED` 并汇报，不可死磕消耗 Token。

### 单模块交付 4 项硬门禁指标

- ① **原文件备份**：`<stem>_.zig` 必须完好无损；
- ② **编译与 AST 校验**：`cpc --lib <stem>.cp -o <stem>.zig && vendor/zig/zig ast-check <stem>.zig` 0 报错；
- ③ **静态零违例**：0 `zig {}` 逃逸、0 `var`、0 `.@"0"`/`.@"1"`、0 `*anyopaque`/`?*anyopaque`；
- ④ **代码膨胀率**：`<stem>.cp` 总行数 $\le 1.20\times$ `<stem>_.zig` 原始行数。

- **多参 `.exec(...)` 被截断**：`cpc` 会将任意 `Obj.exec(a, b, ...)` 误按 `RegExp.exec(str)` 截断为单参；改用 `const fn_exec = Obj.exec; fn_exec(a, b, ...)`。
- **`struct` 命名空间级静态可变变量**：写 `static let x: T = ...;`（无 `static` 会成为实例字段）。

- **带显式类型标注的 `@intFromFloat`/`@bitCast` 被丢弃**：`const x: T = @intFromFloat(...)` / `@bitCast(...)` 会被 `cpc` 静默丢弃；改写为 `const x = @as(T, @intFromFloat(...))`。

- **类型位置中的匿名结构体参数**：如 `bun.AbsPath({ sep: .auto })` 需先在模块顶层定义为常量别名再引用，否则 codegen 原样输出 `{sep:.auto}` 导致 `ast-check` 报错。

- **多参 `.release(...)`（已修复）**：`release` 现已彻底退回普通方法（不再作为析构函数，也不再按无参 RC 截断），`obj.release(a, b)` 可直接正常调用。
- **顶层可选函数指针类型别名**：拆为 `pub const FnType = *const fn (...) callconv(.c) Ret; pub type T = FnType?;` 避免 `?` 误 lower 为 `try` 或丢失 `callconv(.c)` 返回类型。
- **表达式位裸指针泛参**：如 `bun.LinearFifo(*!?, .Dynamic)` 先声明 `type OpaquePtr = *!?;` 再传入。

- **`cpc fix` `*this` 误修 `**_`（已修复）**：Release `cpc fix` 现已正确将 `*this`/`*self` 修为 `*_`、将 `E0201` 修为 `param: *T`、将 `param: move T` 修为 `move param: T`；批量迁移旧 `.cp` 文件可直接运行 `python3 /data/dev/cp/scripts/migrate_cp_syntax.py --apply --no-release <dir>`。
- **非 Channel 对象的 `.send(...)` 误触发 E0305**：改用 `const send_fn = Type.send; send_fn(obj, ...)`。
- **`switch` 分支含函数调用模式 `Ptr.case(T) =>`**：改用 `if / else if` 链判定。

- **赋值右侧 `_.field = opt ?? default` 误降级**：改用 `if (v := opt) { _.field = v; } else { _.field = default; }`。
- **`switch` 分支右值匿名结构体 `{ field: val }`**：会被误解析为带标签代码块，须显式写 `Type{ field: val }`。

- **`import { A as B }` 成员名误映射**：会误生成 `.B` 而非 `.A`，改用 `import * as M from "..."; const B = M.A;`。
- **字段默认值 `&UpperCase.Const` 误解析为指针类型**：先用小写绑定 `const _v = UpperCase.Const;` 再取 `&_v`。
- **函数体内 `*UpperCaseType` 作为类型实参**：会被误解析为 `(&UpperCaseType)`，先在顶层声明 `type Ptr = *UpperCaseType;`。

- **[2026-11-20 工具链广播 `syntax-tooling-20261120-cap-cpc-fast-modular`]**：`cap` (ReleaseFast) 与 `cpc` (Release) 已支持全工程递归 mtime 感知、`-d/--debug` 原生后端、原子二进制覆写、`-o <dir>` 并发工作区隔离以及 `-m/--modular` (`cpc --emit-dir`) 按文件 FNV-1a 增量发射（0 改动 ~13ms）。
- **`@export(&Obj.UpperField, ...)` 误解析**：即使 `Obj` 为小写，成员首字母大写也会被误解析为 `*const Obj.UpperField`；改用 `@export(&@field(Obj, "UpperField"), ...)`。
- **元组解构 `CpUnpackElemType` 陷阱**：`--lib` 模式下 `const [a, b] = expr;` 若触发 `CpUnpackElemType` 会因 `?Message` (`?anyopaque`) 报 Zig 语义错；改用 `const pair = expr; const a = pair[0]; const b = pair[1];`。
- **二级裸指针必须带尾置 `!` 写 `**T!`（严禁漏 `!` 写 `**T`）**：`*T` 是安全可变借用而非指针，漏写 `!` 的 `**T` 会被当成嵌套可变借用压平为 Zig `*T`；在 CP 中直接写 **`p: **T!`** / **`p: **const T!`** 即可在底层 Zig 中原生输出 `**T` / `**const T`（无需声明 `type TPtr` 绕行）。
- **元组按索引访问/赋值 comptime 失败**：`t[i] = v` / `t[0]` 会被插入 `cp_cast_int(usize, ...)` 导致 comptime 失败；改用 `@field(t, "0")` 或 `@field(t, std.fmt.comptimePrint("{d}", .{ i }))`。
- **局部对象取址 `&local` 防剥离**：实参或字段赋值中的 `&local` 若被剥离 `&`，包裹 `unsafe { &local }` 可稳定保留取址。
- **纯错误集返回值（非错误联合）丢弃**：如 `throwOutOfMemory()` / `throwError()` 返回 `bun.JSError` 时不可用 `catch {}` 或 `* =`，应写 `if (call() == error.JSError) {}`。
- **同名标识符类型串扰**：同函数内同名局部/字段（如 `channel`、第二个 `.err |e|`、`resolver` 捕获遮蔽形参）会让 `cpc` 误推导类型，改名规避。
- **`switch(bool)`**：会被追加 `else => unreachable` 导致 Zig 报错，改 `if/else`。
- **`x = y ?? .z`**（x 为字段）：会被降成 `if (x == null)`，用中间 const。
- **`if (*v := opt) return v`**：会被降成 `v.*`。
- **一元负号作用于 `a.b`**：优先级错成 `(0 - a).b`，先取局部变量再取负。
- **`let x: [N]u8` 中含调用表达式**：会降成 `struct {…}`，ast-check 仍过，需检查生成的 `.zig`；改 `const n = comptime f(); [n]u8`。
- **向 `void` 值参数传 `{}`**：会降成 `.{}`，用 `undefined`。
- **单参方法调用 `x.isLocked(a)`**：会丢实参，改 `const f = T.isLocked; f(x, a)`。
- **`err` 作枚举/联合成员名**：会被改写成 `e`，需写 `@"err"`。
- **自由函数体内出现 `this`**：`cpc` 会给自由函数插入 `self:` 形参，避开 `this` 命名。
- **局部变量不可命名 `impl`**：`impl` 为关键字，会被降成 `@"impl"()`。
- **`if (*v := optField)`**：会降成 `(v.*).*`，改用 `!= null` + `.?`。
- **`T == X || (...)` 布尔表达式**：会误触发错误集合并，改用 `if/else`。
- **无接收者的 hash/eql 上下文方法**：必须在函数体内引用 `_`，`cpc` 才会生成 `self`。
- **`[2026-11-21 语言与编译器重大升级广播 syntax-raii-20261121-v2]`（已上线 Release `cpc`）**：
  1. **唯一双重幂等析构 `~init()`，`release` 彻底退回普通方法**：`init(...)` 为 `class` 构造函数；`~init() { ... }` 为 `struct`/`class` 唯一析构函数；提前手动释放用 `drop(x)`（消耗所有权防误用）或 `x.~init()`（双重幂等防 Double-Free）。**无论是否带参，`release` 均不再作为析构函数、绝不触发自动析构且绝不截断参数**；裸指针 `*T!` 与字段读取 `const h = _.handlers;` 均绝不误析构。
  2. **三态赋值与五态解构**：直接赋值默认只读（RAII 零析构，不再隐式 move），转移必须显式写 `move`；对象解构支持整体可变 `= *_`、局部成员分离可变 `{ *a: ma, *b: mb } = obj`、整体/单字段 `move`；`&v` 回收为纯取址符可直接初始化 `*T!` / `*const T!`。
  3. **语法增强**：饱和/环绕复合赋值（`+|=`, `-|=`, `*|=` , `<<|=`, `+%=`, `-%=`, `*%=`）、`?? break :label` / `?? continue :label` 与带标签 `for` 循环均已修复并发布 Release `cpc`。

已定位 test/regression/issue/28083.test.ts（dgram 全败）的根因：src/runtime/socket/udp_socket.cp:337 的 this_ptr.socket = uws.udp.Socket.create(...)
orelse { ... } 被 cpc 误降级为 if (this_ptr.socket == null) 吞掉了 Socket.create 调用，导致每次绑定都抛出 "Failed to bind socket"。

在编译 udp_socket 修复时暴露出 src/uws_sys/udp.cp 中 PacketBuffer / Loop / Socket 可选指针类型优先级错误（*T? 误生成为 *?*T）；已全部修正为 (*T)? /
*PacketBuffer 并通过 cpc_brief.py 门禁。
