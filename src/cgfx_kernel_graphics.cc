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

#undef    glewGetContext
#define   glewGetContext()    (&display->GLEW)

#undef    wglewGetContext
#define   wglewGetContext()   (&display->WGLEW)

/*/////////////////
//   Constants   //
/////////////////*/

/*///////////////////
//   Local Types   //
///////////////////*/
/// @summary Define the TEST01 graphics pipeline command identifiers.
enum cg_graphics_pipeline_test01_command_id_e : uint16_t
{
    CG_GRAPHICS_TEST01_CMD_SET_VIEWPORT    = 0, /// Set the active viewport. Data is int[4].
    CG_GRAPHICS_TEST01_CMD_SET_PROJECTION  = 1, /// Set the active projection matrix. Data is float[16].
    CG_GRAPHICS_TEST01_CMD_DRAW_TRIANGLES  = 2, /// Submit geometry for rendering. Data is cg_handle_t[2].
};

/// @summary Define the arguments for the TEST01 SET_VIEWPORT command.
struct CG_GFX_TEST01_SET_VIEWPORT
{
    int              X;                         /// The x-coordinate of the upper-left corner of the viewing region.
    int              Y;                         /// The y-coordinate of the upper-left corner of the viewing region.
    int              Width;                     /// The width of the viewing region, in pixels.
    int              Height;                    /// The height of the viewing region, in pixels.
};

/// @summary Define the arguments for the TEST01 SET_PROJECTION command.
struct CG_GFX_TEST01_SET_PROJECTION
{
    float            Matrix[16];                /// The combined model, view and projection matrix.
};

/// @summary Define the arguments for the TEST01 DRAW_TRIANGLES command, which submits an indexed triangle list.
/// Each vertex is 16 bytes and comprised of three FLOAT32 components specifying x,y,z position, plus an RGBA 
/// color value expressed as 4 UINT8 values (normalized.) Indices are 16-bit unsigned integer values.
struct CG_GFX_TEST01_DRAW_TRIANGLES
{
    cg_handle_t      VertexSource;              /// The handle of the vertex data source.
    uint32_t         MinIndex;                  /// The smallest index value appearing in the index data.
    uint32_t         MaxIndex;                  /// The largest index value appearing in the index data.
    int32_t          BaseVertex;                /// The offset value added to each index read from the index buffer.
    size_t           PrimitiveCount;            /// The number of triangles to draw.
};

/// @summary Define the internal state for the TEST01 graphics pipeline.
struct CG_GFX_TEST01_STATE
{
    int              Viewport[4];               /// The active viewport.
    float            MVP[16];                   /// The active projection matrix.
    CG_GLSL_UNIFORM *uMSS;                      /// Information about the projection matrix uniform.
};

/*///////////////
//   Globals   //
///////////////*/

/*///////////////////////
//   Local Functions   //
///////////////////////*/
/// @summary Implements the SET_VIEWPORT command for the TEST01 graphics pipeline.
/// @param ctx The CGFX context defining the command queue.
/// @param queue The CGFX graphics command queue.
/// @param pipeline The CGFX pipeline state being updated.
/// @param state The private pipeline state.
/// @param cmd The graphics pipeline dispatch command data.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_COMPILE_FAILED, CG_BAD_GLCONTEXT, CG_OUT_OF_MEMORY, CG_ERROR or another result code.
internal_function int
cgExecuteGraphicsTest01SetViewport
(
    CG_CONTEXT             *ctx, 
    CG_QUEUE               *queue, 
    CG_GRAPHICS_PIPELINE   *pipeline, 
    CG_GFX_TEST01_STATE    *state, 
    cg_pipeline_cmd_data_t *bdp
)
{   UNREFERENCED_PARAMETER(ctx);
    UNREFERENCED_PARAMETER(queue);
    UNREFERENCED_PARAMETER(pipeline);
    CG_GFX_TEST01_SET_VIEWPORT *ddp = (CG_GFX_TEST01_SET_VIEWPORT*) bdp->ArgsData;
    state->Viewport[0] = ddp->X;
    state->Viewport[1] = ddp->Y;
    state->Viewport[2] = ddp->Width;
    state->Viewport[3] = ddp->Height;
    return CG_SUCCESS;
}

/// @summary Implements the SET_PROJECTION command for the TEST01 graphics pipeline.
/// @param ctx The CGFX context defining the command queue.
/// @param queue The CGFX graphics command queue.
/// @param pipeline The CGFX pipeline state being updated.
/// @param state The private pipeline state.
/// @param cmd The graphics pipeline dispatch command data.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_COMPILE_FAILED, CG_BAD_GLCONTEXT, CG_OUT_OF_MEMORY, CG_ERROR or another result code.
internal_function int
cgExecuteGraphicsTest01SetProjection
(
    CG_CONTEXT             *ctx, 
    CG_QUEUE               *queue, 
    CG_GRAPHICS_PIPELINE   *pipeline, 
    CG_GFX_TEST01_STATE    *state, 
    cg_pipeline_cmd_data_t *bdp
)
{   UNREFERENCED_PARAMETER(ctx);
    UNREFERENCED_PARAMETER(queue);
    UNREFERENCED_PARAMETER(pipeline);
    CG_GFX_TEST01_SET_PROJECTION *ddp = (CG_GFX_TEST01_SET_PROJECTION*) bdp->ArgsData;
    memcpy(state->MVP, ddp->Matrix, sizeof(float) * 16);
    return CG_SUCCESS;
}

/// @summary Implements the DRAW_TRIANGLES command for the TEST01 graphics pipeline.
/// @param ctx The CGFX context defining the command queue.
/// @param queue The CGFX graphics command queue.
/// @param pipeline The CGFX pipeline state being updated.
/// @param state The private pipeline state.
/// @param cmd The graphics pipeline dispatch command data.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_COMPILE_FAILED, CG_BAD_GLCONTEXT, CG_OUT_OF_MEMORY, CG_ERROR or another result code.
internal_function int
cgExecuteGraphicsTest01DrawTriangles
(
    CG_CONTEXT             *ctx, 
    CG_QUEUE               *queue, 
    CG_GRAPHICS_PIPELINE   *pipeline, 
    CG_GFX_TEST01_STATE    *state, 
    cg_pipeline_cmd_data_t *bdp
)
{   UNREFERENCED_PARAMETER(queue);
    CG_GFX_TEST01_DRAW_TRIANGLES *ddp = (CG_GFX_TEST01_DRAW_TRIANGLES*) bdp->ArgsData;
    CG_VERTEX_DATA_SOURCE        *vds =  cgObjectTableGet(&ctx->VertexSourceTable, ddp->VertexSource);

    if (vds == NULL)
    {
        return CG_INVALID_VALUE;
    }

    // bind the vertex array and shader program objects.
    CG_DISPLAY *display = pipeline->AttachedDisplay;
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glUseProgram(pipeline->ShaderProgram.Program);
    glBindVertexArray(vds->VertexArray);
    glViewport(
        GLint  (state->Viewport[0]), GLint  (state->Viewport[1]), 
        GLsizei(state->Viewport[2]), GLsizei(state->Viewport[3]));

    // update uniform values.
    glUniformMatrix4fv(state->uMSS->Location, 1, GL_FALSE, state->MVP);

    // submit the draw call to the GPU command queue.
    glDrawRangeElementsBaseVertex(GL_TRIANGLES, ddp->MinIndex, ddp->MaxIndex, ddp->PrimitiveCount * 3, GL_UNSIGNED_SHORT, (GLvoid*) 0, ddp->BaseVertex);
    return CG_SUCCESS;
}

/// @summary Primary command dispatch function for the TEST01 graphics pipeline.
/// @param ctx The CGFX context defining the command queue.
/// @param queue The CGFX graphics command queue.
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
{   UNREFERENCED_PARAMETER(cmdbuf);
    int                     res =  CG_SUCCESS;
    cg_pipeline_cmd_data_t *bdp = (cg_pipeline_cmd_data_t*) cmd->Data;
    CG_GFX_TEST01_STATE     *ps = (CG_GFX_TEST01_STATE   *) pipeline->PrivateState; 
    CG_GRAPHICS_PIPELINE    *gp = &pipeline->Graphics;
    switch (bdp->PipelineCmd)
    {
    case CG_GRAPHICS_TEST01_CMD_SET_VIEWPORT:
        res = cgExecuteGraphicsTest01SetViewport(ctx, queue, gp, ps, bdp);
        break;
    case CG_GRAPHICS_TEST01_CMD_SET_PROJECTION:
        res = cgExecuteGraphicsTest01SetProjection(ctx, queue, gp, ps, bdp);
        break;
    case CG_GRAPHICS_TEST01_CMD_DRAW_TRIANGLES:
        res = cgExecuteGraphicsTest01DrawTriangles(ctx, queue, gp, ps, bdp);
        break;
    default:
        res = CG_COMMAND_NOT_IMPLEMENTED;
        break;
    }
    return cgSetupGraphicsCompleteEvent(ctx, queue, gp->AttachedDisplay, bdp->CompleteEvent, res);
}

/// @summary Frees all internal state associated with the TEST01 graphics pipeline.
/// @param ctx A CGFX context returned by cgEnumerateDevices.
/// @param pipeline A pointer to the CG_PIPELINE object.
/// @param opaque The opaque state data supplied when the pipeline was created.
internal_function void
cgTeardownGraphicsPipelineTest01
(
    uintptr_t context, 
    uintptr_t pipeline, 
    void     *opaque
)
{
    CG_CONTEXT          *ctx   = (CG_CONTEXT *) context;
    CG_PIPELINE         *pipe  = (CG_PIPELINE*) pipeline;
    CG_GFX_TEST01_STATE *state = (CG_GFX_TEST01_STATE*) opaque;
    // CG_DISPLAY *display = pipe->AttachedDisplay;
    // for freeing OpenGL resources.
    cgFreeHostMemory(&ctx->HostAllocator, state, sizeof(CG_GFX_TEST01_STATE), 0, CG_ALLOCATION_TYPE_OBJECT);
    UNREFERENCED_PARAMETER(pipe);
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
    cg_kernel_code_t        vss, fss;
    cg_graphics_pipeline_t  gp;
    CG_CONTEXT             *ctx        =(CG_CONTEXT*) context;
    CG_GFX_TEST01_STATE    *state      = NULL;
    cg_handle_t             vs         = CG_INVALID_HANDLE;
    cg_handle_t             gs         = CG_INVALID_HANDLE;
    cg_handle_t             fs         = CG_INVALID_HANDLE;
    cg_handle_t             pipeline   = CG_INVALID_HANDLE;
    cg_shader_binding_t     attribs[2] = 
    {
        {"aPOS", 0}, 
        {"aCLR", 1}
    };

    static char const *vs_code = 
        "#version 150\n"
        "uniform mat4 uMSS;\n"
        "in      vec3 aPOS;\n"
        "in      vec4 aCLR;\n"
        "out     vec4 vCLR;\n"
        "void main() {\n"
        "    vCLR = aCLR;\n"
        "    gl_Position = uMSS * vec4(aPOS.x, aPOS.y, 0, 1);\n"
        "}\n";
    vss.Code     = vs_code;
    vss.CodeSize = strlen(vs_code)+1;
    vss.Type     = CG_KERNEL_TYPE_GRAPHICS_VERTEX;
    vss.Flags    = CG_KERNEL_FLAGS_SOURCE;
    if ((vs = cgCreateKernel(context, exec_group, &vss, result)) == CG_INVALID_HANDLE)
    {   // unable to compile the vertex shader source code.
        return CG_INVALID_HANDLE;
    }

    static char const *fs_code = 
        "#version 150\n"
        "in  vec4 vCLR;\n"
        "out vec4 oCLR;\n"
        "void main() {\n"
        "    oCLR = vCLR;\n"
        "}\n";
    fss.Code     = fs_code;
    fss.CodeSize = strlen(fs_code)+1;
    fss.Type     = CG_KERNEL_TYPE_GRAPHICS_FRAGMENT;
    fss.Flags    = CG_KERNEL_FLAGS_SOURCE;
    if ((fs = cgCreateKernel(context, exec_group, &fss, result)) == CG_INVALID_HANDLE)
    {   // unable to compiler the fragment shader source code.
        cgDeleteObject(context, vs);
        return CG_INVALID_HANDLE;
    }

    // set up the common graphics pipeline state.
    gp.VertexShader      = vs;
    gp.GeometryShader    = gs;
    gp.FragmentShader    = fs;
    gp.AttributeCount    = 2;
    gp.AttributeBindings = attribs;
    gp.OutputCount       = 0;
    gp.OutputBindings    = NULL;
    gp.PrimitiveType     = CG_PRIMITIVE_TRIANGLE_LIST;
    cgBlendStateInitNone(gp.BlendState);
    cgRasterStateInitDefault(gp.RasterizerState);
    cgDepthStencilStateInitDefault(gp.DepthStencilState);

    // allocate and initialize our internal state.
    if ((state = (CG_GFX_TEST01_STATE*) cgAllocateHostMemory(&ctx->HostAllocator, sizeof(CG_GFX_TEST01_STATE), 0, CG_ALLOCATION_TYPE_OBJECT)) == NULL)
    {   // unable to allocate the required memory.
        cgDeleteObject(context, fs);
        cgDeleteObject(context, vs);
        result = CG_OUT_OF_MEMORY;
        return CG_INVALID_HANDLE;
    }
    if ((pipeline = cgCreateGraphicsPipeline(context, exec_group, &gp, state, cgTeardownGraphicsPipelineTest01, result)) == CG_INVALID_HANDLE)
    {   // unable to create the graphics pipeline object.
        cgFreeHostMemory(&ctx->HostAllocator, state, sizeof(CG_GFX_TEST01_STATE), 0, CG_ALLOCATION_TYPE_OBJECT);
        cgDeleteObject(context, fs);
        cgDeleteObject(context, vs);
        return CG_INVALID_HANDLE;
    }
    // initialize the viewport to (0, 0)->(0,0).
    // initialize the MVP matrix to identity.
    // retrieve the uniform record for the MVP matrix.
    CG_PIPELINE          *PO = cgObjectTableGet(&ctx->PipelineTable, pipeline);
    CG_GRAPHICS_PIPELINE &PG = PO->Graphics;
    CG_GLSL_PROGRAM      &PS = PG.ShaderProgram;
    memset(state, 0, sizeof(CG_GFX_TEST01_STATE));
    state->MVP[0] =  state->MVP[5] = state->MVP[10] = state->MVP[15] = 1.0f;
    state->uMSS   =  cgFindItemByName("uMSS", PS.UniformNames, PS.UniformCount, PS.Uniforms);
    cgSetGraphicsPipelineCallback(CG_GRAPHICS_PIPELINE_TEST01, cgExecuteGraphicsPipelineTest01);
    return pipeline;
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
    cmd->CommandId     = CG_COMMAND_PIPELINE_DISPATCH;
    cmd->DataSize      = uint16_t(cmd_size);
    bdp->PipelineId    = CG_GRAPHICS_PIPELINE_TEST01;
    bdp->PipelineCmd   = CG_GRAPHICS_TEST01_CMD_SET_VIEWPORT;
    bdp->ArgsDataSize  = sizeof(CG_GFX_TEST01_SET_VIEWPORT);
    bdp->ReservedU16   = 0; // unused
    bdp->WaitEvent     = wait_event;
    bdp->CompleteEvent = done_event;
    bdp->Pipeline      = pipeline;
    ddp->X             = x;
    ddp->Y             = y;
    ddp->Width         = width;
    ddp->Height        = height;
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
    cmd->CommandId     = CG_COMMAND_PIPELINE_DISPATCH;
    cmd->DataSize      = uint16_t(cmd_size);
    bdp->PipelineId    = CG_GRAPHICS_PIPELINE_TEST01;
    bdp->PipelineCmd   = CG_GRAPHICS_TEST01_CMD_SET_PROJECTION;
    bdp->ArgsDataSize  = sizeof(CG_GFX_TEST01_SET_PROJECTION);
    bdp->ReservedU16   = 0; // unused
    bdp->WaitEvent     = wait_event;
    bdp->CompleteEvent = done_event;
    bdp->Pipeline      = pipeline;
    memcpy(ddp->Matrix, mvp, sizeof(float) * 16);
    return cgCommandBufferUnmapAppend(context, cmd_buffer, cmd_size);
}

/// @summary Enqueues a DRAW_TRIANGLES command for a TEST01 graphics pipeline.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param cmd_buffer The handle of the command buffer to write to.
/// @param pipeline The handle of the TEST01 graphics pipeline state, returned by cgCreateGraphicsPipelineTest01.
/// @param data_source The handle of the vertex data source buffers.
/// @param min_index The smallest index value that will be read from the index buffer.
/// @param max_index The largest index value that will be read from the index buffer.
/// @param base_vertex The offset value added to each value read from the index buffer.
/// @param num_triangles The number of triangles to draw. @a num_triangles * 3 indices will be read.
/// @param done_event The handle of the event to signal when the command has been executed, or CG_INVALID_HANDLE.
/// @param wait_event The handle of the event to wait on before executing the command, or CG_INVALID_HANDLE.
library_function int 
cgGraphicsTest01DrawTriangles
(
    uintptr_t   context,
    cg_handle_t cmd_buffer,
    cg_handle_t pipeline,
    cg_handle_t data_source, 
    uint32_t    min_index, 
    uint32_t    max_index, 
    int32_t     base_vertex, 
    size_t      num_triangles,
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
    cmd->CommandId     = CG_COMMAND_PIPELINE_DISPATCH;
    cmd->DataSize      = uint16_t(cmd_size);
    bdp->PipelineId    = CG_GRAPHICS_PIPELINE_TEST01;
    bdp->PipelineCmd   = CG_GRAPHICS_TEST01_CMD_DRAW_TRIANGLES;
    bdp->ArgsDataSize  = sizeof(CG_GFX_TEST01_DRAW_TRIANGLES);
    bdp->ReservedU16   = 0; // unused
    bdp->WaitEvent     = wait_event;
    bdp->CompleteEvent = done_event;
    bdp->Pipeline      = pipeline;
    ddp->VertexSource  = data_source;
    ddp->MinIndex      = min_index;
    ddp->MaxIndex      = max_index;
    ddp->BaseVertex    = base_vertex;
    ddp->PrimitiveCount= num_triangles;
    return cgCommandBufferUnmapAppend(context, cmd_buffer, cmd_size);
}
