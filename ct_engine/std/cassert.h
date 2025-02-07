#ifndef _CASSERT_H_
#define _CASSERT_H_

#include "types.h"

#if CT_DEBUG_BUILD
#  define ASSERT(x)                                                                               \
	{                                                                                             \
	   if (!(x))                                                                                  \
		{                                                                                         \
			if (platform_show_assert_dialog(#x, __FILE__, (u32)__LINE__)) platform_debug_break(); \
		}                                                                                         \
	}
#else
#  define ASSERT(x)
#endif // CT_DEBUG

#if CT_DEBUG_BUILD
#  define ASSERT_CUSTOM(x, message)                                                                    \
    {                                                                                                  \
        if (!(x))                                                                                      \
        {                                                                                              \
            if (platform_show_assert_dialog(message, __FILE__, (u32)__LINE__)) platform_debug_break(); \
        }                                                                                              \
	}
#else
#  define ASSERT_CUSTOM(x, message)
#endif // CT_DEBUG

#define UNIMPLEMENTED ASSERT_CUSTOM(false, "UNIMPLEMENTED.")

fn_export bool platform_show_assert_dialog(const char* message, const char* file, u32 line);
fn_export void platform_show_error_dialog(const char* message);
fn_export void platform_debug_break();

#endif //_CASSERT_H_
