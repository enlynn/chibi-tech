#include "stdafx.h"

#include <Windows.h>
#include <timeapi.h>

#include "../Platform.h"
#include "../Assert.h"
#include "../Console.h"
#include "../Timer.h"

var_global constexpr UINT cDesiredSchedulerMS = 1;

namespace ct {
    namespace os {
        void initOSState()
        {
            auto TimeBeginResult = timeBeginPeriod(cDesiredSchedulerMS);
	        ASSERT(TimeBeginResult == TIMERR_NOERROR);
        }
        
        void deinitOSState()
        {
            auto TimeEndResult = timeEndPeriod(cDesiredSchedulerMS);
	        ASSERT(TimeEndResult == TIMERR_NOERROR);
        }

        void sleepMainThread(const u32 tTimeMS)
        {
            Sleep(tTimeMS);
        }

        // Exit the current application. This is the equivalent of an app crash. Useful for error states
        // that aren't detected with ASSERT. console::fatal is an example of this usage.
        void exitProgram [[noreturn]] ()
        {
            // TODO: proper exit code?
            ExitProcess(0);
        }
    
        // Caller is responsible for freeing the utf16 string
        // NOTE(enlynn): mostly needed for interracting with Win API
        [[nodiscard]] std::wstring utf8ToUtf16(std::string_view tUtf8String)
        {
            // First pass in a null bufferr to get the required size.
            int NumberChars = MultiByteToWideChar(CP_UTF8, 0, tUtf8String.data(), int(tUtf8String.size()), nullptr, 0);
            ASSERT(NumberChars != 0); // If we get 0, then something went wrong.

            //wchar_t* Utf16String = new wchar_t[NumberChars + 1];
            //Utf16String[NumberChars] = L'\0';
            std::wstring Utf16String(NumberChars, 0);

            // Convert the string for real this time
            NumberChars = MultiByteToWideChar(CP_UTF8, 0, tUtf8String.data(), int(tUtf8String.size()), &Utf16String[0], (size_t)NumberChars);
            ASSERT(NumberChars != 0); // If we get 0, then something went wrong.

            return Utf16String;
        }
        
        [[nodiscard]] std::string  utf16ToUtf8(std::wstring_view tUtf16String)
        {
            if (tUtf16String.empty()) 
                return std::string();
            
            size_t SizeNeeded = WideCharToMultiByte(CP_UTF8, 0, tUtf16String.data(), (int)tUtf16String.size(), NULL, 0, NULL, NULL);
            std::string Utf8String(SizeNeeded, 0);
            
            WideCharToMultiByte(CP_UTF8, 0, tUtf16String.data(), (int)tUtf16String.size(), &Utf8String[0], (int)SizeNeeded, NULL, NULL);
            return Utf8String;
        }

        void debugBreak [[noreturn]] ()
        {
            DebugBreak();
            std::exit(1);
        }

        bool showAssertDialog(const char* tMessage, const char* tFile, u32 tLine)
        {
            constexpr size_t OutputBufferSize = 4096;
            char Buf[OutputBufferSize];
            snprintf(Buf, OutputBufferSize,
                    "Assertion Failed!\n"
                    "    File: %s\n"
                    "    Line: %u\n"
                    "    Statement: ASSERT(%s)\n",
                    tFile, tLine, tMessage);
            platformLogToConsole(false, ct::console::Color::Red, ct::console::Color::Black, Buf);
            
            snprintf(Buf, OutputBufferSize,
                    "--File--\n"
                    "%s\n"
                    "\n"
                    "Line %u\n"
                    "\n"
                    "--Statement--\n"
                    "ASSERT(%s)\n"
                    "\n"
                    "Press Abort to stop execution, Retry to set a breakpoint (if debugging), or Ignore to continue execution.\n", tFile, tLine, tMessage);
            
            const UINT MessageFlags = 
                MB_ABORTRETRYIGNORE | 
                MB_ICONERROR        | 
                MB_TOPMOST          |  
                MB_SETFOREGROUND;

            std::wstring BufW = utf8ToUtf16(Buf);

            int Result = MessageBoxW(0, BufW.data(), L"Assertion Failed!", MessageFlags);
            if (Result == IDABORT)
            {
                exitProgram();
            }
            else if (Result == IDRETRY)
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        void showErrorDialog(const char* tMessage)
        { // TODO
        }

        bool writeBufferToFile(const std::filesystem::path& tFilepath, void* tBuffer, size_t tBufferSize, bool tAppend) {
            // winapi desires wide strings
            std::wstring pathAsWide = tFilepath.wstring();

            UINT desiredAccess = GENERIC_WRITE;
            if (tAppend) {
                desiredAccess |= FILE_APPEND_DATA;
            }

            bool result = false;
            HANDLE fileHandle = CreateFileW(pathAsWide.c_str(), desiredAccess, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
            if (fileHandle != INVALID_HANDLE_VALUE)
            {
                DWORD bytesWritten;
                if (WriteFile(fileHandle, tBuffer, (DWORD)tBufferSize, &bytesWritten, nullptr))
                {
                    result = (bytesWritten == tBufferSize);
                }
                CloseHandle(fileHandle);
            }

            return result;
        }

        bool readEntireFileToBuffer(const std::filesystem::path& tFilepath, void** tOutBuffer, size_t* tOutBufferSize) {
            std::wstring pathAsWide = tFilepath.wstring();

            HANDLE fileHandle = CreateFileW(pathAsWide.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (fileHandle == INVALID_HANDLE_VALUE)
            {
                DWORD error = GetLastError();
                ct::console::error("Unable to open file: %s, with error: %d", tFilepath.c_str(), error);
                CloseHandle(fileHandle);
                return false;
            }

            // Get the file info so we can know the size of the file
            BY_HANDLE_FILE_INFORMATION FileInfo = {};
            BOOL FileInfoResult = GetFileInformationByHandle(fileHandle, &FileInfo);
            ASSERT(FileInfoResult != 0);

            // Not going to properly composite the high and low order size bits, and
            // not going to read in files greater than U32_MAX.

            // If this ever happens, need to do something special. Can't just dump a file this large into memory.
            ASSERT(FileInfo.nFileSizeHigh == 0);

            *tOutBufferSize = FileInfo.nFileSizeLow;
            *tOutBuffer = malloc(*tOutBufferSize);

            DWORD BytesRead;
            BOOL FileReadResult = ReadFile(fileHandle, *tOutBuffer, DWORD(*tOutBufferSize), &BytesRead, nullptr);
            ASSERT(FileReadResult != FALSE);

            CloseHandle(fileHandle);
            return true;
        }
    }
}