/*******************************************************************************
 * File: property.hpp
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
#include "method.hpp"
#include "tefkernel/patchlib/property.h"

#include <string>

namespace TEFKernel::PatchLib {

    /**
     * @brief 属性包装类
     */
    struct Property : Handle {
        Property() = default;

        Property(const Handle &h) : Handle(h) {}                              // 视图
        explicit Property(const Handle &h, const bool takeOwnership)
            : Handle(h.Raw(), takeOwnership) {}                              // 显式接管

        ~Property() = default;

        Property(const Property &other) : Handle(static_cast<const Handle &>(other)) {}

        Property &operator=(const Property &other) {
            if (this != &other) {
                Handle::operator=(other);
            }
            return *this;
        }

        Property(Property &&other) noexcept : Handle(std::move(static_cast<Handle &>(other))) {}

        Property &operator=(Property &&other) noexcept {
            if (this != &other) {
                Handle::operator=(std::move(static_cast<Handle &>(other)));
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

        // ==================== 方法获取 ====================
        [[nodiscard]] Method GetGetMethod() const;
        [[nodiscard]] Method GetSetMethod() const;

        // ==================== 运算符 ====================
        explicit operator bool() const { return IsValid(); }
        bool operator==(const Property &other) const { return _value == other._value; }
        bool operator!=(const Property &other) const { return !(*this == other); }
    };
}