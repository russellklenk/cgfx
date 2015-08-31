/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implements the test compute kernels.
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
/// @summary Implements the setup, teardown and command submission code for the TEST01 compute pipeline.
/// @param ctx The CGFX context defining the command queue.
/// @param queue The CGFX compute command queue.
/// @param cmdbuf The CGFX command buffer being submitted to the command queue.
/// @param pipeline The CGFX pipeline object being executed.
/// @param cmd The compute pipeline dispatch command data.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_COMPILE_FAILED, CG_BAD_CLCONTEXT, CG_OUT_OF_MEMORY, CG_ERROR or another result code.
internal_function int
cgExecuteComputeTest01Dispatch
(
    CG_CONTEXT             *ctx, 
    CG_QUEUE               *queue, 
    CG_COMPUTE_PIPELINE    *pipeline, 
    cg_pipeline_cmd_data_t *bdp
)
{
    cg_compute_pipeline_test01_dispatch_t *ddp = (cg_compute_pipeline_test01_dispatch_t*) bdp->ArgsData;
    CG_BUFFER                   *output =  cgObjectTableGet(&ctx->BufferTable, ddp->OutputBuffer);
    int                          result =  CG_SUCCESS;
    size_t                     nmemrefs =  0;
    cl_uint                    nwaitevt =  0;
    cl_event                    acquire =  NULL;
    cl_mem                      memrefs[2];
    cgMemRefListAddBuffer(output, memrefs, nmemrefs, 1, false);
    if ((result = cgAcquireMemoryObjects(ctx, queue, memrefs, nmemrefs, bdp->WaitEvent, &acquire, nwaitevt, 1)) == CL_SUCCESS)
    {
        cl_event cl_done = NULL;
        cl_int   cl_res  = CL_SUCCESS;
        cl_uint  dim     = 1;
        size_t   gsz[1]  ={1};
        size_t   lsz[1]  ={1};

        clSetKernelArg(pipeline->ComputeKernel, 0, sizeof(cl_mem),  &output->ComputeBuffer);
        if ((cl_res = clEnqueueNDRangeKernel(queue->CommandQueue , pipeline->ComputeKernel, dim, NULL, gsz, lsz, nwaitevt, CG_OPENCL_WAIT_LIST(nwaitevt, &acquire), &cl_done)) != CL_SUCCESS)
        {   // the kernel could not be enqueued.
            switch (cl_res)
            {
            case CL_INVALID_PROGRAM_EXECUTABLE   : result = CG_COMPILE_FAILED; break;
            case CL_INVALID_COMMAND_QUEUE        : result = CG_BAD_CLCONTEXT;  break;
            case CL_INVALID_KERNEL               : result = CG_INVALID_VALUE;  break;
            case CL_INVALID_CONTEXT              : result = CG_BAD_CLCONTEXT;  break;
            case CL_INVALID_KERNEL_ARGS          : result = CG_INVALID_VALUE;  break;
            case CL_INVALID_WORK_DIMENSION       : result = CG_INVALID_VALUE;  break;
            case CL_INVALID_WORK_GROUP_SIZE      : result = CG_INVALID_VALUE;  break;
            case CL_INVALID_WORK_ITEM_SIZE       : result = CG_INVALID_VALUE;  break;
            case CL_INVALID_GLOBAL_OFFSET        : result = CG_INVALID_VALUE;  break;
            case CL_INVALID_EVENT_WAIT_LIST      : result = CG_INVALID_VALUE;  break;
            case CL_OUT_OF_RESOURCES             : result = CG_OUT_OF_MEMORY;  break;
            case CL_MEM_OBJECT_ALLOCATION_FAILURE: result = CG_OUT_OF_MEMORY;  break;
            case CL_OUT_OF_HOST_MEMORY           : result = CG_OUT_OF_MEMORY;  break;
            default                              : result = CG_ERROR;          break;
            }
            cgReleaseMemoryObjects(ctx, queue, memrefs, nmemrefs, NULL, 0, CG_INVALID_HANDLE);
            return result;
        }
        return cgReleaseMemoryObjects(ctx, queue, memrefs, nmemrefs, &cl_done, 1, bdp->CompleteEvent);
    }
    else return result;
}

/// @summary Primary command dispatch function for the TEST01 compute pipeline.
/// @param ctx The CGFX context defining the command queue.
/// @param queue The CGFX compute command queue.
/// @param cmdbuf The CGFX command buffer being submitted to the command queue.
/// @param pipeline The CGFX pipeline object being executed.
/// @param cmd The compute pipeline dispatch command data.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_COMPILE_FAILED, CG_BAD_CLCONTEXT, CG_OUT_OF_MEMORY, CG_ERROR or another result code.
internal_function int
cgExecuteComputePipelineTest01
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
    CG_COMPUTE_PIPELINE     *cp = &pipeline->Compute;
    switch (bdp->PipelineCmd)
    {
    case CG_COMPUTE_TEST01_CMD_DISPATCH:
        res = cgExecuteComputeTest01Dispatch(ctx, queue, cp, bdp);
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
/// @summary Compiles the kernels, state and programs to create the TEST01 compute pipeline.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param exec_group The CGFX execution group managing the set of devices the pipeline can execute on.
/// @param result On return, set to CG_SUCCESS or another result code.
/// @return The handle of the compute pipeline object, or CG_INVALID_HANDLE.
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
    cg_handle_t pipeline = cgCreateComputePipeline(context, exec_group, &cp, NULL, NULL, result);
    if (pipeline == CG_INVALID_HANDLE)
    {
        cgDeleteObject(context, kernel);
        return CG_INVALID_HANDLE;
    }

    cgSetComputePipelineCallback(CG_COMPUTE_PIPELINE_TEST01, cgExecuteComputePipelineTest01);
    return pipeline;
}

/// @summary Enqueue a COMPUTE_DISPATCH command for the TEST01 pipeline in a command buffer.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param cmd_buffer The handle of the command buffer to update.
/// @param pipeline The handle of a TEST01 compute pipeline object.
/// @param out_buffer The handle of the output buffer object.
/// @param done_event The handle of the event to signal when the pipeline has finished executing, or CG_INVALID_HANDLE.
/// @param wait_event The handle of the event to wait on before executing the kernel, or CG_INVALID_HANDLE.
/// @return CG_SUCCESS or another result code.
library_function int
cgComputeDispatchTest01
(
    uintptr_t   context, 
    cg_handle_t cmd_buffer, 
    cg_handle_t pipeline,
    cg_handle_t out_buffer, 
    cg_handle_t done_event, 
    cg_handle_t wait_event
)
{
    int           result   = CG_SUCCESS;
    cg_command_t *cmd      = NULL;
    size_t const  cmd_size = sizeof(cg_pipeline_cmd_base_t) + sizeof(cg_compute_pipeline_test01_dispatch_t);
    if ((result = cgCommandBufferMapAppend(context, cmd_buffer, cmd_size, &cmd)) != CG_SUCCESS)
        return result;

    cg_pipeline_cmd_data_t                *bdp = (cg_pipeline_cmd_data_t               *) cmd->Data;
    cg_compute_pipeline_test01_dispatch_t *ddp = (cg_compute_pipeline_test01_dispatch_t*) bdp->ArgsData;
    cmd->CommandId         = CG_COMMAND_PIPELINE_DISPATCH;
    cmd->DataSize          = uint16_t(cmd_size);
    bdp->PipelineId        = CG_COMPUTE_PIPELINE_TEST01;
    bdp->PipelineCmd       = CG_COMPUTE_TEST01_CMD_DISPATCH;
    bdp->ArgsDataSize      = sizeof(cg_compute_pipeline_test01_dispatch_t);
    bdp->ReservedU16       = 0; // unused
    bdp->WaitEvent         = wait_event;
    bdp->CompleteEvent     = done_event;
    bdp->Pipeline          = pipeline;
    ddp->OutputBuffer      = out_buffer;
    return cgCommandBufferUnmapAppend(context, cmd_buffer, cmd_size);
}
