/*******************************************************************************
 * File: hook
 * Project: tefkernel_cpp_wrapper
 * Created: 2026/11/6
 * Author: eternalfuture-e38299
 * Github: https://github.com/eternalfuture-e38299
 *
 * MIT License
 *
 * Copyright (c) 2026 eternalfuture-e38299
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *******************************************************************************/

#pragma once

#include "handle.hpp"
#include "tefkernel/patchlib/method.h"

#include <type_traits>
#include <utility>

namespace TEFKernel::PatchLib {
    struct HookId {
        HookId() : id_(PATCH_HOOK_INVALID_ID) {
        }

        explicit HookId(const patch_hook_id_t id) : id_(id) {
        }

        [[nodiscard]] patch_hook_id_t Get() const { return id_; }
        [[nodiscard]] bool IsValid() const { return id_ != PATCH_HOOK_INVALID_ID; }
        void Reset() { id_ = PATCH_HOOK_INVALID_ID; }

        bool operator==(const HookId &other) const { return id_ == other.id_; }
        bool operator!=(const HookId &other) const { return id_ != other.id_; }
        explicit operator bool() const { return IsValid(); }

    private:
        patch_hook_id_t id_;
    };

    // ==================== SlotProxy ====================
    class SlotProxy {
    public:
        explicit SlotProxy(void *slot) noexcept : _slot(slot) {}

        template<typename T>
        constexpr operator T() const {
            if constexpr (std::is_same_v<T, Handle>) {
                return Handle(*static_cast<patch_handle_t *>(_slot), false);
            } else {
                return *static_cast<T *>(_slot);
            }
        }

        template<typename T>
        constexpr SlotProxy &operator=(const T &value) {
            if constexpr (std::is_same_v<T, Handle>) {
                *static_cast<patch_handle_t *>(_slot) = value.Raw();
            } else {
                *static_cast<T *>(_slot) = value;
            }
            return *this;
        }

        template<typename T> constexpr SlotProxy &operator+=(const T &v) { return apply<T>([&](T &x) { x += v; }); }
        template<typename T> constexpr SlotProxy &operator-=(const T &v) { return apply<T>([&](T &x) { x -= v; }); }
        template<typename T> constexpr SlotProxy &operator*=(const T &v) { return apply<T>([&](T &x) { x *= v; }); }
        template<typename T> constexpr SlotProxy &operator/=(const T &v) { return apply<T>([&](T &x) { x /= v; }); }
        template<typename T> constexpr SlotProxy &operator%=(const T &v) { return apply<T>([&](T &x) { x %= v; }); }
        template<typename T> constexpr SlotProxy &operator&=(const T &v) { return apply<T>([&](T &x) { x &= v; }); }
        template<typename T> constexpr SlotProxy &operator|=(const T &v) { return apply<T>([&](T &x) { x |= v; }); }
        template<typename T> constexpr SlotProxy &operator^=(const T &v) { return apply<T>([&](T &x) { x ^= v; }); }
        template<typename T> constexpr SlotProxy &operator<<=(const T &v) { return apply<T>([&](T &x) { x <<= v; }); }
        template<typename T> constexpr SlotProxy &operator>>=(const T &v) { return apply<T>([&](T &x) { x >>= v; }); }

        constexpr SlotProxy &operator++() { return apply<int>([](int &x) { ++x; }); }
        constexpr SlotProxy &operator--() { return apply<int>([](int &x) { --x; }); }

        template<typename T> T operator++(int) const { T old = Get<T>(); apply<T>([](T &x) { ++x; }); return old; }
        template<typename T> T operator--(int) const { T old = Get<T>(); apply<T>([](T &x) { --x; }); return old; }

        template<typename T> T Get() const {
            if constexpr (std::is_same_v<T, Handle>) {
                return Handle(*static_cast<patch_handle_t *>(_slot), false);
            } else {
                return *static_cast<T *>(_slot);
            }
        }

        template<typename T> constexpr SlotProxy &Set(const T &value) {
            if constexpr (std::is_same_v<T, Handle>) {
                *static_cast<patch_handle_t *>(_slot) = value.Raw();
            } else {
                *static_cast<T *>(_slot) = value;
            }
            return *this;
        }

        [[nodiscard]] Handle TakeOwnership() const {
            return Handle::Copy(*static_cast<patch_handle_t *>(_slot));
        }

        [[nodiscard]] void *Raw() const { return _slot; }

    private:
        template<typename T, typename F>
        constexpr SlotProxy &apply(F &&f) {
            static_assert(!std::is_same_v<T, Handle>,
                          "Handle does not support arithmetic/bitwise operators");
            T &x = *static_cast<T *>(_slot);
            f(x);
            return *this;
        }

        void *_slot;
    };

    // ==================== HookArgs / HookResult ====================
    class HookArgs {
    public:
        explicit HookArgs(void **args) noexcept : _args(args) {}
        [[nodiscard]] constexpr void *RawArg(const int i) const { return _args[i]; }
        [[nodiscard]] constexpr void **RawArgs() const { return _args; }

        template<typename T> constexpr T Get(const int i) const { return SlotProxy(_args[i]).Get<T>(); }
        template<typename T> constexpr void Set(const int i, const T &value) const { SlotProxy(_args[i]).Set<T>(value); }
        [[nodiscard]] SlotProxy operator[](const int i) const { return SlotProxy(_args[i]); }

    private:
        void **_args;
    };

    class HookResult {
    public:
        explicit HookResult(void *result) noexcept : _result(result) {}
        [[nodiscard]] bool IsValid() const { return _result != nullptr; }
        [[nodiscard]] void *Raw() const { return _result; }

        template<typename T> constexpr T Get() const {
            if (_result == nullptr) return T{};
            return SlotProxy(_result).Get<T>();
        }
        template<typename T> constexpr void Set(const T &value) const {
            if (_result != nullptr) SlotProxy(_result).Set<T>(value);
        }

        [[nodiscard]] SlotProxy Proxy() const { return SlotProxy(_result); }
        template<typename T> constexpr operator T() const { return Get<T>(); }
        template<typename T> constexpr HookResult &operator=(const T &value) { Set<T>(value); return *this; }
        [[nodiscard]] Handle TakeOwnership() const { return SlotProxy(_result).TakeOwnership(); }

    private:
        void *_result;
    };

    // ==================== 空回调占位 ====================
    struct NoPrefix {
        bool operator()(const Handle&, HookArgs &, HookResult, const patch_method_signature_t *) const {
            return true;
        }
        static constexpr bool kIsEmpty = true;
    };

    struct NoPostfix {
        void operator()(const Handle&, HookArgs &, HookResult, const patch_method_signature_t *) const {}
        static constexpr bool kIsEmpty = true;
    };

    // ==================== HookSlot：唯一 tag ====================
    template<int N>
    struct HookSlot {};

    // ==================== HookCallbacks ====================
    /**
     * @brief Hook 回调包装（模板跳板）。
     *
     * Tag 保证每个 hook 注册点有独立跳板（独立 s_active），
     * 因此同一 (PrefixFn, PostfixFn) 类型组合可以有多个活跃 hook，
     * 只要 Tag 不同。
     *
     * PrefixFn / PostfixFn 任意一个可以是 NoPrefix / NoPostfix（空回调）。
     */
    template<typename Tag, typename PrefixFn, typename PostfixFn>
    class HookCallbacks {
    public:
        HookCallbacks(PrefixFn prefix, PostfixFn postfix)
            : _prefix(std::move(prefix)), _postfix(std::move(postfix)) {
            s_active = this;
        }

        ~HookCallbacks() {
            if (s_active == this) s_active = nullptr;
        }

        HookCallbacks(const HookCallbacks &) = delete;
        HookCallbacks &operator=(const HookCallbacks &) = delete;

        // ---- 跳板：匹配 prefix_callback_t ----
        static bool TrampolinePrefix(patch_handle_t instance, void **args,
                                     const patch_method_signature_t *sig,
                                     void *result) {
            if constexpr (std::is_same_v<PrefixFn, NoPrefix>) {
                return true;
            } else {
                if (s_active == nullptr) return true;
                HookArgs ctx(args);
                const HookResult res(result);
                return InvokePrefix(s_active->_prefix, Handle(instance, false), ctx, res, sig);
            }
        }

        // ---- 跳板：匹配 postfix_callback_t ----
        static void TrampolinePostfix(patch_handle_t instance, void **args,
                                      void *result,
                                      const patch_method_signature_t *sig) {
            if constexpr (!std::is_same_v<PostfixFn, NoPostfix>) {
                if (s_active == nullptr) return;
                HookArgs ctx(args);
                HookResult res(result);
                s_active->_postfix(Handle(instance, false), ctx, res, sig);
            }
        }

        // ---- 是否注册某跳板 ----
        static constexpr bool HasPrefix() { return !std::is_same_v<PrefixFn, NoPrefix>; }
        static constexpr bool HasPostfix() { return !std::is_same_v<PostfixFn, NoPostfix>; }

    private:
        static bool InvokePrefix(PrefixFn &fn, Handle inst, HookArgs &args,
                                 HookResult result, const patch_method_signature_t *sig) {
            if constexpr (std::is_invocable_v<PrefixFn &, Handle, HookArgs &,
                                              HookResult, const patch_method_signature_t *>) {
                return fn(std::move(inst), args, result, sig);
            } else {
                return fn(inst.Raw(), args.RawArgs(), sig, result.Raw());
            }
        }

        PrefixFn _prefix;
        PostfixFn _postfix;

        static inline HookCallbacks *s_active = nullptr;
    };

    // 便捷构造（自动推导 Tag 需要显式给，所以这里不带 Tag 版本的工厂不给）
    template<typename Tag, typename PrefixFn, typename PostfixFn>
    [[nodiscard]] auto MakeHookCallbacks(PrefixFn prefix, PostfixFn postfix) {
        return new HookCallbacks<Tag, PrefixFn, PostfixFn>(
            std::move(prefix), std::move(postfix));
    }
}