#include "GpuSwapchain.h"
#include "GpuDevice.h"
#include "GpuQueue.h"
#include "GpuState.h"

#include <Platform/Console.h>
#include <Platform/Assert.h>
#include <Platform/Window.h>

#define GLFW_INCLUDE_NONE
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

GpuSwapchain::GpuSwapchain(GpuFrameCache* FrameCache, GpuSwapchainInfo& Info, const ct::os::Window& tWindow)
	: mInfo(Info)
{
	ASSERT(mInfo.mDevice);
    ASSERT(mInfo.mPresentQueue);
    ASSERT(mInfo.mRenderTargetDesciptorHeap);

    auto [windowWidth, windowHeight] = tWindow.getSize();

    GLFWwindow* glfwHandle = tWindow.asHandle();
    HWND win32Handle = glfwGetWin32Window(glfwHandle);

    auto DeviceHandle  = mInfo.mDevice->asHandle();
	auto DeviceAdapter = mInfo.mDevice->asAdapter();

	IDXGIFactory5* DeviceFactory = nullptr;
	AssertHr(DeviceAdapter->GetParent(ComCast(&DeviceFactory)));

	// Is Tearing supported?
	bool IsTearingSupported = mInfo.mAllowTearing;
	if (IsTearingSupported)
	{
		if (FAILED(DeviceFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &IsTearingSupported, sizeof(IsTearingSupported))))
		{
			IsTearingSupported = false;
		}

		if (!IsTearingSupported)
		{
			ct::console::warn("Tearing support requested, but not available.");
		}
	}

	u32 Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
	if (IsTearingSupported) Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	DXGI_SWAP_CHAIN_DESC1 SwapchainDesc = {};
	SwapchainDesc.Width       = windowWidth;
	SwapchainDesc.Height      = windowHeight;
	SwapchainDesc.Format      = mInfo.mSwapchainFormat;
	SwapchainDesc.Stereo      = FALSE;
	SwapchainDesc.SampleDesc  = { .Count = 1, .Quality = 0 }; // Number of multisamples per pixels
	SwapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapchainDesc.BufferCount = mInfo.mBackbufferCount;
	SwapchainDesc.Scaling     = DXGI_SCALING_STRETCH;
	SwapchainDesc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	SwapchainDesc.AlphaMode   = DXGI_ALPHA_MODE_UNSPECIFIED;
	SwapchainDesc.Flags       = Flags;

	IDXGISwapChain1* SwapchainBase = nullptr;
	AssertHr(DeviceFactory->CreateSwapChainForHwnd(mInfo.mPresentQueue->asHandle(), (HWND)win32Handle, &SwapchainDesc, nullptr, nullptr, &SwapchainBase));

	// Disable the automatic Alt+Enter fullscreen toggle. We'll do this ourselves to support adaptive refresh rate stuff.
	AssertHr(DeviceFactory->MakeWindowAssociation((HWND)win32Handle, DXGI_MWA_NO_ALT_ENTER));

	// Let's get the actual swapchain
	AssertHr(SwapchainBase->QueryInterface(ComCast(&mHandle)));
	SwapchainBase->Release();

	mBackbufferIndex = mHandle->GetCurrentBackBufferIndex();

	mHandle->SetMaximumFrameLatency(mInfo.mBackbufferCount - 1);

	ForRange(u32, i, cMaxBackBufferCount)
	{
        mFenceValues[i] = {};
	}

    // It is my expectation the number of Buffers we requested, matches the number actually created.
    DXGI_SWAP_CHAIN_DESC1 Desc1 = {};
    mHandle->GetDesc1(&Desc1);
    ASSERT(Desc1.BufferCount == Info.mBackbufferCount);

    // Make sure the width/height is accurate to the swapchain and not the client window
    mWidth  = Desc1.Width;
    mHeight = Desc1.Height;

	// Update render target views
    updateRenderTargetViews(FrameCache);

	ComSafeRelease(DeviceFactory);
}

void GpuSwapchain::updateRenderTargetViews(GpuFrameCache* FrameCache)
{
    // TODO: Wouldn't work for resizing. Technically, we are not guaranteed for the number of backbuffers to
    // remain the same. Will need to iterate over this list and free the Max and then initialize new buffers.
    //
    // So for now, it seems we will keep the same number of backbuffers. Now, this isn't always advisable,
    // but will suffice at the moment.
    //
    ForRange(u32, i, mInfo.mBackbufferCount)
    {
        ID3D12Resource* D3DBackbuffer = nullptr;
        AssertHr(mHandle->GetBuffer(i, ComCast(&D3DBackbuffer)));

        D3D12_CLEAR_VALUE ClearValue = {};
        ClearValue.Format = mInfo.mSwapchainFormat;
        ClearValue.Color[0] = 0.0f;
        ClearValue.Color[1] = 0.0f;
        ClearValue.Color[2] = 0.0f;
        ClearValue.Color[3] = 1.0f;

        // When resizing the swpachain, we can have any lingering backbuffer images. Have to immediate release
        // the underlying Resource.
        if (mBackbuffers[i].getResource()->asHandle())
        {
            const GpuResource* Resource = mBackbuffers[i].getResource();
            FrameCache->removeTrackedResource(*Resource);
            mBackbuffers[i].releaseUnsafe(FrameCache);
        }

        GpuResource Backbuffer = GpuResource(*mInfo.mDevice, D3DBackbuffer, ClearValue);
        FrameCache->trackResource(Backbuffer, D3D12_RESOURCE_STATE_COMMON);

        mBackbuffers[i] = GpuTexture(FrameCache, Backbuffer);
    }
}

void GpuSwapchain::resize(GpuFrameCache* FrameCache, u32 Width, u32 Height)
{
    if (mWidth != Width || mHeight != Height)
    {
        // Cannot create a window smaller than (1, 1)
        mWidth  = std::max(1u, Width);
        mHeight = std::max(1u, Height);

        // Make sure no more work is being executed on any work queue
        // TODO(enlynn): this should probably be the responsibility of the renderer, not the swapchain.
        FrameCache->flushGpu();

        DXGI_SWAP_CHAIN_DESC1 Desc1 = {};
        mHandle->GetDesc1(&Desc1);

        // Always want the waitable object flag, and the tearing flag when supported
        u32 Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        if ((Desc1.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) != 0)
            Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

        AssertHr(mHandle->ResizeBuffers(mInfo.mBackbufferCount, 0, 0, DXGI_FORMAT_UNKNOWN, Flags));

        mBackbufferIndex = mHandle->GetCurrentBackBufferIndex();
        updateRenderTargetViews(FrameCache);
    }
}

void  GpuSwapchain::release(GpuFrameCache* FrameCache)
{
	ForRange(u32, i, cMaxBackBufferCount)
	{ // Release all possible backbuffers
        mBackbuffers[i].releaseUnsafe(FrameCache);
	}

	ComSafeRelease(mHandle);

	mInfo = {};
}

GpuRenderTarget* GpuSwapchain::getRenderTarget()
{
    mRenderTarget.reset();
    mRenderTarget.attachTexture(AttachmentPoint::Color0, &mBackbuffers[mBackbufferIndex]);
    // TODO(enlynn): check if there is a depth buffer present.
    return &mRenderTarget;
}

u64 GpuSwapchain::present()
{
	const UINT SyncInterval = mInfo.mVSyncEnabled ? 1 : 0;

	u32 PresentFlags = 0;
	if (SyncInterval == 0)
	{
		DXGI_SWAP_CHAIN_DESC1 Desc = {};
		AssertHr(mHandle->GetDesc1(&Desc));

		if ((Desc.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) != 0)
			PresentFlags |= DXGI_PRESENT_ALLOW_TEARING;
	}

	AssertHr(mHandle->Present(SyncInterval, PresentFlags));

	mFenceValues[mBackbufferIndex] = mInfo.mPresentQueue->signal();
	mBackbufferIndex               = mHandle->GetCurrentBackBufferIndex();

    mInfo.mPresentQueue->waitForFence(mFenceValues[mBackbufferIndex]);

	return mBackbufferIndex;
}