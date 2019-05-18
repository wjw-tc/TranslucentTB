#include "messagewindow.hpp"
#include <algorithm>

#include "../ttberror.hpp"
#include "undoc/uxtheme.hpp"

static const auto uxtheme = LoadLibrary(UXTHEME_DLL);
static const auto darkmode = reinterpret_cast<PFN_ALLOW_DARK_MODE_FOR_WINDOW>(GetProcAddress(uxtheme, ADMFW_ORDINAL));

LRESULT MessageWindow::WindowProcedure(Window window, unsigned int uMsg, WPARAM wParam, LPARAM lParam)
{
	const auto &callbackMap = m_CallbackMap[uMsg];
	if (!callbackMap.empty())
	{
		long result = 0;
		for (const auto &[_, callback] : callbackMap)
		{
			result = std::max(callback(wParam, lParam), result);
		}
		return result;
	}

	return DefWindowProc(window, uMsg, wParam, lParam);
}

void MessageWindow::RunMessageLoop()
{
	MSG msg;
	BOOL ret;
	while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0)
	{
		if (ret != -1)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			LastErrorHandle(Error::Level::Fatal, L"GetMessage failed!");
		}
	}
}

MessageWindow::MessageWindow(const std::wstring &className, const std::wstring &windowName, HINSTANCE hInstance, Window parent, const wchar_t *iconResource) :
	m_WindowClass(
		std::bind(&MessageWindow::WindowProcedure, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4),
		className,
		iconResource,
		0,
		hInstance
	)
{
	m_WindowHandle = Window::Create(0, m_WindowClass, windowName, 0, 0, 0, 0, 0, parent, 0, hInstance, this);

	if (!m_WindowHandle)
	{
		LastErrorHandle(Error::Level::Fatal, L"Failed to create message window!");
	}

	darkmode(m_WindowHandle, true);
	BOOL dark = TRUE;
	DwmSetWindowAttribute(m_WindowHandle, 19, &dark, sizeof(dark));

	SetWindowTheme(m_WindowHandle, DARKMODE_THEME, nullptr);
	send_message(WM_THEMECHANGED, 0, 0);

	RegisterCallback(WM_DPICHANGED, [this, iconResource](...)
	{
		m_WindowClass.ChangeIcon(*this, iconResource);

		return 0;
	});
}

MessageWindow::~MessageWindow()
{
	if (!DestroyWindow(m_WindowHandle))
	{
		LastErrorHandle(Error::Level::Log, L"Failed to destroy message window!");
	}
}