#include "stdafx.h"

#include "Windows.h"

#include "../Console.h"
#include "../Assert.h"
#include "../Platform.h"

struct win32_standard_stream
{
    HANDLE Handle;                         // Stream handle (STD_OUTPUT_HANDLE or STD_ERROR_HANDLE).
    bool   IsRedirected;                   // True if redirected to file.
};

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
enum class win32_console_color : int
{
    black        = 0,                                                    // color = black        | value = 0
    dark_blue    = FOREGROUND_BLUE,                                      // color = dark blue    | value = 1
    dark_green   = FOREGROUND_GREEN,                                     // color = dark green   | value = 2
    dark_cyan    = FOREGROUND_BLUE | FOREGROUND_GREEN,                   // color = dark cyan    | value = 3
    dark_red     = FOREGROUND_RED,                                       // color = dark red     | value = 3
    dark_magenta = FOREGROUND_BLUE | FOREGROUND_RED,                     // color = dark magenta | value = 5
    dark_yellow  = FOREGROUND_RED | FOREGROUND_GREEN,                    // color = dark yellow  | value = 6
    grey         = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED,  // color = grey         | value = 7
    dark_grey    = FOREGROUND_INTENSITY,                                 // color = dark grey    | value = 8
    blue         = FOREGROUND_INTENSITY | dark_blue,                     // color = blue         | value = 9
    green        = FOREGROUND_INTENSITY | dark_green,                    // color = green        | value = 10
    cyan         = FOREGROUND_INTENSITY | dark_cyan,                     // color = cyan         | value = 11
    red          = FOREGROUND_INTENSITY | dark_red,                      // color = red          | value = 12
    magenta      = FOREGROUND_INTENSITY | dark_magenta,                  // color = magenta      | value = 13
    yellow       = FOREGROUND_INTENSITY | dark_yellow,                   // color = yellow       | value = 14
    white        = FOREGROUND_INTENSITY | grey,                          // color = white        | value = 15
};

inline WORD Win32ComposeConsoleColor(win32_console_color Foreground, win32_console_color Background)
{
    return int(Foreground) | (int(Background) << 4);
}

inline WORD Win32GetConsoleColor(HANDLE console_handle)
{
    WORD Result = Win32ComposeConsoleColor(win32_console_color::white, win32_console_color::black);

    CONSOLE_SCREEN_BUFFER_INFO console_info{};
    BOOL info_Result = GetConsoleScreenBufferInfo(console_handle, &console_info);
    if (info_Result > 0)
    {
        Result = console_info.wAttributes;
    }

    return Result;
}

win32_console_color LogColorToWin32Color(ct::console::Color in_color)
{
    win32_console_color OutColor = win32_console_color::black;
    switch (in_color)
    {
        case ct::console::Color::Black:        OutColor = win32_console_color::black;        break;
        case ct::console::Color::DarkBlue:     OutColor = win32_console_color::dark_blue;    break;
        case ct::console::Color::DarkGreen:    OutColor = win32_console_color::dark_green;   break;
        case ct::console::Color::DarkCyan:     OutColor = win32_console_color::dark_cyan;    break;
        case ct::console::Color::DarkRed:      OutColor = win32_console_color::dark_red;     break;
        case ct::console::Color::DarkMagenta:  OutColor = win32_console_color::dark_magenta; break;
        case ct::console::Color::DarkYellow:   OutColor = win32_console_color::dark_yellow;  break;
        case ct::console::Color::Grey:         OutColor = win32_console_color::grey;         break;
        case ct::console::Color::DarkGrey:     OutColor = win32_console_color::dark_grey;    break;
        case ct::console::Color::Blue:         OutColor = win32_console_color::blue;         break;
        case ct::console::Color::Green:        OutColor = win32_console_color::green;        break;
        case ct::console::Color::Cyan:         OutColor = win32_console_color::cyan;         break;
        case ct::console::Color::Red:          OutColor = win32_console_color::red;          break;
        case ct::console::Color::Magenta:      OutColor = win32_console_color::magenta;      break;
        case ct::console::Color::Yellow:       OutColor = win32_console_color::yellow;       break;
        case ct::console::Color::White:        OutColor = win32_console_color::white;        break;
    }
    return OutColor;
}

bool Win32RedirectConsoleIo()
{
    bool Result = true;
    FILE* fp;

    // Redirect STDIN if the console has an input handle
    if (GetStdHandle(STD_INPUT_HANDLE) != INVALID_HANDLE_VALUE)
    {
        if (freopen_s(&fp, "CONIN$", "r", stdin) != 0)
        Result = false;
    }
    else
    {
        setvbuf(stdin, NULL, _IONBF, 0);
    }

    // Redirect STDOUT if the console has an output handle
    if (GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE)
    {
        if (freopen_s(&fp, "CONOUT$", "w", stdout) != 0)
        Result = false;
    }
    else
    {
        setvbuf(stdout, NULL, _IONBF, 0);
    }

    // Redirect STDERR if the console has an error handle
    if (GetStdHandle(STD_ERROR_HANDLE) != INVALID_HANDLE_VALUE)
    {
        if (freopen_s(&fp, "CONOUT$", "w", stderr) != 0)
        Result = false;
    }
    else
    {
        setvbuf(stderr, NULL, _IONBF, 0);
    }

    return Result;
}

// Sets up a standard stream (stdout or stderr).
win32_standard_stream Win32GetStandardStream(DWORD StreamType)
{
    win32_standard_stream Result{};

    // If we don't have our own stream and can't find a parent console, allocate a new console.
    Result.Handle = GetStdHandle(StreamType);
    if (!Result.Handle || Result.Handle == INVALID_HANDLE_VALUE)
    {
        if (!AttachConsole(ATTACH_PARENT_PROCESS))
        {
            AllocConsole();
            bool redirect_Result = Win32RedirectConsoleIo();
            ASSERT(redirect_Result);
        }

        Result.Handle = GetStdHandle(StreamType);
        ASSERT(Result.Handle != INVALID_HANDLE_VALUE);
    }

    // Check if the stream is redirected to a file. If it does, check if the file already exists.
    if (Result.Handle != INVALID_HANDLE_VALUE)
    {
        DWORD dummy;
        DWORD type = GetFileType(Result.Handle) & (~FILE_TYPE_REMOTE);
        Result.IsRedirected = (type == FILE_TYPE_CHAR) ? !GetConsoleMode(Result.Handle, &dummy) : true;
    }

    return Result;
}

// Prints a message to a platform stream. If the stream is a console, uses supplied colors.
void Win32PrintToStream(std::string_view Message, win32_standard_stream stream, WORD text_color)
{
    std::wstring MessageW = ct::os::utf8ToUtf16(Message);

    // If redirected, write to a file instead of console.
    DWORD dummy;
    if (stream.IsRedirected)
    {
        WriteFile(stream.Handle, MessageW.data(), (DWORD)MessageW.size(), &dummy, 0);
    }
    else
    {
        WORD previous_color = Win32GetConsoleColor(stream.Handle);

        SetConsoleTextAttribute(stream.Handle, text_color);
        WriteConsoleW(stream.Handle, MessageW.data(), (DWORD)MessageW.size(), &dummy, 0);

        // Restore console colors
        SetConsoleTextAttribute(stream.Handle, previous_color);
    }
}

namespace ct {
    namespace console {
        void platformLogToConsole(bool tIsError, Color tForeground, Color tBackground, std::string_view tMessage)
        {
            win32_console_color Win32Foreground = LogColorToWin32Color(tForeground);
            win32_console_color Win32Background = LogColorToWin32Color(tBackground);
            WORD win32ConsoleColor = Win32ComposeConsoleColor(Win32Foreground, Win32Background);

            if (tIsError)
            {
                static win32_standard_stream ErrorStream = Win32GetStandardStream(STD_ERROR_HANDLE);
                Win32PrintToStream(tMessage, ErrorStream, win32ConsoleColor);
            }
            else
            {
                static win32_standard_stream StandardStream = Win32GetStandardStream(STD_OUTPUT_HANDLE);
                Win32PrintToStream(tMessage, StandardStream, win32ConsoleColor);
            }
        }

        void platformLogToDebugConsole(std::string_view tMessage)
        {
#if CT_DEBUG
            if (IsDebuggerPresent())
            {
                OutputDebugStringA(tMessage.data());
            }
#endif
        }
    }
}