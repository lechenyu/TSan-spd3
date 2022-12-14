; RUN: llc -mtriple=aarch64-linux-gnu -mattr=+sve < %s | FileCheck %s
; RUN: llc -O0 -mtriple=aarch64-linux-gnu -mattr=+sve < %s | FileCheck %s
; RUN: llc -mtriple=aarch64-linux-gnu -mattr=+sme < %s | FileCheck %s
; RUN: llc -O0 -mtriple=aarch64-linux-gnu -mattr=+sme < %s | FileCheck %s

;
; ST1B
;

define void @st1b_i8(<vscale x 16 x i8> %data, <vscale x 16 x i1> %pred, i8* %addr) {
; CHECK-LABEL: st1b_i8:
; CHECK: st1b { z0.b }, p0, [x0]
; CHECK-NEXT: ret
  call void @llvm.aarch64.sve.st1.nxv16i8(<vscale x 16 x i8> %data,
                                          <vscale x 16 x i1> %pred,
                                          i8* %addr)
  ret void
}

define void @st1b_h(<vscale x 8 x i16> %data, <vscale x 8 x i1> %pred, i8* %addr) {
; CHECK-LABEL: st1b_h:
; CHECK: st1b { z0.h }, p0, [x0]
; CHECK-NEXT: ret
  %trunc = trunc <vscale x 8 x i16> %data to <vscale x 8 x i8>
  call void @llvm.aarch64.sve.st1.nxv8i8(<vscale x 8 x i8> %trunc,
                                         <vscale x 8 x i1> %pred,
                                         i8* %addr)
  ret void
}

define void @st1b_s(<vscale x 4 x i32> %data, <vscale x 4 x i1> %pred, i8* %addr) {
; CHECK-LABEL: st1b_s:
; CHECK: st1b { z0.s }, p0, [x0]
; CHECK-NEXT: ret
  %trunc = trunc <vscale x 4 x i32> %data to <vscale x 4 x i8>
  call void @llvm.aarch64.sve.st1.nxv4i8(<vscale x 4 x i8> %trunc,
                                         <vscale x 4 x i1> %pred,
                                         i8* %addr)
  ret void
}

define void @st1b_d(<vscale x 2 x i64> %data, <vscale x 2 x i1> %pred, i8* %addr) {
; CHECK-LABEL: st1b_d:
; CHECK: st1b { z0.d }, p0, [x0]
; CHECK-NEXT: ret
  %trunc = trunc <vscale x 2 x i64> %data to <vscale x 2 x i8>
  call void @llvm.aarch64.sve.st1.nxv2i8(<vscale x 2 x i8> %trunc,
                                         <vscale x 2 x i1> %pred,
                                         i8* %addr)
  ret void
}

;
; ST1H
;

define void @st1h_i16(<vscale x 8 x i16> %data, <vscale x 8 x i1> %pred, i16* %addr) {
; CHECK-LABEL: st1h_i16:
; CHECK: st1h { z0.h }, p0, [x0]
; CHECK-NEXT: ret
  call void @llvm.aarch64.sve.st1.nxv8i16(<vscale x 8 x i16> %data,
                                          <vscale x 8 x i1> %pred,
                                          i16* %addr)
  ret void
}

define void @st1h_f16(<vscale x 8 x half> %data, <vscale x 8 x i1> %pred, half* %addr) {
; CHECK-LABEL: st1h_f16:
; CHECK: st1h { z0.h }, p0, [x0]
; CHECK-NEXT: ret
  call void @llvm.aarch64.sve.st1.nxv8f16(<vscale x 8 x half> %data,
                                          <vscale x 8 x i1> %pred,
                                          half* %addr)
  ret void
}

define void @st1h_bf16(<vscale x 8 x bfloat> %data, <vscale x 8 x i1> %pred, bfloat* %addr) #0 {
; CHECK-LABEL: st1h_bf16:
; CHECK: st1h { z0.h }, p0, [x0]
; CHECK-NEXT: ret
  call void @llvm.aarch64.sve.st1.nxv8bf16(<vscale x 8 x bfloat> %data,
                                           <vscale x 8 x i1> %pred,
                                           bfloat* %addr)
  ret void
}

define void @st1h_s(<vscale x 4 x i32> %data, <vscale x 4 x i1> %pred, i16* %addr) {
; CHECK-LABEL: st1h_s:
; CHECK: st1h { z0.s }, p0, [x0]
; CHECK-NEXT: ret
  %trunc = trunc <vscale x 4 x i32> %data to <vscale x 4 x i16>
  call void @llvm.aarch64.sve.st1.nxv4i16(<vscale x 4 x i16> %trunc,
                                         <vscale x 4 x i1> %pred,
                                         i16* %addr)
  ret void
}

define void @st1h_d(<vscale x 2 x i64> %data, <vscale x 2 x i1> %pred, i16* %addr) {
; CHECK-LABEL: st1h_d:
; CHECK: st1h { z0.d }, p0, [x0]
; CHECK-NEXT: ret
  %trunc = trunc <vscale x 2 x i64> %data to <vscale x 2 x i16>
  call void @llvm.aarch64.sve.st1.nxv2i16(<vscale x 2 x i16> %trunc,
                                         <vscale x 2 x i1> %pred,
                                         i16* %addr)
  ret void
}

;
; ST1W
;

define void @st1w_i32(<vscale x 4 x i32> %data, <vscale x 4 x i1> %pred, i32* %addr) {
; CHECK-LABEL: st1w_i32:
; CHECK: st1w { z0.s }, p0, [x0]
; CHECK-NEXT: ret
  call void @llvm.aarch64.sve.st1.nxv4i32(<vscale x 4 x i32> %data,
                                          <vscale x 4 x i1> %pred,
                                          i32* %addr)
  ret void
}

define void @st1w_f32(<vscale x 4 x float> %data, <vscale x 4 x i1> %pred, float* %addr) {
; CHECK-LABEL: st1w_f32:
; CHECK: st1w { z0.s }, p0, [x0]
; CHECK-NEXT: ret
  call void @llvm.aarch64.sve.st1.nxv4f32(<vscale x 4 x float> %data,
                                          <vscale x 4 x i1> %pred,
                                          float* %addr)
  ret void
}

define void @st1w_d(<vscale x 2 x i64> %data, <vscale x 2 x i1> %pred, i32* %addr) {
; CHECK-LABEL: st1w_d:
; CHECK: st1w { z0.d }, p0, [x0]
; CHECK-NEXT: ret
  %trunc = trunc <vscale x 2 x i64> %data to <vscale x 2 x i32>
  call void @llvm.aarch64.sve.st1.nxv2i32(<vscale x 2 x i32> %trunc,
                                         <vscale x 2 x i1> %pred,
                                         i32* %addr)
  ret void
}

;
; ST1D
;

define void @st1d_i64(<vscale x 2 x i64> %data, <vscale x 2 x i1> %pred, i64* %addr) {
; CHECK-LABEL: st1d_i64:
; CHECK: st1d { z0.d }, p0, [x0]
; CHECK-NEXT: ret
  call void @llvm.aarch64.sve.st1.nxv2i64(<vscale x 2 x i64> %data,
                                          <vscale x 2 x i1> %pred,
                                          i64* %addr)
  ret void
}

define void @st1d_f64(<vscale x 2 x double> %data, <vscale x 2 x i1> %pred, double* %addr) {
; CHECK-LABEL: st1d_f64:
; CHECK: st1d { z0.d }, p0, [x0]
; CHECK-NEXT: ret
  call void @llvm.aarch64.sve.st1.nxv2f64(<vscale x 2 x double> %data,
                                          <vscale x 2 x i1> %pred,
                                          double* %addr)
  ret void
}

declare void @llvm.aarch64.sve.st1.nxv16i8(<vscale x 16 x i8>, <vscale x 16 x i1>, i8*)

declare void @llvm.aarch64.sve.st1.nxv8i8(<vscale x 8 x i8>, <vscale x 8 x i1>, i8*)
declare void @llvm.aarch64.sve.st1.nxv8i16(<vscale x 8 x i16>, <vscale x 8 x i1>, i16*)
declare void @llvm.aarch64.sve.st1.nxv8f16(<vscale x 8 x half>, <vscale x 8 x i1>, half*)
declare void @llvm.aarch64.sve.st1.nxv8bf16(<vscale x 8 x bfloat>, <vscale x 8 x i1>, bfloat*)

declare void @llvm.aarch64.sve.st1.nxv4i8(<vscale x 4 x i8>, <vscale x 4 x i1>, i8*)
declare void @llvm.aarch64.sve.st1.nxv4i16(<vscale x 4 x i16>, <vscale x 4 x i1>, i16*)
declare void @llvm.aarch64.sve.st1.nxv4i32(<vscale x 4 x i32>, <vscale x 4 x i1>, i32*)
declare void @llvm.aarch64.sve.st1.nxv4f32(<vscale x 4 x float>, <vscale x 4 x i1>, float*)

declare void @llvm.aarch64.sve.st1.nxv2i8(<vscale x 2 x i8>, <vscale x 2 x i1>, i8*)
declare void @llvm.aarch64.sve.st1.nxv2i16(<vscale x 2 x i16>, <vscale x 2 x i1>, i16*)
declare void @llvm.aarch64.sve.st1.nxv2i32(<vscale x 2 x i32>, <vscale x 2 x i1>, i32*)
declare void @llvm.aarch64.sve.st1.nxv2i64(<vscale x 2 x i64>, <vscale x 2 x i1>, i64*)
declare void @llvm.aarch64.sve.st1.nxv2f64(<vscale x 2 x double>, <vscale x 2 x i1>, double*)

; +bf16 is required for the bfloat version.
attributes #0 = { "target-features"="+bf16" }
