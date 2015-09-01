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
/// @summary A structure used to read or write vertex data in the TEST01 pipeline vertex buffer.
struct CG_GFX_TEST01_VERTEX
{
    float            Position[3];               /// The X,Y,Z position value.
    uint32_t         RGBA;                      /// The RGBA vertex color value.
};

/*/////////////////
//   Functions   //
/////////////////*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

cg_handle_t
cgCreateGraphicsPipelineTest01                  /// Compile the kernel(s) and generate the pipeline object for TEST01.
(
    uintptr_t   context,                        /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t exec_group,                     /// The handle of the execution group used to execute the pipeline.
    int        &result                          /// On return, set to CG_SUCCESS or another value.
);

int
cgGraphicsTest01SetViewport                     /// Enqueue a viewport change command for graphics pipeline Test01.
(
    uintptr_t   context,                        /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t cmd_buffer,                     /// The handle of the command buffer to write to.
    cg_handle_t pipeline,                       /// The handle of the TEST01 graphics pipeline state, returned by cgCreateGraphicsPipelineTest01.
    int         x,                              /// The x-coordinate of the upper-left corner of the viewport.
    int         y,                              /// The y-coordinate of the upper-left corner of the viewport.
    int         width,                          /// The width of the viewport, in pixels.
    int         height,                         /// The height of the viewport, in pixels.
    cg_handle_t done_event,                     /// The handle of the event to signal when the command has been executed, or CG_INVALID_HANDLE.
    cg_handle_t wait_event                      /// The handle of the event to wait on before executing the command, or CG_INVALID_HANDLE.
);

int
cgGraphicsTest01SetProjection                   /// Enqueue a model-view-projection matrix change for graphics pipeline Test01.
(
    uintptr_t   context,                        /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t cmd_buffer,                     /// The handle of the command buffer to write to.
    cg_handle_t pipeline,                       /// The handle of the TEST01 graphics pipeline state, returned by cgCreateGraphicsPipelineTest01.
    float       mvp[16],                        /// The combined 4x4 model-view-projection matrix to apply.
    cg_handle_t done_event,                     /// The handle of the event to signal when the command has been executed, or CG_INVALID_HANDLE.
    cg_handle_t wait_event                      /// The handle of the event to wait on before executing the command, or CG_INVALID_HANDLE.
);

int
cgGraphicsTest01DrawTriangles                   /// Enqueue an explicit draw command for graphics pipeline Test01.
(
    uintptr_t   context,                        /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t cmd_buffer,                     /// The handle of the command buffer to write to.
    cg_handle_t pipeline,                       /// The handle of the TEST01 graphics pipeline state, returned by cgCreateGraphicsPipelineTest01.
    cg_handle_t data_source,                    /// The handle of the vertex data source buffers.
    uint32_t    min_index,                      /// The smallest index value read from the index buffer.
    uint32_t    max_index,                      /// The largest index value read from the index buffer.
    int32_t     base_vertex,                    /// The offset added to each index value read from the index buffer.
    size_t      num_triangles,                  /// The number of triangles to draw.
    cg_handle_t done_event,                     /// The handle of the event to signal when the command has been executed, or CG_INVALID_HANDLE.
    cg_handle_t wait_event                      /// The handle of the event to wait on before executing the command, or CG_INVALID_HANDLE.
);

#ifdef __cplusplus
};     /* extern "C"  */
#endif /* __cplusplus */

#undef  CGFX_GRAPHICS_KERNELS_DEFINED
#define CGFX_GRAPHICS_KERNELS_DEFINED
#endif /* !defined(LIB_CGFX_KERNEL_GRAPHICS_H) */
