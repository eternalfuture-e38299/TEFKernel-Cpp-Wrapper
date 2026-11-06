/*******************************************************************************
 * File: dictionary.hpp
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
#include "tefkernel/patchlib/struct/dictionary.h"

#include <stdexcept>
#include <type_traits>
#include <unordered_map>

namespace TEFKernel::PatchLib::Struct {

    template<typename Key, typename Value>
    struct Dictionary : Handle {
        Dictionary() = default;

        Dictionary(const Handle &h) : Handle(h) {}
        explicit Dictionary(const Handle &h, const bool takeOwnership)
            : Handle(h.Raw(), takeOwnership) {}

        explicit Dictionary(const size_t capacity, Handle keyType = {}, Handle valueType = {})
            : Handle() {
            if (keyType.Raw() == nullptr) keyType = Type::GetBasicType(GetKeyPatchType());
            if (valueType.Raw() == nullptr) valueType = Type::GetBasicType(GetValuePatchType());

            _value = patchlib_dictionary_create(keyType.Raw(), valueType.Raw(), capacity);
            _own   = true;
            if (_value == nullptr) {
                throw std::runtime_error("Failed to create dictionary");
            }
        }

        explicit Dictionary(const std::unordered_map<Key, Value> &map, size_t capacity = 0,
                            Handle keyType = {}, Handle valueType = {})
            : Dictionary(capacity > 0 ? capacity : map.size(), keyType, valueType) {
            for (const auto &pair : map) add(pair.first, pair.second);
        }

        ~Dictionary() = default;

        Dictionary(const Dictionary &other) : Handle(static_cast<const Handle &>(other)) {}

        Dictionary &operator=(const Dictionary &other) {
            if (this != &other) {
                Handle::operator=(static_cast<const Handle &>(other));
            }
            return *this;
        }

        Dictionary(Dictionary &&other) noexcept : Handle(std::move(static_cast<Handle &>(other))) {}

        Dictionary &operator=(Dictionary &&other) noexcept {
            if (this != &other) {
                Handle::operator=(std::move(static_cast<Handle &>(other)));
            }
            return *this;
        }

        [[nodiscard]] bool IsValid() const { return patchlib_is_valid(_value); }

        void Reset(const Handle &newHandle = {}, const bool takeOwnership = false) {
            Handle::Reset(newHandle.Raw(), takeOwnership);
        }

        Handle release() {
            Handle h(_value, _own);
            _value = nullptr;
            _own   = false;
            return h;
        }

        // ==================== 元素操作 ====================
        void Add(const Key &key, const Value &value) {
            if constexpr (std::is_same_v<Key, Handle> || std::is_same_v<Value, Handle>) {
                patch_handle_t kRaw, vRaw;
                const void *kPtr = &key;
                const void *vPtr = &value;
                if constexpr (std::is_same_v<Key, Handle>) { kRaw = key.Raw(); kPtr = &kRaw; }
                if constexpr (std::is_same_v<Value, Handle>) { vRaw = value.Raw(); vPtr = &vRaw; }
                if (!patchlib_dictionary_add(_value,
                                             const_cast<void *>(kPtr),
                                             const_cast<void *>(vPtr))) {
                    throw std::runtime_error("Failed to add key-value pair to dictionary");
                                             }
            } else {
                if (!patchlib_dictionary_add(_value,
                                             const_cast<void *>(static_cast<const void *>(&key)),
                                             const_cast<void *>(static_cast<const void *>(&value)))) {
                    throw std::runtime_error("Failed to add key-value pair to dictionary");
                                             }
            }
        }

        Value Get(const Key &key) const {
            if constexpr (std::is_same_v<Key, Handle> || std::is_same_v<Value, Handle>) {
                patch_handle_t kRaw;
                const void *kPtr = &key;
                if constexpr (std::is_same_v<Key, Handle>) { kRaw = key.Raw(); kPtr = &kRaw; }

                if constexpr (std::is_same_v<Value, Handle>) {
                    patch_handle_t vRaw = nullptr;
                    if (!patchlib_dictionary_get_value(_value, const_cast<void *>(kPtr), &vRaw)) {
                        throw std::out_of_range("Key not found in dictionary");
                    }
                    return Handle(vRaw, true);
                } else {
                    Value value;
                    if (!patchlib_dictionary_get_value(_value, const_cast<void *>(kPtr), &value)) {
                        throw std::out_of_range("Key not found in dictionary");
                    }
                    return value;
                }
            } else {
                Value value;
                if (!patchlib_dictionary_get_value(_value,
                                                   const_cast<void *>(static_cast<const void *>(&key)),
                                                   &value)) {
                    throw std::out_of_range("Key not found in dictionary");
                                                   }
                return value;
            }
        }

        bool TryGet(const Key &key, Value &outValue) const {
            if constexpr (std::is_same_v<Key, Handle> || std::is_same_v<Value, Handle>) {
                patch_handle_t kRaw;
                const void *kPtr = &key;
                if constexpr (std::is_same_v<Key, Handle>) { kRaw = key.Raw(); kPtr = &kRaw; }

                if constexpr (std::is_same_v<Value, Handle>) {
                    patch_handle_t vRaw = nullptr;
                    if (!patchlib_dictionary_get_value(_value, const_cast<void *>(kPtr), &vRaw)) {
                        return false;
                    }
                    outValue = Handle(vRaw, true);
                    return true;
                } else {
                    return patchlib_dictionary_get_value(_value, const_cast<void *>(kPtr), &outValue);
                }
            } else {
                return patchlib_dictionary_get_value(_value,
                                                     const_cast<void *>(static_cast<const void *>(&key)),
                                                     &outValue);
            }
        }

        void Set(const Key &key, const Value &value) {
            patch_handle_t kRaw;
            patch_handle_t vRaw;
            const void *kPtr = &key;
            const void *vPtr = &value;

            if constexpr (std::is_same_v<Key, Handle>) {
                kRaw = key.Raw();
                kPtr = &kRaw;
            }
            if constexpr (std::is_same_v<Value, Handle>) {
                vRaw = value.Raw();
                vPtr = &vRaw;
            }

            if (!patchlib_dictionary_set_value(_value,
                                               const_cast<void *>(kPtr),
                                               const_cast<void *>(vPtr))) {
                throw std::runtime_error("Failed to set value in dictionary");
                                               }
        }

        bool Remove(const Key &key) {
            patch_handle_t kRaw;
            const void *kPtr = &key;

            if constexpr (std::is_same_v<Key, Handle>) {
                kRaw = key.Raw();
                kPtr = &kRaw;
            }

            return patchlib_dictionary_remove(_value, const_cast<void *>(kPtr));
        }

        void Clear() const {
            if (!patchlib_dictionary_clear(_value)) {
                throw std::runtime_error("Failed to clear dictionary");
            }
        }

        Value operator[](const Key &key) const { return Get(key); }

        // 非 const 版本对 Handle 不提供引用
        template<typename V = Value>
        std::enable_if_t<!std::is_same_v<V, Handle>, V &> operator[](const Key &key) {
            static V temp;
            try { temp = Get(key); }
            catch (const std::out_of_range &) {
                V defaultVal{};
                Add(key, defaultVal);
                temp = defaultVal;
            }
            return temp;
        }

        // ==================== 容量 ====================
        [[nodiscard]] size_t Length() const { return patchlib_dictionary_length(_value); }
        [[nodiscard]] bool Empty() const { return Length() == 0; }
        [[nodiscard]] size_t size() const { return Length(); }

        // ==================== 运算符 ====================
        explicit operator bool() const { return IsValid(); }

        bool operator==(const Dictionary &other) const {
            if (Length() != other.Length()) return false;
            return _value == other._value;
        }

        bool operator!=(const Dictionary &other) const { return !(*this == other); }

    private:
        // 把 Key / Value 转成 void* 供 C API 读
        template<typename T>
        static void *AsVoidPtr(const T &value) {
            if constexpr (std::is_same_v<T, Handle>) {
                // 必须把裸指针放到一个变量里，不能取临时地址
                // 这里用 thread_local 静态存储避免悬空
                thread_local patch_handle_t raw;
                raw = value.Raw();
                return &raw;
            } else {
                return const_cast<void *>(static_cast<const void *>(&value));
            }
        }

        static patch_type_t GetKeyPatchType() {
            if constexpr (std::is_same_v<Key, void>) return PATCH_VOID;
            else if constexpr (std::is_same_v<Key, int8_t>) return PATCH_INT8;
            else if constexpr (std::is_same_v<Key, int16_t>) return PATCH_INT16;
            else if constexpr (std::is_same_v<Key, int32_t>) return PATCH_INT32;
            else if constexpr (std::is_same_v<Key, int64_t>) return PATCH_INT64;
            else if constexpr (std::is_same_v<Key, uint8_t>) return PATCH_UINT8;
            else if constexpr (std::is_same_v<Key, uint16_t>) return PATCH_UINT16;
            else if constexpr (std::is_same_v<Key, uint32_t>) return PATCH_UINT32;
            else if constexpr (std::is_same_v<Key, uint64_t>) return PATCH_UINT64;
            else if constexpr (std::is_same_v<Key, bool>) return PATCH_BOOL;
            else if constexpr (std::is_same_v<Key, float>) return PATCH_FLOAT;
            else if constexpr (std::is_same_v<Key, double>) return PATCH_DOUBLE;
            else if constexpr (std::is_same_v<Key, char>) return PATCH_CHAR;
            else return PATCH_OBJECT;
        }

        static patch_type_t GetValuePatchType() {
            if constexpr (std::is_same_v<Value, void>) return PATCH_VOID;
            else if constexpr (std::is_same_v<Value, int8_t>) return PATCH_INT8;
            else if constexpr (std::is_same_v<Value, int16_t>) return PATCH_INT16;
            else if constexpr (std::is_same_v<Value, int32_t>) return PATCH_INT32;
            else if constexpr (std::is_same_v<Value, int64_t>) return PATCH_INT64;
            else if constexpr (std::is_same_v<Value, uint8_t>) return PATCH_UINT8;
            else if constexpr (std::is_same_v<Value, uint16_t>) return PATCH_UINT16;
            else if constexpr (std::is_same_v<Value, uint32_t>) return PATCH_UINT32;
            else if constexpr (std::is_same_v<Value, uint64_t>) return PATCH_UINT64;
            else if constexpr (std::is_same_v<Value, bool>) return PATCH_BOOL;
            else if constexpr (std::is_same_v<Value, float>) return PATCH_FLOAT;
            else if constexpr (std::is_same_v<Value, double>) return PATCH_DOUBLE;
            else if constexpr (std::is_same_v<Value, char>) return PATCH_CHAR;
            else return PATCH_OBJECT;
        }
    };
}