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
struct cg_cpu_counts_t;

/*/////////////////
//   Constants   //
/////////////////*/
/// @summary A special value representing an invalid handle.
#define CG_INVALID_HANDLE  ((cg_handle_t)0)

/*////////////////////////////
//  Function Pointer Types  //
////////////////////////////*/
typedef void*       (CG_API *cgMemoryAlloc_fn     )(size_t, size_t, int, uintptr_t);
typedef void        (CG_API *cgMemoryFree_fn      )(void *, size_t, size_t, int, uintptr_t);
typedef char const* (CG_API *cgResultString_fn    )(int);
typedef int         (CG_API *cgEnumerateDevices_fn)(cg_application_info_t const*, cg_allocation_callbacks_t *, size_t&, cg_handle_t *, size_t, uintptr_t &);
typedef int         (CG_API *cgGetContextInfo_fn  )(uintptr_t, int, void *, size_t, size_t *);

/*//////////////////
//   Data Types   //
//////////////////*/
/// @summary Define the recognized function return codes.
enum cg_result_e : int
{
    CG_ERROR                      = -1,        /// A generic error occurred.
    CG_INVALID_VALUE              = -2,        /// One or more input values are invalid.
    CG_OUT_OF_MEMORY              = -3,        /// The requested memory could not be allocated.
    CG_NO_PIXELFORMAT             = -4,        /// Unable to find an accelerated pixel format.
    CG_BAD_PIXELFORMAT            = -5,        /// Unable to set the drawable's pixel format.
    CG_NO_GLCONTEXT               = -6,        /// Unable to create the OpenGL rendering context.
    CG_BAD_GLCONTEXT              = -7,        /// Unable to activate the OpenGL rendering context.
    CG_NO_GLSHARING               = -8,        /// No sharing between OpenGL and OpenCL is available.
    CG_SUCCESS                    =  0,        /// The operation completed successfully.
    CG_UNSUPPORTED                =  1,        /// The function completed successfully, but the operation is not supported.
    CG_NOT_READY                  =  2,        /// The function completed successfully, but the result is not available yet.
    CG_TIMEOUT                    =  3,        /// The wait completed because the timeout interval elapsed.
    CG_SIGNALED                   =  4,        /// The wait completed because the event became signaled.
    CG_BUFFER_TOO_SMALL           =  5,        /// The supplied buffer cannot store the required amount of data.
    CG_NO_OPENCL                  =  6,        /// No compatible OpenCL platforms or devices are available.
    CG_NO_OPENGL                  =  7,        /// The display does not support the required version of OpenGL.
};

/// @summary Define the categories of host memory allocations performed by CGFX.
enum cg_allocation_type_e : int
{
    CG_ALLOCATION_TYPE_OBJECT     = 1,         /// Allocation used for an API object or data with API lifetime.
    CG_ALLOCATION_TYPE_INTERNAL   = 2,         /// Allocation used for long-lived internal data.
    CG_ALLOCATION_TYPE_TEMP       = 3,         /// Allocation used for short-lived internal data.
    CG_ALLOCATION_TYPE_KERNEL     = 4,         /// Allocation used for short-lived kernel compilation.
    CG_ALLOCATION_TYPE_DEBUG      = 5          /// Allocation used for debug data.
};

/// @summary Define the recognized object type identifiers.
enum cg_object_e : uint32_t
{
    CG_OBJECT_NONE                = (0 <<  0), /// An invalid object type identifier.
    CG_OBJECT_DEVICE              = (1 <<  0), /// The object type identifier for a device.
    CG_OBJECT_DISPLAY             = (1 <<  1), /// The object type identifier for a display output.
    CG_OBJECT_QUEUE               = (1 <<  2), /// The object type identifier for a queue.
    CG_OBJECT_COMMAND_BUFFER      = (1 <<  3), /// The object type identifier for a command buffer.
    CG_OBJECT_MEMORY              = (1 <<  4), /// The object type identifier for a generic memory object.
    CG_OBJECT_FENCE               = (1 <<  5), /// The object type identifier for a fence.
    CG_OBJECT_EVENT               = (1 <<  6), /// The object type identifier for a waitable event.
    CG_OBJECT_QUERY_POOL          = (1 <<  7), /// The object type identifier for a query pool.
    CG_OBJECT_IMAGE               = (1 <<  8), /// The object type identifier for an image.
    CG_OBJECT_KERNEL              = (1 <<  9), /// The object type identifier for a compute or shader kernel.
    CG_OBJECT_PIPELINE            = (1 << 10), /// The object type identifier for a compute or display pipeline.
    CG_OBJECT_SAMPLER             = (1 << 11), /// The object type identifier for an image sampler.
    CG_OBJECT_DESCRIPTOR_SET      = (1 << 12), /// The object type identifier for a descriptor set.
    CG_OBJECT_RASTER_STATE        = (1 << 13), /// The object type identifier for a rasterizer state description.
    CG_OBJECT_VIEWPORT_STATE      = (1 << 14), /// The object type identifier for a viewport state description.
    CG_OBJECT_BLEND_STATE         = (1 << 15), /// The object type identifier for a blend state description.
    CG_OBJECT_DEPTH_STENCIL_STATE = (1 << 16), /// The object type identifier for a depth/stencil state description.
    CG_OBJECT_MULTISAMPLE_STATE   = (1 << 17), /// The object type identifier for a multisample antialiasing state description.
};

/// @summary Define the queryable or settable data on a CGFX context.
enum cg_context_info_param_e : int
{
    CG_CONTEXT_CPU_COUNTS         = 0,         /// Retrieve the number of CPU resources in the system. Data is cg_cpu_counts_t.
    CG_CONTEXT_DEVICE_COUNT       = 1,         /// Retrieve the number of capable compute devices in the system. Data is size_t.
    CG_CONTEXT_DISPLAY_COUNT      = 2,         /// Retrieve the number of capable display devices attached to the system. Data is size_t. 
};

/// @summary Data used to describe the application to the system. Strings are NULL-terminated, ASCII only.
struct cg_application_info_t
{
    char const         *AppName;               /// The name of the application.
    char const         *DriverName;            /// The name of the application driver.
    uint32_t            ApiVersion;            /// The version of the UISS API being requested.
    uint32_t            AppVersion;            /// The version of the application.
    uint32_t            DriverVersion;         /// The version of the application driver.
};

/// @summary Function pointers and application state used to supply a custom memory allocator for internal data.
struct cg_allocation_callbacks_t
{
    cgMemoryAlloc_fn    Allocate;              /// Called when CGFX needs to allocate memory.
    cgMemoryFree_fn     Release;               /// Called when CGFX wants to return memory.
    uintptr_t           UserData;              /// Opaque data to be passed to the callbacks. May be 0.
};

/// @summary Define the data returned when querying for the CPU resources available in the system.
/// cg_context_info_param_e::CG_CONTEXT_CPU_COUNTS.
struct cg_cpu_counts_t
{
    size_t              NUMANodes;             /// The number of NUMA nodes in the system.
    size_t              PhysicalCPUs;          /// The number of physical CPU packages in the system.
    size_t              PhysicalCores;         /// The number of physical CPU cores in the system.
    size_t              HardwareThreads;       /// The number of hardware threads in the system.
};

/*/////////////////
//   Functions   //
/////////////////*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

char const*
cgResultString                                 /// Convert a result code into a string.
(
    int                          result        /// One of cg_result_e.
);

int
cgEnumerateDevices                             /// Enumerate all devices installed in the system.
(
    cg_application_info_t const *app_info,     /// Information describing the calling application.
    cg_allocation_callbacks_t   *alloc_cb,     /// Application-defined memory allocation callbacks, or NULL.
    size_t                      &device_count, /// On return, store the number of devices in the system.
    cg_handle_t                 *device_list,  /// On return, populated with a list of device handles.
    size_t                       max_devices,  /// The maximum number of device handles to write to device_list.
    uintptr_t                   &context       /// If 0, on return, store the handle of a new CGFX context; otherwise, specifies an existing CGFX context.
);

int
cgGetContextInfo                               /// Retrieve context-level capabilities or data.
(
    uintptr_t                    context,      /// A CGFX context returned by cgEnumerateDevices.
    int                          param,        /// One of cg_context_info_param_e.
    void                        *data,         /// Buffer to receive the data.
    size_t                       buffer_size,  /// The maximum number of bytes to write to the data buffer.
    size_t                      *bytes_needed  /// On return, if non-NULL, store the number of bytes required to receive the data.
);

#ifdef __cplusplus
};     /* extern "C"  */
#endif /* __cplusplus */

#undef  CGFX_INTERFACE_DEFINED
#define CGFX_INTERFACE_DEFINED
#endif /* !defined(LIB_CGFX_H) */
