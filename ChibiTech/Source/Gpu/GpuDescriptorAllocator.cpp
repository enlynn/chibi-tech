#include "GpuDescriptorAllocator.h"
#include "GpuDevice.h"

#include <Platform/Assert.h>

D3D12_CPU_DESCRIPTOR_HANDLE 
CpuDescriptor::getDescriptorHandle(u32 Offset) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE Result = mCpuDescriptor;
	Result.ptr += (u64)Offset * mDescriptorStride;
	return Result;
}

void 
CpuDescriptorPage::init(GpuDevice &Device, D3D12_DESCRIPTOR_HEAP_TYPE Type, u32 MaxDescriptors)
{
	auto DeviceHandle = Device.asHandle();

	mType             = Type;
	mTotalDescriptors = MaxDescriptors;

	mFreeDescriptors = std::vector<FreeDescriptorBlock>();
    mFreeDescriptors.reserve(10);

	mHeap = Device.createDescriptorHeap(mType, mTotalDescriptors, false);

	mBaseDescriptor   = mHeap->GetCPUDescriptorHandleForHeapStart();
	mDescriptorStride = DeviceHandle->GetDescriptorHandleIncrementSize(mType);
	mFreeHandles      = 0;

    freeBlock(0, mTotalDescriptors);
}

void 
CpuDescriptorPage::deinit()
{
	mFreeDescriptors.clear();
	ComSafeRelease(mHeap);

	mTotalDescriptors = 0;
	mFreeHandles      = 0;
}

CpuDescriptor
CpuDescriptorPage::allocate(u32 Count)
{
	CpuDescriptor Descriptor = {};

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
		//Descriptor.mPageIndex; <- Assigned by CpuDescriptorAllocator

		mFreeHandles -= Count;
	}

	return Descriptor;
}

void 
CpuDescriptorPage::releaseDescriptors(CpuDescriptor CpuDescriptors)
{
	u32 Offset = (u32) computeOffset(CpuDescriptors.mCpuDescriptor);
    freeBlock(Offset, CpuDescriptors.mNumHandles);
}

constexpr bool 
CpuDescriptorPage::hasSpace(u32 Count) const
{
	s64 Leftover = (s64)mFreeHandles - (s64)Count;
	return Leftover >= 0;
}

u64
CpuDescriptorPage::computeOffset(D3D12_CPU_DESCRIPTOR_HANDLE Handle)
{
	return (mBaseDescriptor.ptr - Handle.ptr) / mDescriptorStride;
}

void 
CpuDescriptorPage::freeBlock(u32 Offset, u32 NumDescriptors)
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
		FreeDescriptorBlock NewBlock = { .Offset = Offset, .Count = NumDescriptors };
		mFreeDescriptors.insert(mFreeDescriptors.begin() + InsertIndex, NewBlock);
	}
}

void CpuDescriptorAllocator::init(GpuDevice *Device, D3D12_DESCRIPTOR_HEAP_TYPE Type, u32 CountPerHeap)
{
	mDevice              = Device;
	mType                = Type;
	mDescriptorsPerPage  = CountPerHeap;
	mDescriptorPageCount = 1;

    mDescriptorPages[0].init(*mDevice, mType, mDescriptorsPerPage);
}

void CpuDescriptorAllocator::deinit()
{
	ForRange(u32, i, mDescriptorPageCount)
	{
        mDescriptorPages[i].deinit();
		mDevice              = nullptr;
		mDescriptorsPerPage  = 0;
		mDescriptorPageCount = 0;
	}
}

// Allocates a number of contiguous descriptors from a CPU visible heap. Cannot be more than the number 
// of descriptors per heap.
CpuDescriptor CpuDescriptorAllocator::allocate(u32 NumDescriptors)
{
	ASSERT(NumDescriptors < mDescriptorsPerPage);

	CpuDescriptor Result = {};

	ForRange(u32, i, mDescriptorPageCount)
	{
		if (mDescriptorPages[i].hasSpace(NumDescriptors))
		{
			Result = mDescriptorPages[i].allocate(NumDescriptors);
			if (!Result.isNull())
			{
				Result.mPageIndex = i;
				break;
			}
		}
	}

	if (Result.isNull())
	{
		ASSERT(mDescriptorPageCount < 256);
		
		// Create a new page and add it to the last. Attempt to allocate one more time - it is allowed to fail.
        mDescriptorPages[mDescriptorPageCount].init(*mDevice, mType, mDescriptorsPerPage);
		Result = mDescriptorPages[mDescriptorPageCount].allocate(NumDescriptors);

		Result.mPageIndex = mDescriptorPageCount;
		mDescriptorPageCount += 1;
	}

	return Result;
}


void 
CpuDescriptorAllocator::releaseDescriptors(CpuDescriptor Descriptors)
{
	if (!Descriptors.isNull())
	{
		ASSERT(Descriptors.mPageIndex < mDescriptorPageCount);
        mDescriptorPages[Descriptors.mPageIndex].releaseDescriptors(Descriptors);
	}
}