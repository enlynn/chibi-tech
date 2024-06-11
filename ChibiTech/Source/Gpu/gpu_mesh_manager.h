#pragma once

// TODO: This file needs to be integrated into the new SparseStorage container.
#if 0
#include "gpu_resource.h"
#include "gpu_state.h"

#include <util/id.h>
#include <util/bit.h>
#include <util/array.h>
#include <math/math.h>

DEFINE_TYPE_ID(gpu_mesh_id);

enum class gpu_mesh_state : u8
{
    unallocated,       // the render mesh is unallocated and is a free slot
    allocated,         // an id for mesh has been allocated, but nothing has been done with the memory
    pending_delete,    // Resource is no longer valid, but still might be in use on the GPU.
    pending_upload,    // the render mesh has been initialized, but not queued yet for uploads
    uploading,         // the render mesh is uploading to the GPU, but not ready for use
    active,            // the render mesh is resident on the GPU and ready for rendering
};

// For now, there are no mesh Flags but I want to prototype a "Filter"
enum gpu_mesh_flags
{
    gpu_mesh_flag_none       = 0x00,
    gpu_mesh_flag_hidden     = BitMaskU8(1),  // Will prevent the mesh from being rendered
    gpu_mesh_flag_renderable = BitMaskU8(2),  // Marks the mesh as a mesh that can be rendered
};

struct gpu_draw_data
{
    gpu_resource            mVertexBufferResource = {};
    gpu_resource            mIndexBufferResource  = {};
    D3D12_INDEX_BUFFER_VIEW mIndexBufferView      = {};
    u32                     mElementCount         = 0;
};

struct gpu_mesh_data
{
    f32x44 Transform = {};
};

struct gpu_mesh_upload
{
    void* Vertices       = nullptr;
    u32   VerticesCount  = 0;
    u32   VerticesStride = 0;

    void* Indices        = nullptr;
    u64   IndicesCount   = 0;
    bool  IsU16          = false;    // If true, the Indices should be of type uint16_t, otherwise should be of type uint32_t
};

class gpu_mesh_manger
{
public:
    gpu_mesh_manger();
    void Deinit(struct gpu_frame_cache* FrameCache);

    gpu_mesh_id AcquireMesh();                                          // Acquire a new id for a mesh. Guarenteed to be unique.
    void ReleaseMesh(gpu_mesh_id Id, struct gpu_frame_cache* FrameCache);   // Release a mesh id. The data for the mesh is removed and unallocated.
                                                                        // Must pass FrameCache to release the state
    void SetMeshState(gpu_mesh_id Id, gpu_mesh_state State);
    gpu_mesh_state GetMeshState(gpu_mesh_id Id) const;

    void SetDrawData(gpu_mesh_id Id, gpu_mesh_upload UploadData);
    // NOTE(enlynn): I don't think I should get individual draw data from the storage - at least for now.
    // To render the meshes in the manager, call GetDrawList

    void SetMeshData(gpu_mesh_id Id, const gpu_mesh_data& MeshData);
    gpu_mesh_data GetMeshData(gpu_mesh_id Id) const;

    // Gets a compact list of draw data for the frame. List must be managed externally to the mesh manager.
    // If DrawList is null then the function will return the number
    //
    // NOTE(Enlynn): this isn't exactly how we want to do this in the long run but will work until indirect
    // is properly setup.
    void GetDrawData(darray<gpu_draw_data>& DrawList, gpu_mesh_flags FilterFlags);

private:
    static constexpr int cTotalAllowedMeshes = 10;
    using render_mesh_generator = id_generator<gpu_mesh_id, cTotalAllowedMeshes>;

    struct mesh_metadata
    { // Small header for mesh data
        gpu_mesh_state mState = gpu_mesh_state::unallocated;
        u8             mFlags = gpu_mesh_flag_none;
    };

    render_mesh_generator mIdGenerator = {};
    mesh_metadata         mMeshStates[cTotalAllowedMeshes];
    gpu_draw_data         mDrawData[cTotalAllowedMeshes];
    gpu_mesh_data         mMeshData[cTotalAllowedMeshes];    // This should be a StructuredBuffer instead, how to manage multiple frames of data?
};

#endif

//
// Render Mesh System Workflow
//
// render_mesh_id NewMesh = RenderMeshSystem->MeshAcquire();
//
// FrameCache->AddMeshUpload(NewMesh, GpuData);
// RenderMeshSystem->SetMeshState(NewMesh, render_mesh_state::pending_uploaded);
//
// //... later when the frame_cache is submitting uploads
// RenderMeshSystem->SetMeshState(NewMesh, render_mesh_state::uploading);
//
// //... later after the frame_cache is reset and we can guarantee the mesh is on the gpu
// render_upload_data Upload = ...;
//
// RenderMeshSystem->SetDrawData(Upload.MeshId, Upload.DrawData);
// RenderMeshSystem->SetMeshState(Upload.MeshId, render_mesh_state::active);
//
// // ... later when drawing
// u32 MeshFilterFlags = render_mesh_flags::none;
// u32 RenderMeshSystem->BuildDrawList(FrameCache->DrawList, MeshFilterFlags);
//
