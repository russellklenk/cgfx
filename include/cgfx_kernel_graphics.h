/*/////////////////////////////////////////////////////////////////////////////
/// @summary Define the public constants, data types and functions for the 
/// publicly available graphics pipelines.
///////////////////////////////////////////////////////////////////////////80*/

#ifndef LIB_CGFX_KERNEL_GRAPHICS_H
#define LIB_CGFX_KERNEL_GRAPHICS_H

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
/// @summary Define the arguments for the Test01 graphics kernel. This kernel draws a triangle 
struct cg_graphics_pipeline_test01_t
{
};

/*/////////////////
//   Functions   //
/////////////////*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

cg_handle_t
cgCreateGraphicsPipelineTest01        /// Compile the kernel(s) and generate the pipeline object for Test01.
(
    uintptr_t   context,              /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t exec_group,           /// The handle of the execution group used to execute the pipeline.
    int        &result                /// On return, set to CG_SUCCESS or another value.
);

int
cgEnqueueComputeDispatchTest01        /// Enqueue a dispatch command for compute kernel Test01.
(
    uintptr_t   context,              /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t cmd_buffer,           /// The handle of the command buffer to write to.
    cg_handle_t pipeline,             /// The handle of the pipeline to execute, returned by cgCreateComputePipelineTest01.
    cg_handle_t out_buffer,           /// The handle of the buffer to write to.
    cg_handle_t done_event,           /// The handle of the event to signal when the pipeline has finished executing.
    cg_handle_t wait_event            /// The handle of the event to wait on before executing the pipeline.
);

// problem: we need to insert pipeline-specific commands into the generic command stream.
// for each pipeline, we need to have some function pointers.
// the existing dispatch commands are basically flush commands.
// pipelines may define their own custom commands.
// aside from the transfer and synchronization commands, all commands are custom pipeline commands.

#ifdef __cplusplus
};     /* extern "C"  */
#endif /* __cplusplus */

#undef  CGFX_GRAPHICS_KERNELS_DEFINED
#define CGFX_GRAPHICS_KERNELS_DEFINED
#endif /* !defined(LIB_CGFX_KERNEL_GRAPHICS_H) */
