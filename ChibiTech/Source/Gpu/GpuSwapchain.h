#pragma once

#include <types.h>

#include "D3D12Common.h"

#include "GpuResource.h"
#include "GpuDescriptorAllocator.h"
#include "GpuTexture.h"
#include "GpuRenderTarget.h"
#include "GpuQueue.h"

class platform_window;
class GpuDevice;

struct GpuFrameCache;

namespace ct::os {
    class Window;
};

struct GpuSwapchainInfo
{
	// Required to set.
	GpuDevice*               mDevice                    = nullptr;
	GpuQueue*         mPresentQueue              = nullptr;
	CpuDescriptorAllocator*  mRenderTargetDesciptorHeap = nullptr;
	// Optional settings
	u32                      mBackbufferCount      = 2;                          // TODO(enlynn): Determine the correct number of backbuffers
	DXGI_FORMAT              mSwapchainFormat      = DXGI_FORMAT_R8G8B8A8_UNORM; // TODO(enlynn): HDR?
	bool                     mAllowTearing         = false;                      // TODO(enlynn): How should tearing be supported?
	bool                     mVSyncEnabled         = true;
};

class GpuSwapchain
{
public:
	GpuSwapchain() = default;
	GpuSwapchain(GpuFrameCache* FrameCache, GpuSwapchainInfo& Info, const ct::os::Window& ClientWindow);

	~GpuSwapchain() = default; // { /* if (mHandle) deinit(); */ }
	void release(GpuFrameCache* FrameCache);

    void updateRenderTargetViews(GpuFrameCache* FrameCache);
    void resize(GpuFrameCache* FrameCache, u32 Width, u32 Height);

	u64 present();

	GpuRenderTarget* getRenderTarget();
    // NOTE(enlynn): This will be handle by GpuRenderTarget from now on
	//const CpuDescriptor& GetCurrentRenderTarget() const { return mDescriptors[mBackbufferIndex]; }

	DXGI_FORMAT           getSwapchainFormat()                   const { return mInfo.mSwapchainFormat;         }
    void                  getDimensions(u32& Width, u32& Height) const { Width = mWidth; Height = mHeight;      }
	 
	static constexpr u32 cMaxBackBufferCount = DXGI_MAX_SWAP_CHAIN_BUFFERS;

private:
	GpuSwapchainInfo mInfo;

	IDXGISwapChain3*  mHandle                                    = nullptr;
	u64               mBackbufferIndex                           = 0;
	GpuTexture        mBackbuffers[cMaxBackBufferCount]          = {};
	GpuFence          mFenceValues[cMaxBackBufferCount]          = {};
	u32               mWidth                                     = 0;
	u32               mHeight                                    = 0;
	//CpuDescriptor   mDescriptors[cMaxBackBufferCount]          = {};
    GpuRenderTarget   mRenderTarget                              = {};
};