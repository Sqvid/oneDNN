/*******************************************************************************
* Copyright 2018-2024 Intel Corporation
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

#include "cpu/x64/jit_generator.hpp"

#include "cpu/x64/gemm/s8x8s32/common_u8.hpp"

namespace dnnl {
namespace impl {
namespace cpu {
namespace x64 {

jit_avx512_core_u8_copy_sum_an_kern::jit_avx512_core_u8_copy_sum_an_kern()
    : jit_generator(jit_name()) {}

void jit_avx512_core_u8_copy_sum_an_kern::generate() {

#ifndef _WIN32
#define M rdi
#define N rsi
#define A rdx
#define LDA rcx
#define ALPHA r8
#define B r9

#define I rax
#define A1 r10
#define A2 r8
#define LDA3 r11

#define ARG_BIAS (24 + stacksize + rsp)

#else

#define M rcx
#define N rdx
#define A r8
#define LDA r9
#define ALPHA rax
#define B rdi

#define I rax
#define A1 rsi
#define A2 r10
#define LDA3 r11

#define ARG_ALPHA 40 + stacksize + rsp
#define ARG_B 48 + stacksize + rsp
#define ARG_BIAS 72 + stacksize + rsp

#endif

    inLocalLabel();
    {
        std::vector<Xbyak::Label> labels(46);

        preamble();
        auto stacksize = get_size_of_abi_save_regs();
#ifdef _WIN32
        mov(ALPHA, ptr[ARG_ALPHA]);
        mov(B, ptr[ARG_B]);
#endif

        bool vnni_ver = mayiuse(avx512_core_vnni);

        static const unsigned one_u4 = 0x01010101u;
        static const int one_s2 = 0x00010001;
        if (vnni_ver) {
            mov(A1, (size_t)&one_u4);
            vbroadcastss(ymm15, ptr[A1]);
        } else {
            mov(A1, (size_t)&one_u4);
            vbroadcastss(zmm15, ptr[A1]);
            mov(A1, (size_t)&one_s2);
            vbroadcastss(zmm14, ptr[A1]);
        }

        mov(M, qword[M]);
        mov(N, qword[N]);
        mov(LDA, qword[LDA]);
        lea(LDA3, ptr[LDA + LDA * 2]);
        sub(A, -128);
        sub(B, -128);
        cmp(N, 0x30);
        jl(labels[16], T_NEAR);
        align(4);

        L(labels[12]);
        mov(A1, A);
        add(A, 0x30);
        vxorps(ymm8, ymm8, ymm8);
        vxorps(ymm9, ymm9, ymm9);
        vxorps(ymm10, ymm10, ymm10);
        vxorps(ymm11, ymm11, ymm11);
        vxorps(ymm12, ymm12, ymm12);
        vxorps(ymm13, ymm13, ymm13);
        mov(I, M);
        sar(I, 0x2);
        jle(labels[13], T_NEAR);
        align(4);

        L(labels[19]);
        vmovdqu(xmm0, xword[A1 - 0x80]);
        vmovdqu(xmm1, xword[A1 + LDA * 1 - 0x80]);
        vmovdqu(xmm2, xword[A1 + LDA * 2 - 0x80]);
        vmovdqu(xmm3, xword[A1 + LDA3 * 1 - 0x80]);
        vpunpcklbw(xmm4, xmm0, xmm1);
        vpunpckhbw(xmm5, xmm0, xmm1);
        vpunpcklbw(xmm6, xmm2, xmm3);
        vpunpckhbw(xmm7, xmm2, xmm3);
        vpunpcklwd(xmm0, xmm4, xmm6);
        vpunpckhwd(xmm1, xmm4, xmm6);
        vpunpcklwd(xmm2, xmm5, xmm7);
        vpunpckhwd(xmm3, xmm5, xmm7);

        vinserti32x4(ymm0, ymm0, xmm1, 1);
        vinserti32x4(ymm2, ymm2, xmm3, 1);

        if (vnni_ver) {
            vpdpbusd(ymm8, ymm15, ymm0);
            vpdpbusd(ymm9, ymm15, ymm2);
            vmovups(yword[B - 0x80], ymm0);
            vmovups(yword[B - 0x60], ymm2);
        } else {
            vinsertf64x4(zmm0, zmm0, ymm2, 1);
            vpmaddubsw(zmm1, zmm15, zmm0);
            vpmaddwd(zmm1, zmm1, zmm14);
            vpaddd(ymm8, ymm8, ymm1);
            vextractf64x4(ymm2, zmm1, 1);
            vpaddd(ymm9, ymm9, ymm2);
            vmovups(zword[B - 0x80], zmm0);
        }

        vmovdqu(xmm0, xword[A1 - 0x70]);
        vmovdqu(xmm1, xword[A1 + LDA * 1 - 0x70]);
        vmovdqu(xmm2, xword[A1 + LDA * 2 - 0x70]);
        vmovdqu(xmm3, xword[A1 + LDA3 * 1 - 0x70]);
        vpunpcklbw(xmm4, xmm0, xmm1);
        vpunpckhbw(xmm5, xmm0, xmm1);
        vpunpcklbw(xmm6, xmm2, xmm3);
        vpunpckhbw(xmm7, xmm2, xmm3);
        vpunpcklwd(xmm0, xmm4, xmm6);
        vpunpckhwd(xmm1, xmm4, xmm6);
        vpunpcklwd(xmm2, xmm5, xmm7);
        vpunpckhwd(xmm3, xmm5, xmm7);

        vinserti32x4(ymm0, ymm0, xmm1, 1);
        vinserti32x4(ymm2, ymm2, xmm3, 1);
        if (vnni_ver) {
            vpdpbusd(ymm10, ymm15, ymm0);
            vpdpbusd(ymm11, ymm15, ymm2);
            vmovups(yword[B - 0x40], ymm0);
            vmovups(yword[B - 0x20], ymm2);
        } else {
            vinsertf64x4(zmm0, zmm0, ymm2, 1);
            vpmaddubsw(zmm1, zmm15, zmm0);
            vpmaddwd(zmm1, zmm1, zmm14);
            vpaddd(ymm10, ymm10, ymm1);
            vextractf64x4(ymm2, zmm1, 1);
            vpaddd(ymm11, ymm11, ymm2);
            vmovups(zword[B - 0x40], zmm0);
        }

        vmovdqu(xmm0, xword[A1 - 0x60]);
        vmovdqu(xmm1, xword[A1 + LDA * 1 - 0x60]);
        vmovdqu(xmm2, xword[A1 + LDA * 2 - 0x60]);
        vmovdqu(xmm3, xword[A1 + LDA3 * 1 - 0x60]);
        lea(A1, ptr[A1 + LDA * 4]);
        vpunpcklbw(xmm4, xmm0, xmm1);
        vpunpckhbw(xmm5, xmm0, xmm1);
        vpunpcklbw(xmm6, xmm2, xmm3);
        vpunpckhbw(xmm7, xmm2, xmm3);
        vpunpcklwd(xmm0, xmm4, xmm6);
        vpunpckhwd(xmm1, xmm4, xmm6);
        vpunpcklwd(xmm2, xmm5, xmm7);
        vpunpckhwd(xmm3, xmm5, xmm7);

        vinserti32x4(ymm0, ymm0, xmm1, 1);
        vinserti32x4(ymm2, ymm2, xmm3, 1);

        if (vnni_ver) {
            vpdpbusd(ymm12, ymm15, ymm0);
            vpdpbusd(ymm13, ymm15, ymm2);
            vmovups(yword[B], ymm0);
            vmovups(yword[B + 0x20], ymm2);
        } else {
            vinsertf64x4(zmm0, zmm0, ymm2, 1);
            vpmaddubsw(zmm1, zmm15, zmm0);
            vpmaddwd(zmm1, zmm1, zmm14);
            vpaddd(ymm12, ymm12, ymm1);
            vextractf64x4(ymm2, zmm1, 1);
            vpaddd(ymm13, ymm13, ymm2);
            vmovups(zword[B], zmm0);
        }

        sub(B, -192);
        dec(I);
        jg(labels[19], T_NEAR);
        align(4);

        L(labels[13]);
        test(M, 0x2);
        jle(labels[14], T_NEAR);
        vmovdqu(xmm0, xword[A1 - 0x80]);
        vmovdqu(xmm1, xword[A1 - 0x70]);
        vmovdqu(xmm2, xword[A1 - 0x60]);
        add(A1, LDA);
        vmovdqu(xmm6, xword[A1 - 0x80]);
        vmovdqu(xmm4, xword[A1 - 0x70]);
        vmovdqu(xmm5, xword[A1 - 0x60]);
        add(A1, LDA);
        vpunpcklbw(xmm3, xmm0, xmm6);
        vpunpckhbw(xmm0, xmm0, xmm6);
        vpmovsxbw(ymm7, xmm3);
        vmovhlps(xmm6, xmm3, xmm3);
        vpmovsxbw(ymm6, xmm6);
        vphaddw(ymm7, ymm7, ymm6);
        vpmovsxwd(ymm7, xmm7);
        vpaddd(ymm8, ymm8, ymm7);
        vmovdqu(xword[B - 0x80], xmm3);
        vpmovsxbw(ymm7, xmm0);
        vmovhlps(xmm6, xmm0, xmm0);
        vpmovsxbw(ymm6, xmm6);
        vphaddw(ymm7, ymm7, ymm6);
        vpmovsxwd(ymm7, xmm7);
        vpaddd(ymm9, ymm9, ymm7);
        vmovdqu(xword[B - 0x70], xmm0);
        vpunpcklbw(xmm3, xmm1, xmm4);
        vpunpckhbw(xmm0, xmm1, xmm4);
        vpmovsxbw(ymm7, xmm3);
        vmovhlps(xmm6, xmm3, xmm3);
        vpmovsxbw(ymm6, xmm6);
        vphaddw(ymm7, ymm7, ymm6);
        vpmovsxwd(ymm7, xmm7);
        vpaddd(ymm10, ymm10, ymm7);
        vmovdqu(xword[B - 0x60], xmm3);
        vpmovsxbw(ymm7, xmm0);
        vmovhlps(xmm6, xmm0, xmm0);
        vpmovsxbw(ymm6, xmm6);
        vphaddw(ymm7, ymm7, ymm6);
        vpmovsxwd(ymm7, xmm7);
        vpaddd(ymm11, ymm11, ymm7);
        vmovdqu(xword[B - 0x50], xmm0);
        vpunpcklbw(xmm3, xmm2, xmm5);
        vpunpckhbw(xmm0, xmm2, xmm5);
        vpmovsxbw(ymm7, xmm3);
        vmovhlps(xmm6, xmm3, xmm3);
        vpmovsxbw(ymm6, xmm6);
        vphaddw(ymm7, ymm7, ymm6);
        vpmovsxwd(ymm7, xmm7);
        vpaddd(ymm12, ymm12, ymm7);
        vmovdqu(xword[B - 0x40], xmm3);
        vpmovsxbw(ymm7, xmm0);
        vmovhlps(xmm6, xmm0, xmm0);
        vpmovsxbw(ymm6, xmm6);
        vphaddw(ymm7, ymm7, ymm6);
        vpmovsxwd(ymm7, xmm7);
        vpaddd(ymm13, ymm13, ymm7);
        vmovdqu(xword[B - 0x30], xmm0);
        sub(B, -96);
        align(4);

        L(labels[14]);
        test(M, 0x1);
        jle(labels[15], T_NEAR);
        vmovdqu(xmm0, xword[A1 - 0x80]);
        vmovdqu(xmm1, xword[A1 - 0x70]);
        vmovdqu(xmm2, xword[A1 - 0x60]);
        add(A1, LDA);
        vpmovsxbd(ymm7, xmm0);
        vpaddd(ymm8, ymm8, ymm7);
        vmovhlps(xmm7, xmm0, xmm0);
        vpmovsxbd(ymm7, xmm7);
        vpaddd(ymm9, ymm9, ymm7);
        vmovdqu(xword[B - 0x80], xmm0);
        vpmovsxbd(ymm7, xmm1);
        vpaddd(ymm10, ymm10, ymm7);
        vmovhlps(xmm7, xmm1, xmm1);
        vpmovsxbd(ymm7, xmm7);
        vpaddd(ymm11, ymm11, ymm7);
        vmovdqu(xword[B - 0x70], xmm1);
        vpmovsxbd(ymm7, xmm2);
        vpaddd(ymm12, ymm12, ymm7);
        vmovhlps(xmm7, xmm2, xmm2);
        vpmovsxbd(ymm7, xmm7);
        vpaddd(ymm13, ymm13, ymm7);
        vmovdqu(xword[B - 0x60], xmm2);
        sub(B, -48);
        align(4);

        L(labels[15]);
        mov(A1, qword[ARG_BIAS]);
        vmovdqu(yword[A1], ymm8);
        vmovdqu(yword[A1 + 0x20], ymm9);
        vmovdqu(yword[A1 + 0x40], ymm10);
        vmovdqu(yword[A1 + 0x60], ymm11);
        vmovdqu(yword[A1 + 0x80], ymm12);
        vmovdqu(yword[A1 + 0xa0], ymm13);
        add(qword[ARG_BIAS], 0xc0);
        sub(N, 0x30);
        cmp(N, 0x30);
        jge(labels[12], T_NEAR);
        vzeroupper();
        align(4);

        L(labels[16]);
        cmp(N, 0x20);
        jl(labels[23], T_NEAR);
        align(4);

        L(labels[17]);
        mov(A1, A);
        add(A, 0x20);
        pxor(xmm8, xmm8);
        pxor(xmm9, xmm9);
        pxor(xmm10, xmm10);
        pxor(xmm11, xmm11);
        pxor(xmm12, xmm12);
        pxor(xmm13, xmm13);
        pxor(xmm14, xmm14);
        pxor(xmm15, xmm15);
        mov(I, M);
        sar(I, 0x2);
        jle(labels[20], T_NEAR);
        align(4);

        L(labels[18]);
        movdqu(xmm0, xword[A1 - 0x80]);
        movdqu(xmm1, xword[A1 + LDA * 1 - 0x80]);
        movdqu(xmm2, xword[A1 + LDA * 2 - 0x80]);
        movdqu(xmm3, xword[A1 + LDA3 * 1 - 0x80]);
        movdqa(xmm4, xmm0);
        punpcklbw(xmm0, xmm1);
        punpckhbw(xmm4, xmm1);
        movdqa(xmm5, xmm2);
        punpcklbw(xmm2, xmm3);
        punpckhbw(xmm5, xmm3);
        movdqa(xmm1, xmm0);
        punpcklwd(xmm0, xmm2);
        punpckhwd(xmm1, xmm2);
        movdqa(xmm2, xmm4);
        punpcklwd(xmm4, xmm5);
        punpckhwd(xmm2, xmm5);
        pmovsxbw(xmm5, xmm0);
        movhlps(xmm6, xmm0);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm5, xmm6);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm8, xmm5);
        movdqu(xword[B - 0x80], xmm0);
        pmovsxbw(xmm5, xmm1);
        movhlps(xmm6, xmm1);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm5, xmm6);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm9, xmm5);
        movdqu(xword[B - 0x70], xmm1);
        pmovsxbw(xmm5, xmm4);
        movhlps(xmm6, xmm4);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm5, xmm6);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm10, xmm5);
        movdqu(xword[B - 0x60], xmm4);
        pmovsxbw(xmm5, xmm2);
        movhlps(xmm6, xmm2);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm5, xmm6);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm11, xmm5);
        movdqu(xword[B - 0x50], xmm2);
        movdqu(xmm0, xword[A1 - 0x70]);
        movdqu(xmm1, xword[A1 + LDA * 1 - 0x70]);
        movdqu(xmm2, xword[A1 + LDA * 2 - 0x70]);
        movdqu(xmm3, xword[A1 + LDA3 * 1 - 0x70]);
        lea(A1, ptr[A1 + LDA * 4]);
        movdqa(xmm4, xmm0);
        punpcklbw(xmm0, xmm1);
        punpckhbw(xmm4, xmm1);
        movdqa(xmm5, xmm2);
        punpcklbw(xmm2, xmm3);
        punpckhbw(xmm5, xmm3);
        movdqa(xmm1, xmm0);
        punpcklwd(xmm0, xmm2);
        punpckhwd(xmm1, xmm2);
        movdqa(xmm2, xmm4);
        punpcklwd(xmm4, xmm5);
        punpckhwd(xmm2, xmm5);
        pmovsxbw(xmm5, xmm0);
        movhlps(xmm6, xmm0);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm5, xmm6);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm12, xmm5);
        movdqu(xword[B - 0x40], xmm0);
        pmovsxbw(xmm5, xmm1);
        movhlps(xmm6, xmm1);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm5, xmm6);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm13, xmm5);
        movdqu(xword[B - 0x30], xmm1);
        pmovsxbw(xmm5, xmm4);
        movhlps(xmm6, xmm4);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm5, xmm6);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm14, xmm5);
        movdqu(xword[B - 0x20], xmm4);
        pmovsxbw(xmm5, xmm2);
        movhlps(xmm6, xmm2);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm5, xmm6);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm15, xmm5);
        movdqu(xword[B - 0x10], xmm2);
        sub(B, -128);
        dec(I);
        jg(labels[18], T_NEAR);
        align(4);

        L(labels[20]);
        test(M, 0x2);
        jle(labels[21], T_NEAR);
        movdqu(xmm0, xword[A1 - 0x80]);
        movdqu(xmm1, xword[A1 - 0x70]);
        add(A1, LDA);
        movdqu(xmm2, xword[A1 - 0x80]);
        movdqu(xmm3, xword[A1 - 0x70]);
        add(A1, LDA);
        movdqa(xmm4, xmm0);
        punpcklbw(xmm0, xmm2);
        punpckhbw(xmm4, xmm2);
        pmovsxbw(xmm5, xmm0);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm8, xmm5);
        movhlps(xmm6, xmm0);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm6, xmm6);
        pmovsxwd(xmm6, xmm6);
        paddd(xmm9, xmm6);
        movdqu(xword[B - 0x80], xmm0);
        pmovsxbw(xmm5, xmm4);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm10, xmm5);
        movhlps(xmm6, xmm4);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm6, xmm6);
        pmovsxwd(xmm6, xmm6);
        paddd(xmm11, xmm6);
        movdqu(xword[B - 0x70], xmm4);
        movdqa(xmm4, xmm1);
        punpcklbw(xmm1, xmm3);
        punpckhbw(xmm4, xmm3);
        pmovsxbw(xmm5, xmm1);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm12, xmm5);
        movhlps(xmm6, xmm1);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm6, xmm6);
        pmovsxwd(xmm6, xmm6);
        paddd(xmm13, xmm6);
        movdqu(xword[B - 0x60], xmm1);
        pmovsxbw(xmm5, xmm4);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm14, xmm5);
        movhlps(xmm6, xmm4);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm6, xmm6);
        pmovsxwd(xmm6, xmm6);
        paddd(xmm15, xmm6);
        movdqu(xword[B - 0x50], xmm4);
        sub(B, -64);
        align(4);

        L(labels[21]);
        test(M, 0x1);
        jle(labels[22], T_NEAR);
        movdqu(xmm0, xword[A1 - 0x80]);
        movdqu(xmm1, xword[A1 - 0x70]);
        add(A1, LDA);
        pmovsxbd(xmm5, xmm0);
        paddd(xmm8, xmm5);
        pshufd(xmm6, xmm0, 0x55);
        pmovsxbd(xmm6, xmm6);
        paddd(xmm9, xmm6);
        pshufd(xmm5, xmm0, 0xaa);
        pmovsxbd(xmm5, xmm5);
        paddd(xmm10, xmm5);
        pshufd(xmm6, xmm0, 0xff);
        pmovsxbd(xmm6, xmm6);
        paddd(xmm11, xmm6);
        movdqu(xword[B - 0x80], xmm0);
        pmovsxbd(xmm5, xmm1);
        paddd(xmm12, xmm5);
        pshufd(xmm6, xmm1, 0x55);
        pmovsxbd(xmm6, xmm6);
        paddd(xmm13, xmm6);
        pshufd(xmm5, xmm1, 0xaa);
        pmovsxbd(xmm5, xmm5);
        paddd(xmm14, xmm5);
        pshufd(xmm6, xmm1, 0xff);
        pmovsxbd(xmm6, xmm6);
        paddd(xmm15, xmm6);
        movdqu(xword[B - 0x70], xmm1);
        sub(B, -32);
        align(4);

        L(labels[22]);
        mov(A1, qword[ARG_BIAS]);
        movdqu(xword[A1], xmm8);
        movdqu(xword[A1 + 0x10], xmm9);
        movdqu(xword[A1 + 0x20], xmm10);
        movdqu(xword[A1 + 0x30], xmm11);
        movdqu(xword[A1 + 0x40], xmm12);
        movdqu(xword[A1 + 0x50], xmm13);
        movdqu(xword[A1 + 0x60], xmm14);
        movdqu(xword[A1 + 0x70], xmm15);
        add(qword[ARG_BIAS], 0x80);
        sub(N, 0x20);
        cmp(N, 0x20);
        jge(labels[17], T_NEAR);
        align(4);

        L(labels[23]);
        cmp(N, 0x10);
        jl(labels[29], T_NEAR);
        align(4);

        L(labels[24]);
        mov(A1, A);
        add(A, 0x10);
        pxor(xmm8, xmm8);
        pxor(xmm9, xmm9);
        pxor(xmm10, xmm10);
        pxor(xmm11, xmm11);
        mov(I, M);
        sar(I, 0x2);
        jle(labels[26], T_NEAR);
        align(4);

        L(labels[25]);
        movdqu(xmm0, xword[A1 - 0x80]);
        add(A1, LDA);
        movdqu(xmm1, xword[A1 - 0x80]);
        add(A1, LDA);
        movdqu(xmm2, xword[A1 - 0x80]);
        add(A1, LDA);
        movdqu(xmm3, xword[A1 - 0x80]);
        add(A1, LDA);
        movdqa(xmm4, xmm0);
        punpcklbw(xmm0, xmm1);
        punpckhbw(xmm4, xmm1);
        movdqa(xmm1, xmm2);
        punpcklbw(xmm2, xmm3);
        punpckhbw(xmm1, xmm3);
        movdqa(xmm3, xmm0);
        punpcklwd(xmm0, xmm2);
        punpckhwd(xmm3, xmm2);
        movdqa(xmm2, xmm4);
        punpcklwd(xmm4, xmm1);
        punpckhwd(xmm2, xmm1);
        pmovsxbw(xmm5, xmm0);
        movhlps(xmm6, xmm0);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm5, xmm6);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm8, xmm5);
        pmovsxbw(xmm5, xmm3);
        movhlps(xmm6, xmm3);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm5, xmm6);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm9, xmm5);
        movdqu(xword[B - 0x80], xmm0);
        movdqu(xword[B - 0x70], xmm3);
        pmovsxbw(xmm5, xmm4);
        movhlps(xmm6, xmm4);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm5, xmm6);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm10, xmm5);
        pmovsxbw(xmm5, xmm2);
        movhlps(xmm6, xmm2);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm5, xmm6);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm11, xmm5);
        movdqu(xword[B - 0x60], xmm4);
        movdqu(xword[B - 0x50], xmm2);
        sub(B, -64);
        dec(I);
        jg(labels[25], T_NEAR);
        align(4);

        L(labels[26]);
        test(M, 0x2);
        jle(labels[27], T_NEAR);
        movdqu(xmm0, xword[A1 - 0x80]);
        add(A1, LDA);
        movdqu(xmm1, xword[A1 - 0x80]);
        add(A1, LDA);
        movdqa(xmm2, xmm0);
        punpcklbw(xmm0, xmm1);
        punpckhbw(xmm2, xmm1);
        pmovsxbw(xmm5, xmm0);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm8, xmm5);
        movhlps(xmm6, xmm0);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm6, xmm6);
        pmovsxwd(xmm6, xmm6);
        paddd(xmm9, xmm6);
        pmovsxbw(xmm5, xmm2);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm10, xmm5);
        movhlps(xmm6, xmm2);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm6, xmm6);
        pmovsxwd(xmm6, xmm6);
        paddd(xmm11, xmm6);
        movdqu(xword[B - 0x80], xmm0);
        movdqu(xword[B - 0x70], xmm2);
        sub(B, -32);
        align(4);

        L(labels[27]);
        test(M, 0x1);
        jle(labels[28], T_NEAR);
        movdqu(xmm0, xword[A1 - 0x80]);
        add(A1, LDA);
        pmovsxbd(xmm5, xmm0);
        paddd(xmm8, xmm5);
        pshufd(xmm6, xmm0, 0x55);
        pmovsxbd(xmm6, xmm6);
        paddd(xmm9, xmm6);
        pshufd(xmm5, xmm0, 0xaa);
        pmovsxbd(xmm5, xmm5);
        paddd(xmm10, xmm5);
        pshufd(xmm6, xmm0, 0xff);
        pmovsxbd(xmm6, xmm6);
        paddd(xmm11, xmm6);
        movdqu(xword[B - 0x80], xmm0);
        sub(B, -16);
        align(4);

        L(labels[28]);
        mov(A1, qword[ARG_BIAS]);
        movdqu(xword[A1], xmm8);
        movdqu(xword[A1 + 0x10], xmm9);
        movdqu(xword[A1 + 0x20], xmm10);
        movdqu(xword[A1 + 0x30], xmm11);
        add(qword[ARG_BIAS], 0x40);
        sub(N, 0x10);
        cmp(N, 0x10);
        jge(labels[24], T_NEAR);
        align(4);

        L(labels[29]);
        cmp(N, 0x8);
        jl(labels[36], T_NEAR);
        align(4);

        L(labels[30]);
        mov(A1, A);
        add(A, 0x8);
        pxor(xmm8, xmm8);
        pxor(xmm9, xmm9);
        mov(I, M);
        sar(I, 0x3);
        jle(labels[32], T_NEAR);
        align(4);

        L(labels[31]);
        movq(xmm0, qword[A1 - 0x80]);
        add(A1, LDA);
        movq(xmm1, qword[A1 - 0x80]);
        add(A1, LDA);
        movq(xmm2, qword[A1 - 0x80]);
        add(A1, LDA);
        movq(xmm3, qword[A1 - 0x80]);
        add(A1, LDA);
        punpcklbw(xmm0, xmm1);
        punpcklbw(xmm2, xmm3);
        movdqa(xmm1, xmm0);
        punpcklwd(xmm0, xmm2);
        punpckhwd(xmm1, xmm2);
        pmovsxbw(xmm5, xmm0);
        movhlps(xmm6, xmm0);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm5, xmm6);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm8, xmm5);
        pmovsxbw(xmm5, xmm1);
        movhlps(xmm6, xmm1);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm5, xmm6);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm9, xmm5);
        movdqu(xword[B - 0x80], xmm0);
        movdqu(xword[B - 0x70], xmm1);
        movq(xmm0, qword[A1 - 0x80]);
        add(A1, LDA);
        movq(xmm1, qword[A1 - 0x80]);
        add(A1, LDA);
        movq(xmm2, qword[A1 - 0x80]);
        add(A1, LDA);
        movq(xmm3, qword[A1 - 0x80]);
        add(A1, LDA);
        punpcklbw(xmm0, xmm1);
        punpcklbw(xmm2, xmm3);
        movdqa(xmm1, xmm0);
        punpcklwd(xmm0, xmm2);
        punpckhwd(xmm1, xmm2);
        pmovsxbw(xmm5, xmm0);
        movhlps(xmm6, xmm0);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm5, xmm6);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm8, xmm5);
        pmovsxbw(xmm5, xmm1);
        movhlps(xmm6, xmm1);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm5, xmm6);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm9, xmm5);
        movdqu(xword[B - 0x60], xmm0);
        movdqu(xword[B - 0x50], xmm1);
        sub(B, -64);
        dec(I);
        jg(labels[31], T_NEAR);
        align(4);

        L(labels[32]);
        test(M, 0x4);
        jle(labels[33], T_NEAR);
        movq(xmm0, qword[A1 - 0x80]);
        add(A1, LDA);
        movq(xmm1, qword[A1 - 0x80]);
        add(A1, LDA);
        movq(xmm2, qword[A1 - 0x80]);
        add(A1, LDA);
        movq(xmm3, qword[A1 - 0x80]);
        add(A1, LDA);
        punpcklbw(xmm0, xmm1);
        punpcklbw(xmm2, xmm3);
        movdqa(xmm1, xmm0);
        punpcklwd(xmm0, xmm2);
        punpckhwd(xmm1, xmm2);
        pmovsxbw(xmm5, xmm0);
        movhlps(xmm6, xmm0);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm5, xmm6);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm8, xmm5);
        pmovsxbw(xmm5, xmm1);
        movhlps(xmm6, xmm1);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm5, xmm6);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm9, xmm5);
        movdqu(xword[B - 0x80], xmm0);
        movdqu(xword[B - 0x70], xmm1);
        sub(B, -32);
        align(4);

        L(labels[33]);
        test(M, 0x2);
        jle(labels[34], T_NEAR);
        movq(xmm0, qword[A1 - 0x80]);
        add(A1, LDA);
        movq(xmm1, qword[A1 - 0x80]);
        add(A1, LDA);
        punpcklbw(xmm0, xmm1);
        pmovsxbw(xmm5, xmm0);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm8, xmm5);
        movhlps(xmm6, xmm0);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm6, xmm6);
        pmovsxwd(xmm6, xmm6);
        paddd(xmm9, xmm6);
        movdqu(xword[B - 0x80], xmm0);
        sub(B, -16);
        align(4);

        L(labels[34]);
        test(M, 0x1);
        jle(labels[35], T_NEAR);
        movq(xmm0, qword[A1 - 0x80]);
        add(A1, LDA);
        pmovsxbd(xmm5, xmm0);
        pshufd(xmm6, xmm0, 0x55);
        pmovsxbd(xmm6, xmm6);
        paddd(xmm8, xmm5);
        paddd(xmm9, xmm6);
        movq(qword[B - 0x80], xmm0);
        sub(B, -8);
        align(4);

        L(labels[35]);
        mov(A1, qword[ARG_BIAS]);
        movdqu(xword[A1], xmm8);
        movdqu(xword[A1 + 0x10], xmm9);
        add(qword[ARG_BIAS], 0x20);
        sub(N, 0x8);
        cmp(N, 0x8);
        jge(labels[30], T_NEAR);
        align(4);

        L(labels[36]);
        cmp(N, 0x4);
        jl(labels[43], T_NEAR);
        align(4);

        L(labels[37]);
        mov(A1, A);
        add(A, 0x4);
        pxor(xmm7, xmm7);
        mov(I, M);
        sar(I, 0x3);
        jle(labels[39], T_NEAR);
        align(4);

        L(labels[38]);
        movd(xmm0, dword[A1 - 0x80]);
        add(A1, LDA);
        movd(xmm1, dword[A1 - 0x80]);
        add(A1, LDA);
        movd(xmm2, dword[A1 - 0x80]);
        add(A1, LDA);
        movd(xmm3, dword[A1 - 0x80]);
        add(A1, LDA);
        punpcklbw(xmm0, xmm1);
        punpcklbw(xmm2, xmm3);
        punpcklwd(xmm0, xmm2);
        pmovsxbw(xmm5, xmm0);
        movhlps(xmm6, xmm0);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm5, xmm6);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm7, xmm5);
        movdqu(xword[B - 0x80], xmm0);
        movd(xmm0, dword[A1 - 0x80]);
        add(A1, LDA);
        movd(xmm1, dword[A1 - 0x80]);
        add(A1, LDA);
        movd(xmm2, dword[A1 - 0x80]);
        add(A1, LDA);
        movd(xmm3, dword[A1 - 0x80]);
        add(A1, LDA);
        punpcklbw(xmm0, xmm1);
        punpcklbw(xmm2, xmm3);
        punpcklwd(xmm0, xmm2);
        pmovsxbw(xmm5, xmm0);
        movhlps(xmm6, xmm0);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm5, xmm6);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm7, xmm5);
        movdqu(xword[B - 0x70], xmm0);
        sub(B, -32);
        dec(I);
        jg(labels[38], T_NEAR);
        align(4);

        L(labels[39]);
        test(M, 0x4);
        jle(labels[40], T_NEAR);
        movd(xmm0, dword[A1 - 0x80]);
        add(A1, LDA);
        movd(xmm1, dword[A1 - 0x80]);
        add(A1, LDA);
        movd(xmm2, dword[A1 - 0x80]);
        add(A1, LDA);
        movd(xmm3, dword[A1 - 0x80]);
        add(A1, LDA);
        punpcklbw(xmm0, xmm1);
        punpcklbw(xmm2, xmm3);
        punpcklwd(xmm0, xmm2);
        pmovsxbw(xmm5, xmm0);
        movhlps(xmm6, xmm0);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm5, xmm6);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm7, xmm5);
        movdqu(xword[B - 0x80], xmm0);
        sub(B, -16);
        align(4);

        L(labels[40]);
        test(M, 0x2);
        jle(labels[41], T_NEAR);
        movd(xmm0, dword[A1 - 0x80]);
        add(A1, LDA);
        movd(xmm1, dword[A1 - 0x80]);
        add(A1, LDA);
        punpcklbw(xmm0, xmm1);
        pmovsxbw(xmm5, xmm0);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm7, xmm5);
        movq(qword[B - 0x80], xmm0);
        sub(B, -8);
        align(4);

        L(labels[41]);
        test(M, 0x1);
        jle(labels[42], T_NEAR);
        movd(xmm0, dword[A1 - 0x80]);
        pmovsxbd(xmm5, xmm0);
        paddd(xmm7, xmm5);
        movd(dword[B - 0x80], xmm0);
        sub(B, -4);
        align(4);

        L(labels[42]);
        mov(A1, qword[ARG_BIAS]);
        movdqu(xword[A1], xmm7);
        add(qword[ARG_BIAS], 0x10);
        sub(N, 0x4);
        cmp(N, 0x4);
        jge(labels[37], T_NEAR);
        align(4);

        L(labels[43]);
        cmp(N, 0x2);
        jl(labels[4], T_NEAR);
        align(4);

        L(labels[44]);
        mov(A1, A);
        add(A, 0x2);
        pxor(xmm7, xmm7);
        mov(LDA3, M);
        sar(LDA3, 0x3);
        jle(labels[0], T_NEAR);
        align(4);

        L(labels[45]);
        mov(ax, word[A1 - 0x80]);
        add(A1, LDA);
        pinsrw(xmm0, eax, 0x0);
        mov(ax, word[A1 - 0x80]);
        add(A1, LDA);
        pinsrw(xmm1, eax, 0x0);
        mov(ax, word[A1 - 0x80]);
        add(A1, LDA);
        pinsrw(xmm2, eax, 0x0);
        mov(ax, word[A1 - 0x80]);
        add(A1, LDA);
        pinsrw(xmm3, eax, 0x0);
        punpcklbw(xmm0, xmm1);
        punpcklbw(xmm2, xmm3);
        punpcklwd(xmm0, xmm2);
        mov(ax, word[A1 - 0x80]);
        add(A1, LDA);
        pinsrw(xmm1, eax, 0x0);
        mov(ax, word[A1 - 0x80]);
        add(A1, LDA);
        pinsrw(xmm2, eax, 0x0);
        mov(ax, word[A1 - 0x80]);
        add(A1, LDA);
        pinsrw(xmm3, eax, 0x0);
        mov(ax, word[A1 - 0x80]);
        add(A1, LDA);
        pinsrw(xmm4, eax, 0x0);
        punpcklbw(xmm1, xmm2);
        punpcklbw(xmm3, xmm4);
        punpcklwd(xmm1, xmm3);
        punpcklqdq(xmm0, xmm1);
        pshufd(xmm6, xmm0, 0xd8);
        pmovsxbw(xmm5, xmm6);
        movhlps(xmm6, xmm6);
        pmovsxbw(xmm6, xmm6);
        phaddw(xmm5, xmm6);
        phaddw(xmm5, xmm5);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm7, xmm5);
        movdqu(xword[B - 0x80], xmm0);
        sub(B, -16);
        dec(LDA3);
        jg(labels[45], T_NEAR);
        align(4);

        L(labels[0]);
        test(M, 0x4);
        jle(labels[1], T_NEAR);
        mov(ax, word[A1 - 0x80]);
        add(A1, LDA);
        pinsrw(xmm0, eax, 0x0);
        mov(ax, word[A1 - 0x80]);
        add(A1, LDA);
        pinsrw(xmm1, eax, 0x0);
        mov(ax, word[A1 - 0x80]);
        add(A1, LDA);
        pinsrw(xmm2, eax, 0x0);
        mov(ax, word[A1 - 0x80]);
        add(A1, LDA);
        pinsrw(xmm3, eax, 0x0);
        punpcklbw(xmm0, xmm1);
        punpcklbw(xmm2, xmm3);
        punpcklwd(xmm0, xmm2);
        pmovsxbw(xmm5, xmm0);
        phaddw(xmm5, xmm5);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm7, xmm5);
        movq(qword[B - 0x80], xmm0);
        sub(B, -8);
        align(4);

        L(labels[1]);
        test(M, 0x2);
        jle(labels[2], T_NEAR);
        mov(ax, word[A1 - 0x80]);
        add(A1, LDA);
        pinsrw(xmm0, eax, 0x0);
        mov(ax, word[A1 - 0x80]);
        add(A1, LDA);
        pinsrw(xmm1, eax, 0x0);
        punpcklbw(xmm0, xmm1);
        pmovsxbw(xmm5, xmm0);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm7, xmm5);
        movd(dword[B - 0x80], xmm0);
        sub(B, -4);
        align(4);

        L(labels[2]);
        test(M, 0x1);
        jle(labels[3], T_NEAR);
        mov(ax, word[A1 - 0x80]);
        pinsrw(xmm0, eax, 0x0);
        pmovsxbd(xmm5, xmm0);
        paddd(xmm7, xmm5);
        mov(word[B - 0x80], ax);
        sub(B, -2);
        align(4);

        L(labels[3]);
        mov(A1, qword[ARG_BIAS]);
        movq(qword[A1], xmm7);
        add(qword[ARG_BIAS], 0x8);
        sub(N, 0x2);
        cmp(N, 0x2);
        jge(labels[44], T_NEAR);
        align(4);

        L(labels[4]);
        cmp(N, 0x1);
        jl(labels[11], T_NEAR);
        align(4);

        L(labels[5]);
        mov(A1, A);
        add(A, 0x1);
        pxor(xmm7, xmm7);
        mov(LDA3, M);
        sar(LDA3, 0x3);
        jle(labels[7], T_NEAR);
        align(4);

        L(labels[6]);
        mov(al, byte[A1 - 0x80]);
        add(A1, LDA);
        pinsrb(xmm0, eax, 0x0);
        mov(al, byte[A1 - 0x80]);
        add(A1, LDA);
        pinsrb(xmm0, eax, 0x1);
        mov(al, byte[A1 - 0x80]);
        add(A1, LDA);
        pinsrb(xmm0, eax, 0x2);
        mov(al, byte[A1 - 0x80]);
        add(A1, LDA);
        pinsrb(xmm0, eax, 0x3);
        mov(al, byte[A1 - 0x80]);
        add(A1, LDA);
        pinsrb(xmm0, eax, 0x4);
        mov(al, byte[A1 - 0x80]);
        add(A1, LDA);
        pinsrb(xmm0, eax, 0x5);
        mov(al, byte[A1 - 0x80]);
        add(A1, LDA);
        pinsrb(xmm0, eax, 0x6);
        mov(al, byte[A1 - 0x80]);
        add(A1, LDA);
        pinsrb(xmm0, eax, 0x7);
        pmovsxbw(xmm5, xmm0);
        phaddw(xmm5, xmm6);
        phaddw(xmm5, xmm5);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm7, xmm5);
        movq(qword[B - 0x80], xmm0);
        sub(B, -8);
        dec(LDA3);
        jg(labels[6], T_NEAR);
        align(4);

        L(labels[7]);
        test(M, 0x4);
        jle(labels[8], T_NEAR);
        mov(al, byte[A1 - 0x80]);
        add(A1, LDA);
        pinsrb(xmm0, eax, 0x0);
        mov(al, byte[A1 - 0x80]);
        add(A1, LDA);
        pinsrb(xmm0, eax, 0x1);
        mov(al, byte[A1 - 0x80]);
        add(A1, LDA);
        pinsrb(xmm0, eax, 0x2);
        mov(al, byte[A1 - 0x80]);
        add(A1, LDA);
        pinsrb(xmm0, eax, 0x3);
        pmovsxbw(xmm5, xmm0);
        phaddw(xmm5, xmm5);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm7, xmm5);
        movd(dword[B - 0x80], xmm0);
        sub(B, -4);
        align(4);

        L(labels[8]);
        test(M, 0x2);
        jle(labels[9], T_NEAR);
        mov(al, byte[A1 - 0x80]);
        add(A1, LDA);
        pinsrb(xmm0, eax, 0x0);
        mov(byte[B - 0x80], al);
        mov(al, byte[A1 - 0x80]);
        add(A1, LDA);
        pinsrb(xmm0, eax, 0x1);
        pmovsxbw(xmm5, xmm0);
        phaddw(xmm5, xmm5);
        pmovsxwd(xmm5, xmm5);
        paddd(xmm7, xmm5);
        mov(byte[B - 0x7f], al);
        sub(B, -2);
        align(4);

        L(labels[9]);
        test(M, 0x1);
        jle(labels[10], T_NEAR);
        mov(al, byte[A1 - 0x80]);
        pinsrw(xmm0, eax, 0x0);
        pmovsxbd(xmm5, xmm0);
        paddd(xmm7, xmm5);
        mov(byte[B - 0x80], al);
        sub(B, -1);
        align(4);

        L(labels[10]);
        mov(A1, qword[ARG_BIAS]);
        movd(dword[A1], xmm7);
        add(qword[ARG_BIAS], 0x4);
        sub(N, 0x1);
        cmp(N, 0x1);
        jge(labels[5], T_NEAR);
        align(4);

        L(labels[11]);

        postamble();
    }
    outLocalLabel();

#undef M
#undef N
#undef A
#undef LDA
#undef ALPHA
#undef B
#undef I
#undef A1
#undef A2
#undef LDA3
#ifdef _WIN32
#undef ARG_ALPHA
#undef ARG_B
#endif
#undef ARG_BIAS
}

} // namespace x64
} // namespace cpu
} // namespace impl
} // namespace dnnl
