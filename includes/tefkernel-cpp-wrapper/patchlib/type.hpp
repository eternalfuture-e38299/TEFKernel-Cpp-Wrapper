/*******************************************************************************
 * File: type.hpp
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
#include "../tefstd/vector.hpp"
#include <string>

namespace TEFKernel::PatchLib {
    struct Type : Handle {
        Type() = default;

        // 唯一两个入口：
        Type(const Handle &h) : Handle(h) {
        } // 由 Handle 构造（隐式）
        Type(const std::string &ns, const std::string &name); // 由名字查

        // 显式指定所有权（如果需要接管一个已有句柄）
        explicit Type(const Handle &h, const bool takeOwnership) : Handle(h.Raw(), takeOwnership) {
        }

        ~Type();

        void ManualDestroy();

        Type(const Type &other);

        Type &operator=(const Type &other);

        Type(Type &&other) noexcept;

        Type &operator=(Type &&other) noexcept;

        [[nodiscard]] bool IsValid() const;

        void Reset(const Handle &newHandle = {}, bool takeOwnership = false);

        Handle Release();

        [[nodiscard]] Handle GetMonoType() const;

        [[nodiscard]] std::string GetName() const;

        [[nodiscard]] std::string GetNamespace() const;

        [[nodiscard]] std::string GetFullName() const;

        [[nodiscard]] Type GetParent() const;

        static Type GetBasicType(patch_type_t basicType); // patch_type_t 是枚举，保留

        [[nodiscard]] Handle NewInstance() const;

        [[nodiscard]] Type MakeGenericType(const Std::Vector<Handle> &typeArgs) const;

        [[nodiscard]] Type GetInnerType(const std::string &name) const;

        [[nodiscard]] class Field GetField(const std::string &name) const;

        [[nodiscard]] class Property GetProperty(const std::string &name) const;

        [[nodiscard]] class Method GetMethod(const std::string &name) const;

        [[nodiscard]] Method GetMethod(const std::string &name, int argsCount) const;

        [[nodiscard]] Method GetMethod(const std::string &name,
                                       const Std::Vector<std::string> &argsNames) const;

        [[nodiscard]] Method GetMethod(const std::string &name,
                                       const Std::Vector<Type> &argsTypes) const;

        [[nodiscard]] Method GetMethod(const std::string &name,
                                       const Std::Vector<Type> &argsTypes,
                                       const Std::Vector<std::string> &argsNames) const;

        [[nodiscard]] Std::Vector<Type> GetInnerTypes(bool includingParent = false) const;

        [[nodiscard]] Std::Vector<Method> GetMethods(bool includingParent = false) const;

        [[nodiscard]] Std::Vector<Field> GetFields(bool includingParent = false) const;

        [[nodiscard]] Std::Vector<Property> GetProperties(bool includingParent = false) const;

        bool operator==(const Type &other) const;

        bool operator!=(const Type &other) const;

        explicit operator bool() const;

        bool operator==(const Handle &other) const;

        bool operator!=(const Handle &other) const;
    };
}
