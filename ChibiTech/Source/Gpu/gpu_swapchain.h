#pragma once

#include <types.h>

#include "D3D12Common.h"

#include "gpu_resource.h"
#include "gpu_descriptor_allocator.h"
#include "gpu_texture.h"
#include "gpu_render_target.h"

class platform_window;
class gpu_device;
class gpu_command_queue;

struct gpu_frame_cache;

namespace ct::os {
    class Window;
};

struct gpu_swapchain_info
{
	// Required to set.
	gpu_device*               mDevice                    = nullptr;
	gpu_command_queue*        mPresentQueue              = nullptr;
	cpu_descriptor_allocator* mRenderTargetDesciptorHeap = nullptr;
	// Optional settings
	u32                      mBackbufferCount      = 2;                          // TODO(enlynn): Determine the correct number of backbuffers
	DXGI_FORMAT              mSwapchainFormat      = DXGI_FORMAT_R8G8B8A8_UNORM; // TODO(enlynn): HDR?
	bool                     mAllowTearing         = false;                      // TODO(enlynn): How should tearing be supported?
	bool                     mVSyncEnabled         = true;
};

class gpu_swapchain
{
public:
	gpu_swapchain() = default;
	gpu_swapchain(gpu_frame_cache* FrameCache, gpu_swapchain_info& Info, const ct::os::Window& ClientWindow);

	~gpu_swapchain() { /* if (mHandle) Deinit(); */ }
	void Release(gpu_frame_cache* FrameCache);

    void UpdateRenderTargetViews(gpu_frame_cache* FrameCache);
    void Resize(gpu_frame_cache* FrameCache, u32 Width, u32 Height);

	u64 Present();

	gpu_render_target* GetRenderTarget();
    // NOTE(enlynn): This will be handle by gpu_render_target from now on
	//const cpu_descriptor& GetCurrentRenderTarget() const { return mDescriptors[mBackbufferIndex]; }

	DXGI_FORMAT           GetSwapchainFormat()                   const { return mInfo.mSwapchainFormat;         }
    void                  GetDimensions(u32& Width, u32& Height) const { Width = mWidth; Height = mHeight;      }
	 
	static constexpr u32 cMaxBackBufferCount = DXGI_MAX_SWAP_CHAIN_BUFFERS;

private:
	gpu_swapchain_info mInfo;

	IDXGISwapChain3*  mHandle                                    = nullptr;
	u64               mBackbufferIndex                           = 0;
	gpu_texture       mBackbuffers[cMaxBackBufferCount]          = {};
	u64               mFenceValues[cMaxBackBufferCount]          = {};
	u32               mWidth                                     = 0;
	u32               mHeight                                    = 0;
	//cpu_descriptor   mDescriptors[cMaxBackBufferCount]          = {};
    gpu_render_target mRenderTarget                              = {};
};