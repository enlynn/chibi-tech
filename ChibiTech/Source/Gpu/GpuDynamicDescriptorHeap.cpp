#include "stdafx.h"
#include "GpuDynamicDescriptorHeap.h"

#include "GpuCommandList.h"
#include "GpuDevice.h"
#include "GpuRootSignature.h"

#include <Platform/Assert.h>
#include <util/bit.h>

static void
SetRootDescriptorGraphicsWrapper(ID3D12GraphicsCommandList* CommandList, UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE GpuDescriptorHandle)
{
	CommandList->SetGraphicsRootDescriptorTable(RootIndex, GpuDescriptorHandle);
}

static void
SetRootDescriptorComputeWrapper(ID3D12GraphicsCommandList* CommandList, UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE GpuDescriptorHandle)
{
	CommandList->SetComputeRootDescriptorTable(RootIndex, GpuDescriptorHandle);
}

static void
SetGraphicsRootCBVWrapper(ID3D12GraphicsCommandList* CommandList, UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS GpuDescriptorHandle)
{
	CommandList->SetGraphicsRootConstantBufferView(RootIndex, GpuDescriptorHandle);
}

static void
SetGraphicsRootSRVWrapper(ID3D12GraphicsCommandList* CommandList, UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS GpuDescriptorHandle)
{
	CommandList->SetGraphicsRootShaderResourceView(RootIndex, GpuDescriptorHandle);
}

static void
SetGraphicsRootUAVWrapper(ID3D12GraphicsCommandList* CommandList, UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS GpuDescriptorHandle)
{
	CommandList->SetGraphicsRootUnorderedAccessView(RootIndex, GpuDescriptorHandle);
}

static void
SetComputeRootCBVWrapper(ID3D12GraphicsCommandList* CommandList, UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS GpuDescriptorHandle)
{
	CommandList->SetComputeRootConstantBufferView(RootIndex, GpuDescriptorHandle);
}

static void
SetComputeRootSRVWrapper(ID3D12GraphicsCommandList* CommandList, UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS GpuDescriptorHandle)
{
	CommandList->SetComputeRootShaderResourceView(RootIndex, GpuDescriptorHandle);
}

static void
SetComputeRootUAVWrapper(ID3D12GraphicsCommandList* CommandList, UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS GpuDescriptorHandle)
{
	CommandList->SetComputeRootUnorderedAccessView(RootIndex, GpuDescriptorHandle);
}

GpuDynamicDescriptorHeap::GpuDynamicDescriptorHeap(GpuDevice *Device, DynamicHeapType Type, u32 CountPerHeap)
: mDevice(Device)
, mDescriptorsPerHeap(CountPerHeap)
, mDescriptorStride(mDevice->asHandle()->GetDescriptorHandleIncrementSize(mHeapType))
, mCpuHandleCache(new D3D12_CPU_DESCRIPTOR_HANDLE[mDescriptorsPerHeap])
, mDescriptorHeapList({})
{
	switch (Type)
	{
		case DynamicHeapType::Buffer: mHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; break;
		case DynamicHeapType::Sampler: mHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;     break;
	}
}

void 
GpuDynamicDescriptorHeap::deinit()
{
    if (mCpuHandleCache) {
        delete[] mCpuHandleCache;
        mCpuHandleCache = nullptr;
    }

	ForRange(u64, i, mDescriptorHeapList.size())
		ComSafeRelease(mDescriptorHeapList[i]);
	mDescriptorHeapList.clear();

	mDevice = nullptr;
}

void
GpuDynamicDescriptorHeap::reset()
{
	mNextAvailableHeap            = 0;
	mCurrentHeap                  = nullptr;
	mCurrentCpuHandle             = {};
	mCurrentGpuHandle             = {};
	mNumFreeHandles               = 0;
	mStaleDescriptorTableBitmask  = 0;
	mCachedDescriptorTableBitmask = 0;
	mStaleCbvBitmask              = 0;
	mStaleSrvBitmask              = 0;
	mStaleUavBitmask              = 0;

	// reset the table cache
	ForRange (int, i, cMaxDescriptorTables)
	{
		mDescriptorTableCache[i].mNumDescriptors = 0;
		mDescriptorTableCache[i].mBaseDescriptor = nullptr;
	}

	ForRange(int, i, cMaxInlineDescriptors)
	{
		mInlineCbv[i] = 0ull;
		mInlineSrv[i] = 0ull;
		mInlineUav[i] = 0ull;
	}
}

void
GpuDynamicDescriptorHeap::parseRootSignature(const GpuRootSignature& RootSignature)
{
	const u32 RootParameterCount = RootSignature.getRootParameterCount();

	// if the root sig changes, must rebind all descriptors to the command list
	mStaleDescriptorTableBitmask = 0;

	// Update the Descriptor Table Mask
	mCachedDescriptorTableBitmask = RootSignature.getDescriptorTableBitmask(GpuDescriptorType::Cbv);
	u64 TableBitmask              = mCachedDescriptorTableBitmask;

	u32 CurrentDescriptorOffset = 0;
	DWORD RootIndex;
	while (_BitScanForward64(&RootIndex, TableBitmask) && RootIndex < RootParameterCount)
	{
		u32 NumDescriptors = RootSignature.getNumDescriptors(RootIndex);
		ASSERT(CurrentDescriptorOffset <= mDescriptorsPerHeap && "Root signature requires more than the Max number of descriptors per descriptor heap. Consider enlaring the maximum.");

		// Pre-allocate the CPU Descriptor Ranges needed - overwriting the CPU Cache is safe because it is
		// the staging descriptor to eventually be copied to the GPU descriptor.
		DescriptorTableCache* TableCache = &mDescriptorTableCache[RootIndex];
		TableCache->mNumDescriptors = NumDescriptors;
		TableCache->mBaseDescriptor = mCpuHandleCache + CurrentDescriptorOffset;

		CurrentDescriptorOffset += NumDescriptors;

		// Flip the descirptor table so it's not scanned again
		TableBitmask = BitFlip(TableBitmask, (u64)RootIndex);
	}
}

void 
GpuDynamicDescriptorHeap::stageDescriptors(u32 RootParameterIndex, u32 DescriptorOffset, u32 NumDescriptors, D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptorHandle)
{
	// Cannot stage more than the maximum number od descriptors per heap
	// Cannot stage more than MaxDescriptorTables root params
	ASSERT(NumDescriptors <= mDescriptorsPerHeap || RootParameterIndex < cMaxDescriptorTables);

	// Check number of descriptors to copy does not exceed the number of descriptors expected in the descriptor table
	DescriptorTableCache* TableCache = &mDescriptorTableCache[RootParameterIndex];
	ASSERT((DescriptorOffset + NumDescriptors) <= TableCache->mNumDescriptors);

	// Copy the source CPU Descriptor into the Descriptor Table's CPU Descriptor Cache
	D3D12_CPU_DESCRIPTOR_HANDLE* CpuDescriptors = (TableCache->mBaseDescriptor + DescriptorOffset);
	ForRange (u32, DescriptorIndex, NumDescriptors)
	{
		CpuDescriptors[DescriptorIndex].ptr = CpuDescriptorHandle.ptr + DescriptorIndex * mDescriptorStride;
	}

	// Mark the root parameter for update
	mStaleDescriptorTableBitmask = BitSet(mStaleDescriptorTableBitmask, u64(RootParameterIndex));
}

void 
GpuDynamicDescriptorHeap::commitDescriptorTables(GpuCommandList* CommandList, commit_descriptor_table_pfn CommitTablesPFN)
{
	// mCompute the number of descriptors that need to be updated
	u32 NumDescriptorsToCommit = computeStaleDescriptorTableCount();
	if (NumDescriptorsToCommit == 0) return; // If no descriptors have been updated, don't do anything

	// Update the active descriptor heap based on the number of descriptors we want to commit
    updateCurrentHeap(CommandList, NumDescriptorsToCommit);
	
	// Scan for the descriptor tables that have been updated, and only copy the new descriptors.
	// If this is a new heap, all descriptors are updated since they have to be on the same heap.
	DWORD RootIndex;
	while (_BitScanForward64(&RootIndex, mStaleDescriptorTableBitmask))
	{
		UINT                         DescriptorCount      = mDescriptorTableCache[RootIndex].mNumDescriptors;
		D3D12_CPU_DESCRIPTOR_HANDLE* SrcDescriptorHandles = mDescriptorTableCache[RootIndex].mBaseDescriptor;

		// TODO(enlynn): Would it be possible to update all stale descriptors at once?
		D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptorRangeStarts[] = { mCurrentCpuHandle };
		UINT                        DestDescriptorRangeSizes[]  = { DescriptorCount   };

		// Copy the staged CPU visible descriptors to the GPU visible descriptor heap.
        mDevice->asHandle()->CopyDescriptors(1, DestDescriptorRangeStarts, DestDescriptorRangeSizes,
                                             DescriptorCount, SrcDescriptorHandles, nullptr, mHeapType);

		// Set the descriptors on the command list using the passed-in setter function.
		CommitTablesPFN(CommandList->asHandle(), RootIndex, mCurrentGpuHandle);

		// Offset current CPU and GPU descriptor handles.
		mCurrentCpuHandle.ptr = mCurrentCpuHandle.ptr + u64(DescriptorCount * mDescriptorStride);
		mCurrentGpuHandle.ptr = mCurrentGpuHandle.ptr + u64(DescriptorCount * mDescriptorStride);
		mNumFreeHandles      -= DescriptorCount;

		// Flip the stale bit so the descriptor table is not recopied until the descriptor is updated
		mStaleDescriptorTableBitmask = BitFlip(mStaleDescriptorTableBitmask, u64(RootIndex));
	}
}

void 
GpuDynamicDescriptorHeap::commitInlineDescriptors(
        GpuCommandList*            CommandList,
        D3D12_GPU_VIRTUAL_ADDRESS*   GPuDescriptorHandles,
        u32*                         DescriptorBitmask,
        commit_descriptor_inline_pfn CommitInlinePFN)
{
	u32 Bitmask = *DescriptorBitmask;
	if (Bitmask == 0) return; // Don't do anything if no descriptors have been updated
	
	DWORD RootIndex;
	while (_BitScanForward(&RootIndex, Bitmask))
	{
		CommitInlinePFN(CommandList->asHandle(), RootIndex, GPuDescriptorHandles[RootIndex]);
		// Flip the stale bit so the descriptor is not recopied until updated again
		Bitmask = BitFlip(Bitmask, (u32)RootIndex);
	}

	*DescriptorBitmask = Bitmask;
}

void 
GpuDynamicDescriptorHeap::commitStagedDescriptorsForDraw(GpuCommandList* CommandList)
{
    commitDescriptorTables(CommandList, &SetRootDescriptorGraphicsWrapper);
    commitInlineDescriptors(CommandList, mInlineCbv, &mStaleCbvBitmask, &SetGraphicsRootCBVWrapper);
    commitInlineDescriptors(CommandList, mInlineSrv, &mStaleSrvBitmask, &SetGraphicsRootSRVWrapper);
    commitInlineDescriptors(CommandList, mInlineUav, &mStaleUavBitmask, &SetGraphicsRootUAVWrapper);
}

void 
GpuDynamicDescriptorHeap::commitStagedDescriptorsForDispatch(GpuCommandList* CommandList)
{
    commitDescriptorTables(CommandList, &SetRootDescriptorGraphicsWrapper);
    commitInlineDescriptors(CommandList, mInlineCbv, &mStaleCbvBitmask, &SetComputeRootCBVWrapper);
    commitInlineDescriptors(CommandList, mInlineSrv, &mStaleSrvBitmask, &SetComputeRootSRVWrapper);
    commitInlineDescriptors(CommandList, mInlineUav, &mStaleUavBitmask, &SetComputeRootUAVWrapper);
}

D3D12_GPU_DESCRIPTOR_HANDLE 
GpuDynamicDescriptorHeap::copyDescriptor(GpuCommandList* CommandList, D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptorHandle)
{
	const u32 DescriptorsToUpdate = 1;

	// Update the active descriptor heap based on the number of descriptors we want to commit
    updateCurrentHeap(CommandList, DescriptorsToUpdate);

	D3D12_GPU_DESCRIPTOR_HANDLE Result = mCurrentGpuHandle;

	// Copy the Source CPU Descriptor Handle into the CPU Descriptor Cache
    mDevice->asHandle()->CopyDescriptorsSimple(1, mCurrentCpuHandle, CpuDescriptorHandle, mHeapType);

	// Offset current CPU and GPU descriptor handles.
	mCurrentCpuHandle.ptr = mCurrentCpuHandle.ptr + u64(DescriptorsToUpdate * mDescriptorStride);
	mCurrentGpuHandle.ptr = mCurrentGpuHandle.ptr + u64(DescriptorsToUpdate * mDescriptorStride);
	mNumFreeHandles -= DescriptorsToUpdate;

	return Result;
}

void
GpuDynamicDescriptorHeap::updateCurrentHeap(GpuCommandList* CommandList, u32 NumDescriptorsToCommit)
{
	if (!mCurrentHeap || mNumFreeHandles < NumDescriptorsToCommit)
	{
		// Not enough space to commit the requested descriptor Count. Acquire a new heap to fullfil the request.
		mCurrentHeap      = requestDescriptorHeap();
		mCurrentCpuHandle = mCurrentHeap->GetCPUDescriptorHandleForHeapStart();
		mCurrentGpuHandle = mCurrentHeap->GetGPUDescriptorHandleForHeapStart();
		mNumFreeHandles   = mDescriptorsPerHeap;

		// Update the current heap bound to the command list
        CommandList->setDescriptorHeap(mHeapType, mCurrentHeap);

		// When updating the descriptor heap on the command list, all descriptor tables must be (re)recopied to 
		// the new descriptor heap (not just the stale descriptor tables).
		mStaleDescriptorTableBitmask = mCachedDescriptorTableBitmask;
	}
}

ID3D12DescriptorHeap* 
GpuDynamicDescriptorHeap::requestDescriptorHeap()
{
	ID3D12DescriptorHeap* Result = 0;

	if (mNextAvailableHeap < mDescriptorHeapList.size())
	{
		Result = mDescriptorHeapList[mNextAvailableHeap];
		mNextAvailableHeap += 1;
	}
	else
	{
		Result = mDevice->createDescriptorHeap(mHeapType, mDescriptorsPerHeap, true);
		mDescriptorHeapList.push_back(Result);
	}

	return Result;
}

u32 
GpuDynamicDescriptorHeap::computeStaleDescriptorTableCount()
{
	u32 StaleDescriptorCount = 0;
	
	DWORD BitIndex;
	u64   Bitmask = mStaleDescriptorTableBitmask;

	// Count the number of bits set to 1
	while (_BitScanForward64(&BitIndex, Bitmask))
	{
		StaleDescriptorCount += mDescriptorTableCache[BitIndex].mNumDescriptors;
		Bitmask = BitFlip(Bitmask, (u64)BitIndex);
	}

	return StaleDescriptorCount;
}

// Stage inline descriptors
void 
GpuDynamicDescriptorHeap::stageInlineCbv(u32 RootIndex, D3D12_GPU_VIRTUAL_ADDRESS GpuDescriptorHandle)
{
	ASSERT(RootIndex < cMaxInlineDescriptors);
	mInlineCbv[RootIndex] = GpuDescriptorHandle;
	mStaleCbvBitmask      = BitSet(mStaleCbvBitmask, RootIndex);
}

void 
GpuDynamicDescriptorHeap::stageInlineSrv(u32 RootIndex, D3D12_GPU_VIRTUAL_ADDRESS GpuDescriptorHandle)
{
	ASSERT(RootIndex < cMaxInlineDescriptors);
	mInlineSrv[RootIndex] = GpuDescriptorHandle;
	mStaleSrvBitmask      = BitSet(mStaleSrvBitmask, RootIndex);
}

void 
GpuDynamicDescriptorHeap::stageInlineUav(u32 RootIndex, D3D12_GPU_VIRTUAL_ADDRESS GpuDescriptorHandle)
{
	ASSERT(RootIndex < cMaxInlineDescriptors);
	mInlineUav[RootIndex] = GpuDescriptorHandle;
	mStaleUavBitmask      = BitSet(mStaleUavBitmask, RootIndex);
}