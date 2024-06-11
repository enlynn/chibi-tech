#pragma once

#include <types.h>
#include "D3D12Common.h"

#include <vector>

class gpu_device;
class gpu_command_list;
class gpu_root_signature;

//
// gpu_dynamic_descriptor_heap is an allocator for creating Per-Frame GPU-visible descriptors. The allocator stores
// a cache of bitmasks that represents descriptor bindings into the active Root Signature. Descriptor Heaps and Descriptor
// Copies only occur for bitmasks that have become stale (new bindings). This reduces redundant Descriptor Copies and
// bindings. The Gpu Descriptor Heap is treated as an Arena Allocator - once one heap fills up, another heap is created. The 
// Heaps are all reset with a single call to gpu_dynamic_descriptor_heap::Reset() and all previous GPU descriptors are invalidated. 
//

enum class dynamic_heap_type : u8
{ // Only these two types are allowed GPU-visible descriptors
    buffer,  // Maps to D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
    sampler, // Maps to D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
    max,
};

class gpu_dynamic_descriptor_heap
{
public:
    gpu_dynamic_descriptor_heap() = default;
    gpu_dynamic_descriptor_heap(gpu_device *Device, dynamic_heap_type Type, u32 CountPerHeap = 1024);
    void Deinit();

    // Stages a contiguous range of CPU descriptors. Descriptors are not copied to the
    // GPU visible descriptor heap until the CommitStagedDescriptors function is called
    void StageDescriptors(u32 RootParameterIndex, u32 DescriptorOffset, u32 NumDescriptors, D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptorHandle);

    //
    // TODO(enlynn): These functions have an annoying cyclical dependency with gpu_command_list. A Command List calls into the descriptor
    // heap, then the descriptor heaps calls into the command list to actually bind the descriptors. It would be nice if this didn't happen.
    //
    // Copy all of the descriptors to the GPU visible descriptor heap and bind the descriptor heap and the descriptor tables to the
    // command list. The passed-in function object is used to set the GPU visible descriptors on the command list. Two posible functions:
    // - Before a draw:     ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable
    // - Before a dispatch: ID3D12GraphicsCommandList::SetComputeRootDescriptorTable
    //
    using commit_descriptor_table_pfn  = void (*)(ID3D12GraphicsCommandList* CommandList, UINT DescriptorRootIndex, D3D12_GPU_DESCRIPTOR_HANDLE GpuDescriptorHandle);
    using commit_descriptor_inline_pfn = void (*)(ID3D12GraphicsCommandList* CommandList, UINT DescriptorRootIndex, D3D12_GPU_VIRTUAL_ADDRESS   GpuDescriptorHandle);

    void CommitDescriptorTables(gpu_command_list* CommandList, commit_descriptor_table_pfn CommitTablesPFN);
    void CommitInlineDescriptors(gpu_command_list* CommandList, D3D12_GPU_VIRTUAL_ADDRESS* GPuDescriptorHandles, u32* DescriptorBitmask, commit_descriptor_inline_pfn CommitInlinePFN);

    void CommitStagedDescriptorsForDraw(gpu_command_list* CommandList);
    void CommitStagedDescriptorsForDispatch(gpu_command_list* CommandList);

    //
    // Copies a single CPU visible descriptor to a GPU visible descriptor heap.
    // Useful for the
    // - ID3D12GraphicsCommandList::ClearUnorderedAccessViewFloat
    // - ID3D12GraphicsCommandList::ClearUnorderedAccessViewUint
    // functions which require both a CPU and GPU visible descriptors for a UAV Resource.
    //
    D3D12_GPU_DESCRIPTOR_HANDLE CopyDescriptor(gpu_command_list* CommandList, D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptorHandle);

    // parse root signature to determine which root parameters contain descriptor tables and determine the number of
    // descriptors needed for each table
    void ParseRootSignature(const gpu_root_signature& RootSignature);

    // Reset used descriptors. This should only be done if any
    // descriptors that are being referenced by a command list 
    // has finished executing on the command queue
    void Reset();

    // Stage inline descriptors
    void StageInlineCBV(u32 RootIndex, D3D12_GPU_VIRTUAL_ADDRESS GpuDescriptorHandle);
    void StageInlineSRV(u32 RootIndex, D3D12_GPU_VIRTUAL_ADDRESS GpuDescriptorHandle);
    void StageInlineUAV(u32 RootIndex, D3D12_GPU_VIRTUAL_ADDRESS GpuDescriptorHandle);

private:
    // Updates the current heap if the active heap is not large enough for the number of descriptors needed to commit
    void                  UpdateCurrentHeap(gpu_command_list* CommandList, u32 NumDescriptorsToCommit);
    // Request a heap - if one is available
    ID3D12DescriptorHeap* RequestDescriptorHeap();
    // mCompute the number of stale descriptors that need to be copied to the GPU visible descriptor heap
    u32 ComputeStaleDescriptorTableCount();

    static const u8 cMaxDescriptorTables  = 64; // A Descriptor Table is 1 DWORD, and a Root Signature can hold up to 64 Tables
    static const u8 cMaxInlineDescriptors = 32; // An Inline Descriptor is 2 DWORD, and a Root Signature can hold up to 32 Inline Descriptors

    struct descriptor_table_cache
    {
        u32                          mNumDescriptors = 0;
        // A Pointer into the mCpuHandleCache
        D3D12_CPU_DESCRIPTOR_HANDLE* mBaseDescriptor = 0;
    };

    gpu_device*                          mDevice             = nullptr;

    // Describes the type of descriptors that can be staged using this
    // dynamic descriptor heap. Valid values are:
    // - D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
    // - D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
    D3D12_DESCRIPTOR_HEAP_TYPE           mHeapType           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    u32                                  mDescriptorsPerHeap = 256;
    u32                                  mDescriptorStride   = 0;

    // Since of CPU Descriptor Handles that will be mapped to GPU Descriptor Handles
    // Size of this array matches the number of allowed GPU Handles in a Heap.
    D3D12_CPU_DESCRIPTOR_HANDLE*         mCpuHandleCache     = nullptr; // Size == mDescriptorsPerHeap

    // List of allocated descriptor heap pools. Acts as a backing storage
    // for Reset calls. On Reset, the heap pool is copied over to
    // avail_descriptors.
    std::vector<ID3D12DescriptorHeap*>   mDescriptorHeapList = {};
    u32                                  mNextAvailableHeap  = 0;

    // Current descriptor heap bound to the command list
    ID3D12DescriptorHeap*                mCurrentHeap      = nullptr;
    D3D12_GPU_DESCRIPTOR_HANDLE          mCurrentGpuHandle = { .ptr = 0 };
    D3D12_CPU_DESCRIPTOR_HANDLE          mCurrentCpuHandle = { .ptr = 0 };
    u32                                  mNumFreeHandles   = 0;

    // Cached Descriptors
    descriptor_table_cache               mDescriptorTableCache[cMaxDescriptorTables] = {};
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