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

# MinGW-w64 ships two threading models. std::thread / std::this_thread (used by the
# deferred-apply path in the patcher) exist only in the posix model; the win32 model
# -- the Debian/Ubuntu default -- includes <thread> fine but defines nothing, so the
# build fails deep in compilation. Prefer the posix variant when the distro provides
# it as a separate binary (Debian/Ubuntu); fall back to the plain name where the
# default is already posix (Fedora). Override with CXX_WIN=... in the environment.
CXX_WIN   ?= $(shell command -v i686-w64-mingw32-g++-posix 2>/dev/null || echo i686-w64-mingw32-g++)
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

# Always build to these fixed, space-free in-tree paths. The final destinations
# (DLL_OUT/SO_OUT) may be absolute paths under a directory containing spaces -- the
# release tree lives under ".../KotOR Patch Manager/" -- and Make cannot use such a
# path as a target: it splits target and prerequisite lists on whitespace, so a
# spaced path silently becomes several bogus targets. Build here, then copy to the
# requested destination in the recipe below, where the shell (not Make) handles the
# spaces via quoting.
BUILD_DLL := $(BUILD)/KotorPatcher.dll
BUILD_SO  := $(BUILD)/KotorPatcher.so

# Optional final destination for each artifact. When set, the dll/so target copies
# the built binary there (the release scripts use this to drop it into the release
# tree); when empty, the binary is left at its BUILD_* path. Overridable in the env.
DLL_OUT ?=
SO_OUT  ?=

.PHONY: all dll so clean
all: dll so

# The copy is skipped when DLL_OUT/SO_OUT is empty or already points at the built
# file (-ef: same file), so `make dll DLL_OUT=<build path>` can't cp a file onto itself.
dll: $(BUILD_DLL)
	@if [ -n "$(DLL_OUT)" ] && [ ! "$(DLL_OUT)" -ef "$(BUILD_DLL)" ]; then cp -f "$(BUILD_DLL)" "$(DLL_OUT)"; fi
so: $(BUILD_SO)
	@if [ -n "$(SO_OUT)" ] && [ ! "$(SO_OUT)" -ef "$(BUILD_SO)" ]; then cp -f "$(BUILD_SO)" "$(SO_OUT)"; fi

$(BUILD_DLL): $(WIN_OBJS)
	@mkdir -p $(dir $@)
	$(CXX_WIN) $(WIN_CXXFLAGS) -o $@ $^ $(WIN_LDFLAGS)

$(BUILD_SO): $(LINUX_OBJS)
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
