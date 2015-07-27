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
struct cg_cpu_counts_t;
struct cg_command_t;
struct cg_kernel_code_t;

/*/////////////////
//   Constants   //
/////////////////*/
/// @summary A special value representing an invalid handle.
#define CG_INVALID_HANDLE   ((cg_handle_t)0)

/*////////////////////////////
//  Function Pointer Types  //
////////////////////////////*/
typedef void*        (CG_API *cgMemoryAlloc_fn               )(size_t, size_t, int, uintptr_t);
typedef void         (CG_API *cgMemoryFree_fn                )(void *, size_t, size_t, int, uintptr_t);
typedef char const*  (CG_API *cgResultString_fn              )(int);
typedef int          (CG_API *cgEnumerateDevices_fn          )(cg_application_info_t const*, cg_allocation_callbacks_t *, size_t&, cg_handle_t *, size_t, uintptr_t &);
typedef int          (CG_API *cgGetContextInfo_fn            )(uintptr_t, int, void *, size_t, size_t *);
typedef size_t       (CG_API *cgGetDeviceCount_fn            )(uintptr_t);
typedef int          (CG_API *cgGetDeviceInfo_fn             )(uintptr_t, cg_handle_t, int, void *, size_t, size_t *);
typedef size_t       (CG_API *cgGetDisplayCount_fn           )(uintptr_t);
typedef cg_handle_t  (CG_API *cgGetPrimaryDisplay_fn         )(uintptr_t);
typedef cg_handle_t  (CG_API *cgGetDisplayByOrdinal_fn       )(uintptr_t);
typedef int          (CG_API *cgGetDisplayInfo_fn            )(uintptr_t, cg_handle_t, int, void *, size_t, size_t *);
typedef cg_handle_t  (CG_API *cgGetDisplayDevice_fn          )(uintptr_t, cg_handle_t);
typedef int          (CG_API *cgGetCPUDevices_fn             )(uintptr_t, size_t &, size_t const, cg_handle_t *);
typedef int          (CG_API *cgGetGPUDevices_fn             )(uintptr_t, size_t &, size_t const, cg_handle_t *);
typedef int          (CG_API *cgGetAcceleratorDevices_fn     )(uintptr_t, size_t &, size_t const, cg_handle_t *);
typedef int          (CG_API *cgGetCPUDevicesInShareGroup_fn )(uintptr_t, cg_handle_t, size_t &, size_t const, cg_handle_t *);
typedef int          (CG_API *cgGetGPUDevicesInShareGroup_fn )(uintptr_t, cg_handle_t, size_t &, size_t const, cg_handle_t *);
typedef cg_handle_t  (CG_API *cgCreateExecutionGroup_fn      )(uintptr_t, cg_execution_group_t const *, int &);
typedef int          (CG_API *cgGetExecutionGroupInfo_fn     )(uintptr_t, cg_handle_t, int, void *, size_t, size_t *);
typedef cg_handle_t  (CG_API *cgGetQueueForDevice_fn         )(uintptr_t, cg_handle_t, int, int &);
typedef cg_handle_t  (CG_API *cgGetQueueForDisplay_fn        )(uintptr_t, cg_handle_t, int, int &);
typedef int          (CG_API *cgDeleteObject_fn              )(uintptr_t, cg_handle_t);
typedef cg_handle_t  (CG_API *cgCreateCommandBuffer_fn       )(uintptr_t, int, int &);
typedef int          (CG_API *cgBeginCommandBuffer_fn        )(uintptr_t, cg_handle_t, uint32_t);
typedef int          (CG_API *cgResetCommandBuffer_fn        )(uintptr_t, cg_handle_t);
typedef int          (CG_API *cgCommandBufferAppend_fn       )(uintptr_t, cg_handle_t, uint16_t, size_t, void const*);
typedef int          (CG_API *cgCommandBufferMapAppend_fn    )(uintptr_t, cg_handle_t, size_t, cg_command_t **);
typedef int          (CG_API *cgCommandBufferUnmapAppend_fn  )(uintptr_t, cg_handle_t, size_t);
typedef int          (CG_API *cgEndCommandBuffer_fn          )(uintptr_t, cg_handle_t);
typedef int          (CG_API *cgCommandBufferCanRead_fn      )(uintptr_t, cg_handle_t, size_t &);
typedef cg_command_t*(CG_API *cgCommandBufferCommandAt_fn    )(uintptr_t, cg_handle_t, size_t &, int &);
typedef cg_handle_t  (CG_API *cgCreateKernel_fn              )(uintptr_t, cg_handle_t, cg_kernel_code_t const *, int &);

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
    CG_ALLOCATION_TYPE_OBJECT          = 1,         /// Allocation used for an API object or data with API lifetime.
    CG_ALLOCATION_TYPE_INTERNAL        = 2,         /// Allocation used for long-lived internal data.
    CG_ALLOCATION_TYPE_TEMP            = 3,         /// Allocation used for short-lived internal data.
    CG_ALLOCATION_TYPE_KERNEL          = 4,         /// Allocation used for short-lived kernel compilation.
    CG_ALLOCATION_TYPE_DEBUG           = 5          /// Allocation used for debug data.
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
    CG_OBJECT_MEMORY                   = (1 <<  5), /// The object type identifier for a generic memory object.
    CG_OBJECT_FENCE                    = (1 <<  6), /// The object type identifier for a fence.
    CG_OBJECT_EVENT                    = (1 <<  7), /// The object type identifier for a waitable event.
    CG_OBJECT_QUERY_POOL               = (1 <<  8), /// The object type identifier for a query pool.
    CG_OBJECT_IMAGE                    = (1 <<  9), /// The object type identifier for an image.
    CG_OBJECT_KERNEL                   = (1 << 10), /// The object type identifier for a compute or shader kernel.
    CG_OBJECT_PIPELINE                 = (1 << 11), /// The object type identifier for a compute or display pipeline.
    CG_OBJECT_SAMPLER                  = (1 << 12), /// The object type identifier for an image sampler.
};

/// @summary Define the queryable or settable data on a CGFX context.
enum cg_context_info_param_e : int
{
    CG_CONTEXT_CPU_COUNTS              = 0,         /// Retrieve the number of CPU resources in the system. Data is cg_cpu_counts_t.
    CG_CONTEXT_DEVICE_COUNT            = 1,         /// Retrieve the number of capable compute devices in the system. Data is size_t.
    CG_CONTEXT_DISPLAY_COUNT           = 2,         /// Retrieve the number of capable display devices attached to the system. Data is size_t. 
};

/// @summary Define the queryable or settable data on a CGFX device.
enum cg_device_info_param_e : int
{
    CG_DEVICE_CL_PLATFORM_ID           = 0,         /// Retrieve the OpenCL platform ID of the device. Data is cl_platform_id.
    CG_DEVICE_CL_DEVICE_ID             = 1,         /// Retrieve the OpenCL device ID of the device. Data is cl_device_id.
    CG_DEVICE_CL_DEVICE_TYPE           = 2,         /// Retrieve the OpenCL device type of the device. Data is cl_device_type.
    CG_DEVICE_WINDOWS_HGLRC            = 3,         /// Retrieve the OpenGL rendering context of the device. Windows-only. Data is HGLRC.
    CG_DEVICE_DISPLAY_COUNT            = 4,         /// Retrieve the number of displays attached to the device. Data is size_t.
    CG_DEVICE_ATTACHED_DISPLAYS        = 5,         /// Retrieve the object handles of the attached displays. Data is cg_handle_t[CG_DEVICE_DISPLAY_COUNT].
    CG_DEVICE_PRIMARY_DISPLAY          = 6,         /// Retrieve the primary display attached to the device. Data is cg_handle_t.
};

/// @summary Define the queryable or settable data on a CGFX display.
enum cg_display_info_param_e : int
{
    CG_DISPLAY_DEVICE                  = 0,         /// Retrieve the CGFX handle of the compute device driving the display. Data is cg_handle_t.
    CG_DISPLAY_CL_PLATFORM_ID          = 1,         /// Retrieve the OpenCL platform ID of the device driving the display. Data is cl_platform_id.
    CG_DISPLAY_CL_DEVICE_ID            = 2,         /// Retrieve the OpenCL device ID of the device driving the display. Data is cl_platform_id.
    CG_DISPLAY_WINDOWS_HDC             = 3,         /// Retrieve the Windows device context of the display. Data is HDC.
    CG_DISPLAY_WINDOWS_HGLRC           = 4,         /// Retrieve the Windows OpenGL rendering context of the device. Data is HGLRC.
    CG_DISPLAY_POSITION                = 5,         /// Retrieve the x and y-coordinate of the upper-left corner of the display in global display coordinates. Data is int[2].
    CG_DISPLAY_SIZE                    = 6,         /// Retrieve the width and height of the display, in pixels. Data is size_t[2].
    CG_DISPLAY_ORIENTATION             = 7,         /// Retrieve the current orientation of the display. Data is cg_display_orientation_e.
    CG_DISPLAY_REFRESH_RATE            = 8,         /// Retrieve the vertical refresh rate of the display, in hertz. Data is float.
};

/// @summary Define the queryable or settable data on a CGFX execution group.
enum cg_execution_group_info_param_e : int
{
    CG_EXEC_GROUP_DEVICE_COUNT         = 0,         /// Retrieve the number of devices in the execution group. Data is size_t.
    CG_EXEC_GROUP_DEVICES              = 1,         /// Retrieve the handles of all devices in the execution group. Data is cg_handle_t[CG_EXEC_GROUP_DEVICE_COUNT].
    CG_EXEC_GROUP_CPU_COUNT            = 2,         /// Retrieve the number of CPU devices in the execution group. Data is size_t.
    CG_EXEC_GROUP_CPU_DEVICES          = 3,         /// Retrieve the handles of all CPU devices in the execution group. Data is cg_handle_t[CG_EXEC_GROUP_CPU_COUNT].
    CG_EXEC_GROUP_GPU_COUNT            = 4,         /// Retrieve the number of GPU devices in the execution group. Data is size_t.
    CG_EXEC_GROUP_GPU_DEVICES          = 5,         /// Retrieve the handles of all GPU devices in the execution group. Data is cg_handle_t[CG_EXEC_GROUP_GPU_COUNT].
    CG_EXEC_GROUP_ACCELERATOR_COUNT    = 6,         /// Retrieve the number of accelerator devices in the execution group. Data is size_t.
    CG_EXEC_GROUP_ACCELERATOR_DEVICES  = 7,         /// Retrieve the handles of all accelerator devices in the execution group. Data is cg_handle_t[CG_EXEC_GROUP_ACCELERATOR_COUNT].
    CG_EXEC_GROUP_DISPLAY_COUNT        = 8,         /// Retrieve the number of displays attached to the execution group. Data is size_t.
    CG_EXEC_GROUP_ATTACHED_DISPLAYS    = 9,         /// Retrieve the handles of all attached displays in the execution group. Data is cg_handle_t[CG_EXEC_GROUP_DISPLAY_COUNT].
    CG_EXEC_GROUP_QUEUE_COUNT          = 10,        /// Retrieve the number of queues in the execution group. Data is size_t.
    CG_EXEC_GROUP_QUEUES               = 11,        /// Retrieve the handles of all queues in the execution group. Data is cg_handle_t[CG_EXEC_GROUP_QUEUE_COUNT].
    CG_EXEC_GROUP_COMPUTE_QUEUE_COUNT  = 12,        /// Retrieve the number of compute queues in the execution group. Data is size_t.
    CG_EXEC_GROUP_COMPUTE_QUEUES       = 13,        /// Retrieve the handles of all compute queues in the execution group. Data is cg_handle_t[CG_EXEC_GROUP_COMPUTE_QUEUE_COUNT].
    CG_EXEC_GROUP_TRANSFER_QUEUE_COUNT = 14,        /// Retrieve the number of transfer queues in the execution group. Data is size_t.
    CG_EXEC_GROUP_TRANSFER_QUEUES      = 15,        /// Retrieve the handles of all transfer queues in the execution group. Data is cg_handle_t[CG_EXEC_GROUP_TRANSFER_QUEUE_COUNT].
    CG_EXEC_GROUP_GRAPHICS_QUEUE_COUNT = 16,        /// Retrieve the number of graphics queues in the execution group. Data is size_t.
    CG_EXEC_GROUP_GRAPHICS_QUEUES      = 17,        /// Retrieve the handles of all graphics queues in the execution group. Data is cg_handle_t[CG_EXEC_GROUP_GRAPHICS_QUEUE_COUNT].
};

/// @summary Define the recognized display orientation values.
enum cg_display_orientation_e : int
{
    CG_DISPLAY_ORIENTATION_UNKNOWN     = 0,         /// The display orientation is not known.
    CG_DISPLAY_ORIENTATION_LANDSCAPE   = 1,         /// The display device is in landscape mode.
    CG_DISPLAY_ORIENTATION_PORTRAIT    = 2,         /// The display device is in portrait mode.
};

/// @summary Define the supported types of execution group queues.
enum cg_queue_type_e : int
{
    CG_QUEUE_TYPE_COMPUTE              =  0,        /// The queue is used for submitting compute kernel dispatch commands.
    CG_QUEUE_TYPE_GRAPHICS             =  1,        /// The queue is used for submitting graphics kernel dispatch commands.
    CG_QUEUE_TYPE_TRANSFER             =  2,        /// The queue is used for submitting data transfer commands using a DMA engine.
};

/// @summary Define the supported types of kernel fragments.
enum cg_kernel_type_e : int
{
    CG_KERNEL_TYPE_GRAPHICS_VERTEX     =  0,        /// The kernel corresponds to the vertex shader stage.
    CG_KERNEL_TYPE_GRAPHICS_FRAGMENT   =  1,        /// The kernel corresponds to the fragment shader stage.
    CG_KERNEL_TYPE_GRAPHICS_PRIMITIVE  =  2,        /// The kernel corresponds to the geometry shader stage.
    CG_KERNEL_TYPE_COMPUTE             =  3,        /// The kernel is a compute shader.
};

/// @summary Define the supported types of memory heaps.
enum cg_heap_type_e : int
{
    CG_HEAP_TYPE_LOCAL                 =  0,        /// The heap is in device-local memory.
    CG_HEAP_TYPE_REMOTE                =  1,        /// The heap is in non-local device memory.
    CG_HEAP_TYPE_EMBEDDED              =  2,        /// The heap is in embedded (on-chip) memory.
};

/// @summary Define flags that can be specified when creating an execution group.
enum cg_execution_group_flags_e : uint32_t
{
    CG_EXECUTION_GROUP_FLAGS_NONE      = (0 << 0),  /// Configure CPUs for data parallel operation, specify devices explicitly.
    CG_EXECUTION_GROUP_TASK_PARALLEL   = (1 << 0),  /// Configure all CPU devices in the group for task-parallel operation.
    CG_EXECUTION_GROUP_HIGH_THROUGHPUT = (1 << 1),  /// Configure all CPU devices in the group for maximum throughput.
    CG_EXECUTION_GROUP_CPU_PARTITION   = (1 << 2),  /// Configure CPU devices according to a specified partition layout.
    CG_EXECUTION_GROUP_CPUS            = (1 << 3),  /// Include all CPUs in the sharegroup of the master device.
    CG_EXECUTION_GROUP_GPUS            = (1 << 4),  /// Include all GPUs in the sharegroup of the master device.
    CG_EXECUTION_GROUP_ACCELERATORS    = (1 << 5),  /// Include all accelerators in the sharegroup of the master device.
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

/// @summary Define the flags that can be specified with a memory object reference for command buffer submission.
enum cg_memory_ref_flags_e : uint32_t
{
    CG_MEMORY_REF_FLAGS_READ_WRITE     = (0 << 0),  /// The memory object may be both read and written.
    CG_MEMORY_REF_FLAGS_READ_ONLY      = (1 << 0),  /// The memory object will only be read from.
    CG_MEMORY_REF_FLAGS_WRITE_ONLY     = (1 << 1),  /// The memory object will only be written to.
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

/// @summary Define the data used to create an execution group.
struct cg_execution_group_t
{
    cg_handle_t                   RootDevice;       /// The handle of the root device used to determine the share group.
    size_t                        DeviceCount;      /// The number of explicitly-specified devices in the execution group.
    cg_handle_t                  *DeviceList;       /// An array of DeviceCount handles of the explicitly-specified devices in the execution group.
    size_t                        PartitionCount;   /// The number of CPU device partitions specified.
    int                          *ThreadCounts;     /// An array of PartitionCount items specifying the number of hardware threads per-partition.
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
/// cg_context_info_param_e::CG_CONTEXT_CPU_COUNTS.
struct cg_cpu_counts_t
{
    size_t                        NUMANodes;        /// The number of NUMA nodes in the system.
    size_t                        PhysicalCPUs;     /// The number of physical CPU packages in the system.
    size_t                        PhysicalCores;    /// The number of physical CPU cores in the system.
    size_t                        HardwareThreads;  /// The number of hardware threads in the system.
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
    char const                   *Options;          /// A NULL-terminated string specifying compilation options and constants, or NULL.
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
cgEnumerateDevices                                  /// Enumerate all devices and displays installed in the system.
(
    cg_application_info_t const  *app_info,         /// Information describing the calling application.
    cg_allocation_callbacks_t    *alloc_cb,         /// Application-defined memory allocation callbacks, or NULL.
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
cgCreateKernel                                      /// Create a new kernel object from some device-executable code.
(
    uintptr_t                     context,          /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t                   exec_group,       /// The handle of the execution group for which the kernel will be compiled.
    cg_kernel_code_t const       *create_info,      /// An object specifying source code and behavior.
    int                          &result            /// On return, set to CG_SUCCESS or another result code.
);

#ifdef __cplusplus
};     /* extern "C"  */
#endif /* __cplusplus */

#undef  CGFX_INTERFACE_DEFINED
#define CGFX_INTERFACE_DEFINED
#endif /* !defined(LIB_CGFX_H) */
