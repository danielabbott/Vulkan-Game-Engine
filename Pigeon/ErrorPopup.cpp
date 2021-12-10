#include "ErrorPopup.h"

#include <spdlog/spdlog.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <Windows.h>

using namespace std;
void show_error_popup(string const& text)
{
	spdlog::error("{}", text);
	MessageBoxA(nullptr, text.c_str(), "Error", 0x10);
}
#else

using namespace std;
void show_error_popup(string const& text)
{
	spdlog::error("{}", text);
}
#endif