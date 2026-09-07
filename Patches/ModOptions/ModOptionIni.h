#pragma once
#include "Common.h"
#include "ModOptionsConfig.h"

#include "GameAPI/CExoIni.h"
#include "GameAPI/CExoString.h"

#include <string>

// CExoIni returns 0 on failure. CExoString takes char* and copies, so const_cast off
// a std::string is safe; it is non-copyable, hence all the in-place construction.

// ToStdString builds from CExoString's length field, and CExoIni reports two bytes more
// than the text -- a stored "1" arrives as 31 00 00. Cut at the first NUL, then strip
// anything non-printable, so padding and line endings both go.
inline std::string TrimIniValue(const std::string& value) {
	std::string text = value.substr(0, value.find('\0'));

	size_t first = 0;
	while (first < text.size() && (unsigned char)text[first] <= ' ') {
		++first;
	}
	size_t last = text.size();
	while (last > first && (unsigned char)text[last - 1] <= ' ') {
		--last;
	}
	return text.substr(first, last - first);
}

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

	outValue = TrimIniValue(value.ToStdString());
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
