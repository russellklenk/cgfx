/*/////////////////////////////////////////////////////////////////////////////
/// @summary Define the public constants, data types and functions for the 
/// publicly available compute pipelines.
///////////////////////////////////////////////////////////////////////////80*/

#ifndef LIB_CGFX_KERNEL_COMPUTE_H
#define LIB_CGFX_KERNEL_COMPUTE_H

/*////////////////
//   Includes   //
////////////////*/
#include "cgfx.h"

/*////////////////////
//   Preprocessor   //
////////////////////*/

/*////////////////////////////
//   Forward Declarations   //
////////////////////////////*/

/*/////////////////
//   Constants   //
/////////////////*/

/*////////////////////////////
//  Function Pointer Types  //
////////////////////////////*/

/*//////////////////
//   Data Types   //
//////////////////*/
/// @summary Define the TEST01 compute pipeline command identifiers.
enum cg_compute_pipeline_test01_command_id_e : uint16_t
{
    CG_COMPUTE_TEST01_CMD_DISPATCH     = 0,    /// Invoke the TEST01 kernel.
};

/// @summary Define the arguments for the Test01 compute kernel. This kernel simply writes the string 'Hello!\0' to an output buffer.
struct cg_compute_pipeline_test01_dispatch_t
{
    cg_handle_t OutputBuffer;         /// The handle of the buffer to write to. The buffer must be at least 7 bytes.
};

/*/////////////////
//   Functions   //
/////////////////*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

cg_handle_t
cgCreateComputePipelineTest01         /// Compile the kernel(s) and generate the pipeline object for Test01.
(
    uintptr_t   context,              /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t exec_group,           /// The handle of the execution group used to execute the pipeline.
    int        &result                /// On return, set to CG_SUCCESS or another value.
);

int
cgComputeDispatchTest01               /// Enqueue a dispatch command for compute kernel Test01.
(
    uintptr_t   context,              /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t cmd_buffer,           /// The handle of the command buffer to write to.
    cg_handle_t pipeline,             /// The handle of the pipeline to execute, returned by cgCreateComputePipelineTest01.
    cg_handle_t out_buffer,           /// The handle of the buffer to write to.
    cg_handle_t done_event,           /// The handle of the event to signal when the pipeline has finished executing.
    cg_handle_t wait_event            /// The handle of the event to wait on before executing the pipeline.
);

#ifdef __cplusplus
};     /* extern "C"  */
#endif /* __cplusplus */

#undef  CGFX_COMPUTE_KERNELS_DEFINED
#define CGFX_COMPUTE_KERNELS_DEFINED
#endif /* !defined(LIB_CGFX_KERNEL_COMPUTE_H) */
