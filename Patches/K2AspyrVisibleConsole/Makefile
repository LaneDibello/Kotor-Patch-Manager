# The native Linux module, 32-bit to match the game.
#
# Nothing links libGL. The GL entry points this calls are ones the game imports itself,
# so leaving them undefined lets them bind to the libGL already in the process.
CXX ?= g++
CXXFLAGS := -m32 -O2 -fPIC -shared -fno-exceptions -fno-rtti -Wall -Wextra
OUT ?= binaries/linux_x86.so

$(OUT): VisibleConsole.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(OUT)

.PHONY: clean
