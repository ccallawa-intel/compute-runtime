/*
 * Copyright (c) 2017, Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "runtime/built_ins/sip.h"
#include "runtime/device/device.h"
#include "runtime/helpers/debug_helpers.h"
#include "runtime/helpers/string.h"

namespace OCLRT {

const char *getSipKernelCompilerInternalOptions(SipKernelType kernel) {
    switch (kernel) {
    default:
        DEBUG_BREAK_IF(true);
        return "";
    case SipKernelType::Csr:
        return "-cl-include-sip-csr";
    }
}

const char *getSipLlSrc(const Device &device) {
#define M_DUMMY_LL_SRC                              \
    "define void @f()  {                        \n" \
    "    ret void                               \n" \
    "}                                          \n" \
    "!opencl.compiler.options = !{!0}           \n" \
    "!opencl.kernels = !{!1}                    \n" \
    "!0 = !{}                                   \n" \
    "!1 = !{void()* @f, !2, !3, !4, !5, !6, !7} \n" \
    "!2 = !{!\"kernel_arg_addr_space\"}         \n" \
    "!3 = !{!\"kernel_arg_access_qual\"}        \n" \
    "!4 = !{!\"kernel_arg_type\"}               \n" \
    "!5 = !{!\"kernel_arg_type_qual\"}          \n" \
    "!6 = !{!\"kernel_arg_base_type\"}          \n" \
    "!7 = !{!\"kernel_arg_name\"}               \n"

    constexpr const char *llDummySrc32 =
        "target datalayout = \"e-p:32:32:32\"       \n"
        "target triple = \"spir\"                   \n" M_DUMMY_LL_SRC;

    constexpr const char *llDummySrc64 =
        "target datalayout = \"e-p:64:64:64\"       \n"
        "target triple = \"spir64\"                 \n" M_DUMMY_LL_SRC;

#undef M_DUMMY_LL_SRC

    const uint32_t ptrSize = device.getDeviceInfo().force32BitAddressess ? 4 : sizeof(void *);

    return (ptrSize == 8) ? llDummySrc64 : llDummySrc32;
}

SipKernel::SipKernel(SipKernelType type, const void *binary, size_t binarySize)
    : type(type) {

    UNRECOVERABLE_IF(binary == nullptr);
    UNRECOVERABLE_IF(binarySize == 0);

    this->binary.reset(new char[binarySize]);
    memcpy_s(this->binary.get(), binarySize, binary, binarySize);
    this->binarySize = binarySize;
}
}
