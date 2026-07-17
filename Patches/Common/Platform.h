#pragma once

// The KOTOR 1 and 2 game builds we target are all 32-bit x86, so every patch must be
// built 32-bit x86 to match. That covers the Windows builds run under Wine/Proton
// (patches built with MSVC, or cross-built with MinGW) and the native Linux KOTOR2
// (Aspyr) build, which is 32-bit x86 too. The games also ship ARM mobile ports
// (Android/iOS); those are a separate target this patch system does not handle. Fail
// loudly on anything that is not 32-bit x86 rather than emit a silently-wrong binary.
// Common.h includes this, so patches get the check for free; a standalone patch can
// include it directly.
#if defined(_M_IX86) || defined(__i386__)
// 32-bit x86: MSVC (_M_IX86), or GCC/Clang (__i386__: MinGW or native Linux). OK.
#elif defined(_WIN64) || defined(_M_X64) || defined(__x86_64__)
#error "KOTOR patches must be built as 32-bit x86, not 64-bit."
#else
// Non-x86, most likely ARM: the mobile ports, which are not supported.
#error "KOTOR patches target the 32-bit x86 builds, not the ARM mobile ports."
#endif
