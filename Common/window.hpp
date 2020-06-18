#pragma once
#include "arch.h"
#include <dwmapi.h>
#include <errhandlingapi.h>
#include <optional>
#include <utility>
#include <windef.h>
#include <winerror.h>
#include <winuser.h>

#include "util/null_terminated_string_view.hpp"

#ifdef _TRANSLUCENTTB_EXE
#include <filesystem>
#include <string>

#include "../TranslucentTB/windows/windowclass.hpp"
#include "../ProgramLog/error/win32.hpp"
#endif

class Window {
private:
	static constexpr bool is_exception_free_v =
#ifdef _TRANSLUCENTTB_EXE
		false;
#else
		true;
#endif

	inline static void handle_error([[maybe_unused]] std::wstring_view err, [[maybe_unused]] HRESULT hr = HRESULT_FROM_WIN32(GetLastError())) noexcept(is_exception_free_v)
	{
#ifdef _TRANSLUCENTTB_EXE
		HresultHandle(hr, spdlog::level::info, err);
#endif
	}

	template<DWMWINDOWATTRIBUTE attrib>
	struct attrib_return_type;

	template<>
	struct attrib_return_type<DWMWA_NCRENDERING_ENABLED> {
		using type = BOOL;
	};

	template<>
	struct attrib_return_type<DWMWA_CAPTION_BUTTON_BOUNDS> {
		using type = RECT;
	};

	template<>
	struct attrib_return_type<DWMWA_EXTENDED_FRAME_BOUNDS> {
		using type = RECT;
	};

	template<>
	struct attrib_return_type<DWMWA_CLOAKED> {
		using type = DWORD;
	};

	template<DWMWINDOWATTRIBUTE attrib>
	using attrib_return_t = typename attrib_return_type<attrib>::type;

	template<DWMWINDOWATTRIBUTE attrib>
	inline std::optional<attrib_return_t<attrib>> get_attribute() const noexcept(is_exception_free_v)
	{
		attrib_return_t<attrib> val;
		const HRESULT hr = DwmGetWindowAttribute(m_WindowHandle, attrib, &val, sizeof(val));
		if (SUCCEEDED(hr))
		{
			return val;
		}
		else
		{
			handle_error(L"Failed to get window attribute.", hr);
			return std::nullopt;
		}
	}

protected:
	HWND m_WindowHandle;

public:
	static constexpr HWND NullWindow = nullptr;
	inline static const HWND BroadcastWindow = HWND_BROADCAST;
	inline static const HWND MessageOnlyWindow = HWND_MESSAGE;

	class FindEnum;

	inline static Window Find(Util::null_terminated_wstring_view className = { }, Util::null_terminated_wstring_view windowName = { }, Window parent = Window::NullWindow, Window childAfter = Window::NullWindow) noexcept
	{
		return FindWindowEx(parent, childAfter, className.empty() ? nullptr : className.c_str(), windowName.empty() ? nullptr : windowName.c_str());
	}

	inline static Window Create(unsigned long dwExStyle, LPCWSTR winClass, HINSTANCE hInstance,
		Util::null_terminated_wstring_view windowName, unsigned long dwStyle, int x = CW_USEDEFAULT, int y = CW_USEDEFAULT,
		int nWidth = CW_USEDEFAULT, int nHeight = CW_USEDEFAULT, Window parent = Window::NullWindow,
		HMENU hMenu = nullptr, void *lpParam = nullptr) noexcept
	{
		return CreateWindowEx(dwExStyle, winClass, windowName.c_str(), dwStyle, x, y, nWidth, nHeight,
			parent, hMenu, hInstance, lpParam);
	}

#ifdef _TRANSLUCENTTB_EXE
	inline static Window Create(unsigned long dwExStyle, const WindowClass &winClass,
		Util::null_terminated_wstring_view windowName, unsigned long dwStyle, int x = CW_USEDEFAULT, int y = CW_USEDEFAULT,
		int nWidth = CW_USEDEFAULT, int nHeight = CW_USEDEFAULT, Window parent = Window::NullWindow,
		HMENU hMenu = nullptr, void *lpParam = nullptr) noexcept
	{
		return Create(dwExStyle, winClass.atom(), winClass.hinstance(), windowName, dwStyle, x, y, nWidth, nHeight, parent, hMenu, lpParam);
	}
#endif

	inline static std::optional<UINT> RegisterMessage(Util::null_terminated_wstring_view message) noexcept(is_exception_free_v)
	{
		const UINT msg = RegisterWindowMessage(message.c_str());
		if (msg)
		{
			return msg;
		}
		else
		{
			handle_error(L"Failed to register window message.");
			return std::nullopt;
		}
	}

	inline static Window ForegroundWindow() noexcept
	{
		return GetForegroundWindow();
	}

	inline static Window DesktopWindow() noexcept
	{
		return GetDesktopWindow();
	}

	inline static Window ShellWindow() noexcept
	{
		return GetShellWindow();
	}

	constexpr Window(HWND handle = Window::NullWindow) noexcept : m_WindowHandle(handle) { }

#ifdef _TRANSLUCENTTB_EXE
	std::optional<std::wstring> title() const;

	std::optional<std::wstring> classname() const;

	std::optional<std::filesystem::path> file() const;

	std::optional<bool> on_current_desktop() const;

	// TODO: this should not return true for the cortana window (and make sure it doesn't for start/action center/tray overflow, tray icons)
	bool is_user_window() const;
#endif

	inline bool valid() const noexcept
	{
		return IsWindow(m_WindowHandle);
	}

	inline bool maximised() const noexcept
	{
		return IsZoomed(m_WindowHandle);
	}

	inline bool minimised() const noexcept
	{
		return IsIconic(m_WindowHandle);
	}

	inline bool show(int state = SW_SHOW) noexcept
	{
		return ShowWindow(m_WindowHandle, state);
	}

	inline bool visible() const noexcept
	{
		return IsWindowVisible(m_WindowHandle);
	}

	inline bool cloaked() const noexcept(is_exception_free_v)
	{
		const auto attr = get_attribute<DWMWA_CLOAKED>();
		return attr && *attr;
	}

	inline HMONITOR monitor() const noexcept
	{
		return MonitorFromWindow(m_WindowHandle, MONITOR_DEFAULTTONULL);
	}

	inline Window get(UINT cmd) const noexcept
	{
		return GetWindow(m_WindowHandle, cmd);
	}

	inline Window ancestor(UINT flags) const noexcept
	{
		return GetAncestor(m_WindowHandle, flags);
	}

	inline HANDLE prop(Util::null_terminated_wstring_view name) const noexcept
	{
		return GetProp(m_WindowHandle, name.c_str());
	}

	inline std::optional<LONG_PTR> long_ptr(int index) const noexcept(is_exception_free_v)
	{
		SetLastError(NO_ERROR);
		const LONG_PTR val = GetWindowLongPtr(m_WindowHandle, index);
		if (val)
		{
			return val;
		}
		else
		{
			if (const DWORD lastErr = GetLastError(); lastErr == NO_ERROR)
			{
				return val;
			}
			else
			{
				handle_error(L"Failed to get window pointer", HRESULT_FROM_WIN32(lastErr));
				return std::nullopt;
			}
		}
	}

	inline DWORD thread_id() const noexcept
	{
		return GetWindowThreadProcessId(m_WindowHandle, nullptr);
	}

	inline DWORD process_id() const noexcept
	{
		DWORD pid { };
		GetWindowThreadProcessId(m_WindowHandle, &pid);
		return pid;
	}

	inline std::pair<DWORD, DWORD> thread_process_id() const noexcept
	{
		std::pair<DWORD, DWORD> pair { };
		pair.first = GetWindowThreadProcessId(m_WindowHandle, &pair.second);
		return pair;
	}

	inline std::optional<RECT> rect() const noexcept(is_exception_free_v)
	{
		RECT result { };
		if (GetWindowRect(m_WindowHandle, &result))
		{
			return result;
		}
		else
		{
			handle_error(L"Failed to get window region.");
			return std::nullopt;
		}
	}

	inline std::optional<RECT> client_rect() const noexcept(is_exception_free_v)
	{
		RECT result { };
		if (GetClientRect(m_WindowHandle, &result))
		{
			return result;
		}
		else
		{
			handle_error(L"Failed to get client region.");
			return std::nullopt;
		}
	}

	inline std::optional<TITLEBARINFO> titlebar_info() const noexcept(is_exception_free_v)
	{
		TITLEBARINFO info = { sizeof(info) };
		if (GetTitleBarInfo(m_WindowHandle, &info))
		{
			return info;
		}
		else
		{
			handle_error(L"Failed to get titlebar info.");
			return std::nullopt;
		}
	}

	inline LRESULT send_message(unsigned int message, WPARAM wparam = 0, LPARAM lparam = 0) const noexcept
	{
		return SendMessage(m_WindowHandle, message, wparam, lparam);
	}

	inline bool post_message(unsigned int message, WPARAM wparam = 0, LPARAM lparam = 0) const noexcept
	{
		return PostMessage(m_WindowHandle, message, wparam, lparam);
	}

	inline Window find_child(Util::null_terminated_wstring_view className = { }, Util::null_terminated_wstring_view windowName = { }, Window childAfter = Window::NullWindow) const noexcept
	{
		return Find(className, windowName, m_WindowHandle, childAfter);
	}

	constexpr FindEnum find_childs(Util::null_terminated_wstring_view className = { }, Util::null_terminated_wstring_view windowName = { }) const noexcept;

	constexpr HWND handle() const noexcept
	{
		return m_WindowHandle;
	}

	constexpr HWND *put() noexcept
	{
		return &m_WindowHandle;
	}

	constexpr operator HWND() const noexcept
	{
		return m_WindowHandle;
	}

	inline explicit operator bool() const noexcept
	{
		return valid();
	}

	constexpr bool operator ==(Window right) const noexcept
	{
		return m_WindowHandle == right.m_WindowHandle;
	}

	constexpr bool operator ==(HWND right) const noexcept
	{
		return m_WindowHandle == right;
	}

	friend struct std::hash<Window>;
};

// Specialize std::hash to allow the use of Window as unordered_map and unordered_set key.
template<>
struct std::hash<Window> {
	inline std::size_t operator()(Window k) const noexcept
	{
		static constexpr std::hash<HWND> hasher;
		return hasher(k.m_WindowHandle);
	}
};

// Iterator class for FindEnum
class FindWindowIterator {
private:
	Util::null_terminated_wstring_view m_class, m_name;
	Window m_parent, m_currentWindow;

	inline void MoveNext() noexcept
	{
		m_currentWindow = m_parent.find_child(m_class, m_name, m_currentWindow);
	}

	constexpr FindWindowIterator() noexcept { }

	inline FindWindowIterator(Util::null_terminated_wstring_view className, Util::null_terminated_wstring_view windowName, Window parent) noexcept :
		m_class(className),
		m_name(windowName),
		m_parent(parent)
	{
		MoveNext();
	}

	friend class Window::FindEnum;

public:
	inline FindWindowIterator &operator ++() noexcept
	{
		MoveNext();
		return *this;
	}

	constexpr bool operator ==(const FindWindowIterator &right) const noexcept
	{
		return m_currentWindow == right.m_currentWindow;
	}

	constexpr bool operator !=(const FindWindowIterator &right) const noexcept
	{
		return !operator==(right);
	}

	constexpr Window operator *() const noexcept
	{
		return m_currentWindow;
	}
};

class Window::FindEnum {
private:
	Util::null_terminated_wstring_view m_class, m_name;
	Window m_parent;
public:
	constexpr FindEnum(Util::null_terminated_wstring_view className = { }, Util::null_terminated_wstring_view windowName = { }, Window parent = Window::NullWindow) noexcept :
		m_class(className),
		m_name(windowName),
		m_parent(parent)
	{ }

	inline FindWindowIterator begin() const noexcept
	{
		return { m_class, m_name, m_parent };
	}

	constexpr FindWindowIterator end() const noexcept
	{
		return { };
	}
};

constexpr Window::FindEnum Window::find_childs(Util::null_terminated_wstring_view className, Util::null_terminated_wstring_view windowName) const noexcept
{
	return FindEnum(className, windowName, m_WindowHandle);
}
