# Build the KotorPatcher engine on Linux hosts.
#
# One engine, two artifacts from shared sources:
#   KotorPatcher.dll  i686 PE, injected into the Windows game (also under Wine)
#   KotorPatcher.so   i386 ELF, loaded into the Aspyr KOTOR2 Linux build
#
# The MSVC KotorPatcher.vcxproj stays the Windows release build; this Makefile is
# the Linux-side build (contributors, CI, the Wine/Proton DLL, and the Linux .so).
# KOTOR is 32-bit, so both artifacts are i686/i386.
#
# sqlite is not linked: the runtime calls no sqlite symbol (only the per-patch
# DLLs use it, through their own GameAPI build).

CXX_WIN   := i686-w64-mingw32-g++
CXX_LINUX := g++

BUILD    := build
INCLUDES := -Iinclude -Iinclude/wrappers -Iexternal

# -s strips symbols: the static libstdc++ + toml++ template instantiations
# otherwise balloon each binary past 12 MB; stripped they are ~1.4 MB.
COMMON := -std=c++17 -O2 -s -static-libgcc -static-libstdc++ $(INCLUDES)

WIN_CXXFLAGS := $(COMMON) -DWIN32 -DNDEBUG -DKOTORPATCHER_EXPORTS -D_WINDOWS -D_USRDLL -DUNICODE -D_UNICODE
WIN_LDFLAGS  := -shared -static -lkernel32

# -pthread for the deferred-apply std::thread; -ldl for dlopen/dladdr.
LINUX_CXXFLAGS := $(COMMON) -m32 -fPIC -pthread
LINUX_LDFLAGS  := -m32 -shared -pthread -ldl

# The Windows DLL and the Linux .so are the same engine; only the entry point
# (dllmain / soentry) and the platform backend differ.
CORE_SRCS := src/core/config_reader.cpp \
             src/core/patcher.cpp \
             src/core/trampoline.cpp \
             src/core/wrapper_x86.cpp

WIN_SRCS   := $(CORE_SRCS) src/platform_win32.cpp src/dllmain.cpp
LINUX_SRCS := $(CORE_SRCS) src/platform_posix.cpp src/soentry.cpp

# Windows and Linux objects are compiled from overlapping sources with different
# compilers, so they must not share an object directory.
WIN_OBJS   := $(patsubst src/%.cpp,$(BUILD)/win/%.o,$(WIN_SRCS))
LINUX_OBJS := $(patsubst src/%.cpp,$(BUILD)/linux/%.o,$(LINUX_SRCS))

# Overridable so the release scripts can link straight to an absolute output path.
DLL_OUT ?= $(BUILD)/KotorPatcher.dll
SO_OUT  ?= $(BUILD)/KotorPatcher.so

.PHONY: all dll so clean
all: dll so
dll: $(DLL_OUT)
so:  $(SO_OUT)

$(DLL_OUT): $(WIN_OBJS)
	@mkdir -p $(dir $@)
	$(CXX_WIN) $(WIN_CXXFLAGS) -o $@ $^ $(WIN_LDFLAGS)

$(SO_OUT): $(LINUX_OBJS)
	@mkdir -p $(dir $@)
	$(CXX_LINUX) $(LINUX_CXXFLAGS) -o $@ $^ $(LINUX_LDFLAGS)

$(BUILD)/win/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_WIN) $(WIN_CXXFLAGS) -c $< -o $@

$(BUILD)/linux/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_LINUX) $(LINUX_CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD)
