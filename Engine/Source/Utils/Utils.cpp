#include "Framework.h"
#include "Utils.h"

bool Utils::StartsWith(const std::string& str, const std::string& comp)
{
	return str.rfind(comp, 0) == 0;
}

bool Utils::StartsWith(const std::wstring& str, const std::wstring& comp)
{
	return str.rfind(comp, 0) == 0;
}

void Utils::Replace(OUT std::string& str, const std::string& comp, const std::string& rep)
{
	if (comp.empty()) return;

	size_t startPos = 0;
	while ((startPos = str.find(comp, startPos)) != std::string::npos)
	{
		str.replace(startPos, comp.length(), rep);
		startPos += rep.length();
	}
}

void Utils::Replace(OUT std::wstring& str, const std::wstring& comp, const std::wstring& rep)
{
	if (comp.empty()) return;

	size_t startPos = 0;
	while ((startPos = str.find(comp, startPos)) != std::wstring::npos)
	{
		str.replace(startPos, comp.length(), rep);
		startPos += rep.length();
	}
}

std::wstring Utils::ToWString(const std::string& value)
{
	std::wstring result;
	result.reserve(value.size());
	for (char ch : value)
		result.push_back(static_cast<unsigned char>(ch));
	return result;
}

std::string Utils::ToString(const std::wstring& value)
{
	std::string result;
	result.reserve(value.size());
	for (wchar_t ch : value)
		result.push_back(static_cast<char>(ch & 0xFF));
	return result;
}
