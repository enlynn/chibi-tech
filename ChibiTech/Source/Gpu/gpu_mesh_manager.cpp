#if 0
#include "gpu_mesh_manager.h"
#include "gpu_state.h"
#include "renderer/dx12/gpu_mesh_manager.h"
#include "util/id.h"


gpu_mesh_manger::gpu_mesh_manger()
{
    mIdGenerator = render_mesh_generator();

    ForRange(u32, i, cTotalAllowedMeshes)
        mMeshStates[i] = {}; // Default mesh state is unallocated

    // Can leave the remaining data unitialized since their access
    // depends on the state within mMeshStates. Marking the state
    // as unallocated should be enough to block accessing unitialized data.

    // TODO(enlynn): Determine how to handle per-mesh data StructuredBuffer.
}

void gpu_mesh_manger::Deinit(gpu_frame_cache* FrameCache)
{
    ForRange(u32, i, cTotalAllowedMeshes)
    {
        // If the mesh is pending delete, then it has already been marked for
        // removal and might be tracked by another FrameCache. Better let the
        // original caller release the Resource.
        //
        // If the mesh is pending upload, then it has been marked in the FrameCache
        // to be uploaded. No Resource state should yet be initialized for this mesh.
        // The FrameCache will not upload the Resource when shutting down the renderer.
        if (mMeshStates[i].mState > gpu_mesh_state::pending_upload)
        {
            mMeshStates[i].mState = gpu_mesh_state::pending_delete;
            FrameCache->AddStaleResource(mDrawData[i].mVertexBufferResource);
            FrameCache->AddStaleResource(mDrawData[i].mIndexBufferResource);
        }
    }
}

gpu_mesh_id gpu_mesh_manger::AcquireMesh()
{
    gpu_mesh_id Result = mIdGenerator.AcquireId();
    assert(IsValid(Result) && "Have created too many Render Meshes!");
    return Result;
}

void gpu_mesh_manger::ReleaseMesh(gpu_mesh_id Id, gpu_frame_cache* FrameCache)
{
    if (mIdGenerator.IsIdValid(Id))
    {
        index_type Index = GetIndex(Id);

        gpu_mesh_state State = mMeshStates[Index].mState;
        if (State > gpu_mesh_state::pending_upload)
        {
            mMeshStates[Index].mState = gpu_mesh_state::pending_delete;
            mMeshStates[Index].mFlags = gpu_mesh_flag_none;
            // NOTE(enlynn): Currently, the FrameCache has no way of telling the
            // Mesh Manager the mesh was deleted, so won't be able to update
            // the state!

            FrameCache->AddStaleResource(mDrawData[Index].mVertexBufferResource);
            FrameCache->AddStaleResource(mDrawData[Index].mIndexBufferResource);
        }
    }
}

void gpu_mesh_manger::SetMeshState(gpu_mesh_id Id, gpu_mesh_state State)
{

}

gpu_mesh_state gpu_mesh_manger::GetMeshState(gpu_mesh_id Id) const
{
    return {};
}

void gpu_mesh_manger::SetDrawData(gpu_mesh_id Id, gpu_mesh_upload UploadData)
{

}

void gpu_mesh_manger::SetMeshData(gpu_mesh_id Id, const gpu_mesh_data& MeshData)
{

}

gpu_mesh_data gpu_mesh_manger::GetMeshData(gpu_mesh_id Id) const
{
    return {};
}

void gpu_mesh_manger::GetDrawData(std::vector<gpu_draw_data>& DrawList, gpu_mesh_flags FilterFlags)
{
}
#endif