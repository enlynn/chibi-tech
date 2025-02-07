#include "win32.h"
#include <Objbase.h>

#include "../types.h"
#include "../dll.h"
#include "../str.h"
#include "../log.h"
#include "../cassert.h"
#include "../mem.h"

#include <stdio.h>

fn_export int
utf8_to_utf16(const char* utf8, int utf8_len, wchar_t* utf16, int utf16_buff_len)
{
    // First pass in a null bufferr to get the required size.
    int num_chars = MultiByteToWideChar(CP_UTF8, 0, utf8, utf8_len, NULL, 0);
    ASSERT(num_chars != 0); // If we get 0, then something went wrong.

    // Convert the string for real this time
    num_chars = MultiByteToWideChar(CP_UTF8, 0, utf8,utf8_len, utf16, utf16_buff_len);
    ASSERT(num_chars != 0); // If we get 0, then something went wrong.

    utf16[num_chars] = L'\0';
    return num_chars;
}

fn_export int
utf16_to_utf8(const wchar_t* utf16, int utf16_len, char* utf8, int utf8_buf_len)
{
    int num_chars = WideCharToMultiByte(CP_UTF8, 0, utf16, utf16_len, NULL, 0, NULL, NULL);

    if (utf8 != NULL && num_chars > 0) {
        num_chars = WideCharToMultiByte(CP_UTF8, 0, utf16, utf16_len, utf8, utf8_buf_len, NULL, NULL);
        utf8[num_chars] = 0;
    }

    return num_chars;
}

fn_export os_dll_t
dll_load(const char *libpath)
{
    wchar_t libpath_w[MAX_PATH];
    utf8_to_utf16(libpath, str_len(libpath), libpath_w, MAX_PATH);

    os_dll_t result = {};
    result.handle = LoadLibraryW(libpath_w);

    return result;
}

fn_export bool
dll_is_loaded(os_dll_t *dll)
{
    return dll->handle != NULL;
}

fn_export void
dll_unload(os_dll_t *dll)
{
    if (dll->handle)
    {
        FreeLibrary(dll->handle);
        dll->handle = NULL;
    }
}

fn_export void*
dll_get_fn(os_dll_t *dll, const char* fn_name)
{
    if (dll->handle)
    {
        return GetProcAddress(dll->handle, fn_name);
    }

    return NULL;
}

fn_export bool
platform_show_assert_dialog(const char* message, const char* file, u32 line)
{
    #define output_buffer_size 4096
    char buf[output_buffer_size];
    snprintf(buf, output_buffer_size,
            "Assertion Failed!\n"
            "    File: %s\n"
            "    Line: %u\n"
            "    Statement: ASSERT(%s)\n",
            file, line, message);

    platform_log_to_console(false, CONSOLE_COLOR_RED, CONSOLE_COLOR_BLACK, buf);

    snprintf(buf, output_buffer_size,
            "--File--\n"
            "%s\n"
            "\n"
            "Line %u\n"
            "\n"
            "--Statement--\n"
            "ASSERT(%s)\n"
            "\n"
            "Press Abort to stop execution, Retry to set a breakpoint (if debugging), or Ignore to continue execution.\n", file, line, message);

    const UINT message_flags =
        MB_ABORTRETRYIGNORE |
        MB_ICONERROR        |
        MB_TOPMOST          |
        MB_SETFOREGROUND;

    wchar_t message_w[4096]; //todo: allocate from FrameMemory
    utf8_to_utf16(buf, output_buffer_size, message_w, 4096);

    int result = MessageBoxW(0, message_w, L"Assertion Failed!", message_flags);
    if (result == IDABORT)
    {
        ExitProcess(0);
    }
    else if (result == IDRETRY)
    {
        return true;
    }
    else
    {
        return false;
    }
}

fn_export void
platform_show_error_dialog(const char* message)
{ // TODO
}

fn_export void
platform_debug_break()
{
    DebugBreak();
    ExitProcess(0);
}

fn_internal size_t
win32_get_page_size()
{
    SYSTEM_INFO sSysInfo;
    GetSystemInfo(&sSysInfo);
    return sSysInfo.dwPageSize;
}

fn_export void*
virtual_alloc(void *base_ptr, size_t size, enum virtual_alloc_flag flags)
{
    if (flags == VIRTUAL_ALLOC_NONE) flags = VIRTUAL_ALLOC_DEFAULT;

    const size_t page_size = win32_get_page_size();
    size = FORWARD_ALIGN(size, page_size);

    DWORD alloc_type = 0;
    if (flags & VIRTUAL_ALLOC_RESERVE) alloc_type |= MEM_RESERVE;
    if (flags & VIRTUAL_ALLOC_COMMIT)  alloc_type |= MEM_COMMIT;

    LPVOID result = VirtualAlloc(base_ptr, size, alloc_type, PAGE_READWRITE);
    ASSERT(result != NULL);

    return result;
}

fn_export void
virtual_free(void *ptr, size_t size /* required for mmap */)
{
    BOOL Err = VirtualFree(ptr, 0, MEM_RELEASE);
    ASSERT(Err > 0);
}


//fn_export int
//platform_get_exe_path(char* exe_path, int exe_buffer_size)
//{
//	DWORD size_of_filename = GetModuleFileNameW(0, Win32State->exe_filename, sizeof(Win32State->exe_filename));
//    Win32State->exe_one_past_last_slash = Win32State->exe_filename;
//    for (char *Scan = Win32State->exe_filename; *Scan; ++Scan)
//    {
//        if (*Scan == '\\')
//        {
//            Win32State->exe_one_past_last_slash = Scan + 1;
//        }
//    }
//}
//
//fn_export void
//platform_build_path_from_exe(win32_state *Win32State, char *Filename, s64 DestinationLength, char *Destination)
//{
//    StringCat(Win32State->exe_one_past_last_slash - Win32State->exe_filename, Win32State->exe_filename,
//			  StringLength(Filename), Filename,
//			  DestinationLength, Destination);
//}


var_global const guid_t c_invalid_guid = {};

fn_export bool 
platform_compare_guids(const guid_t *left, const guid_t *right)
{
    u128 *uleft  = (u128*)left;
    u128 *uright = (u128*)right;
    return uleft->bits64[0] == uright->bits64[0]
        && uleft->bits64[1] == uright->bits64[1];
}

fn_export guid_t 
platform_generate_guid()
{
	guid_t result = c_invalid_guid;

	do
	{
		GUID win32_result;
		HRESULT err = CoCreateGuid(&win32_result);
		mem_copy(&result,  &win32_result, sizeof(guid_t));
	} while (platform_compare_guids(&result, &c_invalid_guid));

	return result;
}

fn_export bool 
platform_is_guid_valid(guid_t guid)
{
    return !platform_compare_guids(&guid, &c_invalid_guid);
}

fn_export guid_t 
platform_get_invalid_guid()
{
    return c_invalid_guid;
}

fn_export int 
platfor_guid_to_string(guid_t ct_guid, char *str_buffer, int buffer_len)
{
    GUID guid = *(GUID*)&ct_guid;

	s32 req = snprintf(NULL, 0, "%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX", 
		guid.Data1, guid.Data2, guid.Data3, 
		guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
		guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

	if (req < buffer_len - 1)
	{
		req = snprintf(str_buffer, buffer_len, "%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX", 
			guid.Data1, guid.Data2, guid.Data3, 
			guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
			guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

		str_buffer[req] = 0;
	}

    return 0;
}

fn_export guid_t 
platform_string_to_guid(const char* guid_str)
{
    GUID result;
    
    int res = sscanf_s(guid_str, "%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
                       &result.Data1, &result.Data2, &result.Data3, 
                       &result.Data4[0], &result.Data4[1], &result.Data4[2], &result.Data4[3],
                       &result.Data4[4], &result.Data4[5], &result.Data4[6], &result.Data4[7]);
    
    return *(guid_t*)&result;
}