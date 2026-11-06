/*******************************************************************************
 * File: method
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

#include "tefkernel-cpp-wrapper/patchlib/method.hpp"

#include <cstring>
#include <memory>

using TEFKernel::PatchLib::Handle;
using TEFKernel::PatchLib::HookCallbacks;
using TEFKernel::PatchLib::HookId;
using TEFKernel::PatchLib::Method;
using TEFKernel::PatchLib::MethodSignature;

using TEFKernel::PatchLib::MethodSignature;

MethodSignature::MethodSignature() : patch_method_signature_t() {
    method      = nullptr;
    is_instance = false;
    return_type = PATCH_VOID;
    arg_types   = {};
    arg_names   = {};
    token       = 0;
}

MethodSignature::~MethodSignature() {
    patchlib_method_signature_free(this);
}

MethodSignature::MethodSignature(MethodSignature &&other) noexcept
    : patch_method_signature_t(other) {
    std::memset(static_cast<patch_method_signature_t *>(&other), 0,
                sizeof(patch_method_signature_t));
}

MethodSignature &MethodSignature::operator=(MethodSignature &&other) noexcept {
    if (this != &other) {
        patchlib_method_signature_free(this);
        static_cast<patch_method_signature_t &>(*this) = other;
        std::memset(static_cast<patch_method_signature_t *>(&other), 0,
                    sizeof(patch_method_signature_t));
    }
    return *this;
}

TEFKernel::Std::Vector<patch_type_t> MethodSignature::GetArgTypes() const {
    Std::Vector<patch_type_t> result;
    const auto *types = static_cast<const patch_type_t *>(arg_types.data);
    for (size_t i = 0; i < tefstd_vector_size(&arg_types); ++i) {
        result.push_back(types[i]);
    }
    return result;
}

TEFKernel::Std::Vector<std::string> MethodSignature::GetArgNames() const {
    Std::Vector<std::string> result;
    const auto *names = static_cast<const char * const *>(arg_names.data);
    for (size_t i = 0; i < tefstd_vector_size(&arg_names); ++i) {
        if (names[i]) result.push_back(std::string(names[i]));
    }
    return result;
}

// ==================== Method ====================
std::string Method::GetName() const {
    const char *name = patchlib_method_get_name(_value);
    return name ? std::string(name) : std::string();
}

int Method::GetParamCount() const        { return patchlib_method_get_param_count(_value); }
bool Method::IsInstance() const          { return patchlib_method_is_instance(_value); }
bool Method::IsStatic() const            { return patchlib_method_is_static(_value); }
int Method::GetToken() const             { return patchlib_method_get_token(_value); }
patch_type_t Method::GetReturnType() const { return patchlib_method_get_return_type(_value); }

Method Method::MakeGenericInstance(const Std::Vector<Handle> &templateTypes) const {
    Std::Vector<patch_handle_t> raw;
    raw.reserve(templateTypes.size());
    for (const auto &h : templateTypes) raw.push_back(h.Raw());

    return {Handle(patchlib_method_make_generic_instance(_value, raw.getHandle()), true)};
}

#if __ANDROID__
void *Method::GetPointer() const {
    return patchlib_method_get_pointer(_value);
}
#endif

std::unique_ptr<MethodSignature> Method::GetSignature() const {
    if (auto sig = std::make_unique<MethodSignature>();
        patchlib_method_get_signature(_value,
            const_cast<patch_method_signature_t *>(sig->GetRaw()))) {
        return sig;
    }
    return nullptr;
}

bool Method::UninstallHook(const HookId &hookId) {
    return patchlib_uninstall_hook(hookId.Get());
}