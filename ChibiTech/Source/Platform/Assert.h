#pragma once

#include <Types.h>

// Platform specific
namespace ct::os {
    bool showAssertDialog(const char* tMessage, const char* tFile, u32 tLine);
    void showErrorDialog(const char* tMessage);
    void debugBreak [[noreturn]] ();
}


#if CT_DEBUG
#  define ASSERT(x)                                                                     \
	{                                                                                   \
		if (!(x))                                                                       \
		{                                                                               \
			if (ct::os::showAssertDialog(#x, __FILE__, (u32)__LINE__)) ct::os::debugBreak(); \
		}                                                                               \
	}
#else
#  define ASSERT(x)
#endif // CT_DEBUG

#if CT_DEBUG
#  define ASSERT_CUSTOM(x, message)                                                          \
	{                                                                                        \
		if (!(x))                                                                            \
		{                                                                                    \
			if (ct::os::showAssertDialog(message, __FILE__, (u32)__LINE__)) ct::os::debugBreak(); \
		}                                                                                    \
	}
#else
#  define ASSERT_CUSTOM(x, message)
#endif // CT_DEBUG

#define UNIMPLEMENTED ASSERT_CUSTOM(false, "UNIMPLEMENTED.")