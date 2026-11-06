/*******************************************************************************
 * File: field.hpp
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
#include "type.hpp"
#include "tefkernel/patchlib/field.h"

#include <string>
#include <type_traits>

namespace TEFKernel::PatchLib {

    /**
     * @brief 字段包装类
     *
     * 读写约定：
     *   写：field << v                    写字段值（v 可为 Handle 或标量）
     *       field[inst] << v              临时绑定实例写入
     *       field.SetValue<T>(v)          显式写
     *   读：T v = field;                  隐式读（T 不可为 bool / Handle / Field）
     *       T v = field[inst];            临时绑定实例隐式读
     *       field.GetValue<T>()           显式读（T 可为 Handle，返回视图）
     *   比较：field == v / field != v      仅对非 Handle 标量
     */
    struct Field : Handle {
        Handle _instance{};

        Field() = default;

        Field(const Handle &h) : Handle(h) {}
        explicit Field(const Handle &h, const bool takeOwnership)
            : Handle(h.Raw(), takeOwnership) {}

        ~Field() = default;

        Field(const Field &other)
            : Handle(static_cast<const Handle &>(other)),
              _instance(other._instance) {}

        Field &operator=(const Field &other) {
            if (this != &other) {
                Handle::operator=(static_cast<const Handle &>(other));
                _instance = other._instance;
            }
            return *this;
        }

        Field(Field &&other) noexcept
            : Handle(std::move(static_cast<Handle &>(other))),
              _instance(std::move(other._instance)) {}

        Field &operator=(Field &&other) noexcept {
            if (this != &other) {
                Handle::operator=(std::move(static_cast<Handle &>(other)));
                _instance = std::move(other._instance);
            }
            return *this;
        }

        [[nodiscard]] bool IsValid() const { return patchlib_is_valid(_value); }

        void Reset(const Handle &newHandle = {}, const bool takeOwnership = false) {
            Handle::Reset(newHandle.Raw(), takeOwnership);
        }

        Handle Release() {
            Handle h(_value, _own);
            _value = nullptr;
            _own   = false;
            return h;
        }

        // ==================== 基本信息 ====================
        [[nodiscard]] std::string GetName() const;
        [[nodiscard]] bool IsStatic() const;
        [[nodiscard]] bool IsInstance() const;
        [[nodiscard]] bool IsConst() const;
        [[nodiscard]] bool IsThreadStatic() const;

        // ==================== 实例绑定 ====================
        void SetInstance(const Handle &instance) { _instance = instance; }
        [[nodiscard]] const Handle &GetInstance() const { return _instance; }
        void ClearInstance() { _instance.Reset(); }

        // ==================== 显式取值 / 赋值 ====================
        template<typename T>
        T GetValue() const {
#if !defined(__ANDROID__)
            if constexpr (std::is_same_v<T, Handle>) {
                patch_handle_t raw = nullptr;
                patchlib_field_get_value(_value, _instance.Raw(), &raw);
                return Handle(raw, false);   // 视图
            } else {
                T value;
                patchlib_field_get_value(_value, _instance.Raw(), &value);
                return value;
            }
#else
            if constexpr (std::is_same_v<T, Handle>) {
                return Handle(
                    *static_cast<patch_handle_t *>(GetPointer(_instance.Raw())),
                    false);
            } else {
                return *static_cast<T *>(GetPointer(_instance.Raw()));
            }
#endif
        }

        template<typename T>
        void SetValue(const T &value) const {
#if !defined(__ANDROID__)
            if constexpr (std::is_same_v<T, Handle>) {
                patch_handle_t raw = value.Raw();
                patchlib_field_set_value(_value, _instance.Raw(), &raw);
            } else {
                patchlib_field_set_value(_value, _instance.Raw(),
                                         const_cast<void *>(static_cast<const void *>(&value)));
            }
#else
            if constexpr (std::is_same_v<T, Handle>) {
                *static_cast<patch_handle_t *>(GetPointer(_instance.Raw())) = value.Raw();
            } else {
                *static_cast<T *>(GetPointer(_instance.Raw())) = value;
            }
#endif
        }

        template<typename T>
        T GetValue(const Handle &instance) const {
            Field tmp(*this);
            tmp.SetInstance(instance);
            return tmp.GetValue<T>();
        }

        template<typename T>
        void SetValue(const Handle &instance, const T &value) {
            SetInstance(instance);
            SetValue<T>(value);
        }

        // ==================== 运算符：field[instance] ====================
        [[nodiscard]] Field operator[](const Handle &instance) const {
            Field tmp(*this);
            tmp.SetInstance(instance);
            return tmp;
        }

        // ==================== 运算符：隐式取值（T v = field;）====================
        template<typename T,
                 typename = std::enable_if_t<
                     !std::is_same_v<T, bool> &&
                     !std::is_same_v<T, Handle> &&
                     !std::is_same_v<T, Field>>>
        operator T() const { return GetValue<T>(); }

        // ==================== 运算符：写入（<<）====================
        template<typename T>
        std::enable_if_t<!std::is_same_v<T, Field>, const Field &>
        operator<<(const T &value) const {
            SetValue<T>(value);
            return *this;
        }

        // ==================== 运算符：比较 ====================
        template<typename T>
        std::enable_if_t<
            !std::is_same_v<T, Handle> && !std::is_same_v<T, Field>,
            bool
        >
        operator==(const T &value) const { return GetValue<T>() == value; }

        template<typename T>
        std::enable_if_t<
            !std::is_same_v<T, Handle> && !std::is_same_v<T, Field>,
            bool
        >
        operator!=(const T &value) const { return !(*this == value); }

        // ==================== 类型信息 ====================
        [[nodiscard]] size_t GetSize() const;
        [[nodiscard]] patch_type_t GetType() const;

#if __ANDROID__
        [[nodiscard]] void *GetPointer(void *instance) const;
#endif

        // ==================== 与其他 Field 比较 / 布尔 ====================
        explicit operator bool() const { return IsValid(); }
        bool operator==(const Field &other) const { return _value == other._value; }
        bool operator!=(const Field &other) const { return !(*this == other); }
    };
}