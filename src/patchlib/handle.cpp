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

#include "tefkernel-cpp-wrapper/patchlib/handle.hpp"

using TEFKernel::PatchLib::Handle;

Handle::Handle(const Handle &other) : _value(other._value) {
    if (other._own && other._value != nullptr) {
#if !defined(__ANDROID__)
        _value = patchlib_handle_copy(other._value);
        _own   = true;
#else
        // Android：无 copy，退化为非拥有视图
        _own = false;
#endif
    }
}

Handle &Handle::operator=(const Handle &other) {
    if (this != &other) {
        Reset();
        if (other._own && other._value != nullptr) {
#if !defined(__ANDROID__)
            _value = patchlib_handle_copy(other._value);
            _own   = true;
#else
            _value = other._value;
            _own   = false;
#endif
        } else {
            _value = other._value;
            _own   = false;
        }
    }
    return *this;
}

Handle::Handle(Handle &&other) noexcept
    : _value(other._value), _own(other._own) {
    other._value = nullptr;
    other._own   = false;
}

Handle &Handle::operator=(Handle &&other) noexcept {
    if (this != &other) {
        Reset();
        _value = other._value;
        _own   = other._own;
        other._value = nullptr;
        other._own   = false;
    }
    return *this;
}

Handle::~Handle() {
    if (_own && _value != nullptr) {
#if !defined(__ANDROID__)
        patchlib_free(_value);
#endif
    }
}

bool Handle::IsValid() const { return patchlib_is_valid(_value); }

void Handle::Reset(patch_handle_t newHandle, const bool takeOwnership) {
    if (_own && _value != nullptr) {
#if !defined(__ANDROID__)
        patchlib_free(_value);
#endif
    }
    _value = newHandle;
    _own   = takeOwnership;
}

patch_handle_t Handle::Release() {
    patch_handle_t h = _value;
    _value = nullptr;
    _own   = false;
    return h;
}