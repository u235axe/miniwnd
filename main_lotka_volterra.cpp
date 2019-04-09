#include <iostream>
#include <array>
#include <random>
#include "miniwindow.h"

template<typename T>
struct Vector2
{
	T x, y;
};
template<typename T> Vector2<T> operator+(Vector2<T> const& v1, Vector2<T> const& v2){ return {v1.x + v2.x, v1.y + v2.y}; }
template<typename T> Vector2<T> operator-(Vector2<T> const& v1, Vector2<T> const& v2){ return {v1.x - v2.x, v1.y - v2.y}; }
template<typename T> Vector2<T> operator*(T const& scl, Vector2<T> const& v){ return {scl*v.x, scl*v.y}; }
template<typename T> Vector2<T> operator*(Vector2<T> const& v, T const& scl){ return {v.x*scl, v.y*scl}; }
template<typename T> Vector2<T> operator/(Vector2<T> const& v, T const& scl){ return {v.x/scl, v.y/scl}; }

template<typename State, typename T, typename RHS, typename Callback> 
auto solve_rk4(State y0, T t0, T t1, T h, RHS f, Callback cb)
{
	T t = t0;
	State y = y0;
	while(t < t1)
	{
		if(t + h > t1){ h = t1 - t; }
		State k1 = f(t, y);
		State k2 = f(t + h * (T)0.5, y + (h * (T)0.5) * k1);
		State k3 = f(t + h * (T)0.5, y + (h * (T)0.5) * k2);
		State k4 = f(t + h,          y +  h           * k3);
	
        y = y + (k1 + k4 + (T)2 * (k2 + k3)) * (h / (T)6);
		t = t + h;
		cb(t, y);
	}
	return y;
}

struct App
{
	MainWindow wnd;

	Vector2<double> state;
	double time, h;
	double a, b, c, d;

	std::vector<Vector2<double>> data;

	App()
	{
		a = 20., b = 1., c = 30.0, d = 1.;
		h = 0.00002;
		time = 0.0;
		state = {8.0, 12.0};
	}

	int enterApp()
	{
		wnd.window.eventDriven = false;

		wnd.mouseHandler([&](Mouse const&){ });
		wnd.resizeHandler([&](int /*w*/, int /*h*/, StateChange /*sc*/){} );
		wnd.idleHandler([&]
		{
			state = solve_rk4(state, time, time + 0.001, h,
			[this](double const& /*t*/, Vector2<double> const& s)->Vector2<double>
			{
				return {a*s.x - b*s.x*s.y, d*s.x*s.y - c*s.y };
			},
			[this](double const& /*t*/, Vector2<double> const& state)
			{
				//std::cout << t << "   " << state.x << "   " << state.y << "\n";
				data.push_back(state);
			} );
		});
		wnd.exitHandler([&]{});

		wnd.renderHandler( [&](SoftwareRenderer& r)
		{
			r.forall_pixels([](auto, auto, auto){ return color(255, 255, 255); });
			if(data.size() > 4)
			{
				r.lineplot(16, 16, wnd.width()-32, wnd.height()-32, 0.0, (double)(data.size()-1), 0.0, 100.0, color(128, 128, 128), [&](double i){ return data[(int)i].x; });
				r.lineplot(16, 16, wnd.width()-32, wnd.height()-32, 0.0, (double)(data.size()-1), 0.0, 100.0, color(255, 64, 0), [&](double i){ return data[(int)i].y; });
			}
		});

		bool res = wnd.open(L"C++ App", {42, 64}, {640, 480}, true, [&]{ return true; });
		return res ? 0 : -1;
	}
};

//#ifdef _WIN32
//int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
//#else
int main()
//#endif
{
	return App{}.enterApp();
}