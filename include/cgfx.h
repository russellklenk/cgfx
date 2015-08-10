/*/////////////////////////////////////////////////////////////////////////////
/// @summary Define the public constants, data types and functions exposed by 
/// the unified Compute and Graphics library.
///////////////////////////////////////////////////////////////////////////80*/

#ifndef LIB_CGFX_H
#define LIB_CGFX_H

/*////////////////
//   Includes   //
////////////////*/
#include <stddef.h>
#include <stdint.h>

/*////////////////////
//   Preprocessor   //
////////////////////*/
/// @summary Define the calling convention, for platforms where it matters.
#ifdef  _MSC_VER
#define CG_API                    __cdecl
#else
#define CG_API
#endif

/// @summary Define macros and constants for packing, cracking and formatting version numbers.
#ifndef CG_MAKE_VERSION
#define CG_VERSION_MAX_CHARS      14
#define CG_VERSION_FORMATA        "%d.%d.%d"
#define CG_VERSION_FORMATW        L"%d.%d.%d"
#define CG_VERSION_COMPONENT_MASK 0xFF
#define CG_VERSION_BUILD_MASK     0xFFFF
#define CG_VERSION_MAJOR_SHIFT    0
#define CG_VERSION_MINOR_SHIFT    8
#define CG_VERSION_BUILD_SHIFT    16
#define CG_MAKE_VERSION(mj,mi,bn) \
    ((((mj) & CG_VERSION_COMPONENT_MASK) << CG_VERSION_MAJOR_SHIFT) | \
     (((mi) & CG_VERSION_COMPONENT_MASK) << CG_VERSION_MINOR_SHIFT) | \
     (((bn) & CG_VERSION_BUILD_MASK)     << CG_VERSION_BUILD_SHIFT))
#define CG_VERSION_MAJOR(ver)     \
    (((ver) >>CG_VERSION_MAJOR_SHIFT)     & CG_VERSION_COMPONENT_MASK)
#define CG_VERSION_MINOR(ver)     \
    (((ver) >>CG_VERSION_MINOR_SHIFT)     & CG_VERSION_COMPONENT_MASK)
#define CG_VERSION_BUILD(ver)     \
    (((ver) >>CG_VERSION_BUILD_SHIFT)     & CG_VERSION_BUILD_MASK)
#endif

/// @summary Define the current version of the cgfx API.
#ifndef CG_API_VERSION
#define CG_API_VERSION_MAJOR      1
#define CG_API_VERSION_MINOR      0
#define CG_API_VERSION_BUILD      1
#define CG_API_VERSION            CG_MAKE_VERSION(CG_API_VERSION_MAJOR, CG_API_VERSION_MINOR, CG_API_VERSION_BUILD)
#endif

/*////////////////////////////
//   Forward Declarations   //
////////////////////////////*/
/// @summary The handle type used to uniquely identify a resource or API object.
/// Defined up here because this type is referenced everywhere.
typedef uint64_t cg_handle_t;

/// @summary Forward declarations for types used in the function pointers below.
struct cg_application_info_t;
struct cg_allocation_callbacks_t;
struct cg_execution_group_t;
struct cg_cpu_partition_t;
struct cg_cpu_info_t;
struct cg_heap_info_t;
struct cg_command_t;
struct cg_kernel_code_t;
struct cg_blend_state_t;
struct cg_raster_state_t;
struct cg_depth_stencil_state_t;
struct cg_graphics_pipeline_t;
struct cg_compute_pipeline_t;

/*/////////////////
//   Constants   //
/////////////////*/
/// @summary A special value representing an invalid handle.
#define CG_INVALID_HANDLE   ((cg_handle_t)0)

/// @summary A special value meaning that CGFX will determine the best heap placement.
#define CG_IDEAL_HEAP       (~size_t(0))

/*////////////////////////////
//  Function Pointer Types  //
////////////////////////////*/
typedef void*        (CG_API *cgMemoryAlloc_fn                 )(size_t, size_t, int, uintptr_t);
typedef void         (CG_API *cgMemoryFree_fn                  )(void *, size_t, size_t, int, uintptr_t);
typedef char const*  (CG_API *cgResultString_fn                )(int);
typedef int          (CG_API *cgGetCpuInfo_fn                  )(cg_cpu_info_t *);
typedef int          (CG_API *cgDefaultCpuPartition_fn         )(cg_cpu_partition_t *);
typedef int          (CG_API *cgValidateCpuPartition_fn        )(cg_cpu_partition_t const *, cg_cpu_info_t const *);
typedef int          (CG_API *cgEnumerateDevices_fn            )(cg_application_info_t const*, cg_allocation_callbacks_t *, size_t&, cg_handle_t *, size_t, uintptr_t &);
typedef int          (CG_API *cgGetContextInfo_fn              )(uintptr_t, int, void *, size_t, size_t *);
typedef size_t       (CG_API *cgGetHeapCount_fn                )(uintptr_t);
typedef int          (CG_API *cgGetHeapProperties_fn           )(uintptr_t, size_t, cg_heap_info_t &);
typedef size_t       (CG_API *cgGetDeviceCount_fn              )(uintptr_t);
typedef int          (CG_API *cgGetDeviceInfo_fn               )(uintptr_t, cg_handle_t, int, void *, size_t, size_t *);
typedef size_t       (CG_API *cgGetDisplayCount_fn             )(uintptr_t);
typedef cg_handle_t  (CG_API *cgGetPrimaryDisplay_fn           )(uintptr_t);
typedef cg_handle_t  (CG_API *cgGetDisplayByOrdinal_fn         )(uintptr_t);
typedef int          (CG_API *cgGetDisplayInfo_fn              )(uintptr_t, cg_handle_t, int, void *, size_t, size_t *);
typedef cg_handle_t  (CG_API *cgGetDisplayDevice_fn            )(uintptr_t, cg_handle_t);
typedef int          (CG_API *cgGetCPUDevices_fn               )(uintptr_t, size_t &, size_t const, cg_handle_t *);
typedef int          (CG_API *cgGetGPUDevices_fn               )(uintptr_t, size_t &, size_t const, cg_handle_t *);
typedef int          (CG_API *cgGetAcceleratorDevices_fn       )(uintptr_t, size_t &, size_t const, cg_handle_t *);
typedef int          (CG_API *cgGetCPUDevicesInShareGroup_fn   )(uintptr_t, cg_handle_t, size_t &, size_t const, cg_handle_t *);
typedef int          (CG_API *cgGetGPUDevicesInShareGroup_fn   )(uintptr_t, cg_handle_t, size_t &, size_t const, cg_handle_t *);
typedef cg_handle_t  (CG_API *cgCreateExecutionGroup_fn        )(uintptr_t, cg_execution_group_t const *, int &);
typedef int          (CG_API *cgGetExecutionGroupInfo_fn       )(uintptr_t, cg_handle_t, int, void *, size_t, size_t *);
typedef cg_handle_t  (CG_API *cgGetQueueForDevice_fn           )(uintptr_t, cg_handle_t, int, int &);
typedef cg_handle_t  (CG_API *cgGetQueueForDisplay_fn          )(uintptr_t, cg_handle_t, int, int &);
typedef int          (CG_API *cgDeleteObject_fn                )(uintptr_t, cg_handle_t);
typedef cg_handle_t  (CG_API *cgCreateCommandBuffer_fn         )(uintptr_t, int, int &);
typedef int          (CG_API *cgBeginCommandBuffer_fn          )(uintptr_t, cg_handle_t, uint32_t);
typedef int          (CG_API *cgResetCommandBuffer_fn          )(uintptr_t, cg_handle_t);
typedef int          (CG_API *cgCommandBufferAppend_fn         )(uintptr_t, cg_handle_t, uint16_t, size_t, void const*);
typedef int          (CG_API *cgCommandBufferMapAppend_fn      )(uintptr_t, cg_handle_t, size_t, cg_command_t **);
typedef int          (CG_API *cgCommandBufferUnmapAppend_fn    )(uintptr_t, cg_handle_t, size_t);
typedef int          (CG_API *cgEndCommandBuffer_fn            )(uintptr_t, cg_handle_t);
typedef int          (CG_API *cgCommandBufferCanRead_fn        )(uintptr_t, cg_handle_t, size_t &);
typedef cg_command_t*(CG_API *cgCommandBufferCommandAt_fn      )(uintptr_t, cg_handle_t, size_t &, int &);
typedef cg_handle_t  (CG_API *cgCreateEvent_fn                 )(uintptr_t, cg_handle_t, uint32_t, int &);
typedef int          (CG_API *cgHostWaitForEvent_fn            )(uintptr_t, cg_handle_t);
typedef cg_handle_t  (CG_API *cgCreateKernel_fn                )(uintptr_t, cg_handle_t, cg_kernel_code_t const *, int &);
typedef cg_handle_t  (CG_API *cgCreateComputePipeline_fn       )(uintptr_t, cg_handle_t, cg_compute_pipeline_t const *, int &);
typedef cg_handle_t  (CG_API *cgCreateGraphicsPipeline_fn      )(uintptr_t, cg_handle_t, cg_graphics_pipeline_t const *, int &);
typedef cg_handle_t  (CG_API *cgCreateDataBuffer_fn            )(uintptr_t, cg_handle_t, size_t, uint32_t, uint32_t, uint32_t, int, int, int &);
typedef int          (CG_API *cgGetDataBufferInfo_fn           )(uintptr_t, cg_handle_t, int, void *, size_t, size_t *);
typedef void*        (CG_API *cgMapDataBuffer_fn               )(uintptr_t, cg_handle_t, cg_handle_t, size_t, size_t, uint32_t, int);
typedef int          (CG_API *cgUnmapDataBuffer_fn             )(uintptr_t, cg_handle_t, cg_handle_t, void *, cg_handle_t *);
typedef int          (CG_API *cgBlendStateInitNone_fn          )(cg_blend_state_t &);
typedef int          (CG_API *cgBlendStateInitAlpha_fn         )(cg_blend_state_t &);
typedef int          (CG_API *cgBlendStateInitAdditive_fn      )(cg_blend_state_t &);
typedef int          (CG_API *cgBlendStateInitPremultiplied_fn )(cg_blend_state_t &);
typedef int          (CG_API *cgRasterStateInitDefault_fn      )(cg_raster_state_t &);
typedef int          (CG_API *cgDepthStencilStateInitDefault_fn)(cg_depth_stencil_state_t &);

/*//////////////////
//   Data Types   //
//////////////////*/
/// @summary Define the recognized function return codes.
enum cg_result_e : int
{
    // CORE API RESULT CODES - FAILURE
    CG_ERROR                           = -1,        /// A generic error occurred.
    CG_INVALID_VALUE                   = -2,        /// One or more input values are invalid.
    CG_OUT_OF_MEMORY                   = -3,        /// The requested memory could not be allocated.
    CG_NO_PIXELFORMAT                  = -4,        /// Unable to find an accelerated pixel format.
    CG_BAD_PIXELFORMAT                 = -5,        /// Unable to set the drawable's pixel format.
    CG_NO_GLCONTEXT                    = -6,        /// Unable to create the OpenGL rendering context.
    CG_BAD_GLCONTEXT                   = -7,        /// Unable to activate the OpenGL rendering context.
    CG_NO_CLCONTEXT                    = -8,        /// Unable to create the OpenCL device context.
    CG_BAD_CLCONTEXT                   = -9,        /// The OpenCL context is invalid.
    CG_OUT_OF_OBJECTS                  = -10,       /// There are no more available objects of the requested type.
    CG_UNKNOWN_GROUP                   = -11,       /// The object has no associated execution group.
    CG_INVALID_STATE                   = -12,       /// The object is in an invalid state for the operation.
    CG_COMPILE_FAILED                  = -13,       /// The kernel source code compilation failed.
    CG_LINK_FAILED                     = -14,       /// The pipeline linking phase failed.

    // EXTENSION API RESULT CODES - FAILURE
    CG_RESULT_FAILURE_EXT              = -100000,   /// The first valid failure result code for extensions.

    // CORE API RESULT CODES - NON-FAILURE
    CG_SUCCESS                         =  0,        /// The operation completed successfully.
    CG_UNSUPPORTED                     =  1,        /// The function completed successfully, but the operation is not supported.
    CG_NOT_READY                       =  2,        /// The function completed successfully, but the result is not available yet.
    CG_TIMEOUT                         =  3,        /// The wait completed because the timeout interval elapsed.
    CG_SIGNALED                        =  4,        /// The wait completed because the event became signaled.
    CG_BUFFER_TOO_SMALL                =  5,        /// The supplied buffer cannot store the required amount of data.
    CG_NO_OPENCL                       =  6,        /// No compatible OpenCL platforms or devices are available.
    CG_NO_OPENGL                       =  7,        /// The display does not support the required version of OpenGL.
    CG_NO_GLSHARING                    =  8,        /// Rendering context created, but no sharing between OpenGL and OpenCL is available.
    CG_NO_QUEUE_OF_TYPE                =  9,        /// The device or display does not have a queue of the specified type.
    CG_END_OF_BUFFER                   =  10,       /// The end of the buffer has been reached.

    // EXTENSION API RESULT CODES - NON-FAILURE
    CG_RESULT_NON_FAILURE_EXT          =  100000,   /// The first valid non-failure result code for extensions.
};

/// @summary Define the categories of host memory allocations performed by CGFX.
enum cg_allocation_type_e : int
{
    CG_ALLOCATION_TYPE_OBJECT          =  1,        /// Allocation used for an API object or data with API lifetime.
    CG_ALLOCATION_TYPE_INTERNAL        =  2,        /// Allocation used for long-lived internal data.
    CG_ALLOCATION_TYPE_TEMP            =  3,        /// Allocation used for short-lived internal data.
    CG_ALLOCATION_TYPE_KERNEL          =  4,        /// Allocation used for short-lived kernel compilation.
    CG_ALLOCATION_TYPE_DEBUG           =  5         /// Allocation used for debug data.
};

/// @summary Define the recognized object type identifiers.
enum cg_object_e : uint32_t
{
    CG_OBJECT_NONE                     = (0 <<  0), /// An invalid object type identifier.
    CG_OBJECT_DEVICE                   = (1 <<  0), /// The object type identifier for a device.
    CG_OBJECT_DISPLAY                  = (1 <<  1), /// The object type identifier for a display output.
    CG_OBJECT_EXECUTION_GROUP          = (1 <<  2), /// The object type identifier for an execution group.
    CG_OBJECT_QUEUE                    = (1 <<  3), /// The object type identifier for a queue.
    CG_OBJECT_COMMAND_BUFFER           = (1 <<  4), /// The object type identifier for a command buffer.
    CG_OBJECT_FENCE                    = (1 <<  5), /// The object type identifier for a fence.
    CG_OBJECT_EVENT                    = (1 <<  6), /// The object type identifier for a waitable event.
    CG_OBJECT_QUERY_POOL               = (1 <<  7), /// The object type identifier for a query pool.
    CG_OBJECT_IMAGE                    = (1 <<  8), /// The object type identifier for an image.
    CG_OBJECT_KERNEL                   = (1 <<  9), /// The object type identifier for a compute or shader kernel.
    CG_OBJECT_PIPELINE                 = (1 << 10), /// The object type identifier for a compute or display pipeline.
    CG_OBJECT_SAMPLER                  = (1 << 11), /// The object type identifier for an image sampler.
    CG_OBJECT_BUFFER                   = (1 << 12), /// The object type identifier for a data buffer.
};

/// @summary Define the queryable data on a CGFX context object.
enum cg_context_info_param_e : int
{
    CG_CONTEXT_CPU_COUNTS              =  0,        /// Retrieve the number of CPU resources in the system. Data is cg_cpu_counts_t.
    CG_CONTEXT_DEVICE_COUNT            =  1,        /// Retrieve the number of capable compute devices in the system. Data is size_t.
    CG_CONTEXT_DISPLAY_COUNT           =  2,        /// Retrieve the number of capable display devices attached to the system. Data is size_t. 
};

/// @summary Define the queryable data on a CGFX device object.
enum cg_device_info_param_e : int
{
    CG_DEVICE_CL_PLATFORM_ID           =  0,        /// Retrieve the OpenCL platform ID of the device. Data is cl_platform_id.
    CG_DEVICE_CL_DEVICE_ID             =  1,        /// Retrieve the OpenCL device ID of the device. Data is cl_device_id.
    CG_DEVICE_CL_DEVICE_TYPE           =  2,        /// Retrieve the OpenCL device type of the device. Data is cl_device_type.
    CG_DEVICE_WINDOWS_HGLRC            =  3,        /// Retrieve the OpenGL rendering context of the device. Windows-only. Data is HGLRC.
    CG_DEVICE_DISPLAY_COUNT            =  4,        /// Retrieve the number of displays attached to the device. Data is size_t.
    CG_DEVICE_ATTACHED_DISPLAYS        =  5,        /// Retrieve the object handles of the attached displays. Data is cg_handle_t[CG_DEVICE_DISPLAY_COUNT].
    CG_DEVICE_PRIMARY_DISPLAY          =  6,        /// Retrieve the primary display attached to the device. Data is cg_handle_t.
};

/// @summary Define the queryable data on a CGFX display object.
enum cg_display_info_param_e : int
{
    CG_DISPLAY_DEVICE                  =  0,        /// Retrieve the CGFX handle of the compute device driving the display. Data is cg_handle_t.
    CG_DISPLAY_CL_PLATFORM_ID          =  1,        /// Retrieve the OpenCL platform ID of the device driving the display. Data is cl_platform_id.
    CG_DISPLAY_CL_DEVICE_ID            =  2,        /// Retrieve the OpenCL device ID of the device driving the display. Data is cl_platform_id.
    CG_DISPLAY_WINDOWS_HDC             =  3,        /// Retrieve the Windows device context of the display. Data is HDC.
    CG_DISPLAY_WINDOWS_HGLRC           =  4,        /// Retrieve the Windows OpenGL rendering context of the device. Data is HGLRC.
    CG_DISPLAY_POSITION                =  5,        /// Retrieve the x and y-coordinate of the upper-left corner of the display in global display coordinates. Data is int[2].
    CG_DISPLAY_SIZE                    =  6,        /// Retrieve the width and height of the display, in pixels. Data is size_t[2].
    CG_DISPLAY_ORIENTATION             =  7,        /// Retrieve the current orientation of the display. Data is cg_display_orientation_e.
    CG_DISPLAY_REFRESH_RATE            =  8,        /// Retrieve the vertical refresh rate of the display, in hertz. Data is float.
};

/// @summary Define the queryable data on a CGFX execution group object.
enum cg_execution_group_info_param_e : int
{
    CG_EXEC_GROUP_DEVICE_COUNT         =  0,        /// Retrieve the number of devices in the execution group. Data is size_t.
    CG_EXEC_GROUP_DEVICES              =  1,        /// Retrieve the handles of all devices in the execution group. Data is cg_handle_t[CG_EXEC_GROUP_DEVICE_COUNT].
    CG_EXEC_GROUP_CPU_COUNT            =  2,        /// Retrieve the number of CPU devices in the execution group. Data is size_t.
    CG_EXEC_GROUP_CPU_DEVICES          =  3,        /// Retrieve the handles of all CPU devices in the execution group. Data is cg_handle_t[CG_EXEC_GROUP_CPU_COUNT].
    CG_EXEC_GROUP_GPU_COUNT            =  4,        /// Retrieve the number of GPU devices in the execution group. Data is size_t.
    CG_EXEC_GROUP_GPU_DEVICES          =  5,        /// Retrieve the handles of all GPU devices in the execution group. Data is cg_handle_t[CG_EXEC_GROUP_GPU_COUNT].
    CG_EXEC_GROUP_ACCELERATOR_COUNT    =  6,        /// Retrieve the number of accelerator devices in the execution group. Data is size_t.
    CG_EXEC_GROUP_ACCELERATOR_DEVICES  =  7,        /// Retrieve the handles of all accelerator devices in the execution group. Data is cg_handle_t[CG_EXEC_GROUP_ACCELERATOR_COUNT].
    CG_EXEC_GROUP_DISPLAY_COUNT        =  8,        /// Retrieve the number of displays attached to the execution group. Data is size_t.
    CG_EXEC_GROUP_ATTACHED_DISPLAYS    =  9,        /// Retrieve the handles of all attached displays in the execution group. Data is cg_handle_t[CG_EXEC_GROUP_DISPLAY_COUNT].
    CG_EXEC_GROUP_QUEUE_COUNT          =  10,       /// Retrieve the number of queues in the execution group. Data is size_t.
    CG_EXEC_GROUP_QUEUES               =  11,       /// Retrieve the handles of all queues in the execution group. Data is cg_handle_t[CG_EXEC_GROUP_QUEUE_COUNT].
    CG_EXEC_GROUP_COMPUTE_QUEUE_COUNT  =  12,       /// Retrieve the number of compute queues in the execution group. Data is size_t.
    CG_EXEC_GROUP_COMPUTE_QUEUES       =  13,       /// Retrieve the handles of all compute queues in the execution group. Data is cg_handle_t[CG_EXEC_GROUP_COMPUTE_QUEUE_COUNT].
    CG_EXEC_GROUP_TRANSFER_QUEUE_COUNT =  14,       /// Retrieve the number of transfer queues in the execution group. Data is size_t.
    CG_EXEC_GROUP_TRANSFER_QUEUES      =  15,       /// Retrieve the handles of all transfer queues in the execution group. Data is cg_handle_t[CG_EXEC_GROUP_TRANSFER_QUEUE_COUNT].
    CG_EXEC_GROUP_GRAPHICS_QUEUE_COUNT =  16,       /// Retrieve the number of graphics queues in the execution group. Data is size_t.
    CG_EXEC_GROUP_GRAPHICS_QUEUES      =  17,       /// Retrieve the handles of all graphics queues in the execution group. Data is cg_handle_t[CG_EXEC_GROUP_GRAPHICS_QUEUE_COUNT].
    CG_EXEC_GROUP_CL_CONTEXT           =  18,       /// Retrieve the OpenCL resource context for the execution group. Data is cl_context.
    CG_EXEC_GROUP_WINDOWS_HGLRC        =  19,       /// Retrieve the OpenGL rendering context for the execution group. Data is HGLRC.
};

/// @summary Define the queryable data on a CGFX data buffer object.
enum cg_data_buffer_info_param_e : int
{
    CG_DATA_BUFFER_HEAP_ORDINAL        =  0,        /// Retrieve the ordinal of the heap on which the buffer was allocated.
    CG_DATA_BUFFER_HEAP_TYPE           =  1,        /// Retrieve the cg_heap_type_e of the heap from which the buffer was allocated. Data is int.
    CG_DATA_BUFFER_HEAP_FLAGS          =  2,        /// Retrieve the cg_heap_flags_e of the heap from which the buffer was allocated. Data is uint32_t.
    CG_DATA_BUFFER_ALLOCATED_SIZE      =  3,        /// Retrieve the number of bytes allocated for the data buffer. Data is size_t.
};

/// @summary Define the recognized display orientation values.
enum cg_display_orientation_e : int
{
    CG_DISPLAY_ORIENTATION_UNKNOWN     =  0,        /// The display orientation is not known.
    CG_DISPLAY_ORIENTATION_LANDSCAPE   =  1,        /// The display device is in landscape mode.
    CG_DISPLAY_ORIENTATION_PORTRAIT    =  2,        /// The display device is in portrait mode.
};

/// @summary Define the supported types of execution group queues.
enum cg_queue_type_e : int
{
    CG_QUEUE_TYPE_COMPUTE              =  1,        /// The queue is used for submitting compute kernel dispatch commands.
    CG_QUEUE_TYPE_GRAPHICS             =  2,        /// The queue is used for submitting graphics kernel dispatch commands.
    CG_QUEUE_TYPE_TRANSFER             =  3,        /// The queue is used for submitting data transfer commands using a DMA engine.
};

/// @summary Define the supported types of kernel fragments.
enum cg_kernel_type_e : int
{
    CG_KERNEL_TYPE_GRAPHICS_VERTEX     =  1,        /// The kernel corresponds to the vertex shader stage.
    CG_KERNEL_TYPE_GRAPHICS_FRAGMENT   =  2,        /// The kernel corresponds to the fragment shader stage.
    CG_KERNEL_TYPE_GRAPHICS_PRIMITIVE  =  3,        /// The kernel corresponds to the geometry shader stage.
    CG_KERNEL_TYPE_COMPUTE             =  4,        /// The kernel is a compute shader.
};

/// @summary Define the supported types of pipelines.
enum cg_pipeline_type_e : int
{
    CG_PIPELINE_TYPE_GRAPHICS          =  1,        /// The pipeline definition is for graphics shaders.
    CG_PIPELINE_TYPE_COMPUTE           =  2,        /// The pipeline definition is for compute kernels.
};

/// @summary Define the supported types of memory heaps.
enum cg_heap_type_e : int
{
    CG_HEAP_TYPE_LOCAL                 =  1,        /// The heap is in device-local memory.
    CG_HEAP_TYPE_REMOTE                =  2,        /// The heap is in non-local device memory.
    CG_HEAP_TYPE_EMBEDDED              =  3,        /// The heap is in embedded (on-chip) memory.
};

/// @summary Define the fixed-function blending methods in the blending equation.
enum cg_blend_function_e : int
{
    CG_BLEND_FUNCTION_ADD              =  1,        /// The source and destination components are added.
    CG_BLEND_FUNCTION_SUBTRACT         =  2,        /// The destination component is subtracted from the source component.
    CG_BLEND_FUNCTION_REVERSE_SUBTRACT =  3,        /// The source component is subtracted from the destination component.
    CG_BLEND_FUNCTION_MIN              =  4,        /// The minimum of the source and destination components is selected.
    CG_BLEND_FUNCTION_MAX              =  5,        /// The maximum of the source and destination components is selected.
};

/// @summary Define fixed-function methods for computing the source and destination components of the blending equation.
enum cg_blend_factor_e : int
{
    CG_BLEND_FACTOR_ZERO               =  1,        /// The blend factor is set to transparent black.
    CG_BLEND_FACTOR_ONE                =  2,        /// The blend factor is set to opaque white.
    CG_BLEND_FACTOR_SRC_COLOR          =  3,        /// The blend factor is set to the RGB color coming from the fragment shader.
    CG_BLEND_FACTOR_INV_SRC_COLOR      =  4,        /// The blend factor is set to 1.0 minus the RGB color coming from the fragment shader.
    CG_BLEND_FACTOR_DST_COLOR          =  5,        /// The blend factor is set to the RGB color coming from the framebuffer.
    CG_BLEND_FACTOR_INV_DST_COLOR      =  6,        /// The blend factor is set to 1.0 minus the RGB color coming from the framebuffer.
    CG_BLEND_FACTOR_SRC_ALPHA          =  7,        /// The blend factor is set to the alpha value coming from the fragment shader.
    CG_BLEND_FACTOR_INV_SRC_ALPHA      =  8,        /// The blend factor is set to 1.0 minus the alpha value coming from the fragment shader.
    CG_BLEND_FACTOR_DST_ALPHA          =  9,        /// The blend factor is set to the alpha value coming from the framebuffer.
    CG_BLEND_FACTOR_INV_DST_ALPHA      = 10,        /// The blend factor is set to 1.0 minus the alpha value coming from the framebuffer.
    CG_BLEND_FACTOR_CONST_COLOR        = 11,        /// The blend factor is set to the constant RGB value specified in the blend state.
    CG_BLEND_FACTOR_INV_CONST_COLOR    = 12,        /// The blend factor is set to 1.0 minus the constant RGB value specified in the blend state.
    CG_BLEND_FACTOR_CONST_ALPHA        = 13,        /// The blend factor is set to the constant alpha value specified in the blend state.
    CG_BLEND_FACTOR_INV_CONST_ALPHA    = 14,        /// The blend factor is set to 1.0 minus the constant alpha value specified in the blend state.
    CG_BLEND_FACTOR_SRC_ALPHA_SAT      = 15         /// The blend factor is set to the alphva value coming from the fragment shader, clamped to 1.0 minus the alpha value coming from the framebuffer.
};

/// @summary Define fixed-function methods for depth and stencil testing.
enum cg_compare_function_e : int
{
    CG_COMPARE_NEVER                   =  1,        /// The input value never passes the test.
    CG_COMPARE_LESS                    =  2,        /// The input value passes if it is less than the existing value.
    CG_COMPARE_EQUAL                   =  3,        /// The input value passes if it is equal to the existing value.
    CG_COMPARE_LESS_EQUAL              =  4,        /// The input value passes if it is less than or equal to the existing value.
    CG_COMPARE_GREATER                 =  5,        /// The input value passes if it is greater than the existing value.
    CG_COMPARE_NOT_EQUAL               =  6,        /// The input value passes if it is not equal to the existing value.
    CG_COMPARE_GREATER_EQUAL           =  7,        /// The input value passes if it is greater than or equal to the existing value.
    CG_COMPARE_ALWAYS                  =  8,        /// The input value always passes the test.
};

/// @summary Define the supported primitive fill modes.
enum cg_fill_mode_e : int
{
    CG_FILL_SOLID                      =  0,        /// The interior of the primitive is rasterized.
    CG_FILL_WIREFRAME                  =  1,        /// Only the outline of the primitive is rasterized.
};

/// @summary Define which side of a primitive is removed during the culling phase.
enum cg_cull_mode_e : int
{
    CG_CULL_NONE                       =  0,        /// Both front and back-facing primitives are rasterized.
    CG_CULL_FRONT                      =  1,        /// Front-facing primitives are rasterized, back-facing primitives are discarded.
    CG_CULL_BACK                       =  2,        /// Back-facing primitives are rasterized, front-facing primitives are discarded.
};

/// @summary Define the winding order used to identify front-facing primitives.
enum cg_winding_e : int
{
    CG_WINDING_CCW                     =  0,        /// Vertices ordered counter-clockwise define the front face of a primitive.
    CG_WINDING_CW                      =  1,        /// Vertices ordered clockwise define the front face of a primitive.
};

/// @summary Define the supported types of primitives used for rendering geometry.
enum cg_primitive_topology_e : int
{
    CG_PRIMITIVE_POINT_LIST            =  1,        /// Vertices identify individual points.
    CG_PRIMITIVE_LINE_LIST             =  2,        /// Every two vertices defines a discrete line segment.
    CG_PRIMITIVE_LINE_STRIP            =  3,        /// Vertex N+1 defines a line segment linked to vertex N.
    CG_PRIMITIVE_TRIANGLE_LIST         =  4,        /// Every three vertices defines a discrete triangle.
    CG_PRIMITIVE_TRIANGLE_STRIP        =  5,        /// Vertices define a strip of N-2 triangles.
};

/// @summary Define the fixed-function operations to perform when the stencil test passes.
enum cg_stencil_operation_e : int
{
    CG_STENCIL_OP_KEEP                 =  1,        /// Keep the existing stencil value unchanged.
    CG_STENCIL_OP_ZERO                 =  2,        /// Set the stencil value to zero.
    CG_STENCIL_OP_REPLACE              =  3,        /// Set the stencil value to the stencil reference value.
    CG_STENCIL_OP_INC_CLAMP            =  4,        /// Increment the existing stencil value by one and clamp to the maximum.
    CG_STENCIL_OP_DEC_CLAMP            =  5,        /// Decrement the existing stencil value by one and clamp to the minimum.
    CG_STENCIL_OP_INVERT               =  6,        /// Invert the existing stencil value.
    CG_STENCIL_OP_INC_WRAP             =  7,        /// Increment the existing stencil value by one, wrapping to minimum if the result exceeds the maximum value.
    CG_STENCIL_OP_DEC_WRAP             =  8,        /// Decrement the existing stencil value by one, wrapping to maximum if the result exceeds the minimum value.
};

/// @summary Define the possible types of memory object placement. Placement hints help determine where a memory object is allocated.
enum cg_memory_placement_e : int
{
    CG_MEMORY_PLACEMENT_HOST           =  1,        /// The memory object will be primarily accessed by the host.
    CG_MEMORY_PLACEMENT_PINNED         =  2,        /// The memory object will be accessed equally by both host and device.
    CG_MEMORY_PLACEMENT_DEVICE         =  3,        /// The memory object will be primarily accessed by the device.
};

/// @summary Define categories for the update frequency of a memory object.
enum cg_memory_update_frequency_e : int
{
    CG_MEMORY_UPDATE_ONCE              =  1,        /// The memory object will be written once or very infrequently.
    CG_MEMORY_UPDATE_PER_FRAME         =  2,        /// The memory object will be written once per-frame.
    CG_MEMORY_UPDATE_PER_DISPATCH      =  3,        /// The memory object will be written on the order of once per work dispatch.
};

/// @summary Define the supported types of CPU device paritioning.
enum cg_cpu_partition_type_e : int
{
    CG_CPU_PARTITION_NONE              =  0,        /// The CPU device will not be partitioned.
    CG_CPU_PARTITION_PER_CORE          =  1,        /// The CPU device will be partitioned into one sub-device per physical core.
    CG_CPU_PARTITION_PER_NODE          =  2,        /// The CPU device will be partitioned into one sub-device per NUMA node.
    CG_CPU_PARTITION_EXPLICIT          =  3,        /// The CPU device will be partitioned into one or more sub-devices, each with a user-specified number of hardware threads.
};

/// @summary Define flags that can be specified when creating an execution group.
enum cg_execution_group_flags_e : uint32_t
{
    CG_EXECUTION_GROUP_NONE            = (0 << 0),  /// Configure CPUs for data parallel operation, specify devices explicitly.
    CG_EXECUTION_GROUP_CPUS            = (1 << 0),  /// Include all CPUs in the sharegroup of the master device.
    CG_EXECUTION_GROUP_GPUS            = (1 << 1),  /// Include all GPUs in the sharegroup of the master device.
    CG_EXECUTION_GROUP_ACCELERATORS    = (1 << 2),  /// Include all accelerators in the sharegroup of the master device.
    CG_EXECUTION_GROUP_DISPLAY_OUTPUT  = (1 << 3),  /// The execution group will use OpenGL for display output. The RootDevice must specify the GPU attached to the output display(s).
};

/// @summary Define flags that can be specified with kernel code.
enum cg_kernel_flags_e : uint32_t
{
    CG_KERNEL_FLAGS_NONE               = (0 << 0),  /// No flag bits are set.
    CG_KERNEL_FLAGS_SOURCE             = (1 << 0),  /// Code is supplied as text source code.
    CG_KERNEL_FLAGS_BINARY             = (1 << 1),  /// Code is supplied as a shader IL blob.
};

/// @summary Define flags specifying attributes of memory heaps.
enum cg_heap_flags_e : uint32_t
{
    CG_HEAP_CPU_ACCESSIBLE             = (1 << 0),  /// The CPU can directly access allocations from the heap.
    CG_HEAP_GPU_ACCESSIBLE             = (1 << 1),  /// The GPU can directly access allocations from the heap.
    CG_HEAP_HOLDS_PINNED               = (1 << 2),  /// The heap allocates from the non-paged pool accessible to both CPU and GPU.
    CG_HEAP_SHAREABLE                  = (1 << 3),  /// Memory objects can be shared between GPUs.
};

/// @summary Define the types of kernels that will access a given memory object.
enum cg_memory_object_kernel_e : uint32_t
{
    CG_MEMORY_OBJECT_KERNEL_COMPUTE    = (1 << 0),  /// The memory object will be used with compute kernels.
    CG_MEMORY_OBJECT_KERNEL_GRAPHICS   = (1 << 1),  /// The memory object will be used with graphics kernels.
};

/// @summary Define the flags specifying how data will be accessed.
enum cg_memory_access_flags_e : uint32_t
{
    CG_MEMORY_ACCESS_NONE              = (0 << 0),  /// The memory object will not be read or written.
    CG_MEMORY_ACCESS_READ              = (1 << 0),  /// The memory object will be read.
    CG_MEMORY_ACCESS_WRITE             = (1 << 1),  /// The memory object will be written.
    CG_MEMORY_ACCESS_PRESERVE          = (1 << 2),  /// The memory contents should be preserved.
    CG_MEMORY_ACCESS_READ_WRITE        =            /// The memory object will be both read and written.
        CG_MEMORY_ACCESS_READ          | 
        CG_MEMORY_ACCESS_WRITE
};

/// @summary Define flags used to specify whether an event will be used for compute, graphics or both.
enum cg_event_usage_e : uint32_t
{
    CG_EVENT_USAGE_COMPUTE             = (1 << 0),  /// The event object will be used to synchronize compute operations.
    CG_EVENT_USAGE_GRAPHICS            = (1 << 1),  /// The event object will be used to synchronize graphics operations.
};

/// @summary Data used to describe the application to the system. Strings are NULL-terminated, ASCII only.
struct cg_application_info_t
{
    char const                   *AppName;          /// The name of the application.
    char const                   *DriverName;       /// The name of the application driver.
    uint32_t                      ApiVersion;       /// The version of the CGFX API being requested.
    uint32_t                      AppVersion;       /// The version of the application.
    uint32_t                      DriverVersion;    /// The version of the application driver.
};

/// @summary Function pointers and application state used to supply a custom memory allocator for internal data.
struct cg_allocation_callbacks_t
{
    cgMemoryAlloc_fn              Allocate;         /// Called when CGFX needs to allocate memory.
    cgMemoryFree_fn               Release;          /// Called when CGFX wants to return memory.
    uintptr_t                     UserData;         /// Opaque data to be passed to the callbacks. May be 0.
};

/// @summary Data used to describe how the CPU device should be configured for kernel execution.
struct cg_cpu_partition_t
{
    int                           PartitionType;    /// The CPU device partition type, one of cg_cpu_partition_type_e.
    size_t                        ReserveThreads;   /// The number of hardware threads to reserve for use by the application. Kernels will not be executed on reserved threads.
    size_t                        PartitionCount;   /// The number of explicit partitions, if PartitionType is CG_CPU_PARTITION_EXPLICIT. Otherwise, set to zero.
    int                          *ThreadCounts;     /// An array of PartitionCount items, each specifying the number of hardware threads to assign to the corresponding partition.
};

/// @summary Define the data used to create an execution group.
struct cg_execution_group_t
{
    cg_handle_t                   RootDevice;       /// The handle of the root device used to determine the share group.
    size_t                        DeviceCount;      /// The number of explicitly-specified devices in the execution group.
    cg_handle_t                  *DeviceList;       /// An array of DeviceCount handles of the explicitly-specified devices in the execution group.
    size_t                        ExtensionCount;   /// The number of extensions to enable.
    char const                  **ExtensionNames;   /// An array of ExtensionCount NULL-terminated ASCII strings naming the extensions to enable.
    uint32_t                      CreateFlags;      /// A combination of cg_extension_group_flags_e specifying group creation flags.
    int                           ValidationLevel;  /// One of cg_validation_level_e specifying the validation level to enable.
};

/// @summary Define the data associated with a reference to a memory object. 
/// A complete list of referenced memory objects is required when a command buffer is submitted for execution.
struct cg_memory_ref_t
{
    cg_handle_t                   Memory;           /// The handle of the referenced memory object.
    uint32_t                      Flags;            /// A combination of cg_memory_ref_flags_e specifying how the memory object is used. 
};

/// @summary Define the data returned when querying for the CPU resources available in the system.
struct cg_cpu_info_t
{
    size_t                        NUMANodes;        /// The number of NUMA nodes in the system.
    size_t                        PhysicalCPUs;     /// The number of physical CPU packages in the system.
    size_t                        PhysicalCores;    /// The number of physical CPU cores in the system.
    size_t                        HardwareThreads;  /// The number of hardware threads in the system.
    size_t                        ThreadsPerCore;   /// The number of hardware threads per CPU core.
    char                          VendorName[13];   /// The CPU vendor string returned by executing CPUID with EAX=0.
    char                          PreferAMD;        /// true if the AMD OpenCL platform is preferred.
    char                          PreferIntel;      /// true if the Intel OpenCL platform is preferred.
    char                          IsVirtualMachine; /// Is the application running in a virtualized environment?
};

/// @summary Define basic properties of a memory heap.
struct cg_heap_info_t
{
    size_t                        Ordinal;          /// The heap ordinal identifier.
    int                           Type;             /// One of cg_heap_type_e specifying the type of video memory.
    uint32_t                      Flags;            /// A combination of cg_heap_flags_e specifying heap attributes.
    uint64_t                      HeapSize;         /// The total size of the heap memory, in bytes.
    uint64_t                      PinnableSize;     /// The total number of bytes of heap memory that can be pinned.
    size_t                        DeviceAlignment;  /// The address alignment of any buffer allocated on the heap.
    size_t                        UserAlignment;    /// The address alignment of any user-allocated memory shared with the heap.
    size_t                        UserSizeAlign;    /// The allocation size multiple for user-allocated memory.
};

/// @summary Define the basic in-memory format of a single command in a command buffer.
struct cg_command_t
{
    uint16_t                      CommandId;        /// The unique identifier of the command.
    uint16_t                      DataSize;         /// The size of the buffer pointed to by Data.
    uint8_t                       Data[1];          /// Variable-length data, up to 64KB in size.
};

/// @summary Describes a bit of kernel (device-executable) code. 
struct cg_kernel_code_t
{
    int                           Type;             /// One of cg_kernel_type_e specifying the type of kernel.
    uint32_t                      Flags;            /// A combination of cg_kernel_flags_e.
    void const                   *Code;             /// The buffer specifying the kernel code.
    size_t                        CodeSize;         /// The size of the code buffer, in bytes.
};

/// @summary Describes fixed-function state configuration for the blending unit.
struct cg_blend_state_t
{
    bool                         BlendEnabled;         /// Specify true if alpha blending is enabled.
    int                          SrcBlendColor;        /// The source component of the blending equation for color channels, one of cg_blend_factor_e.
    int                          DstBlendColor;        /// The destination component of the blending equation for color channels, one of cg_blend_factor_e.
    int                          ColorBlendFunction;   /// The blending function to use for the color channels, one of cg_blend_function_e.
    int                          SrcBlendAlpha;        /// The source component of the blending equation for alpha, one of cg_blend_factor_e.
    int                          DstBlendAlpha;        /// The destination component of the blending equation for alpha, one of cg_blend_factor_e.
    int                          AlphaBlendFunction;   /// The blending function to use for the alpha channel, one of cg_blend_function_e.
    float                        ConstantRGBA[4];      /// RGBA values in [0, 1] specifying a constant blend color.
};

/// @summary Describes fixed-function state configuration for the rasterizer.
struct cg_raster_state_t
{
    int                          FillMode;             /// The primitive fill mode, one of cg_fill_mode_e.
    int                          CullMode;             /// The primitive culling mode, one of cg_cull_mode_e.
    int                          FrontFace;            /// The winding order used to determine front-facing primitives, one of cg_winding_t.
    int                          DepthBias;            /// The depth bias value added to fragment depth.
    float                        SlopeScaledDepthBias; /// The scale of the slope-based value added to fragment depth.
};

/// @summary Describes fixed-function state configuration for depth and stencil testing.
struct cg_depth_stencil_state_t
{
    bool                         DepthTestEnable;      /// Specify true if depth testing is enabled.
    bool                         DepthWriteEnable;     /// Specify true if depth buffer writes are enabled.
    bool                         DepthBoundsEnable;    /// Specify true if the depth buffer range is enabled.
    int                          DepthTestFunction;    /// The depth value comparison function, one of cg_compare_function_e.
    float                        DepthMin;             /// The minimum depth buffer value.
    float                        DepthMax;             /// The maximum depth buffer value.
    bool                         StencilTestEnable;    /// Specify true if stencil testing is enabled.
    int                          StencilTestFunction;  /// The stencil value comparison function, one of cg_compare_function_e.
    int                          StencilFailOp;        /// The stencil operation to apply when the stencil test fails, one of cg_stencil_operation_e.
    int                          StencilPassZPassOp;   /// The stencil operation to apply when both the stencil and depth tests pass, one of cg_stencil_operation_e.
    int                          StencilPassZFailOp;   /// The stencil operation to apply when the stencil test passes and the depth test fails, one of cg_stencil_operation_e.
    uint8_t                      StencilReadMask;      /// The bitmask to apply to stencil reads.
    uint8_t                      StencilWriteMask;     /// The bitmask to apply to stencil writes.
    uint8_t                      StencilReference;     /// The stencil reference value.
};

/// @summary Describes a binding of a named vertex attribute or fragment output to a register index.
struct cg_shader_binding_t
{
    char const                  *Name;                 /// A NULL-terminated ASCII string specifying the vertex attribute or fragment output name as it appears in the shader.
    unsigned int                 Location;             /// The unique zero-based register index to bind to the named vertex attribute or fragment output.
};

/// @summary Defines the configuration for a graphics pipeline.
struct cg_graphics_pipeline_t
{
    cg_depth_stencil_state_t     DepthStencilState;    /// The fixed-function configuration for depth and stencil testing.
    cg_raster_state_t            RasterizerState;      /// The fixed-function configuration for primitive rasterization.
    cg_blend_state_t             BlendState;           /// The fixed-function configuration for the blending unit.
    int                          PrimitiveType;        /// The type of geometry submitted for draw calls against the pipeline.
    cg_handle_t                  VertexShader;         /// The handle of the vertex shader kernel.
    cg_handle_t                  GeometryShader;       /// The handle of the geometry shader kernel, or CG_INVALID_HANDLE.
    cg_handle_t                  FragmentShader;       /// The handle of the fragment shader kernel.
    size_t                       AttributeCount;       /// The number of vertex attribute locations to set explicitly.
    cg_shader_binding_t const   *AttributeBindings;    /// An array of AttributeCount vertex attribute bindings.
    size_t                       OutputCount;          /// The number of fragment output locations to set explicitly.
    cg_shader_binding_t const   *OutputBindings;       /// An array of OutputCount fragment output bindings.
};

/// @summary Defines the configuration for a compute pipeline.
struct cg_compute_pipeline_t
{
    char const                  *KernelName;           /// A NULL-terminated ASCII string specifying the name of the kernel function in the program code.
    cg_handle_t                  KernelProgram;        /// The handle of the kernel code to execute.
};

/*/////////////////
//   Functions   //
/////////////////*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

char const*
cgResultString                                      /// Convert a result code into a string.
(
    int                           result            /// One of cg_result_e.
);

int
cgGetCpuInfo                                        /// Retrieve information about the CPUs installed in the local system.
(
    cg_cpu_info_t                *cpu_info          /// On return, stores information about the CPUs in the system.
);

int
cgDefaultCpuPartition                               /// Retrieve the default CPU partition layout.
(
    cg_cpu_partition_t           *cpu_partition     /// The CPU partition layout to initialize.
);

int
cgValidateCpuPartition                              /// Perform basic validate of a CPU partition definition.
(
    cg_cpu_partition_t const     *cpu_partition,    /// The CPU partition layout to validate.
    cg_cpu_info_t      const     *cpu_info          /// The CPU information to validate against.
);

int
cgEnumerateDevices                                  /// Enumerate all devices and displays installed in the system.
(
    cg_application_info_t const  *app_info,         /// Information describing the calling application.
    cg_allocation_callbacks_t    *alloc_cb,         /// Application-defined memory allocation callbacks, or NULL.
    cg_cpu_partition_t const     *cpu_partition,    /// Information about how CPU devices should be exposed.
    size_t                       &device_count,     /// On return, store the number of devices in the system.
    cg_handle_t                  *device_list,      /// On return, populated with a list of device handles.
    size_t                        max_devices,      /// The maximum number of device handles to write to device_list.
    uintptr_t                    &context           /// If 0, on return, store the handle of a new CGFX context; otherwise, specifies an existing CGFX context.
);

int
cgDestroyContext                                    /// Free all resources associated with a CGFX context.
(
    uintptr_t                     context           /// A CGFX context returned by cgEnumerateDevices.
);

int
cgGetContextInfo                                    /// Retrieve context-level capabilities or data.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    int                           param,            /// One of cg_context_info_param_e.
    void                         *data,             /// Buffer to receive the data.
    size_t                        buffer_size,      /// The maximum number of bytes to write to the data buffer.
    size_t                       *bytes_needed      /// On return, if non-NULL, store the number of bytes required to receive the data.
);

size_t
cgGetHeapCount                                      /// Retrieve the number of memory heaps available to compute devices.
(
    uintptr_t                     context           /// A CGFX context returned by cgEnumerateDevices.
);

int
cgGetHeapProperties                                 /// Query the attributes of a heap memory.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    size_t                        ordinal,          /// The ordinal number of the heap to query, in [0, cgGetHeapCount(context)).
    cg_heap_info_t               &heap_info         /// On return, stores attributes of the specified heap.
);

size_t
cgGetDeviceCount                                    /// Retrieve the number of available compute devices.
(
    uintptr_t                     context           /// A CGFX context returned by cgEnumerateDevices.
);

int
cgGetDeviceInfo                                     /// Retrieve compute device properties.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   device,           /// The handle of the device to query.
    int                           param,            /// One of cg_device_info_param_e.
    void                         *data,             /// Buffer to receive the data.
    size_t                        buffer_size,      /// The maximum number of bytes to write to the data buffer.
    size_t                       *bytes_needed      /// On return, if non-NULL, store the number of bytes required to receive the data.
);

size_t
cgGetDisplayCount                                   /// Retrieve the number of attached displays.
(
    uintptr_t                     context           /// A CGFX context returned by cgEnumerateDevices.
);

cg_handle_t
cgGetPrimaryDisplay                                 /// Retrieve the handle of the primary display.
(
    uintptr_t                     context           /// A CGFX context returned by cgEnumerateDevices.
);

cg_handle_t
cgGetDisplayByOrdinal                               /// Retrieve the handle of a display identified by ordinal value.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    size_t                        ordinal           /// The zero-based display index.
);

int
cgGetDisplayInfo                                    /// Retrieve display properties.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   display,          /// The handle of the display to query.
    int                           param,            /// One of cg_display_info_param_e.
    void                         *data,             /// Buffer to receive the data.
    size_t                        buffer_size,      /// The maximum number of bytes to write to the data buffer.
    size_t                       *bytes_needed      /// On return, if non-NULL, store the number of bytes required to receive the data.
);

cg_handle_t
cgGetDisplayDevice                                  /// Retrieve the handle of the GPU devices that drives a given display.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   display           /// The handle of the display to query.
);

int
cgGetCPUDevices                                     /// Retrieve the count of and handles to all CPU devices in the system.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    size_t                       &cpu_count,        /// On return, stores the number of CPU devices in the system.
    size_t const                  max_devices,      /// The maximum number of device handles to write to cpu_devices.
    cg_handle_t                  *cpu_devices       /// On return, if non-NULL, stores min(max_devices, cpu_count) handles of CPU devices.
);

int
cgGetGPUDevices                                     /// Retrieve the count of and handles to all GPU devices in the system.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    size_t                       &gpu_count,        /// On return, stores the number of GPU devices in the system.
    size_t const                  max_devices,      /// The maximum number of device handles to write to gpu_devices.
    cg_handle_t                  *gpu_devices       /// On return, if non-NULL, stores min(max_devices, gpu_count) handles of GPU devices.
);

int
cgGetAcceleratorDevices                             /// Retrieve the count of and handles to all accelerator devices in the system.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    size_t                       &acl_count,        /// On return, stores the number of accelerator devices in the system.
    size_t const                  max_devices,      /// The maximum number of device handles to write to acl_devices.
    cg_handle_t                  *acl_devices       /// On return, if non-NULL, stores min(max_devices, acl_count) handles of accelerator devices.
);

int
cgGetCPUDevicesInShareGroup                         /// Retrieve the count of and handles to CPU devices that can share resources with a given device.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   device,           /// The handle of a device in the sharegroup.
    size_t                       &cpu_count,        /// On return, stores the number of CPU devices in the sharegroup.
    size_t const                  max_devices,      /// The maximum number of device handles to write to cpu_devices.
    cg_handle_t                  *cpu_devices       /// On return, if non-NULL, stores min(max_devices, cpu_count) handles of devices in the share group.
);

int
cgGetGPUDevicesInShareGroup                         /// Retrieve the count of and handles to GPU devices that can share resources with a given device.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   device,           /// The handle of a device in the sharegroup.
    size_t                       &gpu_count,        /// On return, stores the number of GPU devices in the sharegroup.
    size_t const                  max_devices,      /// The maximum number of device handles to write to gpu_devices.
    cg_handle_t                  *gpu_devices       /// On return, if non-NULL, stores min(max_devices, gpu_count) handles of devices in the share group.
);

cg_handle_t
cgCreateExecutionGroup                              /// Create an execution group consisting of one or more devices.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_execution_group_t const   *create_info,      /// Information describing the share group configuration.
    int                          &result            /// On return, set to CG_SUCCESS or another result code.
);

int
cgGetExecutionGroupInfo                             /// Retrieve execution group properties.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   group,            /// The handle of the execution group to query.
    int                           param,            /// One of cg_execution_group_info_param_e.
    void                         *data,             /// Buffer to receive the data.
    size_t                        buffer_size,      /// The maximum number of bytes to write to the data buffer.
    size_t                       *bytes_needed      /// On return, if non-NULL, store the number of bytes required to receive the data.
);

cg_handle_t
cgGetQueueForDevice                                 /// Retrieve the queue associated with a particular device.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   device,           /// The handle of the device to query.
    int                           queue_type,       /// The type of queue to retrieve, one of cg_queue_type_e.
    int                          &result            /// On return, set to CG_SUCCESS or another result code.
);

cg_handle_t
cgGetQueueForDisplay                                /// Retrieve the queue associated with a particular display.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   display,          /// The handle of the display to query.
    int                           queue_type,       /// The type of queue to retrieve, one of cg_queue_type_e.
    int                          &result            /// On return, set to CG_SUCCESS or another result code.
);

int
cgDeleteObject                                      /// Frees resources for an object and invalidates the object handle.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   object            /// The handle of the object to delete.
);

cg_handle_t
cgCreateCommandBuffer                               /// Create a new command buffer object.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    int                           queue_type,       /// The type of queue the command buffer will be submitted to.
    int                          &result            /// On return, set to CG_SUCCESS or another result code.
);

int
cgBeginCommandBuffer                                /// Begins command buffer construction and places the command buffer into the building state.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   cmd_buffer,       /// The command buffer handle.
    uint32_t                      flags             /// Flags used to optimize command buffer submission.
);

int
cgResetCommandBuffer                                /// Explicitly resets a command buffer and releases any resources associated with it.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   cmd_buffer        /// The command buffer handle.
);

int
cgCommandBufferAppend                               /// Append a fixed-length command to a command buffer.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   cmd_buffer,       /// The command buffer handle.
    uint16_t                      cmd_type,         /// The type identifier of the command being written.
    size_t                        data_size,        /// The size of the command data, in bytes. 64KB maximum.
    void const                   *cmd_data          /// The command data to be copied to the command buffer.
);

int
cgCommandBufferMapAppend                            /// Begin appending a variable-length command to a command buffer.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   cmd_buffer,       /// The command buffer handle.
    size_t                        reserve_size,     /// The maximum number of data bytes that will be written to the command buffer.
    cg_command_t                **command           /// On return, points to the command descriptor to populate.
);

int
cgCommandBufferUnmapAppend                          /// Finish appending variable-length data to a command buffer.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   cmd_buffer,       /// The command buffer handle.
    size_t                        data_written      /// The number of data bytes actually written.
);

int
cgEndCommandBuffer                                  /// Completes recording of a command buffer in the building state.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   cmd_buffer        /// The command buffer handle.
);

int
cgCommandBufferCanRead                              /// Determine whether a command buffer can be read from.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   cmd_buffer,       /// The command buffer handle.
    size_t                       &bytes_total       /// The number of bytes of data in the command buffer.
);

cg_command_t*
cgCommandBufferCommandAt                            /// Read a command buffer at a given byte offset. Only use this function is cgCommandBufferCanRead returns CG_SUCCESS.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   cmd_buffer,       /// The command buffer handle.
    size_t                       &cmd_offset,       /// The byte offset of the command to map for reading. On return, updated to point to the start of the next command.
    int                          &result            /// On return, set to CG_SUCCESS or another result code.
);

cg_handle_t
cgCreateEvent                                       /// Create a new event object in the unsignaled state.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   exec_group,       /// The handle of the execution group that will signal the event.
    uint32_t                      usage,            /// One or more of cg_event_usage_e specifying whether graphics, compute, or both will use the event.
    int                          &result            /// On return, set to CG_SUCCESS or another result code.
);

int
cgHostWaitForEvent                                  /// Blocks the calling thread until the device signals an event.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   wait_event        /// The handle of the event to wait on.
);

cg_handle_t
cgCreateKernel                                      /// Create a new kernel object from some device-executable code.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   exec_group,       /// The handle of the execution group for which the kernel will be compiled.
    cg_kernel_code_t const       *create_info,      /// An object specifying source code and behavior.
    int                          &result            /// On return, set to CG_SUCCESS or another result code.
);

cg_handle_t
cgCreateComputePipeline                             /// Create a new compute pipeline object.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   exec_group,       /// The handle of the execution group that will execute the pipeline.
    cg_compute_pipeline_t const  *create_info,      /// A description of the compute pipeline to create.
    int                          &result            /// On return, set to CG_SUCCESS or another value.
);

cg_handle_t
cgCreateGraphicsPipeline                            /// Create a new graphics pipeline object.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   exec_group,       /// The handle of the execution group that will execute the pipeline.
    cg_graphics_pipeline_t const *create_info,      /// A description of the graphics pipeline to create.
    int                          &result            /// On return, set to CG_SUCCESS, CG_UNSUPPORTED or another value.
);

cg_handle_t
cgCreateDataBuffer                                  /// Create a new data buffer object.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   exec_group,       /// The handle of the execution group defining the devices that will operate on the buffer.
    size_t                        buffer_size,      /// The desired size of the data buffer, in bytes.
    uint32_t                      kernel_types,     /// One or more of cg_memory_object_kernel_e specifying the type(s) of kernels requiring access to the buffer.
    uint32_t                      kernel_access,    /// One or more of cg_memory_object_access_e specifying how kernels will access the memory.
    uint32_t                      host_access,      /// One or more of cg_memory_object_access_e specifying how the host will access the memory.
    int                           placement_hint,   /// One of cg_memory_placement_e specifying the placement preference for the memory.
    int                           frequency_hint,   /// One of cg_memory_update_frequency_e specifying how often the memory contents will be updated.
    int                          &result            /// On return, set to CG_SUCCESS or another result code.
);

int
cgGetDataBufferInfo                                 /// Retrieve data buffer properties.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   buffer,           /// The handle of the data buffer to query.
    int                           param,            /// One of cg_data_buffer_info_param_e.
    void                         *data,             /// Buffer to receive the data.
    size_t                        buffer_size,      /// The maximum number of bytes to write to the data buffer.
    size_t                       *bytes_needed      /// On return, if non-NULL, store the number of bytes required to receive the data.
);

void*
cgMapDataBuffer                                     /// Map a portion of a data buffer into the host address space. This may initiate a blocking device to host data transfer operation.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   queue,            /// The handle of the transfer or display queue.
    cg_handle_t                   buffer,           /// The handle of the buffer to map.
    size_t                        offset,           /// The byte offset of the start of the buffer range to map.
    size_t                        amount,           /// The number of bytes of the buffer that will be accessed.
    uint32_t                      flags,            /// A combination of cg_memory_access_flags_e specifying how the buffer will be accessed.
    int                          &result            /// On return, set to CG_SUCCESS or another result code.
);

int
cgUnmapDataBuffer                                   /// Unmap a portion of a data buffer from the host address space. This may initiate a non-blocking host to device data transfer operation.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   queue,            /// The handle of the transfer or display queue passed to cgMapDataBuffer.
    cg_handle_t                   buffer,           /// The handle of the buffer to unmap.
    void                         *mapped_region,    /// The pointer to the mapped region returned by cgMapDataBuffer.
    cg_handle_t                  *event_handle      /// On return, if not NULL, stores the handle to an event signaled when the transfer is complete.
);

int
cgBlendStateInitNone                                /// Initialize fixed-function alpha blending configuration for no blending.
(
    cg_blend_state_t             &state             /// The blending state to update.
);

int
cgBlendStateInitAlpha                               /// Initialize fixed-function alpha blending configuration for standard texture transparency.
(
    cg_blend_state_t             &state             /// The blending state to update.
);

int
cgBlendStateInitAdditive                            /// Initialize fixed-function alpha blending configuration for additive blending.
(
    cg_blend_state_t             &state             /// The blending state to update.
);

int 
cgBlendStateInitPremultiplied                       /// Initialize fixed-function alpha blending configuration for blending with premultiplied alpha in the source texture.
(
    cg_blend_state_t             &state             /// The blending state to update.
);

int
cgRasterStateInitDefault                            /// Initialize fixed-function rasterizer configuration to the default values.
(
    cg_raster_state_t            &state             /// The rasterizer state to update.
);

int
cgDepthStencilStateInitDefault                      /// Initialize fixed-function depth and stencil testing configuration to the default values.
(
    cg_depth_stencil_state_t     &state             /// The depth and stencil testing configuration to update.
);

#ifdef __cplusplus
};     /* extern "C"  */
#endif /* __cplusplus */

#undef  CGFX_INTERFACE_DEFINED
#define CGFX_INTERFACE_DEFINED
#endif /* !defined(LIB_CGFX_H) */
