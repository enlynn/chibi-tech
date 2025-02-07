#include "../window.h"

#include <std/win32/win32.h>
#include <std/str.h>
#include <std/mem.h>

typedef struct win32_window_t
{
	HANDLE handle;
	bool   is_live;
	//todo: other state
} win32_window_t;

const char* DEFAULT_CLASS_NAME = "CHIBI_WINDOW_CLASS";

fn_internal LRESULT CALLBACK
win32_window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	win32_window_t *window = NULL;
    if (message == WM_NCCREATE)
    {
        LPCREATESTRUCT create_struct = (LPCREATESTRUCT)lparam;
        window = (win32_window_t*)create_struct->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)window);
    }
    else
    {
        window = (win32_window_t*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    }

    if (window)
    {
    	switch (message)
        {
        	case WM_CLOSE:
            case WM_DESTROY:
            {
                window->is_live = false;
            } break;
        }
    }

	return DefWindowProcW(hwnd, message, wparam, lparam);
}

fn_internal void
win32_register_window_class(const char* class_name)
{
	wchar_t class_name_w[1024];
	utf8_to_utf16(class_name, str_len(class_name), class_name_w, 1024);

	WNDCLASSEXW window_class = {};
	ZeroMemory(&window_class, sizeof(WNDCLASSEX));

	window_class.cbSize        = sizeof(WNDCLASSEX);
	window_class.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	window_class.lpfnWndProc   = win32_window_proc;
	window_class.hInstance     = GetModuleHandle(NULL);
	window_class.hCursor       = (HICON)LoadImage(0, IDC_ARROW, IMAGE_CURSOR,     0, 0, LR_DEFAULTSIZE | LR_SHARED);
	window_class.hIcon         = (HICON)LoadImage(0, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
	window_class.lpszClassName = class_name_w;

	RegisterClassExW(&window_class);
}

fn_internal void
win32_create_window(
	win32_window_t *window,
	const char*     class_name,
	const char*     window_name,
	u32             requested_width,
	u32             requested_height,
	void*           user_data)
{
	wchar_t class_name_w[1024];
	utf8_to_utf16(class_name, str_len(class_name), class_name_w, 1024);

	wchar_t window_name_w[1024];
	utf8_to_utf16(window_name, str_len(window_name), window_name_w, 1024);

	int screen_width  = GetSystemMetrics(SM_CXSCREEN);
	int screen_height = GetSystemMetrics(SM_CYSCREEN);

	RECT rect = { 0, 0, (LONG)requested_width, (LONG)requested_height };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

	LONG width  = rect.right  - rect.left;
	LONG height = rect.bottom - rect.top;

	LONG window_x = (screen_width  - width)  / 2;
	LONG window_y = (screen_height - height) / 2;

	if (window_x < 0) window_x = CW_USEDEFAULT;
	if (window_y < 0) window_y = CW_USEDEFAULT;
	if (width < 0)    width    = CW_USEDEFAULT;
	if (height < 0)   height   = CW_USEDEFAULT;

	DWORD style    = WS_OVERLAPPEDWINDOW;
	DWORD ex_style = WS_EX_OVERLAPPEDWINDOW;

	window->handle = CreateWindowExW(
		ex_style, class_name_w, window_name_w,      style,
		window_x, window_y,     width,              height,
		0,        0,            GetModuleHandle(0), user_data);
}

fn_export void
os_window_create(size_t width, size_t height, const char* window_title, struct os_window_o *window_i, size_t *window_size)
{
	*window_size = sizeof(win32_window_t);
	if (!window_i) return;

	win32_window_t *result = (win32_window_t*)window_i;
	win32_register_window_class(DEFAULT_CLASS_NAME);
	win32_create_window(result, DEFAULT_CLASS_NAME, window_title, width, height, result);

	result->is_live = true;
}

fn_export void
os_window_destroy(struct os_window_o *window_i)
{
	win32_window_t *window = (win32_window_t*)window_i;
	DestroyWindow(window->handle);

	ZERO_STRUCT(window);
}

fn_export void
os_window_show(struct os_window_o *window_i, os_window_show_flags_t flags)
{
	win32_window_t *window = (win32_window_t*)window_i;

	int show_cmd = 0;
	switch (flags)
	{
		case WINDOW_SHOW_FLAG_NORMAL:    show_cmd = SW_SHOWNORMAL;    break;
		case WINDOW_SHOW_FLAG_HIDE:      show_cmd = SW_HIDE;          break;
		case WINDOW_SHOW_FLAG_MINIMIZED: show_cmd = SW_SHOWMINIMIZED; break;
		case WINDOW_SHOW_FLAG_MAXIMIZED: show_cmd = SW_SHOWMAXIMIZED; break;
	}

	ShowWindow(window->handle, show_cmd);
}


fn_export void
os_window_get_dims(struct os_window_o *window_i, u32 *width, u32 *height)
{
	win32_window_t *window = (win32_window_t*)window_i;

	RECT rect;
    GetClientRect(window->handle, &rect);

    *width  = rect.right  - rect.left;
    *height = rect.bottom - rect.top;
}

fn_export void
os_window_process_pending_messages(struct os_window_o *window_i)
{
	win32_window_t *window = (win32_window_t*)window_i;

	MSG msg;
    while (window->is_live && PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        switch (msg.message)
        {
        	case WM_CLOSE:
            case WM_DESTROY:
            {
                window->is_live = false;
            } break;

            case WM_SYSKEYDOWN:
            case WM_KEYDOWN:
            {
            	if (msg.wParam == VK_ESCAPE) window->is_live = false;
            }
        }
    }
}

fn_export bool
os_window_is_valid(struct os_window_o *window_i)
{
	win32_window_t *window = (win32_window_t*)window_i;
	return window->is_live > 0;
}

fn_export struct os_window_surface
os_window_get_surface(struct os_window_o *self)
{
    win32_window_t *window = (win32_window_t*)self;

    struct os_window_surface surface = {};
    surface.type = OS_SURFACE_WIN32;
    surface.win32_module = GetModuleHandleW(NULL);
    surface.win32_window = window->handle;

    return surface;
}
