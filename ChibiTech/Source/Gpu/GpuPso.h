#pragma once

#include <initializer_list>

#include <Types.h>

#include "D3D12Common.h"
#include "GpuFormat.h"

namespace ct {
    struct ShaderResource;
}

enum class GpuRasterState : u8
{
    DefaultRaster,
    DefaultMsaa,
    DefaultCw,
    DefaultCwMsaa,
    TwoSided,
    TwoSidedMsaa,
    Shadow,
    ShadowCw,
    ShadowTwoSided,
};

enum class GpuDepthStencilState : u8
{
    Disabled,
    ReadWrite,
    ReadOnly,
    ReadOnlyReversed,
    TestEqual,
};

enum class GpuBlendState : u8
{
    Disabled,
    NoColorWrite,
    Traditional,
    PreMultiplied,
    Additive,
    TraditionAdditive,
};

// Get a default description of the rasterizer state
D3D12_RASTERIZER_DESC getRasterizerState(GpuRasterState Type);
// Get a default description of the depth-stencil state
D3D12_DEPTH_STENCIL_DESC getDepthStencilState(GpuDepthStencilState Type);
// Get a default description of the blend state
D3D12_BLEND_DESC getBlendState(GpuBlendState Type);

class GpuPso
{
public:
    GpuPso() = default;
    explicit GpuPso(ID3D12PipelineState* State) : mHandle(State) {}

    void release() { if (mHandle) ComSafeRelease(mHandle); }

    [[nodiscard]] struct ID3D12PipelineState* asHandle() const { return mHandle; }

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

    // TODO(enlynn): go through the tedium of fixing the naming convention
DEFINE_PSO_SUB_OBJECT(GpuPsoRootSignature,       D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE,        ID3D12RootSignature*);
DEFINE_PSO_SUB_OBJECT(GpuPsoVertexShader,        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS,                    D3D12_SHADER_BYTECODE);
DEFINE_PSO_SUB_OBJECT(GpuPsoPixelShader,         D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS,                    D3D12_SHADER_BYTECODE);
DEFINE_PSO_SUB_OBJECT(GpuPsoComputeShader,       D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS,                    D3D12_SHADER_BYTECODE);
DEFINE_PSO_SUB_OBJECT(GpuPsoDomainShader,        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS,                    D3D12_SHADER_BYTECODE);
DEFINE_PSO_SUB_OBJECT(GpuPsoHullShader,          D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS,                    D3D12_SHADER_BYTECODE);
DEFINE_PSO_SUB_OBJECT(GpuPsoGeometryShader,      D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS,                    D3D12_SHADER_BYTECODE);
DEFINE_PSO_SUB_OBJECT(GpuPsoAmplificationShader, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS,                    D3D12_SHADER_BYTECODE);
DEFINE_PSO_SUB_OBJECT(GpuPsoMeshShader,          D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS,                    D3D12_SHADER_BYTECODE);
DEFINE_PSO_SUB_OBJECT(GpuPsoStreamOutput,        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT,         D3D12_STREAM_OUTPUT_DESC);
DEFINE_PSO_SUB_OBJECT(GpuPsoBlend,               D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND,                 D3D12_BLEND_DESC);
DEFINE_PSO_SUB_OBJECT(GpuPsoSampleMask,          D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK,           UINT);
DEFINE_PSO_SUB_OBJECT(GpuPsoRasterState,         D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER,            D3D12_RASTERIZER_DESC);
DEFINE_PSO_SUB_OBJECT(GpuPsoDepthStencil,        D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL,         D3D12_DEPTH_STENCIL_DESC);
DEFINE_PSO_SUB_OBJECT(GpuPsoInputLayout,         D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT,          D3D12_INPUT_LAYOUT_DESC);
DEFINE_PSO_SUB_OBJECT(GpuPsoIbStripCute,         D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE,    D3D12_INDEX_BUFFER_STRIP_CUT_VALUE);
DEFINE_PSO_SUB_OBJECT(GpuPsoPrimitiveTopology,   D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY,    D3D12_PRIMITIVE_TOPOLOGY_TYPE);
DEFINE_PSO_SUB_OBJECT(GpuPsoRtvFormats,          D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS, D3D12_RT_FORMAT_ARRAY);
DEFINE_PSO_SUB_OBJECT(GpuPsoDepthStencilFormat,  D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT,  DXGI_FORMAT);
DEFINE_PSO_SUB_OBJECT(GpuPsoSampleDesc,          D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC,           DXGI_SAMPLE_DESC);
DEFINE_PSO_SUB_OBJECT(GpuPsoNodeMask,            D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK,             UINT);
DEFINE_PSO_SUB_OBJECT(GpuPsoCachedPso,           D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO,            D3D12_CACHED_PIPELINE_STATE);
DEFINE_PSO_SUB_OBJECT(GpuPsoTypeFlags,           D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS,                 D3D12_PIPELINE_STATE_FLAGS);
DEFINE_PSO_SUB_OBJECT(GpuPsoDepthStencil1,       D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1,        D3D12_DEPTH_STENCIL_DESC1);
DEFINE_PSO_SUB_OBJECT(GpuPsoViewInstancing,      D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING,       D3D12_VIEW_INSTANCING_DESC);
// D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MAX_VALID

//
// An Example PSO Creation using the Test Triangle Pipeline
//
// GpuGraphicsPsoBuilder PsoBuilder = GpuGraphicsPsoBuilder::builder();
// PsoBuilder.setRootSignature(RootSig)
//           .setVertexShader(VertexShader)
//           .setPixelShader(PixelShader)
//           .SetRTVFormats(1, { SwapchainFormat });
// GpuPso PSO = PsoBuilder.compile(FrameCache);
//

// Create Graphics PSO
struct GpuGraphicsPsoBuilder
{
    static GpuGraphicsPsoBuilder builder();            // Retrieved a default initialized struct
    GpuPso compile(class GpuFrameCache* FrameCache);  // Compiles the PSO, returning the new PSO handle

    GpuGraphicsPsoBuilder& setRootSignature(class GpuRootSignature *RootSignature);

    // Set Shader Stages, mVertex and mPixel Stages are required.
    GpuGraphicsPsoBuilder& setVertexShader(ct::ShaderResource& VertexShader);
    GpuGraphicsPsoBuilder& setPixelShader(ct::ShaderResource& PixelShader);
    GpuGraphicsPsoBuilder& setHullShader(ct::ShaderResource& HullShader);
    GpuGraphicsPsoBuilder& setDomainShader(ct::ShaderResource& DomainShader);
    GpuGraphicsPsoBuilder& setGeometryShader(ct::ShaderResource& GeometryShader);

    // Blend State
    GpuGraphicsPsoBuilder& setDefaultBlendState(GpuBlendState BlendState); // Selects a default configuration for blending
    GpuGraphicsPsoBuilder& setBlendState(D3D12_BLEND_DESC BlendDesc);

    // Rasterizer State
    GpuGraphicsPsoBuilder& setDefaultRasterState(GpuRasterState RasterState); // Selects a default configuration for rasterization
    GpuGraphicsPsoBuilder& setRasterState(D3D12_RASTERIZER_DESC RasterDesc);

    // Depth Stencil State
    GpuGraphicsPsoBuilder& setDefaultDepthStencilState(
            GpuDepthStencilState DepthStencil,
            GpuFormat            DepthFormat); // Selects a default configuration for depth/stencil
    GpuGraphicsPsoBuilder& setDepthStencilState(D3D12_DEPTH_STENCIL_DESC DepthStencilDesc, DXGI_FORMAT DepthFormat);
    GpuGraphicsPsoBuilder& setDepthFormat(DXGI_FORMAT DepthFormat);

        // Set Render Target Formats
    GpuGraphicsPsoBuilder& setRenderTargetFormats(std::initializer_list<DXGI_FORMAT> Formats);

    GpuGraphicsPsoBuilder& setSampleQuality(u32 Count = 1, u32 Quality = 0);
    GpuGraphicsPsoBuilder& setTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE Topology);

    GpuPsoRootSignature       mRootSignature;
    GpuPsoVertexShader        mVertexShader;
    GpuPsoPixelShader         mPixelShader;
    GpuPsoDomainShader        mDomainShader;
    GpuPsoHullShader          mHullShader;
    GpuPsoGeometryShader      mGeometryShader;
    GpuPsoBlend               mBlend;
    GpuPsoRasterState         mRasterizer;
    GpuPsoDepthStencil        mDepthStencil;
    GpuPsoPrimitiveTopology   mTopology;
    GpuPsoRtvFormats          mRenderTargetFormats;
    GpuPsoDepthStencilFormat  mDepthFormat;
    GpuPsoSampleDesc          mSampleDesc;

    // NOTE(enlynn): gpu_pso_sample_mask
    // NOTE(enlynn): gpu_pso_ib_strip_cut
    // NOTE(enlynn): gpu_pso_cached_pso
    // NOTE(enlynn): gpu_pso_type_flags, specifies debug.
    // NOTE(enlynn): gpu_pso_depth_stencil1, more advanced version of DepthStencil
    // NOTE(enlynn): gpu_pso_view_instancing
};

// Create mCompute PSO

struct GpuComputePsoBuilder
{
    static GpuComputePsoBuilder builder();             // Retrieved a default initialized struct
    GpuPso compile(class GpuFrameCache* FrameCache);  // Compiles the PSO, returning the new handle

    GpuComputePsoBuilder& setRootSignature(class GpuRootSignature *RootSignature);
    GpuComputePsoBuilder& setComputeShader(ct::ShaderResource& VertexShader);

    GpuPsoRootSignature mRootSignature; //Required.
    GpuPsoComputeShader mComputeShader; //Required.

    // NOTE(enlynn): gpu_pso_cached_pso
    // NOTE(enlynn): gpu_pso_type_flags, specifies debug.
};

struct GpuMeshPsoBuilder
{
    static GpuMeshPsoBuilder builder();                // Retrieved a default initialized struct
    GpuPso compile(class GpuFrameCache* FrameCache);  // Compiles the PSO, returning the new handle

    GpuPsoRootSignature       mRootSignature;          //Required.
    GpuPsoMeshShader          mMeshShader;             //Required.
    GpuPsoAmplificationShader mAmplificationShader;
    GpuPsoPixelShader         mPixelShader;
    GpuPsoBlend                mBlend;
    GpuPsoRasterState         mRasterizer;
    GpuPsoDepthStencil        mDepthStencil;
    GpuPsoPrimitiveTopology   mTopology;
    GpuPsoRtvFormats          mRenderTargetFormats;
    GpuPsoDepthStencilFormat mDepthFormat;
    GpuPsoSampleDesc          mSampleDesc;

    // NOTE(enlynn): gpu_pso_sample_mask
    // NOTE(enlynn): gpu_pso_ib_strip_cut
    // NOTE(enlynn): gpu_pso_cached_pso
    // NOTE(enlynn): gpu_pso_type_flags, specifies debug.
    // NOTE(enlynn): gpu_pso_depth_stencil1, more advanced version of DepthStencil
    // NOTE(enlynn): gpu_pso_view_instancing
};
