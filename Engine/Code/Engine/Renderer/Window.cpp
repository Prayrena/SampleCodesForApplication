#include "Engine/Renderer/Window.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/core/ErrorWarningAssert.hpp"
#include "Engine/core/EventSystem.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/Time.hpp"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

Window* Window :: s_theWindow = nullptr;// Actual definition of the global advertised in .hppFile

extern InputSystem* g_theInput;

//-----------------------------------------------------------------------------------------------
// Handles Windows (Win32) messages/events; i.e. the OS is trying to tell us something happened.
// This function is called back by Windows whenever we tell it to (by calling DispatchMessage).
//
// #SD1ToDo: We will move this function to a more appropriate place (Engine/Renderer/Window.cpp) later on...
//windows will notify you message through this function(say a key is pressed down) 
LRESULT CALLBACK WindowsMessageHandlingProcedure(HWND windowHandle, UINT wmMessageCode, WPARAM wParam, LPARAM lParam)
{
	// Get the window (assume we only have one for now)
	Window* window = Window::GetMainWindowPtr();
	GUARANTEE_OR_DIE(window != nullptr, "window was null");

	// Ask the window for a pointer to the InputSystem it was created with (in its Input SystemConfig)
	InputSystem* input = window->GetConfig().m_inputSystem;
	GUARANTEE_OR_DIE(input != nullptr, "Window's InputSystem pointer was null");

	switch (wmMessageCode)
	{
		// App close requested via "X" button, or right-click "Close Window" on task bar, or "Close" from system menu, or Alt-F4
		// it triggers when player closes this program (task manager will kill the program rootly)
		case WM_CLOSE:
		{
			FireEvent("quit");
			return 0; // "Consumes" this message (tells Windows "okay, we handled it")
		}

		// Raw physical keyboard "key-was-just-depressed" event (case-insensitive, not translated)
		// user could get multiple key down with only one keyup
		// case WM_KEYDOWN:
		// {
		// 	// use this line to check which key 
		// 	// char unicode characters, use''
		// 	// string use " "
		// 	unsigned char asKey = (unsigned char)wParam;
		// 	//std :: string testing = "q";
		// 	if ( input )
		// 	{
		// 		input->HandleKeyPressed(asKey);
		// 		return 0; // "Consumes" this message (tells Windows "okay, we handled it")
		// 	}
		// 	break;
		// }

		case WM_CHAR: // will only contains the character information but not the arrow input
		{
			EventArgs args;
			args.SetValue("TextInput", Stringf("%d", (unsigned char)wParam));
			FireEvent("CharInput", args);
			return 0;
		}

		case WM_KEYDOWN:
		{
			EventArgs args;
			args.SetValue("KeyCode", Stringf("%d", (unsigned char)wParam));
			FireEvent("KeyPressed", args);
			return 0;
		}

		case WM_KEYUP:
		{
			EventArgs args;
			args.SetValue("KeyCode", Stringf("%d", (unsigned char)wParam));
			FireEvent("KeyReleased", args);
			return 0;
		}


		// Raw physical keyboard "key-was-just-released" event (case-insensitive, not translated)
		//case WM_KEYUP:
		//{
		//	unsigned char asKey = (unsigned char)wParam;
		//	if (input)
		//	{
		//		input->HandleKeyReleased(asKey);
		//		return 0; // "Consumes" this message (tells Windows "okay, we handled it")
		//	}
		//	// #SD1ToDo: Tell the App (or InputSystem later) about this key-released event...
		//	break;
		//}

		// case WM_KEYUP:
		// {
		// 	EventArgs args;
		// 	args.SetValue("KeyCodeRelease", Stringf("%d", (unsigned char)wParam));
		// 	FireEvent("KeyCodeRelease, args");
		// 	return 0;
		// }

		// treat this special mouse button windows message as if it were an ordinary key down for us
		case WM_LBUTTONDOWN:
		{
			unsigned char keyCode = KEYCODE_LEFT_MOUSE;
			if (input)
			{
				input->HandleKeyPressed(keyCode);
				return 0;// "consumes" this message(tell windows "Okay, we handled it")
			}
			break;
		}
		// treat this special mouse button windows message as if it were an ordinary key down for us
		case WM_LBUTTONUP:
		{
			unsigned char keyCode = KEYCODE_LEFT_MOUSE;
			if (input)
			{
				input->HandleKeyReleased(keyCode);
				return 0;// "consumes" this message(tell windows "Okay, we handled it")
			}
			break;
		}
		case WM_RBUTTONDOWN:
		{
			unsigned char keyCode = KEYCODE_RIGHT_MOUSE;
			if (input)
			{
				input->HandleKeyPressed(keyCode);
				return 0;// "consumes" this message(tell windows "Okay, we handled it")
			}
			break;
		}
		// treat this special mouse button windows message as if it were an ordinary key down for us
		case WM_RBUTTONUP:
		{
			unsigned char keyCode = KEYCODE_RIGHT_MOUSE;
			if (input)
			{
				input->HandleKeyReleased(keyCode);
				return 0;// "consumes" this message(tell windows "Okay, we handled it")
			}
			break;
		}
	}

	// Send back to Windows any unhandled/unconsumed messages we want other apps to see (e.g. play/pause in music apps, etc.)
	return DefWindowProc(windowHandle, wmMessageCode, wParam, lParam);
}


Window::Window(WindowConfig const& config)
	: m_config( config )
{
	s_theWindow = this;
}

Window::~Window()
{

}

void Window::Startup()
{
	CreateOSWindow();
	// Clock::s_theSystemClock = *(new Clock());
}

void Window::CreateOSWindow()
{
	HMODULE applicationInstanceHandle = GetModuleHandle(NULL);
	// Define a window style/class
	WNDCLASSEX windowClassDescription;
	memset(&windowClassDescription, 0, sizeof(windowClassDescription));
	windowClassDescription.cbSize = sizeof(windowClassDescription);
	windowClassDescription.style = CS_OWNDC; // Redraw on move, request own Display Context
	windowClassDescription.lpfnWndProc = static_cast<WNDPROC>(WindowsMessageHandlingProcedure); // Register our Windows message-handling function
	windowClassDescription.hInstance = applicationInstanceHandle;
	windowClassDescription.hIcon = NULL;
	windowClassDescription.hCursor = NULL;
	windowClassDescription.lpszClassName = TEXT("Simple Window Class");
	RegisterClassEx(&windowClassDescription);

	// #SD1ToDo: Add support for fullScreen mode (requires different window style flags than windowed mode)
	const DWORD windowStyleFlags = WS_CAPTION | WS_BORDER | WS_THICKFRAME | WS_SYSMENU | WS_OVERLAPPED;
	const DWORD windowStyleExFlags = WS_EX_APPWINDOW;

	// Get desktop rect, dimensions, aspect
	RECT desktopRect;
	HWND desktopWindowHandle = GetDesktopWindow();
	GetClientRect(desktopWindowHandle, &desktopRect);
	float desktopWidth = (float)(desktopRect.right - desktopRect.left);
	float desktopHeight = (float)(desktopRect.bottom - desktopRect.top);
	float desktopAspect = desktopWidth / desktopHeight;

	// Calculate maximum client size (as some % of desktop size)
	float clientWidth = desktopWidth * m_config.maxClientFractionOfDesktop;
	float clientHeight = desktopHeight * m_config.maxClientFractionOfDesktop;
	if (m_config.m_aspectRatio > desktopAspect)
	{
		// Client window has a wider aspect than desktop; shrink client height to match its width
		clientHeight = clientWidth / m_config.m_aspectRatio;
	}
	else
	{
		// Client window has a taller aspect than desktop; shrink client width to match its height
		clientWidth = clientHeight * m_config.m_aspectRatio;
	}

	// Calculate client rect bounds by centering the client area
	float clientMarginX = 0.5f * (desktopWidth - clientWidth);
	float clientMarginY = 0.5f * (desktopHeight - clientHeight);
	RECT clientRect;
	clientRect.left = (int)clientMarginX;
	clientRect.right = clientRect.left + (int)clientWidth;
	clientRect.top = (int)clientMarginY;
	clientRect.bottom = clientRect.top + (int)clientHeight;

	// Calculate the outer dimensions of the physical window, including frame et. al.
	RECT windowRect = clientRect;
	::AdjustWindowRectEx(&windowRect, windowStyleFlags, FALSE, windowStyleExFlags);

	WCHAR windowTitle[1024];
	::MultiByteToWideChar(GetACP(), 0, m_config.m_windowTitle.c_str(), -1, windowTitle, sizeof(windowTitle) / sizeof(windowTitle[0]));
	m_hwnd = static_cast<HWND>( CreateWindowEx(
		windowStyleExFlags,
		windowClassDescription.lpszClassName,
		windowTitle,
		windowStyleFlags,
		windowRect.left,
		windowRect.top,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL,
		NULL,
		applicationInstanceHandle, 
		NULL) );

	HWND hwnd = static_cast<HWND>(m_hwnd);
	m_displayContext = ::GetDC(hwnd);

	::ShowWindow(hwnd, SW_SHOW);
	::SetForegroundWindow(hwnd);
	::SetFocus(hwnd);

	HCURSOR cursor = ::LoadCursor(NULL, IDC_ARROW);
	::SetCursor(cursor);
}

void Window::BeginFrame()
{
	RunMessagePump();
}

void Window::EndFrame()
{
	// Sleep( 1 );

	// "present" the backBuffer by swapping the front (visible) and back (working) screen buffers
	// SwapBuffers(reinterpret_cast<HDC>(m_displayContext));

}

void Window::ShutDown()
{
	return;
}

WindowConfig const& Window::GetConfig() const
{
	return m_config;
}

float Window::GetAspect() const
{
	return m_config.m_aspectRatio;
}

IntVec2 Window::GetClientDimensions() const
{
	RECT clientRec;
	GetClientRect((HWND)m_hwnd, &clientRec);
	IntVec2 clientDimensions;
	clientDimensions.x = (int)(clientRec.right - clientRec.left);
	clientDimensions.y = (int)(clientRec.bottom - clientRec.top);

	return clientDimensions;
}

void* Window::GetDeviceContext() const
{
	return m_displayContext;
}

Window* Window::GetMainWindowPtr()
{
	return s_theWindow;
}


void* Window::GetHwnd() const
{
	return m_hwnd;
}



Vec2 Window::GetNormalizedCursorPos() const
{
	HWND windowHandle = HWND(m_hwnd);
	POINT cursorCoords;
	RECT clientRect;
	::GetCursorPos(&cursorCoords); // in screen coordinates, (0,0) top-left
	::ScreenToClient(windowHandle, &cursorCoords); // relative to the window interior
	::GetClientRect(windowHandle, &clientRect); // size of window interior(0,0 to width, height)
	float cursorX = float(cursorCoords.x) / float(clientRect.right); // normalized x position
	float cursorY = float(cursorCoords.y) / float(clientRect.bottom); // normalized y position
	return Vec2(cursorX, 1.f - cursorY);// we want (0,0) in the bottom-left
}

void Window::RunMessagePump()
{
	MSG queuedMessage;
	for (;; )
	{
		const BOOL wasMessagePresent = PeekMessage(&queuedMessage, NULL, 0, 0, PM_REMOVE);
		if (!wasMessagePresent)
		{
			break;
		}

		TranslateMessage(&queuedMessage);
		DispatchMessage(&queuedMessage); // This tells Windows to call our "WindowsMessageHandlingProcedure" (a.k.a. "WinProc") function
	}
}
