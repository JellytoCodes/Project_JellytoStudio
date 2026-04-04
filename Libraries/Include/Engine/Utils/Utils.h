#pragma once

class Utils
{
public:
	static bool StartsWith(std::string str, std::string comp);
	static bool StartsWith(std::wstring str, std::wstring comp);

	static void Replace(OUT std::string& str, std::string comp, std::string rep);
	static void Replace(OUT std::wstring& str, std::wstring comp, std::wstring rep);

	static std::wstring ToWString(std::string value);
	static std::string ToString(std::wstring value);
};

