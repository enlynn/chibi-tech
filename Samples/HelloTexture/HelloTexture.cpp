#include <Game.h>
#include <Engine.h>

#include <Platform/Console.h>

#include <Gpu/GpuBuffer.h>
#include <Gpu/GpuState.h>
#include <Gpu/GpuUtils.h>
#include <Gpu/GpuResourceViews.h>

#include <Systems/ResourceSystem.h>

#include <Math/Geometry.h>

#define STB_IMAGE_IMPLEMENTATION
#include <Stb/stb_image.h>

struct VertexDrawConstants
{
    u32 uVertexOffset      = 0;
    u32 uVertexBufferIndex = 0;
    u32 uMeshDataIndex     = 0;
};

enum class TexRootParamters
{
    VertexBuffer = 0,
    Textures     = 1,
    PerDraw      = 2,
};

class MipsGenerator {
public:
    MipsGenerator() = default;
    explicit MipsGenerator(ct::Engine& tEngine);
    void release();

    void generateMips(GpuFrameCache& tFrameCache, GpuCommandList& tActiveCommandList, GpuTexture& tTexture);

private:
    enum class GenerateMipsParams
    {
        GenerateMipsCB,
        SrcMip,
        OutMip,
        NumRootParameters
    };

    struct alignas(16) GenerateMipsCB
    {
        u32   mSrcMipLevel;   // Texture level of source mip
        u32   mNumMipLevels;  // Number of OutMips to write: [1-4]
        u32   mSrcDimension;  // Width and height of the source texture are even or odd.
        u32   mIsSRGB;        // Must apply gamma correction to sRGB textures.
        float2 mTexelSize;     // 1.0 / OutMip1.Dimensions
    };

    GpuRootSignature      mRootSignature{};
    GpuPso                mPso{};
    CpuDescriptor         mDefaultUAV{};

    void generateMips_UAV(GpuFrameCache& tFrameCache, GpuCommandList& tActiveCommandList, GpuTexture& tTexture, bool tIsSRGB);
};

class HelloTextureApp : public ct::Game {
public:
    [[nodiscard]] ct::GameInfo getGameInfo() const override;

    bool onInit(ct::Engine& tEngine) override;
    bool onUpdate(ct::Engine& tEngine) override;
    bool onRender(ct::Engine& tEngine) override;
    bool onDestroy(ct::Engine& tEngine) override;

    ResourceSystem        mResourceSystem{};

    GpuBuffer             mVertexResource{};
    GpuBuffer             mIndexResource{};
    GpuTexture            mWallTexture{};

    GpuRootSignature      mRootSignature{};
    GpuPso                mPso{};

    MipsGenerator         mMipsGenerator{};
};

namespace {
    void copyTextureSubresource(GpuFrameCache& tFrameCache, GpuCommandList& tCommandList, GpuTexture& tTexture,
                                u32 tFirstSubresource, u32 tNumSubresources, D3D12_SUBRESOURCE_DATA *tSubresources)
    {
        auto dstResource = tTexture.getResource();

        tFrameCache.transitionResource(dstResource, D3D12_RESOURCE_STATE_COPY_DEST);
        tFrameCache.flushResourceBarriers(&tCommandList);

        UINT64 requiredSize = getRequiredIntermediateSize(dstResource->asHandle(), tFirstSubresource, tNumSubresources);

        CommitedResourceInfo resourceInfo{
            .HeapType     = D3D12_HEAP_TYPE_UPLOAD,
            .Size         = requiredSize,
            .InitialState = D3D12_RESOURCE_STATE_GENERIC_READ,
        };

        GpuResource interimResource = tFrameCache.getDevice()->createCommittedResource(resourceInfo);

        updateSubresources(tCommandList.asHandle(), dstResource->asHandle(), interimResource.asHandle(), 0,
                           tFirstSubresource, tNumSubresources, tSubresources);

        tFrameCache.addStaleResource(interimResource);
    }

    GpuTexture loadTextureFromMemory(GpuFrameCache& tFrameCache, GpuCommandList& tCommandList,
                                     void *tPixels, int tWidth, int tHeight, int tNumChannels, bool tIsSrgb)
    {
        DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
        if (tIsSrgb)
        {
            format = GpuTexture::getSrgbFormat(format);
        }

        D3D12_RESOURCE_DESC rsrcDesc = getTex2DDesc(format, tWidth, tHeight);
        GpuTexture result = GpuTexture(&tFrameCache, rsrcDesc);

        D3D12_SUBRESOURCE_DATA subresource = {};
        subresource.RowPitch   = tWidth * (sizeof(u8) * 4);
        subresource.SlicePitch = subresource.RowPitch * tHeight;
        subresource.pData      = tPixels;

        copyTextureSubresource(tFrameCache, tCommandList, result, 0, 1, &subresource);

        return result;
    }

    GpuTexture loadTextureFromFile(GpuFrameCache& tFrameCache, GpuCommandList& tCommandList,
                                   const std::filesystem::path& tTexturePath, MipsGenerator* tMipsGenerator) {
        auto imagePathGeneric = tTexturePath.string(); //stb requires utf8 paths

        int x,y,n;
        unsigned char *data = stbi_load(imagePathGeneric.c_str(), &x, &y, &n, 4);

        GpuTexture result = loadTextureFromMemory(tFrameCache, tCommandList, data, x, y, n, false);

        if (tMipsGenerator)
        {
            tMipsGenerator->generateMips(tFrameCache, tCommandList, result);
        }

        stbi_image_free(data);

        return result;
    }
}

void MipsGenerator::generateMips_UAV(GpuFrameCache& tFrameCache, GpuCommandList& tActiveCommandList, GpuTexture& tTexture, bool tIsSRGB)
{
    auto d3dDevice = tFrameCache.getDevice()->asHandle();

    const GpuResource*  textureRsrc = tTexture.getResource();
    D3D12_RESOURCE_DESC textureDesc = textureRsrc->getResourceDesc();
    ID3D12Resource*     d3d12Rsrc   = textureRsrc->asHandle();

    tActiveCommandList.setPipelineState(mPso);
    tActiveCommandList.setComputeRootSignature(mRootSignature);

    GenerateMipsCB generateMipsCB = {};
    generateMipsCB.mIsSRGB = tIsSRGB;

    // Create an SRV that uses the format of the original texture.
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                  = tIsSRGB ? GpuTexture::getSrgbFormat(textureDesc.Format) : textureDesc.Format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;  // Only 2D textures are supported (this was checked in the calling function).
    srvDesc.Texture2D.MipLevels     = textureDesc.MipLevels;

    GpuShaderResourceView srv(tFrameCache.mGlobal, textureRsrc);

    for (u32 srcMip = 0; srcMip < textureDesc.MipLevels - 1u;)
    {
        u64 srcWidth  = textureDesc.Width  >> srcMip;
        u32 srcHeight = textureDesc.Height >> srcMip;
        u32 dstWidth  = static_cast<u32>( srcWidth >> 1 );
        u32 dstHeight = srcHeight >> 1;

        // 0b00(0): Both width and height are even.
        // 0b01(1): Width is odd, height is even.
        // 0b10(2): Width is even, height is odd.
        // 0b11(3): Both width and height are odd.
        generateMipsCB.mSrcDimension = (srcHeight & 1) << 1 | (srcWidth & 1);

        // How many mipmap levels to compute this pass (max 4 mips per pass)
        DWORD mipCount;

        // The number of times we can half the size of the texture and get
        // exactly a 50% reduction in size.
        // A 1 bit in the width or height indicates an odd dimension.
        // The case where either the width or the height is exactly 1 is handled
        // as a special case (as the dimension does not require reduction).
        _BitScanForward(&mipCount, (dstWidth == 1 ? dstHeight : dstWidth) | (dstHeight == 1 ? dstWidth : dstHeight));

        // Maximum number of mips to generate is 4.
        mipCount = (mipCount < 4) ? mipCount : 4;
        // Clamp to total number of mips left over.
        mipCount = (srcMip + mipCount) >= textureDesc.MipLevels ? textureDesc.MipLevels - srcMip - 1 : mipCount;

        // Dimensions should not reduce to 0.
        // This can happen if the width and height are not the same.
        dstWidth  = (dstWidth  > 1) ? dstWidth  : 1;
        dstHeight = (dstHeight > 1) ? dstHeight : 1;

        generateMipsCB.mSrcMipLevel  = srcMip;
        generateMipsCB.mNumMipLevels = mipCount;
        generateMipsCB.mTexelSize.X  = 1.0f / (float)dstWidth;
        generateMipsCB.mTexelSize.Y  = 1.0f / (float)dstHeight;

        tActiveCommandList.setCompute32BitConstants((u32)GenerateMipsParams::GenerateMipsCB, generateMipsCB);

        tFrameCache.transitionResource(srv.getResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, srcMip, 1);
        tActiveCommandList.setShaderResourceView((u32)GenerateMipsParams::SrcMip, 0, srv);

        for (u32 mip = 0; mip < mipCount; ++mip)
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
            uavDesc.Format                           = textureDesc.Format;
            uavDesc.ViewDimension                    = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice               = srcMip + mip + 1;

            GpuUnorderedAccessView uav(tFrameCache.mGlobal, textureRsrc, nullptr, &uavDesc);

            tFrameCache.transitionResource(uav.getResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, srcMip + mip + 1, 1);
            tActiveCommandList.setUnorderedAccessView((u32)GenerateMipsParams::OutMip, mip, uav);
        }

        // Pad any unused mip levels with a default UAV. Doing this keeps the DX12 runtime happy.
        if (mipCount < 4) {
            tActiveCommandList.stageDynamicDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, (u32)GenerateMipsParams::OutMip,
                                                       mipCount, 4 - mipCount, mDefaultUAV.getDescriptorHandle());
        }

        tFrameCache.flushResourceBarriers(&tActiveCommandList);
        tActiveCommandList.setComputeRootSignature(mRootSignature);
        tActiveCommandList.dispatch(DIVIDE_ALIGN(dstWidth, 8), DIVIDE_ALIGN(dstHeight, 8));

        tFrameCache.uavBarrier(textureRsrc);

        srcMip += mipCount;
    }

    // TODO(enlynn): Clean up the SRV + descriptors?
}

void MipsGenerator::generateMips(GpuFrameCache& tFrameCache, GpuCommandList& tActiveCommandList, GpuTexture& tTexture)
{
    // If the active command list is a copy list, then we need to
    GpuCommandList* commandList = &tActiveCommandList;
    if (commandList->getType() == GpuCommandListType::Copy)
    {
        commandList = tFrameCache.getComputeCommandList();
    }

    auto d3dDevice = tFrameCache.getDevice()->asHandle();

    const GpuResource*  textureRsrc = tTexture.getResource();
    D3D12_RESOURCE_DESC textureDesc = textureRsrc->getResourceDesc();
    ID3D12Resource*     d3d12Rsrc   = textureRsrc->asHandle();

    // if the resource only has a signle mip level, do nothing
    // NOTE(enlynn): Need to watch this branch when testing
    if (textureDesc.MipLevels == 1)
    {
        return;
    }

    // Currently, only non-multi-sampled 2D textures are supported
    ASSERT(textureDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D);
    ASSERT(textureDesc.DepthOrArraySize == 1);
    ASSERT(textureDesc.SampleDesc.Count == 1);

    GpuResource uavResource = *textureRsrc;
    // This is done to perform a GPU copy of resources with different formats.
    // BGR -> RGB texture copies will fail GPU validation unless performed
    // through an alias of the BRG resource in a placed heap.
    GpuResource aliasResource{};

    // If the passed-in resource does not allow for UAV access then create a staging resource that is used to
    // generate the mipmap chain.
    if (!tTexture.checkUavSupport() || (textureDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0)
    {
        // Describe an alias resource that is used to copy the original texture.
        D3D12_RESOURCE_DESC aliasDesc = textureDesc;
        // Placed resources can't be render targets or depth-stencil views.
        aliasDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        aliasDesc.Flags &= ~( D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL );

        // Describe a UAV compatible resource that is used to perform mipmapping of the original texture.
        D3D12_RESOURCE_DESC uavDesc = aliasDesc;  // The flags for the UAV description must match that of the alias description.
        uavDesc.Format = GpuTexture::getUavCompatibleFormat(textureDesc.Format);

        D3D12_RESOURCE_DESC resourceDescs[] = { aliasDesc, uavDesc };

        // Create a heap that is large enough to store a copy of the original resource.
        D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = d3dDevice->GetResourceAllocationInfo(0, _countof( resourceDescs ), resourceDescs);

        D3D12_HEAP_DESC heapDesc                 = {};
        heapDesc.SizeInBytes                     = allocationInfo.SizeInBytes;
        heapDesc.Alignment                       = allocationInfo.Alignment;
        heapDesc.Flags                           = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
        heapDesc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapDesc.Properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;

        ID3D12Heap *heap;
        AssertHr(d3dDevice->CreateHeap(&heapDesc, ComCast(&heap)));

        // Make sure the heap does not go out of scope until the command list is finished executing on the command queue.
        tFrameCache.addStaleObject(heap);

        // Create a placed resource that matches the description of the original resource. This resource is used to copy
        // the original texture to the UAV compatible resource.
        PlacedResourceInfo placedInfo{
            .mHeap         = heap,
            .mDesc         = &aliasDesc,
            .mInitialState = D3D12_RESOURCE_STATE_COMMON,
        };

        aliasResource = tFrameCache.getDevice()->createPlacedResource(placedInfo);
        tFrameCache.trackResource(aliasResource, placedInfo.mInitialState);

        // Ensure the scope of the alias resource.
        tFrameCache.addStaleResource(aliasResource);

        // Create a UAV compatible resource in the same heap as the alias resource.
        placedInfo.mDesc = &uavDesc;
        uavResource = tFrameCache.getDevice()->createPlacedResource(placedInfo);
        tFrameCache.trackResource(uavResource, placedInfo.mInitialState);

        // Ensure the scope of the alias resource.
        tFrameCache.addStaleResource(aliasResource);

        // Add an aliasing barrier for the alias resource.
        tFrameCache.aliasBarrier(nullptr, &aliasResource);

        { // Copy the original resource to the alias resource. This ensures GPU validation.
            tFrameCache.transitionResource(&aliasResource, D3D12_RESOURCE_STATE_COPY_DEST);
            tFrameCache.transitionResource(textureRsrc,    D3D12_RESOURCE_STATE_COPY_SOURCE);
            tFrameCache.flushResourceBarriers(&tActiveCommandList);
            tActiveCommandList.copyResource(aliasResource, *textureRsrc);
        }

        // Add an aliasing barrier for the UAV compatible resource.
        tFrameCache.aliasBarrier(&aliasResource, &uavResource);
    }

    // Generate mips with the UAV compatible resource.
    GpuTexture uavTexture = GpuTexture(&tFrameCache, uavResource);
    generateMips_UAV(tFrameCache, *commandList, uavTexture, GpuTexture::isSrgbFormat(textureDesc.Format));

    // Transition the UAV resource back to the Alias Resource and copy the alias back to the original texture.
    if (aliasResource.isValid()) {
        tFrameCache.aliasBarrier(&uavResource, &aliasResource);

        { // Copy the alias resource back to the original resource.
            tFrameCache.transitionResource(&aliasResource, D3D12_RESOURCE_STATE_COPY_SOURCE);
            tFrameCache.transitionResource(textureRsrc,    D3D12_RESOURCE_STATE_COPY_DEST);
            tFrameCache.flushResourceBarriers(&tActiveCommandList);
            tActiveCommandList.copyResource(*textureRsrc, aliasResource);
        }
    }

    // Free the staging buffer only after the command list has finished executing
    if (aliasResource.isValid()) {
        tFrameCache.removeTrackedResource(aliasResource);
        tFrameCache.removeTrackedResource(uavResource);
    }
}

MipsGenerator::MipsGenerator(ct::Engine& tEngine) {
    auto gpuState = tEngine.getGpuState();
    auto frameCache = gpuState->getFrameCache();

    GpuDescriptorRange srcMipRange{
            .mType               = GpuDescriptorType::Srv,
            .mNumDescriptors     = 1,
            .mBaseShaderRegister = 0,
            .mRegisterSpace      = 0,
            .mDescriptorOffset   = 0, // should this be D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND (-1)?
            .mFlags              = GpuDescriptorRangeFlags::DataConstant,
    };

    GpuDescriptorRange dstMipRange{
            .mType               = GpuDescriptorType::Uav,
            .mNumDescriptors     = 4,
            .mBaseShaderRegister = 0,
            .mRegisterSpace      = 0,
            .mDescriptorOffset   = 0, // should this be D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND (-1)?
            .mFlags              = GpuDescriptorRangeFlags::DataConstant,
    };

    GpuDescriptorTable descriptorTables[2]{
            { .mRootIndex = (u32)GenerateMipsParams::SrcMip, .mDescriptorRanges = farray(&srcMipRange, 1)},
            { .mRootIndex = (u32)GenerateMipsParams::OutMip, .mDescriptorRanges = farray(&dstMipRange, 1)},
    };

    GpuRootConstant mipsCb{
            .mRootIndex      = (u32)GenerateMipsParams::GenerateMipsCB,
            .mNum32bitValues = sizeof(GenerateMipsCB) / 4,
    };

    D3D12_STATIC_SAMPLER_DESC sampler = getStaticSamplerDesc(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                                                             D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

    GpuRootSignatureInfo rsInfo{
            .mDescriptorTables    = farray(descriptorTables, 2),
            .mDescriptorConstants = farray(&mipsCb, 1),
            .mStaticSamplers      = farray(&sampler, 1),
            .mName                = "Generate Mips Root Signature"
    };

    mRootSignature = GpuRootSignature(*frameCache->getDevice(), rsInfo);

    // load the mips compute shader
    ct::ShaderResource mipsShader = tEngine.loadShader("GenerateMips", ct::ShaderStage::Compute);

    // Create the PSO for GenerateMips shader.
    GpuComputePsoBuilder builder = GpuComputePsoBuilder::builder();
    builder.setRootSignature(&mRootSignature)
            .setComputeShader(mipsShader);

    mPso = builder.compile(frameCache);

    mDefaultUAV = gpuState->allocateCpuDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4);
    for (UINT i = 0; i < 4; ++i)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.ViewDimension                    = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Format                           = DXGI_FORMAT_R8G8B8A8_UNORM;
        uavDesc.Texture2D.MipSlice               = i;
        uavDesc.Texture2D.PlaneSlice             = 0;
        gpuState->mDevice->asHandle()->CreateUnorderedAccessView(nullptr, nullptr, &uavDesc, mDefaultUAV.getDescriptorHandle(i));
    }
}

void MipsGenerator::release() {
    mRootSignature.release();
    mPso.release();
}

std::unique_ptr<ct::Game> onGameLoad() {
    return std::make_unique<HelloTextureApp>();
}

ct::GameInfo HelloTextureApp::getGameInfo() const {
    return {
            .mWindowTitle = "Hello Textured Triangle",
            .mAssetPath   = HelloTexture_CONTENT_PATH,
    };
}

bool HelloTextureApp::onInit(ct::Engine& tEngine) {
    auto gpuState = tEngine.getGpuState();
    auto frameCache = gpuState->getFrameCache();

    { // Create a root signature
        GpuRootDescriptor vertexBuffers[]    = {
                { .mRootIndex = (u32)TexRootParamters::VertexBuffer, .mType = GpuDescriptorType::Srv },
        };

        GpuDescriptorRange diffuseTextureRange{
                .mType               = GpuDescriptorType::Srv,
                .mNumDescriptors     = 1,
                .mBaseShaderRegister = 1,
                .mRegisterSpace      = 1,
                .mDescriptorOffset   = 0, // should this be D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND (-1)?
                //.mFlags              = GpuDescriptorRangeFlags::Constant,
        };

        GpuDescriptorTable descriptorTables[1]{
                {
                    .mRootIndex        = (u32)TexRootParamters::Textures,
                    .mVisibility       = GpuDescriptorVisibility::Pixel,
                    .mDescriptorRanges = farray(&diffuseTextureRange, 1),
                },
        };

        D3D12_STATIC_SAMPLER_DESC sampler = getStaticSamplerDesc(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);

        GpuRootConstant   perDrawConstants[] = {{ .mRootIndex = (u32)TexRootParamters::PerDraw, .mNum32bitValues = sizeof(VertexDrawConstants) / 4 } };
        const char*       debugName          = "Hello Triangle RS";

        GpuRootSignatureInfo Info = {};
        Info.mDescriptorTables    = farray(descriptorTables, 1);
        Info.mDescriptors         = farray(vertexBuffers,    ArrayCount(vertexBuffers));
        Info.mDescriptorConstants = farray(perDrawConstants, ArrayCount(perDrawConstants));
        Info.mStaticSamplers      = farray(&sampler, 1);
        Info.mName                = debugName;
        mRootSignature = GpuRootSignature(*frameCache->getDevice(), Info);
    }

    { // Create a pipeline state object
        ct::ShaderResource vertexShader = tEngine.loadShader("Texture", ct::ShaderStage::Vertex);
        ct::ShaderResource pixelShader  = tEngine.loadShader("Texture", ct::ShaderStage::Pixel);

        DXGI_FORMAT rtFormat = frameCache->mGlobal->mSwapchain->getSwapchainFormat();

        D3D12_RT_FORMAT_ARRAY psoRTFormats = {};
        psoRTFormats.NumRenderTargets = 1;
        psoRTFormats.RTFormats[0]     = rtFormat;

        // leave msaa disabled
        u32 sampleCount   = 1;
        u32 sampleQuality = 0;

        GpuGraphicsPsoBuilder builder = GpuGraphicsPsoBuilder::builder();
        builder.setRootSignature(&mRootSignature)
                .setVertexShader(vertexShader)
                .setPixelShader(pixelShader)
                .setRenderTargetFormats({rtFormat})
                .setSampleQuality(sampleCount, sampleQuality)
                .setDepthStencilState(getDepthStencilState(GpuDepthStencilState::ReadWrite), DXGI_FORMAT_D32_FLOAT);

        mPso = builder.compile(frameCache);
    }

    { // Create the triangle and upload data
        struct Vertex {
            float3 mPos{};
            float2 mTex{};
        };

        Vertex vertices[3] = {
                //   Positions           Tex Coords
                { {-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}},
                { {0.5f, -0.5f, 0.0f}, {0.0f, 1.0f}},
                { {0.0f,  0.5f, 0.0f}, {0.0f, 0.0f}},
        };

        u16 indices[] = { 0, 1, 2 };

        GpuByteAddressBufferInfo vertexInfo = {
                .mStride = sizeof(Vertex),
                .mCount  = 3,
                .mData   = vertices,
        };
        mVertexResource = GpuBuffer::createByteAdressBuffer(frameCache, vertexInfo);

        GpuIndexBufferInfo indexInfo = {
                .mIsU16      = true,
                .mIndexCount = ArrayCount(indices),
                .mIndices    = indices,
        };
        mIndexResource = GpuBuffer::createIndexBuffer(frameCache, indexInfo);

        frameCache->submitCopyCommandList();
    }

    mMipsGenerator = MipsGenerator(tEngine);

    { // Load the texture and upload to the GPU!
        auto* commandList = frameCache->getComputeCommandList();

        namespace fs = std::filesystem;
        fs::path imagePath = fs::path(HelloTexture_CONTENT_PATH) / "Textures/wall.jpg";
        mWallTexture = loadTextureFromFile(*frameCache, *commandList, imagePath, nullptr); // TODO: fix mip mapping.

        frameCache->submitComputeCommandList();
    }

    frameCache->flushGpu(); // Forcibly upload everything (for now)
    return true;
}

bool HelloTextureApp::onUpdate(ct::Engine& tEngine) {
    return true;
}

bool HelloTextureApp::onRender(ct::Engine& tEngine) {
    auto gpuState = tEngine.getGpuState();

    gpuState->beginFrame();

    auto frameCache = gpuState->getFrameCache();
    GpuCommandList *commandList = frameCache->getGraphicsCommandList();

    GpuRenderTarget renderTarget{};
    { // Setup Render Target
        // Technically don't need the depth buffer, but bind it for demonstration purposes
        GpuTexture *SceneFramebuffer = frameCache->getFramebuffer(GpuFramebufferBinding::MainColor);
        GpuTexture *DepthBuffer = frameCache->getFramebuffer(GpuFramebufferBinding::DepthStencil);

        renderTarget.attachTexture(AttachmentPoint::Color0, SceneFramebuffer);
        renderTarget.attachTexture(AttachmentPoint::DepthStencil, DepthBuffer);

        // Clear the Framebuffer and bind the render target.
        frameCache->transitionResource(SceneFramebuffer->getResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
        frameCache->transitionResource(DepthBuffer->getResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

        float4 ClearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        commandList->bindRenderTarget(&renderTarget, &ClearColor, true);
    }

    { // Scissor / Viewport
        D3D12_VIEWPORT Viewport = renderTarget.getViewport();
        commandList->setViewport(Viewport);

        D3D12_RECT ScissorRect = {};
        ScissorRect.left = 0;
        ScissorRect.top = 0;
        ScissorRect.right = (LONG) Viewport.Width;
        ScissorRect.bottom = (LONG) Viewport.Height;
        commandList->setScissorRect(ScissorRect);
    }

    { // Draw the triangle
        commandList->setPipelineState(mPso);
        commandList->setGraphicsRootSignature(mRootSignature);

        {
            auto desc = mWallTexture.getResource()->getResourceDesc();
            auto flags = desc.Flags;
        }

        // Bind the texture
        frameCache->transitionResource(mWallTexture.getResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->setShaderResourceView((u32)TexRootParamters::Textures, 0, mWallTexture);

        // Draw
        commandList->setTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        commandList->setIndexBuffer(mIndexResource.getIndexBufferView());
        commandList->setShaderResourceViewInline((u32)TexRootParamters::VertexBuffer, mVertexResource.getGpuResource());

        VertexDrawConstants Constants = {};
        commandList->setGraphics32BitConstants((u32)TexRootParamters::PerDraw, &Constants);

        frameCache->flushResourceBarriers(commandList); // flush barriers before drawing

        commandList->drawIndexedInstanced(mIndexResource.getIndexCount());
    }

    { // Copy the Scene Target into the swapchain's render target
        GpuRenderTarget* swapchainRenderTarget = frameCache->mGlobal->mSwapchain->getRenderTarget();

        const GpuTexture* swapchainBackBuffer = swapchainRenderTarget->getTexture(AttachmentPoint::Color0);
        const GpuTexture* sceneTexture        = frameCache->getFramebuffer(GpuFramebufferBinding::MainColor);

        commandList->copyResource(frameCache, swapchainBackBuffer->getResource(), sceneTexture->getResource());

        frameCache->transitionResource(swapchainBackBuffer->getResource(), D3D12_RESOURCE_STATE_PRESENT);
        frameCache->flushResourceBarriers(commandList);
    }

    gpuState->endFrame(); // TODO: endFrame should optionally take the scene framebuffer, and handle MSAA resolve.

    return true;
}

bool HelloTextureApp::onDestroy(ct::Engine& tEngine) {
    auto gpuState = tEngine.getGpuState();
    auto frameCache = gpuState->getFrameCache();

    frameCache->flushGpu();

    mWallTexture.releaseUnsafe(frameCache);

    mPso.release();
    mRootSignature.release();

    return true;
}

// entry point
#include <Entry.h>