#pragma once

#include <string>
#include <Stringapiset.h>
#include <algorithm>

std::string ws2s(const std::wstring& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0);
	char* buf = new char[len];
	WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, buf, len, 0, 0);
	std::string r(buf);
	delete[] buf;
	return r;
}

std::wstring s2ws(const std::string& str)
{
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

std::string getFilenameFromPath(const std::string& path)
{
	auto lastSlash1 = path.find_last_of('\\');
	auto lastSlash2 = path.find_last_of('/');
	auto lastSlashPos = std::max<int>(lastSlash1, lastSlash2) + 1;
	return path.substr(lastSlashPos, path.length() - lastSlashPos);
}

inline std::string getDoubleStringRounded(const double amount, const int decimalPlaces)
{
	char floatingString[32];
	sprintf_s(floatingString, 32, "%.2f", amount);
	return std::string(floatingString);
}