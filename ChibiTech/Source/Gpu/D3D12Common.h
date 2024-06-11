#pragma once

#include "../Types.h"

#include <dxgi.h>
#include <d3d12/d3d12.h>
#include <dxgi1_4.h>
#include <dxgi1_5.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#if CT_DEBUG
#include <dxgidebug.h>
#endif

#include <Platform/Assert.h>

#include <windows.h>

#define ComCast(DoublePtr)               __uuidof(**(DoublePtr)), (void**)((DoublePtr))
#define ExplicitComCast(Type, DoublePtr) __uuidof(Type), (void**)(DoublePtr)
#define ComSafeRelease(Ptr)              if (Ptr) { (Ptr)->Release(); (Ptr) = nullptr; }

#ifdef _DEBUG 
#define AssertHr(Func) {       \
		HRESULT Hr = (Func);   \
		ASSERT(SUCCEEDED(Hr)); \
	}
#else
#define AssertHr(Func) (Func)
#endif