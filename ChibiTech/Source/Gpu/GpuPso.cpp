#include "GpuPso.h"
#include "GpuDevice.h"
#include "GpuRootSignature.h"
#include "GpuState.h"
#include "GpuDevice.h"

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
getRasterizerState(GpuRasterState Type)
{
    D3D12_RASTERIZER_DESC Result = {};
    
    if (Type == GpuRasterState::DefaultRaster)
        Result = cRasterizerDefault;
    else if (Type == GpuRasterState::DefaultMsaa)
        Result = cRasterizerDefaultMsaa;
    else if (Type == GpuRasterState::DefaultCw)
        Result = cRasterizerDefaultCw;
    else if (Type == GpuRasterState::DefaultCwMsaa)
        Result = cRasterizerDefaultCwMsaa;
    else if (Type == GpuRasterState::TwoSided)
        Result = cRasterizerTwoSided;
    else if (Type == GpuRasterState::TwoSidedMsaa)
        Result = cRasterizerTwoSidedMsaa;
    else if (Type == GpuRasterState::Shadow)
        Result = cRasterizerShadow;
    else if (Type == GpuRasterState::ShadowCw)
        Result = cRasterizerShadowCW;
    else if (Type == GpuRasterState::ShadowTwoSided)
        Result = cRasterizerShadowTwoSided;
    
    return Result;
}

// Get a default description of the depth-stencil state
D3D12_DEPTH_STENCIL_DESC 
getDepthStencilState(GpuDepthStencilState Type)
{
    D3D12_DEPTH_STENCIL_DESC Result = {};
    
    if (Type == GpuDepthStencilState::Disabled)
        Result = cDepthStateDisabled;
    else if (Type == GpuDepthStencilState::ReadWrite)
        Result = cDepthStateReadWrite;
    else if (Type == GpuDepthStencilState::ReadOnly)
        Result = cDepthStateReadOnly;
    else if (Type == GpuDepthStencilState::ReadOnlyReversed)
        Result = cDepthStateReadOnlyReversed;
    else if (Type == GpuDepthStencilState::TestEqual)
        Result = cDepthStateTestEqual;
    
    return Result;
}

// Get a default description of the blend state
D3D12_BLEND_DESC 
getBlendState(GpuBlendState Type)
{
    D3D12_BLEND_DESC Result = {};
    
    if (Type == GpuBlendState::Disabled)
        Result = cBlendDisable;
    else if (Type == GpuBlendState::NoColorWrite)
        Result = cBlendNoColorWrite;
    else if (Type == GpuBlendState::Traditional)
        Result = cBlendTraditional;
    else if (Type == GpuBlendState::PreMultiplied)
        Result = cBlendPreMultiplied;
    else if (Type == GpuBlendState::Additive)
        Result = cBlendAdditive;
    else if (Type == GpuBlendState::TraditionAdditive)
        Result = cBlendTraditionalAdditive;
    
    return Result;
}

//
// State Stream Implementation: Graphics Pipeline
//

GpuGraphicsPsoBuilder GpuGraphicsPsoBuilder::builder()
{
    GpuGraphicsPsoBuilder Result = {};

    Result.mRootSignature = {};

    Result.mVertexShader = {};
    Result.mPixelShader  = {};

    Result.mDomainShader   = {};
    Result.mHullShader     = {};
    Result.mGeometryShader = {};

    Result.mBlend = {};
    Result.mBlend.mObject = getBlendState(GpuBlendState::Disabled);

    Result.mRasterizer = {};
    Result.mRasterizer.mObject = getRasterizerState(GpuRasterState::DefaultRaster);

    Result.mDepthStencil = {};
    Result.mDepthStencil.mObject = getDepthStencilState(GpuDepthStencilState::Disabled);

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

GpuPso GpuGraphicsPsoBuilder::compile(GpuFrameCache *FrameCache)
{
    D3D12_PIPELINE_STATE_STREAM_DESC PSSDesc = {
        .SizeInBytes                   = sizeof(GpuGraphicsPsoBuilder),
        .pPipelineStateSubobjectStream = this
    };

    GpuDevice* Device = FrameCache->getDevice();

    ID3D12PipelineState* PSO = nullptr;
    AssertHr(Device->asHandle()->CreatePipelineState(&PSSDesc, ComCast(&PSO)));

    return GpuPso(PSO);
}

GpuGraphicsPsoBuilder& GpuGraphicsPsoBuilder::setRootSignature(struct GpuRootSignature *RootSignature)
{
    mRootSignature.mObject = RootSignature->asHandle();
    return *this;
}

GpuGraphicsPsoBuilder& GpuGraphicsPsoBuilder::setVertexShader(ct::ShaderResource& VertexShader)
{
    ct::ShaderBytecode bytecode = VertexShader.getShaderBytecode();

    D3D12_SHADER_BYTECODE d3d12Bytecode{};
    d3d12Bytecode.pShaderBytecode = bytecode.mShaderBytecode;
    d3d12Bytecode.BytecodeLength  = bytecode.mBytecodeLength;
    mVertexShader.mObject = d3d12Bytecode;

    return *this;
}

GpuGraphicsPsoBuilder& GpuGraphicsPsoBuilder::setPixelShader(ct::ShaderResource& PixelShader)
{
    ct::ShaderBytecode bytecode = PixelShader.getShaderBytecode();

    D3D12_SHADER_BYTECODE d3d12Bytecode{};
    d3d12Bytecode.pShaderBytecode = bytecode.mShaderBytecode;
    d3d12Bytecode.BytecodeLength  = bytecode.mBytecodeLength;
    mPixelShader.mObject = d3d12Bytecode;

    return *this;
}

GpuGraphicsPsoBuilder& GpuGraphicsPsoBuilder::setHullShader(ct::ShaderResource& HullShader)
{
    ct::ShaderBytecode bytecode = HullShader.getShaderBytecode();

    D3D12_SHADER_BYTECODE d3d12Bytecode{};
    d3d12Bytecode.pShaderBytecode = bytecode.mShaderBytecode;
    d3d12Bytecode.BytecodeLength  = bytecode.mBytecodeLength;
    mHullShader.mObject = d3d12Bytecode;

    return *this;
}

GpuGraphicsPsoBuilder& GpuGraphicsPsoBuilder::setDomainShader(ct::ShaderResource& DomainShader)
{
    ct::ShaderBytecode bytecode = DomainShader.getShaderBytecode();

    D3D12_SHADER_BYTECODE d3d12Bytecode{};
    d3d12Bytecode.pShaderBytecode = bytecode.mShaderBytecode;
    d3d12Bytecode.BytecodeLength  = bytecode.mBytecodeLength;
    mDomainShader.mObject = d3d12Bytecode;

    return *this;
}

GpuGraphicsPsoBuilder& GpuGraphicsPsoBuilder::setGeometryShader(ct::ShaderResource& GeometryShader)
{
    ct::ShaderBytecode bytecode = GeometryShader.getShaderBytecode();

    D3D12_SHADER_BYTECODE d3d12Bytecode{};
    d3d12Bytecode.pShaderBytecode = bytecode.mShaderBytecode;
    d3d12Bytecode.BytecodeLength  = bytecode.mBytecodeLength;
    mGeometryShader.mObject = d3d12Bytecode;

    return *this;
}

GpuGraphicsPsoBuilder& GpuGraphicsPsoBuilder::setDefaultBlendState(GpuBlendState BlendState)
{
    mBlend.mObject = getBlendState(BlendState);
    return *this;
}

GpuGraphicsPsoBuilder& GpuGraphicsPsoBuilder::setBlendState(D3D12_BLEND_DESC BlendDesc)
{
    mBlend.mObject = BlendDesc;
    return *this;
}

GpuGraphicsPsoBuilder& GpuGraphicsPsoBuilder::setDefaultRasterState(GpuRasterState RasterState)
{
    mRasterizer.mObject = getRasterizerState(RasterState);
    return *this;
}

GpuGraphicsPsoBuilder& GpuGraphicsPsoBuilder::setRasterState(D3D12_RASTERIZER_DESC RasterDesc)
{
    mRasterizer.mObject = RasterDesc;
    return *this;
}

GpuGraphicsPsoBuilder& GpuGraphicsPsoBuilder::setDefaultDepthStencilState(GpuDepthStencilState DepthStencil, GpuFormat DepthFormat)
{
    mDepthStencil.mObject = getDepthStencilState(DepthStencil);
    mDepthFormat.mObject  = toDxgiFormat(DepthFormat);
    return *this;
}

GpuGraphicsPsoBuilder& GpuGraphicsPsoBuilder::setDepthStencilState(D3D12_DEPTH_STENCIL_DESC DepthStencilDesc, DXGI_FORMAT DepthFormat)
{
    mDepthStencil.mObject = DepthStencilDesc;
    mDepthFormat.mObject  = DepthFormat;
    return *this;
}

GpuGraphicsPsoBuilder& GpuGraphicsPsoBuilder::setDepthFormat(DXGI_FORMAT DepthFormat)
{
    mDepthFormat.mObject  = DepthFormat;
    return *this;
}

GpuGraphicsPsoBuilder& GpuGraphicsPsoBuilder::setRenderTargetFormats(std::initializer_list<DXGI_FORMAT> Formats)
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

GpuGraphicsPsoBuilder& GpuGraphicsPsoBuilder::setSampleQuality(u32 Count, u32 Quality)
{
    mSampleDesc.mObject.Count   = Count;
    mSampleDesc.mObject.Quality = Quality;
    return *this;
}

GpuGraphicsPsoBuilder& GpuGraphicsPsoBuilder::setTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE Topology)
{
    mTopology.mObject = Topology;
    return *this;
}

//
// State Stream Implementation: mCompute Pipeline
//

GpuComputePsoBuilder GpuComputePsoBuilder::builder()
{
    GpuComputePsoBuilder result{};
    return result;
}

GpuComputePsoBuilder& GpuComputePsoBuilder::setRootSignature(struct GpuRootSignature *RootSignature)
{
    mRootSignature.mObject = RootSignature->asHandle();
    return *this;
}

GpuComputePsoBuilder& GpuComputePsoBuilder::setComputeShader(ct::ShaderResource& VertexShader)
{
    ct::ShaderBytecode bytecode = VertexShader.getShaderBytecode();

    D3D12_SHADER_BYTECODE d3d12Bytecode{};
    d3d12Bytecode.pShaderBytecode = bytecode.mShaderBytecode;
    d3d12Bytecode.BytecodeLength  = bytecode.mBytecodeLength;
    mComputeShader.mObject = d3d12Bytecode;

    return *this;
}

GpuPso GpuComputePsoBuilder::compile(struct GpuFrameCache* FrameCache)
{
    D3D12_PIPELINE_STATE_STREAM_DESC PSSDesc = {
        .SizeInBytes = sizeof(GpuComputePsoBuilder),
        .pPipelineStateSubobjectStream = this
    };

    GpuDevice* Device = FrameCache->getDevice();

    ID3D12PipelineState* PSO = nullptr;
    AssertHr(Device->asHandle()->CreatePipelineState(&PSSDesc, ComCast(&PSO)));

    return GpuPso(PSO);
}

//
// State Stream Implementation: Mesh Pipeline
//

GpuMeshPsoBuilder GpuMeshPsoBuilder::builder()
{
    ASSERT(false && "GpuMeshPsoBuilder::builder Unimplemented");
    return {};
}

GpuPso GpuMeshPsoBuilder::compile(struct GpuFrameCache* FrameCache)
{
    ASSERT(false && "GpuMeshPsoBuilder::compile Unimplemented");
    return {};
}
