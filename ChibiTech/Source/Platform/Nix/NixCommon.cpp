#include "stdafx.h"

#include <chrono>
#include <thread>

#include "../Platform.h"
#include "../Assert.h"
#include "../Console.h"
#include "../Timer.h"

namespace ct::os {
    void initOSState() {}

    void deinitOSState() {}

    void sleepMainThread(const u32 tTimeMS) {
        std::this_thread::sleep_for(std::chrono::milliseconds(tTimeMS));
    }

    // Exit the current application. This is the equivalent of an app crash. Useful for error states
    // that aren't detected with ASSERT. console::fatal is an example of this usage.
    void exitProgram [[noreturn]]() {
        std::exit(1);
    };

    [[nodiscard]] std::wstring utf8ToUtf16(std::string_view tUtf8String) {
        ASSERT_CUSTOM(false, "Should not be calling utf8ToUtf16 on Unix. This is a utility function for Windows code.");
        return {};
    }

    [[nodiscard]] std::string utf16ToUtf8(std::wstring_view tUtf16String) {
        ASSERT_CUSTOM(false, "Should not be calling utf8ToUtf16 on Unix. This is a utility function for Windows code.");
        return {};
    }

    bool showAssertDialog(const char *tMessage, const char *tFile, u32 tLine) {
        return true;
    }

    void showErrorDialog(const char *tMessage) {}

    void debugBreak [[noreturn]]() {
        exitProgram();
    }
}

namespace ct::console {
    void platformLogToConsole(bool tIsError, Color tForeground, Color tBackground, std::string_view tMessage)
    {
        // ANSI Color Codes
        constexpr int cForegroundBlack = 30;
        constexpr int cBackgroundBlack = 40;

        constexpr int cForegroundRed = 31;
        constexpr int cBackgroundRed = 41;

        constexpr int cForegroundGreen = 32;
        constexpr int cBackgroundGreen = 42;

        constexpr int cForegroundYellow = 33;
        constexpr int cBackgroundYellow = 43;

        constexpr int cForegroundBlue = 34;
        constexpr int cBackgroundBlue = 44;

        constexpr int cForegroundMagenta = 35;
        constexpr int cBackgroundMagenta = 45;

        constexpr int cForegroundCyan = 36;
        constexpr int cBackgroundCyan = 46;

        constexpr int cForegroundWhite = 37;
        constexpr int cBackgroundWhite = 47;

        constexpr int cBoldBrightOn  = 1;
        constexpr int cBoldBrightOff = 21;

        bool isForegroundBright = false;
        int foregroundCode = 0;
        switch (tForeground)
        {
            case Color::Black:       foregroundCode = cForegroundBlack;
            case Color::DarkBlue:    foregroundCode = cForegroundBlue;
            case Color::DarkGreen:   foregroundCode = cForegroundGreen;
            case Color::DarkCyan:    foregroundCode = cForegroundCyan;
            case Color::DarkRed:     foregroundCode = cForegroundRed;
            case Color::DarkMagenta: foregroundCode = cForegroundMagenta;
            case Color::DarkYellow:  foregroundCode = cForegroundYellow;
            case Color::DarkGrey:    foregroundCode = cForegroundBlue | cForegroundGreen | cForegroundRed;
            case Color::Grey:        foregroundCode = cForegroundBlue | cForegroundGreen | cForegroundRed; isForegroundBright = true; break;
            case Color::Blue:        foregroundCode = cForegroundBlue;    isForegroundBright = true; break;
            case Color::Green:       foregroundCode = cForegroundGreen;   isForegroundBright = true; break;
            case Color::Cyan:        foregroundCode = cForegroundCyan;    isForegroundBright = true; break;
            case Color::Red:         foregroundCode = cForegroundRed;     isForegroundBright = true; break;
            case Color::Magenta:     foregroundCode = cForegroundMagenta; isForegroundBright = true; break;
            case Color::Yellow:      foregroundCode = cForegroundYellow;  isForegroundBright = true; break;
            case Color::White:       foregroundCode = cForegroundWhite;   isForegroundBright = true; break;
            case Color::Count: // intentional fallthrough
            default:
                ASSERT_CUSTOM(false, "Invalid color.")
        }

        bool isBackgroundBright = false;
        int backgroundCode = 0;
        switch (tBackground)
        {
            case Color::Black:       backgroundCode = cBackgroundBlack;
            case Color::DarkBlue:    backgroundCode = cBackgroundBlue;
            case Color::DarkGreen:   backgroundCode = cBackgroundGreen;
            case Color::DarkCyan:    backgroundCode = cBackgroundCyan;
            case Color::DarkRed:     backgroundCode = cBackgroundRed;
            case Color::DarkMagenta: backgroundCode = cBackgroundMagenta;
            case Color::DarkYellow:  backgroundCode = cBackgroundYellow;
            case Color::DarkGrey:    backgroundCode = cBackgroundBlue | cBackgroundGreen | cBackgroundRed;
            case Color::Grey:        backgroundCode = cBackgroundBlue | cBackgroundGreen | cBackgroundRed; isBackgroundBright = true; break;
            case Color::Blue:        backgroundCode = cBackgroundBlue;    isBackgroundBright = true; break;
            case Color::Green:       backgroundCode = cBackgroundGreen;   isBackgroundBright = true; break;
            case Color::Cyan:        backgroundCode = cBackgroundCyan;    isBackgroundBright = true; break;
            case Color::Red:         backgroundCode = cBackgroundRed;     isBackgroundBright = true; break;
            case Color::Magenta:     backgroundCode = cBackgroundMagenta; isBackgroundBright = true; break;
            case Color::Yellow:      backgroundCode = cBackgroundYellow;  isBackgroundBright = true; break;
            case Color::White:       backgroundCode = cBackgroundWhite;   isBackgroundBright = true; break;
            case Color::Count: // intentional fallthrough
            default:
            ASSERT_CUSTOM(false, "Invalid color.")
        }

        const bool isAnyBright = isForegroundBright | isBackgroundBright;
        const int brightOnCode = isAnyBright ? cBoldBrightOn : cBoldBrightOff;

        // TODO(enlynn): Fix background code.
        // << ";" << backgroundCode

        std::cout << "\033[" << brightOnCode << ";" << foregroundCode << "m" << tMessage << "\033[0m";
    }

    void platformLogToDebugConsole(std::string_view tMessage)
    {
        // Nothing to do for linux. this is a windows-specific function (for now)
    }
}