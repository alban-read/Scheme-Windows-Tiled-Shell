#pragma once
#include "ContainerApp.h"
class Utility
{

public:

	static std::wstring widen(const std::string& in);
	static std::wstring GetFullPathFor(LPCWSTR relativePath);
	static std::wstring s2_ws(const std::string& str);
	static std::string ws_2s(const std::wstring& wstr);
	static std::string trim(std::string s);
	static ptr UTF8toSchemeString(std::string s);
	static const char* Ss2s(ptr sparam);
	static CDockContainerApp* get_app();
	static void set_main(HWND h);
	static HWND get_main();
	static void set_browser(HWND h);
	static HWND get_browser();
};

