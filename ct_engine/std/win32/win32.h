#ifndef _CT_WIN32_H_
#define _CT_WIN32_H_

#ifdef NOMINMAX
#  undef NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef UNICODE
#  define UNICODE
#endif

#include <Windows.h>

#endif //_CT_WIN32_H_
