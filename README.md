# TEFKernel-Cpp-Wrapper

> 对 `tefkernel/patchlib` C API 的现代 C++ 封装库。

---

## 目录

- [1. 简介](#1-简介)
- [2. 快速开始](#2-快速开始)
- [3. 核心概念：Handle 与所有权](#3-核心概念handle-与所有权)
- [4. 类型系统](#4-类型系统)
  - [4.1 Type](#41-type)
  - [4.2 Field](#42-field)
  - [4.3 Method](#43-method)
  - [4.4 Property](#44-property)
  - [4.5 String](#45-string)
- [5. 容器](#5-容器)
  - [5.1 Array](#51-array)
  - [5.2 List](#52-list)
  - [5.3 Dictionary](#53-dictionary)
- [6. Hook 系统](#6-hook-系统)
  - [6.1 HookId](#61-hookid)
  - [6.2 HookArgs / HookResult / SlotProxy](#62-hookargs--hookresult--slotproxy)
  - [6.3 安装 Hook](#63-安装-hook)
  - [6.4 Tag 与跳板机制](#64-tag-与跳板机制)
- [7. 类型转换规则](#7-类型转换规则)
- [8. 最佳实践](#8-最佳实践)
- [9. 已知问题与注意事项](#9-已知问题与注意事项)
- [10. 构建与依赖](#10-构建与依赖)

---

## 1. 简介

TEFKernel-Cpp-Wrapper 是对 `tefkernel/patchlib` 的 C API 的 C++ 封装，提供：

- **RAII 句柄管理**：`Handle` 负责底层资源的生命周期
- **类型安全的反射接口**：`Type`、`Field`、`Method`、`Property`
- **容器封装**：`Array<T>`、`List<T>`、`Dictionary<K,V>`
- **Hook 系统**：类型安全的回调注册，支持前缀/后缀拦截
- **灵活的转换语义**：视图（不拥有）与接管（拥有）两种模式

目标平台：**非 Android**（NDK）与 **Android**（部分 API 有平台差异，见代码中的 `#if __ANDROID__`）。

---

## 2. 快速开始

```cpp
#include "patchlib/type.hpp"
#include "patchlib/method.hpp"
#include "patchlib/struct/array.hpp"

using namespace TEFKernel::PatchLib;

int main() {
    // 1. 定位类型
    Type playerType("Game", "Player");
    if (!playerType) return 1;

    // 2. 读取字段
    Field hp = playerType.GetField("health");
    Handle player = /* 从某处获取实例 */;
    int currentHp = hp[player];
    hp[player] << (currentHp - 10);

    // 3. 调用方法
    Method takeDamage = playerType.GetMethod("TakeDamage", 1);
    int realDamage = takeDamage[player].Invoke<int>(50);

    // 4. 创建容器
    Struct::Array<int> arr(10);
    arr.Set(0, 42);
    int v = arr[0];

    return 0;
}
```

---

## 3. 核心概念：Handle 与所有权

`Handle` 是所有包装类的基类，封装一个 `patch_handle_t` 和一个所有权标志 `_own`。

### 3.1 构造方式

```cpp
Handle()                                  // 空句柄
Handle(patch_handle_t value)              // 隐式，视图（不拥有）
explicit Handle(patch_handle_t value, bool own)  // 显式指定所有权
Handle(const Type &t)                     // 从 Type 构造
Handle(const Handle &other)               // 拷贝（语义取决于实现，见 §9）
Handle(Handle &&other) noexcept           // 移动
```

### 3.2 所有权规则

| `_own` | 含义 | 析构行为 |
|---|---|---|
| `false` | **视图**：只引用，不负责释放 | 无操作 |
| `true` | **拥有**：负责底层资源 | 调用 `patchlib_free` |

**关键原则**：

1. **C API 返回、调用方负责释放的句柄** → 用 `_own = true`
2. **包装类内部返回的视图** → 用 `_own = false`
3. **从视图复制** → 默认仍是视图；想独立拥有用 `Copy()`

### 3.3 Copy：获得拥有所有权的副本

```cpp
Handle Copy() const;                          // 成员：复制自身
static Handle Copy(patch_handle_t raw);       // 静态：复制任意裸句柄
```

`Copy()` 内部调用 `patchlib_handle_copy`，返回**拥有所有权**的新句柄，适合在 hook 回调中持久保存实例。

```cpp
// hook 中保存实例
static std::vector<Handle> visited;
visited.push_back(inst.Copy());   // ✅ 独立拥有
// visited.push_back(inst);       // ❌ 视图，可能悬空
```

### 3.4 基本操作

```cpp
bool IsValid() const;                              // _value != nullptr
void Reset(patch_handle_t h = nullptr, bool own = false);  // 重置
patch_handle_t Release();                          // 放弃所有权并返回裸句柄
patch_handle_t Raw() const;                        // 只读裸句柄（仅内部用）
explicit operator bool() const;                    // if (handle) { ... }
```

---

## 4. 类型系统

### 4.1 Type

`Type` 代表一个反射类型。

```cpp
// 构造
Type();                                            // 空
Type(const Handle &h);                             // 从 Handle（视图）
Type(const std::string &ns, const std::string &name);  // 按名字查
explicit Type(const Handle &h, bool takeOwnership);    // 显式接管

// 元数据
std::string GetName() const;
std::string GetNamespace() const;
std::string GetFullName() const;
Type GetParent() const;
Handle GetMonoType() const;
bool IsValid() const;

// 基本类型
static Type GetBasicType(patch_type_t basicType);

// 实例化
Handle NewInstance() const;
Type MakeGenericType(const Std::Vector<Handle> &typeArgs) const;
Type GetInnerType(const std::string &name) const;

// 反射成员
Field GetField(const std::string &name) const;
Property GetProperty(const std::string &name) const;

// 方法查询（支持重载解析）
Method GetMethod(const std::string &name) const;
Method GetMethod(const std::string &name, int argsCount) const;
Method GetMethod(const std::string &name,
                 const Std::Vector<std::string> &argsNames) const;
Method GetMethod(const std::string &name,
                 const Std::Vector<Type> &argsTypes) const;
Method GetMethod(const std::string &name,
                 const Std::Vector<Type> &argsTypes,
                 const Std::Vector<std::string> &argsNames) const;

// 批量查询
Std::Vector<Type> GetInnerTypes(bool includingParent = false) const;
Std::Vector<Method> GetMethods(bool includingParent = false) const;
Std::Vector<Field> GetFields(bool includingParent = false) const;
Std::Vector<Property> GetProperties(bool includingParent = false) const;
```

**注意**：`Type` 有自定义析构函数（`~Type` 内部调用 `ManualDestroy`），与其他包装类的生命周期语义不同。从 `Handle` 构造 `Type` 时需注意所有权。

**示例**：

```cpp
Type stringType("System", "String");
Type listOfInt = Type("System.Collections.Generic", "List`1")
                     .MakeGenericType({Type::GetBasicType(PATCH_INT32)});

Method m = stringType.GetMethod("Substring", 2);
```

### 4.2 Field

`Field` 封装字段的读写。

```cpp
// 构造
Field(const Handle &h);                            // 视图
explicit Field(const Handle &h, bool takeOwnership);  // 接管

// 元信息
std::string GetName() const;
bool IsStatic() const;
bool IsInstance() const;
bool IsConst() const;
bool IsThreadStatic() const;
size_t GetSize() const;
patch_type_t GetType() const;

// 实例绑定
void SetInstance(const Handle &instance);
const Handle &GetInstance() const;
void ClearInstance();

// 读写（显式）
template<typename T> T GetValue() const;
template<typename T> void SetValue(const T &value) const;
template<typename T> T GetValue(const Handle &instance) const;
template<typename T> void SetValue(const Handle &instance, const T &value);

// 运算符
Field operator[](const Handle &instance) const;     // 临时绑定实例

// 隐式读（T 不可为 bool/Handle/Field）
template<typename T> operator T() const;

// 写入
template<typename T> const Field &operator<<(const T &value) const;

// 比较（非 Handle 标量）
template<typename T> bool operator==(const T &value) const;
template<typename T> bool operator!=(const T &value) const;
```

**示例**：

```cpp
Field hp = playerType.GetField("health");

// 方式 1：绑定实例后操作
Field bound = hp[player];
int v = bound;                       // 隐式读
bound << 100;                        // 写入

// 方式 2：显式
int v2 = hp.GetValue<int>(player);
hp.SetValue(player, 100);

// 方式 3：比较
if (hp[player] == 100) { ... }
```

**限制**：

- 隐式读 `T` 不能是 `bool`、`Handle`、`Field`
- `Handle` 只能用 `GetValue<Handle>()` 读（返回视图）
- 比较运算符仅对非 `Handle` 标量

### 4.3 Method

`Method` 封装方法调用与 Hook。

```cpp
// 构造
Method(const Handle &h);
explicit Method(const Handle &h, bool takeOwnership);

// 元信息
std::string GetName() const;
int GetParamCount() const;
bool IsInstance() const;
bool IsStatic() const;
int GetToken() const;
patch_type_t GetReturnType() const;
std::unique_ptr<MethodSignature> GetSignature() const;

// 泛型
Method MakeGenericInstance(const Std::Vector<Handle> &templateTypes) const;

// 实例绑定
void SetInstance(const Handle &instance);
const Handle &GetInstance() const;
void ClearInstance();

// 调用
template<typename ReturnType, typename... Args>
ReturnType Invoke(Args... args) const;

template<typename ReturnType, typename... Args>
ReturnType InvokeWithInstance(const Handle &instance, Args... args) const;

template<typename... Args>
Handle InvokeConstructor(Args... args) const;    // 构造函数专用

// 运算符
Method operator[](const Handle &instance) const;  // 绑定实例
template<typename ReturnType = void, typename... Args>
ReturnType operator()(Args... args) const;        // 调用

// Hook 安装
template<typename Tag, typename PrefixFn, typename PostfixFn>
HookId InstallPrePostHook(PrefixFn prefix, PostfixFn postfix) const;

template<typename Tag, typename PrefixFn>
HookId InstallPrefixHook(PrefixFn prefix) const;

template<typename Tag, typename PostfixFn>
HookId InstallPostfixHook(PostfixFn postfix) const;

static bool UninstallHook(const HookId &hookId);
```

**示例**：

```cpp
Method m = playerType.GetMethod("TakeDamage", 1);

// 调用方式 1
int dmg = m[player].Invoke<int>(50);

// 调用方式 2
m[player](50);

// 调用方式 3
m.InvokeWithInstance<int>(player, 50);

// 构造函数
Method ctor = playerType.GetMethod(".ctor", 1);
Handle inst = ctor.InvokeConstructor(100);
```

**平台差异**：Android 下 `Invoke` 通过函数指针调用（`GetPointer()`），参数顺序和返回值处理不同，见代码中的 `#if __ANDROID__`。

#### MethodSignature

```cpp
struct MethodSignature : patch_method_signature_t {
    MethodSignature();
    explicit MethodSignature(const patch_method_signature_t &sig);

    patch_handle_t getMethod() const;
    bool isInstance() const;
    patch_type_t getReturnType() const;
    int getToken() const;
    Std::Vector<patch_type_t> GetArgTypes() const;
    Std::Vector<std::string> GetArgNames() const;
    const patch_method_signature_t *GetRaw() const;
};
```

### 4.4 Property

```cpp
Property(const Handle &h);
explicit Property(const Handle &h, bool takeOwnership);

std::string GetName() const;
Method GetGetMethod() const;
Method GetSetMethod() const;

bool IsValid() const;
```

`Property` 本身不直接读写值，需要通过它暴露的 getter/setter `Method`：

```cpp
Property p = type.GetProperty("Length");
int len = p.GetGetMethod()[obj].Invoke<int>();
```

### 4.5 String

```cpp
// 构造
String(const char *str);                          // explicit
String(const std::string &str);                   // explicit
String(const Handle &h);                          // 视图
explicit String(const Handle &h, bool takeOwnership);

// 赋值（从 C 字符串重新创建句柄）
String &operator=(const char *str);
String &operator=(const std::string &str);

// 读
std::string ToString() const;
std::unique_ptr<char, decltype(&free)> CStr() const;
size_t Length() const;
bool Empty() const;
explicit operator bool() const;

// 比较
bool operator==(const String &other) const;
bool operator==(const char *str) const;
bool operator==(const std::string &str) const;
bool operator!=(...) const;
bool operator<(...) const;
bool operator>(...) const;
bool operator<=(...) const;
bool operator>=(...) const;

// 友元比较（左操作数 C 字符串/std::string）
friend bool operator==(const char *lhs, const String &rhs);
friend bool operator==(const std::string &lhs, const String &rhs);
// 同理 !=
```

**注意**：

- `String` **没有** `operator std::string()`，需显式 `ToString()`
- `String s = h;`（`h` 为 `Handle`）会走 `String(const Handle&)`，语义是视图拷贝（详见 §7）

---

## 5. 容器

所有容器都继承 `Handle`，支持视图/接管两种构造方式。

### 5.1 Array

固定长度数组封装。

```cpp
// 构造
Array(size_t size, Handle type = {});                    // 新建
Array(const T *data, size_t size, Handle type = {});     // 从 C 数组
Array(const std::vector<T> &vec, Handle type = {});      // 从 vector

// 元素访问
T At(size_t index) const;                                // 越界抛 std::out_of_range
T operator[](size_t index) const;
void Set(size_t index, const T &value);
void SetUnchecked(size_t index, const T &value);         // 不抛异常

// 容量
size_t Length() const;
size_t Size() const;
bool IsEmpty() const;

// 填充
void Fill(const T &value);

// 复制
void CopyToC(T *dest, size_t count) const;
void CopyFromC(const T *src, size_t count);
void CopyFrom(const Array &src, size_t count);
Array Resized(size_t newSize, const T &defaultValue) const;
std::vector<T> ToStdVector() const;

// 迭代器
Iterator Begin();
Iterator End();
ConstIterator Begin() const;
ConstIterator End() const;
ConstIterator CBegin() const;
ConstIterator CEnd() const;

// 比较
bool operator==(const Array &other) const;
bool operator!=(const Array &other) const;

// 工厂
static Array CreateEmpty();
static Array FromInitializerList(const std::initializer_list<T> &list);

// 自由函数
template<typename T> Array<T> MakeArray(size_t size);
template<typename T> Array<T> MakeArrayFromVector(const std::vector<T> &vec);
template<typename T> Array<T> MakeArrayFromList(const std::initializer_list<T> &list);
```

**示例**：

```cpp
Array<int> arr({1, 2, 3, 4});
arr.Set(0, 42);
int v = arr[0];
for (auto it = arr.Begin(); it != arr.End(); ++it) { ... }

Array<Handle> objs(5, Type("System", "Object"));
Handle h = objs.At(0);
```

**Handle 元素特化**：当 `T == Handle` 时，`At`/`Set`/`Fill`/`CopyToC`/`CopyFromC`/`Resized` 都有专门分支处理裸句柄与 `Handle` 包装的转换。

### 5.2 List

动态数组封装。

```cpp
// 构造
List(size_t capacity, Handle type = {});
List(const std::vector<T> &vec, Handle type = {});
List(const Array<T> &array, Handle type = {});          // 从 Array 拷贝

// 元素操作
void Add(const T &value);
bool Remove(const T &value);
void RemoveAt(size_t index) const;
void Clear() const;

// 容量
size_t Length() const;
size_t Size() const;
bool Empty() const;

// 转换
Array<T> GetArray() const;                              // 视图（拥有所有权 true）
std::vector<T> toStdVector() const;

// 迭代器（与 Array 类似）
Iterator Begin();
Iterator End();
ConstIterator Begin() const;
ConstIterator End() const;

// 比较
bool operator==(const List &other) const;
bool operator!=(const List &other) const;
```

**示例**：

```cpp
List<int> lst({1, 2, 3});
lst.Add(4);
lst.Remove(2);
Array<int> a = lst.GetArray();
```

### 5.3 Dictionary

键值字典封装。

```cpp
// 构造
Dictionary(size_t capacity, Handle keyType = {}, Handle valueType = {});
Dictionary(const std::unordered_map<Key, Value> &map, size_t capacity = 0,
           Handle keyType = {}, Handle valueType = {});

// 元素操作
void Add(const Key &key, const Value &value);
Value Get(const Key &key) const;                        // 不存在抛 std::out_of_range
bool TryGet(const Key &key, Value &outValue) const;
void Set(const Key &key, const Value &value);
bool Remove(const Key &key);
void Clear() const;

// 访问
Value operator[](const Key &key) const;                 // const 版本 = Get

// 非 const 版本（仅非 Handle 值）
template<typename V = Value>
std::enable_if_t<!std::is_same_v<V, Handle>, V &> operator[](const Key &key);

// 容量
size_t Length() const;
size_t size() const;
bool Empty() const;

// 比较
bool operator==(const Dictionary &other) const;
bool operator!=(const Dictionary &other) const;
```

**示例**：

```cpp
Dictionary<std::string, int> d(16);
d.Add("a", 1);
d.Set("a", 2);
int v = d.Get("a");
int out;
if (d.TryGet("b", out)) { ... }
d.Remove("a");
```

**Handle 键值**：当 `Key` 或 `Value` 为 `Handle` 时，`Add`/`Get`/`TryGet`/`Set`/`Remove` 都有特化分支。

---

## 6. Hook 系统

### 6.1 HookId

```cpp
struct HookId {
    HookId();                                    // PATCH_HOOK_INVALID_ID
    explicit HookId(patch_hook_id_t id);

    patch_hook_id_t Get() const;
    bool IsValid() const;
    void Reset();

    bool operator==(const HookId &other) const;
    bool operator!=(const HookId &other) const;
    explicit operator bool() const;
};
```

### 6.2 HookArgs / HookResult / SlotProxy

#### SlotProxy

封装 `void*` 槽位的读写代理。

```cpp
explicit SlotProxy(void *slot);

// 隐式转换
template<typename T> operator T() const;         // Handle 返回视图

// 写入
template<typename T> SlotProxy &operator=(const T &value);
template<typename T> SlotProxy &Set(const T &value);

// 就地运算（Handle 不支持，static_assert）
template<typename T> SlotProxy &operator+=(const T &v);
template<typename T> SlotProxy &operator-=(const T &v);
template<typename T> SlotProxy &operator*=(const T &v);
template<typename T> SlotProxy &operator/=(const T &v);
template<typename T> SlotProxy &operator%=(const T &v);
template<typename T> SlotProxy &operator&=(const T &v);
template<typename T> SlotProxy &operator|=(const T &v);
template<typename T> SlotProxy &operator^=(const T &v);
template<typename T> SlotProxy &operator<<=(const T &v);
template<typename T> SlotProxy &operator>>=(const T &v);
SlotProxy &operator++();
SlotProxy &operator--();
template<typename T> T operator++(int);
template<typename T> T operator--(int);

// 读
template<typename T> T Get() const;

// 所有权
Handle TakeOwnership() const;                    // patchlib_handle_copy
void *Raw() const;
```

#### HookArgs

```cpp
explicit HookArgs(void **args);

void *RawArg(int i) const;
void **RawArgs() const;

template<typename T> T Get(int i) const;
template<typename T> void Set(int i, const T &value) const;
SlotProxy operator[](int i) const;
```

#### HookResult

```cpp
explicit HookResult(void *result);

bool IsValid() const;
void *Raw() const;

template<typename T> T Get() const;
template<typename T> void Set(const T &value) const;

SlotProxy Proxy() const;
template<typename T> operator T() const;
template<typename T> HookResult &operator=(const T &value);
Handle TakeOwnership() const;
```

**示例**：

```cpp
auto prefix = [](Handle inst, HookArgs &args, HookResult result,
                 const patch_method_signature_t *sig) -> bool {
    int dmg = args.Get<int>(0);
    args.Set<int>(0, dmg / 2);
    args[0] += 10;                            // SlotProxy 就地运算
    return true;                              // false 阻断原方法
};

auto postfix = [](Handle inst, HookArgs &args, HookResult result,
                  const patch_method_signature_t *sig) {
    if (result.IsValid()) {
        int r = result.Get<int>();
        result.Set<int>(r + 1);
    }
};
```

### 6.3 安装 Hook

#### 回调签名

两种都支持：

```cpp
// C++ 风格（推荐）
bool prefix(Handle instance, HookArgs &args, HookResult result,
            const patch_method_signature_t *sig);
void postfix(Handle instance, HookArgs &args, HookResult result,
             const patch_method_signature_t *sig);

// C 原生风格
bool prefix(patch_handle_t instance, void **args,
            const patch_method_signature_t *sig, void *result);
void postfix(patch_handle_t instance, void **args,
             void *result, const patch_method_signature_t *sig);
```

`HookCallbacks::InvokePrefix` 先尝试 C++ 签名，失败则退回 C 签名。

#### 安装方式

```cpp
struct MyTag {};

// 前缀 + 后缀
HookId id = method.InstallPrePostHook<MyTag>(prefix, postfix);

// 仅前缀
HookId id = method.InstallPrefixHook<MyTag>(prefix);

// 仅后缀
HookId id = method.InstallPostfixHook<MyTag>(postfix);

// 卸载
Method::UninstallHook(id);
id.Reset();
```

**空回调占位**：`NoPrefix` / `NoPostfix` 可用于只挂单侧：

```cpp
method.InstallPrePostHook<MyTag>(prefix, NoPostfix{});
```

### 6.4 Tag 与跳板机制

`HookCallbacks<Tag, PrefixFn, PostfixFn>` 使用 `Tag` 区分不同的 hook 注册点。

- 每个 `Tag` 对应一个独立的静态跳板（`static s_active`）
- 同一 `Tag` 同时只能有一个活跃的 `HookCallbacks` 实例
- **不同注册点必须使用不同的 `Tag`**

```cpp
namespace {
    struct DamageTag {};      // 匿名命名空间，避免冲突
    struct HealTag {};
}

method1.InstallPrefixHook<DamageTag>(...);
method2.InstallPrefixHook<HealTag>(...);
```

---

## 7. 类型转换规则

### 7.1 继承关系

所有包装类都**公有继承** `Handle`：

```
Handle
├── Type
├── Field
├── Method
├── Property
├── String
├── Array<T>
├── List<T>
└── Dictionary<K,V>
```

### 7.2 转换矩阵

| 转换 | 可行性 | 语义 |
|---|---|---|
| `String → Handle` | ✅ 隐式 | 向上转换（切片） |
| `Handle → String` | ✅ 隐式（`String(const Handle&)`） | 视图，`_own` 保留 |
| `Handle → String` 显式接管 | ✅ `String(h, true)` | 接管 |
| `String → Field` 直接 | ❌ | 无继承关系 |
| `String → Field` 经 Handle | ✅ `Field(static_cast<Handle>(s))` | 语义通常无意义 |
| `String → bool` | ❌（explicit） | `static_cast<bool>(s)` |
| `String → std::string` | ❌ | 用 `ToString()` |
| `String → const char*` | ❌ | 用 `CStr()` |
| `Handle → 标量` | ❌ | 仅 `explicit operator bool` |
| `Field → 标量 T` | ✅（模板 `operator T`） | 隐式读，T ≠ bool/Handle/Field |
| `SlotProxy → Handle` | ✅ | 视图 |
| `SlotProxy → String` | ❌ UB | 先 `Get<Handle>()` 再构造 |
| `HookResult → Handle` | ✅ | 视图 |

### 7.3 `String s = h;` 语义

```cpp
Handle h = /* ... */;
String s = h;        // 调用 String(const Handle&)
// 等价于：
// s._value = h._value;
// s._own   = h._own;
```

- 若 `h._own = false`：`s` 也是视图，安全
- 若 `h._own = true`：`s._own = true`，`h` 和 `s` 都认为拥有 → **可能 double free**（取决于 `Handle` 拷贝构造实现，见 §9）

**推荐**：明确所有权

```cpp
String view(h);              // 视图
String owned = h.Copy();     // 深拷贝（若 Handle::Copy 存在）
```

---

## 8. 最佳实践

### 8.1 回调中持久保存实例

```cpp
static std::vector<Handle> visited;

auto prefix = [](Handle inst, HookArgs &, HookResult,
                 const patch_method_signature_t *) -> bool {
    if (inst.IsValid()) {
        visited.push_back(inst.Copy());   // ✅ 独立拥有
    }
    return true;
};
```

### 8.2 使用唯一 Tag

```cpp
namespace {
    struct TakeDamageTag {};
    struct HealTag {};
}
```

### 8.3 明确所有权

```cpp
// C API 返回裸句柄
patch_handle_t raw = patchlib_string_create("x");

String view(raw);                  // ❌ 泄漏
String owned(raw, true);           // ✅ 接管
```

### 8.4 Android 与非 Android 的差异

关注代码中的 `#if __ANDROID__` / `#if !defined(__ANDROID__)` 分支：

- `Method::Invoke`：Android 走函数指针，非 Android 走 `patchlib_method_invoke_args`
- `Field::GetValue` / `SetValue`：Android 走 `GetPointer` 直接内存访问
- `SlotProxy::TakeOwnership`：Android 返回视图，非 Android 返回 `patchlib_handle_copy`

### 8.5 异常安全

所有失败路径抛标准异常：

- `std::runtime_error`：创建/操作失败
- `std::out_of_range`：索引越界/键不存在

捕获并处理：

```cpp
try {
    int v = arr.At(100);
} catch (const std::out_of_range &e) {
    // ...
}
```

---

## 9. 已知问题与注意事项

### 9.1 Handle 拷贝语义

`Handle(const Handle&)` 的实现决定了 `String s = h;` 等在 `h._own = true` 时是否安全。若实现是浅拷贝（原样复制 `_own`），会导致 double free。

**建议**：检查 `handle.cpp` 的实现，确保拷贝构造做深拷贝（`patchlib_handle_copy`）或引用计数。

### 9.2 Hook 回调对象泄漏

`Method::InstallPrePostHook` 内部 `new HookCallbacks<Tag, ...>(...)`，但目前**没有释放路径**：

- `HookCallbacks` 对象生命周期不受 `HookId` 管理
- `Method::UninstallHook` 不删除 callbacks

**建议**：自行维护 `std::unordered_map<patch_hook_id_t, HookCallbacks*>`，卸载时手动 delete；或改 `HookCallbacks` 用 `std::shared_ptr` 管理。

### 9.3 Tag 单例限制

`HookCallbacks::s_active` 是每个 `Tag` 的静态单例。同一 `Tag` 重复安装会覆盖 `s_active`，导致前一个 hook 失效。

**建议**：每个注册点使用唯一 `Tag`。

### 9.4 `Array<Handle>::At` 所有权

```cpp
T At(size_t index) const {
    if constexpr (std::is_same_v<T, Handle>) {
        // ...
        return Handle(raw, true);   // ⚠️ 注释写"视图"但传 true
    }
}
```

注释与实现不一致：`Handle(raw, true)` 是接管，不是视图。需与 C API 契约核对。

### 9.5 `Dictionary::AsVoidPtr` 非线程安全

```cpp
template<typename T>
static void *AsVoidPtr(const T &value) {
    if constexpr (std::is_same_v<T, Handle>) {
        thread_local patch_handle_t raw;   // ⚠️ thread_local，非重入
        raw = value.Raw();
        return &raw;
    }
    // ...
}
```

非线程安全且不可嵌套调用。当前 `Add`/`Get` 等未直接使用此函数（内联处理），但如后续重构需注意。

### 9.6 `Dictionary::operator[]` 非 const 版本

```cpp
template<typename V = Value>
std::enable_if_t<!std::is_same_v<V, Handle>, V &> operator[](const Key &key) {
    static V temp;   // ⚠️ static，非线程安全，引用下次调用即失效
    // ...
}
```

**建议**：避免使用非 const 版本，改用 `Get`/`Set`/`TryGet`。

### 9.7 `SlotProxy` / `HookResult` 隐式转换仅限标量

```cpp
int i = args[0];             // ✅
Handle h = args[0];          // ✅ 特化
String s = args[0];          // ❌ UB（把槽位当 String 对象解引用）
```

**正确**：

```cpp
Handle raw = args.Get<Handle>(0);
String s(raw);
```

### 9.8 `Type` 的析构语义

`Type` 有自定义 `~Type`（调用 `ManualDestroy`），从 `Handle` 构造时需注意是否会把视图误判为拥有。建议使用 `Type(h.Raw(), false)` 显式指定视图。

### 9.9 回调中写回临时对象

```cpp
args[1].Set<Handle>(String("masked"));   // ⚠️ 局部 String 析构后悬空
```

需确认 C API 在写入时是否 `patchlib_handle_copy`；否则用 `static` 保活或使用持久对象。

---

## 10. 构建与依赖

### 10.1 依赖

- C++17 或更高（使用了 `if constexpr`、`std::is_same_v`、`std::enable_if_t` 等）
- `tefkernel/patchlib` C 库
- 标准库：`<cstdint>`、`<string>`、`<vector>`、`<unordered_map>`、`<memory>`、`<stdexcept>`、`<type_traits>`、`<initializer_list>`

### 10.2 平台

- 非 Android：完整 API
- Android：部分 API 通过 `#if __ANDROID__` 走不同实现

### 10.3 集成

将头文件纳入包含路径：

```
include/
├── handle.hpp
├── type.hpp
├── field.hpp
├── method.hpp
├── property.hpp
├── hook.hpp
├── struct/
│   ├── array.hpp
│   ├── list.hpp
│   ├── dictionary.hpp
│   └── string.hpp
└── tefkernel/patchlib/   # C API 头文件
```

```cpp
#include "patchlib/type.hpp"
#include "patchlib/method.hpp"
```