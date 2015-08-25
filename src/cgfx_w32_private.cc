/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implements some internal functions shared between modules, but not
/// exported publicly by the CGFX implementation.
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

/*///////////////
//   Globals   //
///////////////*/

/*///////////////////////
//   Local Functions   //
///////////////////////*/
/// @summary A helper function to release sync objects used to initialize a completion event.
/// @param queue The CGFX command queue to which the command was enqueued.
/// @param cl_sync The OpenCL event object representing the completion event.
/// @param gl_sync The OpenGL fence object representing the completion event.
/// @param result The CGFX result code to return.
/// @return The input @a result code.
internal_function inline int
cgReleaseSyncObjects
(
    CG_QUEUE    *queue, 
    cl_event     cl_sync, 
    GLsync       gl_sync, 
    int          result
)
{
    CG_DISPLAY  *display = queue->AttachedDisplay;
    if (cl_sync != NULL)   clReleaseEvent(cl_sync);
    if (gl_sync != NULL)   glDeleteSync(gl_sync);
    return result;
}

/// @summary A helper function to release all events in a wait list used to initialize a completion event.
/// @param queue The CGFX command queue to which the command was enqueued.
/// @param wait_list An array of @a wait_count OpenCL events that must become signaled for the command to complete.
/// @param wait_count The number of cl_event references in @a wait_list
/// @param result The CGFX result code to return.
/// @return The input @a result code.
internal_function inline int
cgReleaseWaitList
(
    CG_QUEUE    *queue,
    cl_event    *wait_list, 
    size_t const wait_count,
    int          result
)
{   UNREFERENCED_PARAMETER(queue);
    for (size_t i = 0; i < wait_count; ++i)
    {
        if (wait_list[i] != NULL)
            clReleaseEvent(wait_list[i]);
    }
    return result;
}

/*////////////////////////
//   Public Functions   //
////////////////////////*/
/// @summary Retrieves the cl_event associated with a CGFX event object. The returned event object is intended to be used in an OpenCL event wait list.
/// @param ctx A CGFX context returned by cgEnumerateDevices.
/// @param queue The compute or transfer command queue to which the command is being subitted.
/// @param wait_handle A handle of a CGFX event or fence object to wait on.
/// @param wait_list The wait list to update. The cl_event is appended to this list.
/// @param wait_count The number of items currently in the wait list. On successfuly completion, this value is incremented by one.
/// @param max_events The maximum number of cl_event references that can be written to the wait list.
/// @param release_ev On return, this value is set to true if a temporary cl_event was created and should be released by the caller.
/// @return CG_SUCCESS, CG_OUT_OF_OBJECTS, CG_INVALID_VALUE, CG_INVALID_STATE, CG_BAD_CLCONTEXT or CG_BAD_GLCONTEXT.
export_function int
cgGetWaitEvent
(
    CG_CONTEXT *ctx, 
    CG_QUEUE   *queue, 
    cg_handle_t wait_handle,
    cl_event   *wait_list, 
    cl_uint    &wait_count, 
    cl_uint     max_events, 
    bool       &release_ev
)
{
    if (wait_handle == CG_INVALID_HANDLE)
    {   release_ev   = false;
        return CG_SUCCESS;
    }
    if (wait_count  == max_events)
    {   release_ev   = false;
        return CG_OUT_OF_OBJECTS;
    }
    
    switch (cgGetObjectType(wait_handle))
    {
    case CG_OBJECT_FENCE:
        {   // create a temporary event signaled when the fence is passed.
            cl_int    clres = CL_SUCCESS;
            cl_event  clevt = NULL;
            CG_FENCE *fence = cgObjectTableGet(&ctx->FenceTable, wait_handle);
            if (fence == NULL || fence->QueueType != CG_QUEUE_TYPE_GRAPHICS)
            {   // can only create an event for a graphics fence.
                return CG_INVALID_VALUE;
            }
            if (fence->GraphicsFence == NULL)
            {   // the fence object has not yet been submitted to a command queue.
                return CG_INVALID_STATE;
            }
            if ((clevt = clCreateEventFromGLsyncKHR(queue->ComputeContext, fence->GraphicsFence, &clres)) == NULL)
            {   // could not create an OpenCL event from the OpenGL sync object.
                switch (clres)
                {
                case CL_INVALID_CONTEXT  : return CG_BAD_CLCONTEXT;
                case CL_INVALID_GL_OBJECT: return CG_BAD_GLCONTEXT;
                default: break;
                }
                release_ev = false;
                return CG_ERROR;
            }
            wait_list[wait_count++] = clevt;
            release_ev = true;
        }
        return CG_SUCCESS;

    case CG_OBJECT_EVENT:
        {
            CG_EVENT *event = cgObjectTableGet(&ctx->EventTable, wait_handle);
            if (event == NULL)
            {   // an invalid event handle was specified.
                return CG_INVALID_VALUE;
            }
            if (event->ComputeEvent == NULL)
            {   // the event object has not yet been submitted to a command queue.
                return CG_INVALID_STATE;
            }
            release_ev = false;
            clRetainEvent(event->ComputeEvent);
            wait_list[wait_count++] = event->ComputeEvent;
        }
        return CG_SUCCESS;

    default:
        break;
    }
    release_ev = false;
    return CG_INVALID_VALUE;
}

/// @summary Initializes the fields of a CG_EVENT object such that the handle refers to an OpenCL event associated with the completion of a submitted command.
/// @param queue The command queue to which the command was submitted.
/// @param event The CGFX event object to initialize. If this event refers to an existing event, the existing event is released.
/// @param cl_sync The completion event returned by OpenCL, or NULL if @a gl_sync is specified.
/// @param gl_sync The completion event returned by OpenGL (if the command was submitted to a graphics queue), or NULL if @a cl_sync is specified.
/// @param result The result code of the caller.
/// @return The value @a result.
export_function int
cgSetupExistingEvent
(
    CG_QUEUE *queue, 
    CG_EVENT *event, 
    cl_event  cl_sync, 
    GLsync    gl_sync, 
    int       result
)
{   // if the event has an existing cl_event, release it.
    if (event->ComputeEvent != NULL)
    {
        clReleaseEvent(event->ComputeEvent);
        event->ComputeEvent  = NULL;
    }

    // the event may have already been created and associated with an 
    // OpenCL command. in this case, we only need to save the reference.
    if (cl_sync != NULL)
    {
        event->ComputeEvent = cl_sync;
        return result;
    }

    // if the event is linked to a graphics queue fence, use cl_khr_gl_event 
    // to create the corresponding OpenCL event object.
    if (gl_sync != NULL)
    {
        cl_int clres = CL_SUCCESS;
        if ((cl_sync = clCreateEventFromGLsyncKHR(queue->ComputeContext, gl_sync, &clres)) == NULL)
        {   // could not create an OpenCL event from the OpenGL sync object.
            switch (clres)
            {
            case CL_INVALID_CONTEXT  : return CG_BAD_CLCONTEXT;
            case CL_INVALID_GL_OBJECT: return CG_BAD_GLCONTEXT;
            default: break;
            }
            return CG_ERROR;
        }
        event->ComputeEvent = cl_sync;
        return result;
    }
    // if we reach this point, both cl_sync and gl_sync are NULL.
    return CG_INVALID_VALUE;
}

/// @summary Initialize an existing command completion event.
/// @param ctx The CGFX context that created the event object.
/// @param queue The command queue to which the command was submitted.
/// @param complete_event The handle of the CGFX event object to initialize. Any existing OpenCL event will be released.
/// @param cl_sync The completion event returned by OpenCL, or NULL if @a gl_sync is specified.
/// @param gl_sync The completion event returned by OpenGL (if the command was submitted to a graphics queue), or NULL if @a cl_sync is specified.
/// @param result The result code of the caller.
/// @return The value @a result.
export_function int
cgSetupCompleteEvent
(
    CG_CONTEXT *ctx, 
    CG_QUEUE   *queue, 
    cg_handle_t complete_event, 
    cl_event    cl_sync, 
    GLsync      gl_sync, 
    int         result
)
{
    if (result != CG_SUCCESS)
    {   // don't bother setting up the completion event - just return the result.
        return cgReleaseSyncObjects(queue, cl_sync, gl_sync, result);
    }
    if (complete_event == CG_INVALID_HANDLE)
    {   // no completion event is required - just return the result.
        return cgReleaseSyncObjects(queue, cl_sync, gl_sync, result);
    }
    CG_EVENT *done = cgObjectTableGet(&ctx->EventTable, complete_event);
    if (done == NULL)
    {   // the supplied handle doesn't identify a valid event.
        return cgReleaseSyncObjects(queue, cl_sync, gl_sync, CG_INVALID_VALUE);
    }
    return cgSetupExistingEvent(queue, done, cl_sync, gl_sync, result);
}

/// @summary Initialize an existing command completion event that waits on one or more known events.
/// @param ctx The CGFX context that created the event object.
/// @param queue The command queue to which the command was submitted.
/// @param complete_event The handle of the CGFX event object to initialize. Any existing OpenCL event will be released.
/// @param wait_list An array of @a wait_count OpenCL event objects to wait on. The completion event is signaled when all of these events become signaled.
/// @param wait_count The number of cl_event objects in @a wait_list.
/// @param result The result code of the caller.
/// @return The value @a result.
export_function int
cgSetupCompleteEventWithWaitList
(
    CG_CONTEXT   *ctx, 
    CG_QUEUE     *queue, 
    cg_handle_t   complete_event,
    cl_event     *wait_list, 
    size_t const  wait_count, 
    int           result
)
{
    if (result != CG_SUCCESS || wait_count == 0)
    {   // don't bother setting up the completion event - just return the result.
        return cgReleaseWaitList(queue, wait_list, wait_count, result);
    }
    if (complete_event == CG_INVALID_HANDLE)
    {   // no completion event is required - just return the result.
        return cgReleaseWaitList(queue, wait_list, wait_count, result);
    }
    CG_EVENT *done = cgObjectTableGet(&ctx->EventTable, complete_event);
    if (done == NULL)
    {   // the supplied handle doesn't identify a valid event.
        return cgReleaseWaitList(queue, wait_list, wait_count, CG_INVALID_VALUE);
    }
    if (wait_count  > 1)
    {   // when all of the specified events to become signaled, signal the completion event.
        // the completion event is unsignaled until the marker is passed.
        cl_int  clres = CL_SUCCESS;
        cl_event   ev = NULL;
        if ((clres = clEnqueueMarkerWithWaitList(queue->CommandQueue, cl_uint(wait_count), wait_list, &ev)) != CL_SUCCESS)
        {
            switch (clres)
            {
            case CL_INVALID_COMMAND_QUEUE  : result = CG_BAD_CLCONTEXT; break;
            case CL_INVALID_EVENT_WAIT_LIST: result = CG_INVALID_VALUE; break;
            case CL_OUT_OF_RESOURCES       : result = CG_OUT_OF_MEMORY; break;
            case CL_OUT_OF_HOST_MEMORY     : result = CG_OUT_OF_MEMORY; break;
            default                        : result = CG_ERROR;         break;
            }
            return cgReleaseWaitList(queue, wait_list, wait_count, result);
        }
        (void) cgReleaseWaitList(queue, wait_list, wait_count, result);
        return cgSetupExistingEvent(queue, done, ev, NULL, result);
    }
    else
    {   // don't insert an explicit wait into the command stream.
        // don't cgReleaseWaitList here because the completion event isn't add-ref'd.
        return cgSetupExistingEvent(queue, done, wait_list[0], NULL, result);
    }
}

/// @summary Create and initialize a new command completion event.
/// @param ctx The CGFX context that created the event object.
/// @param queue The command queue to which the command was submitted.
/// @param event_handle A pointer to the handle of the CGFX event object to initialize. If NULL, no event is created. Any existing OpenCL event will be released.
/// @param cl_sync The completion event returned by OpenCL, or NULL if @a gl_sync is specified.
/// @param gl_sync The completion event returned by OpenGL (if the command was submitted to a graphics queue), or NULL if @a cl_sync is specified.
/// @param result The result code of the caller.
/// @return The value @a result.
export_function int
cgSetupNewCompleteEvent
(
    CG_CONTEXT  *ctx, 
    CG_QUEUE    *queue, 
    cg_handle_t *event_handle,
    cl_event     cl_sync,
    GLsync       gl_sync, 
    int          result
)
{
    if (result != CG_SUCCESS)
    {   // don't bother creating a completion event - just return the result.
        if (event_handle != NULL)
        {   // indicate that no event is being returned.
            *event_handle = CG_INVALID_HANDLE;
        }
        return cgReleaseSyncObjects(queue, cl_sync, gl_sync, result);
    }
    if (event_handle == NULL)
    {   // no event was requested by the caller, so just return the result.
        return cgReleaseSyncObjects(queue, cl_sync, gl_sync, result);
    }
    
    CG_EVENT    ev; ev.ComputeEvent = NULL;
    cg_handle_t ev_h = cgObjectTableAdd(&ctx->EventTable, ev);
    if (ev_h == CG_INVALID_HANDLE)
    {   // unable to create the new event object - the object table is full.
        *event_handle = CG_INVALID_HANDLE;
        return cgReleaseSyncObjects(queue, cl_sync, gl_sync, CG_OUT_OF_OBJECTS);
    }
    CG_EVENT *done = cgObjectTableGet(&ctx->EventTable, ev_h);
    return cgSetupExistingEvent(queue, done, cl_sync, gl_sync, result);
}

/// @summary Add a memory object reference to a memref list.
/// @param memref The OpenCL memory object reference to add.
/// @param memref_list The memory object reference list to update. The memref will be appended to the list.
/// @param memref_count The number of items in the memref list. On return, this value is incremented by one.
/// @param max_memrefs The maximum number of items that can be written to the memref list.
/// @param check_list Specify true to search the memref_list for memref to avoid adding duplicate items.
export_function void
cgMemRefListAddMem
(
    cl_mem       memref, 
    cl_mem      *memref_list, 
    size_t      &memref_count, 
    size_t const max_memrefs, 
    bool         check_list
)
{
    if (memref_count >= max_memrefs)
    {   // the list is full. this should never happen.
        assert(memref_count < max_memrefs);
        return;
    }
    if (check_list)
    {   // check for memref in the existing list to avoid adding duplicate entries.
        for (size_t i = 0, n = memref_count; i < n; ++i)
        {
            if (memref_list[i] == memref)
                return;
        }
    }
    memref_list[memref_count++] = memref;
}

/// @summary Add a buffer object to a memref list. The buffer is added only if it is shared with OpenGL.
/// @param buffer The CGFX buffer object reference to add.
/// @param memref_list The memory object reference list to update. The memref will be appended to the list.
/// @param memref_count The number of items in the memref list. On return, this value is incremented by one.
/// @param max_memrefs The maximum number of items that can be written to the memref list.
/// @param check_list Specify true to search the memref_list for memref to avoid adding duplicate items.
export_function void
cgMemRefListAddBuffer
(
    CG_BUFFER    *buffer, 
    cl_mem       *memref_list,
    size_t       &memref_count, 
    size_t const  max_memrefs, 
    bool          check_list
)
{
    if (buffer->GraphicsBuffer == 0 || buffer->ComputeBuffer == NULL)
    {   // the buffer object is not shared with OpenGL, so ignore it.
        return;
    }
    cgMemRefListAddMem(buffer->ComputeBuffer, memref_list, memref_count, max_memrefs, check_list);
}

/// @summary Add an image object to a memref list. The image is added only if it is shared with OpenGL.
/// @param image The CGFX image object reference to add.
/// @param memref_list The memory object reference list to update. The memref will be appended to the list.
/// @param memref_count The number of items in the memref list. On return, this value is incremented by one.
/// @param max_memrefs The maximum number of items that can be written to the memref list.
/// @param check_list Specify true to search the memref_list for memref to avoid adding duplicate items.
export_function void
cgMemRefListAddImage
(
    CG_IMAGE     *image, 
    cl_mem       *memref_list, 
    size_t       &memref_count, 
    size_t const  max_memrefs, 
    bool          check_list
)
{
    if (image->GraphicsImage == 0 || image->ComputeImage == NULL)
    {   // the image object is not shared with OpenGL, so ignore it.
        return;
    }
    cgMemRefListAddMem(image->ComputeImage, memref_list, memref_count, max_memrefs, check_list);
}

/// @summary Acquire shared memory objects for use by OpenCL.
/// @param ctx The CGFX context returned by cgEnumerateDevices that manages the command queue.
/// @param queue The CGFX command queue being updated.
/// @param memref_list The list of referenced memory objects, or NULL.
/// @param memref_count The number of shared memory object references to acquire for compute use.
/// @param sync_event The handle of a completion event returned by cgDeviceFence against a graphics queue. This event must become signaled before shared resources can be acquired.
/// @param wait_events A list of OpenCL event handles. On return, if necessary, an event is appended to this list. This event will be signaled when shared resources have been acquired.
/// @param wait_count The number of OpenCL event handles in @a wait_events. On return, this value may be incremented by one.
/// @param max_wait_events The maximum number of items that can be stored in @a wait_events.
/// @return One of CG_SUCCESS, CG_INVALID_VALUE, CG_BAD_CLCONTEXT, CG_OUT_OF_MEMORY or CG_ERROR.
export_function int 
cgAcquireMemoryObjects
(
    CG_CONTEXT   *ctx, 
    CG_QUEUE     *queue, 
    cl_mem       *memref_list, 
    size_t  const memref_count, 
    cg_handle_t   sync_event, 
    cl_event     *wait_events, 
    cl_uint      &wait_count, 
    cl_uint const max_wait_events
)
{
    // basically, I'm building my graphics command buffer:
    // ...
    // insert command to draw to framebuffer (for example)
    // cgDeviceFence(context, cmd_buffer_gfx, framebuffer_finish_fence, framebuffer_finish_event)
    // ...
    // the graphics command buffer must be submitted before the corresponding compute command buffer.
    // cgExecuteCommandBuffer(cmd_buffer_gfx)
    // 
    // now I'm going to copy the framebuffer to compute-land.
    // cgCopyBuffer(context, cmd_buffer_xfer, compute_buffer, 0, framebuffer, copy_done_event, framebuffer_finish_event);
    // ...
    // cgExecuteCommandBuffer(cmd_buffer_xfer)
    //
    // so, what I want is to pass framebuffer_finish_event into this function.
    // basically, in my graphics queue I will insert a fence to indicate that
    // I'm done modifying a set of resources.
    if (memref_count > 0)
    {
        CG_DISPLAY *display = queue->AttachedDisplay;
        CG_EVENT   *glev    = NULL;
        cl_event    glwait  = NULL;
        cl_uint     glnum   = 0;

        // typically, an event tied to a graphics queue fence will be specified.
        // when this fence is passed, it is safe to acquire the OpenGL resources.
        if (sync_event != CG_INVALID_HANDLE)
        {   // an explicit synchronization event was specified. this is the 
            // most performant path as it doesn't flush the graphics queue.
            if ((glev = cgObjectTableGet(&ctx->EventTable, sync_event)) != NULL)
            {
                if (glev->ComputeEvent != NULL)
                {
                    glwait = glev->ComputeEvent;
                    glnum  = 1;
                }
            }
        }
        if (glnum == 0)
        {   // no explicit synchronization event was specified. brute-force flush 
            // the graphics queue. this causes a device stall and can significantly
            // reduce performance. it is recommended to specify an explicit fence.
            glFinish(); glwait = NULL; glnum = 0;
        }

        cl_event  ev = NULL;
        cl_int clres = clEnqueueAcquireGLObjects(queue->CommandQueue, cl_uint(memref_count), memref_list, glnum, &glwait, &ev);
        if (clres   != CL_SUCCESS)
        {   // convert the OpenCL error code to a CGFX result code.
            int    r = CG_ERROR;
            switch(r)
            {
            case CL_INVALID_VALUE          : r = CG_INVALID_VALUE; break;
            case CL_INVALID_MEM_OBJECT     : r = CG_INVALID_VALUE; break;
            case CL_INVALID_COMMAND_QUEUE  : r = CG_INVALID_VALUE; break;
            case CL_INVALID_CONTEXT        : r = CG_BAD_CLCONTEXT; break;
            case CL_INVALID_GL_OBJECT      : r = CG_INVALID_VALUE; break;
            case CL_INVALID_EVENT_WAIT_LIST: r = CG_INVALID_VALUE; break;
            case CL_OUT_OF_RESOURCES       : r = CG_OUT_OF_MEMORY; break;
            case CL_OUT_OF_HOST_MEMORY     : r = CG_OUT_OF_MEMORY; break;
            default                        : r = CG_ERROR;         break;
            }
            return r;
        }
        if (wait_count < max_wait_events)
        {   // save the event handle in the wait list.
            wait_events[wait_count++] = ev;
        }
        else clReleaseEvent(ev);
    }
    return CG_SUCCESS;
}

/// @summary Release shared memory objects back to OpenGL.
/// @param ctx The CGFX context returned by cgEnumerateDevices that manages the command queue.
/// @param queue The CGFX command queue being updated.
/// @param memref_list The list of referenced memory objects, or NULL. This should be the same list passed to cgAcquireMemoryObjects.
/// @param memref_count The number of shared memory object references to release back to OpenGL. This should be the same count passed to cgAcquireMemoryObjects.
/// @param wait_events The list of OpenCL events to wait on prior to releasing shared resources back to OpenGL, or NULL.
/// @param wait_count The number of items in @a wait_events.
/// @param done_event The handle of a CGFX event object to signal when OpenGL can safely access the device resources.
export_function int
cgReleaseMemoryObjects
(
    CG_CONTEXT   *ctx, 
    CG_QUEUE     *queue, 
    cl_mem       *memref_list, 
    size_t const  memref_count, 
    cl_event     *wait_events,
    size_t const  wait_count, 
    cg_handle_t   done_event
)
{
    // there is no command wrapper. each command follows the basic formula:
    // foreach (mem_object_referenced_by_command)
    //   clMemRefListAdd(...)
    // cgAcquireMemoryObjects(...)
    // ... submit kernels or commands
    // return cgReleaseMemoryObjects(..., done_event)
    //
    // and back on the display thread:
    // cg_handle_t fence = cgCreateFenceForEvent(..., done_event, ...);
    // cgDeviceFence(..., graphics_queue, fence, ...);
    // ... commands that access the resources updated by compute
    // 
    // and this is fine, because we use pipelines, which may submit to multiple kernels.
    // for interop, our wait_event will *always* be a GL fence.
    if (memref_count > 0)
    {   // ensure that any commands that reference shared resources have finished executing.
        if (wait_count == 0 || wait_events == NULL)
        {   // if no explicit command queue events were specified, flush the compute pipeline.
            // this is not the most peformant path, but it guarantees correctness.
            clFinish(queue->CommandQueue);
        }
        // enqueue the command to release the shared resources back to OpenGL.
        cl_event  ev = NULL;
        cl_int clres = clEnqueueReleaseGLObjects(queue->CommandQueue, cl_uint(memref_count), memref_list, cl_uint(wait_count), wait_events, &ev);
        if (clres != CL_SUCCESS)
        {
            int r  = CG_ERROR;
            switch(clres)
            {
            case CL_INVALID_VALUE          : r = CG_INVALID_VALUE; break;
            case CL_INVALID_MEM_OBJECT     : r = CG_BAD_CLCONTEXT; break;
            case CL_INVALID_COMMAND_QUEUE  : r = CG_INVALID_VALUE; break;
            case CL_INVALID_CONTEXT        : r = CG_BAD_CLCONTEXT; break;
            case CL_INVALID_GL_OBJECT      : r = CG_BAD_GLCONTEXT; break;
            case CL_INVALID_EVENT_WAIT_LIST: r = CG_INVALID_VALUE; break;
            case CL_OUT_OF_RESOURCES       : r = CG_OUT_OF_MEMORY; break;
            case CL_OUT_OF_HOST_MEMORY     : r = CG_OUT_OF_MEMORY; break;
            default                        : r = CG_ERROR;         break;
            }
            return cgSetupCompleteEventWithWaitList(ctx, queue, done_event, wait_events, wait_count, r);
        }
        return cgSetupCompleteEvent(ctx, queue, done_event, ev, NULL, CG_SUCCESS);
    }
    return cgSetupCompleteEventWithWaitList(ctx, queue, done_event, wait_events, wait_count, CG_SUCCESS);
}
