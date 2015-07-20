/*/////////////////////////////////////////////////////////////////////////////
/// @summary 
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////////
//   Preprocessor   //
////////////////////*/
/// @summary Tag used to mark a function as available for use outside of the 
/// current translation unit (the default visibility).
#ifndef library_function
#define library_function    
#endif

/// @summary Tag used to mark a function as available for use outside of the
/// current translation unit (the default visibility).
#ifndef export_function
#define export_function     library_function
#endif

/// @summary Tag used to mark a function as available for public use, but not
/// exported outside of the translation unit.
#ifndef public_function
#define public_function     static
#endif

/// @summary Tag used to mark a function internal to the translation unit.
#ifndef internal_function
#define internal_function   static
#endif

/// @summary Tag used to mark a variable as local to a function, and persistent
/// across invocations of that function.
#ifndef local_persist
#define local_persist       static
#endif

/// @summary Tag used to mark a variable as global to the translation unit.
#ifndef global_variable
#define global_variable     static
#endif

/// @summary Disable excessive warnings (MSVC).
#ifdef _MSC_VER
/// 4505: Unreferenced local function was removed
#pragma warning (disable:4505)
#endif

/// @summary Disable deprecation warnings for 'insecure' CRT functions (MSVC).
#ifdef  _MSC_VER
#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif
#endif /* _MSC_VER */

/// @summary Disable CRT function is insecure warnings for CRT functions (MSVC).
#ifdef  _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif /* _MSC_VER */

/// @summary Enable heap checking and leak tracing in debug builds (MSVC).
#ifdef  _MSC_VER
#ifdef  _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif
#endif /* _MSC_VER */

/*////////////////
//   Includes   //
////////////////*/
#include <Windows.h>
#include <windowsx.h>
#include <tchar.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h> // _msize
#include <float.h>

#include <atomic>
#include <thread>

#define  GLEW_MX
#define  GLEW_STATIC
#include "GL/glew.h"
#include "GL/wglew.h"
#include "glew.c"

#undef   glewGetContext
#define  glewGetContext()    (&display->GLEW)

#undef   wglewGetContext
#define  wglewGetContext()   (&display->WGLEW)

#include "CL/cl.h"
#include "CL/cl_gl.h"
#include "CL/cl_gl_ext.h"
#include "icd.c"
#include "icd_dispatch.c"
#include "icd_windows.c"

#include "intrinsics.h"
#include "atomicfifo.h"
#include "cgfx.h"

#include "cgfx_w32_host_alloc.cc"
#include "cgfx_w32_handle.cc"
#include "cgfx_w32_opencl.cc"
#include "cgfx_w32_opengl.cc"

/*////////////////////////////
//   Forward Declarations   //
////////////////////////////*/
struct CG_DEVICE;
struct CG_DISPLAY;

/*/////////////////
//   Constants   //
/////////////////*/
/// @summary Define object table indicies within a context. Used when building handles.
local_persist size_t const CG_DEVICE_TABLE_ID    = 0;
local_persist size_t const CG_DISPLAY_TABLE_ID   = 1;

/// @summary Define object table sizes within a context. Different maximum numbers of objects help control memory usage.
/// Each size value must be a power-of-two, and the maximum number of objects of that type is one less than the stated value.
local_persist size_t const CG_MAX_DEVICES        = 4096;
local_persist size_t const CG_MAX_DISPLAYS       = 32;

/// @summary Define the registered name of the WNDCLASS used for hidden windows.
#define CG_OPENGL_HIDDEN_WNDCLASS_NAME           _T("CGFX_GL_Hidden_WndClass")

/// @summary Define the maximum number of displays that can be driven by a single GPU.
#define CG_OPENGL_MAX_ATTACHED_DISPLAYS          (16)

/// @summary Defines the maximum number of shader stages. OpenGL 3.2+ has
/// stages GL_VERTEX_SHADER, GL_GEOMETRY_SHADER and GL_FRAGMENT_SHADER.
#define CG_OPENGL_MAX_SHADER_STAGES_32           (3U)

/// @summary Defines the maximum number of shader stages. OpenGL 4.0+ has
/// stages GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER,
/// GL_TESS_CONTROL_SHADER, and GL_TESS_EVALUATION_SHADER.
#define CG_OPENGL_MAX_SHADER_STAGES_40           (5U)

/// @summary Defines the maximum number of shader stages. OpenGL 4.3+ has
/// stages GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER,
/// GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER and GL_COMPUTE_SHADER.
#define CG_OPENGL_MAX_SHADER_STAGES_43           (6U)

/// @summary Macro to convert a byte offset into a pointer.
/// @param x The byte offset value.
/// @return The offset value, as a pointer.
#define CG_GL_BUFFER_OFFSET(x)                  ((GLvoid*)(((uint8_t*)NULL)+(x)))

/// @summary Preprocessor identifier for OpenGL version 3.2 (GLSL 1.50).
#define CG_OPENGL_VERSION_32                      32 

/// @summary Preprocessor identifier for OpenGL version 3.3 (GLSL 3.30).
#define CG_OPENGL_VERSION_33                      33

/// @summary Preprocessor identifier for OpenGL version 4.0 (GLSL 4.00).
#define CG_OPENGL_VERSION_40                      40

/// @summary Preprocessor identifier for OpenGL version 4.1 (GLSL 4.10).
#define CG_OPENGL_VERSION_41                      41

/// @summary Preprocessor identifier for OpenGL version 4.3 (GLSL 4.30).
#define CG_OPENGL_VERSION_43                      43

/// @summary Preprocessor identifier for OpenGL version 4.5 (GLSL 4.50).
#define CG_OPENGL_VERSION_45                      45

/// @summary Define the version of OpenGL to build against.
#ifndef CG_OPENGL_VERSION
#define CG_OPENGL_VERSION                         CG_OPENGL_VERSION_32
#endif

/// @summary Define generic names for version-specific constants.
#if     CG_OPENGL_VERSION == CG_OPENGL_VERSION_32
#define CG_OPENGL_MAX_SHADER_STAGES               CG_OPENGL_MAX_SHADER_STAGES_32
#define CG_OPENGL_VERSION_MAJOR                   3
#define CG_OPENGL_VERSION_MINOR                   2
#define CG_OPENGL_VERSION_SUPPORTED               GLEW_VERSION_3_2
#elif   CG_OPENGL_VERSION == CG_OPENGL_VERSION_33
#define CG_OPENGL_MAX_SHADER_STAGES               CG_OPENGL_MAX_SHADER_STAGES_32
#define CG_OPENGL_VERSION_MAJOR                   3
#define CG_OPENGL_VERSION_MINOR                   3
#define CG_OPENGL_VERSION_SUPPORTED               GLEW_VERSION_3_3
#elif   CG_OPENGL_VERSION == CG_OPENGL_VERSION_40
#define CG_OPENGL_MAX_SHADER_STAGES               CG_OPENGL_MAX_SHADER_STAGES_40
#define CG_OPENGL_VERSION_MAJOR                   4
#define CG_OPENGL_VERSION_MINOR                   0
#define CG_OPENGL_VERSION_SUPPORTED               GLEW_VERSION_4_0
#elif   CG_OPENGL_VERSION == CG_OPENGL_VERSION_41
#define CG_OPENGL_MAX_SHADER_STAGES               CG_OPENGL_MAX_SHADER_STAGES_40
#define CG_OPENGL_VERSION_MAJOR                   4
#define CG_OPENGL_VERSION_MINOR                   1
#define CG_OPENGL_VERSION_SUPPORTED               GLEW_VERSION_4_1
#elif   CG_OPENGL_VERSION == CG_OPENGL_VERSION_43
#define CG_OPENGL_MAX_SHADER_STAGES               CG_OPENGL_MAX_SHADER_STAGES_43
#define CG_OPENGL_VERSION_MAJOR                   4
#define CG_OPENGL_VERSION_MINOR                   3
#define CG_OPENGL_VERSION_SUPPORTED               GLEW_VERSION_4_3
#elif   CG_OPENGL_VERSION == CG_OPENGL_VERSION_45
#define CG_OPENGL_MAX_SHADER_STAGES               CG_OPENGL_MAX_SHADER_STAGES_43
#define CG_OPENGL_VERSION_MAJOR                   4
#define CG_OPENGL_VERSION_MINOR                   5
#define CG_OPENGL_VERSION_SUPPORTED               GLEW_VERSION_4_5
#else
#error  No constants defined for target OpenGL version in cgfx_w32.cc!
#endif

/*///////////////////
//   Local Types   //
///////////////////*/
/// @summary Store the capabilities of an OpenCL 1.2-compliant device.
struct CL_DEVICE_CAPS
{
    cl_bool                      LittleEndian;         /// CL_DEVICE_ENDIAN_LITTLE
    cl_bool                      SupportECC;           /// CL_DEVICE_ERROR_CORRECTION_SUPPORT
    cl_bool                      UnifiedMemory;        /// CL_DEVICE_HOST_UNIFIED_MEMORY
    cl_bool                      CompilerAvailable;    /// CL_DEVICE_COMPILER_AVAILABLE
    cl_bool                      LinkerAvailable;      /// CL_DEVICE_LINKER_AVAILABLE
    cl_bool                      PreferUserSync;       /// CL_DEVICE_PREFERRED_INTEROP_USER_SYNC
    cl_uint                      AddressBits;          /// CL_DEVICE_ADDRESS_BITS
    cl_uint                      AddressAlign;         /// CL_DEVICE_MEM_BASE_ADDR_ALIGN
    cl_uint                      MinTypeAlign;         /// CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE
    size_t                       MaxPrintfBuffer;      /// CL_DEVICE_PRINTF_BUFFER_SIZE
    size_t                       TimerResolution;      /// CL_DEVICE_PROFILING_TIMER_RESOLUTION
    size_t                       MaxWorkGroupSize;     /// CL_DEVICE_MAX_WORK_GROUP_SIZE
    cl_ulong                     MaxMallocSize;        /// CL_DEVICE_MAX_MEM_ALLOC_SIZE
    size_t                       MaxParamSize;         /// CL_DEVICE_MAX_PARAMETER_SIZE
    cl_uint                      MaxConstantArgs;      /// CL_DEVICE_MAX_CONSTANT_ARGS
    cl_ulong                     MaxCBufferSize;       /// CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE
    cl_ulong                     GlobalMemorySize;     /// CL_DEVICE_GLOBAL_MEM_SIZE
    cl_device_mem_cache_type     GlobalCacheType;      /// CL_DEVICE_GLOBAL_MEM_CACHE_TYPE
    cl_ulong                     GlobalCacheSize;      /// CL_DEVICE_GLOBAL_MEM_CACHE_SIZE
    cl_uint                      GlobalCacheLineSize;  /// CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE
    cl_device_local_mem_type     LocalMemoryType;      /// CL_DEVICE_LOCAL_MEM_TYPE 
    cl_ulong                     LocalMemorySize;      /// CL_DEVICE_LOCAL_MEM_SIZE
    cl_uint                      ClockFrequency;       /// CL_DEVICE_MAX_CLOCK_FREQUENCY
    cl_uint                      ComputeUnits;         /// CL_DEVICE_MAX_COMPUTE_UNITS
    cl_uint                      VecWidthChar;         /// CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR
    cl_uint                      VecWidthShort;        /// CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT
    cl_uint                      VecWidthInt;          /// CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT
    cl_uint                      VecWidthLong;         /// CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG
    cl_uint                      VecWidthSingle;       /// CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT
    cl_uint                      VecWidthDouble;       /// CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE
    cl_device_fp_config          FPSingleConfig;       /// CL_DEVICE_SINGLE_FP_CONFIG
    cl_device_fp_config          FPDoubleConfig;       /// CL_DEVICE_DOUBLE_FP_CONFIG
    cl_command_queue_properties  CmdQueueConfig;       /// CL_DEVICE_QUEUE_PROPERTIES
    cl_device_exec_capabilities  ExecutionCapability;  /// CL_DEVICE_EXECUTION_CAPABILITIES
    cl_uint                      MaxSubDevices;        /// CL_DEVICE_PARTITION_MAX_SUB_DEVICES
    size_t                       NumPartitionTypes;    /// Computed; number of valid items in PartitionTypes.
    cl_device_partition_property PartitionTypes[4];    /// CL_DEVICE_PARTITION_PROPERTIES
    size_t                       NumAffinityDomains;   /// Computed; number of valid items in AffinityDomains.
    cl_device_affinity_domain    AffinityDomains[7];   /// CL_DEVICE_PARTITION_AFFINITY_DOMAIN
    cl_bool                      SupportImage;         /// CL_DEVICE_IMAGE_SUPPORT
    size_t                       MaxWidth2D;           /// CL_DEVICE_IMAGE2D_MAX_WIDTH
    size_t                       MaxHeight2D;          /// CL_DEVICE_IMAGE2D_MAX_HEIGHT
    size_t                       MaxWidth3D;           /// CL_DEVICE_IMAGE3D_MAX_WIDTH
    size_t                       MaxHeight3D;          /// CL_DEVICE_IMAGE3D_MAX_HEIGHT
    size_t                       MaxDepth3D;           /// CL_DEVICE_IMAGE3D_MAX_DEPTH
    size_t                       MaxImageArraySize;    /// CL_DEVICE_IMAGE_MAX_ARRAY_SIZE
    size_t                       MaxImageBufferSize;   /// CL_DEVICE_IMAGE_MAX_BUFFER_SIZE
    cl_uint                      MaxSamplers;          /// CL_DEVICE_MAX_SAMPLERS
    cl_uint                      MaxImageSources;      /// CL_DEVICE_MAX_READ_IMAGE_ARGS
    cl_uint                      MaxImageTargets;      /// CL_DEVICE_MAX_WRITE_IMAGE_ARGS
    cl_uint                      MaxWorkItemDimension; /// CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS
    size_t                      *MaxWorkItemSizes;     /// CL_DEVICE_MAX_WORK_ITEM_SIZES
};

/// @summary Define the state associated with an OpenCL 1.2 compatible device.
struct CG_DEVICE
{
    #define AD                   CG_OPENGL_MAX_ATTACHED_DISPLAYS
    cl_platform_id               PlatformId;           /// The OpenCL platform identifier. Same-platform devices can share resources.
    cl_device_id                 DeviceId;             /// The OpenCL device identifier, which may or may not be the same as the MasterDeviceId.
    cl_device_id                 MasterDeviceId;       /// The OpenCL device identifier of the parent device.
    cl_device_type               Type;                 /// The OpenCL device type (CPU, GPU, ACCELERATOR or CUSTOM.)

    char                        *Name;                 /// The friendly name of the device.
    char                        *Platform;             /// The friendly name of the platform.
    char                        *Version;              /// The OpenCL version supported by the device.
    char                        *Driver;               /// The version of the OpenCL driver for the device.
    char                        *Extensions;           /// A space-delimited list of extensions supported by the device.

    cl_context                   ComputeContext;       /// The OpenCL resource context associated with the device, which may or may not be shared with other devices.
    cl_command_queue             ComputeQueue;         /// The OpenCL command queue for submitting compute jobs to the device.
    cl_command_queue             TransferQueue;        /// The OpenCL command queue for submitting data transfers to the device.

    size_t                       DisplayCount;         /// The number of attached displays.
    CG_DISPLAY                  *AttachedDisplays[AD]; /// The set of attached display objects.
    HDC                          DisplayDC[AD];        /// The set of Windows GDI device context for the attached displays.
    HGLRC                        DisplayRC;            /// The Windows OpenGL rendering context for the attached displays, or NULL.

    CL_DEVICE_CAPS               Capabilities;         /// Cached device capabilities reported by OpenCL.

    uint32_t                     ObjectId;             /// The internal CGFX object identifier.
    #undef  AD
};

/// @summary Define the state associated with an OpenGL 3.2 compatible display.
struct CG_DISPLAY
{
    DWORD                        Ordinal;              /// The zero-based index of the display passed to EnumDisplayDevices.
    uint32_t                     ObjectId;             /// The internal CGFX object identifier.
    CG_DEVICE                   *DisplayDevice;        /// A reference to the GPU device that drives the display.
    HWND                         DisplayHWND;          /// The hidden fullscreen window used to retrieve the display device context.
    HDC                          DisplayDC;            /// The Windows GDI device context for the hidden fullscreen window.
    HGLRC                        DisplayRC;            /// The OpenGL rendering context for the device driving the display.
    int                          DisplayX;             /// The x-coordinate of the upper-left corner of the display.
    int                          DisplayY;             /// The y-coordinate of the upper-left corner of the display.
    size_t                       DisplayWidth;         /// The width of the display, in pixels.
    size_t                       DisplayHeight;        /// The height of the display, in pixels.
    DISPLAY_DEVICE               DisplayInfo;          /// Information about the display returned by Windows.
    DEVMODE                      DisplayMode;          /// Information about the current display mode, orientation and geometry.
    GLEWContext                  GLEW;                 /// OpenGL extension function pointers for the attached displays.
    WGLEWContext                 WGLEW;                /// OpenGL windowing extension function pointers for the attached displays.
};

/// @summary Typedef the object tables held by a context object.
typedef CG_OBJECT_TABLE<CG_DEVICE , CG_MAX_DEVICES >       CG_DEVICE_TABLE;
typedef CG_OBJECT_TABLE<CG_DISPLAY, CG_MAX_DISPLAYS>       CG_DISPLAY_TABLE;

/// @summary Define the state associated with a CGFX instance, created when devices are enumerated.
struct CG_CONTEXT
{
    CG_HOST_ALLOCATOR            HostAllocator;        /// The allocator implementation used to allocate host memory.

    cg_cpu_counts_t              CPUCounts;            /// Information about the CPU resources available in the local system.

    CG_DEVICE_TABLE              DeviceTable;          /// The object table of all OpenCL 1.2-capable compute devices.
    CG_DISPLAY_TABLE             DisplayTable;         /// The object table of all OpenGL 3.2-capable display devices.
};

/*///////////////
//   Globals   //
///////////////*/
#ifdef _MSC_VER
/// @summary When using the Microsoft Linker, __ImageBase is set to the base address of the DLL.
/// This is the same as the HINSTANCE/HMODULE of the DLL passed to DllMain.
/// See: http://blogs.msdn.com/b/oldnewthing/archive/2004/10/25/247180.aspx
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#else
#error   Need to define __ImageBase for your compiler in cgfx_w32.cc!
#endif

/*///////////////////////
//   Local Functions   //
///////////////////////*/
/// @summary Locate one substring within another, ignoring case.
/// @param search The string to search.
/// @param find The string to locate within the search string.
/// @return A pointer to the start of the first match, or NULL.
internal_function char const* 
cgStristr
(
    char const *search, 
    char const *find
)
{
    if (search == NULL)
    {   // invalid search string.
        return NULL;
    }
    if (*search == 0 && *find == 0)
    {   // both are empty strings.
        return search;
    }
    else if (*search == 0)
    {   // search is an empty string, but find is not.
        return NULL;
    }
    else
    {   // both search and find are non-NULL, non-empty strings.
        char const *si = search;
        int  const  ff = tolower(*find++);
        for ( ; ; )
        {
            int const sc = tolower(*si++);
            if (sc == ff)
            {   // first character of find matches this character of search.
                char const *ss = si;   // si points to one after the match.
                char const *fi = find; // find points to the second character.
                for ( ; ; )
                {
                    int const s = tolower(*ss++);
                    int const f = tolower(*fi++);
                    if (f == 0) return si-1;
                    if (s == 0) return NULL;
                    if (s != f) break;
                }
            }
            if (sc == 0) break;
        }
    }
    return NULL;
}

/// @summary Retrieve the length of an OpenCL platform string value, allocate a buffer and store a copy of the value.
/// @param ctx The CGFX context requesting the string data.
/// @param id The OpenCL platform identifier being queried.
/// @param param The OpenCL parameter identifier to query.
/// @param type The CGFX allocation type, one of cg_allocation_type_e.
/// @return A pointer to the NULL-terminated ASCII string, or NULL.
internal_function char* 
cgClPlatformString
(
    CG_CONTEXT      *ctx,
    cl_platform_id   id, 
    cl_platform_info param, 
    int              type
)
{
    size_t nbytes = 0;
    char  *buffer = NULL;
    clGetPlatformInfo(id, param, 0, NULL, &nbytes);
    if ((buffer = (char*) cgAllocateHostMemory(&ctx->HostAllocator, nbytes, 0, type)) != NULL)
    {   clGetPlatformInfo(id, param, nbytes, buffer, NULL);
        buffer[nbytes - 1] = '\0';
    }
    return buffer;
}

/// @summary Retrieve the length of an OpenCL device string value, allocate a buffer and store a copy of the value.
/// @param ctx The CGFX context requesting the string data.
/// @param id The OpenCL device identifier being queried.
/// @param param The OpenCL parameter identifier to query.
/// @param type The CGFX allocation type, one of cg_allocation_type_e.
/// @return A pointer to the NULL-terminated ASCII string, or NULL.
internal_function char* 
cgClDeviceString
(
    CG_CONTEXT    *ctx,
    cl_device_id   id, 
    cl_device_info param, 
    int            type
)
{
    size_t nbytes = 0;
    char  *buffer = NULL;
    clGetDeviceInfo(id, param, 0, NULL, &nbytes);
    if ((buffer = (char*) cgAllocateHostMemory(&ctx->HostAllocator, nbytes, 0, type)) != NULL)
    {   clGetDeviceInfo(id, param, nbytes, buffer, NULL);
        buffer[nbytes - 1] = '\0';
    }
    return buffer;
}

/// @summary Frees memory allocated for an OpenCL string buffer.
/// @param ctx The CGFX context requesting the string data.
/// @param str The string buffer to free.
/// @param type The CGFX allocation type specified when the buffer was allocated, one of cg_allocation_type_e.
internal_function void
cgClFreeString
(
    CG_CONTEXT *ctx, 
    char       *str,
    int         type
)
{
    cgFreeHostMemory(&ctx->HostAllocator, str, _msize(str), 0, type);
}

/// @summary Determine whether an OpenCL platform or device extension is supported.
/// @param extension_name A NULL-terminated ASCII string specifying the OpenCL extension name to query.
/// @param extension_list A NULL-terminated ASCII string of space-delimited OpenCL extension names supported by the platform or device.
internal_function bool 
cgClIsExtensionSupported
(
    char const *extension_name, 
    char const *extension_list
)
{
    return (strstr(extension_list, extension_name) != NULL);
}

/// @summary Determine whether an OpenCL platform is supported (filter out blacklisted platforms.)
/// @param ctx The CGFX context performing platform enumeration.
/// @param platform The OpenCL platform identifier to check.
/// @return true if the OpenCL platform is supported.
internal_function bool
cgClIsPlatformSupported
(
    CG_CONTEXT    *ctx,
    cl_platform_id platform
)
{
    bool  res   = true;
    char *name  = cgClPlatformString(ctx, platform, CL_PLATFORM_NAME, CG_ALLOCATION_TYPE_TEMP);
    if   (name != NULL)
    {   // unfortunately the black list must be determined manually.
        if (cgStristr(name, "experimental") != NULL)
            res = false;
        cgClFreeString(ctx, name, CG_ALLOCATION_TYPE_TEMP);
    }
    else
    {   // couldn't retrieve the platform name - assume not supported.
        res = false;
    }
    return res;
}

/// @summary Determine the OpenCL version supported by a given device.
/// @param ctx The CGFX context performing platform enumeration.
/// @param device The OpenCL device identifier to check.
/// @param major On return, stores the major version number, or 0.
/// @param minor On return, stores the minor version number, which may be 0.
/// @return true if the OpenCL version number was determined.
internal_function bool
cgClDeviceVersion
(
    CG_CONTEXT  *ctx,
    cl_device_id device, 
    int         &major, 
    int         &minor
)
{
    bool  res      = true;
    char *version  = cgClDeviceString(ctx, device, CL_DEVICE_VERSION, CG_ALLOCATION_TYPE_TEMP);
    if   (version != NULL)
    {   // the OpenCL version has to be parsed out of the string.
        int mj = 0, mi = 0;
        if (sscanf(version, "OpenCL %d.%d", &mj, &mi) == 2)
        {   // the device version was successfully parsed.
            major = mj;
            minor = mi;
            res   = true;
        }
        else
        {   // the device version string has an unexpected format.
            major = 0;
            minor = 0;
            res   = false;
        }
        cgClFreeString(ctx, version, CG_ALLOCATION_TYPE_TEMP);
    }
    else
    {   // couldn't retrieve the device OpenCL version string.
        major = 0;
        minor = 0;
        res = false;
    }
    return res;
}

/// @summary Check an OpenCL version to determine if it is supported by CGFX. The minimum supported version is 1.2.
/// @param major The major OpenCL version number. A value of 0 is considered to be invalid.
/// @param minor The minor OpenCL version number. A value of 0 is considered to be valid.
/// @return true if the OpenCL version is supported by CGFX.
internal_function bool
cgClIsVersionSupported
(
    int major, 
    int minor
)
{
    if (major <= 0) return false;
    if (major == 1 && minor <  2) return false;
    return true;
}

/// @summary Initialize all fields of a CG_DEVICE_CAPS structure to zero.
/// @param caps The CL_DEVICE_CAPS structure to zero out.
internal_function void
cgClDeviceCapsInit
(
    CL_DEVICE_CAPS *caps
)
{
    memset(caps, 0, sizeof(CL_DEVICE_CAPS));
}

/// @summary Frees memory held internally by a CG_DEVICE_CAPS structure.
/// @param ctx The CGFX context that cached the device capabilities.
/// @param caps The CL_DEVICE_CAPS structure to delete.
internal_function void
cgClDeviceCapsFree
(
    CG_CONTEXT     *ctx,
    CL_DEVICE_CAPS *caps
)
{
    size_t    size = caps->MaxWorkItemDimension * sizeof(size_t);
    cgFreeHostMemory(&ctx->HostAllocator, caps->MaxWorkItemSizes, size, 0, CG_ALLOCATION_TYPE_INTERNAL);
    caps->MaxWorkItemDimension = 0;
    caps->MaxWorkItemSizes = NULL;
}

/// @summary Query and store the capabilities of an OpenCL 1.2-compliant device.
/// @param ctx The CGFX context that is enumerating the device capabilities.
/// @param caps The capabilities structure to update.
/// @param device The OpenCL device identifier of the device to query.
internal_function void
cgClDeviceCapsQuery
(
    CG_CONTEXT     *ctx, 
    CL_DEVICE_CAPS *caps,
    cl_device_id    device
)
{
    cl_uint mid = 0;
    clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(mid), &mid, NULL);
    caps->MaxWorkItemDimension = mid;
    caps->MaxWorkItemSizes = (size_t*) cgAllocateHostMemory(&ctx->HostAllocator, mid * sizeof(size_t), 0, CG_ALLOCATION_TYPE_INTERNAL);
    
    // some OpenCL drivers don't properly zero-terminate lists, so ensure the memory is initialized:
    memset(caps->PartitionTypes  , 0, 4 * sizeof(cl_device_partition_property));
    memset(caps->AffinityDomains , 0, 7 * sizeof(cl_device_affinity_domain));
    memset(caps->MaxWorkItemSizes, 0, mid * sizeof(size_t));

    clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES,     mid * sizeof(size_t),                      caps->MaxWorkItemSizes,     NULL);
    clGetDeviceInfo(device, CL_DEVICE_ENDIAN_LITTLE,                 sizeof(caps->LittleEndian),         &caps->LittleEndian,         NULL);
    clGetDeviceInfo(device, CL_DEVICE_ERROR_CORRECTION_SUPPORT,      sizeof(caps->SupportECC),           &caps->SupportECC,           NULL);
    clGetDeviceInfo(device, CL_DEVICE_HOST_UNIFIED_MEMORY,           sizeof(caps->UnifiedMemory),        &caps->UnifiedMemory,        NULL);
    clGetDeviceInfo(device, CL_DEVICE_COMPILER_AVAILABLE,            sizeof(caps->CompilerAvailable),    &caps->CompilerAvailable,    NULL);
    clGetDeviceInfo(device, CL_DEVICE_LINKER_AVAILABLE,              sizeof(caps->LinkerAvailable),      &caps->LinkerAvailable,      NULL);
    clGetDeviceInfo(device, CL_DEVICE_PREFERRED_INTEROP_USER_SYNC,   sizeof(caps->PreferUserSync),       &caps->PreferUserSync,       NULL);
    clGetDeviceInfo(device, CL_DEVICE_ADDRESS_BITS,                  sizeof(caps->AddressBits),          &caps->AddressBits,          NULL);
    clGetDeviceInfo(device, CL_DEVICE_MEM_BASE_ADDR_ALIGN,           sizeof(caps->AddressAlign),         &caps->AddressAlign,         NULL);
    clGetDeviceInfo(device, CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE,      sizeof(caps->MinTypeAlign),         &caps->MinTypeAlign,         NULL);
    clGetDeviceInfo(device, CL_DEVICE_PRINTF_BUFFER_SIZE,            sizeof(caps->MaxPrintfBuffer),      &caps->MaxPrintfBuffer,      NULL);
    clGetDeviceInfo(device, CL_DEVICE_PROFILING_TIMER_RESOLUTION,    sizeof(caps->TimerResolution),      &caps->TimerResolution,      NULL);
    clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE,           sizeof(caps->MaxWorkGroupSize),     &caps->MaxWorkGroupSize,     NULL);
    clGetDeviceInfo(device, CL_DEVICE_MAX_MEM_ALLOC_SIZE,            sizeof(caps->MaxMallocSize),        &caps->MaxMallocSize,        NULL);
    clGetDeviceInfo(device, CL_DEVICE_MAX_PARAMETER_SIZE,            sizeof(caps->MaxParamSize),         &caps->MaxParamSize,         NULL);
    clGetDeviceInfo(device, CL_DEVICE_MAX_CONSTANT_ARGS,             sizeof(caps->MaxConstantArgs),      &caps->MaxConstantArgs,      NULL);
    clGetDeviceInfo(device, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE,      sizeof(caps->MaxCBufferSize),       &caps->MaxCBufferSize,       NULL);
    clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE,               sizeof(caps->GlobalMemorySize),     &caps->GlobalMemorySize,     NULL);
    clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_CACHE_TYPE,         sizeof(caps->GlobalCacheType),      &caps->GlobalCacheType,      NULL);
    clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE,         sizeof(caps->GlobalCacheSize),      &caps->GlobalCacheSize,      NULL);
    clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE,     sizeof(caps->GlobalCacheLineSize),  &caps->GlobalCacheLineSize,  NULL);
    clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_TYPE,                sizeof(caps->LocalMemoryType),      &caps->LocalMemoryType,      NULL);
    clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE,                sizeof(caps->LocalMemorySize),      &caps->LocalMemorySize,      NULL);
    clGetDeviceInfo(device, CL_DEVICE_MAX_CLOCK_FREQUENCY,           sizeof(caps->ClockFrequency),       &caps->ClockFrequency,       NULL);
    clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS,             sizeof(caps->ComputeUnits),         &caps->ComputeUnits,         NULL);
    clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR,   sizeof(caps->VecWidthChar),         &caps->VecWidthChar,         NULL);
    clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT,  sizeof(caps->VecWidthShort),        &caps->VecWidthShort,        NULL);
    clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT,    sizeof(caps->VecWidthInt),          &caps->VecWidthInt,          NULL);
    clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG,   sizeof(caps->VecWidthLong),         &caps->VecWidthLong,         NULL);
    clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT,  sizeof(caps->VecWidthSingle),       &caps->VecWidthSingle,       NULL);
    clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, sizeof(caps->VecWidthDouble),       &caps->VecWidthDouble,       NULL);
    clGetDeviceInfo(device, CL_DEVICE_SINGLE_FP_CONFIG,              sizeof(caps->FPSingleConfig),       &caps->FPSingleConfig,       NULL);
    clGetDeviceInfo(device, CL_DEVICE_DOUBLE_FP_CONFIG,              sizeof(caps->FPDoubleConfig),       &caps->FPDoubleConfig,       NULL);
    clGetDeviceInfo(device, CL_DEVICE_QUEUE_PROPERTIES,              sizeof(caps->CmdQueueConfig),       &caps->CmdQueueConfig,       NULL);
    clGetDeviceInfo(device, CL_DEVICE_EXECUTION_CAPABILITIES,        sizeof(caps->ExecutionCapability),  &caps->ExecutionCapability,  NULL);
    clGetDeviceInfo(device, CL_DEVICE_PARTITION_MAX_SUB_DEVICES,     sizeof(caps->MaxSubDevices),        &caps->MaxSubDevices,        NULL);
    clGetDeviceInfo(device, CL_DEVICE_PARTITION_PROPERTIES,      4 * sizeof(caps->PartitionTypes),        caps->PartitionTypes,       NULL);
    clGetDeviceInfo(device, CL_DEVICE_PARTITION_AFFINITY_DOMAIN, 7 * sizeof(caps->AffinityDomains),       caps->AffinityDomains,      NULL);
    clGetDeviceInfo(device, CL_DEVICE_IMAGE_SUPPORT,                 sizeof(caps->SupportImage),         &caps->SupportImage,         NULL);
    clGetDeviceInfo(device, CL_DEVICE_IMAGE2D_MAX_WIDTH,             sizeof(caps->MaxWidth2D),           &caps->MaxWidth2D,           NULL);
    clGetDeviceInfo(device, CL_DEVICE_IMAGE2D_MAX_HEIGHT,            sizeof(caps->MaxHeight2D),          &caps->MaxHeight2D,          NULL);
    clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_WIDTH,             sizeof(caps->MaxWidth3D),           &caps->MaxWidth3D,           NULL);
    clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_HEIGHT,            sizeof(caps->MaxHeight3D),          &caps->MaxHeight3D,          NULL);
    clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_DEPTH,             sizeof(caps->MaxDepth3D),           &caps->MaxDepth3D,           NULL);
    clGetDeviceInfo(device, CL_DEVICE_IMAGE_MAX_ARRAY_SIZE,          sizeof(caps->MaxImageArraySize),    &caps->MaxImageArraySize,    NULL);
    clGetDeviceInfo(device, CL_DEVICE_IMAGE_MAX_BUFFER_SIZE,         sizeof(caps->MaxImageBufferSize),   &caps->MaxImageBufferSize,   NULL);
    clGetDeviceInfo(device, CL_DEVICE_MAX_SAMPLERS,                  sizeof(caps->MaxSamplers),          &caps->MaxSamplers,          NULL);
    clGetDeviceInfo(device, CL_DEVICE_MAX_READ_IMAGE_ARGS,           sizeof(caps->MaxImageSources),      &caps->MaxImageSources,      NULL);
    clGetDeviceInfo(device, CL_DEVICE_MAX_WRITE_IMAGE_ARGS,          sizeof(caps->MaxImageTargets),      &caps->MaxImageTargets,      NULL);
    // determine the number of partition types we got:
    caps->NumPartitionTypes = 0;
    for (size_t i = 0; i < sizeof(caps->PartitionTypes) / sizeof(caps->PartitionTypes[0]); ++i)
    {
        if (caps->PartitionTypes[i] == 0) break;
        else caps->NumPartitionTypes++;
    }
    // determine the number of affinity domains we got:
    caps->NumAffinityDomains = 0;
    for (size_t i = 0; i < sizeof(caps->AffinityDomains) / sizeof(caps->AffinityDomains[0]); ++i)
    {
        if (caps->AffinityDomains[i] == 0) break;
        else caps->NumAffinityDomains++;
    }
}

/// @summary Frees all resources and releases all references held by a device object.
/// @param ctx The CGFX context that owns the device object.
/// @param device The device object to delete.
internal_function void
cgDeleteDevice
(
    CG_CONTEXT     *ctx, 
    CG_DEVICE      *device
)
{
    if (device->ComputeQueue != NULL)
    {
        clReleaseCommandQueue(device->ComputeQueue);
    }
    if (device->TransferQueue != NULL)
    {
        clReleaseCommandQueue(device->TransferQueue);
    }
    if (device->ComputeContext != NULL)
    {
        clReleaseContext(device->ComputeContext);
    }
    if (device->DisplayRC != NULL)
    {
        wglDeleteContext(device->DisplayRC);
    }
    cgClDeviceCapsFree(ctx, &device->Capabilities);
    cgClFreeString(ctx, device->Extensions, CG_ALLOCATION_TYPE_INTERNAL);
    cgClFreeString(ctx, device->Driver    , CG_ALLOCATION_TYPE_INTERNAL);
    cgClFreeString(ctx, device->Version   , CG_ALLOCATION_TYPE_INTERNAL);
    cgClFreeString(ctx, device->Platform  , CG_ALLOCATION_TYPE_INTERNAL);
    cgClFreeString(ctx, device->Name      , CG_ALLOCATION_TYPE_INTERNAL);
    if (device->DeviceId != NULL)
    {
        clReleaseDevice(device->DeviceId);
    }
    memset(device, 0, sizeof(CG_DEVICE));
}

/// @summary Frees all resources and releases all references held by a display object.
/// @param ctx The CGFX context that owns the display object.
/// @param display The display object to delete.
internal_function void
cgDeleteDisplay
(
    CG_CONTEXT    *ctx, 
    CG_DISPLAY    *display
)
{
    if (display->DisplayDC != NULL)
    {
        ReleaseDC(display->DisplayHWND, display->DisplayDC);
    }
    if (display->DisplayHWND != NULL)
    {
        DestroyWindow(display->DisplayHWND);
    }
    memset(display, 0, sizeof(CG_DISPLAY));
}

/// @summary Allocates memory for and initializes an empty CGFX context object.
/// @param app_info Information about the application associated with the context.
/// @param alloc_cb User-supplied host memory allocator callbacks, or NULL.
/// @param result Set to CG_SUCCESS if the context was created; otherwise, set to the error code.
/// @return A pointer to the new CGFX context, or NULL.
internal_function CG_CONTEXT*
cgCreateContext
(
    cg_application_info_t const  *app_info, 
    cg_allocation_callbacks_t    *alloc_cb, 
    int                          &result
)
{
    CG_CONTEXT       *ctx = NULL;
    CG_HOST_ALLOCATOR host_alloc = CG_HOST_ALLOCATOR_STATIC_INIT;
    if ((result = cgSetupUserHostAllocator(alloc_cb, &host_alloc)) != CG_SUCCESS)
    {   // cannot set up user allocator - can't allocate any memory.
        return NULL;
    }
    if ((ctx = (CG_CONTEXT*) cgAllocateHostMemory(&host_alloc, sizeof(CG_CONTEXT), 0, CG_ALLOCATION_TYPE_OBJECT)) == NULL)
    {   // unable to allocate the core context storage.
        result = CG_OUT_OF_MEMORY;
        return NULL;
    }

    // initialize the various object tables on the context to empty.
    cgObjectTableInit(&ctx->DeviceTable , CG_OBJECT_DEVICE , CG_DEVICE_TABLE_ID);
    cgObjectTableInit(&ctx->DisplayTable, CG_OBJECT_DISPLAY, CG_DISPLAY_TABLE_ID);

    // the context has been fully initialized.
    result             = CG_SUCCESS;
    ctx->HostAllocator = host_alloc;
    return ctx;
}

/// @summary Frees all resources associated with a CGFX context object.
/// @param ctx The CGFX context to delete.
internal_function void
cgDeleteContext
(
    CG_CONTEXT *ctx
)
{
    CG_HOST_ALLOCATOR *host_alloc = &ctx->HostAllocator;
    // free all device objects:
    for (size_t i = 0; i < ctx->DeviceTable.ObjectCount; ++i)
    {
        CG_DEVICE  *obj = &ctx->DeviceTable.Objects[i];
        cgDeleteDevice(ctx, obj);
    }
    // free all display objects:
    for (size_t i = 0; i < ctx->DisplayTable.ObjectCount; ++i)
    {
        CG_DISPLAY *obj = &ctx->DisplayTable.Objects[i];
        cgDeleteDisplay(ctx, obj);
    }
    // finally, free the memory block for the context structure:
    cgFreeHostMemory(host_alloc, ctx, sizeof(CG_CONTEXT), 0, CG_ALLOCATION_TYPE_OBJECT);
}

/// @summary Enumerate all CPU resources available in the system.
/// @param ctx The CGFX context to update with CPU counts.
/// @return CG_SUCCESS or CG_OUT_OF_MEMORY.
internal_function int
cgGetCpuCounts
(
    CG_CONTEXT *ctx
)
{
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *lpibuf = NULL;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *info   = NULL;
    cg_cpu_counts_t &counts = ctx->CPUCounts;
    uint8_t *bufferp     = NULL;
    uint8_t *buffere     = NULL;
    DWORD    buffer_size = 0;
    
    // figure out the amount of space required, and allocate a temporary buffer:
    GetLogicalProcessorInformationEx(RelationAll, NULL, &buffer_size);
    if ((lpibuf = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*) cgAllocateHostMemory(&ctx->HostAllocator, size_t(buffer_size), 0, CG_ALLOCATION_TYPE_TEMP)) == NULL)
    {   // unable to allocate the required memory:
        counts.NUMANodes       = 1;
        counts.PhysicalCPUs    = 1;
        counts.PhysicalCores   = 1;
        counts.HardwareThreads = 1;
        return CG_OUT_OF_MEMORY;
    }
    GetLogicalProcessorInformationEx(RelationAll, lpibuf, &buffer_size);

    // initialize the output counts:
    counts.NUMANodes       = 0;
    counts.PhysicalCPUs    = 0;
    counts.PhysicalCores   = 0;
    counts.HardwareThreads = 0;

    // step through the buffer and update counts:
    bufferp = (uint8_t*) lpibuf;
    buffere =((uint8_t*) lpibuf) + size_t(buffer_size);
    while (bufferp < buffere)
    {
        info = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*) bufferp;
        switch (info->Relationship)
        {
        case RelationNumaNode:
            counts.NUMANodes++;
            break;
        case RelationProcessorPackage:
            counts.PhysicalCPUs++;
            break;
        case RelationProcessorCore:
            counts.PhysicalCores++;
            if (info->Processor.Flags == LTP_PC_SMT)
            {   // this core has two hardware threads:
                counts.HardwareThreads += 2;
            }
            else
            {   // this core only has one hardware thread:
                counts.HardwareThreads++;
            }
            break;
        default: // RelationGroup, RelationCache - don't care.
            break;
        }
        bufferp += size_t(info->Size);
    }
    // free the temporary buffer:
    cgFreeHostMemory(&ctx->HostAllocator, lpibuf, size_t(buffer_size), 0, CG_ALLOCATION_TYPE_TEMP);
    return CG_SUCCESS;
}

/// @summary Check the device table for a context to determine if a given device is already known.
/// @param ctx The CGFX context performing device enumeration.
/// @param platform The OpenCL platform identifier associated with the device.
/// @param device The OpenCL device identifier to search for.
/// @param index If the function returns true, the existing index is stored here; otherwise, the new index is stored.
/// @param check Specify true to actually check the device table, or false during initial enumeration.
internal_function bool
cgClDoesDeviceExist
(
    CG_CONTEXT    *ctx, 
    cl_platform_id platform, 
    cl_device_id   device, 
    size_t        &index, 
    bool           check
)
{
    if (!check)
    {   // during initial enumeration, no devices exist.
        return false;
    }
    for (size_t i = 0, n = ctx->DeviceTable.ObjectCount; i < n; ++i)
    {
        if (ctx->DeviceTable.Objects[i].PlatformId == platform && 
            ctx->DeviceTable.Objects[i].DeviceId   == device)
        {
            index = i;
            return true;
        }
    }
    index = ctx->DeviceTable.ObjectCount;
    return false;
}

/// @summary Enumerate all OpenCL platforms and devices available on the system.
/// @param ctx The CGFX context to populate with device information.
/// @return CG_SUCCESS, CG_NO_OPENCL or CG_OUT_OF_MEMORY.
internal_function int
cgClEnumerateDevices
(
    CG_CONTEXT *ctx
)
{
    cl_uint   total_platforms = 0;
    cl_uint   valid_platforms = 0;
    cl_platform_id *platforms = NULL;
    bool           check_list = false;
    int                   res = CG_SUCCESS;

    if (ctx->DeviceTable.ObjectCount > 0)
    {   // this isn't the first enumeration, so don't generate duplicate devices.
        check_list = true;
    }

    // determine the number of OpenCL platforms and get their IDs.
    clGetPlatformIDs(cl_uint(-1), NULL, &total_platforms);
    if (total_platforms == 0)
    {   // no OpenCL platforms are available, so there are no devices.
        return CG_NO_OPENCL;
    }
    if ((platforms = (cl_platform_id*) cgAllocateHostMemory(&ctx->HostAllocator, total_platforms * sizeof(cl_platform_id), 0, CG_ALLOCATION_TYPE_TEMP)) == NULL)
    {   // unable to allocate memory for platform IDs.
        return CG_OUT_OF_MEMORY;
    }
    clGetPlatformIDs(total_platforms, platforms, NULL);

    // enumerate the devices in each supported platform:
    for (size_t platform_index = 0, platform_count = size_t(total_platforms); 
                platform_index <    platform_count; 
              ++platform_index)
    {   cl_platform_id platform  =  platforms[platform_index];

        // filter out blacklisted platforms:
        if (!cgClIsPlatformSupported(ctx, platform))
            continue;

        // enumerate all devices in the platform and initialize CG_DEVICE objects.
        cl_uint total_devices = 0;
        cl_uint valid_devices = 0;
        cl_device_id *devices = NULL;

        // query for the number of available devices.
        clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL, &total_devices);
        if (total_devices == 0)
            continue;
        if ((devices = (cl_device_id*) cgAllocateHostMemory(&ctx->HostAllocator, total_devices * sizeof(cl_device_id), 0, CG_ALLOCATION_TYPE_TEMP)) == NULL)
        {   // unable to allocate memory for device IDs.
            res = CG_OUT_OF_MEMORY;
            goto cleanup;
        }
        clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, total_devices, devices, NULL);

        // validate each device and create CG_DEVICE objects.
        for (size_t device_index = 0, device_count = size_t(total_devices);
                    device_index <    device_count;
                  ++device_index)
        {   cl_device_id  device =    devices[device_index];
            size_t      existing =    0;
            int           majorv =    0;
            int           minorv =    0;

            // skip devices that are already known:
            if (cgClDoesDeviceExist(ctx, platform, device, existing, check_list))
            {   valid_devices++;
                continue;
            }
            // skip devices that don't support OpenCL 1.2 or later:
            if (!cgClDeviceVersion(ctx, device, majorv, minorv))
            {   // couldn't parse the version information; skip.
                continue;
            }
            if (!cgClIsVersionSupported(majorv, minorv))
            {   // OpenCL exposed by the device is an unsupported version; skip.
                continue;
            }

            // this device meets the CGFX requirements for use; create an object:
            CG_DEVICE dev;
            
            // save basic device information.
            clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(cl_device_type), &dev.Type, NULL);
            dev.PlatformId       = platform;
            dev.DeviceId         = device;
            dev.MasterDeviceId   = device;
            dev.Name             = cgClDeviceString  (ctx, device  , CL_DEVICE_NAME      , CG_ALLOCATION_TYPE_INTERNAL);
            dev.Platform         = cgClPlatformString(ctx, platform, CL_PLATFORM_NAME    , CG_ALLOCATION_TYPE_INTERNAL);
            dev.Version          = cgClDeviceString  (ctx, device  , CL_DEVICE_VERSION   , CG_ALLOCATION_TYPE_INTERNAL);
            dev.Driver           = cgClDeviceString  (ctx, device  , CL_DRIVER_VERSION   , CG_ALLOCATION_TYPE_INTERNAL);
            dev.Extensions       = cgClDeviceString  (ctx, device  , CL_DEVICE_EXTENSIONS, CG_ALLOCATION_TYPE_INTERNAL);

            // the following are set up when the device is configured.
            dev.ComputeContext   = NULL;
            dev.ComputeQueue     = NULL;
            dev.TransferQueue    = NULL;

            // the following are setup during display enumeration.
            dev.DisplayCount     = 0;
            dev.DisplayRC        = NULL;
            memset(dev.AttachedDisplays, NULL, CG_OPENGL_MAX_ATTACHED_DISPLAYS * sizeof(CG_DISPLAY*));
            memset(dev.DisplayDC       , NULL, CG_OPENGL_MAX_ATTACHED_DISPLAYS * sizeof(HDC));

            // query and cache all of the device capabilities.
            cgClDeviceCapsQuery(ctx, &dev.Capabilities, device);

            // insert the device record into the device table for later access.
            if (cgObjectTableAdd(&ctx->DeviceTable, dev) != CG_INVALID_HANDLE)
            {   // the device was added successfully.
                valid_devices++;
            }
            else
            {   // free all of the device memory; the table is full.
                cgClDeviceCapsFree(ctx, &dev.Capabilities);
                cgClFreeString(ctx, dev.Extensions, CG_ALLOCATION_TYPE_INTERNAL);
                cgClFreeString(ctx, dev.Driver    , CG_ALLOCATION_TYPE_INTERNAL);
                cgClFreeString(ctx, dev.Version   , CG_ALLOCATION_TYPE_INTERNAL);
                cgClFreeString(ctx, dev.Platform  , CG_ALLOCATION_TYPE_INTERNAL);
                cgClFreeString(ctx, dev.Name      , CG_ALLOCATION_TYPE_INTERNAL);
            }
        }

        // the platform is only valid if it exposes at least one device:
        if (valid_devices > 0)
        {
            valid_platforms++;
        }

        // free temporary memory allocated for the device ID list.
        cgFreeHostMemory(&ctx->HostAllocator, devices, total_devices * sizeof(cl_device_id), 0, CG_ALLOCATION_TYPE_TEMP);
    }

    // if there are no *supported* OpenCL platforms, we're sunk.
    if (valid_platforms == 0)
    {
        res = CG_NO_OPENCL;
    }
    else
    {   // we have at least one valid OpenCL-capable device.
        res = CG_SUCCESS;
    }
    // fall through to cleanup.
cleanup:
    cgFreeHostMemory(&ctx->HostAllocator, platforms, total_platforms * sizeof(cl_platform_id), 0, CG_ALLOCATION_TYPE_TEMP);
    return res;
}

/// @summary Check the display table for a context to determine if a given display is already known.
/// @param ctx The CGFX context performing device enumeration.
/// @param display_name The display name to search for.
/// @param index If the function returns true, the existing index is stored here; otherwise, the new index is stored.
/// @param check Specify true to actually check the display table, or false during initial enumeration.
internal_function bool
cgGlDoesDisplayExist
(
    CG_CONTEXT  *ctx, 
    TCHAR const *display_name, 
    size_t      &index, 
    bool         check
)
{
    if (!check)
    {   // during initial enumeration, no devices exist.
        return false;
    }
    for (size_t i = 0, n = ctx->DisplayTable.ObjectCount; i < n; ++i)
    {
        if (_tcscmp(ctx->DisplayTable.Objects[i].DisplayInfo.DeviceName, display_name) == 0)
        {
            index = i;
            return true;
        }
    }
    index = ctx->DisplayTable.ObjectCount;
    return false;
}

/// @summary Initialize wgl GLEW and resolve WGL extension entry points.
/// @param ctx The CGFX display context.
/// @param display The display object being initialized.
/// @param x The x-coordinate of the upper-left corner of the target display.
/// @param y The y-coordinate of the upper-left corner of the target display.
/// @param w The width of the target display, in pixels.
/// @param h The height of the target display, in pixels.
/// @return true if wgl GLEW is initialized successfully.
internal_function bool
cgGlInitializeWGLEW
(
    CG_CONTEXT *ctx, 
    CG_DISPLAY *display, 
    int         x, 
    int         y, 
    int         w, 
    int         h
)
{
    PIXELFORMATDESCRIPTOR pfd;
    GLenum e = GLEW_OK;
    HWND wnd = NULL;
    HDC   dc = NULL;
    HGLRC rc = NULL;
    bool res = true;
    int  fmt = 0;

    // create a dummy window, pixel format and rendering context and 
    // make them current so we can retrieve the wgl extension entry 
    // points. in order to create a rendering context, the pixel format
    // must be set on a window, and Windows only allows the pixel format
    // to be set once for a given window - which is dumb.
    if ((wnd = CreateWindow(CG_OPENGL_HIDDEN_WNDCLASS_NAME, _T("CGFX_WGLEW_InitWnd"), WS_POPUP, x, y, w, h, NULL, NULL, (HINSTANCE)&__ImageBase, NULL)) == NULL)
    {   // unable to create the hidden window - can't create an OpenGL rendering context.
        res  = false; goto cleanup;
    }
    if ((dc  = GetDC(wnd)) == NULL)
    {   // unable to retrieve the window device context - can't create an OpenGL rendering context.
        res  = false; goto cleanup;
    }
    // fill out a PIXELFORMATDESCRIPTOR using common pixel format attributes.
    memset(&pfd,   0, sizeof(PIXELFORMATDESCRIPTOR)); 
    pfd.nSize       = sizeof(PIXELFORMATDESCRIPTOR); 
    pfd.nVersion    = 1; 
    pfd.dwFlags     = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW; 
    pfd.iPixelType  = PFD_TYPE_RGBA; 
    pfd.cColorBits  = 32; 
    pfd.cDepthBits  = 24; 
    pfd.cStencilBits=  8;
    pfd.iLayerType  = PFD_MAIN_PLANE; 
    if ((fmt = ChoosePixelFormat(dc, &pfd)) == 0)
    {   // unable to find a matching pixel format - can't create an OpenGL rendering context.
        res  = false; goto cleanup;
    }
    if (SetPixelFormat(dc, fmt, &pfd) != TRUE)
    {   // unable to set the dummy window pixel format - can't create an OpenGL rendering context.
        res  = false; goto cleanup;
    }
    if ((rc  = wglCreateContext(dc))  == NULL)
    {   // unable to create the dummy OpenGL rendering context.
        res  = false; goto cleanup;
    }
    if (wglMakeCurrent(dc, rc) != TRUE)
    {   // unable to make the OpenGL rendering context current.
        res  = false; goto cleanup;
    }
    // finally, we can initialize GLEW and get the function entry points.
    // this populates the function pointers in the display->WGLEW field.
    if ((e   = wglewInit())  != GLEW_OK)
    {   // unable to initialize GLEW - WGLEW extensions are unavailable.
        res  = false; goto cleanup;
    }
    // initialization completed successfully. fallthrough to cleanup.
    res = true;
    // fall-through to cleanup.
cleanup:
    wglMakeCurrent(NULL, NULL);
    if (rc  != NULL) wglDeleteContext(rc);
    if (dc  != NULL) ReleaseDC(wnd, dc);
    if (wnd != NULL) DestroyWindow(wnd);
    return res;
}

/// @summary Create an OpenGL rendering context for a given display, locate its associated OpenCL device, and setup sharing between OpenGL and OpenCL.
/// @param ctx The CGFX context that owns the display.
/// @param display The CGFX display object for which the rendering context is being created.
/// @return CG_SUCCESS, CG_NO_GLSHARING, CG_NO_OPENGL, CG_NO_PIXELFORMAT, CG_BAD_PIXELFORMAT, CG_NO_GLCONTEXT or CG_BAD_GLCONTEXT.
internal_function int
cgGlCreateRenderingContext
(
    CG_CONTEXT *ctx, 
    CG_DISPLAY *display
)
{
    int x = (int) display->DisplayX;
    int y = (int) display->DisplayY;
    int w = (int) display->DisplayWidth;
    int h = (int) display->DisplayHeight;

    // if the device already has a rendering context, there's nothing to do.
    if (display->DisplayRC != NULL)
        return CG_SUCCESS;

    // initialize wgl GLEW function pointers to assist with pixel format 
    // selection and  OpenGL rendering context creation.
    if (!cgGlInitializeWGLEW(ctx, display, x, y, w, h))
    {   // can't initialize GLEW, so only OpenGL 1.1 would be available.
        return CG_NO_OPENGL;
    }

    // the first step is to find an accelerated a pixel format for the display.
    PIXELFORMATDESCRIPTOR pfd;
    HDC  win_dc = display->DisplayDC;
    HGLRC gl_rc = NULL;
    GLenum  err = GLEW_OK;
    int     fmt = 0;
    memset(&pfd , 0, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.nSize   =    sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion= 1;

    // for creating a rendering context and pixel format, we can either use
    // the new method, or if that's not available, fall back to the old way.
    if (WGLEW_ARB_pixel_format)
    {   // use the more recent method to find a pixel format.
        UINT fmt_count     = 0;
        int  fmt_attribs[] = 
        {
            WGL_SUPPORT_OPENGL_ARB,                      GL_TRUE, 
            WGL_DRAW_TO_WINDOW_ARB,                      GL_TRUE, 
            WGL_DEPTH_BITS_ARB    ,                           24, 
            WGL_STENCIL_BITS_ARB  ,                            8,
            WGL_RED_BITS_ARB      ,                            8, 
            WGL_GREEN_BITS_ARB    ,                            8,
            WGL_BLUE_BITS_ARB     ,                            8,
            WGL_PIXEL_TYPE_ARB    ,            WGL_TYPE_RGBA_ARB,
            WGL_ACCELERATION_ARB  ,    WGL_FULL_ACCELERATION_ARB, 
            //WGL_SAMPLE_BUFFERS_ARB,                     GL_FALSE, /* GL_TRUE to enable MSAA */
            //WGL_SAMPLES_ARB       ,                            1, /* ex. 4 = 4x MSAA        */
            0
        };
        if (wglChoosePixelFormatARB(win_dc, fmt_attribs, NULL, 1, &fmt, &fmt_count) != TRUE)
        {   // unable to find a matching pixel format - can't create an OpenGL rendering context.
            return CG_NO_PIXELFORMAT;
        }
    }
    else
    {   // use the legacy method to find the pixel format.
        pfd.dwFlags      = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW; 
        pfd.iPixelType   = PFD_TYPE_RGBA; 
        pfd.cColorBits   = 32; 
        pfd.cDepthBits   = 24; 
        pfd.cStencilBits =  8;
        pfd.iLayerType   = PFD_MAIN_PLANE; 
        if ((fmt = ChoosePixelFormat(win_dc, &pfd)) == 0)
        {   // unable to find a matching pixel format - can't create an OpenGL rendering context.
            return CG_NO_PIXELFORMAT;
        }
    }

    // attempt to set the pixel format, which can only be done once per-window.
    if (SetPixelFormat(win_dc, fmt, &pfd) != TRUE)
    {   // unable to assign the pixel format to the window.
        return CG_BAD_PIXELFORMAT;
    }

    // attempt to create the OpenGL rendering context. we can either use 
    // the new method, or if that's not available, fall back to the old way.
    if (WGLEW_ARB_create_context)
    {
        if (WGLEW_ARB_create_context_profile)
        {
            int  rc_attribs[] = 
            {
                WGL_CONTEXT_MAJOR_VERSION_ARB,    CG_OPENGL_VERSION_MAJOR, 
                WGL_CONTEXT_MINOR_VERSION_ARB,    CG_OPENGL_VERSION_MINOR, 
                WGL_CONTEXT_PROFILE_MASK_ARB ,    WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#ifdef _DEBUG
                WGL_CONTEXT_FLAGS_ARB        ,    WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
                0
            };
            if ((gl_rc = wglCreateContextAttribsARB(win_dc, NULL, rc_attribs)) == NULL)
            {   // unable to create the OpenGL rendering context.
                return CG_NO_GLCONTEXT;
            }
        }
        else
        {
            int  rc_attribs[] = 
            {
                WGL_CONTEXT_MAJOR_VERSION_ARB,    CG_OPENGL_VERSION_MAJOR,
                WGL_CONTEXT_MINOR_VERSION_ARB,    CG_OPENGL_VERSION_MINOR,
#ifdef _DEBUG
                WGL_CONTEXT_FLAGS_ARB        ,    WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
                0
            };
            if ((gl_rc = wglCreateContextAttribsARB(win_dc, NULL, rc_attribs)) == NULL)
            {   // unable to create the OpenGL rendering context.
                return CG_NO_GLCONTEXT;
            }
        }
    }
    else
    {   // use the legacy method to create the OpenGL rendering context.
        if ((gl_rc = wglCreateContext(win_dc)) == NULL)
        {   // unable to create the OpenGL rendering context.
            return CG_NO_GLCONTEXT;
        }
    }
    
    // make the rendering context current.
    if (wglMakeCurrent(win_dc, gl_rc) != TRUE)
    {   // unable to activate the OpenGL rendering context on the current thread.
        wglDeleteContext(gl_rc);
        return CG_BAD_GLCONTEXT;
    }

    // initialize GLEW function pointers for the rendering context.
    // if the required version of OpenGL isn't supported, fail.
    // if glewExperimental isn't set to GL_TRUE, we crash. See:
    // http://www.opengl.org/wiki/OpenGL_Loading_Library
    glewExperimental = GL_TRUE;
    if ((err = glewInit()) != GLEW_OK)
    {   // unable to initialize GLEW - OpenGL entry points are not available.
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(gl_rc);
        return CG_BAD_GLCONTEXT;
    }
    if (CG_OPENGL_VERSION_SUPPORTED == GL_FALSE)
    {   // the target OpenGL version is not supported by the driver.
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(gl_rc);
        return CG_BAD_GLCONTEXT;
    }

    // enable synchronization with vertical retrace.
    if (WGLEW_EXT_swap_control)
    {
        if (WGLEW_EXT_swap_control_tear)
        {   // SwapBuffers synchronizes with the vertical retrace interval, 
            // except when the interval is missed, in which case the swap 
            // is performed immediately.
            wglSwapIntervalEXT(-1);
        }
        else
        {   // SwapBuffers synchronizes with the vertical retrace interval.
            wglSwapIntervalEXT(+1);
        }
    }

    // find the OpenCL device that's driving this display.
    for (size_t i = 0, n = ctx->DeviceTable.ObjectCount; i < n; ++i)
    {
        CG_DEVICE            *device  = &ctx->DeviceTable.Objects[i];
        cl_context_properties props[] = 
        {
            CL_CONTEXT_PLATFORM, (cl_context_properties) device->PlatformId, 
            CL_GL_CONTEXT_KHR  , (cl_context_properties) gl_rc, 
            CL_WGL_HDC_KHR     , (cl_context_properties) win_dc,
            0
        };

        // skip CPU devices; they do not drive displays.
        if (device->Type == CL_DEVICE_TYPE_CPU)
            continue;

        // skip devices that don't export cl_khr_gl_sharing.
        if (!cgClIsExtensionSupported("cl_khr_gl_sharing", device->Extensions))
            continue;

        // get the address of the extension function to call:
        clGetGLContextInfoKHR_fn clGetGLContextInfo = (clGetGLContextInfoKHR_fn)
            clGetExtensionFunctionAddressForPlatform(device->PlatformId, "clGetGLContextInfoKHR");
        if (clGetGLContextInfo == NULL)
            continue;

        // finally, get the device ID of the preferred interop device.
        cl_device_id device_id    = NULL;
        size_t       device_size  = 0;
        size_t       device_count = 0;
        clGetGLContextInfo(props, CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, sizeof(cl_device_id), &device_id, &device_size);
        if ((device_count = device_size / sizeof(cl_device_id)) == 0)
            continue; // no interop between this RC and CL device.

        // does the returned value identify this device?
        if (device_id != device->DeviceId)
            continue; // no, so keep looking.

        // we've found a match. 
        if (device->DisplayRC != NULL)
        {   // delete the RC we just created and use the active device RC.
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(gl_rc);
            gl_rc = device->DisplayRC;
        }
        else
        {   // save the rendering context for use with the device.
            device->DisplayRC  = gl_rc;
        }
        // setup display references to the device:
        display->DisplayDevice = device;
        display->DisplayRC     = gl_rc;
        // setup device references to the display:
        device->AttachedDisplays[device->DisplayCount] = display;
        device->DisplayDC[device->DisplayCount] = display->DisplayDC;
        device->DisplayCount++;
        break;
    }
    // if there's no OpenCL device driving the display, that's fine.
    if (display->DisplayDevice == NULL)
        return CG_NO_GLSHARING;
    else
        return CG_SUCCESS;
}

/// @summary Enumerate all enabled display devices on the system. OpenGL contexts are not created.
/// @param ctx The CGFX context to populate with device information.
/// @return Always returns CG_SUCCESS.
internal_function int
cgGlEnumerateDisplays
(
    CG_CONTEXT *ctx
)
{
    DISPLAY_DEVICE dd; dd.cb =  sizeof(DISPLAY_DEVICE);
    HINSTANCE      instance  = (HINSTANCE)&__ImageBase;
    WNDCLASSEX     wndclass  = {};
    size_t    valid_displays = 0;
    bool          check_list = true;

    if (ctx->DisplayTable.ObjectCount == 0)
    {   // no displays have been enumerated, so don't check for duplicates.
        check_list = false;
    }

    // register the window class used for hidden windows, if necessary.
    if (!GetClassInfoEx(instance, CG_OPENGL_HIDDEN_WNDCLASS_NAME, &wndclass))
    {   // the window class has not been registered yet.
        wndclass.cbSize         = sizeof(WNDCLASSEX);
        wndclass.cbClsExtra     = 0;
        wndclass.cbWndExtra     = sizeof(void*);
        wndclass.hInstance      = (HINSTANCE)&__ImageBase;
        wndclass.lpszClassName  = CG_OPENGL_HIDDEN_WNDCLASS_NAME;
        wndclass.lpszMenuName   = NULL;
        wndclass.lpfnWndProc    = DefWindowProc;
        wndclass.hIcon          = LoadIcon  (0, IDI_APPLICATION);
        wndclass.hIconSm        = LoadIcon  (0, IDI_APPLICATION);
        wndclass.hCursor        = LoadCursor(0, IDC_ARROW);
        wndclass.style          = CS_OWNDC;
        wndclass.hbrBackground  = NULL;
        if (!RegisterClassEx(&wndclass))
        {   // unable to register the hidden window class - don't proceed.
            return CG_SUCCESS;
        }
    }

    // enumerate all displays available in the system.
    // displays and OpenGL capability are optional, so don't fail if there's an error.
    for (DWORD ordinal = 0; EnumDisplayDevices(NULL, ordinal, &dd, 0); ++ordinal)
    {   // ignore pseudo-displays and displays that aren't attached to a desktop.
        if ((dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) != 0)
            continue;
        if ((dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) == 0)
            continue;

        // if this display is already known, don't re-initialize it.
        size_t existing = 0;
        if (cgGlDoesDisplayExist(ctx, dd.DeviceName, existing, check_list))
            continue;

        // retrieve display position, orientation and geometry.
        DEVMODE dm;
        memset(&dm, 0, sizeof(DEVMODE));
        dm.dmSize    = sizeof(DEVMODE);
        if (!EnumDisplaySettingsEx(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm, 0))
        {   // try for the registry settings instead.
            if (!EnumDisplaySettingsEx(dd.DeviceName, ENUM_REGISTRY_SETTINGS, &dm, 0))
            {   // unable to retrieve the display settings. skip the display.
                continue;
            }
        }
        int x, y, w, h;
        if (dm.dmFields & DM_POSITION)
        {
            x = dm.dmPosition.x;
            y = dm.dmPosition.y;
        }
        else continue;
        if (dm.dmFields & DM_PELSWIDTH ) w = dm.dmPelsWidth;
        else continue;
        if (dm.dmFields & DM_PELSHEIGHT) h = dm.dmPelsHeight;
        else continue;

        // create a hidden fullscreen window on the display.
        HWND window  = CreateWindowEx(0, CG_OPENGL_HIDDEN_WNDCLASS_NAME, dd.DeviceName, WS_POPUP, x, y, w, h, NULL, NULL, instance, NULL);
        if  (window == NULL)
        {   // unable to create the display window. skip the display.
            continue;
        }
        
        // create the display object.
        CG_DISPLAY           disp;
        disp.Ordinal       = ordinal;
        disp.DisplayDevice = NULL;
        disp.DisplayHWND   = window;
        disp.DisplayDC     = GetDC(window);
        disp.DisplayRC     = NULL;
        disp.DisplayX      = x;
        disp.DisplayY      = y;
        disp.DisplayWidth  = size_t(w);
        disp.DisplayHeight = size_t(h);
        memcpy(&disp.DisplayInfo, &dd, size_t(dd.cb));
        memcpy(&disp.DisplayMode, &dm, size_t(dm.dmSize));
        memset(&disp.GLEW       ,   0, sizeof(GLEWContext));
        memset(&disp.WGLEW      ,   0, sizeof(WGLEWContext));

        // insert the device record into the device table for later access.
        if (cgObjectTableAdd(&ctx->DisplayTable, disp) == CG_INVALID_HANDLE)
        {   // destroy the display object; the display table is full.
            ReleaseDC(window, disp.DisplayDC);
            DestroyWindow(window);
            continue;
        }
    }
    return CG_SUCCESS;
}

/*////////////////////////
//   Public Functions   //
////////////////////////*/
/// @summary Convert a cg_result_e to a string description.
/// @param result One of cg_result_e.
/// @return A pointer to a NULL-terminated ASCII string description. Do not attempt to modify or free the returned string.
library_function char const*
cgResultString
(
    int result
)
{
    switch (result)
    {
    case CG_ERROR:            return "Generic error (CG_ERROR)";
    case CG_INVALID_VALUE:    return "One or more arguments are invalid (CG_INVALID_VALUE)";
    case CG_OUT_OF_MEMORY:    return "Unable to allocate required memory (CG_OUT_OF_MEMORY)";
    case CG_NO_PIXELFORMAT:   return "No accelerated pixel format could be found (CG_NO_PIXELFORMAT)";
    case CG_BAD_PIXELFORMAT:  return "Unable to set the display pixel format (CG_BAD_PIXELFORMAT)";
    case CG_NO_GLCONTEXT:     return "Unable to create an OpenGL rendering context (CG_NO_GLCONTEXT)";
    case CG_BAD_GLCONTEXT:    return "The OpenGL rendering context is invalid (CG_BAD_GLCONTEXT)";
    case CG_SUCCESS:          return "Success (CG_SUCCESS)";
    case CG_UNSUPPORTED:      return "Unsupported operation (CG_UNSUPPORTED)";
    case CG_NOT_READY:        return "Result not ready (CG_NOT_READY)";
    case CG_TIMEOUT:          return "Wait completed due to timeout (CG_TIMEOUT)";
    case CG_SIGNALED:         return "Wait completed due to event signal (CG_SIGNALED)";
    case CG_BUFFER_TOO_SMALL: return "Buffer too small (CG_BUFFER_TOO_SMALL)";
    case CG_NO_OPENCL:        return "No compatible OpenCL platforms or devices available (CG_NO_OPENCL)";
    case CG_NO_OPENGL:        return "No compatible OpenGL devices are available (CG_NO_OPENGL)";
    case CG_NO_GLSHARING:     return "OpenGL rendering context created but OpenCL interop unavailable (CG_NO_GLSHARING)";
    default:                   break;
    }
    return "Unknown Result";
}

/// @summary Register the application with CGFX and enumerate all compute resources and displays in the system.
/// @param app_info Information about the application using CGFX services.
/// @param alloc_cb A custom host memory allocator, or NULL to use the default host memory allocator.
/// @param device_count On return, this value is updated with the number of compute devices available in the system.
/// @param device_list An array to populate with compute device handles, or NULL.
/// @param max_devices The maximum number of device handles that can be written to device_list.
/// @param context If 0, on return, this value is updated with a handle to a new CGFX context; otherwise, it specifies the CGFX context returned by a prior call to cgEnumerateDevices.
/// @return CG_SUCCESS, CG_INVALID_VALUE or CG_UNSUPPORTED if no compatible devices are available.
library_function int
cgEnumerateDevices
(
    cg_application_info_t const *app_info,
    cg_allocation_callbacks_t   *alloc_cb, 
    size_t                      &device_count, 
    cg_handle_t                 *device_list,
    size_t                       max_devices, 
    uintptr_t                   &context
)
{
    int         res = CG_SUCCESS;
    bool    new_ctx = false;
    CG_CONTEXT *ctx = NULL;

    if (context != 0)
    {   // the caller is calling with an existing context to be updated.
        ctx      =(CG_CONTEXT*) context;
        new_ctx  = false;
    }
    else
    {   // allocate a new, empty context for the caller.
        if ((ctx = cgCreateContext(app_info, alloc_cb, res)) == NULL)
        {   // the CGFX context object could not be allocated.
            device_count  = 0;
            context       = 0;
            return res;
        }
        new_ctx  = true;
    }

    // count the number of CPU resources available in the system.
    if ((res = cgGetCpuCounts(ctx)) != CG_SUCCESS)
    {   // typically, memory allocation failed.
        goto error_cleanup;
    }

    // enumerate all displays attached to the system.
    if ((res = cgGlEnumerateDisplays(ctx)) != CG_SUCCESS)
    {   // several things could have gone wrong.
        goto error_cleanup;
    }

    // enumerate all OpenCL platforms and devices in the system.
    if ((res = cgClEnumerateDevices(ctx)) != CG_SUCCESS)
    {   // several things could have gone wrong.
        goto error_cleanup;
    }

    // create rendering contexts for each attached display.
    // this sets up sharing between OpenGL and OpenCL and 
    // establishes device<->display associations.
    for (size_t i = 0, n = ctx->DisplayTable.ObjectCount; i < n; ++i)
    {   // creating rendering contexts could fail for several reasons.
        // we only care about whether the RC is interoperable with CL.
        cgGlCreateRenderingContext(ctx, &ctx->DisplayTable.Objects[i]);
    }

    // update device count and handle tables:
    if (device_list == NULL || max_devices == 0)
    {   // return the total number of compatible devices in the system. 
        device_count = ctx->DeviceTable.ObjectCount;
    }
    else if (device_list != NULL && max_devices > 0)
    {   // return actual device handles to the caller.
        device_count  = 0;
        for (size_t i = 0, n = ctx->DeviceTable.ObjectCount; i < n && i < max_devices; ++i)
        {
            device_list[i] = cgMakeHandle(&ctx->DeviceTable, i);
            device_count++;
        }
    }
    // return the context reference back to the caller.
    context = (uintptr_t) ctx;
    return res;

error_cleanup:
    if (new_ctx) cgDeleteContext(ctx);
    device_count = 0;
    return res;
}

/// @summary Query context-level information.
/// @param context A CGFX context returned by cgEnumerateDevices().
/// @param param One of cg_context_param_e specifying the data to return.
/// @param buffer A caller-managed buffer to receive the data.
/// @param buffer_size The maximum number of bytes that can be written to @a buffer.
/// @param bytes_needed On return, stores the number of bytes copied to the buffer, or the number of bytes required to store the data.
/// @return CG_SUCCESS, CG_BUFFER_TOO_SMALL, CG_INVALID_VALUE or CG_OUT_OF_MEMORY.
library_function int
cgGetContextInfo
(
    uintptr_t        context,
    int              param, 
    void            *buffer, 
    size_t           buffer_size, 
    size_t          *bytes_needed
)
{
    CG_CONTEXT *ctx = (CG_CONTEXT*) context;

    // reduce the amount of boilerplate code using these macros.
#define BUFFER_CHECK_TYPE(type) \
    if (buffer == NULL || buffer_size < sizeof(type)) \
    { \
        if (bytes_needed != NULL) *bytes_needed = sizeof(type); \
        return CG_BUFFER_TOO_SMALL; \
    } \
    else if (bytes_needed != NULL) \
    { \
        *bytes_needed = sizeof(type); \
    }
#define BUFFER_CHECK_SIZE(size) \
    if (buffer == NULL || buffer_size < (size)) \
    { \
        if (bytes_needed != NULL) *bytes_needed = (size); \
        return CG_BUFFER_TOO_SMALL; \
    } \
    else if (bytes_needed != NULL) \
    { \
        *bytes_needed = (size); \
    }
    // ==========================================================
    // ==========================================================

    // retrieve parameter data:
    switch (param)
    {
    case CG_CONTEXT_CPU_COUNTS:
        {   BUFFER_CHECK_TYPE(cg_cpu_counts_t);
            memcpy(buffer, &ctx->CPUCounts, sizeof(cg_cpu_counts_t));
        }
        return CG_SUCCESS;

    case CG_CONTEXT_DEVICE_COUNT:
        {   BUFFER_CHECK_TYPE(size_t);
            *((size_t*) buffer) = ctx->DeviceTable.ObjectCount;
        }
        return CG_SUCCESS;

    case CG_CONTEXT_DISPLAY_COUNT:
        {   BUFFER_CHECK_TYPE(size_t);
            *((size_t*) buffer) = ctx->DisplayTable.ObjectCount;
        }
        return CG_SUCCESS;

    default:
        {
            if (bytes_needed != NULL) *bytes_needed = 0;
            return CG_INVALID_VALUE;
        }
    }

#undef  BUFFER_CHECK_SIZE
#undef  BUFFER_CHECK_TYPE
}

/// @summary Retrieve the number of compute devices found by cgEnumerateDevices.
/// @param context The CGFX context returned by a prior call to cgEnumerateDevices().
/// @return The number of OpenCL 1.2-capable compute devices.
library_function size_t
cgGetDeviceCount
(
    uintptr_t context
)
{
    CG_CONTEXT *ctx = (CG_CONTEXT*) context;
    return ctx->DeviceTable.ObjectCount;
}

/// @summary Query device-level information.
/// @param context A CGFX context returned by cgEnumerateDevices().
/// @param device_handle The handle of the device to query.
/// @param param One of cg_context_param_e specifying the data to return.
/// @param buffer A caller-managed buffer to receive the data.
/// @param buffer_size The maximum number of bytes that can be written to @a buffer.
/// @param bytes_needed On return, stores the number of bytes copied to the buffer, or the number of bytes required to store the data.
/// @return CG_SUCCESS, CG_BUFFER_TOO_SMALL, CG_INVALID_VALUE or CG_OUT_OF_MEMORY.
library_function int
cgGetDeviceInfo
(
    uintptr_t   context, 
    cg_handle_t device_handle, 
    int         param,
    void       *data, 
    size_t      buffer_size, 
    size_t     *bytes_needed
)
{
    CG_CONTEXT *ctx    = (CG_CONTEXT*) context;
    CG_DEVICE  *device =  NULL;

    if ((device = cgObjectTableGet(&ctx->DeviceTable, device_handle)) == NULL)
    {
        if (bytes_needed != NULL) *bytes_needed = 0;
        return CG_INVALID_HANDLE;
    }

    // reduce the amount of boilerplate code using these macros.
#define BUFFER_CHECK_TYPE(type) \
    if (buffer == NULL || buffer_size < sizeof(type)) \
    { \
        if (bytes_needed != NULL) *bytes_needed = sizeof(type); \
        return CG_BUFFER_TOO_SMALL; \
    } \
    else if (bytes_needed != NULL) \
    { \
        *bytes_needed = sizeof(type); \
    }
#define BUFFER_CHECK_SIZE(size) \
    if (buffer == NULL || buffer_size < (size)) \
    { \
        if (bytes_needed != NULL) *bytes_needed = (size); \
        return CG_BUFFER_TOO_SMALL; \
    } \
    else if (bytes_needed != NULL) \
    { \
        *bytes_needed = (size); \
    }
    // ==========================================================
    // ==========================================================

    // retrieve parameter data:
    switch (param)
    {
    // get cl_platform_id
    // get cl_device_id
    // get cl_context
    // get cl_command_queue
    // get HGLRC
    // get attached display count
    // get attached display IDs
    default:
        {
            if (bytes_needed != NULL) *bytes_needed = 0;
            return CG_INVALID_VALUE;
        }
    }

#undef  BUFFER_CHECK_SIZE
#undef  BUFFER_CHECK_TYPE
}

/// @summary Retrieve the number of displays found by cgEnumerateDevices.
/// @param context The CGFX context returned by a prior call to cgEnumerateDevices().
/// @return The number of attached displays.
library_function size_t
cgGetDisplayCount
(
    uintptr_t context
)
{
    CG_CONTEXT *ctx = (CG_CONTEXT*)context;
    return ctx->DisplayTable.ObjectCount;
}

/// @summary Retrieve the handle of the primary display.
/// @param context The CGFX context returned by a prior call to cgEnumerateDevices().
/// @return The display handle, or CG_INVALID_HANDLE if no displays are attached.
library_function cg_handle_t
cgGetPrimaryDisplay
(
    uintptr_t context
)
{
    CG_CONTEXT *ctx = (CG_CONTEXT*)context;
    for (size_t i = 0, n = ctx->DisplayTable.ObjectCount; i < n; ++i)
    {
        CG_DISPLAY *display = &ctx->DisplayTable.Objects[i];
        if (display->DisplayInfo.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
            return cgMakeHandle(&ctx->DisplayTable, i);
    }
    return CG_INVALID_HANDLE;
}

/// @summary Retrieves the handle of a display identified by ordinal value.
/// @param context The CGFX context returned by a prior call to cgEnumerateDevices().
/// @param ordinal The zero-based index of the display to retrieve.
/// @return The display handle, or CG_INVALID_HANDLE if the supplied ordinal is invalid.
library_function cg_handle_t
cgGetDisplayByOrdinal
(
    uintptr_t context, 
    size_t    ordinal
)
{
    CG_CONTEXT   *ctx = (CG_CONTEXT*)context;
    if (ordinal < ctx->DisplayTable.ObjectCount)
        return cgMakeHandle(&ctx->DisplayTable, ordinal);
    else
        return CG_INVALID_HANDLE;
}

/// @summary Query display information.
/// @param context A CGFX context returned by cgEnumerateDevices().
/// @param display_handle The handle of the display to query.
/// @param param One of cg_context_param_e specifying the data to return.
/// @param buffer A caller-managed buffer to receive the data.
/// @param buffer_size The maximum number of bytes that can be written to @a buffer.
/// @param bytes_needed On return, stores the number of bytes copied to the buffer, or the number of bytes required to store the data.
/// @return CG_SUCCESS, CG_BUFFER_TOO_SMALL, CG_INVALID_VALUE or CG_OUT_OF_MEMORY.
library_function int
cgGetDisplayInfo
(
    uintptr_t   context, 
    cg_handle_t display_handle, 
    int         param,
    void       *data, 
    size_t      buffer_size, 
    size_t     *bytes_needed
)
{
    CG_CONTEXT *ctx     = (CG_CONTEXT*) context;
    CG_DISPLAY *display =  NULL;

    if ((display = cgObjectTableGet(&ctx->DisplayTable, display_handle)) == NULL)
    {
        if (bytes_needed != NULL) *bytes_needed = 0;
        return CG_INVALID_HANDLE;
    }

    // reduce the amount of boilerplate code using these macros.
#define BUFFER_CHECK_TYPE(type) \
    if (buffer == NULL || buffer_size < sizeof(type)) \
    { \
        if (bytes_needed != NULL) *bytes_needed = sizeof(type); \
        return CG_BUFFER_TOO_SMALL; \
    } \
    else if (bytes_needed != NULL) \
    { \
        *bytes_needed = sizeof(type); \
    }
#define BUFFER_CHECK_SIZE(size) \
    if (buffer == NULL || buffer_size < (size)) \
    { \
        if (bytes_needed != NULL) *bytes_needed = (size); \
        return CG_BUFFER_TOO_SMALL; \
    } \
    else if (bytes_needed != NULL) \
    { \
        *bytes_needed = (size); \
    }
    // ==========================================================
    // ==========================================================

    // retrieve parameter data:
    switch (param)
    {
    // get cg_handle_t of shared OpenCL device
    // get cl_platform_id of shared OpenCL device
    // get cl_device_id of shared OpenCL device
    // get HDC
    // get HGLRC
    // get bounding rectangle
    // get orientation
    default:
        {
            if (bytes_needed != NULL) *bytes_needed = 0;
            return CG_INVALID_VALUE;
        }
    }

#undef  BUFFER_CHECK_SIZE
#undef  BUFFER_CHECK_TYPE
}
