#pragma once
#include <stdio.h>
#include <algorithm>
#include <string>
#include <vector>
#include <codecvt>
#include <locale>
#include <chrono>
#include <functional>
#include <cmath>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <WinUser.h>
#include <Windowsx.h>

#else
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/Xutil.h>

std::string to_utf8_string(std::wstring const& wstr){ return std::wstring_convert<std::codecvt_utf8<wchar_t>>{}.to_bytes(wstr); }

#endif

struct Pos2D{ int x, y; };

struct Size2D{ int w, h; int area() const { return w*h; } };

template<typename T> struct Point{ T x, y; };

struct Color
{
	unsigned char b, g, r, a;
};

auto color(int r, int g, int b, int a = 255){ return Color{(unsigned char)b, (unsigned char)g, (unsigned char)r, (unsigned char)a}; }

#ifdef _WIN32
unsigned long packed_color(Color const& c){ return RGB((unsigned char)c.r, (unsigned char)c.g, (unsigned char)c.b); }
#else
unsigned long packed_color(Color const& c){ return ((((unsigned long)c.a*256 + (unsigned long)c.r)*256)+(unsigned long)c.g)*256+(unsigned long)c.b; }
#endif

struct Image2D
{
	std::vector<Color> data;
	int w, h;

	Image2D():data{}, w{0}, h{0}{}

	void resize(int w_, int h_)
	{
		data.resize((size_t)w_ * (size_t)h_);
		w = w_; h = h_;
	}

	Color      & operator()(int x, int y)      { return data[(size_t)y*(size_t)w+(size_t)x]; }
	Color const& operator()(int x, int y)const { return data[(size_t)y*(size_t)w+(size_t)x]; }
};

struct Mouse
{
	enum Event{ Move, Scroll, LeftDown, LeftUp, MiddleDown, MiddleUp, RightDown, RightUp};
	int x, y, dz;
	Event event;
	bool left, middle, right;
};

enum StateChange{ Restored, Minimized, Maximized, Resized, Unknown };

namespace MainWindowDetails
{
	struct ProcRelay
	{
		Mouse mouse;

		std::function<void(void)>                  onRender;
		std::function<void(int, int, StateChange)> onResize;
		std::function<void(void)>                  onExit;
		std::function<void(Mouse const&)>          onMouseEvent;

		ProcRelay():onRender{[]{}}, onResize{[](int, int, StateChange){}}, onExit{[]{}}, onMouseEvent{[](Mouse const&){}}{}

		void mouse_trigger(Mouse::Event e){ mouse.event = e; onMouseEvent(mouse); }
		void mouse_xy    (int x, int y){ mouse.x = x; mouse.y = y; mouse_trigger(Mouse::Move      ); }
		void mouse_z     (int       dz){ mouse.dz = dz;            mouse_trigger(Mouse::Scroll    ); }
		void mouse_left_down(         ){ mouse.left = true;        mouse_trigger(Mouse::LeftDown  ); }
		void mouse_left_up(           ){ mouse.left = false;       mouse_trigger(Mouse::LeftUp    ); }
		void mouse_middle_down(       ){ mouse.middle = true;      mouse_trigger(Mouse::MiddleDown); }
		void mouse_middle_up(         ){ mouse.middle = false;     mouse_trigger(Mouse::MiddleUp  ); }
		void mouse_right_down(        ){ mouse.right = true;       mouse_trigger(Mouse::RightDown ); }
		void mouse_right_up(          ){ mouse.right = false;      mouse_trigger(Mouse::RightUp   ); }
	};
	/*inline*/ ProcRelay relay;

#ifdef _WIN32
	static StateChange fromWPARAM(WPARAM wp)
	{
		if     (wp == SIZE_RESTORED ){ return StateChange::Restored; }
		else if(wp == SIZE_MINIMIZED){ return StateChange::Minimized; }
		else if(wp == SIZE_MAXIMIZED){ return StateChange::Maximized; }
		return StateChange::Unknown;
	}

	static LRESULT CALLBACK Proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch(message)
		{
		case WM_MOUSEMOVE:   relay.mouse_xy(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)); break;
		case WM_LBUTTONDOWN: relay.mouse_left_down();                                    break;
		case WM_LBUTTONUP:   relay.mouse_left_up();                                      break;
		case WM_MBUTTONDOWN: relay.mouse_middle_down();                                  break;
		case WM_MBUTTONUP:   relay.mouse_middle_up();                                    break;
		case WM_RBUTTONDOWN: relay.mouse_right_down();                                   break;
		case WM_RBUTTONUP:   relay.mouse_right_up();                                     break;
		case WM_MOUSEWHEEL:  relay.mouse_z(GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA);  break;

		case WM_ERASEBKGND:  return 1;
		case WM_SIZE:        relay.onResize(LOWORD(lParam), HIWORD(lParam), fromWPARAM(wParam)); break;

		case WM_CLOSE:     relay.onExit();   break;
		case WM_PAINT:     relay.onRender(); break;
		default: return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	}
#else
	static void Proc(Display* /*display*/, Window& /*handle*/, XEvent e, Size2D const& size, bool& isResizing)
	{
		switch(e.type)
		{
		case MotionNotify: relay.mouse_xy(e.xbutton.x, e.xbutton.y); break;
		case EnterNotify:  break;
		case LeaveNotify:  break;
		case ButtonPress:
		{
			switch(e.xbutton.button)
			{
			case 1: relay.mouse_left_down();   break;
			case 2: relay.mouse_middle_down(); break;
			case 3: relay.mouse_right_down();  break;
			case 4: relay.mouse_z(1);          break;
			case 5: relay.mouse_z(-1);         break;
			}
			break;
		}
		case ButtonRelease:
		{
			switch(e.xbutton.button)
			{
			case 1: relay.mouse_left_up();   break;
			case 2: relay.mouse_middle_up(); break;
			case 3: relay.mouse_right_up();  break;
				//case 4: relay.mouse_z(1);          break;//?
				//case 5: relay.mouse_z(-1)          break;//?
			}
			break;
		}
		case ResizeRequest:
		{
			//XResizeRequestEvent re = e.xresizerequest;
			//printf("RR\n");
			break;
		}
		case ConfigureRequest:
		{
			//XConfigureRequestEvent re = e.xconfigurerequest;
			//printf("CR\n");
			break;
		}
		case ConfigureNotify:
		{
			XConfigureEvent re = e.xconfigure;
			if( size.w == re.width && size.h == re.height )
			{
				//printf("CN Move %i %i\n", re.x, re.y);
				//XMoveWindow(display, handle, 0, 0);
			}//move only
			else
			{
				isResizing = true;
				//printf("CN Resize %i %i\n", re.width, re.height);
				relay.onResize(re.width, re.height, StateChange::Resized);
			}
			break;
		}
		//CM handled outside.
		case Expose: relay.onRender(); break;
		}
	}
#endif
}

struct PlatformWindowData
{
	enum State{ Invalid, Normal, Minimized, Maximized, Hidden, Fullscreen };

	Pos2D  pos,  last_pos;
	Size2D size, last_size;
	State state, last_state;
	std::wstring title;
	
#ifdef _WIN32
	WNDCLASSW wc;
	DWORD type;
	DWORD style;
	HWND handle;
	HICON icon;
	bool eventDriven;
	PlatformWindowData():handle{nullptr}, eventDriven{true}, state{State::Invalid}, last_state{State::Invalid}{}

	bool open(std::wstring const& title_, Pos2D pos_, Size2D size_, bool Decorated_/*, bool FullScreen_*/)
	{
		using namespace MainWindowDetails;
		wc = WNDCLASSW{0};
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wc.lpfnWndProc   = Proc;
		wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(BLACK_BRUSH);
		wc.lpszClassName = L"MainWindowClass";

		if( !RegisterClassW(&wc) ){ return false; }

		type  = Decorated_ ? WS_OVERLAPPEDWINDOW : WS_POPUP;
		style = WS_CLIPCHILDREN  | WS_CLIPSIBLINGS;
		RECT rect0; rect0.left = pos_.x; rect0.right = pos_.x+size_.w; rect0.top = pos_.y; rect0.bottom = pos_.y+size_.h;
		if(!AdjustWindowRectEx(&rect0, style, FALSE, 0)){ return false; }

		pos.x = rect0.left;
		size.w = rect0.right - rect0.left;

		pos.y = rect0.top;
		size.h = rect0.bottom - rect0.top;

		last_state = State::Invalid;
		state = State::Invalid;

		handle = CreateWindowExW(0, wc.lpszClassName, title_.c_str(), style | type, rect0.left, rect0.top, rect0.right - rect0.left, rect0.bottom - rect0.top, 0, 0, nullptr, (LPVOID)&relay);
		if( !handle ){ return false; }

		last_state = State::Normal;
		state = State::Hidden;

		last_pos  = pos;
		last_size = size;
		
		title = title_;
		return true;
	}

	bool rename(std::wstring const& name)
	{
		if(SetWindowTextW(handle, name.c_str())){ title = name; return true; }
		return false;
	}

	bool setIcon( Image2D const& img )
	{
		HDC hdcScreen = GetDC(NULL);
		HDC hdcMem = CreateCompatibleDC(hdcScreen);

		// Create the bitmap, and select it into the device context for drawing.
		HBITMAP hbmp = CreateCompatibleBitmap(hdcScreen, img.w, img.h);    
		HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcMem, hbmp);

		HDC     tmpdc  = CreateCompatibleDC(hdcScreen);
		HBITMAP bmp    = CreateBitmap(img.w, img.h, 1, 32, img.data.data());
		HGDIOBJ tmpbmp = SelectObject(tmpdc, bmp);
		BitBlt(hdcMem, 0, 0, img.w, img.h, tmpdc, 0, 0, SRCCOPY);
		
		HBITMAP hbmpMask = CreateCompatibleBitmap(hdcScreen, img.w, img.h);
		ICONINFO ii;
		ii.fIcon = TRUE;
		ii.hbmMask = hbmpMask;
		ii.hbmColor = hbmp;
		icon = CreateIconIndirect(&ii);
		DeleteObject(hbmpMask);

		// Clean-up.
		SelectObject(tmpdc, tmpbmp);
		DeleteObject(tmpbmp);
		ReleaseDC(NULL, tmpdc);
		SelectObject(hdcMem, hbmpOld);
		DeleteObject(hbmp);
		DeleteDC(hdcMem);
		ReleaseDC(NULL, hdcScreen);

		SetClassLongPtr(handle, GCLP_HICON, (LONG_PTR)icon);

		return true;
	}

	template<typename F>
	void loop(F&& step)
	{
		MSG msg{0};
		DWORD last_time = 0;
		while( true ) 
		{
			if( eventDriven )
			{
				if( GetMessage(&msg, 0, 0, 0) < 0 ){ close(); }
				{
					TranslateMessage( &msg );
					DispatchMessage( &msg );
					if(msg.message == WM_QUIT){ break; }
				}
				step();
				if(msg.time - last_time > 100)
				{
					redraw();
					last_time = msg.time;
				}
			}
			else
			{
				while( PeekMessage(&msg, 0, 0, 0, PM_REMOVE) != 0 )
				{
					TranslateMessage( &msg );
					DispatchMessage( &msg );
					if(msg.message == WM_QUIT){ break; }
					
				}
				if(msg.message == WM_QUIT){ break; }
				step();
				//++last_time;
				//if(last_time > 2)
				{
					redraw();
					//last_time = 0;
				}
			}
		}
	}

	void redraw()   const { RedrawWindow(handle, 0, 0, RDW_ERASE|RDW_INVALIDATE); }
	void hide()     { if(state != State::Invalid && state != State::Hidden){ ShowWindow(handle, SW_HIDE); last_state = state; state = State::Hidden; } }
	void show()     { if(state != State::Invalid && state == State::Hidden){ ShowWindow(handle, SW_SHOW); std::swap(state, last_state); } }
	void minimize() { if(state != State::Invalid || state != State::Minimized){ last_pos = pos; last_size = size; ShowWindow(handle, SW_MINIMIZE);           last_state = state; state = State::Minimized; } }
	void maximize() { if(state != State::Invalid || state != State::Maximized){ last_pos = pos; last_size = size; ShowWindow(handle, SW_MAXIMIZE); redraw(); last_state = state; state = State::Maximized; } }
	void restore()  { if(state != State::Invalid ){ ShowWindow(handle, SW_RESTORE); state = last_state; pos = last_pos; size = last_size; } }
	void quit()     { PostQuitMessage(0); }
	bool close(){ int res = DestroyWindow(handle); handle = nullptr; eventDriven = true; state = State::Invalid; return res != 0; }

	/*bool fullscreen(bool b)
	{
		bool res;
		if(b)
		{
			last_pos = pos;
			last_size = size;

			DEVMODE s{};
			s.dmSize = sizeof(s);
			EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &s);
			//s.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;
			std::cout << "w x h = " << s.dmPelsWidth << " x " << s.dmPelsHeight << " depth: " << s.dmBitsPerPel << " Freq: " << s.dmDisplayFrequency << " Hz.\n";

			//SetWindowLongPtr(handle, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_TOPMOST);
			SetWindowLongPtr(handle, GWL_STYLE,   WS_POPUP | WS_VISIBLE);
			SetWindowPos    (handle, HWND_TOPMOST, 0, 0, s.dmPelsWidth, s.dmPelsHeight, SWP_SHOWWINDOW);
			res = ChangeDisplaySettings(&s, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL;
			ShowWindow(handle, SW_MAXIMIZE);
			return res;
		}
		else
		{
			SetWindowLongPtr(handle, GWL_EXSTYLE, WS_EX_LEFT);
			SetWindowLongPtr(handle, GWL_STYLE, type | WS_VISIBLE);
			res = ChangeDisplaySettings(nullptr, CDS_RESET) == DISP_CHANGE_SUCCESSFUL;
			SetWindowPos(handle, HWND_NOTOPMOST, last_pos.x, last_pos.y, last_size.w, last_size.h, SWP_SHOWWINDOW);
			ShowWindow(handle, SW_RESTORE);
			return res;
		}
		return true;
	}*/
#else
	Display* display;
	Visual* visual;
	int screen;
	unsigned int depth;
	Window handle;
	Atom AWM_DELETE_WINDOW, AWM_PROTOCOLS;
	bool eventDriven, needRedraw, isResizing, isQuit;
	PlatformWindowData():display{nullptr}, visual{nullptr}, screen{0}, eventDriven{true}, needRedraw{false}, isResizing{false}, isQuit{true}{}

	bool rename(std::wstring const& name)
	{
		auto title2 = to_utf8_string(name);
		Atom wmname = XInternAtom(display, "_NET_WM_NAME", True);
		Atom utf8str = XInternAtom(display, "UTF8_STRING", True);
		if(wmname == None){ printf("_NET_WM_NAME not found. Window title was not set.\n"); }
		else
		{
			if(utf8str == None){ printf("UTF8_STRING not found. Window title was not set.\n"); }
			else
			{
				XChangeProperty(display, handle, wmname, utf8str, 8, PropModeReplace, (const unsigned char*)title2.c_str(), title2.size());
				title = name;
				return true;
			}
		}
		return false;
	}

	bool open(std::wstring const& title_, Pos2D pos_, Size2D size_, bool Decorated_/*, bool FullScreen_*/)
	{
		printf("Creating window\n");

		display = XOpenDisplay(0);
		if(!display){ printf("Cannot open display\n"); return false; }
		screen = DefaultScreen(display);
		visual = DefaultVisual(display, screen);
		depth = DefaultDepth(display, screen);
		XVisualInfo vi;
		if( XMatchVisualInfo(display, screen, 32, TrueColor, &vi) != 0 )
		{
			visual = vi.visual;
			depth = vi.depth;
			printf("XMatchVisual success!\n");
		}
		else
		{
			printf("Visual not found! Default is used.\n");
		}

		if(visual->c_class != TrueColor)
		{
			printf("No TrueColor...\n");
		}

		XSetWindowAttributes wa{};
		wa.colormap = XCreateColormap(display, DefaultRootWindow(display), visual, AllocNone);
		//wa.background_pixel = 0;
		wa.border_pixel = 0;
		wa.backing_store = Always;
		wa.background_pixmap = None;
		//wa.override_redirect = !Decorated_ ? True : False;
		unsigned long mask = /*CWBackPixel | */CWBackPixmap | CWColormap | CWBorderPixel | CWBackingStore;// | CWOverrideRedirect;
		printf("Depth = %i, x = %i, y = %i\n", depth, pos_.x, pos_.y);
		handle = XCreateWindow(display, DefaultRootWindow(display), pos_.x, pos_.y, size_.w, size_.h, 0, depth, InputOutput, visual, mask, &wa);
		printf("handle = %ld\n", handle);
		isQuit = false;
		XSelectInput(display, handle, ExposureMask | StructureNotifyMask | SubstructureNotifyMask //| SubstructureRedirectMask // | */ResizeRedirectMask
			| KeyPressMask | KeyReleaseMask
			| ButtonPressMask | PointerMotionMask| LeaveWindowMask| EnterWindowMask| ButtonReleaseMask );
		
		rename(title_);
		last_pos = pos = pos_;
		last_size = size = size_;

		AWM_PROTOCOLS     = XInternAtom(display, "WM_PROTOCOLS", False);
		AWM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", False);
		XSetWMProtocols(display, handle, &AWM_DELETE_WINDOW, 1);

		if(!Decorated_)
		{
			struct Hints
			{
				unsigned long   flags;       //Functions 1, Decorations 2, InputMode 4, Status 8
				unsigned long   functions;   //None 0, All 1, Resize 2, Move 4, Minimize 8, Maximize 16, Close 32
				unsigned long   decorations; //None 0, All 1, Border 2, ResizeH 4, Title 8, Menu 16, Minimize 32, Maximize 64
				long            inputMode;   //Modeless = 0, PrimaryApplicationModal = 1, SystemModal = 2, FullApplicationModal = 3
				unsigned long   status;      //TearoffWindow 1
			};
			Hints   hints; hints.flags = 2; hints.decorations = 0;
			Atom    prop = XInternAtom(display,"_MOTIF_WM_HINTS",True);
			XChangeProperty(display, handle, prop, prop, 32, PropModeReplace, (unsigned char*)&hints, 5);
		}
		return true;
	}

	template<typename F>
	void loop(F&& step)
	{
		auto t0 = std::chrono::high_resolution_clock::now();
		auto t1 = t0, trsz = t0;
		printf("Entering event loop\n");
		long nredraw = 0, nredrawproc = 0;

		auto sendExposeEvent = [&]()
		{
			XExposeEvent xee{};
			xee.type = Expose;
			xee.send_event = True;
			xee.display = display;
			xee.window = handle;
			XSendEvent(display, handle, False, NoEventMask, (XEvent*)&xee);
		};

		//Reposition the window, because some WMs move the window initially despite the x, y, set in create window.
		XMoveResizeWindow(display, handle, last_pos.x, last_pos.y, last_size.w, last_size.h);
		while(true)
		{
			XEvent e;
			if(eventDriven)
			{
				XNextEvent(display, &e);
				if(e.type == ClientMessage && e.xclient.message_type == AWM_PROTOCOLS && (Atom)(e.xclient.data.l[0]) == AWM_DELETE_WINDOW){ isQuit = true; MainWindowDetails::relay.onExit(); break; }
				else{ MainWindowDetails::Proc(display, handle, e, size, isResizing); }
				//step();
				//printf("while 1\n");
				needRedraw = true;
			}
			else
			{
				//auto res = True;
				//while(res == True && !isQuit)
				{
					while(XEventsQueued(display, QueuedAlready) > 0)
					{
						XNextEvent(display, &e);
						if(e.type == ClientMessage)
						{
							if(e.xclient.send_event){ /*printf("[%zi] Sent CM ", e.xclient.serial);*/ }
							else{ printf("[%zi] Not sent CM ", e.xclient.serial); }

							if(e.xclient.message_type == AWM_PROTOCOLS)
							{
								//printf("WM_PROTOCOLS ");
								if( (Atom)(e.xclient.data.l[0]) == AWM_DELETE_WINDOW){ /*printf("AWM_DELETE_WINDOW\n");*/ isQuit = true; MainWindowDetails::relay.onExit(); break; }
								else{ printf("Unknown protocol message %zi\n", e.xclient.data.l[0]); }
							}
							else if(e.xclient.message_type == 424242){ nredrawproc = e.xclient.data.l[0]; /*printf("Redraw processed: %zi\n", nredrawproc);*/ }
							else{ printf("Other Atom %zi ", e.xclient.message_type); }
						}
						else
						{
							bool wasResize = false;
							MainWindowDetails::Proc(display, handle, e, size, wasResize);
							if(wasResize){ isResizing = true; trsz = std::chrono::high_resolution_clock::now(); }
						}
					}
					needRedraw = true;
				}
			}

			if(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()-trsz).count()/1000.0 > 300.0)
			{
				//printf("isResizing = false\n");
				isResizing = false;
				needRedraw = true;
			}

			if(needRedraw)
			{
				double limit = (isResizing || eventDriven) ? 250.0 : 1.0;
				t1 = std::chrono::high_resolution_clock::now();
				auto dt = std::chrono::duration_cast<std::chrono::microseconds>(t1-t0).count()/1000.0;
				if(dt > limit)
				{
					if(eventDriven){ /*printf("limited redraw, expose event, dt = %f\n", dt); sendExposeEvent();*//*XClearArea(display, handle, 0, 0, 1, 1, true);*/ }
					else if(!eventDriven && (nredraw == nredrawproc))
					{
						step();
						sendExposeEvent();//XClearArea(display, handle, 0, 0, 1, 1, true);
						nredraw += 1;
						XEvent ev{0};
						ev.type = ClientMessage;
						ev.xclient.format = 32;
						ev.xclient.window = handle;
						ev.xclient.message_type = 424242;
						ev.xclient.data.l[0] = nredraw;
						XSendEvent(display, handle, False, NoEventMask, &ev);
						XSync(display, False);
						//XFlush(display);
					}
					t0 = t1;
					needRedraw = false;
				}
			}
			//
			//
			if(isQuit){ break; }
		}
	}

	/*void print_last() const
	{
		printf("lx= %i, ly= %i, lw= %i, lh= %i\n", last_pos.x, last_pos.y, last_size.w, last_size.h);
		printf("x= %i, y= %i, w= %i, h= %i\n", pos.x, pos.y, size.w, size.h);
	}*/

	void redraw() { /*printf("Redraw req\n");*/ needRedraw = true; /*XSync(display, False);*/ }
	void show() const { /*printf("show\n");*/ XMapRaised(display, handle); }
	void hide() const { XUnmapWindow(display, handle); }
	void minimize() { last_pos = pos; last_size = size; XIconifyWindow(display, handle, 0); }
	void maximize()
	{
		last_pos = pos; last_size = size;

		XEvent xev{};
		Atom wm_state  =  XInternAtom(display, "_NET_WM_STATE", False);
		Atom max_horz  =  XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
		Atom max_vert  =  XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);

		xev.type = ClientMessage;
		xev.xclient.window = handle;
		xev.xclient.message_type = wm_state;
		xev.xclient.format = 32;
		xev.xclient.data.l[0] = 1;//Add
		xev.xclient.data.l[1] = max_horz;
		xev.xclient.data.l[2] = max_vert;
		xev.xclient.data.l[3] = 0;
		XSendEvent(display, DefaultRootWindow(display), False, SubstructureNotifyMask, &xev);
		XSync(display, False);
	}

	void restore()
	{
		//switch back:
		/*int n;
		XF86VidModeModeInfo** modes;
		XF86VidModeGetAllModeLines(display, screen, &n, &modes);
		XF86VidModeSwitchToMode(display, screen, modes[0]);
		XF86VidModeSetViewPort(display, screen, 0, 0);
		XFree(modes);*/

		/*XSetWindowAttributes attributes;
		attributes.override_redirect = False;
		XChangeWindowAttributes(display, handle, CWOverrideRedirect, &attributes);*/

		//XSync(display, False);

		{
			XEvent xev{};
			Atom wm_state  =  XInternAtom(display, "_NET_WM_STATE", False);
			Atom max_horz  =  XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
			Atom max_vert  =  XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);

			xev.type = ClientMessage;
			xev.xclient.window = handle;
			xev.xclient.message_type = wm_state;
			xev.xclient.format = 32;
			xev.xclient.data.l[0] = 0;//Remove
			xev.xclient.data.l[1] = max_horz;
			xev.xclient.data.l[2] = max_vert;
			xev.xclient.data.l[3] = 1;
			XSendEvent(display, DefaultRootWindow(display), False, SubstructureNotifyMask, &xev);
		}

		/*{
			XEvent xev{};
			Atom wm_state      = XInternAtom(display, "_NET_WM_STATE", False);
			Atom wm_fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);

			xev.type = ClientMessage;
			xev.xclient.window = handle;
			xev.xclient.message_type = wm_state;
			xev.xclient.format = 32;
			xev.xclient.data.l[0] = 0;//Remove
			xev.xclient.data.l[1] = wm_fullscreen;
			xev.xclient.data.l[2] = 0;
			xev.xclient.data.l[3] = 1;
			XSendEvent(display, DefaultRootWindow(display), False, SubstructureNotifyMask, &xev);
		}*/
		//XSync(display, False);
		
		XMoveResizeWindow(display, handle, last_pos.x, last_pos.y, last_size.w, last_size.h);
		pos = last_pos;
		size = last_size;
		//XRaiseWindow(display, handle);
		XSync(display, False);
	}

	bool setIcon( Image2D const& img )
	{
		Atom net_wm_icon = XInternAtom(display, "_NET_WM_ICON", False);
		Atom cardinal = XInternAtom(display, "CARDINAL", False);
		size_t length = 2 + img.w * img.h;
		std::vector<unsigned long> data; data.resize(length);
		data[0] = img.w;
		data[1] = img.h;
		for(int i=0; i<img.w*img.h; ++i)
		{
			data[2+i] = packed_color(img.data[i]);
		}
		XChangeProperty(display, handle, net_wm_icon, cardinal, 32, PropModeReplace, (const unsigned char*)data.data(), length);
		XSync(display, False);
		return true;
	}

	void quit() const
	{
		XEvent e;
		e.xclient.type = ClientMessage;
		e.xclient.window = handle;
		e.xclient.message_type = XInternAtom(display, "WM_PROTOCOLS", True);
		e.xclient.format = 32;
		e.xclient.data.l[0] = XInternAtom(display, "WM_DELETE_WINDOW", False);
		e.xclient.data.l[1] = CurrentTime;
		XSendEvent(display, handle, False, NoEventMask, &e);
		XSync(display, False);
	}
	bool close(){ XDestroyWindow(display, handle); XCloseDisplay(display); eventDriven = true; return true; }

	void fullscreen()
	{
		last_pos = pos; last_size = size;
		//maximize();
		/*Atom wm_state      = XInternAtom(display, "_NET_WM_STATE", False );
		Atom wm_fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False );
		XChangeProperty(display, handle, wm_state, XA_ATOM, 32, PropModeReplace, (unsigned char *)&wm_fullscreen, 1);*/


		XEvent xev{};
		Atom wm_state      = XInternAtom(display, "_NET_WM_STATE", False);
		Atom wm_fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);

		xev.type = ClientMessage;
		xev.xclient.window = handle;
		xev.xclient.message_type = wm_state;
		xev.xclient.format = 32;
		xev.xclient.data.l[0] = 1;//Add
		xev.xclient.data.l[1] = wm_fullscreen;
		xev.xclient.data.l[2] = 0;
		xev.xclient.data.l[3] = 1;
		XSendEvent(display, DefaultRootWindow(display), False, SubstructureNotifyMask, &xev);


		/*XSetWindowAttributes attributes;
		attributes.override_redirect = True;
		XChangeWindowAttributes(display, handle, CWOverrideRedirect, &attributes);*/

		XSync(display, False);

		/*int n;
		XF86VidModeModeInfo** modes;
		XF86VidModeModeInfo* fullscreenMode;

		auto screenWidth  = XWidthOfScreen(XScreenOfDisplay(display, screen));
		auto screenHeight = XHeightOfScreen(XScreenOfDisplay(display, screen));

		//switch to full screen mode:
		XF86VidModeGetAllModeLines (display, screen, &n, &modes);
		for (int i = 0; i < n; ++i)
		{
			if ((modes[i]->hdisplay == screenWidth) &&
				(modes[i]->vdisplay == screenHeight))
			{
				fullscreenMode = modes[i];
			}
		}

		XF86VidModeSwitchToMode(display, screen, fullscreenMode);
		XF86VidModeSetViewPort(display, screen, 0, 0);
		auto displayWidth = fullscreenMode->hdisplay;
		auto displayHeight = fullscreenMode->vdisplay;
		XFree(modes);*/

		/* Warp mouse pointer at center of the screen */
		//XWarpPointer(display, None, handle, 0, 0, 0, 0, displayWidth / 2, displayHeight / 2);
		//XMapRaised(display, win->win);
		//XGrabKeyboard(display, handle, True, GrabModeAsync, GrabModeAsync, CurrentTime);
		//XGrabPointer(display, handle, True, ButtonPressMask, GrabModeAsync, GrabModeAsync, handle, None, CurrentTime);
		//XSync(display, False);
	}
#endif
};

bool is_finite(double x){ return std::isfinite(x); }
bool is_finite(float x){ return std::isfinite(x); }
bool is_finite(long double x){ return std::isfinite(x); }

template<typename T, typename R = std::enable_if_t<std::is_integral<T>::value>>
bool is_finite(T x){ return true; }

template<typename T>
auto clamp(T x, T min, T max){ return x < min ? min : (x > max ? max : x); }

struct SoftwareRenderer
{
	Image2D backbuffer;

	void init  (int w, int h){ backbuffer.resize(w, h); }
	void resize(int w, int h){ printf("Renderer resize %i %i\n", w, h); backbuffer.resize(w, h); }
	void close(){}

	void setpixel(int x, int y, Color c)
	{
		if(clamp(x, 0, backbuffer.w-1) == x && clamp(y, 0, backbuffer.h-1) == y){ backbuffer(x, y) = c; }
	}

	Color getpixel(int x, int y)
	{
		if(clamp(x, 0, backbuffer.w-1) == x && clamp(y, 0, backbuffer.h-1) == y){ return backbuffer(x, y); }
		else{ return Color{0, 0, 0, 0}; }
	}

	template<typename F>
	void forall_pixels(F&& f)
	{
		const int w = backbuffer.w;
		for(int y=0; y<backbuffer.h; ++y)
		{
			int i = y * w;
			for(int x=0; x<w; ++x, ++i)
			{
				backbuffer.data[i] = f(x, y, backbuffer.data[i]);
			}
		}
	}

	template<typename T, typename F>
	void lineplot(int x, int y, int w, int h, T xmin, T xmax, T ymin, T ymax, Color col, F&& f)
	{
		auto n = std::abs(w);
		if(n == 0){ return; }
	
		auto yl = f(xmin);
		auto iyl = y + h - (yl - ymin) / (ymax - ymin) * h;
		for(int i=1; i<n; ++i)
		{
			T xc = ((T)i/(T)n)*(xmax - xmin) + xmin;
			auto yc = f(xc);
			if(is_finite(yl) && is_finite(yc))
			{
				auto iyc = y + h - (yc - ymin) / (ymax - ymin) * h;
				line(x+i-1, (int)iyl, x+i, (int)iyc, [&](auto){ return col; });
				iyl = iyc;
				yl = yc;
			}
		}
	}

	template<typename T, typename F>
	void lineplot(int x, int y, int w, int h, T xmin, T xmax, Color col, F&& f)
	{
		using R = decltype(f(xmin));
		auto n = std::abs(w);
		if(n == 0){ return; }
		std::vector<R> tmp(n);

		R ymin = std::numeric_limits<R>::max();
		R ymax = std::numeric_limits<R>::min();
		for(int i=0; i<n; ++i)
		{
			T xc = ((T)i/(T)n)*(xmax - xmin) + xmin;
			auto yc = f(xc);
			if( is_finite(yc) )
			{
				if(yc < ymin){ ymin = yc; }
				if(yc > ymax){ ymax = yc; }
			}
			tmp[i] = yc;
		}

		auto yl = tmp[0];
		auto iyl = y + h - (yl - ymin) / (ymax - ymin) * h;
		for(int i=1; i<n; ++i)
		{
			auto yc = tmp[i];
			if(is_finite(yl) && is_finite(yc))
			{
				auto iyc = y + h - (yc - ymin) / (ymax - ymin) * h;
				line(x+i-1, (int)iyl, x+i, (int)iyc, [&](auto){ return col; });
				iyl = iyc;
				yl = yc;
			}
		}
	}

	template<typename IT>
	void barplot(int x, int y, int w, int h, IT begin, IT end, Color col)
	{
		using R = std::remove_reference_t<decltype(*begin)>;
		auto n = std::distance(begin, end);
		if(n == 0){ return; }
		
		R ymin = std::numeric_limits<R>::max();
		R ymax = std::numeric_limits<R>::min();
		for(auto i = begin; i!=end; ++i)
		{
			auto yc = *i;
			if( is_finite(yc) )
			{
				if(yc < ymin){ ymin = yc; }
				if(yc > ymax){ ymax = yc; }
			}
		}

		auto dx = (float)w / (float)n;
		auto X = x;
		auto z = y + h - ((float)0 - (float)ymin) / ((float)ymax - (float)ymin) * h;
		for(auto it = begin; it!=end; ++it)
		{
			auto yc = *it;
			auto hc = y + h - ((float)yc - (float)ymin) / ((float)ymax - (float)ymin) * h;
			if(is_finite(yc))
			{
				if(yc > 0)
				{
					filledrect(X, (int)hc, (int)dx, (int)(z - hc), col);
				}
				else
				{
					filledrect(X, (int)z, (int)dx, (int)(hc - z), col);
				}
				
			}
			X = (int)(X + dx);
		}
	}

	template<typename F>
	void plot_by_index(int x, int y, int w, int h, F&& f)
	{
		if(w <= 0 || h <= 0){ return; }
		auto ymin = clamp(y,   0, backbuffer.h-1);
		auto ymax = clamp(y+h, 0, backbuffer.h-1);
		auto xmin = clamp(x,   0, backbuffer.w-1);
		auto xmax = clamp(x+w, 0, backbuffer.w-1);
		for(int j=ymin; j<ymax; ++j)
		{
			if(j - ymin >= h){ break; }
			int k = j * backbuffer.w + xmin;
			for(int i=xmin; i<xmax; ++i, ++k)
			{
				if(i - xmin >= w){ break; }
				backbuffer.data[k] = f(i-x , j-y);
			}
		}
	}

	void rect(int x, int y, int w, int h, Color col)
	{
		if(w <= 0 || h <= 0){ return; }
		line(x,   y,   x+w, y,   [&](auto){ return col; });
		line(x+w, y,   x+w, y+h, [&](auto){ return col; });
		line(x+w, y+h, x,   y+h, [&](auto){ return col; });
		line(x,   y+h, x,   y,   [&](auto){ return col; });
	}

	void filledrect(int x, int y, int w, int h, Color col)
	{
		if(w <= 0 || h <= 0){ return; }
		auto ymin = clamp(y,   0, backbuffer.h-1);
		auto ymax = clamp(y+h, 0, backbuffer.h-1);
		auto xmin = clamp(x,   0, backbuffer.w-1);
		auto xmax = clamp(x+w, 0, backbuffer.w-1);
		for(int j=ymin; j<=ymax; ++j)
		{
			int k = j * backbuffer.w + xmin;
			for(int i=xmin; i<=xmax; ++i, ++k)
			{
				backbuffer.data[k] = col;
			}
		}
	}

	template<typename F>
	void line(int x0, int y0, int x1, int y1, F&& f)
	{
		int dx = abs(x1-x0);
		int sx = x0 < x1 ? 1 : -1;
		int dy = -abs(y1-y0);
		int sy = y0 < y1 ? 1 : -1;
		int err = dx + dy;
		int e2 = 0;
		for (;;){
			setpixel(x0, y0, f(getpixel(x0, y0)));
			e2 = 2*err;
			if (e2 >= dy) {
				if (x0 == x1) break;
				err += dy; x0 += sx;
			}
			if (e2 <= dx) {
				if (y0 == y1) break;
				err += dx; y0 += sy;
			}
		}
	}

	template<typename I, typename F>
	void hline(int x0, int x1, int y, I&& i, F&& f)
	{
		for(int x = std::min(x0, x1); x<=std::max(x0, x1); ++x){ if(i(x, y)){ backbuffer(x, y) = f(backbuffer(x, y)); } }
	}

	template<typename T, typename F>
	void triangle(T x0, T y0, T x1, T y1, T x2, T y2, F&& f)
	{
		auto isInside = [](auto X0, auto Y0, auto X1, auto Y1, auto X2, auto Y2/*, auto b0, auto b1, auto b2*/)
		{
			auto edgeFunction = [](auto px0, auto py0, auto px1, auto py1, auto px2, auto py2){ return (px2 - px0) * (py1 - py0) - (py2 - py0) * (px1 - px0); };
			return [=](T x, T y)
			{
				const float c = 0.0f;
				const float d = 0.0f;
				auto w0 = edgeFunction(X0+d, Y0+d, X1+d, Y1+d, x+c, y+c);//+b0; 
				auto w1 = edgeFunction(X1+d, Y1+d, X2+d, Y2+d, x+c, y+c);//+b1;
				auto w2 = edgeFunction(X2+d, Y2+d, X0+d, Y0+d, x+c, y+c);//+b2;
				if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f) { return true; }
				else{ return false; }
			};
		};

		//Sort vertices in asc order:
		if( y0 > y1 ){ std::swap(x0, x1); std::swap(y0, y1); }
		if( y0 > y2 ){ std::swap(x0, x2); std::swap(y0, y2); }
		if( y1 > y2 ){ std::swap(x1, x2); std::swap(y1, y2); }

		/*if( x2 < x1 ){ std::swap(x1, x2); std::swap(y1, y2); }

		auto xmin = std::min({x0, x1, x2});
		auto ymin = std::min({y0, y1, y2});
		auto xmax = std::max({x0, x1, x2});
		auto ymax = std::max({y0, y1, y2});

		auto iI0 = isInside(x0, y0, x1, y1, x2, y2);
		for(int x=xmin; x<=xmax; ++x)
		{
		for(int y=ymin; y<=ymax; ++y)
		{
		if(iI0(x, y)){ backbuffer(x, y) = f(backbuffer(x, y)); }
		}
		}

		return;*/

		auto fillBottomFlatTriangle = [&](T X0, T Y0, T X1, T Y1, T X2, T Y2, auto iI)
		{
			if(X1 > X2){ std::swap(X1, X2); std::swap(Y1, Y2); }

			auto ylim = (int)std::min(Y1, Y2);

			auto dx1 =  abs(X1-X0); auto sx1 = X0 < X1 ? 1 : -1;
			auto dy1 = -abs(Y1-Y0); auto sy1 = Y0 < Y1 ? 1 : -1;
			auto dx2 =  abs(X2-X0); auto sx2 = X0 < X2 ? 1 : -1;
			auto dy2 = -abs(Y2-Y0); auto sy2 = Y0 < Y2 ? 1 : -1;

			auto err1 = dx1 + dy1; auto e21 = 0.0f; 
			auto err2 = dx2 + dy2; auto e22 = 0.0f;
			auto x01 = X0;
			auto x02 = X0;
			auto y01 = Y0;
			auto y02 = Y0;
			for (;;){
				for(;;)
				{
					e21 = 2 * err1 + 0.5f;
					if(e21 >= dy1){ err1 += dy1; x01 += sx1; }
					if(e21 <= dx1){ err1 += dx1; break; }
				}
				for(;;)
				{
					e22 = 2 * err2 + 0.5f;
					if(e22 >= dy2){ err2 += dy2; x02 += sx2; }
					if(e22 <= dx2){ err2 += dx2; break; }
				}
				hline(x01, x02, y01, iI, f);
				y01 += sy1;
				y02 += sy2;
				if ((int)y01 > (int)ylim) break;
			}
		};

		auto fillTopFlatTriangle = [&](T X0, T Y0, T X1, T Y1, T X2, T Y2, auto iI)
		{
			if(X0 > X1){ std::swap(X1, X0); std::swap(Y1, Y0); }
			auto ylim = (int)std::max(Y0, Y1);
			auto dx1 =  abs(X0 - X2); auto sx1 = X2 < X0 ? 1 : -1;
			auto dy1 = -abs(Y0 - Y2); auto sy1 = Y2 < Y0 ? 1 : -1;
			auto dx2 =  abs(X1 - X2); auto sx2 = X2 < X1 ? 1 : -1;
			auto dy2 = -abs(Y1 - Y2); auto sy2 = Y2 < Y1 ? 1 : -1;

			auto err1 = dx1 + dy1; auto e21 = 0.0f; 
			auto err2 = dx2 + dy2; auto e22 = 0.0f;

			auto x01 = X2;
			auto x02 = X2;
			auto y01 = Y2;
			auto y02 = Y2;
			for (;;){
				for(;;)
				{
					e21 = 2 * err1 + 0.5f;
					if(e21 >= dy1){ err1 += dy1; x01 += sx1; }
					if(e21 <= dx1){	err1 += dx1; break; }
				}
				for(;;)
				{
					e22 = 2 * err2 + 0.5f;
					if(e22 >= dy2){ err2 += dy2; x02 += sx2; }
					if(e22 <= dx2){ err2 += dx2; break; }
				}
				hline(x01, x02, y01, iI, f);
				y01 += sy1;
				y02 += sy2;
				if ((int)y01 <= (int)ylim) break;
			}
		};

		auto xx1 = x1;
		auto yy1 = y1;
		auto xx2 = x2;
		auto yy2 = y2;
		if( x2 < x1 ){ std::swap(xx1, xx2); std::swap(yy1, yy2); }
		auto iI = isInside(x0, y0, xx1, yy1, xx2, yy2);

		auto sx = (x0+x1+x2)/(T)3;
		auto sy = (y0+y1+y2)/(T)3;

		auto r = std::max(x2-x0, y2-y0);
		const T scale = 1.0 + 10.0 / r;
		auto ex0 = (x0 - sx) * scale + sx;
		auto ex1 = (x1 - sx) * scale + sx;
		auto ex2 = (x2 - sx) * scale + sx;

		auto ey0 = (y0 - sy) * scale + sy;
		auto ey1 = (y1 - sy) * scale + sy;
		auto ey2 = (y2 - sy) * scale + sy;

		/*auto wh = [](auto ){ return bgr8(255, 255, 255); };
		line(ex0, ey0, ex1, ey1, wh);
		line(ex1, ey1, ex2, ey2, wh);
		line(ex2, ey2, ex0, ey0, wh);*/

		if((int)y1 == (int)y2)
		{
			fillBottomFlatTriangle(ex0, ey0, ex1, ey1, ex2, ey2, iI);
		}
		else if((int)y0 == (int)y1)
		{
			fillTopFlatTriangle(ex0, ey0, ex1, ey1, ex2, ey2, iI);
		} 
		else
		{
			fillBottomFlatTriangle(ex0, ey0, ex1, ey1, ex2, ey2, iI);
			fillTopFlatTriangle   (ex0, ey0, ex1, ey1, ex2, ey2, iI);
		}
	}

	template<typename F>
	void ellipse(int xm, int ym, int a, int b, F&& f)
	{
		long x = -a, y = 0; /* II. quadrant from bottom left to top right */
		long e2 = b, dx = (1+2*x)*e2*e2; /* error increment */
		long dy = x*x, err = dx+dy; /* error of 1.step */
		do {
			backbuffer(xm-x, ym+y) = f(backbuffer(xm-x, ym+y));//setPixel(xm-x, ym+y); /* I. Quadrant */
			backbuffer(xm+x, ym+y) = f(backbuffer(xm+x, ym+y));//setPixel(xm+x, ym+y); /* II. Quadrant */
			backbuffer(xm+x, ym-y) = f(backbuffer(xm+x, ym-y));//setPixel(xm+x, ym-y); /* III. Quadrant */
			backbuffer(xm-x, ym-y) = f(backbuffer(xm-x, ym-y));//setPixel(xm-x, ym-y); /* IV. Quadrant */
			e2 = 2*err;
			if (e2 >= dx) { x++; err += dx += 2*(long)b*b; } /* x step */
			if (e2 <= dy) { y++; err += dy += 2*(long)a*a; } /* y step */
		} while (x <= 0);
		while (y++ < b) { /* to early stop for flat ellipses with a=1, */
			backbuffer(xm, ym+y) = f(backbuffer(xm, ym+y));//setPixel(xm, ym+y); /* -> finish tip of ellipse */
			backbuffer(xm, ym+y) = f(backbuffer(xm, ym+y));//setPixel(xm, ym-y);
		}
	}
};

struct MainWindow
{
	PlatformWindowData	window;
	SoftwareRenderer	renderer;

#ifdef _WIN32
	HDC					hdc;
	HBITMAP				bmp;
	HGDIOBJ				oldbmp;
#else
	GC					gc;
	Pixmap				bmp;
#endif

	std::function<void(void)> onAppStep, onAppExit;
	std::function<void(int, int, StateChange)> onAppResize;
	std::function<void(SoftwareRenderer&)> onAppRender; 
	MainWindow():onAppStep{[]{}}, onAppExit{[]{}}, onAppResize{[](int, int, StateChange){}}, onAppRender{[](SoftwareRenderer&){}}
	{
#ifdef _WIN32
		hdc = 0; bmp = 0; oldbmp = 0;
#else
		bmp = 0;
#endif
	}

	auto width() const { return window.size.w; }
	auto height() const { return window.size.h; }

	template<typename FInit>
	bool open(std::wstring const& title, Pos2D pos_, Size2D size_, bool Decorated_, /*bool FullScreen_, */FInit&& finit)
	{
		using namespace MainWindowDetails;
		if( !window.open(title, pos_, size_, Decorated_/*, FullScreen_*/) ){ return false; }

		relay.onRender = [&]{ this->onRender(); };
		relay.onExit   = [&]{ this->onExit(); };
		relay.onResize = [&](int w, int h, StateChange sc){ this->onResize(w, h, sc); };

		renderer.init(width(), height());
		allocate_buffers();
#ifdef _WIN32
#else
		gc = XCreateGC(window.display, window.handle, 0, 0);
#endif
		onResize(width(), height(), StateChange::Resized);

		if( !finit() ){ return false; }
		window.show();
		window.loop(onAppStep);
		renderer.close();
		return window.close();
	}

	void quit(){ window.quit(); }

	template<typename F> void   exitHandler(F&& f){ onAppExit   = std::forward<F>(f); }
	template<typename F> void   idleHandler(F&& f){ onAppStep   = std::forward<F>(f); }
	template<typename F> void renderHandler(F&& f){ onAppRender = std::forward<F>(f); }
	template<typename F> void resizeHandler(F&& f){ onAppResize = std::forward<F>(f); }
	template<typename F> void  mouseHandler(F&& f){ MainWindowDetails::relay.onMouseEvent = std::forward<F>(f); }

	void onRender()
	{
		//printf("OnRender\n");
		onAppRender(renderer);

#ifdef _WIN32
		PAINTSTRUCT ps;
		auto paintdc = BeginPaint(window.handle, &ps);
		
		HDC     tmpdc     = CreateCompatibleDC(hdc);
		HBITMAP tmpbmp    = CreateBitmap(width(), height(), 1, 32, renderer.backbuffer.data.data());
		HGDIOBJ oldtmpbmp = SelectObject(tmpdc, tmpbmp);
		
		BitBlt(paintdc, 0, 0, width(), height(), tmpdc, 0, 0, SRCCOPY);

		SelectObject(tmpdc, oldtmpbmp);
		DeleteObject(tmpbmp);
		DeleteDC(tmpdc);

		EndPaint(window.handle, &ps);
		//ValidateRect(window.handle, NULL);
#else
		XImage* image = XCreateImage(window.display, window.visual, window.depth, ZPixmap, 0, (char*)renderer.backbuffer.data.data(), width(), height(), 32, 0);
		XPutImage(window.display, bmp, gc, image, 0, 0, 0, 0, width(), height());
		XFree(image);
		XCopyArea(window.display, bmp, window.handle, gc, 0, 0, width(), height(), 0, 0);
		XFlush(window.display);
#endif
	}

	void allocate_buffers()
	{
#ifdef _WIN32
		auto dcw = GetDC(window.handle);
		hdc    = CreateCompatibleDC(dcw);
		bmp    = CreateCompatibleBitmap(dcw, width(), height());
		oldbmp = SelectObject(hdc, bmp);
		ReleaseDC(window.handle, dcw);
#else
		bmp = XCreatePixmap(window.display, window.handle, width(), height(), window.depth);
#endif
	}

	void free_buffers()
	{
#ifdef _WIN32
		if(hdc)
		{
			SelectObject(hdc, oldbmp);
			DeleteObject(bmp);
			DeleteDC(hdc);
		}
#else
		if(bmp){ XFreePixmap(window.display, bmp); }
#endif
	}

	void onResize(int w, int h, StateChange sc)
	{
		//printf("resize\n");
		bool m = (w == width()) && (h == height());
		if(!m && sc != StateChange::Minimized)
		{
			printf("realloc buffers\n");
			free_buffers();
			renderer.resize(w, h);
			window.size.w = w; window.size.h = h;
			allocate_buffers();
			
		}
		onAppResize(w, h, sc);
		if(!m)
		{
			//printf("stepping and redrawing\n");
			//onAppStep();
			//window.redraw();
		}
	}

	void onExit()
	{
		onAppExit();
		quit();
	}
};