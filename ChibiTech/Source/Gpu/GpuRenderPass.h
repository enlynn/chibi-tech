#pragma once

#include <types.h>

#include "GpuPso.h"
#include "GpuRootSignature.h"
#include "GpuRenderTarget.h"

//
// struct renderpass {};
//
// The renderpass knows the state it wants everything in.
// - So, at the start of the pass, it transitions everything.
//
// The renderpass takes in the frame_cache when it renders to get a list of global state.
// There is no need to automatically transition resources.
//
// global_data ->
// --- darray<gpu_resource_states> ResourceStateMap;
//
// Renderpass::Render(FrameCache)
//      texture* Color0 = FrameCache->GetLastColor0Framebuffer(ColorFormat);
//      texture* Color1 = FrameCache->GetLastColor1Framebuffer(ColorFormat);
//      texture* Depth  = FrameCache->GetLastDepthBuffer(DepthFormat);
//
//      FrameCache->transitionResource(Color0, ...)
//      FrameCache->transitionResource(Color1, ...)
//      FrameCache->transitionResource(Depth,  ...)
//
//      GpuBuffer Materials = FrameCache->GetMaterialBuffer();
//      GpuBuffer Geometry  = FrameCache->GetGeometryBuffer();
//
//      FrameCache->transitionResource(Materials, ...)
//      FrameCache->transitionResource(Geometry,  ...)
//
//      GpuCommandList* CmdList = FrameCache->getGraphicsCommandList();
//      FrameCache->FlushPendingBarriers(CmdList);
//
//      CmdList->bindRenderTarget(mRenderTarget);
//      CmdList->BindBuffer(GeometryBuffer);
//      CmdList->BindBuffer(MaterialBuffer);
//
//      CmdList->IndirectDraw(...);
//
// Overall, I think this is the way to do things. It keeps Resource state tracking *out* of the command list, and
// let's Resource state transitions be explicit, while allowing for the implementation to batch barriers.
//

class RenderPass
{
public:
    RenderPass() = default;

    virtual void onInit(struct GpuFrameCache* FrameCache)   = 0;
    virtual void onDeinit(struct GpuFrameCache* FrameCache) = 0;
    virtual void onRender(struct GpuFrameCache* FrameCache) = 0;

protected:
    GpuRootSignature mRootSignature = {};
    GpuPso           mPso           = {};
    GpuRenderTarget  mRenderTarget  = {};
};