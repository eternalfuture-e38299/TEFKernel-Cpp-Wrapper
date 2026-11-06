/*******************************************************************************
 * File: field
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

#include "tefkernel-cpp-wrapper/patchlib/field.hpp"

using TEFKernel::PatchLib::Field;

std::string Field::GetName() const {
    const char *name = patchlib_field_get_name(_value);
    return name ? std::string(name) : std::string();
}

bool Field::IsStatic() const {
    return patchlib_field_is_static(_value);
}

bool Field::IsInstance() const {
    return patchlib_field_is_instance(_value);
}

bool Field::IsConst() const {
    return patchlib_field_is_const(_value);
}

bool Field::IsThreadStatic() const {
    return patchlib_field_is_thread_static(_value);
}

size_t Field::GetSize() const {
    return patchlib_field_get_size(_value);
}

patch_type_t Field::GetType() const {
    return patchlib_field_get_type(_value);
}

#if __ANDROID__
void *Field::GetPointer(void *instance) const {
    return patchlib_field_get_pointer(_value, instance);
}
#endif