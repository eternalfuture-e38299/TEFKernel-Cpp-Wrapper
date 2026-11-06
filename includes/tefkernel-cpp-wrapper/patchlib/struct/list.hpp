/*******************************************************************************
 * File: list.hpp
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

#include "array.hpp"
#include "tefkernel/patchlib/struct/list.h"

#include <stdexcept>
#include <type_traits>
#include <vector>

namespace TEFKernel::PatchLib::Struct {

    template<typename T>
    struct List : Handle {
        List() = default;

        List(const Handle &h) : Handle(h) {}
        explicit List(const Handle &h, const bool takeOwnership) : Handle(h.Raw(), takeOwnership) {}

        explicit List(const size_t capacity, Handle type = {}) : Handle() {
            if (type.Raw() == nullptr) {
                type = Type::GetBasicType(GetPatchType());
            }
            _value = patchlib_list_create(capacity, type.Raw());
            _own   = true;
            if (_value == nullptr) {
                throw std::runtime_error("Failed to create list");
            }
        }

        explicit List(const std::vector<T> &vec, Handle type = {}) : List(vec.size(), type) {
            for (const auto &elem : vec) add(elem);
        }

        explicit List(const Array<T> &array, Handle type = {}) : List(array.Length(), type) {
            if (!patchlib_list_copy_from(_value, array.Raw())) {
                throw std::runtime_error("Failed to copy from array");
            }
        }

        ~List() = default;

        List(const List &other) : Handle(static_cast<const Handle &>(other)) {}

        List &operator=(const List &other) {
            if (this != &other) {
                Handle::operator=(static_cast<const Handle &>(other));
            }
            return *this;
        }

        List(List &&other) noexcept : Handle(std::move(static_cast<Handle &>(other))) {}

        List &operator=(List &&other) noexcept {
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

        // ==================== 元素操作 ====================
        void Add(const T &value) {
            if constexpr (std::is_same_v<T, Handle>) {
                patch_handle_t raw = value.Raw();
                if (!patchlib_list_add(_value, &raw)) {
                    throw std::runtime_error("Failed to add element to list");
                }
            } else {
                if (!patchlib_list_add(_value,
                                       const_cast<void *>(static_cast<const void *>(&value)))) {
                    throw std::runtime_error("Failed to add element to list");
                                       }
            }
        }

        bool Remove(const T &value) {
            if constexpr (std::is_same_v<T, Handle>) {
                patch_handle_t raw = value.Raw();
                return patchlib_list_remove(_value, &raw);
            } else {
                return patchlib_list_remove(_value,
                                            const_cast<void *>(static_cast<const void *>(&value)));
            }
        }

        void RemoveAt(const size_t index) const {
            if (!patchlib_list_remove_at(_value, index)) {
                throw std::out_of_range("List index out of bounds");
            }
        }

        void Clear() const {
            if (!patchlib_list_clear(_value)) {
                throw std::runtime_error("Failed to clear list");
            }
        }

        // ==================== 容量 ====================
        [[nodiscard]] size_t Length() const { return GetArray().Length(); }
        [[nodiscard]] bool Empty() const { return Length() == 0; }
        [[nodiscard]] size_t Size() const { return Length(); }

        // ==================== 转换 ====================
        Array<T> GetArray() const {
            return Array<T>(Handle(patchlib_list_get_array(_value), true));
        }

        std::vector<T> toStdVector() const { return GetArray().ToStdVector(); }

        // ==================== 迭代器 ====================
        class Iterator {
        public:
            Iterator(List *list, const size_t index) : list_(list), index_(index) {}
            Iterator &operator++() { ++index_; return *this; }
            Iterator operator++(int) { Iterator t = *this; ++(*this); return t; }
            Iterator &operator--() { --index_; return *this; }
            Iterator operator--(int) { Iterator t = *this; --(*this); return t; }
            T operator*() const { return list_->GetArray()[index_]; }
            bool operator==(const Iterator &o) const { return list_ == o.list_ && index_ == o.index_; }
            bool operator!=(const Iterator &o) const { return !(*this == o); }
        private:
            List *list_;
            size_t index_;
        };

        class ConstIterator {
        public:
            ConstIterator(const List *list, const size_t index) : list_(list), index_(index) {}
            ConstIterator &operator++() { ++index_; return *this; }
            ConstIterator operator++(int) { ConstIterator t = *this; ++(*this); return t; }
            ConstIterator &operator--() { --index_; return *this; }
            ConstIterator operator--(int) { ConstIterator t = *this; --(*this); return t; }
            T operator*() const { return list_->GetArray()[index_]; }
            bool operator==(const ConstIterator &o) const { return list_ == o.list_ && index_ == o.index_; }
            bool operator!=(const ConstIterator &o) const { return !(*this == o); }
        private:
            const List *list_;
            size_t index_;
        };

        Iterator Begin() { return Iterator(this, 0); }
        Iterator End() { return Iterator(this, Length()); }
        ConstIterator Begin() const { return ConstIterator(this, 0); }
        ConstIterator End() const { return ConstIterator(this, Length()); }
        ConstIterator CBegin() const { return ConstIterator(this, 0); }
        ConstIterator CEnd() const { return ConstIterator(this, Length()); }

        // ==================== 运算符 ====================
        explicit operator bool() const { return IsValid(); }

        bool operator==(const List &other) const {
            if (Length() != other.Length()) return false;
            Array<T> a1 = GetArray();
            Array<T> a2 = other.GetArray();
            for (size_t i = 0; i < Length(); ++i) {
                if (a1[i] != a2[i]) return false;
            }
            return true;
        }

        bool operator!=(const List &other) const { return !(*this == other); }

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
            else return PATCH_OBJECT;
        }
    };
}