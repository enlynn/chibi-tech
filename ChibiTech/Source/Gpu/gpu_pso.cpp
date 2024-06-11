#include "gpu_pso.h"
#include "gpu_device.h"
#include "gpu_root_signature.h"
#include "GpuState.h"
#include "gpu_device.h"

#include <Platform/Assert.h>
#include <Systems/ShaderLoader.h>

// SOURCE for constants: 
// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/GraphicsCommon.cpp

var_global constexpr D3D12_RASTERIZER_DESC cRasterizerDefault = {
    D3D12_FILL_MODE_SOLID,                     // Fill Mode
    D3D12_CULL_MODE_NONE,                      // Cull mode
    TRUE,                                      // Winding order (true == ccw)
    D3D12_DEFAULT_DEPTH_BIAS,                  // Depth Bias
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
    D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,     // Slope scaled depth bias
    TRUE,                                      // Depth Clip Enable
    FALSE,                                     // Multisample Enable
    FALSE,                                     // Antialiased enable
    0,                                         // Forced sample Count
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
};

var_global constexpr D3D12_RASTERIZER_DESC cRasterizerDefaultMsaa = {
    D3D12_FILL_MODE_SOLID,                     // Fill Mode
    D3D12_CULL_MODE_BACK,                      // Cull mode
    TRUE,                                      // Winding order (true == ccw)
    D3D12_DEFAULT_DEPTH_BIAS,                  // Depth Bias
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
    D3D12_DEFAULT_DEPTH_BIAS,                  // Slope scaled depth bias
    TRUE,                                      // Depth Clip Enable
    TRUE,                                      // Multisample Enable
    FALSE,                                     // Antialiased enable
    0,                                         // Forced sample Count
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
};
    
    // Clockwise winding mVertex winding
    
var_global constexpr D3D12_RASTERIZER_DESC cRasterizerDefaultCw = {
    D3D12_FILL_MODE_SOLID,                     // Fill Mode
    D3D12_CULL_MODE_NONE,                      // Cull mode
    FALSE,                                     // Winding order (true == ccw)
    D3D12_DEFAULT_DEPTH_BIAS,                  // Depth Bias
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
    D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,     // Slope scaled depth bias
    TRUE,                                      // Depth Clip Enable
    FALSE,                                     // Multisample Enable
    FALSE,                                     // Antialiased enable
    0,                                         // Forced sample Count
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
};
    
var_global constexpr D3D12_RASTERIZER_DESC cRasterizerDefaultCwMsaa = {
    D3D12_FILL_MODE_SOLID,                     // Fill Mode
    D3D12_CULL_MODE_BACK,                      // Cull mode
    FALSE,                                     // Winding order (true == ccw)
    D3D12_DEFAULT_DEPTH_BIAS,                  // Depth Bias
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
    D3D12_DEFAULT_DEPTH_BIAS,                  // Slope scaled depth bias
    TRUE,                                      // Depth Clip Enable
    TRUE,                                      // Multisample Enable
    FALSE,                                     // Antialiased enable
    0,                                         // Forced sample Count
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
};
    
    // CCW Two Sided
    
    var_global constexpr D3D12_RASTERIZER_DESC cRasterizerTwoSided = {
    D3D12_FILL_MODE_SOLID,                     // Fill Mode
    D3D12_CULL_MODE_NONE,                      // Cull mode
    TRUE,                                      // Winding order (true == ccw)
    D3D12_DEFAULT_DEPTH_BIAS,                  // Depth Bias
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
    D3D12_DEFAULT_DEPTH_BIAS,                  // Slope scaled depth bias
    TRUE,                                      // Depth Clip Enable
    FALSE,                                     // Multisample Enable
    FALSE,                                     // Antialiased enable
    0,                                         // Forced sample Count
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
    };
    
var_global constexpr D3D12_RASTERIZER_DESC cRasterizerTwoSidedMsaa = {
    D3D12_FILL_MODE_SOLID,                     // Fill Mode
    D3D12_CULL_MODE_NONE,                      // Cull mode
    TRUE,                                      // Winding order (true == ccw)
    D3D12_DEFAULT_DEPTH_BIAS,                  // Depth Bias
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
    D3D12_DEFAULT_DEPTH_BIAS,                  // Slope scaled depth bias
    TRUE,                                      // Depth Clip Enable
    TRUE,                                      // Multisample Enable
    FALSE,                                     // Antialiased enable
    0,                                         // Forced sample Count
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
};
    
var_global constexpr D3D12_RASTERIZER_DESC cRasterizerShadow = {
    D3D12_FILL_MODE_SOLID,                     // Fill Mode
    D3D12_CULL_MODE_BACK,                      // Cull mode
    TRUE,                                      // Winding order (true == ccw)
    -100,                                      // Depth Bias
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
    -1.5,                                      // Slope scaled depth bias
    TRUE,                                      // Depth Clip Enable
    FALSE,                                     // Multisample Enable
    FALSE,                                     // Antialiased enable
    0,                                         // Forced sample Count
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
};
    
var_global constexpr D3D12_RASTERIZER_DESC cRasterizerShadowCW = {
    D3D12_FILL_MODE_SOLID,                     // Fill Mode
    D3D12_CULL_MODE_BACK,                      // Cull mode
    FALSE,                                     // Winding order (true == ccw)
    -100,                                      // Depth Bias
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
    -1.5,                                      // Slope scaled depth bias
    TRUE,                                      // Depth Clip Enable
    FALSE,                                     // Multisample Enable
    FALSE,                                     // Antialiased enable
    0,                                         // Forced sample Count
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
};
    
var_global constexpr D3D12_RASTERIZER_DESC cRasterizerShadowTwoSided = {
    D3D12_FILL_MODE_SOLID,                     // Fill Mode
    D3D12_CULL_MODE_NONE,                      // Cull mode
    TRUE,                                      // Winding order (true == ccw)
    -100,                                      // Depth Bias
    D3D12_DEFAULT_DEPTH_BIAS_CLAMP,            // Depth Bias clamp 
    -1.5,                                      // Slope scaled depth bias
    TRUE,                                      // Depth Clip Enable
    FALSE,                                     // Multisample Enable
    FALSE,                                     // Antialiased enable
    0,                                         // Forced sample Count
    D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF, // Conservative raster
};

var_global constexpr D3D12_DEPTH_STENCIL_DESC cDepthStateDisabled = {
    FALSE,                            // DepthEnable
    D3D12_DEPTH_WRITE_MASK_ZERO,      // DepthWriteMask
    D3D12_COMPARISON_FUNC_ALWAYS,     // DepthFunc
    FALSE,                            // StencilEnable
    D3D12_DEFAULT_STENCIL_READ_MASK,  // StencilReadMask
    D3D12_DEFAULT_STENCIL_WRITE_MASK, // StencilWriteMask
    {                                 // FrontFace
        D3D12_STENCIL_OP_KEEP,            // StencilFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
    },
    {                                 // BackFace
        D3D12_STENCIL_OP_KEEP,            // StencilFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
    },
};

var_global constexpr D3D12_DEPTH_STENCIL_DESC cDepthStateReadWrite = {
    TRUE,                             // DepthEnable
    D3D12_DEPTH_WRITE_MASK_ALL,       // DepthWriteMask
    D3D12_COMPARISON_FUNC_LESS,       // DepthFunc
    FALSE,                            // StencilEnable
    D3D12_DEFAULT_STENCIL_READ_MASK,  // StencilReadMask
    D3D12_DEFAULT_STENCIL_WRITE_MASK, // StencilWriteMask
    {                                 // FrontFace
        D3D12_STENCIL_OP_KEEP,            // StencilFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
    },
    {                                 // BackFace
        D3D12_STENCIL_OP_KEEP,            // StencilFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
    },
};

var_global constexpr D3D12_DEPTH_STENCIL_DESC cDepthStateReadOnly = {
    TRUE,                             // DepthEnable
    D3D12_DEPTH_WRITE_MASK_ZERO,      // DepthWriteMask
    D3D12_COMPARISON_FUNC_GREATER_EQUAL, // DepthFunc
    FALSE,                            // StencilEnable
    D3D12_DEFAULT_STENCIL_READ_MASK,  // StencilReadMask
    D3D12_DEFAULT_STENCIL_WRITE_MASK, // StencilWriteMask
    {                                 // FrontFace
        D3D12_STENCIL_OP_KEEP,            // StencilFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
    },
    {                                 // BackFace
        D3D12_STENCIL_OP_KEEP,            // StencilFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
    },
};
    
var_global constexpr D3D12_DEPTH_STENCIL_DESC cDepthStateReadOnlyReversed = {
    TRUE,                             // DepthEnable
    D3D12_DEPTH_WRITE_MASK_ZERO,      // DepthWriteMask
    D3D12_COMPARISON_FUNC_LESS,       // DepthFunc
    FALSE,                            // StencilEnable
    D3D12_DEFAULT_STENCIL_READ_MASK,  // StencilReadMask
    D3D12_DEFAULT_STENCIL_WRITE_MASK, // StencilWriteMask
    {                                 // FrontFace
        D3D12_STENCIL_OP_KEEP,            // StencilFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
    },
    {                                 // BackFace
        D3D12_STENCIL_OP_KEEP,            // StencilFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
    },
};

var_global constexpr D3D12_DEPTH_STENCIL_DESC cDepthStateTestEqual = {
    TRUE,                             // DepthEnable
    D3D12_DEPTH_WRITE_MASK_ZERO,      // DepthWriteMask
    D3D12_COMPARISON_FUNC_EQUAL,      // DepthFunc
    FALSE,                            // StencilEnable
    D3D12_DEFAULT_STENCIL_READ_MASK,  // StencilReadMask
    D3D12_DEFAULT_STENCIL_WRITE_MASK, // StencilWriteMask
    {                                 // FrontFace
        D3D12_STENCIL_OP_KEEP,            // StencilFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
    },
    {                                 // BackFace
        D3D12_STENCIL_OP_KEEP,            // StencilFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilDepthFailOp
        D3D12_STENCIL_OP_KEEP,            // StencilPassOp
        D3D12_COMPARISON_FUNC_ALWAYS,     // StencilFunc;
    },
};
    
// alpha blend
var_global constexpr D3D12_BLEND_DESC cBlendNoColorWrite = {
    FALSE,                             // AlphaToCoverageEnable
    FALSE,                             // IndependentBlendEnable
    {                                  // RenderTarget[8]
        {                                  // index = 0
            FALSE,                             // BlendEnable
            FALSE,                             // LogicOpEnable
            D3D12_BLEND_SRC_ALPHA,             // SrcBlend
            D3D12_BLEND_INV_SRC_ALPHA,         // DestBlend
            D3D12_BLEND_OP_ADD,                // BlendOp
            D3D12_BLEND_ONE,                   // SrcBlendAlpha
            D3D12_BLEND_INV_SRC_ALPHA,         // DestBlendAlpha
            D3D12_BLEND_OP_ADD,                // BlendOpAlpha
            D3D12_LOGIC_OP_NOOP,               // LogicOp
            0,                                 // RenderTargetWriteMask
        }, {}, {}, {}, {}, {}, {}, {} // Make sure to default init the other slots
    },
};
    
var_global constexpr D3D12_BLEND_DESC cBlendDisable = {
    FALSE,                             // AlphaToCoverageEnable
    FALSE,                             // IndependentBlendEnable
    {                                  // RenderTarget[8]
        {                                  // index = 0
            FALSE,                             // BlendEnable
            FALSE,                             // LogicOpEnable
            D3D12_BLEND_SRC_ALPHA,             // SrcBlend
            D3D12_BLEND_INV_SRC_ALPHA,         // DestBlend
            D3D12_BLEND_OP_ADD,                // BlendOp
            D3D12_BLEND_ONE,                   // SrcBlendAlpha
            D3D12_BLEND_INV_SRC_ALPHA,         // DestBlendAlpha
            D3D12_BLEND_OP_ADD,                // BlendOpAlpha
            D3D12_LOGIC_OP_NOOP,               // LogicOp
            D3D12_COLOR_WRITE_ENABLE_ALL,      // RenderTargetWriteMask
        }, {}, {}, {}, {}, {}, {}, {} // Make sure to default init the other slots
    },
};
    
var_global constexpr D3D12_BLEND_DESC cBlendTraditional = {
    FALSE,                             // AlphaToCoverageEnable
    FALSE,                             // IndependentBlendEnable
    {                                  // RenderTarget[8]
        {                                  // index = 0
            TRUE,                              // BlendEnable
            FALSE,                             // LogicOpEnable
            D3D12_BLEND_SRC_ALPHA,             // SrcBlend
            D3D12_BLEND_INV_SRC_ALPHA,         // DestBlend
            D3D12_BLEND_OP_ADD,                // BlendOp
            D3D12_BLEND_ONE,                   // SrcBlendAlpha
            D3D12_BLEND_INV_SRC_ALPHA,         // DestBlendAlpha
            D3D12_BLEND_OP_ADD,                // BlendOpAlpha
            D3D12_LOGIC_OP_NOOP,               // LogicOp
            D3D12_COLOR_WRITE_ENABLE_ALL,      // RenderTargetWriteMask
        }, {}, {}, {}, {}, {}, {}, {} // Make sure to default init the other slots
    },
};
    
var_global constexpr D3D12_BLEND_DESC cBlendPreMultiplied = {
    FALSE,                             // AlphaToCoverageEnable
    FALSE,                             // IndependentBlendEnable
    {                                  // RenderTarget[8]
        {                                  // index = 0
            TRUE,                              // BlendEnable
            FALSE,                             // LogicOpEnable
            D3D12_BLEND_ONE,                   // SrcBlend
            D3D12_BLEND_INV_SRC_ALPHA,         // DestBlend
            D3D12_BLEND_OP_ADD,                // BlendOp
            D3D12_BLEND_ONE,                   // SrcBlendAlpha
            D3D12_BLEND_INV_SRC_ALPHA,         // DestBlendAlpha
            D3D12_BLEND_OP_ADD,                // BlendOpAlpha
            D3D12_LOGIC_OP_NOOP,               // LogicOp
            D3D12_COLOR_WRITE_ENABLE_ALL,      // RenderTargetWriteMask
        }, {}, {}, {}, {}, {}, {}, {} // Make sure to default init the other slots
    },
};
    
var_global constexpr D3D12_BLEND_DESC cBlendAdditive = {
    FALSE,                             // AlphaToCoverageEnable
    FALSE,                             // IndependentBlendEnable
    {                                  // RenderTarget[8]
        {                                  // index = 0
            TRUE,                              // BlendEnable
            FALSE,                             // LogicOpEnable
            D3D12_BLEND_ONE,                   // SrcBlend
            D3D12_BLEND_ONE,                   // DestBlend
            D3D12_BLEND_OP_ADD,                // BlendOp
            D3D12_BLEND_ONE,                   // SrcBlendAlpha
            D3D12_BLEND_INV_SRC_ALPHA,         // DestBlendAlpha
            D3D12_BLEND_OP_ADD,                // BlendOpAlpha
            D3D12_LOGIC_OP_NOOP,               // LogicOp
            D3D12_COLOR_WRITE_ENABLE_ALL,      // RenderTargetWriteMask
        }, {}, {}, {}, {}, {}, {}, {} // Make sure to default init the other slots
    },
};
    
var_global constexpr D3D12_BLEND_DESC cBlendTraditionalAdditive = {
    FALSE,                             // AlphaToCoverageEnable
    FALSE,                             // IndependentBlendEnable
    {                                  // RenderTarget[8]
        {                                  // index = 0
            TRUE,                              // BlendEnable
            FALSE,                             // LogicOpEnable
            D3D12_BLEND_SRC_ALPHA,             // SrcBlend
            D3D12_BLEND_ONE,                   // DestBlend
            D3D12_BLEND_OP_ADD,                // BlendOp
            D3D12_BLEND_ONE,                   // SrcBlendAlpha
            D3D12_BLEND_INV_SRC_ALPHA,         // DestBlendAlpha
            D3D12_BLEND_OP_ADD,                // BlendOpAlpha
            D3D12_LOGIC_OP_NOOP,               // LogicOp
            D3D12_COLOR_WRITE_ENABLE_ALL,      // RenderTargetWriteMask
        }, {}, {}, {}, {}, {}, {}, {} // Make sure to default init the other slots
    },
};

// Get a default description of the rasterizer state
D3D12_RASTERIZER_DESC 
GetRasterizerState(gpu_raster_state type)
{
    D3D12_RASTERIZER_DESC Result = {};
    
    if (type == gpu_raster_state::default_raster)
        Result = cRasterizerDefault;
    else if (type == gpu_raster_state::default_msaa)
        Result = cRasterizerDefaultMsaa;
    else if (type == gpu_raster_state::default_cw)
        Result = cRasterizerDefaultCw;
    else if (type == gpu_raster_state::default_cw_msaa)
        Result = cRasterizerDefaultCwMsaa;
    else if (type == gpu_raster_state::two_sided)
        Result = cRasterizerTwoSided;
    else if (type == gpu_raster_state::two_sided_msaa)
        Result = cRasterizerTwoSidedMsaa;
    else if (type == gpu_raster_state::shadow)
        Result = cRasterizerShadow;
    else if (type == gpu_raster_state::shadow_cw)
        Result = cRasterizerShadowCW;
    else if (type == gpu_raster_state::shadow_two_sided)
        Result = cRasterizerShadowTwoSided;
    
    return Result;
}

// Get a default description of the depth-stencil state
D3D12_DEPTH_STENCIL_DESC 
GetDepthStencilState(gpu_depth_stencil_state type)
{
    D3D12_DEPTH_STENCIL_DESC Result = {};
    
    if (type == gpu_depth_stencil_state::disabled)
        Result = cDepthStateDisabled;
    else if (type == gpu_depth_stencil_state::read_write)
        Result = cDepthStateReadWrite;
    else if (type == gpu_depth_stencil_state::read_only)
        Result = cDepthStateReadOnly;
    else if (type == gpu_depth_stencil_state::read_only_reversed)
        Result = cDepthStateReadOnlyReversed;
    else if (type == gpu_depth_stencil_state::test_equal)
        Result = cDepthStateTestEqual;
    
    return Result;
}

// Get a default description of the blend state
D3D12_BLEND_DESC 
GetBlendState(gpu_blend_state type)
{
    D3D12_BLEND_DESC Result = {};
    
    if (type == gpu_blend_state::disabled)
        Result = cBlendDisable;
    else if (type == gpu_blend_state::no_color_write)
        Result = cBlendNoColorWrite;
    else if (type == gpu_blend_state::traditional)
        Result = cBlendTraditional;
    else if (type == gpu_blend_state::pre_multiplied)
        Result = cBlendPreMultiplied;
    else if (type == gpu_blend_state::additive)
        Result = cBlendAdditive;
    else if (type == gpu_blend_state::tradition_additive)
        Result = cBlendTraditionalAdditive;
    
    return Result;
}

//
// State Stream Implementation: Graphics Pipeline
//

gpu_graphics_pso_builder gpu_graphics_pso_builder::Builder()
{
    gpu_graphics_pso_builder Result = {};

    Result.mRootSignature = {};

    Result.mVertexShader = {};
    Result.mPixelShader  = {};

    Result.mDomainShader   = {};
    Result.mHullShader     = {};
    Result.mGeometryShader = {};

    Result.mBlend = {};
    Result.mBlend.mObject = GetBlendState(gpu_blend_state::disabled);

    Result.mRasterizer = {};
    Result.mRasterizer.mObject = GetRasterizerState(gpu_raster_state::default_raster);

    Result.mDepthStencil = {};
    Result.mDepthStencil.mObject = GetDepthStencilState(gpu_depth_stencil_state::disabled);

    Result.mTopology = {};
    Result.mTopology.mObject = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    Result.mRenderTargetFormats = {};
    Result.mRenderTargetFormats.mObject.NumRenderTargets = 0;

    Result.mDepthFormat = {}; // DXGI_FORMAT_UNKNOWN

    Result.mSampleDesc = {};
    Result.mSampleDesc.mObject.Count   = 1;
    Result.mSampleDesc.mObject.Quality = 0;

    return Result;
}

gpu_pso gpu_graphics_pso_builder::Compile(gpu_frame_cache* FrameCache)
{
    D3D12_PIPELINE_STATE_STREAM_DESC PSSDesc = {
        .SizeInBytes                   = sizeof(gpu_graphics_pso_builder),
        .pPipelineStateSubobjectStream = this
    };

    gpu_device* Device = FrameCache->GetDevice();

    ID3D12PipelineState* PSO = nullptr;
    AssertHr(Device->AsHandle()->CreatePipelineState(&PSSDesc, ComCast(&PSO)));

    return gpu_pso(PSO);
}

gpu_graphics_pso_builder& gpu_graphics_pso_builder::SetRootSignature(gpu_root_signature* RootSignature)
{
    mRootSignature.mObject = RootSignature->AsHandle();
    return *this;
}

gpu_graphics_pso_builder& gpu_graphics_pso_builder::SetVertexShader(ct::ShaderResource& tVertexShader)
{
    ct::ShaderBytecode bytecode = tVertexShader.getShaderBytecode();

    D3D12_SHADER_BYTECODE d3d12Bytecode{};
    d3d12Bytecode.pShaderBytecode = bytecode.mShaderBytecode;
    d3d12Bytecode.BytecodeLength  = bytecode.mBytecodeLength;
    mVertexShader.mObject = d3d12Bytecode;

    return *this;
}

gpu_graphics_pso_builder& gpu_graphics_pso_builder::SetPixelShader(ct::ShaderResource& tPixelShader)
{
    ct::ShaderBytecode bytecode = tPixelShader.getShaderBytecode();

    D3D12_SHADER_BYTECODE d3d12Bytecode{};
    d3d12Bytecode.pShaderBytecode = bytecode.mShaderBytecode;
    d3d12Bytecode.BytecodeLength  = bytecode.mBytecodeLength;
    mPixelShader.mObject = d3d12Bytecode;

    return *this;
}

gpu_graphics_pso_builder& gpu_graphics_pso_builder::SetHullShader(ct::ShaderResource& tHullShader)
{
    ct::ShaderBytecode bytecode = tHullShader.getShaderBytecode();

    D3D12_SHADER_BYTECODE d3d12Bytecode{};
    d3d12Bytecode.pShaderBytecode = bytecode.mShaderBytecode;
    d3d12Bytecode.BytecodeLength  = bytecode.mBytecodeLength;
    mHullShader.mObject = d3d12Bytecode;

    return *this;
}

gpu_graphics_pso_builder& gpu_graphics_pso_builder::SetDomainShader(ct::ShaderResource& tDomainShader)
{
    ct::ShaderBytecode bytecode = tDomainShader.getShaderBytecode();

    D3D12_SHADER_BYTECODE d3d12Bytecode{};
    d3d12Bytecode.pShaderBytecode = bytecode.mShaderBytecode;
    d3d12Bytecode.BytecodeLength  = bytecode.mBytecodeLength;
    mDomainShader.mObject = d3d12Bytecode;

    return *this;
}

gpu_graphics_pso_builder& gpu_graphics_pso_builder::SetGeometryShader(ct::ShaderResource& tGeometryShader)
{
    ct::ShaderBytecode bytecode = tGeometryShader.getShaderBytecode();

    D3D12_SHADER_BYTECODE d3d12Bytecode{};
    d3d12Bytecode.pShaderBytecode = bytecode.mShaderBytecode;
    d3d12Bytecode.BytecodeLength  = bytecode.mBytecodeLength;
    mGeometryShader.mObject = d3d12Bytecode;

    return *this;
}

gpu_graphics_pso_builder& gpu_graphics_pso_builder::SetDefaultBlendState(gpu_blend_state BlendState)
{
    mBlend.mObject = GetBlendState(BlendState);
    return *this;
}

gpu_graphics_pso_builder& gpu_graphics_pso_builder::SetBlendState(D3D12_BLEND_DESC BlendDesc)
{
    mBlend.mObject = BlendDesc;
    return *this;
}

gpu_graphics_pso_builder& gpu_graphics_pso_builder::SetDefaultRasterState(gpu_raster_state RasterState)
{
    mRasterizer.mObject = GetRasterizerState(RasterState);
    return *this;
}

gpu_graphics_pso_builder& gpu_graphics_pso_builder::SetRasterState(D3D12_RASTERIZER_DESC RasterDesc)
{
    mRasterizer.mObject = RasterDesc;
    return *this;
}

gpu_graphics_pso_builder& gpu_graphics_pso_builder::SetDefaultDepthStencilState(gpu_depth_stencil_state DepthStencil, gpu_format DepthFormat)
{
    mDepthStencil.mObject = GetDepthStencilState(DepthStencil);
    mDepthFormat.mObject  = to_dxgi_format(DepthFormat);
    return *this;
}

gpu_graphics_pso_builder& gpu_graphics_pso_builder::SetDepthStencilState(D3D12_DEPTH_STENCIL_DESC DepthStencilDesc, DXGI_FORMAT DepthFormat)
{
    mDepthStencil.mObject = DepthStencilDesc;
    mDepthFormat.mObject  = DepthFormat;
    return *this;
}

gpu_graphics_pso_builder& gpu_graphics_pso_builder::SetDepthFormat(DXGI_FORMAT DepthFormat)
{
    mDepthFormat.mObject  = DepthFormat;
    return *this;
}

gpu_graphics_pso_builder& gpu_graphics_pso_builder::SetRenderTargetFormats(std::initializer_list<DXGI_FORMAT> Formats)
{
    ASSERT(Formats.size() <= 8 && "Cannot create PSO with more than 8 Render Target Formats");
    mRenderTargetFormats.mObject.NumRenderTargets = (UINT)Formats.size();

    int i = 0;
    for (const auto& RTFormat : Formats)
    {
        mRenderTargetFormats.mObject.RTFormats[i] = RTFormat;
        i += 1;
    }

    return *this;
}

gpu_graphics_pso_builder& gpu_graphics_pso_builder::SetSampleQuality(u32 Count, u32 Quality)
{
    mSampleDesc.mObject.Count   = Count;
    mSampleDesc.mObject.Quality = Quality;
    return *this;
}

gpu_graphics_pso_builder& gpu_graphics_pso_builder::SetTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE Topology)
{
    mTopology.mObject = Topology;
    return *this;
}

//
// State Stream Implementation: mCompute Pipeline
//

gpu_compute_pso_builder gpu_compute_pso_builder::Builder()
{
    gpu_graphics_pso_builder Result = {};

    Result.mRootSignature = {};
    ASSERT(false && "gpu_compute_pso_builder::Builder Unimplemented");
    return {};
}

gpu_compute_pso_builder& gpu_compute_pso_builder::SetRootSignature(gpu_root_signature* RootSignature)
{
    mRootSignature.mObject = RootSignature->AsHandle();
    return *this;
}

gpu_compute_pso_builder& gpu_compute_pso_builder::SetComputeShader(ct::ShaderResource& tComputeShader)
{
    ct::ShaderBytecode bytecode = tComputeShader.getShaderBytecode();

    D3D12_SHADER_BYTECODE d3d12Bytecode{};
    d3d12Bytecode.pShaderBytecode = bytecode.mShaderBytecode;
    d3d12Bytecode.BytecodeLength  = bytecode.mBytecodeLength;
    mComputeShader.mObject = d3d12Bytecode;

    return *this;
}

gpu_pso gpu_compute_pso_builder::Compile(struct gpu_frame_cache* FrameCache)
{
    D3D12_PIPELINE_STATE_STREAM_DESC PSSDesc = {
        .SizeInBytes = sizeof(gpu_compute_pso_builder),
        .pPipelineStateSubobjectStream = this
    };

    gpu_device* Device = FrameCache->GetDevice();

    ID3D12PipelineState* PSO = nullptr;
    AssertHr(Device->AsHandle()->CreatePipelineState(&PSSDesc, ComCast(&PSO)));

    return {PSO};
}

//
// State Stream Implementation: Mesh Pipeline
//

gpu_mesh_pso_builder gpu_mesh_pso_builder::Builder()
{
    ASSERT(false && "gpu_mesh_pso_builder::Builder Unimplemented");
    return {};
}

gpu_pso gpu_mesh_pso_builder::Compile(struct gpu_frame_cache* FrameCache)
{
    ASSERT(false && "gpu_mesh_pso_builder::Compile Unimplemented");
    return {};
}
