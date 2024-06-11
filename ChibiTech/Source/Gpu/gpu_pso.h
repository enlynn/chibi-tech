#pragma once

#include <initializer_list>

#include <Types.h>

#include "D3D12Common.h"
#include "gpu_format.h"

namespace ct {
    struct ShaderResource;
}

enum class gpu_raster_state : u8
{
    default_raster,
    default_msaa,
    default_cw,
    default_cw_msaa,
    two_sided,
    two_sided_msaa,
    shadow,
    shadow_cw,
    shadow_two_sided,
};

enum class gpu_depth_stencil_state : u8
{
    disabled,
    read_write,
    read_only,
    read_only_reversed,
    test_equal,
};

enum class gpu_blend_state : u8
{
    disabled,
    no_color_write,
    traditional,
    pre_multiplied,
    additive,
    tradition_additive,
};

// Get a default description of the rasterizer state
D3D12_RASTERIZER_DESC GetRasterizerState(gpu_raster_state Type);
// Get a default description of the depth-stencil state
D3D12_DEPTH_STENCIL_DESC GetDepthStencilState(gpu_depth_stencil_state Type);
// Get a default description of the blend state
D3D12_BLEND_DESC GetBlendState(gpu_blend_state Type);

class gpu_pso 
{
public:
    gpu_pso() = default;
    gpu_pso(ID3D12PipelineState* State) : mHandle(State) {}

    void Release() { if (mHandle) ComSafeRelease(mHandle); }

    struct ID3D12PipelineState* AsHandle() const { return mHandle; }

private:
    struct ID3D12PipelineState *mHandle = nullptr;
};

//
// Pipeline State Stream Implementation for Sub-Object Types
//

#define DEFINE_PSO_SUB_OBJECT(Name, Suboject, Struct)           \
    struct alignas(void*) Name {                                \
        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE mType   = Suboject; \
        Struct                              mObject = {};       \
    };

DEFINE_PSO_SUB_OBJECT(gpu_pso_root_signature,       D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE,        ID3D12RootSignature*);
DEFINE_PSO_SUB_OBJECT(gpu_pso_vertex_shader,        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS,                    D3D12_SHADER_BYTECODE);
DEFINE_PSO_SUB_OBJECT(gpu_pso_pixel_shader,         D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS,                    D3D12_SHADER_BYTECODE);
DEFINE_PSO_SUB_OBJECT(gpu_pso_compute_shader,       D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS,                    D3D12_SHADER_BYTECODE);
DEFINE_PSO_SUB_OBJECT(gpu_pso_domain_shader,        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS,                    D3D12_SHADER_BYTECODE);
DEFINE_PSO_SUB_OBJECT(gpu_pso_hull_shader,          D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS,                    D3D12_SHADER_BYTECODE);
DEFINE_PSO_SUB_OBJECT(gpu_pso_geometry_shader,      D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS,                    D3D12_SHADER_BYTECODE);
DEFINE_PSO_SUB_OBJECT(gpu_pso_amplification_shader, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS,                    D3D12_SHADER_BYTECODE);
DEFINE_PSO_SUB_OBJECT(gpu_pso_mesh_shader,          D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS,                    D3D12_SHADER_BYTECODE);
DEFINE_PSO_SUB_OBJECT(gpu_pso_stream_output,        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT,         D3D12_STREAM_OUTPUT_DESC);
DEFINE_PSO_SUB_OBJECT(gpu_pso_blend,                D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND,                 D3D12_BLEND_DESC);
DEFINE_PSO_SUB_OBJECT(gpu_pso_sample_mask,          D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK,           UINT);
DEFINE_PSO_SUB_OBJECT(gpu_pso_raster_state,         D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER,            D3D12_RASTERIZER_DESC);
DEFINE_PSO_SUB_OBJECT(gpu_pso_depth_stencil,        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL,         D3D12_DEPTH_STENCIL_DESC);
DEFINE_PSO_SUB_OBJECT(gpu_pso_input_layout,         D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT,          D3D12_INPUT_LAYOUT_DESC);
DEFINE_PSO_SUB_OBJECT(gpu_pso_ib_strip_cut,         D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE,    D3D12_INDEX_BUFFER_STRIP_CUT_VALUE);
DEFINE_PSO_SUB_OBJECT(gpu_pso_primitive_topology,   D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY,    D3D12_PRIMITIVE_TOPOLOGY_TYPE);
DEFINE_PSO_SUB_OBJECT(gpu_pso_rtv_formats,          D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS, D3D12_RT_FORMAT_ARRAY);
DEFINE_PSO_SUB_OBJECT(gpu_pso_depth_stencil_format, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT,  DXGI_FORMAT);
DEFINE_PSO_SUB_OBJECT(gpu_pso_sample_desc,          D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC,           DXGI_SAMPLE_DESC);
DEFINE_PSO_SUB_OBJECT(gpu_pso_nodemask,             D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK,             UINT);
DEFINE_PSO_SUB_OBJECT(gpu_pso_cached_pso,           D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO,            D3D12_CACHED_PIPELINE_STATE);
DEFINE_PSO_SUB_OBJECT(gpu_pso_type_flags,           D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS,                 D3D12_PIPELINE_STATE_FLAGS);
DEFINE_PSO_SUB_OBJECT(gpu_pso_depth_stencil1,       D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1,        D3D12_DEPTH_STENCIL_DESC1);
DEFINE_PSO_SUB_OBJECT(gpu_pso_view_instancing,      D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING,       D3D12_VIEW_INSTANCING_DESC);
// D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MAX_VALID

//
// An Example PSO Creation using the Test Triangle Pipeline
//
// gpu_graphics_pso_builder PsoBuilder = gpu_graphics_pso_builder::Builder();
// PsoBuilder.SetRootSignature(RootSig)
//           .SetVertexShader(VertexShader)
//           .SetPixelShader(PixelShader)
//           .SetRTVFormats(1, { SwapchainFormat });
// gpu_pso PSO = PsoBuilder.Compile(FrameCache);
//

// Create Graphics PSO
struct gpu_graphics_pso_builder
{
    static gpu_graphics_pso_builder Builder();            // Retrieved a default initialized struct
    gpu_pso Compile(struct gpu_frame_cache* FrameCache);  // Compiles the PSO, returning the new PSO handle

    gpu_graphics_pso_builder& SetRootSignature(class gpu_root_signature* RootSignature);

    // Set Shader Stages, mVertex and mPixel Stages are required.
    gpu_graphics_pso_builder& SetVertexShader(  ct::ShaderResource& VertexShader);
    gpu_graphics_pso_builder& SetPixelShader(   ct::ShaderResource& PixelShader);
    gpu_graphics_pso_builder& SetHullShader(    ct::ShaderResource& HullShader);
    gpu_graphics_pso_builder& SetDomainShader(  ct::ShaderResource& DomainShader);
    gpu_graphics_pso_builder& SetGeometryShader(ct::ShaderResource& GeometryShader);

    // Blend State
    gpu_graphics_pso_builder& SetDefaultBlendState(gpu_blend_state BlendState); // Selects a default configuration for blending
    gpu_graphics_pso_builder& SetBlendState(D3D12_BLEND_DESC BlendDesc);

    // Rasterizer State
    gpu_graphics_pso_builder& SetDefaultRasterState(gpu_raster_state RasterState); // Selects a default configuration for rasterization
    gpu_graphics_pso_builder& SetRasterState(D3D12_RASTERIZER_DESC RasterDesc);

    // Depth Stencil State
    gpu_graphics_pso_builder& SetDefaultDepthStencilState(
        gpu_depth_stencil_state DepthStencil,
        gpu_format              DepthFormat); // Selects a default configuration for depth/stencil
    gpu_graphics_pso_builder& SetDepthStencilState(D3D12_DEPTH_STENCIL_DESC DepthStencilDesc, DXGI_FORMAT DepthFormat);
    gpu_graphics_pso_builder& SetDepthFormat(DXGI_FORMAT DepthFormat);

        // Set Render Target Formats
    gpu_graphics_pso_builder& SetRenderTargetFormats(std::initializer_list<DXGI_FORMAT> Formats);

    gpu_graphics_pso_builder& SetSampleQuality(u32 Count = 1, u32 Quality = 0);
    gpu_graphics_pso_builder& SetTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE Topology);

    gpu_pso_root_signature       mRootSignature;
    gpu_pso_vertex_shader        mVertexShader;
    gpu_pso_pixel_shader         mPixelShader;
    gpu_pso_domain_shader        mDomainShader;
    gpu_pso_hull_shader          mHullShader;
    gpu_pso_geometry_shader      mGeometryShader;
    gpu_pso_blend                mBlend;
    gpu_pso_raster_state         mRasterizer;
    gpu_pso_depth_stencil        mDepthStencil;
    gpu_pso_primitive_topology   mTopology;
    gpu_pso_rtv_formats          mRenderTargetFormats;
    gpu_pso_depth_stencil_format mDepthFormat;
    gpu_pso_sample_desc          mSampleDesc;

    // NOTE(enlynn): gpu_pso_sample_mask
    // NOTE(enlynn): gpu_pso_ib_strip_cut
    // NOTE(enlynn): gpu_pso_cached_pso
    // NOTE(enlynn): gpu_pso_type_flags, specifies debug.
    // NOTE(enlynn): gpu_pso_depth_stencil1, more advanced version of depth_stencil
    // NOTE(enlynn): gpu_pso_view_instancing
};

// Create mCompute PSO

struct gpu_compute_pso_builder
{
    static gpu_compute_pso_builder Builder();             // Retrieved a default initialized struct
    gpu_pso Compile(struct gpu_frame_cache* FrameCache);  // Compiles the PSO, returning the new handle

    gpu_compute_pso_builder& SetRootSignature(class gpu_root_signature* RootSignature);
    gpu_compute_pso_builder& SetComputeShader(ct::ShaderResource& VertexShader);

    gpu_pso_root_signature mRootSignature; //Required.
    gpu_pso_compute_shader mComputeShader; //Required.

    // NOTE(enlynn): gpu_pso_cached_pso
    // NOTE(enlynn): gpu_pso_type_flags, specifies debug.
};

struct gpu_mesh_pso_builder
{
    static gpu_mesh_pso_builder Builder();                // Retrieved a default initialized struct
    gpu_pso Compile(struct gpu_frame_cache* FrameCache);  // Compiles the PSO, returning the new handle

    gpu_pso_root_signature       mRootSignature;          //Required.
    gpu_pso_mesh_shader          mMeshShader;             //Required.
    gpu_pso_amplification_shader mAmplificationShader;
    gpu_pso_pixel_shader         mPixelShader;
    gpu_pso_blend                mBlend;
    gpu_pso_raster_state         mRasterizer;
    gpu_pso_depth_stencil        mDepthStencil;
    gpu_pso_primitive_topology   mTopology;
    gpu_pso_rtv_formats          mRenderTargetFormats;
    gpu_pso_depth_stencil_format mDepthFormat;
    gpu_pso_sample_desc          mSampleDesc;

    // NOTE(enlynn): gpu_pso_sample_mask
    // NOTE(enlynn): gpu_pso_ib_strip_cut
    // NOTE(enlynn): gpu_pso_cached_pso
    // NOTE(enlynn): gpu_pso_type_flags, specifies debug.
    // NOTE(enlynn): gpu_pso_depth_stencil1, more advanced version of depth_stencil
    // NOTE(enlynn): gpu_pso_view_instancing
};
