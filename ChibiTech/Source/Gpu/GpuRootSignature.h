#pragma once

#include <types.h>
#include "D3D12Common.h"

#include <Util/array.h>

#include <string>

class GpuDevice;

/*

A Root Signature describes the bindings for a Pipeline.

The Root Signature has space for 64 DWORDs (DWORD = 4 bytes)

Root Constants
- 1 DWORD, for a maximum of 64 Root Constants. For example, you can embed 64 ints into the root signature.
- No indirection
- Inlined directly into the root signature
- Appears as a Constant Buffer in the shaders
- Declared with D3D12_ROOT_CONSTANTS
- Relative root signature type needs to be D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS 

Root mDescriptors
- 2 DWORDS (size of a GPU virtual address), for a maximum of 32 descriptors
- 1 layer of indirection
- Inlined in the root arguments
- Made for descriptors that get accessed the most often
- CBV, SRV, UAVs
- Supported SRV/UAV formats are only 32-bit FLOAT/UINT/SINT.
- SRVs of ray tracing acceleration structures are also supported.
- D3D12_ROOT_DESCRIPTOR
- Slot type needs to be D3D12_ROOT_PARAMETER_TYPE_CBV, D3D12_ROOT_PARAMETER_TYPE_SRV, D3D12_ROOT_PARAMETER_TYPE_UAV

Root Descriptor Tables
- 1 DWORD, for a maximum of 64 tables
- 2 layers of indirection
- Pointers to an array of descriptors

Actually creating these...
D3D12_ROOT_PARAMETER 
    D3D12_ROOT_DESCRIPTOR_TABLE 
        Descriptor Ranges
            Range Type
            Num mDescriptors
            Base Shader Register
            Register Space
            Offset in mDescriptors from Table Start
    D3D12_ROOT_CONSTANTS
        Shader Register
        Register Space
        Num 32 bit values
    D3D12_ROOT_DESCRIPTOR
        Shader Register
        Register Space

*/

enum class GpuDescriptorType : u8
{
    Srv,
    Uav,
    Cbv,
    Sampler,
};

enum class GpuDescriptorRangeFlags : u8
{
    None,                // Default behavior, SRV/CBV are static at execute, UAV are volatile
    Constant,            // Data is Constant and mDescriptors are Constant. Best for driver optimization.
    Dynamic,             // Data is Volatile and mDescriptors are Volatile
    DataConstant,        // Data is Constant and mDescriptors are Volatile
    DescriptorConstant,  // Data is Volatile and mDescriptors are constant
};

enum class GpuDescriptorVisibility : u8 {
    All,
    Vertex,
    Pixel,
};

struct GpuRootDescriptor
{
    u32                     mRootIndex;                                      // Root Parameter Index must be set
    GpuDescriptorType       mType           = GpuDescriptorType::Cbv;
    GpuDescriptorRangeFlags mFlags          = GpuDescriptorRangeFlags::None; // Volatile mDescriptors are disallowed for Root mDescriptors
    u32                     mShaderRegister = 0;
    u32                     mRegisterSpace  = 0;

    static constexpr u8 cDWordCount = 2;
};

struct GpuRootConstant
{
    u32 mRootIndex;          // Root Parameter Index must be set
    u32 mShaderRegister = 0;
    u32 mRegisterSpace  = 0;
    u32 mNum32bitValues = 1;

    static constexpr u8 cDWordCount = 1;
};

struct GpuDescriptorRange
{
    GpuDescriptorType       mType               = GpuDescriptorType::Cbv;
    u32                     mNumDescriptors     = 1;
    u32                     mBaseShaderRegister = 0;
    u32                     mRegisterSpace      = 0;
    u32                     mDescriptorOffset   = 0;
    GpuDescriptorRangeFlags mFlags              = GpuDescriptorRangeFlags::None;
};

struct GpuDescriptorTable
{
    u32                        mRootIndex; // Root Parameter Index must be set
    GpuDescriptorVisibility    mVisibility{GpuDescriptorVisibility::All};
    farray<GpuDescriptorRange> mDescriptorRanges = {};

    static constexpr u8 cDWordCount = 1;
};

struct GpuStaticSamplerDesc
{
    D3D12_FILTER               mFilter           = D3D12_FILTER_ANISOTROPIC;
    D3D12_TEXTURE_ADDRESS_MODE mAddressU         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    D3D12_TEXTURE_ADDRESS_MODE mAddressV         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    D3D12_TEXTURE_ADDRESS_MODE mAddressW         = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    FLOAT                      mMipLODBias       = 0;
    UINT                       mMaxAnisotropy    = 16;
    D3D12_COMPARISON_FUNC      mComparisonFunc   = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    D3D12_STATIC_BORDER_COLOR  mBorderColor      = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    FLOAT                      mMinLOD           = 0.0f;
    FLOAT                      mMaxLOD           = D3D12_FLOAT32_MAX;
    UINT                       mShaderRegister   = 0;
    UINT                       mRegisterSpace    = 0;
    D3D12_SHADER_VISIBILITY    mShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
};

struct GpuRootSignatureInfo
{
    farray<GpuDescriptorTable>        mDescriptorTables    = {};
    farray<GpuRootDescriptor>         mDescriptors         = {};
    farray<GpuRootConstant>           mDescriptorConstants = {};
    farray<D3D12_STATIC_SAMPLER_DESC> mStaticSamplers      = {}; //TODO: use an abstraction
    // TODO: Static samplers
    std::string                  mName                = {}; // Optional, Set only in debug mode only
};

class GpuRootSignature
{
public:
	GpuRootSignature() = default;
    GpuRootSignature(const GpuDevice& tDevice, const GpuRootSignatureInfo& tInfo);

    u32 getDescriptorTableBitmask(GpuDescriptorType HeapType) const;
    u32 getNumDescriptors(u32 RootIndex)                        const;
    u32 getRootParameterCount()                                 const { return mRootParameterCount; }

    struct ID3D12RootSignature* asHandle() const { return mHandle; }

    void release() { if (mHandle) ComSafeRelease(mHandle); }

private:
    // RootSignature is limited by the number of "bindings" it can have. The upper bound is 64 DWords (64 * 4 bytes).
    static constexpr u8 cMaxDWordCount = 64;

	struct ID3D12RootSignature* mHandle                     = 0;
    // Total number of root parameters in the root signature
    u32                         mRootParameterCount         = 0;
    // Need to know number of descriptors per table. A maximum of 64 descriptor tables are supported
    u32                         mNumDescriptorsPerTable[cMaxDWordCount] = {};
    // A bitmask that represents the root parameter indices that are descriptor tables for Samplers
    u32                         mSamplerTableBitmask        = 0;
    // A bitmask that represents the root parameter indices that are CBV/UAV/SRV descriptor tables
    u32                         mDescriptorTableBitmask     = 0;
};
