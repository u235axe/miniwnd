#include "miniwindow.h"

int main()
{
	MainWindow wnd;
	bool res = wnd.open(L"Window name", {64, 64}, {640, 480}, true, [&]{ return true; });
	return res ? 0 : -1;
}