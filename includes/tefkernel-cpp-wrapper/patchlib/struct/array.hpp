/*******************************************************************************
 * File: array.hpp
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

#include <cstdint>

#include "../type.hpp"
#include "tefkernel/patchlib/struct/array.h"

#include <initializer_list>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace TEFKernel::PatchLib::Struct::detail {

    // 读：T 从 void* 槽位读出
    template<typename T>
    T ReadSlot(const void *slot) {
        if constexpr (std::is_same_v<T, Handle>) {
            return Handle(*static_cast<const patch_handle_t *>(slot), true);  // 视图
        } else {
            return *static_cast<const T *>(slot);
        }
    }

    // 写：把 T 写入 void* 槽位
    template<typename T>
    void WriteSlot(void *slot, const T &value) {
        if constexpr (std::is_same_v<T, Handle>) {
            *static_cast<patch_handle_t *>(slot) = value.Raw();
        } else {
            *static_cast<T *>(slot) = value;
        }
    }

}

namespace TEFKernel::PatchLib::Struct {

    template<typename T>
    struct Array : Handle {
        Array() = default;

        Array(const Handle &h) : Handle(h) {}                                 // 视图
        explicit Array(const Handle &h, const bool takeOwnership)
            : Handle(h.Raw(), takeOwnership) {}                               // 显式接管

        explicit Array(const size_t size, Handle type = {}) : Handle() {
            if (type.Raw() == nullptr) {
                type = Type::GetBasicType(GetPatchType());
            }
            _value = patchlib_array_create(size, type.Raw());
            _own   = true;
            if (_value == nullptr) {
                throw std::runtime_error("Failed to create array");
            }
        }

        Array(const T *data, size_t size, Handle type = {}) : Array(size, type) {
            if constexpr (std::is_same_v<T, Handle>) {
                std::vector<patch_handle_t> raw(size, nullptr);
                for (size_t i = 0; i < size; ++i) raw[i] = data[i].Raw();
                if (!patchlib_array_copy_from_c(_value, raw.data(), size)) {
                    throw std::runtime_error("Failed to copy from C array");
                }
            } else {
                if (!patchlib_array_copy_from_c(_value, data, size)) {
                    throw std::runtime_error("Failed to copy from C array");
                }
            }
        }

        explicit Array(const std::vector<T> &vec, Handle type = {})
            : Array(vec.data(), vec.size(), type) {}

        ~Array() = default;

        Array(const Array &other) : Handle(static_cast<const Handle &>(other)) {}

        Array &operator=(const Array &other) {
            if (this != &other) {
                Handle::operator=(static_cast<const Handle &>(other));
            }
            return *this;
        }

        Array(Array &&other) noexcept : Handle(std::move(static_cast<Handle &>(other))) {}

        Array &operator=(Array &&other) noexcept {
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

        // ==================== 元素访问 ====================
        T At(const size_t index) const {
            patch_handle_t raw = nullptr;
            if constexpr (std::is_same_v<T, Handle>) {
                if (!patchlib_array_at(_value, index, &raw)) {
                    throw std::out_of_range("Array index out of bounds");
                }
                return Handle(raw, true);
            } else {
                T value;
                if (!patchlib_array_at(_value, index, &value)) {
                    throw std::out_of_range("Array index out of bounds");
                }
                return value;
            }
        }

        T operator[](const size_t index) const {
           return At(index);
        }

        void Set(const size_t index, const T &value) {
            if constexpr (std::is_same_v<T, Handle>) {
                patch_handle_t raw = value.Raw();
                if (!patchlib_array_set(_value, index, &raw)) {
                    throw std::out_of_range("Array index out of bounds");
                }
            } else {
                if (!patchlib_array_set(_value, index,
                                        const_cast<void *>(static_cast<const void *>(&value)))) {
                    throw std::out_of_range("Array index out of bounds");
                                        }
            }
        }

        void SetUnchecked(const size_t index, const T &value) {
            if constexpr (std::is_same_v<T, Handle>) {
                patch_handle_t raw = value.Raw();
                patchlib_array_set(_value, index, &raw);
            } else {
                patchlib_array_set(_value, index,
                                   const_cast<void *>(static_cast<const void *>(&value)));
            }
        }

        // ==================== 容量 ====================
        [[nodiscard]] size_t Length() const { return patchlib_array_length(_value); }
        [[nodiscard]] bool IsEmpty() const { return patchlib_array_empty(_value); }
        [[nodiscard]] size_t Size() const { return Length(); }

        // ==================== 填充 ====================
        void Fill(const T &value) {
            if constexpr (std::is_same_v<T, Handle>) {
                patch_handle_t raw = value.Raw();
                if (!patchlib_array_fill(_value, &raw)) {
                    throw std::runtime_error("Failed to fill array");
                }
            } else {
                if (!patchlib_array_fill(_value,
                                         const_cast<void *>(static_cast<const void *>(&value)))) {
                    throw std::runtime_error("Failed to fill array");
                                         }
            }
        }

        // ==================== 复制 ====================
        void CopyToC(T *dest, const size_t count) const {
            if constexpr (std::is_same_v<T, Handle>) {
                // dest 是 Handle*，C API 写 patch_handle_t*，不能直接用。
                // 先读到临时 patch_handle_t 数组，再逐个包 Handle 视图。
                std::vector<patch_handle_t> raw(count, nullptr);
                if (!patchlib_array_copy_to_c(raw.data(), _value, count)) {
                    throw std::runtime_error("Failed to copy to C array");
                }
                for (size_t i = 0; i < count; ++i)
                    dest[i] = Handle(raw[i], true);
            } else {
                if (!patchlib_array_copy_to_c(dest, _value, count)) {
                    throw std::runtime_error("Failed to copy to C array");
                }
            }
        }

        void CopyFromC(const T *src, const size_t count) {
            if constexpr (std::is_same_v<T, Handle>) {
                std::vector<patch_handle_t> raw(count, nullptr);
                for (size_t i = 0; i < count; ++i) raw[i] = src[i].Raw();
                if (!patchlib_array_copy_from_c(_value, raw.data(), count)) {
                    throw std::runtime_error("Failed to copy from C array");
                }
            } else {
                if (!patchlib_array_copy_from_c(_value, src, count)) {
                    throw std::runtime_error("Failed to copy from C array");
                }
            }
        }

        void CopyFrom(const Array &src, const size_t count) {
            if (!patchlib_array_copy(_value, src.Raw(), count)) {
                throw std::runtime_error("Failed to copy from array");
            }
        }

        Array Resized(const size_t newSize, const T &defaultValue) const {
            patch_handle_t newHandle;
            if constexpr (std::is_same_v<T, Handle>) {
                patch_handle_t raw = defaultValue.Raw();
                newHandle = patchlib_array_resize(_value, newSize, &raw);
            } else {
                newHandle = patchlib_array_resize(
                    _value, newSize,
                    const_cast<void *>(static_cast<const void *>(&defaultValue)));
            }
            if (newHandle == nullptr) {
                throw std::runtime_error("Failed to resize array");
            }
            return Array(Handle(newHandle, true));
        }

        std::vector<T> ToStdVector() const {
            std::vector<T> result;
            result.resize(Length());
            CopyToC(result.data(), result.size());
            return result;
        }

        // ==================== 迭代器 ====================
        class Iterator {
        public:
            Iterator(Array *array, const size_t index) : Array_(array), Index_(index) {}
            Iterator &operator++() { ++Index_; return *this; }
            Iterator operator++(int) { Iterator t = *this; ++(*this); return t; }
            Iterator &operator--() { --Index_; return *this; }
            Iterator operator--(int) { Iterator t = *this; --(*this); return t; }
            T operator*() const { return (*Array_)[Index_]; }
            bool operator==(const Iterator &o) const { return Array_ == o.Array_ && Index_ == o.Index_; }
            bool operator!=(const Iterator &o) const { return !(*this == o); }
        private:
            Array *Array_;
            size_t Index_;
        };

        class ConstIterator {
        public:
            ConstIterator(const Array *array, const size_t index) : Array_(array), Index_(index) {}
            ConstIterator &operator++() { ++Index_; return *this; }
            ConstIterator operator++(int) { ConstIterator t = *this; ++(*this); return t; }
            ConstIterator &operator--() { --Index_; return *this; }
            ConstIterator operator--(int) { ConstIterator t = *this; --(*this); return t; }
            T operator*() const { return (*Array_)[Index_]; }
            bool operator==(const ConstIterator &o) const { return Array_ == o.Array_ && Index_ == o.Index_; }
            bool operator!=(const ConstIterator &o) const { return !(*this == o); }
        private:
            const Array *Array_;
            size_t Index_;
        };

        Iterator Begin() { return Iterator(this, 0); }
        Iterator End() { return Iterator(this, Length()); }
        ConstIterator Begin() const { return ConstIterator(this, 0); }
        ConstIterator End() const { return ConstIterator(this, Length()); }
        ConstIterator CBegin() const { return ConstIterator(this, 0); }
        ConstIterator CEnd() const { return ConstIterator(this, Length()); }

        // ==================== 运算符 ====================
        explicit operator bool() const { return IsValid(); }

        bool operator==(const Array &other) const {
            if (Length() != other.Length()) return false;
            for (size_t i = 0; i < Length(); ++i) {
                if ((*this)[i] != other[i]) return false;
            }
            return true;
        }

        bool operator!=(const Array &other) const { return !(*this == other); }

        // ==================== 静态工厂 ====================
        static Array CreateEmpty() { return Array(0); }

        static Array FromInitializerList(const std::initializer_list<T> &list) {
            return Array(std::vector<T>(list));
        }

    private:
        static patch_type_t GetPatchType() {
            if constexpr (std::is_same_v<T, void>) return PATCH_VOID;
            else if constexpr (std::is_same_v<T, int8_t>) return PATCH_INT8;
            else if constexpr (std::is_same_v<T, int16_t>) return PATCH_INT16;
            else if constexpr (std::is_same_v<T, int32_t>) return PATCH_INT32;
            else if constexpr (std::is_same_v<T, int64_t>) return PATCH_INT64;
            else if constexpr (std::is_same_v<T, uint8_t>) return PATCH_UINT8;
            else if constexpr (std::is_same_v<T, uint16_t>) return PATCH_UINT16;
            else if constexpr (std::is_same_v<T, uint32_t>) return PATCH_UINT32;
            else if constexpr (std::is_same_v<T, uint64_t>) return PATCH_UINT64;
            else if constexpr (std::is_same_v<T, bool>) return PATCH_BOOL;
            else if constexpr (std::is_same_v<T, float>) return PATCH_FLOAT;
            else if constexpr (std::is_same_v<T, double>) return PATCH_DOUBLE;
            else if constexpr (std::is_same_v<T, char>) return PATCH_CHAR;
            else if constexpr (std::is_same_v<T, void *>) return PATCH_POINTER;
            else return PATCH_OBJECT;
        }
    };

    template<typename T>
    Array<T> MakeArray(size_t size) { return Array<T>(size); }

    template<typename T>
    Array<T> MakeArrayFromVector(const std::vector<T> &vec) { return Array<T>(vec); }

    template<typename T>
    Array<T> MakeArrayFromList(const std::initializer_list<T> &list) {
        return Array<T>::FromInitializerList(list);
    }

} // namespace TEFKernel::PatchLib::Struct