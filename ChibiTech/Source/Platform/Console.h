#pragma once 

#include <string_view>
#include <source_location>

#include <Types.h>

namespace ct::console {
    enum class Color : u8
    {
        Black,
        DarkBlue,
        DarkGreen,
        DarkCyan,
        DarkRed,
        DarkMagenta,
        DarkYellow,
        Grey,
        DarkGrey,
        Blue,
        Green,
        Cyan,
        Red,
        Magenta,
        Yellow,
        White,
        Count,
    };

    namespace Flag
    {
        enum {
            None = 0,
            File = 1,
            Editor = 2,
            Console = 3,
            DebugConsole = 4,
            Count,
        };
    };

    using Flags = u64;

    enum class Severity : u8
    {
        Trace,
        Debug,
        Info,
        Warn,
        Error,
        Fatal,
        Count,
    };

    void setMinLogLevel(const Severity tMinLogLevel);
    void setColor(const Severity tLevel, const Color tForeground, const Color tBackground);
    void setFlags(const Flags tFlags);
    void consoleLog(const Severity tLogLevel, std::string_view tFilename, u32 tLine, std::string_view tFmt, ...);

    // Platform dependent implementation
    void platformLogToConsole(bool tIsError, Color tForeground, Color tBackground, std::string_view tMessage);
    void platformLogToDebugConsole(std::string_view tMessage);

    namespace internal { //not meant to be directly referred too
        struct format_location
        {
            const char*          mFmt;
            std::source_location mLoc;

            format_location(const char* tFmt, const std::source_location& loc = std::source_location::current())
            : mFmt(tFmt), mLoc(loc) {}
        };
    }

    template<typename... args> void trace(internal::format_location Format, args&& ... tArgs)
    {
        consoleLog(Severity::Trace, Format.mLoc.file_name(), Format.mLoc.line(), Format.mFmt, tArgs...);
    }

    template<typename... args> void debug(internal::format_location Format, args&& ... tArgs)
    {
        consoleLog(Severity::Debug, Format.mLoc.file_name(), Format.mLoc.line(), Format.mFmt, tArgs...);
    }

    template<typename... args> void info(internal::format_location Format, args&& ... tArgs)
    {
        consoleLog(Severity::Info, Format.mLoc.file_name(), Format.mLoc.line(), Format.mFmt, tArgs...);
    }

    template<typename... args> void warn(internal::format_location Format, args&& ... tArgs)
    {
        consoleLog(Severity::Warn, Format.mLoc.file_name(), Format.mLoc.line(), Format.mFmt, tArgs...);
    }

    template<typename... args> void error(internal::format_location Format, args&& ... tArgs)
    {
        consoleLog(Severity::Error, Format.mLoc.file_name(), Format.mLoc.line(), Format.mFmt, tArgs...);
    }

    template<typename... args> void fatal [[noreturn]] (internal::format_location Format, args&& ... tArgs)
    {
        consoleLog(Severity::Fatal, Format.mLoc.file_name(), Format.mLoc.line(), Format.mFmt, tArgs...);
        std::exit(1);
    }
}