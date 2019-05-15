#include <iostream>
#include <array>
#include <random>
#include <future>
#include "miniwindow.h"

template<typename T>
struct Table2D
{
	std::vector<T> data;
	int w, h;

	Table2D():data{}, w{0}, h{0}{}

	void resize(int w_, int h_, T const& val_ = (T)0)
	{
		data.resize((size_t)w_ * (size_t)h_, val_);
		w = w_; h = h_;
	}

	template<typename F>
	void fill1(F&& f)
	{
		int n = size();
		for(int i=0; i<n; ++i){ data[i] = f(i); }
	}

	template<typename F>
	void fill2(F&& f)
	{
		for(int i=0; i<w; ++i)
		{
			for(int j=0; j<h; ++j)
			{
				(*this)(i, j) = f(i, j);
			}
		}
	}

    template<typename F>
	void parallel_fill2(F&& f)
	{
        static const int n_threads = 4;

        if(h < 100){ fill2(f); return; }

        std::vector<std::future<void>> fs(n_threads);
        int ilow = 0;
        int idelta = h / n_threads;
        int ihi = idelta;
        
        for(int t=0; t<n_threads; ++t)
        {
            if(t == n_threads-1){ ihi = h; }
            //std::cout << ilow << "   " << ihi << std::endl;
            fs[t] = std::async(std::launch::async, [this, f](int lo, int hi)
            {
                
                for(int i=0; i<w; ++i)
                {
                    for(int j=lo; j<hi; ++j)
                    {
                        (*this)(i, j) = f(i, j);
                    }
                }
            }, ilow, ihi);
            ilow = ihi;
            ihi += idelta;
        }

		std::for_each(fs.begin(), fs.end(), [](auto& fut){ fut.get(); });
	}

	int size() const { return w*h; }

	T      & operator[](int i)       { return data[i]; }
	T const& operator[](int i) const { return data[i]; }
	T      & operator()(int x, int y)       { return data[(size_t)y*(size_t)w+(size_t)x]; }
	T const& operator()(int x, int y) const { return data[(size_t)y*(size_t)w+(size_t)x]; }
};

struct App
{
	MainWindow wnd;
	int x, y, z;

	int idx;
	std::array<Table2D<char>, 2> table;
	Color live, dead;

    double frame_time;
    int frame_count;

	void ResizeTables(int w, int h)
	{
		if(w < 0 || h < 0){ w = h = 0; }
		idx = 0;
		table[0].resize(w, h);
		table[1].resize(w, h);

		{
			std::mt19937 mt(42);
			std::uniform_real_distribution<float> d(0.0, 1.0f);
			table[0].fill1([&](int)->char{ return d(mt) < 0.5 ? 0 : 1; }); 
			table[1].fill1([ ](int)->char{ return 0; });
		}
	}

	App()
	{
		x = 0; y = 0, z = 0;

		live = color(200, 200, 200);
		dead = color(64, 64, 64);

        frame_count = 0;
        frame_time = 0.0;
	}

	int enterApp()
	{
		wnd.window.eventDriven = false;

		wnd.mouseHandler([&](Mouse const& m)
		{ 
			x = m.x; y = m.y;
			
			if     (m.event == Mouse::Move      ){            /*std::cout << "Mouse moved: " << x << " " << y << "\n";*/ }
			else if(m.event == Mouse::Scroll    ){ z += m.dz; std::cout << "Mouse scrolled: " << z << "\n";  }
			else if(m.event == Mouse::LeftDown  ){            std::cout << "Mouse Left Down\n"  ; }
			else if(m.event == Mouse::LeftUp    ){            std::cout << "Mouse Left Up\n"    ; }
			else if(m.event == Mouse::MiddleDown){            std::cout << "Mouse Middle Down\n"; }
			else if(m.event == Mouse::MiddleUp  ){            std::cout << "Mouse Middle Up\n"  ; }
			else if(m.event == Mouse::RightDown ){            std::cout << "Mouse Right Down\n" ; }
			else if(m.event == Mouse::RightUp   ){            std::cout << "Mouse Right Up\n"   ; }
		});
		wnd.resizeHandler([&](int w, int h, StateChange /*sc*/)
		{
			ResizeTables(w-32, h-32);
			printf("Resize: %i %i\n", w, h);
		} );
		wnd.idleHandler([&]
		{
            auto t0 = std::chrono::high_resolution_clock::now();
			table[1 - idx].parallel_fill2([&t = table[idx]](int x, int y)->char
			{
				auto w = t.w;
				auto h = t.h;
				if(w < 1 || h < 1){ return 0; }
				auto c = t(x, y);
				auto xp1 = x == w-1 ? 0   : x+1;
				auto xm1 = x == 0   ? w-1 : x-1;
				auto yp1 = y == h-1 ? 0   : y+1;
				auto ym1 = y == 0   ? h-1 : y-1;
				auto sum = t(xm1, ym1) + t(x, ym1) + t(xp1, ym1)
					     + t(xm1, y  ) +             t(xp1, y  )
					     + t(xm1, yp1) + t(x, yp1) + t(xp1, yp1);
				if(c == 0 &&  sum == 3           ){ return 1; }
				if(c == 1 && (sum < 2 || sum > 3)){ return 0; }
				return c;
			});
			idx = 1 - idx;
            auto t1 = std::chrono::high_resolution_clock::now();
            auto dt = (static_cast<std::chrono::duration<double, std::milli>>(t1-t0)).count();
            frame_time += dt;
            frame_count += 1;
		});
		wnd.exitHandler([&]{ });

		wnd.renderHandler( [&](SoftwareRenderer& r)
		{
			r.forall_pixels([](auto, auto, auto){ return color(255, 255, 255); });
			r.plot_by_index(16, 16, table[idx].w, table[idx].h, [&](auto x, auto y){ return table[idx](x, y) == 0 ? dead : live; });
            if(frame_count == 200)
            {
                std::cout << "Average step time: " << frame_time / frame_count << std::endl;
                frame_time = 0.0;
                frame_count = 0;
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