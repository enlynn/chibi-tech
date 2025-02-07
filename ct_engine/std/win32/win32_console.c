#include "win32.h"
#include "../types.h"
#include "../str.h"
#include "../log.h"
#include "../cassert.h"

#include <stdio.h>

typedef struct
{
    HANDLE handle;                         // Stream handle (STD_OUTPUT_HANDLE or STD_ERROR_HANDLE).
    bool   is_redirected;                   // True if redirected to file.
} win32_standard_stream_t;

#if 0 // From wincon.h, here for reference
#define FOREGROUND_BLUE            0x0001
#define FOREGROUND_GREEN           0x0002
#define FOREGROUND_RED             0x0004
#define FOREGROUND_INTENSITY       0x0008
#define BACKGROUND_BLUE            0x0010
#define BACKGROUND_GREEN           0x0020
#define BACKGROUND_RED             0x0040
#define BACKGROUND_INTENSITY       0x0080
#define COMMON_LVB_LEADING_BYTE    0x0100
#define COMMON_LVB_TRAILING_BYTE   0x0200
#define COMMON_LVB_GRID_HORIZONTAL 0x0400
#define COMMON_LVB_GRID_LVERTICAL  0x0800
#define COMMON_LVB_GRID_RVERTICAL  0x1000
#define COMMON_LVB_REVERSE_VIDEO   0x4000
#define COMMON_LVB_UNDERSCORE      0x8000
#endif

// Based on .NET ConsoleColor enumeration
// Foreground and Background colors are essentially the same bitfield, except the Background colors
// are left shifted 4 bits. win32_console_color can be composed by doing:
//     console_color = foreground | (background << 4)
typedef enum
{
    WIN32_CONSOLE_COLOR_BLACK        = 0,                                                    // color = black        | value = 0
    WIN32_CONSOLE_COLOR_DARK_BLUE    = FOREGROUND_BLUE,                                      // color = dark blue    | value = 1
    WIN32_CONSOLE_COLOR_DARK_GREEN   = FOREGROUND_GREEN,                                     // color = dark green   | value = 2
    WIN32_CONSOLE_COLOR_DARK_CYAN    = FOREGROUND_BLUE | FOREGROUND_GREEN,                   // color = dark cyan    | value = 3
    WIN32_CONSOLE_COLOR_DARK_RED     = FOREGROUND_RED,                                       // color = dark red     | value = 3
    WIN32_CONSOLE_COLOR_DARK_MAGENTA = FOREGROUND_BLUE | FOREGROUND_RED,                     // color = dark magenta | value = 5
    WIN32_CONSOLE_COLOR_DARK_YELLOW  = FOREGROUND_RED | FOREGROUND_GREEN,                    // color = dark yellow  | value = 6
    WIN32_CONSOLE_COLOR_GREY         = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED,  // color = grey         | value = 7
    WIN32_CONSOLE_COLOR_DARK_GREY    = FOREGROUND_INTENSITY,                                 // color = dark grey    | value = 8
    WIN32_CONSOLE_COLOR_BLUE         = FOREGROUND_INTENSITY | CONSOLE_COLOR_DARK_BLUE,       // color = blue         | value = 9
    WIN32_CONSOLE_COLOR_GREEN        = FOREGROUND_INTENSITY | CONSOLE_COLOR_DARK_GREEN,      // color = green        | value = 10
    WIN32_CONSOLE_COLOR_CYAN         = FOREGROUND_INTENSITY | CONSOLE_COLOR_DARK_CYAN,       // color = cyan         | value = 11
    WIN32_CONSOLE_COLOR_RED          = FOREGROUND_INTENSITY | CONSOLE_COLOR_DARK_RED,        // color = red          | value = 12
    WIN32_CONSOLE_COLOR_MAGENTA      = FOREGROUND_INTENSITY | CONSOLE_COLOR_DARK_MAGENTA,    // color = magenta      | value = 13
    WIN32_CONSOLE_COLOR_YELLOW       = FOREGROUND_INTENSITY | CONSOLE_COLOR_DARK_YELLOW,     // color = yellow       | value = 14
    WIN32_CONSOLE_COLOR_WHITE        = FOREGROUND_INTENSITY | CONSOLE_COLOR_GREY,            // color = white        | value = 15
} win32_console_color_t;

fn_internal win32_console_color_t
color_to_win32_color(console_color_t in_color)
{
    win32_console_color_t out_color = WIN32_CONSOLE_COLOR_BLACK;
    switch (in_color)
    {
        case CONSOLE_COLOR_DARK_BLUE:    out_color = WIN32_CONSOLE_COLOR_DARK_BLUE;    break;
        case CONSOLE_COLOR_DARK_GREEN:   out_color = WIN32_CONSOLE_COLOR_DARK_GREEN;   break;
        case CONSOLE_COLOR_DARK_CYAN:    out_color = WIN32_CONSOLE_COLOR_DARK_CYAN;    break;
        case CONSOLE_COLOR_DARK_RED:     out_color = WIN32_CONSOLE_COLOR_DARK_RED;     break;
        case CONSOLE_COLOR_DARK_MAGENTA: out_color = WIN32_CONSOLE_COLOR_DARK_MAGENTA; break;
        case CONSOLE_COLOR_DARK_YELLOW:  out_color = WIN32_CONSOLE_COLOR_DARK_YELLOW;  break;
        case CONSOLE_COLOR_GREY:         out_color = WIN32_CONSOLE_COLOR_GREY;         break;
        case CONSOLE_COLOR_DARK_GREY:    out_color = WIN32_CONSOLE_COLOR_DARK_GREY;    break;
        case CONSOLE_COLOR_BLUE:         out_color = WIN32_CONSOLE_COLOR_BLUE;         break;
        case CONSOLE_COLOR_GREEN:        out_color = WIN32_CONSOLE_COLOR_GREEN;        break;
        case CONSOLE_COLOR_CYAN:         out_color = WIN32_CONSOLE_COLOR_CYAN;         break;
        case CONSOLE_COLOR_RED:          out_color = WIN32_CONSOLE_COLOR_RED;          break;
        case CONSOLE_COLOR_MAGENTA:      out_color = WIN32_CONSOLE_COLOR_MAGENTA;      break;
        case CONSOLE_COLOR_YELLOW:       out_color = WIN32_CONSOLE_COLOR_YELLOW;       break;
        case CONSOLE_COLOR_WHITE:        out_color = WIN32_CONSOLE_COLOR_WHITE;        break;
        case CONSOLE_COLOR_COUNT:
        case CONSOLE_COLOR_BLACK:        out_color = WIN32_CONSOLE_COLOR_BLACK;        break;
    }
    return out_color;
}

fn_inline WORD
win32_compose_console_color(win32_console_color_t foreground, win32_console_color_t background)
{
    return (int)foreground | ((int)background << 4);
}

fn_inline WORD
win32_get_console_color(HANDLE console_handle)
{
    WORD result = win32_compose_console_color(WIN32_CONSOLE_COLOR_WHITE, WIN32_CONSOLE_COLOR_BLACK);

    CONSOLE_SCREEN_BUFFER_INFO console_info = {0};
    BOOL info_result = GetConsoleScreenBufferInfo(console_handle, &console_info);
    if (info_result > 0)
    {
        result = console_info.wAttributes;
    }

    return result;
}

fn_internal bool
win32_redirect_console_io()
{
    bool result = true;
    FILE* fp;

    // Redirect STDIN if the console has an input handle
    if (GetStdHandle(STD_INPUT_HANDLE) != INVALID_HANDLE_VALUE)
    {
        if (freopen_s(&fp, "CONIN$", "r", stdin) != 0)
        result = false;
    }
    else
    {
        setvbuf(stdin, NULL, _IONBF, 0);
    }

    // Redirect STDOUT if the console has an output handle
    if (GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE)
    {
        if (freopen_s(&fp, "CONOUT$", "w", stdout) != 0)
        result = false;
    }
    else
    {
        setvbuf(stdout, NULL, _IONBF, 0);
    }

    // Redirect STDERR if the console has an error handle
    if (GetStdHandle(STD_ERROR_HANDLE) != INVALID_HANDLE_VALUE)
    {
        if (freopen_s(&fp, "CONOUT$", "w", stderr) != 0)
        result = false;
    }
    else
    {
        setvbuf(stderr, NULL, _IONBF, 0);
    }

    return result;
}

// Sets up a standard stream (stdout or stderr).
fn_internal win32_standard_stream_t
win32_get_standard_stream(DWORD stream_type)
{
    win32_standard_stream_t result = {0};

    // If we don't have our own stream and can't find a parent console, allocate a new console.
    result.handle = GetStdHandle(stream_type);
    if (!result.handle || result.handle == INVALID_HANDLE_VALUE)
    {
        if (!AttachConsole(ATTACH_PARENT_PROCESS))
        {
            AllocConsole();
            bool redirect_result = win32_redirect_console_io();
            ASSERT(redirect_result);
        }

        result.handle = GetStdHandle(stream_type);
       ASSERT(result.handle != INVALID_HANDLE_VALUE);
    }

    // Check if the stream is redirected to a file. If it does, check if the file already exists.
    if (result.handle != INVALID_HANDLE_VALUE)
    {
        DWORD dummy;
        DWORD type = GetFileType(result.handle) & (~FILE_TYPE_REMOTE);
        result.is_redirected = (type == FILE_TYPE_CHAR) ? !GetConsoleMode(result.handle, &dummy) : true;
    }

    return result;
}

// Prints a message to a platform stream. If the stream is a console, uses supplied colors.
fn_internal void
win32_print_to_stream(const char* message, int message_len, win32_standard_stream_t stream, WORD text_color)
{
    wchar_t message_w[4096]; //todo: allocate from FrameMemory
    int len_w = utf8_to_utf16(message, message_len, message_w, 4096);

    // If redirected, write to a file instead of console.
    DWORD dummy;
    if (stream.is_redirected)
    {
        WriteFile(stream.handle, message_w, (DWORD)len_w, &dummy, 0);
    }
    else
    {
        WORD previous_color = win32_get_console_color(stream.handle);

        SetConsoleTextAttribute(stream.handle, text_color);
        WriteConsoleW(stream.handle, message_w, (DWORD)len_w, &dummy, 0);

        // Restore console colors
        SetConsoleTextAttribute(stream.handle, previous_color);
    }
}

fn_export void
platform_log_to_console(bool is_error, console_color_t foreground, console_color_t background, const char *message)
{
    win32_console_color_t win32_foreground = color_to_win32_color(foreground);
    win32_console_color_t win32_background = color_to_win32_color(background);
    WORD win32_console_color = win32_compose_console_color(win32_foreground, win32_background);

    if (is_error)
    {
        win32_standard_stream_t error_stream = win32_get_standard_stream(STD_ERROR_HANDLE);
        win32_print_to_stream(message, str_len(message), error_stream, win32_console_color);
    }
    else
    {
        win32_standard_stream_t standard_stream = win32_get_standard_stream(STD_OUTPUT_HANDLE);
        win32_print_to_stream(message, str_len(message), standard_stream, win32_console_color);
    }
}

fn_export void
platform_log_to_debug_console(const char *message)
{
#if CT_DEBUG_BUILD
    if (IsDebuggerPresent())
    {
        wchar_t message_w[4096]; //todo: allocate from FrameMemory
        int len_w = utf8_to_utf16(message, str_len(message), message_w, 4096);

        OutputDebugStringW(message_w);
    }
#endif
}
