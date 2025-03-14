/*******************************************************************************
* Copyright 2022-2025 Intel Corporation
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

#ifndef GPU_INTEL_JIT_IR_BLOCK_2D_UTILS_HPP
#define GPU_INTEL_JIT_IR_BLOCK_2D_UTILS_HPP

#include <algorithm>

#include "gpu/intel/jit/utils/utils.hpp"
#include "ngen.hpp"

namespace dnnl {
namespace impl {
namespace gpu {
namespace intel {
namespace jit {

inline int block_2d_min_dim() {
    return 64;
}

inline int block_2d_max_dim() {
    return 1 << 24;
}

inline int block_2d_base_alignment(ngen::HW hw) {
    switch (hw) {
        case ngen::HW::XeHPC: return 64;
        case ngen::HW::Xe2:
        case ngen::HW::Xe3: return 64;
        default: gpu_error_not_expected();
    }
    return 0;
}

inline int block_2d_x_alignment(int type_size) {
    return std::max(4, type_size) / type_size;
}

inline int block_2d_w_alignment(int type_size) {
    return std::max(4, type_size);
}

inline bool block_2d_width_ok(dim_t width, int type_size) {
    dim_t width_bytes = width * type_size;
    if (width_bytes < block_2d_min_dim()) return false;
    if (width_bytes > block_2d_max_dim()) return false;
    if (width_bytes % block_2d_w_alignment(type_size) != 0) return false;
    return true;
}

inline bool block_2d_height_ok(dim_t height) {
    if (height > block_2d_max_dim()) return false;
    return true;
}

inline int block_2d_pitch_alignment(ngen::HW hw) {
    switch (hw) {
        case ngen::HW::XeHPC: return 8;
        case ngen::HW::Xe2: return 16;
        case ngen::HW::Xe3: return 16;
        default: gpu_error_not_expected();
    }
    return 0;
}

inline bool block_2d_pitch_ok(
        ngen::HW hw, dim_t pitch, int type_size, bool use_xy = true) {
    dim_t pitch_bytes = pitch * type_size;
    if (pitch_bytes < block_2d_min_dim()) return false;
    // 2^24 Pitch does not work on Xe2/Xe3
    if (pitch_bytes > block_2d_max_dim() - 1) return false;
    if (pitch_bytes % block_2d_pitch_alignment(hw) != 0) return false;
    // To be able to point the base to different rows.
    if (use_xy && pitch_bytes % block_2d_base_alignment(hw) != 0) return false;
    return true;
}

inline int block_2d_max_count(
        bool is_store, bool is_transpose, int block_width, int type_size) {
    if (is_store || is_transpose) return 1;
    return 64 / (block_width * type_size);
}

} // namespace jit
} // namespace intel
} // namespace gpu
} // namespace impl
} // namespace dnnl

#endif
