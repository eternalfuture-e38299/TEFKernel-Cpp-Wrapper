/*******************************************************************************
 * File: type
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

#include "tefkernel-cpp-wrapper/patchlib/type.hpp"

#include "tefkernel-cpp-wrapper/patchlib/field.hpp"
#include "tefkernel-cpp-wrapper/patchlib/property.hpp"

#include <stdexcept>
#include <vector>

using TEFKernel::PatchLib::Field;
using TEFKernel::PatchLib::Handle;
using TEFKernel::PatchLib::Method;
using TEFKernel::PatchLib::Property;
using TEFKernel::PatchLib::Type;

// ==================== Handle(const Type&) ====================
// 视图语义：不接管所有权
Handle::Handle(const Type &t) : _value(t._value), _own(false) {}

// ==================== 构造 / 析构 ====================
Type::Type(const std::string &ns, const std::string &name)
    : Handle(patchlib_type_get_type(ns.c_str(), name.c_str()), true) {
    if (_value == nullptr) {
        throw std::runtime_error("Failed to get type: " + ns + "." + name);
    }
}

Type::~Type() = default;   // Handle 析构负责释放

void Type::ManualDestroy() {
    Reset();   // Handle::Reset 会按 _own 释放并置空
}

// ==================== 拷贝 / 移动（全部委托给 Handle）====================
Type::Type(const Type &other) : Handle(static_cast<const Handle &>(other)) {}

Type &Type::operator=(const Type &other) = default;

Type::Type(Type &&other) noexcept : Handle(std::move(static_cast<Handle &>(other))) {}

Type &Type::operator=(Type &&other) noexcept {
    Handle::operator=(std::move(static_cast<Handle &>(other)));
    return *this;
}

// ==================== 状态 ====================
bool Type::IsValid() const {
    return patchlib_is_valid(_value);
}

void Type::Reset(const Handle &newHandle, const bool takeOwnership) {
    Handle::Reset(newHandle.Raw(), takeOwnership);
}

Handle Type::Release() {
    return Handle(Handle::Release(), _own);   // 注意顺序：先取 _own 再 Release
}
// 更稳妥的写法：
// Handle Type::Release() {
//     Handle h(_value, _own);
//     _value = nullptr;
//     _own   = false;
//     return h;
// }

// ==================== 类型信息 ====================
Handle Type::GetMonoType() const {
#if defined(__ANDROID__)
    return Handle(patchlib_type_get_mono_type(_value), false);
#else
    return Handle(_value, false);
#endif
}

std::string Type::GetName() const {
    const char *name = patchlib_type_get_name(_value);
    return name ? std::string(name) : std::string();
}

std::string Type::GetNamespace() const {
    const char *ns = patchlib_type_get_namespace(_value);
    return ns ? std::string(ns) : std::string();
}

std::string Type::GetFullName() const {
    char *name = patchlib_type_get_full_name(_value);
    if (!name) return {};
    std::string result(name);
    free(name);
    return result;
}

Type Type::GetParent() const {
    return {Handle(patchlib_type_get_parent(_value), true)};
}

Type Type::GetBasicType(const patch_type_t basicType) {
    return {Handle(patchlib_get_basic_type(basicType), true)};
}

Handle Type::NewInstance() const {
    return Handle(patchlib_type_new_instance(_value), false);
}

Type Type::MakeGenericType(const Std::Vector<Handle> &typeArgs) const {
    Std::Vector<patch_handle_t> raw;
    raw.reserve(typeArgs.size());
    for (const auto &h : typeArgs) raw.push_back(h.Raw());

    return {Handle(patchlib_type_make_generic_type(_value, raw.getHandle()), true)};
}

// ==================== 成员获取 ====================
Type Type::GetInnerType(const std::string &name) const {
    return {Handle(patchlib_type_get_inner_type(_value, name.c_str()), true)};
}

Field Type::GetField(const std::string &name) const {
    return {Handle(patchlib_type_get_field(_value, name.c_str()), true)};
}

Property Type::GetProperty(const std::string &name) const {
    return {Handle(patchlib_type_get_property(_value, name.c_str()), true)};
}

Method Type::GetMethod(const std::string &name) const {
    return {Handle(patchlib_type_get_method(_value, name.c_str()), true)};
}

Method Type::GetMethod(const std::string &name, const int argsCount) const {
    return {
        Handle(patchlib_type_get_method_by_param_count(_value, name.c_str(), argsCount), true)};
}

Method Type::GetMethod(const std::string &name,
                       const Std::Vector<std::string> &argsNames) const {
    std::vector<const char *> cStrings;
    cStrings.reserve(argsNames.size());
    for (const auto &s : argsNames) cStrings.push_back(s.c_str());

    return {Handle(
        patchlib_type_get_method_by_param_names(_value, name.c_str(),
                                                static_cast<int>(argsNames.size()),
                                                cStrings.data()),
        true)};
}

Method Type::GetMethod(const std::string &name, const Std::Vector<Type> &argsTypes) const {
    Std::Vector<patch_handle_t> handles;
    handles.reserve(argsTypes.size());
    for (const auto &type : argsTypes) handles.push_back(type.Raw());

    return {Handle(
        patchlib_type_get_method_by_param_types(_value, name.c_str(),
                                                static_cast<int>(argsTypes.size()),
                                                handles.data()),
        true)};
}

Method Type::GetMethod(const std::string &name,
                       const Std::Vector<Type> &argsTypes,
                       const Std::Vector<std::string> &argsNames) const {
    Std::Vector<patch_handle_t> handles;
    handles.reserve(argsTypes.size());
    for (const auto &type : argsTypes) handles.push_back(type.Raw());

    std::vector<const char *> cStrings;
    cStrings.reserve(argsNames.size());
    for (const auto &s : argsNames) cStrings.push_back(s.c_str());

    return {Handle(
        patchlib_type_get_method_by_signature(_value, name.c_str(),
                                              static_cast<int>(argsTypes.size()),
                                              handles.data(), cStrings.data()),
        true)};
}

// ==================== 批量 ====================
TEFKernel::Std::Vector<Type> Type::GetInnerTypes(const bool includingParent) const {
    tefstd_vector_t vec;
    if (!tefstd_vector_init(&vec, sizeof(patch_handle_t))) return {};

    Std::Vector<Type> result;
    if (patchlib_type_get_inner_types(_value, includingParent, &vec)) {
        const auto *data = static_cast<patch_handle_t *>(vec.data);
        for (size_t i = 0; i < vec.size; ++i)
            result.push_back(Type(Handle(data[i], true)));
    }
    tefstd_vector_destroy(&vec);
    return result;
}

TEFKernel::Std::Vector<Field> Type::GetFields(const bool includingParent) const {
    tefstd_vector_t vec;
    if (!tefstd_vector_init(&vec, sizeof(patch_handle_t))) return {};

    Std::Vector<Field> result;
    if (patchlib_type_get_fields(_value, includingParent, &vec)) {
        const auto *data = static_cast<patch_handle_t *>(vec.data);
        for (size_t i = 0; i < vec.size; ++i)
            result.push_back(Field(Handle(data[i], true)));
    }
    tefstd_vector_destroy(&vec);
    return result;
}

TEFKernel::Std::Vector<Property> Type::GetProperties(const bool includingParent) const {
    tefstd_vector_t vec;
    if (!tefstd_vector_init(&vec, sizeof(patch_handle_t))) return {};

    Std::Vector<Property> result;
    if (patchlib_type_get_properties(_value, includingParent, &vec)) {
        const auto *data = static_cast<patch_handle_t *>(vec.data);
        for (size_t i = 0; i < vec.size; ++i)
            result.push_back(Property(Handle(data[i], true)));
    }
    tefstd_vector_destroy(&vec);
    return result;
}

TEFKernel::Std::Vector<Method> Type::GetMethods(const bool includingParent) const {
    tefstd_vector_t vec;
    if (!tefstd_vector_init(&vec, sizeof(patch_handle_t))) return {};

    Std::Vector<Method> result;
    if (patchlib_type_get_methods(_value, includingParent, &vec)) {
        const auto *data = static_cast<patch_handle_t *>(vec.data);
        for (size_t i = 0; i < vec.size; ++i)
            result.push_back(Method(Handle(data[i], true)));
    }
    tefstd_vector_destroy(&vec);
    return result;
}

// ==================== 运算符 ====================
bool Type::operator==(const Type &other) const {
    return patchlib_type_is_same(_value, other._value);
}

bool Type::operator!=(const Type &other) const {
    return !(*this == other);
}

Type::operator bool() const {
    return IsValid();
}

bool Type::operator==(const Handle &other) const {
    return patchlib_type_is_same(_value, other.Raw());
}

bool Type::operator!=(const Handle &other) const {
    return !(*this == other);
}