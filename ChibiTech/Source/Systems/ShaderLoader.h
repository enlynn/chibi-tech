#pragma once

#include <filesystem>

#include <Types.h>
#include "d3d12/d3d12.h"

struct IDxcLibrary;
struct IDxcCompiler;
struct IDxcBlob;

namespace ct {

    using ShaderResourceBlob = IDxcBlob*;
    struct ShaderResourceModules
    {
        ShaderResourceBlob mVertex  = nullptr;
        ShaderResourceBlob mPixel   = nullptr;
        ShaderResourceBlob mCompute = nullptr;
        // TODO: Handle mesh shading and RT pipeline
    };

    enum class ShaderStage : u8
    {
        Unknown,
        Vertex,
        Pixel,
        Compute,
        Count,
    };

    struct ShaderBytecode {
       const void* mShaderBytecode;
       size_t      mBytecodeLength;
    };

    struct ShaderResource
    {
        explicit ShaderResource(ShaderStage tShaderStage, ShaderResourceBlob tByteCode) : mStage(tShaderStage), mShaderBlob(tByteCode) {}
        ~ShaderResource();

        // For a shader Resource, this doesn't do anything. Nothing to parse.
        // Don't need to override parse because all it would do is make a copy of Data and DataSize

        ShaderBytecode getShaderBytecode();

    private:
        ShaderStage        mStage{ShaderStage::Unknown};
        ShaderResourceBlob mShaderBlob{nullptr};
    };

    // Shader Loader for DX12 HLSL shader files.
    class ShaderLoader {
    public:
        explicit ShaderLoader(const std::filesystem::path& tShaderDirectory);
        void shutdown();

        ShaderResourceBlob loadShader(std::string_view tShaderName, ShaderStage tStage, bool tWatchFile = false /* unimplemented path */);
        void releaseShader(ShaderResourceBlob tShader);

    private:
        struct Extensions {
            static constexpr char sVertex[]          = ".Vtx.hlsl";
            static constexpr char sPixel[]           = ".Pxl.hlsl";
            static constexpr char sCompute[]         = ".Cpt.hlsl";

            static constexpr char sVertexCompiled[]  = ".Vtx.hlsl.cso";
            static constexpr char sPixelCompiled[]   = ".Pxl.hlsl.cso";
            static constexpr char sComputeCompiled[] = ".Cpt.hlsl.cso";
        };

        struct CompilerFlags {
            static constexpr wchar_t sVertexStage[]  = L"vs_6_5";
            static constexpr wchar_t sPixelStage[]   = L"ps_6_5";
            static constexpr wchar_t sComputeStage[] = L"cs_6_5";
        };

        std::filesystem::path mShaderDirectory{};
        std::filesystem::path mCompiledShaderDirectory{};

        IDxcLibrary*          mDXCLibrary{nullptr};
        IDxcCompiler*         mDXCCompiler{nullptr};
    };

} // ct

