/*******************************************************************************
 * File: handle
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

#include "tefkernel/patchlib/type.h"

namespace TEFKernel::PatchLib {
    struct Type;

    struct Handle {
        patch_handle_t _value{};
        bool _own{};

        Handle() = default;

        // patch_handle_t -> Handle（隐式，默认不拥有）
        Handle(patch_handle_t value) : _value(value) {}

        // 显式指定所有权
        explicit Handle(patch_handle_t value, const bool own) : _value(value), _own(own) {}

        // Type -> Handle
        Handle(const Type &t);

        Handle(const Handle &other);
        Handle &operator=(const Handle &other);
        Handle(Handle &&other) noexcept;
        Handle &operator=(Handle &&other) noexcept;

        ~Handle();

        [[nodiscard]] bool IsValid() const;
        void Reset(patch_handle_t newHandle = nullptr, bool takeOwnership = false);
        patch_handle_t Release();

        // 仅供与 C API 交互的内部代码使用
        [[nodiscard]] patch_handle_t Raw() const { return _value; }

        /**
  * @brief 复制当前句柄，返回**拥有所有权**的新 Handle
  *
  * 内部调用 patchlib_handle_copy，新句柄独立于原句柄的生命周期，
  * 适合在 hook 等临时对象场景下保存，避免原句柄被卸载/修改。
  *
  * @return 拥有所有权的新 Handle；若复制失败返回空 Handle（_own=false）
  * @note 新句柄析构时会自动 patchlib_free
  */
        [[nodiscard]] Handle Copy() const {
            patch_handle_t raw = patchlib_handle_copy(_value);
            return Handle(raw, raw != nullptr);
        }

        /**
         * @brief 静态版本：复制任意裸句柄
         */
        [[nodiscard]] static Handle Copy(patch_handle_t raw) {
            patch_handle_t copied = patchlib_handle_copy(raw);
            return Handle(copied, copied != nullptr);
        }

        // 比较
        bool operator==(const Handle &other) const { return _value == other._value; }
        bool operator!=(const Handle &other) const { return !(*this == other); }
        explicit operator bool() const { return IsValid(); }
    };
}