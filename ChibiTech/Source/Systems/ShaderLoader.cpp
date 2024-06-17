#include "ShaderLoader.h"

#include <Platform/Platform.h>
#include <Platform/Console.h>

#include <Gpu/D3D12Common.h>
#include <DXC/dxcapi.h>

namespace ct {
    ShaderLoader::ShaderLoader(const std::filesystem::path &tShaderDirectory)
    : mShaderDirectory(tShaderDirectory)
    , mCompiledShaderDirectory(tShaderDirectory / ".cache") {
        if (!std::filesystem::exists(mCompiledShaderDirectory))
        {
            const bool res = std::filesystem::create_directory(mCompiledShaderDirectory);
            ASSERT(res && std::filesystem::exists(mCompiledShaderDirectory));
        }

        IDxcLibrary *dxcLibrary{nullptr};
        AssertHr(DxcCreateInstance(CLSID_DxcLibrary, ComCast(&dxcLibrary)));

        IDxcCompiler* dxcCompiler{nullptr};
        AssertHr(DxcCreateInstance(CLSID_DxcCompiler, ComCast(&dxcCompiler)));

        mDXCLibrary  = dxcLibrary;
        mDXCCompiler = dxcCompiler;
    }

    void ShaderLoader::shutdown() {
        if (mDXCCompiler) {
            ComSafeRelease(mDXCCompiler);
        }

        if (mDXCLibrary) {
            ComSafeRelease(mDXCLibrary);
        }
    }

    void ShaderLoader::releaseShader(ShaderResourceBlob tShader) {

    }

    ShaderResourceBlob ShaderLoader::loadShader(std::string_view tShaderName, ShaderStage tStage, bool tWatchFile) {
        namespace fs = std::filesystem;

        ShaderResourceBlob result{nullptr};

        // Build the Original Filepath
        //

        std::string filename = tShaderName.data();
        if (tStage == ShaderStage::Vertex) {
            filename += ".Vtx.hlsl";
        }
        else if (tStage == ShaderStage::Pixel) {
            filename += ".Pxl.hlsl";
        }
        else if (tStage == ShaderStage::Compute) {
            filename += ".Cpt.hlsl";
        }

        // TODO(enlynn): Support nested shader directories.
        fs::path filepath = mShaderDirectory / filename;
        if (!fs::exists(filepath)) {
            ct::console::error("Failed to find shader file: %s", filepath.c_str());
            return nullptr;
        }

        // Build the Compiled Filepath
        //

        std::string compiledFilename = tShaderName.data();
        if (tStage == ShaderStage::Vertex) {
            compiledFilename += Extensions::sVertexCompiled;
        }
        else if (tStage == ShaderStage::Pixel) {
            compiledFilename += Extensions::sPixelCompiled;
        }
        else if (tStage == ShaderStage::Compute) {
            compiledFilename += Extensions::sComputeCompiled;
        }

        fs::path compiledFilepath = mCompiledShaderDirectory / compiledFilename;

        // Try to load the compiled shader
        //

        if (fs::exists(compiledFilepath))
        {
            auto originalLastWrite = std::filesystem::last_write_time(filepath);
            auto compiledLastWrite = std::filesystem::last_write_time(compiledFilepath);

            if (originalLastWrite < compiledLastWrite) {
                ct::console::info("Loading compiled shader: %s.", compiledFilename.c_str());

                // cool, it does, so load the compiled shader.
                auto wideFilepath = compiledFilepath.wstring();

                u32 codePage = CP_UTF8;
                IDxcBlobEncoding* sourceBlob{nullptr};
                AssertHr(mDXCLibrary->CreateBlobFromFile(wideFilepath.data(), &codePage, &sourceBlob));

                result = sourceBlob;
                return result;
            }
        }

        // Failed to find the compiled shader, so look for the HLSL version and compile it.
        //

        ct::console::info("Compiling shader: %s.", filename.c_str());

        // DirectX API is quirky, so we need to convert the filepath to wchar_t
        std::wstring wideFilepath = filepath.generic_wstring();

        u32 codePage = CP_UTF8;
        IDxcBlobEncoding* sourceBlob{nullptr};
        AssertHr(mDXCLibrary->CreateBlobFromFile(wideFilepath.data(), &codePage, &sourceBlob));

        const wchar_t* targetProfile = nullptr;
        if (tStage == ShaderStage::Vertex) {
            targetProfile = CompilerFlags::sVertexStage;
        }
        else if (tStage == ShaderStage::Pixel) {
            targetProfile = CompilerFlags::sPixelStage;
        }
        else if (tStage == ShaderStage::Compute) {
            targetProfile = CompilerFlags::sComputeStage;
        }

        IDxcOperationResult* compileStatus{nullptr};
        HRESULT hr = FAILED(mDXCCompiler->Compile(
                sourceBlob,          // pSource
                wideFilepath.data(), // pSourceName
                L"main",             // pEntryPoint
                targetProfile,       // pTargetProfile
                nullptr, 0,          // pArguments, argCount
                nullptr, 0,          // pDefines, defineCount
                nullptr,             // pIncludeHandler
                &compileStatus));    // ppResult

        if (SUCCEEDED(hr)) {
            // we can succeed the compile call, but failed to compile. Get the status of the compilation.
            compileStatus->GetStatus(&hr);
        }

        if (FAILED(hr)) {
            if(compileStatus)
            {
                IDxcBlobEncoding* errorsBlob{nullptr};
                hr = compileStatus->GetErrorBuffer(&errorsBlob);
                if(SUCCEEDED(hr) && errorsBlob)
                {
                    ct::console::fatal("Compilation failed with errors:\n%s\n", (const char*)errorsBlob->GetBufferPointer());
                }
            }
        }

        // Write the compiled binary to disc so we can use that next time
        //

        compileStatus->GetResult(&result);

        bool res = ct::os::writeBufferToFile(compiledFilepath, result->GetBufferPointer(), result->GetBufferSize());
        ASSERT(res);

        // The  caller wants to watch the file, do so!
        //

        if (tWatchFile) {
            UNIMPLEMENTED;
        }

        return result;
    }

    ShaderBytecode ShaderResource::getShaderBytecode() {
        return ShaderBytecode{
            .mShaderBytecode = mShaderBlob->GetBufferPointer(),
            .mBytecodeLength = mShaderBlob->GetBufferSize()
        };
    }

    ShaderResource::~ShaderResource() {
        ComSafeRelease(mShaderBlob);
    }
} // ct