/*******************************************************************************
 * File: string
 * Project: tefkernel_cpp_wrapper
 * Created: 2026/10/29
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

#include "tefkernel-cpp-wrapper/patchlib/struct/string.hpp"

#include <cstdlib>
#include <stdexcept>

using TEFKernel::PatchLib::Handle;
using TEFKernel::PatchLib::Struct::String;

// ==================== 构造 / 赋值 ====================
String::String(const char *str) : Handle() {
    _value = patchlib_string_create(str);
    _own   = true;
    if (_value == nullptr) {
        throw std::runtime_error("Failed to create string");
    }
}

String &String::operator=(const char *str) {
    Reset();
    _value = patchlib_string_create(str);
    _own   = true;
    if (_value == nullptr) {
        throw std::runtime_error("Failed to create string");
    }
    return *this;
}

// ==================== 字符串操作 ====================
std::unique_ptr<char, decltype(&free)> String::CStr() const {
    char *str = patchlib_string_cstr(_value);
    return {str, free};
}

std::string String::ToString() const {
    const auto cstrPtr = CStr();
    return cstrPtr ? std::string(cstrPtr.get()) : std::string();
}

size_t String::Length() const {
    return patchlib_string_length(_value);
}

bool String::Empty() const {
    return patchlib_string_empty(_value);
}

// ==================== 比较 ====================
bool String::operator==(const String &other) const {
    return ToString() == other.ToString();
}

bool String::operator==(const char *str) const {
    return ToString() == str;
}

bool String::operator==(const std::string &str) const {
    return ToString() == str;
}

bool String::operator<(const String &other) const {
    return ToString() < other.ToString();
}

bool String::operator>(const String &other) const {
    return ToString() > other.ToString();
}

bool String::operator<=(const String &other) const {
    return ToString() <= other.ToString();
}

bool String::operator>=(const String &other) const {
    return ToString() >= other.ToString();
}

// ==================== 友元 ====================
bool TEFKernel::PatchLib::Struct::operator==(const char *lhs, const String &rhs) {
    return rhs == lhs;
}

bool TEFKernel::PatchLib::Struct::operator==(const std::string &lhs, const String &rhs) {
    return rhs == lhs;
}

bool TEFKernel::PatchLib::Struct::operator!=(const char *lhs, const String &rhs) {
    return rhs != lhs;
}

bool TEFKernel::PatchLib::Struct::operator!=(const std::string &lhs, const String &rhs) {
    return rhs != lhs;
}