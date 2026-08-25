#pragma once
#include "Common.h"

#include <string>
#include <vector>

#ifndef TOML_EXCEPTIONS
#define TOML_EXCEPTIONS 0
#endif
#ifndef TOML_ENABLE_FORMATTERS
#define TOML_ENABLE_FORMATTERS 0
#endif
#include "External/toml.hpp"

enum class ModOptionType {
	Toggle,	// 0 or 1
	Slider,	// integer in [min, max], min >= 0
	List,	// one of `choices`, by value
	Text,	// free-form string
};

struct ModOption {
	ModOptionType type = ModOptionType::Toggle;
	std::string description;

	std::string ini;
	std::string category;
	std::string key;
	std::string function;

	// Slider only.
	int min = 0;
	int max = 0;

	// List only. Never empty for a List option.
	std::vector<std::string> choices;

	bool HasIni() const { return !ini.empty() && !category.empty() && !key.empty(); }
	bool HasFunction() const { return !function.empty(); }
	bool HasDescription() const { return !description.empty(); }
};

struct ModOptionsConfig {
	// The [menu] block.
	std::string menuName;			// falls back to the file stem
	std::string menuDescription;	// may be empty

	std::string sourcePath;			// empty for configs parsed from memory

	std::vector<ModOption> options;

	// False only when the file could not be read or parsed at all. A malformed
	// [[options]] entry is skipped, not fatal.
	bool loaded = false;
	std::string error;

	const std::string& GetName() const { return menuName; }
	const std::string& GetDescription() const { return menuDescription; }
	const std::string& GetSourcePath() const { return sourcePath; }
	const std::vector<ModOption>& GetOptions() const { return options; }
	size_t OptionCount() const { return options.size(); }
	const ModOption* GetOption(size_t index) const {
		return (index < options.size()) ? &options[index] : nullptr;
	}

	static ModOptionsConfig LoadFromFile(const std::string& path);
	static ModOptionsConfig ParseText(const std::string& text, const std::string& sourceName = std::string());
};

namespace ModOptionsConfigDetail {

	inline std::string FileStem(const std::string& path) {
		size_t start = path.find_last_of("/\\");
		start = (start == std::string::npos) ? 0 : start + 1;
		size_t dot = path.find_last_of('.');
		size_t end = (dot == std::string::npos || dot < start) ? path.size() : dot;
		return path.substr(start, end - start);
	}

	inline bool ParseType(const std::string& text, ModOptionType& outType) {
		if (text == "toggle") { outType = ModOptionType::Toggle; return true; }
		if (text == "slider") { outType = ModOptionType::Slider; return true; }
		if (text == "list") { outType = ModOptionType::List; return true; }
		if (text == "text") { outType = ModOptionType::Text; return true; }
		return false;
	}

	inline std::string StringOr(const toml::table& source, const char* key, const std::string& fallback = std::string()) {
		if (auto value = source[key].value<std::string>()) {
			return *value;
		}
		return fallback;
	}

	inline bool ParseOption(const toml::table& source, size_t index, const std::string& sourceName, ModOption& outOption) {
		auto typeText = source["type"].value<std::string>();
		if (!typeText) {
			debugLog("[ModOptions] %s: option %u has no `type`; skipping\n", sourceName.c_str(), (unsigned)index);
			return false;
		}
		if (!ParseType(*typeText, outOption.type)) {
			debugLog("[ModOptions] %s: option %u has unknown type `%s`; skipping\n",
				sourceName.c_str(), (unsigned)index, typeText->c_str());
			return false;
		}

		outOption.description = StringOr(source, "description");
		outOption.ini = StringOr(source, "ini");
		outOption.category = StringOr(source, "category");
		outOption.key = StringOr(source, "key");
		outOption.function = StringOr(source, "function");

		// An option that neither stores a value nor calls anything does nothing.
		if (!outOption.HasIni() && !outOption.HasFunction()) {
			debugLog("[ModOptions] %s: option %u has neither a complete ini/category/key nor a function; skipping\n",
				sourceName.c_str(), (unsigned)index);
			return false;
		}

		switch (outOption.type) {
		case ModOptionType::Slider: {
			auto min = source["min"].value<int64_t>();
			auto max = source["max"].value<int64_t>();
			if (!min || !max) {
				debugLog("[ModOptions] %s: slider option %u is missing `min` or `max`; skipping\n",
					sourceName.c_str(), (unsigned)index);
				return false;
			}
			if (*min < 0) {
				debugLog("[ModOptions] %s: slider option %u has a negative `min`; skipping\n",
					sourceName.c_str(), (unsigned)index);
				return false;
			}
			if (*max <= *min) {
				debugLog("[ModOptions] %s: slider option %u has max <= min; skipping\n",
					sourceName.c_str(), (unsigned)index);
				return false;
			}
			outOption.min = (int)*min;
			outOption.max = (int)*max;
			break;
		}
		case ModOptionType::List: {
			const toml::array* choices = source["choices"].as_array();
			if (!choices || choices->empty()) {
				debugLog("[ModOptions] %s: list option %u has no `choices` array; skipping\n",
					sourceName.c_str(), (unsigned)index);
				return false;
			}
			outOption.choices.reserve(choices->size());
			for (const auto& choice : *choices) {
				if (auto text = choice.value<std::string>()) {
					outOption.choices.push_back(*text);
				}
				else {
					debugLog("[ModOptions] %s: list option %u has a non-string choice; ignoring it\n",
						sourceName.c_str(), (unsigned)index);
				}
			}
			if (outOption.choices.empty()) {
				debugLog("[ModOptions] %s: list option %u has no usable choices; skipping\n",
					sourceName.c_str(), (unsigned)index);
				return false;
			}
			break;
		}
		default:
			break;
		}

		return true;
	}

	inline ModOptionsConfig FromTable(toml::table&& root, const std::string& sourceName) {
		ModOptionsConfig config;
		config.loaded = true;

		config.menuName = StringOr(root, "name");
		config.menuDescription = StringOr(root, "description");
		if (auto menu = root["menu"].as_table()) {
			config.menuName = StringOr(*menu, "name", config.menuName);
			config.menuDescription = StringOr(*menu, "description", config.menuDescription);
		}
		if (config.menuName.empty()) {
			config.menuName = FileStem(sourceName);
		}

		const toml::array* options = root["options"].as_array();
		if (!options) {
			debugLog("[ModOptions] %s: no [[options]] entries found\n", sourceName.c_str());
			return config;
		}

		config.options.reserve(options->size());
		for (size_t i = 0; i < options->size(); ++i) {
			const toml::table* optionTable = options->get(i)->as_table();
			if (!optionTable) {
				debugLog("[ModOptions] %s: option %u is not a table; skipping\n", sourceName.c_str(), (unsigned)i);
				continue;
			}
			ModOption option;
			if (ParseOption(*optionTable, i, sourceName, option)) {
				config.options.push_back(std::move(option));
			}
		}

		return config;
	}

	inline ModOptionsConfig FromResult(toml::parse_result&& result, const std::string& sourceName) {
#if TOML_EXCEPTIONS
		return FromTable(std::move(result), sourceName);
#else
		if (!result) {
			ModOptionsConfig config;
			config.error = sourceName + ": TOML parse error: " + std::string(result.error().description());
			debugLog("[ModOptions] %s\n", config.error.c_str());
			return config;
		}
		return FromTable(std::move(result).table(), sourceName);
#endif
	}

}

inline ModOptionsConfig ModOptionsConfig::LoadFromFile(const std::string& path) {
#if TOML_EXCEPTIONS
	ModOptionsConfig config;
	try {
		config = ModOptionsConfigDetail::FromResult(toml::parse_file(path), path);
	}
	catch (const toml::parse_error& parseError) {
		config.error = path + ": TOML parse error: " + std::string(parseError.description());
		debugLog("[ModOptions] %s\n", config.error.c_str());
	}
#else
	ModOptionsConfig config = ModOptionsConfigDetail::FromResult(toml::parse_file(path), path);
#endif
	config.sourcePath = path;
	return config;
}

inline ModOptionsConfig ModOptionsConfig::ParseText(const std::string& text, const std::string& sourceName) {
	const std::string source = sourceName.empty() ? std::string("<memory>") : sourceName;
#if TOML_EXCEPTIONS
	try {
		return ModOptionsConfigDetail::FromResult(toml::parse(text, source), source);
	}
	catch (const toml::parse_error& parseError) {
		ModOptionsConfig config;
		config.error = source + ": TOML parse error: " + std::string(parseError.description());
		debugLog("[ModOptions] %s\n", config.error.c_str());
		return config;
	}
#else
	return ModOptionsConfigDetail::FromResult(toml::parse(text, source), source);
#endif
}
