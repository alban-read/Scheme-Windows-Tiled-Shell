#include "stdafx.h"
#include "Utility.h"
#include <codecvt>
#include "ContainerApp.h"


int utf8_string_length(const char* s)
{
	auto i = 0, j = 0;
	while (s[i])
	{
		if ((s[i] & 0xc0) != 0x80)
			j++;
		i++;
	}
	return j;
}

static const unsigned char total_bytes[256] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6
};

static const char trailing_bytes_for_utf8[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
};


ptr constUTF8toSstring(const char* s)
{
	// With UTF8 we just have sequences of bytes in a buffer.
	// in scheme we use a single longer integer for a code point.
	// see https://github.com/Komodo/KomodoEdit/blob/master/src/SciMoz/nsSciMoz.cxx
	// passes the greek test.

	unsigned int byte2;

	if (s == nullptr)
	{
		return Sstring("");
	}
	const auto ll = utf8_string_length(s);
	if (ll == 0)
	{
		return Sstring("");
	}

	const char* cptr = s;
	unsigned int byte = static_cast<unsigned char>(*cptr++);

	ptr ss = Sstring_of_length("", ll);

	auto i = 0;

	while (byte != 0 && i < ll)
	{
		// ascii
		if (byte < 0x80)
		{
			Sstring_set(ss, i++, byte);
			byte = static_cast<unsigned char>(*cptr++);
			continue;
		}

		if (byte < 0xC0)
		{
			Sstring_set(ss, i++, byte);
			byte = static_cast<unsigned char>(*cptr++);
			continue;
		}

		// skip
		while ((byte < 0xC0) && (byte >= 0x80))
		{
			byte = static_cast<unsigned char>(*cptr++);
		}

		if (byte < 0xE0)
		{
			byte2 = static_cast<unsigned char>(*cptr++);

			if ((byte2 & 0xC0) == 0x80)
			{
				byte = (((byte & 0x1F) << 6) | (byte2 & 0x3F));
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
			}
			continue;
		}

		if (byte < 0xF0)
		{
			byte2 = static_cast<unsigned char>(*cptr++);
			const unsigned int byte3 = static_cast<unsigned char>(*cptr++);
			if (((byte2 & 0xC0) == 0x80) && ((byte3 & 0xC0) == 0x80))
			{
				byte = (((byte & 0x0F) << 12) | ((byte2 & 0x3F) << 6) | (byte3 & 0x3F));
				Sstring_set(ss, i++, byte);
				byte = static_cast<unsigned char>(*cptr++);
			}
			continue;
		}

		auto trail = total_bytes[byte] - 1; // expected number of trail bytes
		if (trail > 0)
		{
			int ch = byte & (0x3F >> trail);
			do
			{
				byte2 = static_cast<unsigned char>(*cptr++);
				if ((byte2 & 0xC0) != 0x80)
				{
					Sstring_set(ss, i++, byte);
					byte = static_cast<unsigned char>(*cptr++);
					continue;
				}
				ch <<= 6;
				ch |= (byte2 & 0x3F);
				trail--;
			} while (trail > 0);
			Sstring_set(ss, i++, byte);
			byte = static_cast<unsigned char>(*cptr++);
			continue;
		}

		// no match..
		if (i == ll)
		{
			break;
		}
		byte = static_cast<unsigned char>(*cptr++);
	}

	return ss;
}

ptr constUTF8toSstring(std::string s)
{
	return constUTF8toSstring(s.c_str());
}

std::wstring Utility::widen(const std::string& in)
{
	// Calculate target buffer size (not including the zero terminator).
	const auto len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
		in.c_str(), in.size(), NULL, 0);
	if (len == 0)
	{
		throw std::runtime_error("Invalid character sequence.");
	}

	std::wstring out(len, 0);
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
		in.c_str(), in.size(), &out[0], out.size());
	return out;
}

std::wstring Utility::GetFullPathFor(LPCWSTR relativePath)
{
	WCHAR path[MAX_PATH];
	GetModuleFileNameW(0, path, MAX_PATH);
	std::wstring pathName(path);

	std::size_t index = pathName.find_last_of(L"\\") + 1;
	pathName.replace(index, pathName.length(), relativePath);

	return pathName;
}

std::wstring Utility::s2_ws(const std::string& str)
{
	using convert_type_x = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_type_x, wchar_t> converter_x;
	return converter_x.from_bytes(str);
}

std::string Utility::ws_2s(const std::wstring& wstr)
{
	using convert_type_x = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_type_x, wchar_t> converter_x;
	return converter_x.to_bytes(wstr);
}
 
std::string trim_space(const std::string& s)
{
	auto start = s.begin();
	while (start != s.end() && 
		  (std::isspace(*start) ||        
			(*start)=='â' || 
			(*start)=='€' ||
			(*start)=='‹')) {
		start++;
	}
	
	auto end = s.end();
	do {
		end--;
	} while (std::distance(start, end) > 0 && (std::isspace(*start) ||
		(*start) == 'â' ||
		(*start) == '€' ||
		(*start) == '‹'));

	return std::string(start, end + 1);
}


std::string Utility::trim(std::string s)
{
	return trim_space(s);
}

ptr Utility::UTF8toSchemeString(std::string s)
{
	return constUTF8toSstring(s.c_str());
}

// Our App Actual
CDockContainerApp theApp;



CDockContainerApp* Utility::get_app()
{
	return &theApp;
}


HWND global_main_window = nullptr;

void Utility::set_main(HWND h)
{
	global_main_window = h;
}

HWND Utility::get_main()
{
	return global_main_window;
}


HWND global_browser_window = nullptr;
void Utility::set_browser(HWND h)
{
	global_browser_window = h;
}

HWND Utility::get_browser()
{
	return global_browser_window;
}

 