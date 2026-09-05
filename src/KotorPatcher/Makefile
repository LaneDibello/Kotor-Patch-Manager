# Build the KotorPatcher engine on Linux hosts.
#
# One engine, three artifacts from shared sources:
#   KotorPatcher.dll    i686 PE, injected into the Windows game (also under Wine)
#   KotorPatcher.so     i386 ELF, loaded into the Aspyr KOTOR2 Linux build
#   KotorPatcher.dylib  x86_64 Mach-O, loaded into the Aspyr macOS builds
#
# The MSVC KotorPatcher.vcxproj stays the Windows release build; this Makefile is
# the Linux-side build (contributors, CI, the Wine/Proton DLL, and the Linux .so).
# The Windows and Linux games are 32-bit; the Aspyr macOS builds are x86_64,
# which is why the dylib is the odd one out and why Apple Silicon runs them
# under Rosetta.
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

# Building a Mach-O from Linux needs an osxcross toolchain, which supplies the
# macOS SDK and the ld64 the format requires. It is looked up on PATH and then in
# the usual install location; on a Mac, set CXX_MAC=clang++ instead. The `dylib`
# target is deliberately not part of `all`, so a machine without the toolchain
# still builds the other two.
CXX_MAC   ?= $(shell command -v o64-clang++ 2>/dev/null || \
                     ls /usr/local/lib/osxcross/bin/o64-clang++ 2>/dev/null || \
                     echo o64-clang++)

# Joins the per-architecture slices into the shipped dylib. Present as plain
# `lipo` both in osxcross and on a Mac.
LIPO      ?= $(shell command -v lipo 2>/dev/null || \
                     ls /usr/local/lib/osxcross/bin/lipo 2>/dev/null || \
                     echo lipo)

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

# macOS gets its own base rather than COMMON: -s and the -static-libgcc/-stdc++
# pair are GCC options with no meaning here, and libc++ ships with the OS. The
# deployment target is the older of the two games' minimums (KOTOR II asks for
# 10.9.5), so the engine is never the thing that raises the bar. The install name
# matches what gets written into the game's LC_LOAD_DYLIB, and is the same
# @executable_path form the game already uses for libBink2Macx64.dylib.
# -arch is added per slice below, not here.
MAC_MIN      ?= 10.9
MAC_CXXFLAGS := -std=c++17 -O2 $(INCLUDES) -mmacosx-version-min=$(MAC_MIN) -fPIC
MAC_LDFLAGS  := -dynamiclib -mmacosx-version-min=$(MAC_MIN) -Wl,-x \
                -install_name @executable_path/KotorPatcher.dylib

# Architectures in the shipped dylib. x86_64 is the only one the games need: both
# Aspyr builds are x86_64, and Apple Silicon runs them under Rosetta, which loads
# x86_64 slices. Others are a build away only once their code generation exists,
# so overriding this is not on its own enough:
#   i386   needs an SDK old enough to ship a 32-bit libc++; MacOSX14 does not,
#          and the wrapper generator is already there.
#   arm64  needs a wrapper generator AND an arm64 trampoline, since Trampoline
#          emits E9/E8 by hand. Only worth doing if Aspyr ship a native build.
MAC_ARCHS ?= x86_64

# The wrapper generator is chosen by instruction set: the x86 one is built on
# opcodes that do not exist in long mode.
MAC_WRAPPER_x86_64 := src/core/wrapper_x86_64.cpp
MAC_WRAPPER_i386   := src/core/wrapper_x86.cpp

# All three artifacts are the same engine. They differ in the entry point
# (dllmain / soentry / dylibentry), the platform backend, and the wrapper
# generator: exactly one of those is linked, chosen by the instruction set, since
# the x86 one is built on opcodes that do not exist in long mode.
CORE_SRCS := src/core/config_reader.cpp \
             src/core/patcher.cpp \
             src/core/trampoline.cpp

WIN_SRCS   := $(CORE_SRCS) src/core/wrapper_x86.cpp src/platform_win32.cpp src/dllmain.cpp
LINUX_SRCS := $(CORE_SRCS) src/core/wrapper_x86.cpp src/platform_posix.cpp src/soentry.cpp
MAC_SRCS   := $(CORE_SRCS) src/platform_posix.cpp src/dylibentry.cpp

# Windows and Linux objects are compiled from overlapping sources with different
# compilers, so they must not share an object directory.
WIN_OBJS   := $(patsubst src/%.cpp,$(BUILD)/win/%.o,$(WIN_SRCS))
LINUX_OBJS := $(patsubst src/%.cpp,$(BUILD)/linux/%.o,$(LINUX_SRCS))

# Refuse an architecture whose code generation is missing, rather than link one
# whose GetWrapperGenerator resolves to nothing.
$(foreach arch,$(MAC_ARCHS),$(if $(MAC_WRAPPER_$(arch)),,\
  $(error No wrapper generator for macOS arch '$(arch)'; set MAC_WRAPPER_$(arch))))

# One object directory and one thin dylib per slice, since the slices are
# compiled from overlapping sources for different instruction sets.
define MAC_SLICE_RULES
MAC_OBJS_$(1) := $$(patsubst src/%.cpp,$$(BUILD)/mac/$(1)/%.o,$$(MAC_SRCS) $$(MAC_WRAPPER_$(1)))
MAC_THIN_$(1) := $$(BUILD)/mac/KotorPatcher-$(1).dylib

$$(BUILD)/mac/$(1)/%.o: src/%.cpp
	@mkdir -p $$(dir $$@)
	$$(CXX_MAC) $$(MAC_CXXFLAGS) -arch $(1) -c $$< -o $$@

$$(MAC_THIN_$(1)): $$(MAC_OBJS_$(1))
	@mkdir -p $$(dir $$@)
	$$(CXX_MAC) $$(MAC_CXXFLAGS) -arch $(1) -o $$@ $$^ $$(MAC_LDFLAGS) -arch $(1)
endef
$(foreach arch,$(MAC_ARCHS),$(eval $(call MAC_SLICE_RULES,$(arch))))

MAC_THIN := $(foreach arch,$(MAC_ARCHS),$(MAC_THIN_$(arch)))

# Always build to these fixed, space-free in-tree paths. The final destinations
# (DLL_OUT/SO_OUT) may be absolute paths under a directory containing spaces -- the
# release tree lives under ".../KotOR Patch Manager/" -- and Make cannot use such a
# path as a target: it splits target and prerequisite lists on whitespace, so a
# spaced path silently becomes several bogus targets. Build here, then copy to the
# requested destination in the recipe below, where the shell (not Make) handles the
# spaces via quoting.
BUILD_DLL   := $(BUILD)/KotorPatcher.dll
BUILD_SO    := $(BUILD)/KotorPatcher.so
BUILD_DYLIB := $(BUILD)/KotorPatcher.dylib

# Optional final destination for each artifact. When set, the dll/so target copies
# the built binary there (the release scripts use this to drop it into the release
# tree); when empty, the binary is left at its BUILD_* path. Overridable in the env.
DLL_OUT   ?=
SO_OUT    ?=
DYLIB_OUT ?=

.PHONY: all dll so dylib test clean
all: dll so

# Host tests for the code generators. Each assembles a hook site, runs a stub the
# generator produced, and checks what came back, so they need an x86 host with
# 32-bit support. The emitted bytes do not depend on the OS, so running them here
# covers the Windows and macOS builds too.
#
# tests/any_* are built at both widths; tests/t32_* and tests/t64_* pick the
# wrapper generator that matches their instruction set.
TEST_ENGINE := src/core/trampoline.cpp src/platform_posix.cpp
TESTS_ANY   := $(wildcard tests/any_*.cpp)
TESTS_32    := $(wildcard tests/t32_*.cpp) $(TESTS_ANY)
TESTS_64    := $(wildcard tests/t64_*.cpp) $(TESTS_ANY)
TEST_FLAGS  := -std=c++17 -O1 $(INCLUDES) -Itests -Isrc

# The engine logs to stderr as it generates, which drowns the results, so each
# run is captured and only the non-log lines are shown. The status has to come
# from the test itself: piping it into a filter would report the filter's.
test:
	@mkdir -p $(BUILD)/tests
	@rc=0; \
	run() { \
	  if "$$1" > "$$1.log" 2>&1; then :; else rc=1; fi; \
	  grep -vE '^\[' "$$1.log" || true; \
	}; \
	for t in $(TESTS_32); do \
	  n=$$(basename $$t .cpp); echo "  [i386]   $$n"; \
	  $(CXX_LINUX) $(TEST_FLAGS) -m32 -msse2 -o $(BUILD)/tests/$$n-32 $$t \
	      src/core/wrapper_x86.cpp $(TEST_ENGINE) -ldl || { rc=1; continue; }; \
	  run $(BUILD)/tests/$$n-32; \
	done; \
	for t in $(TESTS_64); do \
	  n=$$(basename $$t .cpp); echo "  [x86_64] $$n"; \
	  $(CXX_LINUX) $(TEST_FLAGS) -o $(BUILD)/tests/$$n-64 $$t \
	      src/core/wrapper_x86_64.cpp $(TEST_ENGINE) -ldl || { rc=1; continue; }; \
	  run $(BUILD)/tests/$$n-64; \
	done; \
	exit $$rc

# The copy is skipped when DLL_OUT/SO_OUT is empty or already points at the built
# file (-ef: same file), so `make dll DLL_OUT=<build path>` can't cp a file onto itself.
dll: $(BUILD_DLL)
	@if [ -n "$(DLL_OUT)" ] && [ ! "$(DLL_OUT)" -ef "$(BUILD_DLL)" ]; then cp -f "$(BUILD_DLL)" "$(DLL_OUT)"; fi
so: $(BUILD_SO)
	@if [ -n "$(SO_OUT)" ] && [ ! "$(SO_OUT)" -ef "$(BUILD_SO)" ]; then cp -f "$(BUILD_SO)" "$(SO_OUT)"; fi
dylib: $(BUILD_DYLIB)
	@if [ -n "$(DYLIB_OUT)" ] && [ ! "$(DYLIB_OUT)" -ef "$(BUILD_DYLIB)" ]; then cp -f "$(BUILD_DYLIB)" "$(DYLIB_OUT)"; fi

$(BUILD_DLL): $(WIN_OBJS)
	@mkdir -p $(dir $@)
	$(CXX_WIN) $(WIN_CXXFLAGS) -o $@ $^ $(WIN_LDFLAGS)

$(BUILD_SO): $(LINUX_OBJS)
	@mkdir -p $(dir $@)
	$(CXX_LINUX) $(LINUX_CXXFLAGS) -o $@ $^ $(LINUX_LDFLAGS)

# lipo on a single input still writes a fat container around it, so a one-slice
# build is copied instead, leaving the artifact exactly as a plain link produced.
$(BUILD_DYLIB): $(MAC_THIN)
	@mkdir -p $(dir $@)
	$(if $(filter 1,$(words $(MAC_THIN))),cp -f $^ $@,$(LIPO) -create $^ -output $@)

$(BUILD)/win/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_WIN) $(WIN_CXXFLAGS) -c $< -o $@

$(BUILD)/linux/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_LINUX) $(LINUX_CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD)
