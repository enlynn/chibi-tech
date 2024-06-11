#pragma once

#include <types.h>
#include <vector>

#include "D3D12Common.h"

class GpuDevice;
class allocator;

constexpr static int cMaxDesctiptorPages = 256;

struct CpuDescriptor
{
	D3D12_CPU_DESCRIPTOR_HANDLE mCpuDescriptor    = {.ptr = 0 };
	u32                         mNumHandles       = 0;
	u32                         mDescriptorStride = 0;
	u8                          mPageIndex        = 0;     

	D3D12_CPU_DESCRIPTOR_HANDLE getDescriptorHandle(u32 Offset = 0) const;
	inline bool                 isNull()                            const { return mCpuDescriptor.ptr == 0; }

	// NOTE(enlynn): The old implementation held backpointers to the owning allocator. 
	// Is that really necessary?
};

class CpuDescriptorPage
{
public:
	CpuDescriptorPage() = default;
	
	void init(GpuDevice &Device, D3D12_DESCRIPTOR_HEAP_TYPE Type, u32 MaxDescriptors);
	void deinit();

	CpuDescriptor allocate(u32 Count = 1);
	void          releaseDescriptors(CpuDescriptor CpuDescriptors);

	constexpr bool hasSpace(u32 Count) const;   // Get the total number of available descriptors. This does not mean we have a
												// contiguous chunk of allocators. 
	constexpr u32  numFreeHandles()    const { return mFreeHandles; }

private:
	struct FreeDescriptorBlock
	{
		u32 Offset;
		u32 Count;
	};

	ID3D12DescriptorHeap*         mHeap             = nullptr;
	D3D12_DESCRIPTOR_HEAP_TYPE    mType             = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	D3D12_CPU_DESCRIPTOR_HANDLE   mBaseDescriptor   = {0};

	std::vector<FreeDescriptorBlock> mFreeDescriptors  = {};

	u32                           mTotalDescriptors = 0;
	u32                           mDescriptorStride = 0;
	u32                           mFreeHandles      = 0;

	u64  computeOffset(D3D12_CPU_DESCRIPTOR_HANDLE Handle);
	void freeBlock(u32 Offset, u32 NumDescriptors);
};

//
// Allocates CPU-Visibile descriptors using a paged-block allocator. There can be a maximum
// of 255 pages, where each page acts as a block allocator.
// 
// This allocators works on the following descriptor types:
// - CBV_SRV_UAV
// - SAMPLER
// - RTV
// - DSV
//
class CpuDescriptorAllocator
{
public:
	CpuDescriptorAllocator() = default;

	void init(GpuDevice *Device, D3D12_DESCRIPTOR_HEAP_TYPE Type, u32 CountPerHeap = 256);
	void deinit();

	// Allocates a number of contiguous descriptors from a CPU visible heap. Cannot be more than the number 
	// of descriptors per heap.
	CpuDescriptor allocate(u32 NumDescriptors = 1);

	void releaseDescriptors(CpuDescriptor Descriptors);

private:
	GpuDevice*                 mDevice = nullptr;
	CpuDescriptorPage          mDescriptorPages[cMaxDesctiptorPages] = {}; // If this fills up, we have a problem.
	u8                         mDescriptorPageCount                  = 0;  // The index of the next page to grab.

	D3D12_DESCRIPTOR_HEAP_TYPE mType                                 = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	u32                        mDescriptorsPerPage                   = 256;
};