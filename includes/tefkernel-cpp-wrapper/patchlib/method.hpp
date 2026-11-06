/*******************************************************************************
 * File: method.hpp
 * Project: tefkernel_cpp_wrapper
 * Created: 2026/6/28
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
#include "../tefstd/vector.hpp"
#include "hook.hpp"

#include <memory>
#include <string>
#include <type_traits>

namespace TEFKernel::PatchLib {
    /**
     * @brief 方法签名信息包装类
     *  直接继承 patch_method_signature_t，字段可直接访问：
     *      method / is_instance / return_type / arg_types / arg_names / token
     */
    struct MethodSignature : patch_method_signature_t {
        MethodSignature();

        // 从 C 结构体拷贝构造（浅拷贝）
        explicit MethodSignature(const patch_method_signature_t &sig)
            : patch_method_signature_t(sig) {
        }

        ~MethodSignature();

        MethodSignature(MethodSignature &&other) noexcept;

        MethodSignature &operator=(MethodSignature &&other) noexcept;

        // 拷贝禁用
        MethodSignature(const MethodSignature &) = delete;

        MethodSignature &operator=(const MethodSignature &) = delete;

        // 便捷访问（可选，直接字段也能用）
        [[nodiscard]] patch_handle_t getMethod() const { return method; }
        [[nodiscard]] bool isInstance() const { return is_instance; }
        [[nodiscard]] patch_type_t getReturnType() const { return return_type; }
        [[nodiscard]] int getToken() const { return token; }

        [[nodiscard]] Std::Vector<patch_type_t> GetArgTypes() const;

        [[nodiscard]] Std::Vector<std::string> GetArgNames() const;

        [[nodiscard]] const patch_method_signature_t *GetRaw() const {
            return this;
        }
    };

    struct Method : Handle {
        Handle _instance{};
        bool _is_ctor{};

        Method() = default;

        Method(const Handle &h) : Handle(h) {
        }

        explicit Method(const Handle &h, const bool takeOwnership)
            : Handle(h.Raw(), takeOwnership) {
            _is_ctor = CheckCtor();
        }

        ~Method() = default;

        Method(const Method &other)
            : Handle(static_cast<const Handle &>(other)), _instance(other._instance) {
            _is_ctor = CheckCtor();
        }

        Method &operator=(const Method &other) {
            if (this != &other) {
                Handle::operator=(other);
                _instance = other._instance;
                _is_ctor = other._is_ctor;
            }
            return *this;
        }

        Method(Method &&other) noexcept
            : Handle(std::move(static_cast<Handle &>(other))),
              _instance(std::move(other._instance)),
              _is_ctor(other._is_ctor) {
        }

        Method &operator=(Method &&other) noexcept {
            if (this != &other) {
                Handle::operator=(std::move(static_cast<Handle &>(other)));
                _instance = std::move(other._instance);
                _is_ctor = other._is_ctor;
            }
            return *this;
        }

        [[nodiscard]] bool IsValid() const { return patchlib_is_valid(_value); }

        void reset(const Handle &newHandle = {}, const bool takeOwnership = false) {
            Reset(newHandle.Raw(), takeOwnership);
        }

        Handle Release() {
            Handle h(_value, _own);
            _value = nullptr;
            _own = false;
            return h;
        }

        // ---- 实例绑定 ----
        void SetInstance(const Handle &instance) { _instance = instance; }
        [[nodiscard]] const Handle &GetInstance() const { return _instance; }
        void ClearInstance() { _instance.Reset(); }

        // ---- 基本信息 ----
        [[nodiscard]] std::string GetName() const;

        [[nodiscard]] int GetParamCount() const;

        [[nodiscard]] bool IsInstance() const;

        [[nodiscard]] bool IsStatic() const;

        [[nodiscard]] int GetToken() const;

        [[nodiscard]] patch_type_t GetReturnType() const;

        // ---- 泛型 ----
        [[nodiscard]] Method MakeGenericInstance(const Std::Vector<Handle> &templateTypes) const;

        // ---- 调用 ----
#if __ANDROID__
        [[nodiscard]] void *GetPointer() const;
#endif

        template<typename ReturnType, typename... Args>
        ReturnType Invoke(Args... args) const {
#if !defined(__ANDROID__)
            void *argArray[] = {ToRawArg(args)...};

            if constexpr (std::is_same_v<ReturnType, Handle>) {
                patch_handle_t raw = nullptr;
                patchlib_method_invoke_args(_value, _instance.Raw(), &raw, argArray);
                return Handle(raw, true); // owning
            } else if constexpr (std::is_void_v<ReturnType>) {
                patchlib_method_invoke_args(_value, _instance.Raw(), nullptr, argArray);
                return;
            } else {
                ReturnType result;
                patchlib_method_invoke_args(_value, _instance.Raw(), &result, argArray);
                return result;
            }
#else
            if constexpr (std::is_same_v<ReturnType, Handle>) {
                if (IsInstance()) {
                    auto fn = reinterpret_cast<patch_handle_t (*)(void *, Args..., void *)>(
                        GetPointer());
                    return Handle(fn(_instance.Raw(), args..., _value), true);
                }
                auto fn = reinterpret_cast<patch_handle_t (*)(Args..., void *)>(GetPointer());
                return Handle(fn(args..., _value), true);
            } else if constexpr (std::is_void_v<ReturnType>) {
                if (IsInstance()) {
                    auto fn = reinterpret_cast<void (*)(void *, Args..., void *)>(GetPointer());
                    fn(_instance.Raw(), args..., _value);
                } else {
                    auto fn = reinterpret_cast<void (*)(Args..., void *)>(GetPointer());
                    fn(args..., _value);
                }
                return;
            } else {
                if (IsInstance()) {
                    auto fn = reinterpret_cast<ReturnType (*)(void *, Args..., void *)>(
                        GetPointer());
                    return fn(_instance.Raw(), args..., _value);
                }
                auto fn = reinterpret_cast<ReturnType (*)(Args..., void *)>(GetPointer());
                return fn(args..., _value);
            }
#endif
        }

        template<typename... Args>
        Handle InvokeConstructor(Args... args) const {
            void *argArray[] = {ToRawArg(args)...};
            patch_handle_t instance = nullptr;
            if (patchlib_constructor_invoke(_value, &instance, argArray)) {
                return Handle(instance, true); // owning
            }
            return {};
        }

        template<typename ReturnType, typename... Args>
        ReturnType InvokeWithInstance(const Handle &instance, Args... args) const {
            Method tmp(*this);
            tmp.SetInstance(instance);
            return tmp.Invoke<ReturnType>(args...);
        }

        // ==================== 运算符：method[instance] ====================
        [[nodiscard]] Method operator[](const Handle &instance) const {
            Method tmp(*this);
            tmp.SetInstance(instance);
            return tmp;
        }

        // ==================== 运算符：像函数一样调用 ====================
        template<typename ReturnType = void, typename... Args>
        ReturnType operator()(Args... args) const {
            return Invoke<ReturnType>(args...);
        }

        [[nodiscard]] std::unique_ptr<MethodSignature> GetSignature() const;

        // Method 里：
        template<typename Tag, typename PrefixFn, typename PostfixFn>
        [[nodiscard]] HookId InstallPrePostHook(PrefixFn prefix, PostfixFn postfix) const {
            using CB = HookCallbacks<Tag, PrefixFn, PostfixFn>;
            auto *callbacks = new CB(std::move(prefix), std::move(postfix));
            const patch_hook_id_t id = patchlib_install_prepost_hook(
                _value,
                CB::HasPrefix()   ? &CB::TrampolinePrefix  : nullptr,
                CB::HasPostfix()  ? &CB::TrampolinePostfix : nullptr);
            // callbacks 生命周期：见下文
            return HookId(id);
        }

        // 只挂前缀
        template<typename Tag, typename PrefixFn>
        [[nodiscard]] HookId InstallPrefixHook(PrefixFn prefix) const {
            return InstallPrePostHook<Tag>(std::move(prefix), NoPostfix{});
        }

        // 只挂后缀
        template<typename Tag, typename PostfixFn>
        [[nodiscard]] HookId InstallPostfixHook(PostfixFn postfix) const {
            return InstallPrePostHook<Tag>(NoPrefix{}, std::move(postfix));
        }

        static bool UninstallHook(const HookId &hookId);

        explicit operator bool() const { return IsValid(); }
        bool operator==(const Method &other) const { return _value == other._value; }
        bool operator!=(const Method &other) const { return !(*this == other); }

    private:
        [[nodiscard]] bool CheckCtor() const {
            if (_value == nullptr) return false;
            const char *name = patchlib_method_get_name(_value);
            return name && std::strcmp(name, ".ctor") == 0;
        }

        template<typename T>
        static void *ToRawArg(T &value) {
            if constexpr (std::is_same_v<std::decay_t<T>, Handle>) {
                return value.Raw();
            } else {
                return &value;
            }
        }

        template<typename T>
        static void *ToRawArg(const T &value) {
            if constexpr (std::is_same_v<std::decay_t<T>, Handle>) {
                return value.Raw();
            } else {
                return const_cast<void *>(static_cast<const void *>(&value));
            }
        }
    };
}
