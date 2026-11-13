// The x86 vector intrinsics the software GS and the VU1 use, on any CPU.
//
// x86-64: the compiler's own (<immintrin.h>). arm64: SIMDe's implementation
// of the same names over NEON (simd-everywhere, MIT; its amalgamated
// x86/avx2.h in a dependency prefix as simde/x86/avx2.h, see
// ports/pc/README.md), which keeps x86's results, rounding and NaN handling
// included, so both builds draw the same picture.
#ifndef PORT_SIMD_H
#define PORT_SIMD_H

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#define PORT_TARGET_AVX2 __attribute__((target("avx2")))
#define PORT_HAVE_AVX2() __builtin_cpu_supports("avx2")
#define port_cpu_pause() __builtin_ia32_pause()
#else
#define SIMDE_ENABLE_NATIVE_ALIASES
#include <simde/x86/avx2.h>
#define PORT_TARGET_AVX2
#define PORT_HAVE_AVX2() 1
#define port_cpu_pause() __asm__ volatile("yield")
#endif

#endif
