#include "GpuRootSignature.h"
#include "GpuDevice.h"

#include <Platform/Platform.h>
#include <Platform/Assert.h>
#include <Platform/Console.h>
#include <Util/bit.h>

inline D3D12_DESCRIPTOR_RANGE_TYPE 
GfxDescriptorTypeToD3D12(GpuDescriptorType Type)
{
    switch (Type)
    {
        case GpuDescriptorType::Cbv:     return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        case GpuDescriptorType::Sampler: return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        case GpuDescriptorType::Srv:     return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        case GpuDescriptorType::Uav:     return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    }

    ASSERT_CUSTOM(false, "Unsupported descriptor type.");
    return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
}

inline D3D12_ROOT_PARAMETER_TYPE
GfxDescriptorTypeToDescriptorParamter(GpuDescriptorType Type)
{
    switch (Type)
    {
        case GpuDescriptorType::Cbv:     return D3D12_ROOT_PARAMETER_TYPE_CBV;
        case GpuDescriptorType::Srv:     return D3D12_ROOT_PARAMETER_TYPE_SRV;
        case GpuDescriptorType::Uav:     return D3D12_ROOT_PARAMETER_TYPE_UAV;
        case GpuDescriptorType::Sampler: // Intentional fallthrough
        default:                           ASSERT(false && "Unsupported descriptor type for a Root Parameter.");
    }

    // this is technically unreachable, but clangd thinks otherwise
    return D3D12_ROOT_PARAMETER_TYPE_CBV;
}

inline D3D12_SHADER_VISIBILITY toD3D12Visibility(GpuDescriptorVisibility tVisbility) {
    D3D12_SHADER_VISIBILITY result = D3D12_SHADER_VISIBILITY_ALL;
    switch (tVisbility) {
        case GpuDescriptorVisibility::Vertex: result = D3D12_SHADER_VISIBILITY_VERTEX; break;
        case GpuDescriptorVisibility::Pixel:  result = D3D12_SHADER_VISIBILITY_PIXEL;  break;
        case GpuDescriptorVisibility::All:    //intentional fallthrough
        default:                              result = D3D12_SHADER_VISIBILITY_ALL;    break;
    }
    return result;
}

GpuRootSignature::GpuRootSignature(const GpuDevice& tDevice, const GpuRootSignatureInfo& tInfo)
{
    // A Root Descriptor can have up to 64 DWORDS, so let's make sure the info fits
    u64 RootSignatureCost = 0;
    RootSignatureCost += tInfo.mDescriptorTables.Length() * GpuDescriptorTable::cDWordCount;
    RootSignatureCost += tInfo.mDescriptors.Length() * GpuRootDescriptor::cDWordCount;
    RootSignatureCost += tInfo.mDescriptorConstants.Length() * GpuRootConstant::cDWordCount;

    if (RootSignatureCost > GpuRootSignature::cMaxDWordCount)
    {
#if CT_DEBUG
        if (!tInfo.mName.empty())
        {
            ct::console::fatal("Attempting to create a Root Signature with too many descriptors: %d", RootSignatureCost);
        }
        else
#endif
        {
            ASSERT(false && "Too big of a root signature.");
        }
    }

    //
    // Build the root signature description
    // 

    // Ok, so let's just declare a reasonable upper bound here. If we hit it then, uh, let's worry about it then.
    constexpr int MaxDescriptorRanges = 64;
    D3D12_DESCRIPTOR_RANGE1 Ranges[MaxDescriptorRanges] = {};
    u32 TotalDescriptorRangeCount = 0;

    D3D12_ROOT_PARAMETER1 RootParameters[GpuRootSignature::cMaxDWordCount];
    u32 ParameterCount = 0;

    // Collect the descriptor tables
    for (const auto& Table : tInfo.mDescriptorTables)
    {
        D3D12_ROOT_PARAMETER1& Parameter = RootParameters[Table.mRootIndex];
        Parameter.ParameterType          = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        Parameter.ShaderVisibility       = toD3D12Visibility(Table.mVisibility); //D3D12_SHADER_VISIBILITY_ALL; // TODO(enlynn): make modifiable
        
        ASSERT(TotalDescriptorRangeCount + Table.mDescriptorRanges.Length() < MaxDescriptorRanges);

        u32 BaseRangeIndex = TotalDescriptorRangeCount;
        u64 RangeCount     = Table.mDescriptorRanges.Length();

        bool FoundSampler = false;
        bool FoundCSU     = false; // Found a CBV, UAV, or SRV descriptor range.

        mNumDescriptorsPerTable[Table.mRootIndex] = 0;

        for (const auto& Range : Table.mDescriptorRanges)
        {
            D3D12_DESCRIPTOR_RANGE1& DescriptorRange = Ranges[TotalDescriptorRangeCount];

            switch (Range.mType)
            {
                case GpuDescriptorType::Cbv:     DescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;     FoundCSU     = true; break;
                case GpuDescriptorType::Uav:     DescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;     FoundCSU     = true; break;
                case GpuDescriptorType::Srv:     DescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;     FoundCSU     = true; break;
                case GpuDescriptorType::Sampler: DescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER; FoundSampler = true; break;
                default:                           break;
            }

            switch (Range.mFlags)
            {
                case GpuDescriptorRangeFlags::Constant:
                { // Data is Constant and mDescriptors are Constant. Best for driver optimization.
                    DescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
                } break;

                case GpuDescriptorRangeFlags::Dynamic:
                { // Data is Volatile and mDescriptors are Volatile
                    DescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE; 
                } break;

                case GpuDescriptorRangeFlags::DataConstant:
                { // Data is Constant and mDescriptors are Volatile
                    DescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE; 
                } break;

                case GpuDescriptorRangeFlags::DescriptorConstant:
                { // Data is Volatile and mDescriptors are constant
                    DescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
                } break;

                case GpuDescriptorRangeFlags::None: // Intentional Fallthrough
                default:
                { // Default behavior, SRV/CBV are static at execute, UAV are volatile 
                    DescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
                } break;
            }

            DescriptorRange.NumDescriptors                    = Range.mNumDescriptors;
            DescriptorRange.BaseShaderRegister                = Range.mBaseShaderRegister;
            DescriptorRange.RegisterSpace                     = Range.mRegisterSpace;
            DescriptorRange.OffsetInDescriptorsFromTableStart = Range.mDescriptorOffset;
            DescriptorRange.RangeType                         = GfxDescriptorTypeToD3D12(Range.mType);

            TotalDescriptorRangeCount += 1;

            mNumDescriptorsPerTable[Table.mRootIndex] += Range.mNumDescriptors;
        }

        if (FoundCSU && FoundSampler)
        {
            ct::console::fatal("Found a descriptor table that contained both CBV/SRV/UAV and a Sampler. This is not allowed.");
        }
        else if (FoundCSU)
        {
            mDescriptorTableBitmask = BitSet(mDescriptorTableBitmask, Table.mRootIndex);
        }
        else if (FoundSampler)
        {
            mSamplerTableBitmask = BitSet(mSamplerTableBitmask, Table.mRootIndex);
        }

        if (RangeCount > 0)
        {
            Parameter.DescriptorTable.NumDescriptorRanges = (UINT)RangeCount;
            Parameter.DescriptorTable.pDescriptorRanges   = Ranges + BaseRangeIndex;
        }
        else
        {
            Parameter.DescriptorTable.NumDescriptorRanges = 0;
            Parameter.DescriptorTable.pDescriptorRanges   = nullptr;
        }

        ParameterCount += 1;
    }

    // Collect the root (inline) descriptors
    for (const auto& Descriptor : tInfo.mDescriptors)
    {
        D3D12_ROOT_PARAMETER1& Parameter = RootParameters[Descriptor.mRootIndex];
        Parameter.ParameterType             = GfxDescriptorTypeToDescriptorParamter(Descriptor.mType);
        Parameter.ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL; // TODO(enlynn): make modifiable
        Parameter.Descriptor.RegisterSpace  = Descriptor.mRegisterSpace;
        Parameter.Descriptor.ShaderRegister = Descriptor.mShaderRegister;

        switch (Descriptor.mFlags)
        {
            case GpuDescriptorRangeFlags::Constant:
            { // Data is Constant and mDescriptors are Constant. Best for driver optimization.
                Parameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
            } break;

            case GpuDescriptorRangeFlags::DataConstant:
            { // Data is Static.
                Parameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
            } break;

            case GpuDescriptorRangeFlags::DescriptorConstant:
            { // Data is Volatile and mDescriptors are constant
                Parameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE;
            } break;

            case GpuDescriptorRangeFlags::Dynamic: // Intentional Fallthrough
            case GpuDescriptorRangeFlags::None:    // Intentional Fallthrough
            default:
            { // Default behavior, SRV/CBV are static at execute, UAV are volatile 
                Parameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
            } break;
        }

        ParameterCount += 1;
    }

    // Collect the root constants
    for (const auto& Constant : tInfo.mDescriptorConstants)
    {
        D3D12_ROOT_PARAMETER1& Parameter = RootParameters[Constant.mRootIndex];
        Parameter.ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        Parameter.ShaderVisibility         = D3D12_SHADER_VISIBILITY_ALL; // TODO(enlynn): make modifiable
        Parameter.Constants.Num32BitValues = Constant.mNum32bitValues;
        Parameter.Constants.RegisterSpace  = Constant.mRegisterSpace;
        Parameter.Constants.ShaderRegister = Constant.mShaderRegister;

        ParameterCount += 1;
    }

    //
    // Create the Root Signature
    //

    // Query the supported Root Signature Version, we want version 1.1
    D3D12_FEATURE_DATA_ROOT_SIGNATURE FeatureData = {};
    FeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

    if (FAILED(tDevice.asHandle()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &FeatureData, sizeof(FeatureData))))
    {
        FeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC Desc = {};
    Desc.Version = FeatureData.HighestVersion;
    Desc.Desc_1_1.Flags             = D3D12_ROOT_SIGNATURE_FLAG_NONE; // NOTE(enlynn): Worth denying shader stages?
    Desc.Desc_1_1.NumParameters     = ParameterCount;
    Desc.Desc_1_1.pParameters       = &RootParameters[0];
    Desc.Desc_1_1.NumStaticSamplers = tInfo.mStaticSamplers.Length();
    Desc.Desc_1_1.pStaticSamplers   = tInfo.mStaticSamplers.Ptr();

    // Serialize the root signature.
    ID3DBlob* RootSignatureBlob = nullptr;
    ID3DBlob* ErrorBlob         = nullptr;
    AssertHr(D3D12SerializeVersionedRootSignature(&Desc, &RootSignatureBlob, &ErrorBlob));

    // Create the root signature.
    AssertHr(tDevice.asHandle()->CreateRootSignature(0, RootSignatureBlob->GetBufferPointer(), RootSignatureBlob->GetBufferSize(), ComCast(&mHandle)));

#if defined(CT_DEBUG)
    auto wideName = ct::os::utf8ToUtf16(tInfo.mName);
    mHandle->SetName(wideName.c_str());
#endif

    mRootParameterCount = ParameterCount;
}

u32 
GpuRootSignature::getDescriptorTableBitmask(GpuDescriptorType HeapType) const
{
    u32 Result = 0;
    switch (HeapType)
    {
        case GpuDescriptorType::Srv:     //Intentional Fallthrough
        case GpuDescriptorType::Uav:     //Intentional Fallthrough
        case GpuDescriptorType::Cbv:     Result = mDescriptorTableBitmask; break;
        case GpuDescriptorType::Sampler: Result = mSamplerTableBitmask;    break;
    }
    return Result;
}

u32 
GpuRootSignature::getNumDescriptors(u32 RootIndex) const
{
    ASSERT(RootIndex < 32);
    return mNumDescriptorsPerTable[RootIndex];
}
