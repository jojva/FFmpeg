;******************************************************************************
;* MMX optimized DSP utils
;* Copyright (c) 2008 Loren Merritt
;* Copyright (c) 2003-2013 Michael Niedermayer
;* Copyright (c) 2013 Daniel Kang
;*
;* This file is part of FFmpeg.
;*
;* FFmpeg is free software; you can redistribute it and/or
;* modify it under the terms of the GNU Lesser General Public
;* License as published by the Free Software Foundation; either
;* version 2.1 of the License, or (at your option) any later version.
;*
;* FFmpeg is distributed in the hope that it will be useful,
;* but WITHOUT ANY WARRANTY; without even the implied warranty of
;* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;* Lesser General Public License for more details.
;*
;* You should have received a copy of the GNU Lesser General Public
;* License along with FFmpeg; if not, write to the Free Software
;* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
;******************************************************************************

%include "libavutil/x86/x86util.asm"

SECTION .text

%macro op_avgh 3
    movh   %3, %2
    pavgb  %1, %3
    movh   %2, %1
%endmacro

%macro op_avg 2
    pavgb  %1, %2
    mova   %2, %1
%endmacro

%macro op_puth 2-3
    movh   %2, %1
%endmacro

%macro op_put 2
    mova   %2, %1
%endmacro

; void pixels4_l2_mmxext(uint8_t *dst, uint8_t *src1, uint8_t *src2, int dstStride, int src1Stride, int h)
%macro PIXELS4_L2 1
%define OP op_%1h
cglobal %1_pixels4_l2, 6,6
    movsxdifnidn r3, r3d
    movsxdifnidn r4, r4d
    test        r5d, 1
    je        .loop
    movd         m0, [r1]
    movd         m1, [r2]
    add          r1, r4
    add          r2, 4
    pavgb        m0, m1
    OP           m0, [r0], m3
    add          r0, r3
    dec         r5d
.loop:
    mova         m0, [r1]
    mova         m1, [r1+r4]
    lea          r1, [r1+2*r4]
    pavgb        m0, [r2]
    pavgb        m1, [r2+4]
    OP           m0, [r0], m3
    OP           m1, [r0+r3], m3
    lea          r0, [r0+2*r3]
    mova         m0, [r1]
    mova         m1, [r1+r4]
    lea          r1, [r1+2*r4]
    pavgb        m0, [r2+8]
    pavgb        m1, [r2+12]
    OP           m0, [r0], m3
    OP           m1, [r0+r3], m3
    lea          r0, [r0+2*r3]
    add          r2, 16
    sub         r5d, 4
    jne       .loop
    REP_RET
%endmacro

INIT_MMX mmxext
PIXELS4_L2 put
PIXELS4_L2 avg

; void pixels8_l2_mmxext(uint8_t *dst, uint8_t *src1, uint8_t *src2, int dstStride, int src1Stride, int h)
%macro PIXELS8_L2 1
%define OP op_%1
cglobal %1_pixels8_l2, 6,6
    movsxdifnidn r3, r3d
    movsxdifnidn r4, r4d
    test        r5d, 1
    je        .loop
    mova         m0, [r1]
    mova         m1, [r2]
    add          r1, r4
    add          r2, 8
    pavgb        m0, m1
    OP           m0, [r0]
    add          r0, r3
    dec         r5d
.loop:
    mova         m0, [r1]
    mova         m1, [r1+r4]
    lea          r1, [r1+2*r4]
    pavgb        m0, [r2]
    pavgb        m1, [r2+8]
    OP           m0, [r0]
    OP           m1, [r0+r3]
    lea          r0, [r0+2*r3]
    mova         m0, [r1]
    mova         m1, [r1+r4]
    lea          r1, [r1+2*r4]
    pavgb        m0, [r2+16]
    pavgb        m1, [r2+24]
    OP           m0, [r0]
    OP           m1, [r0+r3]
    lea          r0, [r0+2*r3]
    add          r2, 32
    sub         r5d, 4
    jne       .loop
    REP_RET
%endmacro

INIT_MMX mmxext
PIXELS8_L2 put
PIXELS8_L2 avg

; void pixels16_l2_mmxext(uint8_t *dst, uint8_t *src1, uint8_t *src2, int dstStride, int src1Stride, int h)
%macro PIXELS16_L2 1
%define OP op_%1
cglobal %1_pixels16_l2, 6,6
    movsxdifnidn r3, r3d
    movsxdifnidn r4, r4d
    test        r5d, 1
    je        .loop
    mova         m0, [r1]
    mova         m1, [r1+8]
    pavgb        m0, [r2]
    pavgb        m1, [r2+8]
    add          r1, r4
    add          r2, 16
    OP           m0, [r0]
    OP           m1, [r0+8]
    add          r0, r3
    dec         r5d
.loop:
    mova         m0, [r1]
    mova         m1, [r1+8]
    add          r1, r4
    pavgb        m0, [r2]
    pavgb        m1, [r2+8]
    OP           m0, [r0]
    OP           m1, [r0+8]
    add          r0, r3
    mova         m0, [r1]
    mova         m1, [r1+8]
    add          r1, r4
    pavgb        m0, [r2+16]
    pavgb        m1, [r2+24]
    OP           m0, [r0]
    OP           m1, [r0+8]
    add          r0, r3
    add          r2, 32
    sub         r5d, 2
    jne       .loop
    REP_RET
%endmacro

INIT_MMX mmxext
PIXELS16_L2 put
PIXELS16_L2 avg

INIT_MMX mmxext
; void pixels(uint8_t *block, const uint8_t *pixels, int line_size, int h)
%macro PIXELS48 2
%if %2 == 4
%define OP movh
%else
%define OP mova
%endif
cglobal %1_pixels%2, 4,5
    movsxdifnidn r2, r2d
    lea          r4, [r2*3]
.loop:
    OP           m0, [r1]
    OP           m1, [r1+r2]
    OP           m2, [r1+r2*2]
    OP           m3, [r1+r4]
    lea          r1, [r1+r2*4]
%ifidn %1, avg
    pavgb        m0, [r0]
    pavgb        m1, [r0+r2]
    pavgb        m2, [r0+r2*2]
    pavgb        m3, [r0+r4]
%endif
    OP         [r0], m0
    OP      [r0+r2], m1
    OP    [r0+r2*2], m2
    OP      [r0+r4], m3
    sub         r3d, 4
    lea          r0, [r0+r2*4]
    jne       .loop
    RET
%endmacro

PIXELS48 put, 4
PIXELS48 avg, 4
PIXELS48 put, 8
PIXELS48 avg, 8


INIT_XMM sse2
; void put_pixels16_sse2(uint8_t *block, const uint8_t *pixels, ptrdiff_t line_size, int h)
cglobal put_pixels16, 4,5,4
    lea          r4, [r2*3]
.loop:
    movu         m0, [r1]
    movu         m1, [r1+r2]
    movu         m2, [r1+r2*2]
    movu         m3, [r1+r4]
    lea          r1, [r1+r2*4]
    mova       [r0], m0
    mova    [r0+r2], m1
    mova  [r0+r2*2], m2
    mova    [r0+r4], m3
    sub         r3d, 4
    lea          r0, [r0+r2*4]
    jnz       .loop
    REP_RET

; void avg_pixels16_sse2(uint8_t *block, const uint8_t *pixels, ptrdiff_t line_size, int h)
cglobal avg_pixels16, 4,5,4
    lea          r4, [r2*3]
.loop:
    movu         m0, [r1]
    movu         m1, [r1+r2]
    movu         m2, [r1+r2*2]
    movu         m3, [r1+r4]
    lea          r1, [r1+r2*4]
    pavgb        m0, [r0]
    pavgb        m1, [r0+r2]
    pavgb        m2, [r0+r2*2]
    pavgb        m3, [r0+r4]
    mova       [r0], m0
    mova    [r0+r2], m1
    mova  [r0+r2*2], m2
    mova    [r0+r4], m3
    sub         r3d, 4
    lea          r0, [r0+r2*4]
    jnz       .loop
    REP_RET
