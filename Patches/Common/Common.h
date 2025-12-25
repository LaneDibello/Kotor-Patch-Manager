#pragma once
#include <windows.h>
#include <cstdio>
#include <cstdarg>
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