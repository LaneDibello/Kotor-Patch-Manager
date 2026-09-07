#pragma once
#include <cctype>
#include <filesystem>
#include <string>

// Case-insensitive filesystem lookup. Windows matches regardless of casing, so this
// only does real work on the platforms whose filesystems care.
namespace PathCasing {

	// ASCII only, which is all the game's own resource names use.
	inline bool Equals(const std::string& a, const std::string& b) {
		if (a.size() != b.size()) {
			return false;
		}
		for (size_t i = 0; i < a.size(); ++i) {
			if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i])) {
				return false;
			}
		}
		return true;
	}

	inline bool HasExtension(const std::filesystem::path& path, const char* extension) {
		return Equals(path.extension().string(), extension);
	}

	// The directory named `name` under `parent`, however it is spelled on disk. An
	// exact match wins; otherwise the first case-insensitive one.
	inline bool FindDirectory(const std::filesystem::path& parent, const std::string& name,
	                          std::filesystem::path& outPath) {
		std::error_code ec;

		std::filesystem::path exact = parent / name;
		if (std::filesystem::is_directory(exact, ec)) {
			outPath = exact;
			return true;
		}

		std::filesystem::directory_iterator entries(parent, ec);
		if (ec) {
			return false;
		}

		for (const auto& entry : entries) {
			if (entry.is_directory(ec) && Equals(entry.path().filename().string(), name)) {
				outPath = entry.path();
				return true;
			}
		}
		return false;
	}

}
