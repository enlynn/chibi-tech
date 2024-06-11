#pragma once

#include <types.h>

#include "D3D12Common.h"

#include "GpuResource.h"
#include "GpuDescriptorAllocator.h"

class GpuCommandList;

//
// Some thoughts:
// - One Buffer to Rule them All
// --- The behavior between them is *mostly* the same.
// - It is not the buffer's responsibility to upload or destroy itself
// --- This is the responsibility of a higher level structure (i.e. mesh_manager)
// 

enum class GpuBufferType : u8
{
    Unknown,
    Vertex,       // TODO: Unsupported
    Index,
    Structured,
    ByteAddress,
};

struct GpuIndexBufferInfo
{
    bool          mIsU16        = false;   // If false, then assumed to be U32
    u32           mIndexCount   = 0;
    void*         mIndices      = nullptr;
    GpuResource*  mBufferHandle = nullptr;
};

struct GpuByteAddressBufferInfo
{
    //bool                mCpuVisible         = false;   // If True, then can write to the buffer without needing an intermediate buffer
    bool                mCreateResourceView = false;
    u32                 mStride             = 1;
    u32                 mCount              = 1;       // FrameSize = mStride * mCount
    u32                 mBufferFrames       = 1;       // TotalSize = FrameSize * mBufferFrames
    void*               mData               = nullptr;
    //GpuResource*       mBufferHandle       = nullptr;
};

struct GpuStructuredBufferInfo
{
    u64  mCount        = 0;     // Number of Elements in the buffer
    u64  mStride       = 0;     // Per-Element Size
    u8   mFrames       = 0;     // How many buffered frames this buffer will have. Buffer Size = mFrames * (mCount * mStride);
    //bool mIsCpuVisible = false; // If True, can bind and write to the buffer on the CPU
    // NOTE(enlynn): for now, let's assume CPU-visible.
};

class GpuBuffer
{
public:
    GpuBuffer() = default;

    // Helper functions to make specific buffer types.
    //static GpuBuffer MakeVertexBuffer();
    static GpuBuffer createIndexBuffer(struct GpuFrameCache *FrameCache, GpuIndexBufferInfo& Info);
    //static GpuBuffer MakeStructuredBuffer();
    static GpuBuffer createByteAdressBuffer(struct GpuFrameCache *FrameCache, GpuByteAddressBufferInfo &Info);

    static GpuBuffer createStructuredBuffer(GpuFrameCache* FrameCache, GpuStructuredBufferInfo& Info);

    void map(u64 Frame);
    void unmap();
    void* getMappedData()       const { return mFrameData;                      }
    u64   getMappedDataOffset() const { return mMappedFrame * mStride * mCount; }

    // Convenience functions for accessing the buffer views
    D3D12_INDEX_BUFFER_VIEW  getIndexBufferView()       const { return mViews.mIndexView;    }
    D3D12_VERTEX_BUFFER_VIEW getVertexBufferView()      const { return mViews.mVertexView;   }
    CpuDescriptor           getResourceView()          const { return mViews.mResourceView; }
    CpuDescriptor           getConstantBufferView()    const { return getResourceView();    }
    CpuDescriptor           getStructuredBufferView()  const { return getResourceView();    }
    CpuDescriptor           GetByteAddressBufferView() const { return getResourceView();    }

    u32                       getIndexCount()  const { return mCount;             }
    D3D12_GPU_VIRTUAL_ADDRESS getGpuAddress()  const { mResource.getGpuAddress(); }
    GpuResource*              getGpuResource()       { return &mResource;         }

private: 
    static GpuResource copyBuffer(
            struct GpuFrameCache *FrameCache,
            void*                   BufferData,
            u64                     BufferSize,
            D3D12_RESOURCE_FLAGS    Flags              = D3D12_RESOURCE_FLAG_NONE,
            D3D12_RESOURCE_STATES   InitialBufferState = D3D12_RESOURCE_STATE_COMMON);

    GpuBufferType mType         = GpuBufferType::Unknown;
    GpuResource    mResource     = {};

    u32             mStride       = 0;
    u32             mCount        = 0;
    u32             mBufferFrames = 0;

    bool            mCpuVisible   = false;
    bool            mIsBound      = false;

    // Mapped data for a frame
    u64             mMappedFrame   = 0;
    void*           mMappedData    = nullptr;
    void*           mFrameData     = nullptr;
    // TODO: Intermediary buffer for non-cpu visible buffers

    union
    {
        D3D12_INDEX_BUFFER_VIEW  mIndexView = {};    // Index Buffers
        D3D12_VERTEX_BUFFER_VIEW mVertexView;        // mVertex Buffers
        CpuDescriptor           mResourceView;      // Byte Address, Structured, and Constant Buffers
    } mViews;
};
