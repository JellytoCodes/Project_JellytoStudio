#pragma once

class Utils
{
public:
	static bool StartsWith(const std::string& str, const std::string& comp);
	static bool StartsWith(const std::wstring& str, const std::wstring& comp);

	static void Replace(OUT std::string& str, const std::string& comp, const std::string& rep);
	static void Replace(OUT std::wstring& str, const std::wstring& comp, const std::wstring& rep);

	static std::wstring ToWString(const std::string& value);
	static std::string ToString(const std::wstring& value);
};
