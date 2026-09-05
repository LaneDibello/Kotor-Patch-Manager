#pragma once
#include "Common.h"
#include "ModOptionsConfig.h"

#include <map>
#include <string>

// Function type for Mod Options handlers.
typedef void(__cdecl* ModOptionHandlerFn)(const char* key, const char* value);

// The installer extracts each patch as <game>\patches\<manifest id>.dll, so the
// module name is unique per patch. GetModuleHandleA rather than LoadLibraryA: the
// patch is either already loaded or not installed.
namespace ModOptionFunctions {

	// Keyed "<patch>:<function>". Nulls are cached too, so a bad config logs once.
	inline std::map<std::string, ModOptionHandlerFn>& Cache() {
		static std::map<std::string, ModOptionHandlerFn> cache;
		return cache;
	}

	inline ModOptionHandlerFn Resolve(const std::string& patchId, const std::string& functionName) {
		if (patchId.empty() || functionName.empty()) {
			return nullptr;
		}

		const std::string cacheKey = patchId + ":" + functionName;
		auto cached = Cache().find(cacheKey);
		if (cached != Cache().end()) {
			return cached->second;
		}

		ModOptionHandlerFn handler = nullptr;
		const std::string moduleName = patchId + ".dll";

		HMODULE module = GetModuleHandleA(moduleName.c_str());
		if (!module) {
			debugLog("[ModOptions] `%s` is not loaded, cannot resolve `%s`",
				moduleName.c_str(), functionName.c_str());
		}
		else {
			handler = reinterpret_cast<ModOptionHandlerFn>(GetProcAddress(module, functionName.c_str()));
			if (!handler) {
				debugLog("[ModOptions] `%s` does not export `%s`",
					moduleName.c_str(), functionName.c_str());
			}
		}

		Cache()[cacheKey] = handler;
		return handler;
	}

}

inline bool InvokeModOptionHandler(const ModOption& option, const std::string& value) {
	if (!option.HasFunction()) {
		return false;
	}

	ModOptionHandlerFn handler = ModOptionFunctions::Resolve(option.patch, option.function);
	if (!handler) {
		return false;
	}

	handler(option.EffectiveKey().c_str(), value.c_str());
	return true;
}
