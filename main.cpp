#include <iostream>
#include <cmath>
#include "miniwindow.h"

struct App
{
	MainWindow wnd;
	int x, y, z;

	App()
	{
		x = 0; y = 0, z = 0;
	}

	int enterApp()
	{
		wnd.window.eventDriven = false;

		wnd.mouseHandler([&](Mouse const& m)
		{ 
			x = m.x; y = m.y;
			
			if     (m.event == Mouse::Move      ){            /*std::cout << "Mouse moved: " << x << " " << y << "\n";*/ }
			else if(m.event == Mouse::Scroll    ){ z += m.dz; std::cout << "Mouse scrolled: " << z << "\n"; }
			else if(m.event == Mouse::LeftDown  ){            std::cout << "Mouse Left Down\n"  ; ; }
			else if(m.event == Mouse::LeftUp    ){            std::cout << "Mouse Left Up\n"    ; }
			else if(m.event == Mouse::MiddleDown){            std::cout << "Mouse Middle Down\n"; wnd.window.fullscreen(); }
			else if(m.event == Mouse::MiddleUp  ){            std::cout << "Mouse Middle Up\n"  ; }
			else if(m.event == Mouse::RightDown ){            std::cout << "Mouse Right Down\n" ; wnd.window.restore(); }
			else if(m.event == Mouse::RightUp   ){            std::cout << "Mouse Right Up\n"   ; }
		});
		wnd.resizeHandler([&](int w, int h, bool b)
		{
			printf("Resize: %i %i\n", w, h);
		} );
		wnd.idleHandler([&]{});
		wnd.exitHandler([&]
		{
			wnd.quit(); std::cout << "App closing\n";
		});

		wnd.renderHandler( [&](SoftwareRenderer& r)
		{
			r.forall_pixels([](auto x, auto y, auto){ return color(0, 0, 64); });
		});

		bool res = wnd.open(L"C++ App", {4, 64}, {640, 480}, true, false, [&]{ return true; });
		return res ? 0 : -1;
	}
};

//#ifdef _WIN32
//int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
//#else
int main()
//#endif
{
	std::cout << "Let the fun begin!\n";
	return App{}.enterApp();
}