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
/// @summary Define the TEST01 graphics pipeline command identifiers.
enum cg_graphics_pipeline_test01_command_id_e : uint16_t
{
    CG_GRAPHICS_TEST01_CMD_SET_VIEWPORT    = 0, /// Set the active viewport. Data is int[4].
    CG_GRAPHICS_TEST01_CMD_SET_PROJECTION  = 1, /// Set the active projection matrix. Data is float[16].
    CG_GRAPHICS_TEST01_CMD_DRAW_TRIANGLES  = 2, /// Submit geometry for rendering. Data is cg_handle_t[2].
};

/// @summary Define the arguments for the Test01 SET_VIEWPORT command.
struct CG_GFX_TEST01_SET_VIEWPORT
{
    int         X;                              /// The x-coordinate of the upper-left corner of the viewing region.
    int         Y;                              /// The y-coordinate of the upper-left corner of the viewing region.
    int         Width;                          /// The width of the viewing region, in pixels.
    int         Height;                         /// The height of the viewing region, in pixels.
};

/// @summary Define the arguments for the Test01 SET_PROJECTION command.
struct CG_GFX_TEST01_SET_PROJECTION
{
    float       Matrix[16];                     /// The combined model, view and projection matrix.
};

/// @summary Define the arguments for the Test01 DRAW_TRIANGLES command, which submits an indexed triangle list.
/// Each vertex is 16 bytes and comprised of three FLOAT32 components specifying x,y,z position, plus an RGBA 
/// color value expressed as 4 UINT8 values (normalized.) Indices are 16-bit unsigned integer values.
struct CG_GFX_TEST01_DRAW_TRIANGLES
{
    cg_handle_t VertexData;                     /// The handle of the buffer containing the vertex data.
    cg_handle_t IndexData;                      /// The handle of the buffer containing the index data.
};

/*/////////////////
//   Functions   //
/////////////////*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

cg_handle_t
cgCreateGraphicsPipelineTest01                  /// Compile the kernel(s) and generate the pipeline object for Test01.
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
    cg_handle_t pipeline,                       /// The handle of the Test01 graphics pipeline state, returned by cgCreateGraphicsPipelineTest01.
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
    cg_handle_t pipeline,                       /// The handle of the Test01 graphics pipeline state, returned by cgCreateGraphicsPipelineTest01.
    float       mvp[16],                        /// The combined 4x4 model-view-projection matrix to apply.
    cg_handle_t done_event,                     /// The handle of the event to signal when the command has been executed, or CG_INVALID_HANDLE.
    cg_handle_t wait_event                      /// The handle of the event to wait on before executing the command, or CG_INVALID_HANDLE.
);

int
cgGraphicsTest01DrawTriangles                   /// Enqueue an explicit draw command for graphics pipeline Test01.
(
    uintptr_t   context,                        /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t cmd_buffer,                     /// The handle of the command buffer to write to.
    cg_handle_t pipeline,                       /// The handle of the Test01 graphics pipeline state, returned by cgCreateGraphicsPipelineTest01.
    cg_handle_t vertex_buffer,                  /// The handle of the buffer containing the vertex data.
    cg_handle_t index_buffer,                   /// The handle of the buffer containing the index data.
    cg_handle_t done_event,                     /// The handle of the event to signal when the command has been executed, or CG_INVALID_HANDLE.
    cg_handle_t wait_event                      /// The handle of the event to wait on before executing the command, or CG_INVALID_HANDLE.
);

#ifdef __cplusplus
};     /* extern "C"  */
#endif /* __cplusplus */

#undef  CGFX_GRAPHICS_KERNELS_DEFINED
#define CGFX_GRAPHICS_KERNELS_DEFINED
#endif /* !defined(LIB_CGFX_KERNEL_GRAPHICS_H) */
