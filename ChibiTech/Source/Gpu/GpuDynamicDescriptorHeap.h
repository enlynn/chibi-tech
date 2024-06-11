#pragma once

#include <Types.h>
#include "D3D12Common.h"

#include <vector>

class GpuDevice;
class GpuCommandList;
class GpuRootSignature;

//
// GpuDynamicDescriptorHeap is an allocator for creating Per-Frame GPU-visible descriptors. The allocator stores
// a cache of bitmasks that represents descriptor bindings into the active Root Signature. Descriptor Heaps and Descriptor
// Copies only occur for bitmasks that have become stale (new bindings). This reduces redundant Descriptor Copies and
// bindings. The Gpu Descriptor Heap is treated as an Arena Allocator - once one heap fills up, another heap is created. The 
// Heaps are all reset with a single call to GpuDynamicDescriptorHeap::reset() and all previous GPU descriptors are invalidated.
//

enum class DynamicHeapType : u8
{ // Only these two types are allowed GPU-visible descriptors
    Buffer,  // Maps to D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
    Sampler, // Maps to D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
    Max,
};

class GpuDynamicDescriptorHeap
{
public:
    GpuDynamicDescriptorHeap() = default;
    GpuDynamicDescriptorHeap(GpuDevice *Device, DynamicHeapType Type, u32 CountPerHeap = 1024);
    void deinit();

    // Stages a contiguous range of CPU descriptors. mDescriptors are not copied to the
    // GPU visible descriptor heap until the CommitStagedDescriptors function is called
    void stageDescriptors(u32 RootParameterIndex, u32 DescriptorOffset, u32 NumDescriptors, D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptorHandle);

    //
    // TODO(enlynn): These functions have an annoying cyclical dependency with GpuCommandList. A Command List calls into the descriptor
    // heap, then the descriptor heaps calls into the command list to actually bind the descriptors. It would be nice if this didn't happen.
    //
    // Copy all of the descriptors to the GPU visible descriptor heap and bind the descriptor heap and the descriptor tables to the
    // command list. The passed-in function object is used to set the GPU visible descriptors on the command list. Two posible functions:
    // - Before a draw:     ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable
    // - Before a dispatch: ID3D12GraphicsCommandList::SetComputeRootDescriptorTable
    //
    using commit_descriptor_table_pfn  = void (*)(ID3D12GraphicsCommandList* CommandList, UINT DescriptorRootIndex, D3D12_GPU_DESCRIPTOR_HANDLE GpuDescriptorHandle);
    using commit_descriptor_inline_pfn = void (*)(ID3D12GraphicsCommandList* CommandList, UINT DescriptorRootIndex, D3D12_GPU_VIRTUAL_ADDRESS   GpuDescriptorHandle);

    void commitDescriptorTables(GpuCommandList* CommandList, commit_descriptor_table_pfn CommitTablesPFN);
    void commitInlineDescriptors(GpuCommandList* CommandList, D3D12_GPU_VIRTUAL_ADDRESS* GPuDescriptorHandles, u32* DescriptorBitmask, commit_descriptor_inline_pfn CommitInlinePFN);

    void commitStagedDescriptorsForDraw(GpuCommandList* CommandList);
    void commitStagedDescriptorsForDispatch(GpuCommandList* CommandList);

    //
    // Copies a single CPU visible descriptor to a GPU visible descriptor heap.
    // Useful for the
    // - ID3D12GraphicsCommandList::ClearUnorderedAccessViewFloat
    // - ID3D12GraphicsCommandList::ClearUnorderedAccessViewUint
    // functions which require both a CPU and GPU visible descriptors for a UAV Resource.
    //
    D3D12_GPU_DESCRIPTOR_HANDLE copyDescriptor(GpuCommandList* CommandList, D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptorHandle);

    // parse root signature to determine which root parameters contain descriptor tables and determine the number of
    // descriptors needed for each table
    void parseRootSignature(const GpuRootSignature& RootSignature);

    // reset used descriptors. This should only be done if any
    // descriptors that are being referenced by a command list 
    // has finished executing on the command queue
    void reset();

    // Stage inline descriptors
    void stageInlineCbv(u32 RootIndex, D3D12_GPU_VIRTUAL_ADDRESS GpuDescriptorHandle);
    void stageInlineSrv(u32 RootIndex, D3D12_GPU_VIRTUAL_ADDRESS GpuDescriptorHandle);
    void stageInlineUav(u32 RootIndex, D3D12_GPU_VIRTUAL_ADDRESS GpuDescriptorHandle);

private:
    // Updates the current heap if the active heap is not large enough for the number of descriptors needed to commit
    void                  updateCurrentHeap(GpuCommandList* CommandList, u32 NumDescriptorsToCommit);
    // Request a heap - if one is available
    ID3D12DescriptorHeap* requestDescriptorHeap();
    // mCompute the number of stale descriptors that need to be copied to the GPU visible descriptor heap
    u32 computeStaleDescriptorTableCount();

    static const u8 cMaxDescriptorTables  = 64; // A Descriptor Table is 1 DWORD, and a Root Signature can hold up to 64 Tables
    static const u8 cMaxInlineDescriptors = 32; // An Inline Descriptor is 2 DWORD, and a Root Signature can hold up to 32 Inline mDescriptors

    struct DescriptorTableCache
    {
        u32                          mNumDescriptors = 0;
        // A Pointer into the mCpuHandleCache
        D3D12_CPU_DESCRIPTOR_HANDLE* mBaseDescriptor = 0;
    };

    GpuDevice*                          mDevice             = nullptr;

    // Describes the type of descriptors that can be staged using this
    // Dynamic descriptor heap. Valid values are:
    // - D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
    // - D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
    D3D12_DESCRIPTOR_HEAP_TYPE           mHeapType           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    u32                                  mDescriptorsPerHeap = 256;
    u32                                  mDescriptorStride   = 0;

    // Since of CPU Descriptor Handles that will be mapped to GPU Descriptor Handles
    // Size of this array matches the number of allowed GPU Handles in a Heap.
    D3D12_CPU_DESCRIPTOR_HANDLE*         mCpuHandleCache     = nullptr; // Size == mDescriptorsPerHeap

    // List of allocated descriptor heap pools. Acts as a backing storage
    // for reset calls. On reset, the heap pool is copied over to
    // avail_descriptors.
    std::vector<ID3D12DescriptorHeap*>   mDescriptorHeapList = {};
    u32                                  mNextAvailableHeap  = 0;

    // Current descriptor heap bound to the command list
    ID3D12DescriptorHeap*                mCurrentHeap      = nullptr;
    D3D12_GPU_DESCRIPTOR_HANDLE          mCurrentGpuHandle = { .ptr = 0 };
    D3D12_CPU_DESCRIPTOR_HANDLE          mCurrentCpuHandle = { .ptr = 0 };
    u32                                  mNumFreeHandles   = 0;

    // Cached mDescriptors
    DescriptorTableCache               mDescriptorTableCache[cMaxDescriptorTables] = {};
    D3D12_GPU_VIRTUAL_ADDRESS            mInlineCbv[cMaxInlineDescriptors]           = {};
    D3D12_GPU_VIRTUAL_ADDRESS            mInlineSrv[cMaxInlineDescriptors]           = {};
    D3D12_GPU_VIRTUAL_ADDRESS            mInlineUav[cMaxInlineDescriptors]           = {};

    // Each bit flagged to 1 represents the index into the Root Signature with a descriptor table
    u64                                  mCachedDescriptorTableBitmask = 0;
    // Each bit flagged to 1 represents the index into the descriptor that has been changed and 
    // needs to be copied to a GPU descriptor
    u64                                  mStaleDescriptorTableBitmask  = 0;
    u32                                  mStaleCbvBitmask              = 0;
    u32                                  mStaleSrvBitmask              = 0;
    u32                                  mStaleUavBitmask              = 0;
};