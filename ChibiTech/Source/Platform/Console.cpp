#include <string>
#include <cstdarg>

#include "Console.h"
#include "Assert.h"
#include "Platform.h"

struct Logger
{
    ct::console::Severity                        mMinLogLevel                             = ct::console::Severity::Trace;
    ct::console::Flags                           mFlags                                   = {};
    ct::console::Color                           mForegroundColors[(u32)ct::console::Severity::Count] = {
        ct::console::Color::Blue,    // trace
        ct::console::Color::Magenta, // debug
        ct::console::Color::White,   // info
        ct::console::Color::Yellow,  // warn
        ct::console::Color::Red,     // error
        ct::console::Color::White,   // fatal
    };
    ct::console::Color                            mBackgroundColors[(u32)ct::console::Severity::Count] = {
        ct::console::Color::Black,   // trace
        ct::console::Color::Black,   // debug
        ct::console::Color::Black,   // info
        ct::console::Color::Black,   // warn
        ct::console::Color::Black,   // error
        ct::console::Color::DarkRed, // fatal
    };
};

var_global Logger gLogger{};

void ct::console::setMinLogLevel(const Severity tMinLogLevel)
{
    gLogger.mMinLogLevel = tMinLogLevel;
}

void ct::console::setFlags(const Flags tFlags)
{
    gLogger.mFlags = tFlags;
}

void ct::console::setColor(const Severity tLevel, const Color tForeground, const Color tBackground)
{
    gLogger.mForegroundColors[(u32)tLevel] = tForeground;
    gLogger.mBackgroundColors[(u32)tLevel] = tBackground;
}

void ct::console::consoleLog(const Severity tLogLevel, std::string_view tFilename, u32 tLine, std::string_view tFmt, ...)
{
    ASSERT(tLogLevel < Severity::Count);
	if (tLogLevel < gLogger.mMinLogLevel) return;

    // [TRACE] [Thread] File:Line : Message
    // --- TODO: write out the Stack Trace
    // [Debug] File:Line : Message
    // [Info] Message
    // [Warn] Message
    // [Error] File:Line : Message
    // [Fatal] File:Line : Message

    const char* logLevelNames[u32(Severity::Count)] = {
        "[Trace] ",
        "[Debug] ",
        "[Info] ",
        "[Warn] ",
        "[Error] ",
        "[Fatal] "
    };

    const char* levelName = logLevelNames[u32(tLogLevel)];

    constexpr int hardcodedMessageSize = 2048;
    char partialMessage[hardcodedMessageSize];

    va_list messageArgs;
    va_start(messageArgs, tFmt);

    int messageLength = vsnprintf(partialMessage, hardcodedMessageSize, tFmt.data(), messageArgs);
    ASSERT(messageLength < hardcodedMessageSize); // TODO: Are exceeding hard coded limits, prolly should do something better

    partialMessage[messageLength] = '\n';
    partialMessage[messageLength + 1] = 0;

    va_end(messageArgs);

    // Build the full message
    std::string message = levelName;
    message += tFilename;
    message += ":";
    message += std::to_string(tLine);
    message += "\t";
    message += partialMessage;

    if ((gLogger.mFlags & Flag::Console) != 0)
    {
        platformLogToConsole(tLogLevel > Severity::Warn, gLogger.mForegroundColors[u32(tLogLevel)], gLogger.mBackgroundColors[u32(tLogLevel)], message);
    }

#if CT_BUILD
    if ((gLogger.mFlags & Flag::DebugConsole) != 0))
    {
        platformLogToDebugConsole(message);
    }
#endif

    if (tLogLevel == Severity::Fatal)
    {
        ct::os::exitProgram();
    }
}