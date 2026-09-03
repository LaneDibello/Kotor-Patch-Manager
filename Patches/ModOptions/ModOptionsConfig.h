#pragma once
#include "Common.h"

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <optional>
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
	std::string name;				// required; the label shown in the UI
	ModOptionType type = ModOptionType::Toggle;
	std::string description;

	std::string ini;
	std::string category;
	std::string key;

	std::string function;
	std::string patch;		// defaults to the [menu] `patch`

	// Slider only.
	int min = 0;
	int max = 0;

	// List only. Never empty for a List option.
	std::vector<std::string> choices;

	// The `default` field, kept in both a numeric and a string form so callers
	// can drive a control or write an ini entry without re-parsing. Meaning of
	// defaultNumber by type:
	//   Toggle -> 0 or 1
	//   Slider -> clamped into [min, max]
	//   List   -> index into `choices`
	//   Text   -> unused (always 0)
	int defaultNumber = 0;
	std::string defaultString;

	const std::string& GetName() const { return name; }
	const std::string& GetDescription() const { return description; }
	const std::string& GetPatch() const { return patch; }
	const std::string& GetFunction() const { return function; }

	// Handler's first argument; `key` is only required when there is an ini.
	const std::string& EffectiveKey() const { return key.empty() ? name : key; }

	int DefaultAsInt() const { return defaultNumber; }
	bool DefaultAsBool() const { return defaultNumber != 0; }
	size_t DefaultChoiceIndex() const { return (size_t)defaultNumber; }
	// Canonical text form, suitable for writing straight into an ini entry.
	const std::string& DefaultAsString() const { return defaultString; }

	bool HasIni() const { return !ini.empty() && !category.empty() && !key.empty(); }
	// Both halves are needed to reach the symbol.
	bool HasFunction() const { return !function.empty() && !patch.empty(); }
	bool HasDescription() const { return !description.empty(); }
};

struct ModOptionsConfig {
	// The [menu] block.
	std::string menuName;			// falls back to the file stem
	std::string menuDescription;	// may be empty
	std::string menuPatch;			// default owning patch id for this file's options

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

	// Accepts the ini-friendly spellings a `default` might use for a toggle.
	inline bool ParseBooleanText(const std::string& text, bool& outValue) {
		std::string lowered;
		lowered.reserve(text.size());
		for (char character : text) {
			lowered.push_back((char)tolower((unsigned char)character));
		}
		if (lowered == "1" || lowered == "true" || lowered == "yes" || lowered == "on") { outValue = true; return true; }
		if (lowered == "0" || lowered == "false" || lowered == "no" || lowered == "off") { outValue = false; return true; }
		return false;
	}

	// Requires the type-specific fields (min/max, choices) to already be parsed.
	// A default that is present but out of range is corrected rather than fatal;
	// a missing or wrongly typed default skips the option.
	inline bool ParseDefault(const toml::table& source, size_t index, const std::string& sourceName, ModOption& outOption) {
		const toml::node* node = source.get("default");
		if (!node) {
			debugLog("[ModOptions] %s: option %u has no `default`; skipping", sourceName.c_str(), (unsigned)index);
			return false;
		}

		switch (outOption.type) {
		case ModOptionType::Toggle: {
			bool value = false;
			if (node->is_boolean()) {
				value = *node->value<bool>();
			}
			else if (auto text = node->value<std::string>()) {
				if (!ParseBooleanText(*text, value)) {
					debugLog("[ModOptions] %s: toggle option %u has an unrecognized `default` of `%s`; skipping",
						sourceName.c_str(), (unsigned)index, text->c_str());
					return false;
				}
			}
			else if (node->is_integer()) {
				auto number = node->value<int64_t>();
				if (*number != 0 && *number != 1) {
					debugLog("[ModOptions] %s: toggle option %u has a `default` of %d, which is not 0 or 1; skipping",
						sourceName.c_str(), (unsigned)index, (int)*number);
					return false;
				}
				value = (*number != 0);
			}
			else {
				debugLog("[ModOptions] %s: toggle option %u has a `default` that is not a boolean; skipping",
					sourceName.c_str(), (unsigned)index);
				return false;
			}
			outOption.defaultNumber = value ? 1 : 0;
			outOption.defaultString = value ? "1" : "0";
			return true;
		}
		case ModOptionType::Slider: {
			std::optional<int64_t> number;
			if (node->is_integer()) {
				number = node->value<int64_t>();
			}
			else if (auto text = node->value<std::string>()) {
				number = (int64_t)atoi(text->c_str());
			}
			if (!number) {
				debugLog("[ModOptions] %s: slider option %u has a `default` that is not an integer; skipping",
					sourceName.c_str(), (unsigned)index);
				return false;
			}
			int64_t clamped = *number;
			if (clamped < outOption.min) { clamped = outOption.min; }
			if (clamped > outOption.max) { clamped = outOption.max; }
			if (clamped != *number) {
				debugLog("[ModOptions] %s: slider option %u has a `default` of %d outside [%d, %d]; clamping to %d",
					sourceName.c_str(), (unsigned)index, (int)*number, outOption.min, outOption.max, (int)clamped);
			}
			outOption.defaultNumber = (int)clamped;
			outOption.defaultString = std::to_string(outOption.defaultNumber);
			return true;
		}
		case ModOptionType::List: {
			if (auto text = node->value<std::string>()) {
				for (size_t choice = 0; choice < outOption.choices.size(); ++choice) {
					if (outOption.choices[choice] == *text) {
						outOption.defaultNumber = (int)choice;
						outOption.defaultString = *text;
						return true;
					}
				}
				debugLog("[ModOptions] %s: list option %u has a `default` of `%s`, which is not one of its choices; using `%s`",
					sourceName.c_str(), (unsigned)index, text->c_str(), outOption.choices[0].c_str());
				outOption.defaultNumber = 0;
				outOption.defaultString = outOption.choices[0];
				return true;
			}
			if (node->is_integer()) {
				auto number = node->value<int64_t>();
				int64_t clamped = *number;
				if (clamped < 0) { clamped = 0; }
				if (clamped >= (int64_t)outOption.choices.size()) { clamped = (int64_t)outOption.choices.size() - 1; }
				if (clamped != *number) {
					debugLog("[ModOptions] %s: list option %u has a `default` index of %d outside its %u choices; clamping to %d",
						sourceName.c_str(), (unsigned)index, (int)*number, (unsigned)outOption.choices.size(), (int)clamped);
				}
				outOption.defaultNumber = (int)clamped;
				outOption.defaultString = outOption.choices[(size_t)clamped];
				return true;
			}
			debugLog("[ModOptions] %s: list option %u has a `default` that is neither a choice nor an index; skipping",
				sourceName.c_str(), (unsigned)index);
			return false;
		}
		case ModOptionType::Text: {
			if (auto text = node->value<std::string>()) {
				outOption.defaultString = *text;
				return true;
			}
			if (node->is_boolean()) {
				outOption.defaultString = *node->value<bool>() ? "true" : "false";
				return true;
			}
			if (node->is_integer()) {
				outOption.defaultString = std::to_string((int)*node->value<int64_t>());
				return true;
			}
			debugLog("[ModOptions] %s: text option %u has a `default` that is not a string; skipping",
				sourceName.c_str(), (unsigned)index);
			return false;
		}
		}

		return false;
	}

	inline bool ParseOption(const toml::table& source, size_t index, const std::string& sourceName,
		const std::string& defaultPatch, ModOption& outOption) {
		outOption.name = StringOr(source, "name");
		if (outOption.name.empty()) {
			debugLog("[ModOptions] %s: option %u has no `name`; skipping", sourceName.c_str(), (unsigned)index);
			return false;
		}

		auto typeText = source["type"].value<std::string>();
		if (!typeText) {
			debugLog("[ModOptions] %s: option %u has no `type`; skipping", sourceName.c_str(), (unsigned)index);
			return false;
		}
		if (!ParseType(*typeText, outOption.type)) {
			debugLog("[ModOptions] %s: option %u has unknown type `%s`; skipping",
				sourceName.c_str(), (unsigned)index, typeText->c_str());
			return false;
		}

		outOption.description = StringOr(source, "description");
		outOption.ini = StringOr(source, "ini");
		outOption.category = StringOr(source, "category");
		outOption.key = StringOr(source, "key");
		outOption.function = StringOr(source, "function");
		outOption.patch = StringOr(source, "patch", defaultPatch);

		if (!outOption.function.empty() && outOption.patch.empty()) {
			debugLog("[ModOptions] %s: option %u names function `%s` but no `patch`",
				sourceName.c_str(), (unsigned)index, outOption.function.c_str());
		}

		// An option that neither stores a value nor calls anything does nothing.
		if (!outOption.HasIni() && !outOption.HasFunction()) {
			debugLog("[ModOptions] %s: option %u has neither a complete ini/category/key nor a resolvable function; skipping",
				sourceName.c_str(), (unsigned)index);
			return false;
		}

		switch (outOption.type) {
		case ModOptionType::Slider: {
			auto min = source["min"].value<int64_t>();
			auto max = source["max"].value<int64_t>();
			if (!min || !max) {
				debugLog("[ModOptions] %s: slider option %u is missing `min` or `max`; skipping",
					sourceName.c_str(), (unsigned)index);
				return false;
			}
			if (*min < 0) {
				debugLog("[ModOptions] %s: slider option %u has a negative `min`; skipping",
					sourceName.c_str(), (unsigned)index);
				return false;
			}
			if (*max <= *min) {
				debugLog("[ModOptions] %s: slider option %u has max <= min; skipping",
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
				debugLog("[ModOptions] %s: list option %u has no `choices` array; skipping",
					sourceName.c_str(), (unsigned)index);
				return false;
			}
			outOption.choices.reserve(choices->size());
			for (const auto& choice : *choices) {
				if (auto text = choice.value<std::string>()) {
					outOption.choices.push_back(*text);
				}
				else {
					debugLog("[ModOptions] %s: list option %u has a non-string choice; ignoring it",
						sourceName.c_str(), (unsigned)index);
				}
			}
			if (outOption.choices.empty()) {
				debugLog("[ModOptions] %s: list option %u has no usable choices; skipping",
					sourceName.c_str(), (unsigned)index);
				return false;
			}
			break;
		}
		default:
			break;
		}

		// Depends on min/max and choices, so it has to come last.
		if (!ParseDefault(source, index, sourceName, outOption)) {
			return false;
		}

		return true;
	}

	inline ModOptionsConfig FromTable(toml::table&& root, const std::string& sourceName) {
		ModOptionsConfig config;
		config.loaded = true;

		config.menuName = StringOr(root, "name");
		config.menuDescription = StringOr(root, "description");
		config.menuPatch = StringOr(root, "patch");
		if (auto menu = root["menu"].as_table()) {
			config.menuName = StringOr(*menu, "name", config.menuName);
			config.menuDescription = StringOr(*menu, "description", config.menuDescription);
			config.menuPatch = StringOr(*menu, "patch", config.menuPatch);
		}
		if (config.menuName.empty()) {
			config.menuName = FileStem(sourceName);
		}

		const toml::array* options = root["options"].as_array();
		if (!options) {
			debugLog("[ModOptions] %s: no [[options]] entries found", sourceName.c_str());
			return config;
		}

		config.options.reserve(options->size());
		for (size_t i = 0; i < options->size(); ++i) {
			const toml::table* optionTable = options->get(i)->as_table();
			if (!optionTable) {
				debugLog("[ModOptions] %s: option %u is not a table; skipping", sourceName.c_str(), (unsigned)i);
				continue;
			}
			ModOption option;
			if (ParseOption(*optionTable, i, sourceName, config.menuPatch, option)) {
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
			debugLog("[ModOptions] %s", config.error.c_str());
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
		debugLog("[ModOptions] %s", config.error.c_str());
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
		debugLog("[ModOptions] %s", config.error.c_str());
		return config;
	}
#else
	return ModOptionsConfigDetail::FromResult(toml::parse(text, source), source);
#endif
}
