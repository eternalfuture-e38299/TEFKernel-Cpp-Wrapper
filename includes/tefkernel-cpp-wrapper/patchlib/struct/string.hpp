/*******************************************************************************
 * File: string.hpp
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

#include "../handle.hpp"
#include "../type.hpp"
#include "tefkernel/patchlib/struct/string.h"

#include <memory>
#include <string>

namespace TEFKernel::PatchLib::Struct {

    /**
     * @brief 字符串包装类
     */
    struct String : Handle {
        String() = default;

        String(const Handle &h) : Handle(h) {}                                // 视图
        explicit String(const Handle &h, const bool takeOwnership)
            : Handle(h.Raw(), takeOwnership) {}                              // 显式接管

        explicit String(const char *str);
        explicit String(const std::string &str) : String(str.c_str()) {}

        ~String() = default;

        String(const String &other) : Handle(static_cast<const Handle &>(other)) {}

        String &operator=(const String &other) {
            if (this != &other) {
                Handle::operator=(static_cast<const Handle &>(other));
            }
            return *this;
        }

        String(String &&other) noexcept : Handle(std::move(static_cast<Handle &>(other))) {}

        String &operator=(String &&other) noexcept {
            if (this != &other) {
                Handle::operator=(std::move(static_cast<Handle &>(other)));
            }
            return *this;
        }

        // 从 C 字符串赋值（创建新句柄）
        String &operator=(const char *str);
        String &operator=(const std::string &str) { return *this = str.c_str(); }

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

        // ==================== 字符串操作 ====================
        [[nodiscard]] std::unique_ptr<char, decltype(&free)> CStr() const;
        [[nodiscard]] std::string ToString() const;
        [[nodiscard]] size_t Length() const;
        [[nodiscard]] bool Empty() const;

        explicit operator bool() const { return IsValid() && !Empty(); }

        // ==================== 比较 ====================
        bool operator==(const String &other) const;
        bool operator==(const char *str) const;
        bool operator==(const std::string &str) const;

        bool operator!=(const String &other) const { return !(*this == other); }
        bool operator!=(const char *str) const { return !(*this == str); }
        bool operator!=(const std::string &str) const { return !(*this == str); }

        bool operator<(const String &other) const;
        bool operator>(const String &other) const;
        bool operator<=(const String &other) const;
        bool operator>=(const String &other) const;

        friend bool operator==(const char *lhs, const String &rhs);
        friend bool operator==(const std::string &lhs, const String &rhs);
        friend bool operator!=(const char *lhs, const String &rhs);
        friend bool operator!=(const std::string &lhs, const String &rhs);
    };

} // namespace TEFKernel::PatchLib::Struct