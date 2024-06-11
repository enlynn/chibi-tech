#pragma once

#include <string>
#include <string_view>
#include <filesystem>

#include <Types.h>

namespace ct {
    namespace os {
        void initOSState();
        void deinitOSState();

        void sleepMainThread(const u32 tTimeMS);

        // Exit the current application. This is the equivalent of an app crash. Useful for error states
        // that aren't detected with ASSERT. console::fatal is an example of this usage.
        void exitProgram [[noreturn]] ();
    
        // Caller is responsible for freeing the utf16 string
        // NOTE(enlynn): mostly needed for interracting with Win API
        [[nodiscard]] std::wstring utf8ToUtf16(std::string_view tUtf8String);
        [[nodiscard]] std::string  utf16ToUtf8(std::wstring_view tUtf16String);

        bool writeBufferToFile(const std::filesystem::path& tFilepath, void* tBuffer, size_t tBufferSize, bool tAppend = false);
        // Buffer allocated with "malloc" and must be freed with a corresponding "free"
        bool readEntireFileToBuffer(const std::filesystem::path& tFilepath, void** tOutBuffer, size_t* tOutBufferSize);
    }
}