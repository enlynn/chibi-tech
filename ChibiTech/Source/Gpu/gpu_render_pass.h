#pragma once

#include <types.h>

#include "gpu_pso.h"
#include "gpu_root_signature.h"
#include "gpu_render_target.h"

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
//      FrameCache->TransitionResource(Color0, ...)
//      FrameCache->TransitionResource(Color1, ...)
//      FrameCache->TransitionResource(Depth,  ...)
//
//      GpuBuffer Materials = FrameCache->GetMaterialBuffer();
//      GpuBuffer Geometry  = FrameCache->GetGeometryBuffer();
//
//      FrameCache->TransitionResource(Materials, ...)
//      FrameCache->TransitionResource(Geometry,  ...)
//
//      gpu_command_list* CmdList = FrameCache->GetGraphicsCommandList();
//      FrameCache->FlushPendingBarriers(CmdList);
//
//      CmdList->BindRenderTarget(mRenderTarget);
//      CmdList->BindBuffer(GeometryBuffer);
//      CmdList->BindBuffer(MaterialBuffer);
//
//      CmdList->IndirectDraw(...);
//
// Overall, I think this is the way to do things. It keeps Resource state tracking *out* of the command list, and
// let's Resource state transitions be explicit, while allowing for the implementation to batch barriers.
//

class render_pass
{
public:
    render_pass() = default;

    virtual void OnInit(struct gpu_frame_cache* FrameCache)   = 0;
    virtual void OnDeinit(struct gpu_frame_cache* FrameCache) = 0;
    virtual void OnRender(struct gpu_frame_cache* FrameCache) = 0;

protected:
    gpu_root_signature mRootSignature = {};
    gpu_pso            mPso           = {};
    gpu_render_target  mRenderTarget  = {};
};