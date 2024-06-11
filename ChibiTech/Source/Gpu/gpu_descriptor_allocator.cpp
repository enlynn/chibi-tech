#include "gpu_descriptor_allocator.h"
#include "gpu_device.h"

#include <Platform/Assert.h>

D3D12_CPU_DESCRIPTOR_HANDLE 
cpu_descriptor::GetDescriptorHandle(u32 Offset) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE Result = mCpuDescriptor;
	Result.ptr += (u64)Offset * mDescriptorStride;
	return Result;
}

void 
cpu_descriptor_page::Init(gpu_device &Device, D3D12_DESCRIPTOR_HEAP_TYPE Type, u32 MaxDescriptors)
{
	auto DeviceHandle = Device.AsHandle();

	mType             = Type;
	mTotalDescriptors = MaxDescriptors;

	mFreeDescriptors = std::vector<free_descriptor_block>();
    mFreeDescriptors.reserve(10);

	mHeap = Device.CreateDescriptorHeap(mType, mTotalDescriptors, false);

	mBaseDescriptor   = mHeap->GetCPUDescriptorHandleForHeapStart();
	mDescriptorStride = DeviceHandle->GetDescriptorHandleIncrementSize(mType);
	mFreeHandles      = 0;

	FreeBlock(0, mTotalDescriptors);
}

void 
cpu_descriptor_page::Deinit()
{
	mFreeDescriptors.clear();
	ComSafeRelease(mHeap);

	mTotalDescriptors = 0;
	mFreeHandles      = 0;
}

cpu_descriptor
cpu_descriptor_page::Allocate(u32 Count)
{
	cpu_descriptor Descriptor = {};

	// First-Fit Search
	bool FoundSlot = false;
	u32 DescriptorIndex = 0;
	ForRange(u32, i, mFreeDescriptors.size())
	{
		if (Count <= mFreeDescriptors[i].Count)
		{
			FoundSlot       = true;
			DescriptorIndex = i;
			break;
		}
	}

	if (FoundSlot)
	{
		u32 Offset = mFreeDescriptors[DescriptorIndex].Offset;

		// It might be that our allocation is too large, so let's see if it can be split into multiple blocks
		if (Count < mFreeDescriptors[DescriptorIndex].Count)
		{
			mFreeDescriptors[DescriptorIndex].Count  -= Count;
			mFreeDescriptors[DescriptorIndex].Offset += Count;
		}
		else
		{
			mFreeDescriptors.erase(mFreeDescriptors.begin() + DescriptorIndex);
		}

		Descriptor.mDescriptorStride  = mDescriptorStride;
		Descriptor.mCpuDescriptor.ptr = mBaseDescriptor.ptr + (u64)Offset * mDescriptorStride;
		Descriptor.mNumHandles        = Count;
		//Descriptor.mPageIndex; <- Assigned by cpu_descriptor_allocator

		mFreeHandles -= Count;
	}

	return Descriptor;
}

void 
cpu_descriptor_page::ReleaseDescriptors(cpu_descriptor Descriptors)
{
	u32 Offset = (u32)ComputeOffset(Descriptors.mCpuDescriptor);
	FreeBlock(Offset, Descriptors.mNumHandles);
}

constexpr bool 
cpu_descriptor_page::HasSpace(u32 Count) const
{
	s64 Leftover = (s64)mFreeHandles - (s64)Count;
	return Leftover >= 0;
}

u64
cpu_descriptor_page::ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE Handle)
{
	return (mBaseDescriptor.ptr - Handle.ptr) / mDescriptorStride;
}

void 
cpu_descriptor_page::FreeBlock(u32 Offset, u32 NumDescriptors)
{
	mFreeHandles += NumDescriptors;

	// Find Where we need to insert the new block
	u32 InsertIndex = (u32)mFreeDescriptors.size();
	ForRange(u32, i, mFreeDescriptors.size())
	{
		if (Offset < mFreeDescriptors[i].Offset)
		{
			InsertIndex = i;
			break;
		}
	}

	// This will find us:
	// Previous Block(InsertIndex - 1) | New Block | Next Block (InsertIndex)
	s32 PrevBlockIndex = (InsertIndex > 0)                       ? static_cast<s32>(InsertIndex) - 1 : -1;
	s32 NextBlockIndex = (InsertIndex < mFreeDescriptors.size()) ? static_cast<s32>(InsertIndex)     : -1;

	// With this, attempt to merge to either side
	bool DidMerge     = false; // Did any merge happen?
	bool DidMergeLeft = false; // Did we merge to the left?

	// Attempt to merge the left block
	// The previous block is exactly behind this block
	//
	// prevblock.offset                  offset
	// |                                 |
	// | <------ prevblock.Count ------> | <------ Count ------> |
	//
	if (PrevBlockIndex >= 0 && mFreeDescriptors[u64(PrevBlockIndex)].Offset + mFreeDescriptors[u64(PrevBlockIndex)].Count == Offset)
	{
		mFreeDescriptors[u64(PrevBlockIndex)].Count += NumDescriptors;
		DidMerge     = true;
		DidMergeLeft = true;
	}

	// Attempt to merge to the right
	// The next block is exactly after this block
	//
	// offset                 nextblock.offset
	// |                      |
	// | <------ size ------> | <------ nextblock.size ------> | 
	//
	if (NextBlockIndex >= 0 && Offset + NumDescriptors == mFreeDescriptors[u64(NextBlockIndex)].Offset)
	{
		if (DidMergeLeft)
		{ // Need to move the right block to the left
			mFreeDescriptors[u64(PrevBlockIndex)].Count += mFreeDescriptors[u64(NextBlockIndex)].Count;
			mFreeDescriptors.erase(mFreeDescriptors.begin() + NextBlockIndex);
		}
		else
		{ // We can plop the new block here
			mFreeDescriptors[u64(NextBlockIndex)].Count += NumDescriptors;
		}

		DidMerge = true;
	}

	if (!DidMerge)
	{
		free_descriptor_block NewBlock = { .Offset = Offset, .Count = NumDescriptors };
		mFreeDescriptors.insert(mFreeDescriptors.begin() + InsertIndex, NewBlock);
	}
}

void cpu_descriptor_allocator::Init(gpu_device *Device, D3D12_DESCRIPTOR_HEAP_TYPE Type, u32 CountPerHeap)
{
	mDevice              = Device;
	mType                = Type;
	mDescriptorsPerPage  = CountPerHeap;
	mDescriptorPageCount = 1;

    mDescriptorPages[0].Init(*mDevice, mType, mDescriptorsPerPage);
}

void cpu_descriptor_allocator::Deinit()
{
	ForRange(u32, i, mDescriptorPageCount)
	{
		mDescriptorPages[i].Deinit();
		mDevice              = nullptr;
		mDescriptorsPerPage  = 0;
		mDescriptorPageCount = 0;
	}
}

// Allocates a number of contiguous descriptors from a CPU visible heap. Cannot be more than the number 
// of descriptors per heap.
cpu_descriptor cpu_descriptor_allocator::Allocate(u32 NumDescriptors)
{
	ASSERT(NumDescriptors < mDescriptorsPerPage);

	cpu_descriptor Result = {};

	ForRange(u32, i, mDescriptorPageCount)
	{
		if (mDescriptorPages[i].HasSpace(NumDescriptors))
		{
			Result = mDescriptorPages[i].Allocate(NumDescriptors);
			if (!Result.IsNull())
			{
				Result.mPageIndex = i;
				break;
			}
		}
	}

	if (Result.IsNull())
	{
		ASSERT(mDescriptorPageCount < 256);
		
		// Create a new page and add it to the last. Attempt to allocate one more time - it is allowed to fail.
        mDescriptorPages[mDescriptorPageCount].Init(*mDevice, mType, mDescriptorsPerPage);
		Result = mDescriptorPages[mDescriptorPageCount].Allocate(NumDescriptors);

		Result.mPageIndex = mDescriptorPageCount;
		mDescriptorPageCount += 1;
	}

	return Result;
}


void 
cpu_descriptor_allocator::ReleaseDescriptors(cpu_descriptor Descriptors)
{
	if (!Descriptors.IsNull())
	{
		ASSERT(Descriptors.mPageIndex < mDescriptorPageCount);
		mDescriptorPages[Descriptors.mPageIndex].ReleaseDescriptors(Descriptors);
	}
}