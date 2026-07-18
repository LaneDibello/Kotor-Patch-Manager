#pragma once
#include "Platform.h"
#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include <type_traits>
#include "VirtualFunctionCall.h"
#pragma pack(push, 4)

extern DWORD OBJECT_DEFAULT;

struct Vector {
	float x;
	float y;
	float z;
};

struct Quaternion {
	float w;
	float x;
	float y;
	float z;
};

struct CScriptLocation {
	Vector postion;
	Vector orientation;
};

enum VirtualMachineEngineStructureTypes : int {
	UNKNOWN = -1,
	EFFECT = 0,
	SCRIPT_EVENT = 1,
	LOCATION = 2,
	SCRIPT_TALENT = 3,
};
#pragma pack(pop)

inline void debugLog(const char* format, ...) {
	char buffer[512];
	va_list args;
	va_start(args, format);
	vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
	va_end(args);
	OutputDebugStringA(buffer);
}

template <class retType>
inline retType getObjectProperty(void* object, int offset) {
	return *((retType*)((char*)object + offset));
}

template <class propType>
inline void setObjectProperty(void* object, int offset, propType value) {
	*((propType*)((char*)object + offset)) = value;
}

// ===== MEMBER FUNCTION -> RAW CODE ADDRESS =====
//
// The game stores GUI callbacks ("menuFunc") as a plain code address and invokes
// them as __thiscall: the registered guiObject in ECX, one parameter on the stack.
// A non-virtual C++ member function has exactly that ABI on both MSVC and MinGW (both
// pass 'this' in ECX and callee-clean the stack), so a real member can serve as a
// menuFunc directly. Register the owning object as the guiObject and its 'this' lands
// in ECX.
//
// The language forbids casting a pointer-to-member to void*, so this helper punches
// through that. The extraction differs by compiler because the pointer-to-member
// representation does (see the two branches below): MSVC packs a non-virtual member
// into a 4-byte code address, MinGW into an 8-byte Itanium { code_ptr, this_adjust }
// pair. The MSVC static_assert rejects any member that is not a plain address (a
// virtual member, or multiple/virtual inheritance) so a bad pointer fails at compile
// time instead of being silently handed to the game. That same precondition is what
// makes MinGW's first-word read the real code address, so the two paths cannot
// disagree: anything that compiles on MSVC extracts correctly under MinGW.
//
// USAGE:
//   button.AddEvent(0x27, this, memberFuncAddr(&MyPanel::OnClick));
template<typename MemFn>
inline void* memberFuncAddr(MemFn fn) {
#if defined(_MSC_VER)
	// MSVC: the member pointer is already a 4-byte code address; alias it out.
	static_assert(sizeof(MemFn) == sizeof(void*),
		"Member function pointer is not a plain code address "
		"(virtual member, or multiple/virtual inheritance?)");
	union { MemFn m; void* p; } u;
	u.m = fn;
	return u.p;
#else
	// MinGW (Itanium ABI): read the code_ptr (first word) out of the 8-byte pair.
	void* p;
	__builtin_memcpy(&p, &fn, sizeof(p));
	return p;
#endif
}

// ===== FREE FUNCTION -> RAW CODE ADDRESS =====
//
// The same idea as memberFuncAddr, but for a plain (non-member) callback. MSVC
// allows the implicit function-pointer-to-void* conversion; GCC requires the
// explicit cast. Works for any calling convention (__cdecl, __fastcall, ...).
//
// USAGE:
//   button.AddEvent(0x27, this, funcAddr(&SomeFreeCallback));
template<typename Fn>
inline void* funcAddr(Fn fn) {
	static_assert(std::is_function<std::remove_pointer_t<Fn>>::value,
		"funcAddr expects a function pointer; use memberFuncAddr for members.");
	return reinterpret_cast<void*>(fn);
}

// ===== X87 FPU FUNCTION CALL WRAPPER =====
//
// KotOR uses the x87 FPU which returns floating-point values in the ST(0) register
// rather than in EAX like integer returns. This wrapper handles that calling convention.
//
// USAGE: Use this wrapper when calling game functions that have a return type of `float10`
//        in their typedef. The float10 type indicates the function returns via x87 FPU.
//
// EXAMPLE:
//   typedef float10(__thiscall* SomeFn)(void* thisPtr, int param);
//   SomeFn gameFunction = ...;
//   float result = CallFPUFunction(gameFunction, objectPtr, 42);
//
// NOTE: This wrapper is specifically for __thiscall convention functions (thisPtr in ECX).

// Type alias for functions returning via x87 FPU ST(0)
typedef float float10;

#ifndef __GNUC__

// Template wrapper for calling x87 FPU functions with no additional parameters
template<typename FuncPtr>
inline float CallFPUFunction(FuncPtr funcPtr, void* thisPtr) {
	float result;
	__asm {
		mov ecx, thisPtr
		call funcPtr
		fstp dword ptr [result]  // Pop ST(0) to result
	}
	return result;
}

// Template wrapper for calling x87 FPU functions with 1 parameter
template<typename FuncPtr, typename Arg1>
inline float CallFPUFunction(FuncPtr funcPtr, void* thisPtr, Arg1 arg1) {
	float result;
	__asm {
		push arg1
		mov ecx, thisPtr
		call funcPtr
		fstp dword ptr [result]
	}
	return result;
}

// Template wrapper for calling x87 FPU functions with 2 parameters
template<typename FuncPtr, typename Arg1, typename Arg2>
inline float CallFPUFunction(FuncPtr funcPtr, void* thisPtr, Arg1 arg1, Arg2 arg2) {
	float result;
	__asm {
		push arg2
		push arg1
		mov ecx, thisPtr
		call funcPtr
		fstp dword ptr [result]
	}
	return result;
}

// Template wrapper for calling x87 FPU functions with 3 parameters
template<typename FuncPtr, typename Arg1, typename Arg2, typename Arg3>
inline float CallFPUFunction(FuncPtr funcPtr, void* thisPtr, Arg1 arg1, Arg2 arg2, Arg3 arg3) {
	float result;
	__asm {
		push arg3
		push arg2
		push arg1
		mov ecx, thisPtr
		call funcPtr
		fstp dword ptr [result]
	}
	return result;
}

// Template wrapper for calling x87 FPU functions with 4 parameters
template<typename FuncPtr, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
inline float CallFPUFunction(FuncPtr funcPtr, void* thisPtr, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) {
	float result;
	__asm {
		push arg4
		push arg3
		push arg2
		push arg1
		mov ecx, thisPtr
		call funcPtr
		fstp dword ptr [result]
	}
	return result;
}

#else

// GCC/MinGW cannot parse MSVC's Intel __asm blocks. It does not need them: on
// i686 a function that returns `float` already returns it in ST(0), which is
// exactly what the MSVC thunks above read out by hand with `fstp dword ptr`. So
// a correctly-typed __thiscall call is ABI-identical, and one variadic template
// covers every arity the overloads above did.
template<typename FuncPtr, typename... Args>
inline float CallFPUFunction(FuncPtr funcPtr, void* thisPtr, Args... args) {
	auto fn = reinterpret_cast<float(__thiscall*)(void*, Args...)>(funcPtr);
	return fn(thisPtr, args...);
}

#endif

typedef enum ResourceType {
    NONE = -1,
    RES = 0,
    BMP = 1,
    MVE = 2,
    TGA = 3,
    WAV = 4,
    PLT = 6,
    INI = 7,
    MP3 = 8,
    MPG = 9,
    TXT = 10,
    WMA = 11,
    WMV = 12,
    XMV = 13,
    LOG = 14,
    PLH = 2000,
    TEX = 2001,
    MDL = 2002,
    THG = 2003,
    FNT = 2005,
    LUA = 2007,
    SLT = 2008,
    NSS = 2009,
    NCS = 2010,
    MOD = 2011,
    ARE = 2012,
    SET = 2013,
    IFO = 2014,
    BIC = 2015,
    WOK = 2016,
    TwoDA = 2017,
    TLK = 2018,
    TXI = 2022,
    GIT = 2023,
    BTI = 2024,
    UTI = 2025,
    BTC = 2026,
    UTC = 2027,
    DLG = 2029,
    ITP = 2030,
    BTT = 2031,
    UTT = 2032,
    DDS = 2033,
    BTS = 2034,
    UTS = 2035,
    LTR = 2036,
    GFF = 2037,
    FAC = 2038,
    BTE = 2039,
    UTE = 2040,
    BTD = 2041,
    UTD = 2042,
    BTP = 2043,
    UTP = 2044,
    DFT = 2045,
    GIC = 2046,
    GUI = 2047,
    CSS = 2048,
    CCS = 2049,
    BTM = 2050,
    UTM = 2051,
    DWK = 2052,
    PWK = 2053,
    JRL = 2056,
    SAV = 2057,
    UTW = 2058,
    FourPC = 2059,
    SSF = 2060,
    HAK = 2061,
    NWM = 2062,
    BIK = 2063,
    NDB = 2064,
    PTM = 2065,
    PTT = 2066,
    LYT = 3000,
    VIS = 3001,
    RIM = 3002,
    PTH = 3003,
    LIP = 3004,
    BWM = 3005,
    TXB = 3006,
    TPC = 3007,
    MDX = 3008,
    RSV = 3009,
    SIG = 3010,
    XBX = 3011,
    ERF = 9997,
    BIF = 9998,
    KEY = 9999
} ResourceType;