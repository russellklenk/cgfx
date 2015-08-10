/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implements the test compute kernels.
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////////
//   Preprocessor   //
////////////////////*/

/*////////////////
//   Includes   //
////////////////*/
#include "cgfx.h"
#include "cgfx_w32_private.h"
#include "cgfx_kernel_compute.h"

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
internal_function void
cgExecuteComputeDispatchTest01
(
    CG_CONTEXT   *ctx, 
    CG_QUEUE     *queue, 
    CG_PIPELINE  *pipe, 
    cg_command_t *cmd
)
{
    cl_event cl_evt = NULL;
    cg_compute_dispatch_cmd_data_t *bdp = (cg_compute_dispatch_cmd_data_t*) cmd->Data;
    cg_compute_pipeline_test01_t   *ddp = (cg_compute_pipeline_test01_t  *) bdp->ArgsData;
    CG_BUFFER *output = cgObjectTableGet(&ctx->BufferTable, ddp->OutputBuffer);
    clSetKernelArg(pipe->Compute.ComputeKernel, 0, sizeof(cl_mem), &output->ComputeBuffer);
    clEnqueueNDRangeKernel(queue->CommandQueue, pipe->Compute.ComputeKernel, cl_uint(bdp->WorkDimension), NULL, bdp->GlobalWorkSize, bdp->LocalWorkSize, 0, NULL, &cl_evt);
    if (bdp->DoneEvent != CG_INVALID_HANDLE)
    {
        CG_EVENT *ev = cgObjectTableGet(&ctx->EventTable, bdp->DoneEvent);
        ev->ComputeSync = cl_evt;
    }
}

/*////////////////////////
//   Public Functions   //
////////////////////////*/
library_function cg_handle_t
cgCreateComputePipelineTest01
(
    uintptr_t   context, 
    cg_handle_t exec_group, 
    int        &result
)
{
    static char const *kernel_code = 
        "__kernel void hello_world(__global char *str)\n"
        "{\n"
        "    str[0] = 'H';\n"
        "    str[1] = 'e';\n"
        "    str[2] = 'l';\n"
        "    str[3] = 'l';\n"
        "    str[4] = 'o';\n"
        "    str[5] = '!';\n"
        "    str[6] = '\\0';\n"
        "}\n";

    cg_kernel_code_t code;
    code.Code          = kernel_code;
    code.CodeSize      = strlen(kernel_code)+1;
    code.Type          = CG_KERNEL_TYPE_COMPUTE;
    code.Flags         = CG_KERNEL_FLAGS_SOURCE;
    cg_handle_t kernel = cgCreateKernel(context, exec_group, &code, result);
    if (kernel == CG_INVALID_HANDLE)
    {
        return CG_INVALID_HANDLE;
    }

    cg_compute_pipeline_t cp;
    cp.KernelName    = "hello_world";
    cp.KernelProgram = kernel;
    cg_handle_t pipeline = cgCreateComputePipeline(context, exec_group, &cp, result);
    if (pipeline == CG_INVALID_HANDLE)
    {
        cgDeleteObject(context, kernel);
        return CG_INVALID_HANDLE;
    }

    // ensure that the dispatch execution callback is registered.
    cgRegisterComputeDispatch(CG_COMPUTE_PIPELINE_TEST01, cgExecuteComputeDispatchTest01);
    return pipeline;
}

library_function int
cgEnqueueComputeDispatchTest01
(
    uintptr_t   context, 
    cg_handle_t cmd_buffer, 
    cg_handle_t pipeline,
    cg_handle_t out_buffer, 
    cg_handle_t done_event
)
{
    CG_CONTEXT *ctx    =(CG_CONTEXT*) context;
    int         result = CG_SUCCESS;

    cg_command_t *cmd      = NULL;
    size_t const  cmd_size = sizeof(cg_compute_dispatch_cmd_base_t) + sizeof(cg_compute_pipeline_test01_t);
    if ((result = cgCommandBufferMapAppend(context, cmd_buffer, cmd_size, &cmd)) != CG_SUCCESS)
        return result;

    // fill out the basic command header.
    cg_compute_dispatch_cmd_data_t *bdp = (cg_compute_dispatch_cmd_data_t*) cmd->Data;
    cg_compute_pipeline_test01_t   *ddp = (cg_compute_pipeline_test01_t  *) bdp->ArgsData;
    cmd->CommandId         = CG_COMMAND_COMPUTE_DISPATCH;
    cmd->DataSize          = cmd_size;
    bdp->PipelineId        = CG_COMPUTE_PIPELINE_TEST01;
    bdp->ArgsDataSize      = sizeof(cg_compute_pipeline_test01_t);
    bdp->Pipeline          = pipeline;
    bdp->DoneEvent         = done_event;
    bdp->WorkDimension     = 1;
    bdp->LocalWorkSize [0] = 1;
    bdp->GlobalWorkSize[0] = 1;
    ddp->OutputBuffer      = out_buffer;

    return cgCommandBufferUnmapAppend(context, cmd_buffer, cmd_size);
}
