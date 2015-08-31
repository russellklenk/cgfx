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
/// @summary Define the arguments for the Test01 graphics kernel. This kernel draws an indexed triangle list.
/// Each vertex is 16 bytes and comprised of three FLOAT32 components specifying x,y,z position, plus an RGBA 
/// color value expressed as 4 UINT8 values (normalized.) Indices are 16-bit unsigned integer values.
struct cg_graphics_pipeline_test01_t
{
    cg_handle_t VertexData;           /// The handle of the buffer containing the vertex data.
    cg_handle_t IndexData;            /// The handle of the buffer containing the index data.
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
cgDrawVertexColorTriangleList         /// Enqueue an explicit draw command for graphics pipeline Test01.
(
    uintptr_t   context,              /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t cmd_buffer,           /// The handle of the command buffer to write to.
    cg_handle_t pipeline,             /// The handle of the pipeline to execute, returned by cgCreateGraphicsPipelineTest01.
    cg_handle_t vertex_buffer,        /// The handle of the buffer containing the vertex data.
    cg_handle_t index_buffer,         /// The handle of the buffer containing the index data.
    cg_handle_t done_event,           /// The handle of the event to signal when the pipeline has finished executing.
    cg_handle_t wait_event            /// The handle of the event to wait on before executing the pipeline.
);

#ifdef __cplusplus
};     /* extern "C"  */
#endif /* __cplusplus */

#undef  CGFX_GRAPHICS_KERNELS_DEFINED
#define CGFX_GRAPHICS_KERNELS_DEFINED
#endif /* !defined(LIB_CGFX_KERNEL_GRAPHICS_H) */
