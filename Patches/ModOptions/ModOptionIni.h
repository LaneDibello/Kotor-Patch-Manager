#pragma once
#include "Common.h"
#include "ModOptionsConfig.h"

#include "GameAPI/CExoIni.h"
#include "GameAPI/CExoString.h"

#include <string>

// CExoIni returns 0 on failure. CExoString takes char* and copies, so const_cast off
// a std::string is safe; it is non-copyable, hence all the in-place construction.

inline bool ReadOptionValue(const ModOption& option, std::string& outValue) {
	if (!option.HasIni()) {
		return false;
	}

	CExoIni ini;
	CExoString value;
	CExoString filename(const_cast<char*>(option.ini.c_str()));
	CExoString category(const_cast<char*>(option.category.c_str()));
	CExoString key(const_cast<char*>(option.key.c_str()));

	if (ini.ReadIniEntry(&value, &filename, &category, &key) == 0) {
		return false;
	}

	char* text = value.GetCStr();
	if (!text) {
		return false;
	}

	outValue = text;
	return true;
}

inline void WriteOptionValue(const ModOption& option, const std::string& value) {
	if (!option.HasIni()) {
		return;
	}

	CExoIni ini;
	CExoString newValue(const_cast<char*>(value.c_str()));
	CExoString filename(const_cast<char*>(option.ini.c_str()));
	CExoString category(const_cast<char*>(option.category.c_str()));
	CExoString key(const_cast<char*>(option.key.c_str()));

	if (ini.WriteIniEntry(&newValue, &filename, &category, &key) == 0) {
		debugLog("[ModOptions] failed to write [%s] %s = %s to %s",
			option.category.c_str(), option.key.c_str(), value.c_str(), option.ini.c_str());
	}
}

// Stored value, or the default seeded into the ini so there is something to edit.
inline std::string ResolveOptionValue(const ModOption& option) {
	if (!option.HasIni()) {
		return option.defaultString;
	}

	std::string stored;
	if (ReadOptionValue(option, stored)) {
		return stored;
	}

	WriteOptionValue(option, option.defaultString);
	return option.defaultString;
}
