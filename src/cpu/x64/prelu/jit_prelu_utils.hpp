/*******************************************************************************
* Copyright 2020-2021 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#ifndef CPU_X64_PRELU_JIT_PRELU_UTILS_HPP
#define CPU_X64_PRELU_JIT_PRELU_UTILS_HPP

#include <set>

#include "cpu/x64/cpu_isa_traits.hpp"

namespace dnnl {
namespace impl {

struct memory_desc_wrapper;

namespace cpu {
namespace x64 {
namespace prelu {

enum class bcast {
    full,
    per_oc_blocked,
    per_oc_n_spatial_c,
    per_oc_n_c_spatial,
    unsupported
};

bcast get_bcast_type(
        const memory_desc_wrapper &lhs, const memory_desc_wrapper &rhs);
cpu_isa_t get_supported_isa();
int get_n_vregs(const cpu_isa_t &isa) noexcept;
bool dt_supported(const std::set<data_type_t> &tensor_data_types) noexcept;
bool is_s8u8(const std::set<data_type_t> &tensor_data_types) noexcept;
int get_simd_w(const std::set<data_type_t> &tensor_data_types) noexcept;

template <typename Vmm>
struct vmm_traits_t {};

template <>
struct vmm_traits_t<Xbyak::Zmm> {
    static constexpr int vlen = 64;
};

template <>
struct vmm_traits_t<Xbyak::Ymm> {
    static constexpr int vlen = 32;
};

template <>
struct vmm_traits_t<Xbyak::Xmm> {
    static constexpr int vlen = 16;
};

} // namespace prelu
} // namespace x64
} // namespace cpu
} // namespace impl
} // namespace dnnl

#endif
