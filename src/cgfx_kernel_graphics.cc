/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implements the test graphics kernels.
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////////
//   Preprocessor   //
////////////////////*/
/// @summary Somewhere in our include chain, someone includes <dxgiformat.h>.
/// Don't re-define the DXGI_FORMAT and D3D11 enumerations.
#define CG_DXGI_ENUMS_DEFINED 1

/*////////////////
//   Includes   //
////////////////*/
#include "cgfx.h"
#include "cgfx_w32_private.h"
#include "cgfx_kernel_graphics.h"

/*/////////////////
//   Constants   //
/////////////////*/

/*///////////////////
//   Local Types   //
///////////////////*/

/*///////////////
//   Globals   //
///////////////*/

/*///////////////////////
//   Local Functions   //
///////////////////////*/
/// @summary Implements the SET_VIEWPORT command for the TEST01 graphics pipeline.
/// @param ctx The CGFX context defining the command queue.
/// @param queue The CGFX graphics command queue.
/// @param cmdbuf The CGFX command buffer being submitted to the command queue.
/// @param pipeline The CGFX pipeline state being updated.
/// @param cmd The graphics pipeline dispatch command data.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_COMPILE_FAILED, CG_BAD_GLCONTEXT, CG_OUT_OF_MEMORY, CG_ERROR or another result code.
internal_function int
cgExecuteGraphicsTest01SetViewport
(
    CG_CONTEXT             *ctx, 
    CG_QUEUE               *queue, 
    CG_GRAPHICS_PIPELINE   *pipeline, 
    cg_pipeline_cmd_data_t *bdp
)
{
    int                         res =  CG_SUCCESS;
    CG_GFX_TEST01_SET_VIEWPORT *ddp = (CG_GFX_TEST01_SET_VIEWPORT*) bdp->ArgsData;
    // TODO(rlk): next problem - where to store custom pipeline state?
    return res;
}

/// @summary Implements the SET_PROJECTION command for the TEST01 graphics pipeline.
/// @param ctx The CGFX context defining the command queue.
/// @param queue The CGFX graphics command queue.
/// @param cmdbuf The CGFX command buffer being submitted to the command queue.
/// @param pipeline The CGFX pipeline state being updated.
/// @param cmd The graphics pipeline dispatch command data.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_COMPILE_FAILED, CG_BAD_GLCONTEXT, CG_OUT_OF_MEMORY, CG_ERROR or another result code.
internal_function int
cgExecuteGraphicsTest01SetProjection
(
    CG_CONTEXT             *ctx, 
    CG_QUEUE               *queue, 
    CG_GRAPHICS_PIPELINE   *pipeline, 
    cg_pipeline_cmd_data_t *bdp
)
{
    int                           res =  CG_SUCCESS;
    CG_GFX_TEST01_SET_PROJECTION *ddp = (CG_GFX_TEST01_SET_PROJECTION*) bdp->ArgsData;
    // TODO(rlk): next problem - where to store custom pipeline state?
    return res;
}

/// @summary Implements the DRAW_TRIANGLES command for the TEST01 graphics pipeline.
/// @param ctx The CGFX context defining the command queue.
/// @param queue The CGFX graphics command queue.
/// @param cmdbuf The CGFX command buffer being submitted to the command queue.
/// @param pipeline The CGFX pipeline state being updated.
/// @param cmd The graphics pipeline dispatch command data.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_COMPILE_FAILED, CG_BAD_GLCONTEXT, CG_OUT_OF_MEMORY, CG_ERROR or another result code.
internal_function int
cgExecuteGraphicsTest01DrawTriangles
(
    CG_CONTEXT             *ctx, 
    CG_QUEUE               *queue, 
    CG_GRAPHICS_PIPELINE   *pipeline, 
    cg_pipeline_cmd_data_t *bdp
)
{
    int                           res =  CG_SUCCESS;
    CG_GFX_TEST01_DRAW_TRIANGLES *ddp = (CG_GFX_TEST01_DRAW_TRIANGLES*) bdp->ArgsData;
    // TODO(rlk): next problem - where to store custom pipeline state?
    return res;
}

/// @summary Primary command dispatch function for the TEST01 graphics pipeline.
/// @param ctx The CGFX context defining the command queue.
/// @param queue The CGFX graphics command queue.
/// @param cmdbuf The CGFX command buffer being submitted to the command queue.
/// @param pipeline The CGFX pipeline object being executed.
/// @param cmd The compute pipeline dispatch command data.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_COMPILE_FAILED, CG_BAD_CLCONTEXT, CG_OUT_OF_MEMORY, CG_ERROR or another result code.
internal_function int
cgExecuteGraphicsPipelineTest01
(
    CG_CONTEXT    *ctx, 
    CG_QUEUE      *queue, 
    CG_CMD_BUFFER *cmdbuf,
    CG_PIPELINE   *pipeline, 
    cg_command_t  *cmd
)
{
    int                     res =  CG_SUCCESS;
    cg_pipeline_cmd_data_t *bdp = (cg_pipeline_cmd_data_t*) cmd->Data;
    CG_GRAPHICS_PIPELINE    *gp = &pipeline->Graphics;
    switch (bdp->PipelineCmd)
    {
    case CG_GRAPHICS_TEST01_CMD_SET_VIEWPORT:
        res = cgExecuteGraphicsTest01SetViewport(ctx, queue, gp, bdp);
        break;
    case CG_GRAPHICS_TEST01_CMD_SET_PROJECTION:
        res = cgExecuteGraphicsTest01SetProjection(ctx, queue, gp, bdp);
        break;
    case CG_GRAPHICS_TEST01_CMD_DRAW_TRIANGLES:
        res = cgExecuteGraphicsTest01DrawTriangles(ctx, queue, gp, bdp);
        break;
    default:
        res = CG_COMMAND_NOT_IMPLEMENTED;
        break;
    }
    UNREFERENCED_PARAMETER(cmdbuf);
    return res;
}

/*////////////////////////
//   Public Functions   //
////////////////////////*/
/// @summary Compiles the kernels, state and programs to create the TEST01 graphics pipeline.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param exec_group The CGFX execution group managing the set of devices the pipeline can execute on.
/// @param result On return, set to CG_SUCCESS or another result code.
/// @return The handle of the graphics pipeline object, or CG_INVALID_HANDLE.
library_function cg_handle_t
cgCreateGraphicsPipelineTest01
(
    uintptr_t   context, 
    cg_handle_t exec_group, 
    int        &result
)
{
    cgSetGraphicsPipelineCallback(CG_GRAPHICS_PIPELINE_TEST01, cgExecuteGraphicsPipelineTest01);
    return CG_INVALID_HANDLE;
}

/// @summary Enqueues a SET_VIEWPORT command for a TEST01 graphics pipeline.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param cmd_buffer The handle of the command buffer to write to.
/// @param pipeline The handle of the TEST01 graphics pipeline state, returned by cgCreateGraphicsPipelineTest01.
/// @param x The x-coordinate of the upper-left corner of the viewport.
/// @param y The y-coordinate of the upper-left corner of the viewport.
/// @param width The width of the viewport, in pixels.
/// @param height The height of the viewport, in pixels.
/// @param done_event The handle of the event to signal when the command has been executed, or CG_INVALID_HANDLE.
/// @param wait_event The handle of the event to wait on before executing the command, or CG_INVALID_HANDLE.
library_function int 
cgGraphicsTest01SetViewport
(
    uintptr_t   context,
    cg_handle_t cmd_buffer,
    cg_handle_t pipeline,
    int         x,
    int         y,
    int         width,
    int         height,
    cg_handle_t done_event,
    cg_handle_t wait_event
)
{
    int           result   = CG_SUCCESS;
    cg_command_t *cmd      = NULL;
    size_t const  cmd_size = sizeof(cg_pipeline_cmd_base_t) + sizeof(CG_GFX_TEST01_SET_VIEWPORT);
    if ((result = cgCommandBufferMapAppend(context, cmd_buffer, cmd_size, &cmd)) != CG_SUCCESS)
        return result;

    cg_pipeline_cmd_data_t        *bdp = (cg_pipeline_cmd_data_t    *) cmd->Data;
    CG_GFX_TEST01_SET_VIEWPORT    *ddp = (CG_GFX_TEST01_SET_VIEWPORT*) bdp->ArgsData;
    cmd->CommandId         = CG_COMMAND_PIPELINE_DISPATCH;
    cmd->DataSize          = uint16_t(cmd_size);
    bdp->PipelineId        = CG_GRAPHICS_PIPELINE_TEST01;
    bdp->PipelineCmd       = CG_GRAPHICS_TEST01_CMD_SET_VIEWPORT;
    bdp->ArgsDataSize      = sizeof(CG_GFX_TEST01_SET_VIEWPORT);
    bdp->ReservedU16       = 0; // unused
    bdp->WaitEvent         = wait_event;
    bdp->CompleteEvent     = done_event;
    bdp->Pipeline          = pipeline;
    ddp->X                 = x;
    ddp->Y                 = y;
    ddp->Width             = width;
    ddp->Height            = height;
    return cgCommandBufferUnmapAppend(context, cmd_buffer, cmd_size);
}

/// @summary Enqueues a SET_PROJECTION command for a TEST01 graphics pipeline.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param cmd_buffer The handle of the command buffer to write to.
/// @param pipeline The handle of the TEST01 graphics pipeline state, returned by cgCreateGraphicsPipelineTest01.
/// @param mvp The 4x4 combined model-view-projection matrix.
/// @param done_event The handle of the event to signal when the command has been executed, or CG_INVALID_HANDLE.
/// @param wait_event The handle of the event to wait on before executing the command, or CG_INVALID_HANDLE.
library_function int 
cgGraphicsTest01SetProjection
(
    uintptr_t   context,
    cg_handle_t cmd_buffer,
    cg_handle_t pipeline,
    float       mvp[16], 
    cg_handle_t done_event,
    cg_handle_t wait_event
)
{
    int           result   = CG_SUCCESS;
    cg_command_t *cmd      = NULL;
    size_t const  cmd_size = sizeof(cg_pipeline_cmd_base_t) + sizeof(CG_GFX_TEST01_SET_PROJECTION);
    if ((result = cgCommandBufferMapAppend(context, cmd_buffer, cmd_size, &cmd)) != CG_SUCCESS)
        return result;

    cg_pipeline_cmd_data_t        *bdp = (cg_pipeline_cmd_data_t      *)   cmd->Data;
    CG_GFX_TEST01_SET_PROJECTION  *ddp = (CG_GFX_TEST01_SET_PROJECTION*)   bdp->ArgsData;
    cmd->CommandId         = CG_COMMAND_PIPELINE_DISPATCH;
    cmd->DataSize          = uint16_t(cmd_size);
    bdp->PipelineId        = CG_GRAPHICS_PIPELINE_TEST01;
    bdp->PipelineCmd       = CG_GRAPHICS_TEST01_CMD_SET_PROJECTION;
    bdp->ArgsDataSize      = sizeof(CG_GFX_TEST01_SET_PROJECTION);
    bdp->ReservedU16       = 0; // unused
    bdp->WaitEvent         = wait_event;
    bdp->CompleteEvent     = done_event;
    bdp->Pipeline          = pipeline;
    memcpy(ddp->Matrix, mvp, sizeof(float) * 16);
    return cgCommandBufferUnmapAppend(context, cmd_buffer, cmd_size);
}

/// @summary Enqueues a DRAW_TRIANGLES command for a TEST01 graphics pipeline.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param cmd_buffer The handle of the command buffer to write to.
/// @param pipeline The handle of the TEST01 graphics pipeline state, returned by cgCreateGraphicsPipelineTest01.
/// @param vertex_buffer The handle of the data buffer containing the vertex data.
/// @param index_buffer The handle of the data buffer containing the index data.
/// @param done_event The handle of the event to signal when the command has been executed, or CG_INVALID_HANDLE.
/// @param wait_event The handle of the event to wait on before executing the command, or CG_INVALID_HANDLE.
library_function int 
cgGraphicsTest01DrawTriangles
(
    uintptr_t   context,
    cg_handle_t cmd_buffer,
    cg_handle_t pipeline,
    cg_handle_t vertex_buffer, 
    cg_handle_t index_buffer,
    cg_handle_t done_event,
    cg_handle_t wait_event
)
{
    int           result   = CG_SUCCESS;
    cg_command_t *cmd      = NULL;
    size_t const  cmd_size = sizeof(cg_pipeline_cmd_base_t) + sizeof(CG_GFX_TEST01_DRAW_TRIANGLES);
    if ((result = cgCommandBufferMapAppend(context, cmd_buffer, cmd_size, &cmd)) != CG_SUCCESS)
        return result;

    cg_pipeline_cmd_data_t        *bdp = (cg_pipeline_cmd_data_t      *)   cmd->Data;
    CG_GFX_TEST01_DRAW_TRIANGLES  *ddp = (CG_GFX_TEST01_DRAW_TRIANGLES*)   bdp->ArgsData;
    cmd->CommandId         = CG_COMMAND_PIPELINE_DISPATCH;
    cmd->DataSize          = uint16_t(cmd_size);
    bdp->PipelineId        = CG_GRAPHICS_PIPELINE_TEST01;
    bdp->PipelineCmd       = CG_GRAPHICS_TEST01_CMD_DRAW_TRIANGLES;
    bdp->ArgsDataSize      = sizeof(CG_GFX_TEST01_DRAW_TRIANGLES);
    bdp->ReservedU16       = 0; // unused
    bdp->WaitEvent         = wait_event;
    bdp->CompleteEvent     = done_event;
    bdp->Pipeline          = pipeline;
    ddp->VertexData        = vertex_buffer;
    ddp->IndexData         = index_buffer;
    return cgCommandBufferUnmapAppend(context, cmd_buffer, cmd_size);
}
