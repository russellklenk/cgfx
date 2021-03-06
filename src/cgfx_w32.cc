/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implements the core CGFX API functions for Microsoft Windows.
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////////
//   Preprocessor   //
////////////////////*/
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

/// @summary Somewhere in our include chain, someone includes <dxgiformat.h>.
/// Don't re-define the DXGI_FORMAT and D3D11 enumerations.
#define CG_DXGI_ENUMS_DEFINED 1

/*////////////////
//   Includes   //
////////////////*/
#include "cgfx.h"
#include "cgfx_ext_win.h"
#include "cgfx_w32_private.h"

#include "glew.c"

#undef   glewGetContext
#define  glewGetContext()    (&display->GLEW)

#undef   wglewGetContext
#define  wglewGetContext()   (&display->WGLEW)

#include "icd.c"
#include "icd_dispatch.c"
#include "icd_windows.c"

/*////////////////////////////
//   Forward Declarations   //
////////////////////////////*/

/*/////////////////
//   Constants   //
/////////////////*/

/*///////////////////
//   Local Types   //
///////////////////*/

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
/// @summary Implements a no-op host memory allocator that always returns NULL.
/// @param size The desired allocation size, in bytes.
/// @param alignment The required alignment of the returned address, in bytes.
/// @param type One of uiss_allocation_type_e.
/// @param user_data Opaque data specified when the allocator implementation was registered.
/// @return A pointer to the host memory block, or NULL.
internal_function void* CG_API
cgHostMemAllocNoOp
(
    size_t    size,
    size_t    alignment,
    int       type,
    uintptr_t user_data
)
{
    UNREFERENCED_PARAMETER(size);
    UNREFERENCED_PARAMETER(alignment);
    UNREFERENCED_PARAMETER(type);
    UNREFERENCED_PARAMETER(user_data);
    return NULL;
}

/// @summary Implements a host memory allocator using the standard malloc function.
/// @param size The desired allocation size, in bytes.
/// @param alignment The required alignment of the returned address, in bytes.
/// @param type One of uiss_allocation_type_e.
/// @param user_data Opaque data specified when the allocator implementation was registered.
/// @return A pointer to the host memory block, or NULL.
internal_function void* CG_API
cgHostMemAllocStdC
(
    size_t    size,
    size_t    alignment,
    int       type,
    uintptr_t user_data
)
{
    UNREFERENCED_PARAMETER(type);
    UNREFERENCED_PARAMETER(user_data);
    if (alignment <= 1) return malloc(size);
    else return _aligned_malloc(size, alignment);
}

/// @summary Implements a no-op host memory release function.
/// @param address The address of the memory block to free, as returned by the allocation function.
/// @param size The requested size of the allocated block, in bytes.
/// @param alignment The required alignment of the allocated block, in bytes.
/// @param type One of uiss_allocation_type_e.
/// @param user_data Opaque data specified when the allocator implementation was registered.
internal_function void CG_API
cgHostMemFreeNoOp
(
    void     *address,
    size_t    size,
    size_t    alignment,
    int       type,
    uintptr_t user_data
)
{
    UNREFERENCED_PARAMETER(address);
    UNREFERENCED_PARAMETER(size);
    UNREFERENCED_PARAMETER(alignment);
    UNREFERENCED_PARAMETER(type);
    UNREFERENCED_PARAMETER(user_data);
}

/// @summary Implements a host memory release function using the standard free function.
/// @param address The address of the memory block to free, as returned by the allocation function.
/// @param size The requested size of the allocated block, in bytes.
/// @param alignment The required alignment of the allocated block, in bytes.
/// @param type One of uiss_allocation_type_e.
/// @param user_data Opaque data specified when the allocator implementation was registered.
internal_function void CG_API
cgHostMemFreeStdC
(
    void     *address,
    size_t    size,
    size_t    alignment,
    int       type,
    uintptr_t user_data
)
{
    UNREFERENCED_PARAMETER(size);
    UNREFERENCED_PARAMETER(type);
    UNREFERENCED_PARAMETER(user_data);
    if (alignment <= 1) return free(address);
    else return _aligned_free(address);
}

/// @summary Validate and initialize a user-defined memory allocator.
/// @param alloc_cb The user-supplied host memory allocation callbacks. Specify NULL to fall back to malloc and free.
/// @param alloc_fn The memory allocator state to initialize.
/// @return CG_SUCCESS, or CG_INVALID_VALUE if one or more inputs are invalid.
internal_function int
cgSetupUserHostAllocator
(
    cg_allocation_callbacks_t *alloc_cb,
    CG_HOST_ALLOCATOR         *alloc_fn
)
{
    if (alloc_fn == NULL)
    {   // no user allocator instance supplied.
        return CG_INVALID_VALUE;
    }
    if (alloc_fn->Initialized)
    {   // the new values must exactly match the current values.
        if (alloc_cb == NULL)
        {   // check against the default allocator implementation.
            if (alloc_fn->Allocate == cgHostMemAllocStdC &&
                alloc_fn->Release  == cgHostMemFreeStdC  &&
                alloc_fn->UserData ==(uintptr_t) 0)
            {   // the setup matches.
                return CG_SUCCESS;
            }
            else return CG_INVALID_VALUE;
        }
        else
        {   // check against the current user allocator implementation.
            if (alloc_fn->Allocate == alloc_cb->Allocate &&
                alloc_fn->Release  == alloc_cb->Release  &&
                alloc_fn->UserData == alloc_cb->UserData)
            {   // the setup matches.
                return CG_SUCCESS;
            }
            else return CG_INVALID_VALUE;
        }
    }
    if (alloc_cb == NULL)
    {   // use the default allocator implementation.
        alloc_fn->Allocate      = cgHostMemAllocStdC;
        alloc_fn->Release       = cgHostMemFreeStdC;
        alloc_fn->UserData      =(uintptr_t) 0;
        alloc_fn->Initialized   = true;
        return CG_SUCCESS;
    }
    else
    {   // initialize with a user-supplied allocator implementation.
        if (alloc_cb->Allocate  == NULL) return CG_INVALID_VALUE;
        if (alloc_cb->Release   == NULL) return CG_INVALID_VALUE;
        alloc_fn->Allocate       = alloc_cb->Allocate;
        alloc_fn->Release        = alloc_cb->Release;
        alloc_fn->UserData       = alloc_cb->UserData;
        alloc_fn->Initialized    = true;
        return CG_SUCCESS;
    }
}

/// @summary Delete a host allocator by setting it up to reference the no-op allocator implementation. All calls to allocate memory will fail.
/// @param host The host allocator state to delete.
internal_function void
cgDeleteUserHostAllocator
(
    CG_HOST_ALLOCATOR *host
)
{
    host->Allocate = cgHostMemAllocNoOp;
    host->Release  = cgHostMemFreeNoOp;
    host->UserData =(uintptr_t) 0;
}

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

/// @summary Duplicate a string, allocating memory through a CG_CONTEXT.
/// @param ctx The CG_CONTEXT to use for memory allocation.
/// @param str A NULL-terminated string to copy.
/// @param type The CGFX allocation type, one of cg_allocation_type_e.
/// @return A pointer to the duplicate string, or NULL. Free with cgClFreeString.
internal_function char*
cgStrdup
(
    CG_CONTEXT *ctx,
    char const *str,
    int         type
)
{
    if (str == NULL)
    {   // input was NULL, return NULL.
        return NULL;
    }
    size_t len = strlen(str) + 1;
    char  *mem =(char*) cgAllocateHostMemory(&ctx->HostAllocator, len, 0, type);
    if    (mem != NULL) memcpy(mem, str, len+1);
    return mem;
}

/// @summary Generates a little-endian FOURCC.
/// @param a...d The four characters comprising the code.
/// @return The packed four-cc value, in little-endian format.
internal_function inline uint32_t 
cgFourCC_le
(
    char a, 
    char b, 
    char c, 
    char d
)
{
    uint32_t A = (uint32_t) a;
    uint32_t B = (uint32_t) b;
    uint32_t C = (uint32_t) c;
    uint32_t D = (uint32_t) d;
    return ((D << 24) | (C << 16) | (B << 8) | (A << 0));
}

/// @summary Generates a big-endian FOURCC.
/// @param a...d The four characters comprising the code.
/// @return The packed four-cc value, in big-endian format.
internal_function inline uint32_t
cgFourCC_be
(
    char a, 
    char b, 
    char c, 
    char d
)
{
    uint32_t A = (uint32_t) a;
    uint32_t B = (uint32_t) b;
    uint32_t C = (uint32_t) c;
    uint32_t D = (uint32_t) d;
    return ((A << 24) | (B << 16) | (C << 8) | (D << 0));
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

/// @summary Retrieve the length of the OpenCL kernel argument name, allocate a buffer and store a copy of the value.
/// @param ctx The CGFX context requesting the string data.
/// @param kernel The OpenCL kernel object handle being queried.
/// @param index The zero-based index of the kernel argument being queried.
/// @param type The CGFX allocation type, one of cg_allocation_type_e.
/// @return A pointer to the NULL-terminated ASCII string, or NULL.
internal_function char*
cgClKernelArgName
(
    CG_CONTEXT *ctx,
    cl_kernel   kernel,
    cl_uint     index,
    int         type
)
{
    size_t nbytes = 0;
    char  *buffer = NULL;
    clGetKernelArgInfo(kernel, index, CL_KERNEL_ARG_NAME, 0, NULL, &nbytes);
    if ((buffer = (char*) cgAllocateHostMemory(&ctx->HostAllocator, nbytes, 0, type)) != NULL)
    {   clGetKernelArgInfo(kernel, index, CL_KERNEL_ARG_NAME, nbytes, buffer, NULL);
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
{   UNREFERENCED_PARAMETER(ctx);
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

/// @summary Frees all resources and releases all references held by a queue object.
/// @param ctx The CGFX context that owns the queue object.
/// @param queue The queue object to delete.
internal_function void
cgDeleteQueue
(
    CG_CONTEXT *ctx,
    CG_QUEUE   *queue
)
{   UNREFERENCED_PARAMETER(ctx);
    if (queue->CommandQueue != NULL)
        clReleaseCommandQueue(queue->CommandQueue);
    memset(queue, 0, sizeof(CG_QUEUE));
}

/// @summary Frees all resources associated with a command buffer object.
/// @param ctx The CGFX context that owns the command buffer object.
/// @param cmdbuf The command buffer object to delete.
internal_function void
cgDeleteCmdBuffer
(
    CG_CONTEXT    *ctx,
    CG_CMD_BUFFER *cmdbuf
)
{   UNREFERENCED_PARAMETER(ctx);
    if (cmdbuf->CommandData != NULL)
        VirtualFree(cmdbuf->CommandData, 0, MEM_RELEASE);
    cmdbuf->BytesTotal   = 0;
    cmdbuf->BytesUsed    = 0;
    cmdbuf->CommandCount = 0;
    cmdbuf->CommandData  = NULL;
}

/// @summary Frees all resources and releases all references associated with a kernel object.
/// @param ctx The CGFX context that owns the kernel object.
/// @param kernel The kernel object to delete.
internal_function void
cgDeleteKernel
(
    CG_CONTEXT *ctx,
    CG_KERNEL  *kernel
)
{   UNREFERENCED_PARAMETER(ctx);
    switch (kernel->KernelType)
    {
    case CG_KERNEL_TYPE_GRAPHICS_VERTEX:
    case CG_KERNEL_TYPE_GRAPHICS_FRAGMENT:
    case CG_KERNEL_TYPE_GRAPHICS_PRIMITIVE:
        {
            CG_DISPLAY *display = kernel->AttachedDisplay;
            if (kernel->GraphicsShader != 0)
            {
                glDeleteShader(kernel->GraphicsShader);
            }
        }
        break;
    case CG_KERNEL_TYPE_COMPUTE:
        {
            if (kernel->ComputeProgram != NULL)
            {
                clReleaseProgram(kernel->ComputeProgram);
            }
        }
        break;
    default:
        break;
    }
    memset(kernel, 0, sizeof(CG_KERNEL));
}

/// @summary Allocate memory for an execution group and initialize the device and display lists.
/// @param ctx The CGFX context that owns the execution group.
/// @param group The execution group to initialize.
/// @param root_device The handle of the device that defines the share group.
/// @param device_list The list of handles of the devices in the execution group.
/// @param device_count The number of devices in the execution group.
/// @return CG_SUCCESS or CG_OUT_OF_MEMORY.
internal_function int
cgAllocExecutionGroup
(
    CG_CONTEXT    *ctx,
    CG_EXEC_GROUP *group,
    cg_handle_t    root_device,
    cg_handle_t   *device_list,
    size_t         device_count
)
{
    CG_DEVICE        *root             = cgObjectTableGet(&ctx->DeviceTable, root_device);
    CG_DEVICE       **device_refs      = NULL;
    cl_device_id     *device_ids       = NULL;
    CG_QUEUE        **compute_queues   = NULL;
    CG_QUEUE        **transfer_queues  = NULL;
    CG_DISPLAY      **display_refs     = NULL;
    CG_QUEUE        **graphics_queues  = NULL;
    CG_QUEUE        **queue_refs       = NULL;
    size_t            display_count    = 0;
    size_t            queue_count      = 0;

    // zero out the execution groupn definition.
    memset(group, 0, sizeof(CG_EXEC_GROUP));

    // determine the number of attached displays and queues.
    for (size_t i = 0; i < device_count; ++i)
    {
        CG_DEVICE *device = cgObjectTableGet(&ctx->DeviceTable, device_list[i]);
        display_count    += device->DisplayCount;
    }
    queue_count += device_count;  // account for compute queues
    queue_count += device_count;  // account for transfer queues
    queue_count += display_count; // account for graphics queues

    // allocate storage. there's always at least one device, but possibly no attached displays.
    if (device_count > 0)
    {   // allocate all of the device list storage.
        device_refs      = (CG_DEVICE   **) cgAllocateHostMemory(&ctx->HostAllocator, device_count  * sizeof(CG_DEVICE*)  , 0, CG_ALLOCATION_TYPE_OBJECT);
        device_ids       = (cl_device_id *) cgAllocateHostMemory(&ctx->HostAllocator, device_count  * sizeof(cl_device_id), 0, CG_ALLOCATION_TYPE_OBJECT);
        compute_queues   = (CG_QUEUE    **) cgAllocateHostMemory(&ctx->HostAllocator, device_count  * sizeof(CG_QUEUE*)   , 0, CG_ALLOCATION_TYPE_OBJECT);
        transfer_queues  = (CG_QUEUE    **) cgAllocateHostMemory(&ctx->HostAllocator, device_count  * sizeof(CG_QUEUE*)   , 0, CG_ALLOCATION_TYPE_OBJECT);
        if (device_refs == NULL || device_ids == NULL || transfer_queues == NULL)
            goto error_cleanup;
        // populate the device reference list and initialize everything else to NULL.
        for (size_t device_index = 0; device_index < device_count; ++device_index)
        {
            device_refs[device_index] = cgObjectTableGet(&ctx->DeviceTable, device_list[device_index]);
            device_ids [device_index] = device_refs[device_index]->DeviceId;
        }
        memset(compute_queues  , 0, device_count * sizeof(CG_QUEUE*));
        memset(transfer_queues , 0, device_count * sizeof(CG_QUEUE*));
    }
    if (display_count > 0)
    {   // allocate all of the display list storage.
        display_refs     = (CG_DISPLAY**) cgAllocateHostMemory(&ctx->HostAllocator, display_count * sizeof(CG_DISPLAY*), 0, CG_ALLOCATION_TYPE_OBJECT);
        graphics_queues  = (CG_QUEUE  **) cgAllocateHostMemory(&ctx->HostAllocator, display_count * sizeof(CG_QUEUE*)  , 0, CG_ALLOCATION_TYPE_OBJECT);
        if (display_refs == NULL || graphics_queues == NULL)
            goto error_cleanup;
        // populate the display list with data.
        for (size_t device_index = 0; device_index < device_count; ++device_index)
        {
            CG_DEVICE  *device = cgObjectTableGet(&ctx->DeviceTable, device_list[device_index]);
            for (size_t display_index = 0, num_displays = device->DisplayCount; display_index < num_displays; ++display_index)
            {
                display_refs[group->DisplayCount] = device->AttachedDisplays[display_index];
                group->DisplayCount++;
            }
        }
        memset(graphics_queues, 0, display_count * sizeof(CG_QUEUE*));
    }
    if (queue_count > 0)
    {   // allocate all of the queue list storage.
        queue_refs  = (CG_QUEUE**) cgAllocateHostMemory(&ctx->HostAllocator, queue_count * sizeof(CG_QUEUE*), 0, CG_ALLOCATION_TYPE_OBJECT);
        if (queue_refs == NULL)
            goto error_cleanup;
        // NULL out all of the queue references.
        memset(queue_refs, 0, queue_count * sizeof(CG_QUEUE*));
    }

    // initialization was successful, save all of the references.
    group->PlatformId       = root->PlatformId;
    group->ComputeContext   = NULL;
    group->RenderingContext = NULL;
    group->AttachedDisplay  = NULL;
    group->DeviceCount      = device_count;
    group->DeviceList       = device_refs;
    group->DeviceIds        = device_ids;
    group->ComputeQueues    = compute_queues;
    group->TransferQueues   = transfer_queues;
    group->DisplayCount     = display_count;
    group->AttachedDisplays = display_refs;
    group->GraphicsQueues   = graphics_queues;
    group->QueueCount       = queue_count;
    group->QueueList        = queue_refs;
    return CG_SUCCESS;

error_cleanup:
    cgFreeHostMemory(&ctx->HostAllocator, queue_refs      , queue_count   * sizeof(CG_QUEUE*)   , 0, CG_ALLOCATION_TYPE_OBJECT);
    cgFreeHostMemory(&ctx->HostAllocator, graphics_queues , display_count * sizeof(CG_QUEUE*)   , 0, CG_ALLOCATION_TYPE_OBJECT);
    cgFreeHostMemory(&ctx->HostAllocator, display_refs    , display_count * sizeof(CG_DISPLAY*) , 0, CG_ALLOCATION_TYPE_OBJECT);
    cgFreeHostMemory(&ctx->HostAllocator, transfer_queues , device_count  * sizeof(CG_QUEUE*)   , 0, CG_ALLOCATION_TYPE_OBJECT);
    cgFreeHostMemory(&ctx->HostAllocator, compute_queues  , device_count  * sizeof(CG_QUEUE*)   , 0, CG_ALLOCATION_TYPE_OBJECT);
    cgFreeHostMemory(&ctx->HostAllocator, device_ids      , device_count  * sizeof(cl_device_id), 0, CG_ALLOCATION_TYPE_OBJECT);
    cgFreeHostMemory(&ctx->HostAllocator, device_refs     , device_count  * sizeof(CG_DEVICE*)  , 0, CG_ALLOCATION_TYPE_OBJECT);
    return CG_OUT_OF_MEMORY;
}

/// @summary Frees all resources and releases all references held by an execution group object.
/// @param ctx The CGFX context that owns the execution group object.
/// @param group The execution group object to delete.
internal_function void
cgDeleteExecutionGroup
(
    CG_CONTEXT    *ctx,
    CG_EXEC_GROUP *group
)
{
    CG_HOST_ALLOCATOR *host_alloc = &ctx->HostAllocator;
    // release device references back to the execution group.
    for (size_t i = 0, n = group->DeviceCount; i < n; ++i)
    {
        group->DeviceList[i]->ExecutionGroup = CG_INVALID_HANDLE;
    }
    if (group->ComputeContext != NULL)
    {
        clReleaseContext(group->ComputeContext);
    }
    cgFreeHostMemory(host_alloc, group->QueueList       , group->QueueCount   * sizeof(CG_QUEUE*)   , 0, CG_ALLOCATION_TYPE_OBJECT);
    cgFreeHostMemory(host_alloc, group->GraphicsQueues  , group->DisplayCount * sizeof(CG_QUEUE*)   , 0, CG_ALLOCATION_TYPE_OBJECT);
    cgFreeHostMemory(host_alloc, group->AttachedDisplays, group->DisplayCount * sizeof(CG_DISPLAY*) , 0, CG_ALLOCATION_TYPE_OBJECT);
    cgFreeHostMemory(host_alloc, group->TransferQueues  , group->DeviceCount  * sizeof(CG_QUEUE*)   , 0, CG_ALLOCATION_TYPE_OBJECT);
    cgFreeHostMemory(host_alloc, group->ComputeQueues   , group->DisplayCount * sizeof(CG_QUEUE*)   , 0, CG_ALLOCATION_TYPE_OBJECT);
    cgFreeHostMemory(host_alloc, group->DeviceIds       , group->DeviceCount  * sizeof(cl_device_id), 0, CG_ALLOCATION_TYPE_OBJECT);
    cgFreeHostMemory(host_alloc, group->DeviceList      , group->DeviceCount  * sizeof(CG_DEVICE*)  , 0, CG_ALLOCATION_TYPE_OBJECT);
    memset(group, 0, sizeof(CG_EXEC_GROUP));
}

/// @summary Frees all resources associated with a compute pipeline object.
/// @param ctx The CGFX context that owns the compute pipeline object.
/// @param pipeline The compute pipeline object to delete.
internal_function void
cgDeleteComputePipeline
(
    CG_CONTEXT          *ctx,
    CG_COMPUTE_PIPELINE *pipeline
)
{
    if (pipeline->ComputeKernel != NULL)
    {
        clReleaseKernel(pipeline->ComputeKernel);
    }
    cgFreeHostMemory(&ctx->HostAllocator, pipeline->Arguments       , pipeline->ArgumentCount * sizeof(CG_CL_KERNEL_ARG)    , 0, CG_ALLOCATION_TYPE_OBJECT);
    cgFreeHostMemory(&ctx->HostAllocator, pipeline->ArgumentNames   , pipeline->ArgumentCount * sizeof(uint32_t)            , 0, CG_ALLOCATION_TYPE_OBJECT);
    cgFreeHostMemory(&ctx->HostAllocator, pipeline->DeviceKernelInfo, pipeline->DeviceCount   * sizeof(CG_CL_WORKGROUP_INFO), 0, CG_ALLOCATION_TYPE_OBJECT);
    memset(pipeline, 0, sizeof(CG_COMPUTE_PIPELINE));
}

/// @summary Frees all resources associated with a graphics pipeline object.
/// @param ctx The CGFX context that owns the graphics pipeline object.
/// @param pipeline The graphics pipeline object to delete.
internal_function void
cgDeleteGraphicsPipeline
(
    CG_CONTEXT           *ctx,
    CG_GRAPHICS_PIPELINE *pipeline
)
{
    CG_DISPLAY   *display = pipeline->AttachedDisplay;
    CG_GLSL_PROGRAM &glsl = pipeline->ShaderProgram;
    if (glsl.Program != 0)
    {
        glDeleteProgram(glsl.Program);
    }
    cgFreeHostMemory(&ctx->HostAllocator, glsl.Samplers      , glsl.SamplerCount   * sizeof(CG_GLSL_SAMPLER)  , 0, CG_ALLOCATION_TYPE_OBJECT);
    cgFreeHostMemory(&ctx->HostAllocator, glsl.SamplerNames  , glsl.SamplerCount   * sizeof(uint32_t)         , 0, CG_ALLOCATION_TYPE_OBJECT);
    cgFreeHostMemory(&ctx->HostAllocator, glsl.Attributes    , glsl.AttributeCount * sizeof(CG_GLSL_ATTRIBUTE), 0, CG_ALLOCATION_TYPE_OBJECT);
    cgFreeHostMemory(&ctx->HostAllocator, glsl.AttributeNames, glsl.AttributeCount * sizeof(uint32_t)         , 0, CG_ALLOCATION_TYPE_OBJECT);
    cgFreeHostMemory(&ctx->HostAllocator, glsl.Uniforms      , glsl.UniformCount   * sizeof(CG_GLSL_UNIFORM)  , 0, CG_ALLOCATION_TYPE_OBJECT);
    cgFreeHostMemory(&ctx->HostAllocator, glsl.UniformNames  , glsl.UniformCount   * sizeof(uint32_t)         , 0, CG_ALLOCATION_TYPE_OBJECT);
    memset(pipeline, 0, sizeof(CG_GRAPHICS_PIPELINE));
}

/// @summary Frees all resources associated with a pipeline object.
/// @param ctx The CGFX context that owns the pipeline object.
/// @param pipeline The pipeline object to delete.
internal_function void
cgDeletePipeline
(
    CG_CONTEXT  *ctx, 
    CG_PIPELINE *pipeline
)
{   // clean up the internal pipeline state first:
    pipeline->DestroyState((uintptr_t) ctx, (uintptr_t) pipeline, pipeline->PrivateState);
    // then clean up the state common to all implementations:
    switch (pipeline->PipelineType)
    {
    case CG_PIPELINE_TYPE_COMPUTE:
        cgDeleteComputePipeline(ctx, &pipeline->Compute);
        break;
    case CG_PIPELINE_TYPE_GRAPHICS:
        cgDeleteGraphicsPipeline(ctx, &pipeline->Graphics);
        break;
    default:
        break;
    }
}

/// @summary Frees all resources associated with a buffer memory object.
/// @param ctx The CGFX context that owns the buffer object.
/// @param buffer The buffer object to delete.
internal_function void
cgDeleteBuffer
(
    CG_CONTEXT *ctx, 
    CG_BUFFER  *buffer
)
{   UNREFERENCED_PARAMETER(ctx);
    if (buffer->ComputeBuffer != NULL)
    {
        clReleaseMemObject(buffer->ComputeBuffer);
    }
    if (buffer->GraphicsBuffer != 0)
    {
        CG_DISPLAY *display = buffer->AttachedDisplay;
        glDeleteBuffers(1, &buffer->GraphicsBuffer);
    }
    memset(buffer, 0, sizeof(CG_BUFFER));
}

/// @summary Frees all resources associated with a fence object.
/// @param ctx The CGFX context that owns the fence object.
/// @param fence The fence object to delete.
internal_function void
cgDeleteFence
(
    CG_CONTEXT *ctx, 
    CG_FENCE   *fence
)
{   UNREFERENCED_PARAMETER(ctx);
    switch (fence->QueueType)
    {
    case CG_QUEUE_TYPE_COMPUTE:
    case CG_QUEUE_TYPE_TRANSFER:
    case CG_QUEUE_TYPE_COMPUTE_OR_TRANSFER:
        if (fence->ComputeFence != NULL)
        {
            clReleaseEvent(fence->ComputeFence);
        }
        break;
    case CG_QUEUE_TYPE_GRAPHICS:
        if (fence->GraphicsFence != NULL)
        {
            CG_DISPLAY *display = fence->AttachedDisplay;
            glDeleteSync(fence->GraphicsFence);
        }
        break;
    default:
        break;
    }
    memset(fence, 0, sizeof(CG_FENCE));
}

/// @summary Frees all resources associated with an event object.
/// @param ctx The CGFX context that owns the event object.
/// @param event The event object to delete.
internal_function void
cgDeleteEvent
(
    CG_CONTEXT *ctx, 
    CG_EVENT   *event
)
{   UNREFERENCED_PARAMETER(ctx);
    if (event->ComputeEvent != NULL)
    {
        clReleaseEvent(event->ComputeEvent);
    }
    memset(event, 0, sizeof(CG_EVENT));
}

/// @summary Frees all resources associated with an image object.
/// @param ctx The CGFX context that owns the image object.
/// @param image The image object to delete.
internal_function void
cgDeleteImage
(
    CG_CONTEXT *ctx, 
    CG_IMAGE   *image
)
{   UNREFERENCED_PARAMETER(ctx);
    if (image->ComputeImage != NULL)
    {
        clReleaseMemObject(image->ComputeImage);
    }
    if (image->GraphicsImage != 0)
    {
        CG_DISPLAY *display = image->AttachedDisplay;
        glDeleteTextures(1, &image->GraphicsImage);
        UNREFERENCED_PARAMETER(display);
    }
    memset(image, 0, sizeof(CG_IMAGE));
}

/// @summary Frees all resources associated with an image sampler object.
/// @param ctx The CGFX context that owns the sampler object.
/// @param sampler The image sampler object to delete.
internal_function void
cgDeleteSampler
(
    CG_CONTEXT *ctx, 
    CG_SAMPLER *sampler
)
{   UNREFERENCED_PARAMETER(ctx);
    CG_DISPLAY *display = sampler->AttachedDisplay;
    if (sampler->GraphicsSampler != 0 && GLEW_ARB_sampler_objects)
    {
        glDeleteSamplers(1, &sampler->GraphicsSampler);
    }
    if (sampler->ComputeSampler != NULL)
    {
        clReleaseSampler(sampler->ComputeSampler);
    }
    memset(sampler, 0, sizeof(CG_SAMPLER));
}

/// @summary Frees all resources associated with an input assembler configuration object.
/// @param ctx The CGFX context that owns the vertex data source object.
/// @param source The input assembler configuration object to delete.
internal_function void
cgDeleteVertexDataSource
(
    CG_CONTEXT            *ctx, 
    CG_VERTEX_DATA_SOURCE *source
)
{
    CG_DISPLAY *display = source->AttachedDisplay;
    if (source->VertexArray != 0)
    {
        glDeleteVertexArrays(1, &source->VertexArray);
    }
    cgFreeHostMemory(&ctx->HostAllocator, source->AttributeData  , source->AttributeCount * sizeof(CG_VERTEX_ATTRIBUTE), 0, CG_ALLOCATION_TYPE_OBJECT);
    cgFreeHostMemory(&ctx->HostAllocator, source->ShaderLocations, source->AttributeCount * sizeof(GLuint)             , 0, CG_ALLOCATION_TYPE_OBJECT);
    cgFreeHostMemory(&ctx->HostAllocator, source->BufferIndices  , source->AttributeCount * sizeof(size_t)             , 0, CG_ALLOCATION_TYPE_OBJECT);
    cgFreeHostMemory(&ctx->HostAllocator, source->BufferStrides  , source->BufferCount    * sizeof(size_t)             , 0, CG_ALLOCATION_TYPE_OBJECT);
    memset(source, 0, sizeof(CG_VERTEX_DATA_SOURCE));
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
{   UNREFERENCED_PARAMETER(app_info);
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
    memset(ctx, 0, sizeof(CG_CONTEXT));

    // initialize the various object tables on the context to empty.
    cgObjectTableInit(&ctx->DeviceTable      , CG_OBJECT_DEVICE            , CG_DEVICE_TABLE_ID);
    cgObjectTableInit(&ctx->DisplayTable     , CG_OBJECT_DISPLAY           , CG_DISPLAY_TABLE_ID);
    cgObjectTableInit(&ctx->QueueTable       , CG_OBJECT_QUEUE             , CG_QUEUE_TABLE_ID);
    cgObjectTableInit(&ctx->CmdBufferTable   , CG_OBJECT_COMMAND_BUFFER    , CG_CMD_BUFFER_TABLE_ID);
    cgObjectTableInit(&ctx->ExecGroupTable   , CG_OBJECT_EXECUTION_GROUP   , CG_EXEC_GROUP_TABLE_ID);
    cgObjectTableInit(&ctx->KernelTable      , CG_OBJECT_KERNEL            , CG_KERNEL_TABLE_ID);
    cgObjectTableInit(&ctx->PipelineTable    , CG_OBJECT_PIPELINE          , CG_PIPELINE_TABLE_ID);
    cgObjectTableInit(&ctx->BufferTable      , CG_OBJECT_BUFFER            , CG_BUFFER_TABLE_ID);
    cgObjectTableInit(&ctx->FenceTable       , CG_OBJECT_FENCE             , CG_FENCE_TABLE_ID);
    cgObjectTableInit(&ctx->EventTable       , CG_OBJECT_EVENT             , CG_EVENT_TABLE_ID);
    cgObjectTableInit(&ctx->ImageTable       , CG_OBJECT_IMAGE             , CG_IMAGE_TABLE_ID);
    cgObjectTableInit(&ctx->SamplerTable     , CG_OBJECT_SAMPLER           , CG_SAMPLER_TABLE_ID);
    cgObjectTableInit(&ctx->VertexSourceTable, CG_OBJECT_VERTEX_DATA_SOURCE, CG_VERTEX_DATA_SOURCE_TABLE_ID);

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
    // free all vertex layout objects:
    for (size_t i = 0, n = ctx->VertexSourceTable.ObjectCount; i < n; ++i)
    {
        CG_VERTEX_DATA_SOURCE *obj = &ctx->VertexSourceTable.Objects[i];
        cgDeleteVertexDataSource(ctx, obj);
    }
    // free all sampler objects:
    for (size_t i = 0, n = ctx->SamplerTable.ObjectCount; i < n; ++i)
    {
        CG_SAMPLER *obj = &ctx->SamplerTable.Objects[i];
        cgDeleteSampler(ctx, obj);
    }
    // free all image objects:
    for (size_t i = 0, n = ctx->ImageTable.ObjectCount; i < n; ++i)
    {
        CG_IMAGE *obj = &ctx->ImageTable.Objects[i];
        cgDeleteImage(ctx, obj);
    }
    // free all event objects:
    for (size_t i = 0, n = ctx->EventTable.ObjectCount; i < n; ++i)
    {
        CG_EVENT *obj = &ctx->EventTable.Objects[i];
        cgDeleteEvent(ctx, obj);
    }
    // free all fence objects:
    for (size_t i = 0, n = ctx->FenceTable.ObjectCount; i < n; ++i)
    {
        CG_FENCE *obj = &ctx->FenceTable.Objects[i];
        cgDeleteFence(ctx, obj);
    }
    // free all buffer objects:
    for (size_t i = 0, n = ctx->BufferTable.ObjectCount; i < n; ++i)
    {
        CG_BUFFER *obj = &ctx->BufferTable.Objects[i];
        cgDeleteBuffer(ctx, obj);
    }
    // free all pipeline objects:
    for (size_t i = 0, n = ctx->PipelineTable.ObjectCount; i < n; ++i)
    {
        CG_PIPELINE *obj = &ctx->PipelineTable.Objects[i];
        cgDeletePipeline(ctx, obj);
    }
    // free all kernel objects:
    for (size_t i = 0, n = ctx->KernelTable.ObjectCount; i < n; ++i)
    {
        CG_KERNEL *obj = &ctx->KernelTable.Objects[i];
        cgDeleteKernel(ctx, obj);
    }
    // free all command buffer objects:
    for (size_t i = 0, n = ctx->CmdBufferTable.ObjectCount; i < n; ++i)
    {
        CG_CMD_BUFFER *obj = &ctx->CmdBufferTable.Objects[i];
        cgDeleteCmdBuffer(ctx, obj);
    }
    // free all command queue objects:
    for (size_t i = 0, n = ctx->QueueTable.ObjectCount; i < n; ++i)
    {
        CG_QUEUE *obj = &ctx->QueueTable.Objects[i];
        cgDeleteQueue(ctx, obj);
    }
    // free all execution group objects:
    for (size_t i = 0, n = ctx->ExecGroupTable.ObjectCount; i < n; ++i)
    {
        CG_EXEC_GROUP *obj = &ctx->ExecGroupTable.Objects[i];
        cgDeleteExecutionGroup(ctx, obj);
    }
    // free all device objects:
    for (size_t i = 0, n = ctx->DeviceTable.ObjectCount; i < n; ++i)
    {
        CG_DEVICE  *obj = &ctx->DeviceTable.Objects[i];
        cgDeleteDevice(ctx, obj);
    }
    // free all display objects:
    for (size_t i = 0, n = ctx->DisplayTable.ObjectCount; i < n; ++i)
    {
        CG_DISPLAY *obj = &ctx->DisplayTable.Objects[i];
        cgDeleteDisplay(ctx, obj);
    }
    // free heap descriptors:
    cgFreeHostMemory(host_alloc, ctx->HeapList, ctx->HeapCount * sizeof(CG_HEAP), 0, CG_ALLOCATION_TYPE_OBJECT);
    // finally, free the memory block for the context structure:
    cgFreeHostMemory(host_alloc, ctx, sizeof(CG_CONTEXT), 0, CG_ALLOCATION_TYPE_OBJECT);
}

/// @summary Determine the number of heaps exposed by the set of devices in the local system.
/// @param ctx The CGFX context managing the device set.
/// @return The number of unique heaps in the local system.
internal_function size_t
cgClCountUniqueHeaps
(
    CG_CONTEXT *ctx
)
{
    size_t cpus_n = 0;
    size_t heap_n = 0;
    for (size_t i = 0, n = ctx->DeviceTable.ObjectCount; i < n; ++i)
    {
        CG_DEVICE const *dev = &ctx->DeviceTable.Objects[i];
        if (dev->Type == CL_DEVICE_TYPE_CPU)
        {   // CPU devices support partitioning. CPUs share a heap.
            cpus_n++;  // heap_n updated outside of the loop.
        }
        else
        {   // GPU and accelerator devices have two heaps.
            // the first is the local heap, which is sized to device global memory.
            // the second is the pinned memory heap, which is sized to the max allocation.
            heap_n += 2;
        }
    }
    if (cpus_n > 0)
    {
        if (cpus_n >= ctx->CpuInfo.NUMANodes)
        {   // CPUs have one heap per-NUMA node.
            heap_n += ctx->CpuInfo.NUMANodes;
        }
        else
        {   // one heap per logical CPU device. 
            // it's possible that one or more nodes are disabled.
            heap_n += cpus_n;
        }
    }
    return heap_n;
}

/// @summary Determine whether a device heap is already defined. This is necessary for CPU devices that may be partitioned.
/// @param partition_type One of cl_device_partition_property, or 0, specifying the parition type of the device.
/// @param heap The heap attributes to search for.
/// @param heap_list The heap list to search.
/// @param heap_count The number of heaps currently defined.
/// @param heap_index If the function returns true, on return, this value contains the zero-based index of the existing heap in @a heap_list.
/// @return true if the specified heap exists.
internal_function bool
cgClHasHeap
(
    intptr_t partition_type,
    CG_HEAP *heap,
    CG_HEAP *heap_list, 
    size_t   heap_count, 
    size_t  &heap_index
)
{
    if (partition_type == CL_DEVICE_PARTITION_AFFINITY_DOMAIN)
        return false;
    for (size_t i = 0; i < heap_count; ++i)
    {
        if (heap->ParentDeviceId == heap_list[i].ParentDeviceId && 
            heap->Type           == heap_list[i].Type           && 
            heap->HeapSizeTotal  == heap_list[i].HeapSizeTotal)
        {
            heap_index = i;
            return true;
        }
    }
    return false;
}

/// @summary Retrieve information about the available heaps in the system.
/// @param ctx The CGFX context defining the device list.
/// @param heaps An array of @a max_heaps items to populate with data.
/// @param max_heaps The maximum number of heaps to write to @a heaps.
/// @return The number of heaps written to @a heaps.
internal_function size_t
cgClGetHeapInformation
(
    CG_CONTEXT *ctx, 
    CG_HEAP    *heaps, 
    size_t      max_heaps
)
{
    SYSTEM_INFO  info;
    size_t heap_count = 0;
    GetNativeSystemInfo(&info);
    for (size_t i = 0 , n = ctx->DeviceTable.ObjectCount; i < n && heap_count < max_heaps; ++i)
    {
        CG_DEVICE const *dev = &ctx->DeviceTable.Objects[i];
        cl_device_id     did = NULL;

        clGetDeviceInfo(dev->DeviceId, CL_DEVICE_PARENT_DEVICE, sizeof(cl_device_id), &did, NULL);
        if (did == NULL)
        {   // this is a root device; it has no parent.
            did = dev->DeviceId;
        }

        size_t props_size = 0;
        cl_device_partition_property props[16];
        memset(props, 0, sizeof(props));
        clGetDeviceInfo(dev->DeviceId, CL_DEVICE_PARTITION_PROPERTIES, sizeof(props), props, &props_size);
        if (props_size == 0) props[0] = 0; // terminate the list explicitly

        // all devices define a local heap.
        size_t   index        = heap_count;
        CG_HEAP &local        = heaps[heap_count];
        local.Ordinal         = index;
        local.ParentDeviceId  = did;
        local.Type            = CG_HEAP_TYPE_LOCAL;
        local.Flags           = 0;
        local.DeviceType      = dev->Type;
        local.HeapSizeTotal   = dev->Capabilities.GlobalMemorySize;
        local.HeapSizeUsed    = 0;
        local.PinnableTotal   = 0;
        local.PinnableUsed    = 0;
        local.DeviceAlignment = dev->Capabilities.AddressAlign;
        local.UserAlignment   = info.dwPageSize;
        local.UserSizeAlign   = dev->Capabilities.AddressAlign;
        if (dev->Type   != CL_DEVICE_TYPE_CPU || dev->Capabilities.UnifiedMemory)
            local.Flags |= CG_HEAP_GPU_ACCESSIBLE;
        if (dev->Type   == CL_DEVICE_TYPE_CPU || dev->Capabilities.UnifiedMemory)
            local.Flags |= CG_HEAP_CPU_ACCESSIBLE;
        if (dev->Type   != CL_DEVICE_TYPE_CPU)
        {   // GPU and accelerator devices are typically accessed over PCIe and 
            // have a portion of address space reserved as pinnable so that data
            // can be transferred to local device memory via DMA.
            local.PinnableTotal = dev->Capabilities.MaxMallocSize;
            local.PinnableUsed  = 0;
            local.Flags        |= CG_HEAP_HOLDS_PINNED;
        }
        if (dev->Capabilities.UnifiedMemory)
        {   // use the maximum of the system page size and the device alignment.
            // zero-copy buffers require 4KB alignment and a size multiple of 64KB (Intel and AMD).
            // https://software.intel.com/en-us/articles/getting-the-most-from-opencl-12-how-to-increase-performance-by-minimizing-buffer-copies-on-intel-processor-graphics
            local.UserSizeAlign = dev->Capabilities.GlobalCacheLineSize;
        }
        if (cgClHasHeap(props[0], &local, heaps, heap_count, index))
        {   // mark the existing heap as being shared.
            heaps[index].Flags |= CG_HEAP_SHAREABLE;
        }
        else heap_count++;
    }
    return heap_count;
}

/// @summary Locate the heap corresponding to a particular memory object placement hint.
/// @param ctx The CGFX context maintaining the heap list.
/// @param placement_hint One of cg_memory_object_placement_e specifying the desired location for the memory object.
/// @return A pointer to the source heap.
internal_function CG_HEAP*
cgFindHeapForPlacement
(
    CG_CONTEXT *ctx, 
    int         placement_hint
)
{
    for (size_t i = 0, n = ctx->HeapCount; i < n; ++i)
    {
        CG_HEAP *heap  = &ctx->HeapList[i];
        if ((placement_hint == CG_MEMORY_PLACEMENT_PINNED) && (heap->Flags & CG_HEAP_HOLDS_PINNED))
            return heap;
        if ((placement_hint == CG_MEMORY_PLACEMENT_HOST  ) && (heap->DeviceType == CL_DEVICE_TYPE_CPU))
            return heap;
        if ((placement_hint == CG_MEMORY_PLACEMENT_DEVICE) && (heap->DeviceType != CL_DEVICE_TYPE_CPU))
            return heap;
    }
    return &ctx->HeapList[0];
}

/// @summary Locate the heap corresponding to OpenGL buffer usage flags.
/// @param ctx The CGFX context maintaining the heap list.
/// @param usage The OpenGL buffer usage hint, for example, GL_STREAM_DRAW.
/// @return A pointer to the source heap.
internal_function CG_HEAP*
cgGlFindHeapForUsage
(
    CG_CONTEXT *ctx, 
    GLenum      usage
)
{   // convert the OpenGL usage hints to a CGFX placement hint.
    int placement_hint = 0;
    switch (usage)
    {
    case GL_STATIC_DRAW:
        placement_hint = CG_MEMORY_PLACEMENT_DEVICE;
        break;
    case GL_STATIC_READ:
        placement_hint = CG_MEMORY_PLACEMENT_PINNED;
        break;
    case GL_STATIC_COPY:
        placement_hint = CG_MEMORY_PLACEMENT_DEVICE;
        break;
    case GL_STREAM_DRAW:
        placement_hint = CG_MEMORY_PLACEMENT_PINNED;
        break;
    case GL_STREAM_READ:
        placement_hint = CG_MEMORY_PLACEMENT_PINNED;
        break;
    case GL_STREAM_COPY:
        placement_hint = CG_MEMORY_PLACEMENT_DEVICE;
        break;
    case GL_DYNAMIC_DRAW:
        placement_hint = CG_MEMORY_PLACEMENT_PINNED;
        break;
    case GL_DYNAMIC_READ:
        placement_hint = CG_MEMORY_PLACEMENT_PINNED;
        break;
    case GL_DYNAMIC_COPY:
        placement_hint = CG_MEMORY_PLACEMENT_DEVICE;
        break;
    default:
        placement_hint = CG_MEMORY_PLACEMENT_HOST;
        break;
    }
    return cgFindHeapForPlacement(ctx, placement_hint);
}

/// @summary Populate a cl_image_desc and cl_image_format to describe an image.
/// @param cl_desc On return, stores the OpenCL image layout descriptor data.
/// @param cl_format On return, stores the OpenCL pixel layout descriptor data.
/// @param gl_target On return, stores the OpenGL texture target.
/// @param pixel_width The image width, in pixels.
/// @param pixel_height The image height, in pixels.
/// @param slice_count The number of slices in the image. For 1D and 2D images, specify 1.
/// @param array_count The number of array elements in the image. For cubemap images, specify 6. For non-array images, specify 1.
/// @param level_count The number of mipmap levels in the image. For no mipmaps, specify 1. For all mipmaps, specify 0.
/// @param row_pitch The number of bytes between each row of each slice in the image.
/// @param slice_pitch The number of bytes between each slice of the image.
/// @return true if the image attributes describe a logically valid combination. The device may or may not support the specified attributes.
internal_function bool
cgClMakeImageDesc
(
    cl_image_desc   &cl_desc,
    cl_image_format &cl_format,
    GLenum          &gl_target,
    uint32_t         dxgi_format,
    size_t           pixel_width, 
    size_t           pixel_height, 
    size_t           slice_count, 
    size_t           array_count, 
    size_t           level_count,
    size_t           row_pitch, 
    size_t           slice_pitch
)
{   UNREFERENCED_PARAMETER(level_count);
    // populate a cl_image_desc object.
    if (array_count > 1)
    {   // this is an array image, which may be 1D or 2D.
        if (pixel_height > 1)
        {
            gl_target                 = GL_TEXTURE_2D_ARRAY;
            cl_desc.image_type        = CL_MEM_OBJECT_IMAGE2D_ARRAY;
            cl_desc.image_width       = pixel_width;
            cl_desc.image_height      = pixel_height;
            cl_desc.image_depth       = 0;
            cl_desc.image_array_size  = array_count;
            cl_desc.image_row_pitch   = row_pitch;
            cl_desc.image_slice_pitch = slice_pitch;
            cl_desc.num_mip_levels    = 0;
            cl_desc.num_samples       = 0;
        }
        else
        {
            gl_target                 = GL_TEXTURE_1D_ARRAY;
            cl_desc.image_type        = CL_MEM_OBJECT_IMAGE1D_ARRAY;
            cl_desc.image_width       = pixel_width;
            cl_desc.image_height      = 0;
            cl_desc.image_depth       = 0;
            cl_desc.image_array_size  = array_count;
            cl_desc.image_row_pitch   = row_pitch;
            cl_desc.image_slice_pitch = slice_pitch;
            cl_desc.num_mip_levels    = 0;
            cl_desc.num_samples       = 0;
        }
    }
    else
    {   // this is a non-array 1D, 2D or 3D image.
        if (slice_count > 1)
        {
            gl_target                 = GL_TEXTURE_3D;
            cl_desc.image_type        = CL_MEM_OBJECT_IMAGE3D;
            cl_desc.image_width       = pixel_width;
            cl_desc.image_height      = pixel_height;
            cl_desc.image_depth       = slice_count;
            cl_desc.image_array_size  = 0;
            cl_desc.image_row_pitch   = row_pitch;
            cl_desc.image_slice_pitch = slice_pitch;
            cl_desc.num_mip_levels    = 0;
            cl_desc.num_samples       = 0;
        }
        else if (pixel_height > 1)
        {
            gl_target                 = GL_TEXTURE_2D;
            cl_desc.image_type        = CL_MEM_OBJECT_IMAGE2D;
            cl_desc.image_width       = pixel_width;
            cl_desc.image_height      = pixel_height;
            cl_desc.image_depth       = 0;
            cl_desc.image_array_size  = 0;
            cl_desc.image_row_pitch   = row_pitch;
            cl_desc.image_slice_pitch = slice_pitch;
            cl_desc.num_mip_levels    = 0;
            cl_desc.num_samples       = 0;
        }
        else
        {
            gl_target                 = GL_TEXTURE_1D;
            cl_desc.image_type        = CL_MEM_OBJECT_IMAGE1D;
            cl_desc.image_width       = pixel_width;
            cl_desc.image_height      = 0;
            cl_desc.image_depth       = 0;
            cl_desc.image_array_size  = 0;
            cl_desc.image_row_pitch   = row_pitch;
            cl_desc.image_slice_pitch = slice_pitch;
            cl_desc.num_mip_levels    = 0;
            cl_desc.num_samples       = 0;
        }
    }

    // populate the cl_image_format object.
    // the following mappings are taken from pp.54-55 of:
    // https://www.khronos.org/registry/cl/specs/opencl-1.2-extensions.pdf
    switch (dxgi_format)
    {
        case DXGI_FORMAT_UNKNOWN:
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case DXGI_FORMAT_R32G32B32_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R32G32_TYPELESS:
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R16G16_TYPELESS:
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_R8G8_TYPELESS:
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R11G11B10_FLOAT:
        case DXGI_FORMAT_R1_UNORM:
        case DXGI_FORMAT_R8G8_B8G8_UNORM:
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
        case DXGI_FORMAT_B4G4R4A4_UNORM:
        case DXGI_FORMAT_AYUV:
        case DXGI_FORMAT_Y410:
        case DXGI_FORMAT_Y416:
        case DXGI_FORMAT_NV12:
        case DXGI_FORMAT_P010:
        case DXGI_FORMAT_P016:
        case DXGI_FORMAT_420_OPAQUE:
        case DXGI_FORMAT_YUY2:
        case DXGI_FORMAT_Y210:
        case DXGI_FORMAT_Y216:
        case DXGI_FORMAT_NV11:
        case DXGI_FORMAT_AI44:
        case DXGI_FORMAT_IA44:
            return false;
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            cl_format.image_channel_data_type = CL_FLOAT;
            cl_format.image_channel_order     = CL_RGBA;
            return true;
        case DXGI_FORMAT_R32G32B32A32_UINT:
            cl_format.image_channel_data_type = CL_UNSIGNED_INT32;
            cl_format.image_channel_order     = CL_RGBA;
            return true;
        case DXGI_FORMAT_R32G32B32A32_SINT:
            cl_format.image_channel_data_type = CL_SIGNED_INT32;
            cl_format.image_channel_order     = CL_RGBA;
            return true;
        case DXGI_FORMAT_R32G32B32_FLOAT:
            cl_format.image_channel_data_type = CL_FLOAT;
            cl_format.image_channel_order     = CL_RGB;
            return true;
        case DXGI_FORMAT_R32G32B32_UINT:
            cl_format.image_channel_data_type = CL_UNSIGNED_INT32;
            cl_format.image_channel_order     = CL_RGB;
            return true;
        case DXGI_FORMAT_R32G32B32_SINT:
            cl_format.image_channel_data_type = CL_SIGNED_INT32;
            cl_format.image_channel_order     = CL_RGB;
            return true;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            cl_format.image_channel_data_type = CL_HALF_FLOAT;
            cl_format.image_channel_order     = CL_RGBA;
            return true;
        case DXGI_FORMAT_R16G16B16A16_UNORM:
            cl_format.image_channel_data_type = CL_UNORM_INT16;
            cl_format.image_channel_order     = CL_RGBA;
            return true;
        case DXGI_FORMAT_R16G16B16A16_UINT:
            cl_format.image_channel_data_type = CL_UNSIGNED_INT16;
            cl_format.image_channel_order     = CL_RGBA;
            return true;
        case DXGI_FORMAT_R16G16B16A16_SNORM:
            cl_format.image_channel_data_type = CL_SNORM_INT16;
            cl_format.image_channel_order     = CL_RGBA;
            return true;
        case DXGI_FORMAT_R16G16B16A16_SINT:
            cl_format.image_channel_data_type = CL_SIGNED_INT16;
            cl_format.image_channel_order     = CL_RGBA;
            return true;
        case DXGI_FORMAT_R32G32_FLOAT:
            cl_format.image_channel_data_type = CL_FLOAT;
            cl_format.image_channel_order     = CL_RG;
            return true;
        case DXGI_FORMAT_R32G32_UINT:
            cl_format.image_channel_data_type = CL_UNSIGNED_INT32;
            cl_format.image_channel_order     = CL_RG;
            return true;
        case DXGI_FORMAT_R32G32_SINT:
            cl_format.image_channel_data_type = CL_SIGNED_INT32;
            cl_format.image_channel_order     = CL_RG;
            return true;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            cl_format.image_channel_data_type = CL_FLOAT;
            cl_format.image_channel_order     = CL_DEPTH;
            return true;
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            cl_format.image_channel_data_type = CL_FLOAT;
            cl_format.image_channel_order     = CL_Rx;
            return true;
        case DXGI_FORMAT_R10G10B10A2_UNORM:
            cl_format.image_channel_data_type = CL_UNORM_INT_101010;
            cl_format.image_channel_order     = CL_RGBA;
            return true;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            cl_format.image_channel_data_type = CL_UNORM_INT8;
            cl_format.image_channel_order     = CL_RGBA;
            return true;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            cl_format.image_channel_data_type = CL_UNORM_INT8;
            cl_format.image_channel_order     = CL_sRGBA;
            return true;
        case DXGI_FORMAT_R8G8B8A8_UINT:
            cl_format.image_channel_data_type = CL_UNSIGNED_INT8;
            cl_format.image_channel_order     = CL_RGBA;
            return true;
        case DXGI_FORMAT_R8G8B8A8_SNORM:
            cl_format.image_channel_data_type = CL_SNORM_INT8;
            cl_format.image_channel_order     = CL_RGBA;
            return true;
        case DXGI_FORMAT_R8G8B8A8_SINT:
            cl_format.image_channel_data_type = CL_SIGNED_INT8;
            cl_format.image_channel_order     = CL_RGBA;
            return true;
        case DXGI_FORMAT_R16G16_FLOAT:
            cl_format.image_channel_data_type = CL_HALF_FLOAT;
            cl_format.image_channel_order     = CL_RG;
            return true;
        case DXGI_FORMAT_R16G16_UNORM:
            cl_format.image_channel_data_type = CL_UNORM_INT16;
            cl_format.image_channel_order     = CL_RG;
            return true;
        case DXGI_FORMAT_R16G16_UINT:
            cl_format.image_channel_data_type = CL_UNSIGNED_INT16;
            cl_format.image_channel_order     = CL_RG;
            return true;
        case DXGI_FORMAT_R16G16_SNORM:
            cl_format.image_channel_data_type = CL_SNORM_INT16;
            cl_format.image_channel_order     = CL_RG;
            return true;
        case DXGI_FORMAT_R16G16_SINT:
            cl_format.image_channel_data_type = CL_SIGNED_INT16;
            cl_format.image_channel_order     = CL_RG;
            return true;
        case DXGI_FORMAT_D32_FLOAT:
            cl_format.image_channel_data_type = CL_FLOAT;
            cl_format.image_channel_order     = CL_DEPTH;
            return true;
        case DXGI_FORMAT_R32_FLOAT:
            cl_format.image_channel_data_type = CL_FLOAT;
            cl_format.image_channel_order     = CL_R;
            return true;
        case DXGI_FORMAT_R32_UINT:
            cl_format.image_channel_data_type = CL_UNSIGNED_INT32;
            cl_format.image_channel_order     = CL_R;
            return true;
        case DXGI_FORMAT_R32_SINT:
            cl_format.image_channel_data_type = CL_SIGNED_INT32;
            cl_format.image_channel_order     = CL_R;
            return true;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            cl_format.image_channel_data_type = CL_UNORM_INT24;
            cl_format.image_channel_order     = CL_DEPTH;
            return true;
        case DXGI_FORMAT_R8G8_UNORM:
            cl_format.image_channel_data_type = CL_UNORM_INT8;
            cl_format.image_channel_order     = CL_RG;
            return true;
        case DXGI_FORMAT_R8G8_UINT:
            cl_format.image_channel_data_type = CL_UNSIGNED_INT8;
            cl_format.image_channel_order     = CL_RG;
            return true;
        case DXGI_FORMAT_R8G8_SNORM:
            cl_format.image_channel_data_type = CL_SNORM_INT8;
            cl_format.image_channel_order     = CL_RG;
            return true;
        case DXGI_FORMAT_R8G8_SINT:
            cl_format.image_channel_data_type = CL_SIGNED_INT8;
            cl_format.image_channel_order     = CL_RG;
            return true;
        case DXGI_FORMAT_R16_FLOAT:
            cl_format.image_channel_data_type = CL_HALF_FLOAT;
            cl_format.image_channel_order     = CL_R;
            return true;
        case DXGI_FORMAT_D16_UNORM:
            cl_format.image_channel_data_type = CL_UNORM_INT16;
            cl_format.image_channel_order     = CL_DEPTH;
            return true;
        case DXGI_FORMAT_R16_UNORM:
            cl_format.image_channel_data_type = CL_UNORM_INT16;
            cl_format.image_channel_order     = CL_R;
            return true;
        case DXGI_FORMAT_R16_UINT:
            cl_format.image_channel_data_type = CL_UNSIGNED_INT16;
            cl_format.image_channel_order     = CL_R;
            return true;
        case DXGI_FORMAT_R16_SNORM:
            cl_format.image_channel_data_type = CL_SNORM_INT16;
            cl_format.image_channel_order     = CL_R;
            return true;
        case DXGI_FORMAT_R16_SINT:
            cl_format.image_channel_data_type = CL_SIGNED_INT16;
            cl_format.image_channel_order     = CL_R;
            return true;
        case DXGI_FORMAT_R8_UNORM:
            cl_format.image_channel_data_type = CL_UNORM_INT8;
            cl_format.image_channel_order     = CL_R;
            return true;
        case DXGI_FORMAT_R8_UINT:
            cl_format.image_channel_data_type = CL_UNSIGNED_INT8;
            cl_format.image_channel_order     = CL_R;
            return true;
        case DXGI_FORMAT_R8_SNORM:
            cl_format.image_channel_data_type = CL_SNORM_INT8;
            cl_format.image_channel_order     = CL_R;
            return true;
        case DXGI_FORMAT_R8_SINT:
            cl_format.image_channel_data_type = CL_SIGNED_INT8;
            cl_format.image_channel_order     = CL_R;
            return true;
        case DXGI_FORMAT_A8_UNORM:
            cl_format.image_channel_data_type = CL_UNORM_INT8;
            cl_format.image_channel_order     = CL_A;
            return true;
        case DXGI_FORMAT_B5G6R5_UNORM:
            cl_format.image_channel_data_type = CL_UNORM_SHORT_565;
            cl_format.image_channel_order     = CL_BGRA;
            return true;
        case DXGI_FORMAT_B5G5R5A1_UNORM:
            cl_format.image_channel_data_type = CL_UNORM_SHORT_555;
            cl_format.image_channel_order     = CL_BGRA;
            return true;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            cl_format.image_channel_data_type = CL_UNORM_INT8;
            cl_format.image_channel_order     = CL_BGRA;
            return true;
        case DXGI_FORMAT_B8G8R8X8_UNORM:
            cl_format.image_channel_data_type = CL_UNORM_INT8;
            cl_format.image_channel_order     = CL_BGRA;
            return true;
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
            cl_format.image_channel_data_type = CL_UNORM_INT_101010;
            cl_format.image_channel_order     = CL_RGBA;
            return true;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            cl_format.image_channel_data_type = CL_UNORM_INT8;
            cl_format.image_channel_order     = CL_sBGRA;
            return true;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            cl_format.image_channel_data_type = CL_UNORM_INT8;
            cl_format.image_channel_order     = CL_sBGRA;
            return true;
        case DXGI_FORMAT_P8:
            cl_format.image_channel_data_type = CL_UNSIGNED_INT8;
            cl_format.image_channel_order     = CL_R;
            return true;
        case DXGI_FORMAT_A8P8:
            cl_format.image_channel_data_type = CL_UNSIGNED_INT8;
            cl_format.image_channel_order     = CL_RA;
            return true;
        default:
            break;
    }
    return false;
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

/// @summary Queries the OpenCL runtime to determine whether a device partitioning method is supported.
/// @param device The OpenCL device being partitioned.
/// @param partition_type The device partition type to check.
/// @param max_subdevices On return, stores the maximum number of device partitions for @a device.
/// @return true if the specified partition type is supported.
internal_function bool
cgClSupportsPartitionType
(
    cl_device_id                 device, 
    cl_device_partition_property partition_type, 
    cl_uint                     &max_subdevices
)
{
    size_t const                 props_n     = 4;
    size_t                       props_out   = 0;
    cl_device_partition_property props[props_n] ;
    cl_uint                      sub_count   = 0;

    // query the number of sub-devices available and supported partition types.
    memset(props, 0, props_n * sizeof(cl_device_partition_property));
    clGetDeviceInfo(device, CL_DEVICE_PARTITION_MAX_SUB_DEVICES, sizeof(cl_uint), &sub_count, NULL);
    clGetDeviceInfo(device, CL_DEVICE_PARTITION_PROPERTIES     , sizeof(cl_device_partition_property) * props_n, props, &props_out);
    if (sub_count == 0)
    {   // this device doesn't support partitioning at all.
        max_subdevices = 0;
        return false;
    }
    // check runtime support for the desired partition type.
    for (size_t i = 0, n = props_out / sizeof(cl_device_partition_property); i < n; ++i)
    {
        if (props[i] == partition_type)
        {   // the desired partition type is supported.
            max_subdevices = sub_count;
            return true;
        }
    }
    // the runtime doesn't support this type of paritioning for the device.
    max_subdevices = 0;
    return false;
}

/// @summary Identifies and partitions the CPU device according to user preferences.
/// @param ctx The CGFX context to populate with device information.
/// @param cpu_partition Information on how the CPU device should be partitioned.
/// @param num_platforms The number of platform IDs in @a platform_ids.
/// @param platform_ids An array of OpenCL platform identifiers for the local system.
/// @return CG_SUCCESS, CG_OUT_OF_MEMORY, CG_INVALID_VALUE or CG_UNSUPPORTED.
internal_function int
cgClPartitionCpuDevice
(
    CG_CONTEXT               *ctx, 
    cg_cpu_partition_t const *cpu_partition, 
    cl_uint                   num_platforms, 
    cl_platform_id           *platform_ids
)
{
    cl_platform_id      platform = NULL;
    cl_platform_id     intel_pid = NULL;
    cl_platform_id       amd_pid = NULL;
    cl_device_id      cpu_device = NULL;
    cl_device_id   *device_list  = NULL;
    size_t          device_count = 0;
    cl_uint       max_subdevices = 0;
    cl_uint       num_subdevices = 0;
    cl_uint                clres = 0;
    cg_cpu_info_t const    &info = ctx->CpuInfo;
    int                   result = CG_SUCCESS;

    // determine which platform to use. this is necessary because the client may 
    // have, for example, an Intel CPU with AMD GPU, and both the Intel and AMD 
    // drivers may expose a CPU device.
    for (size_t i = 0, n = size_t(num_platforms); i < n; ++i)
    {
        cl_platform_id pid = platform_ids[i];
        cl_device_id   did = NULL;
        cl_uint        num = 0;

        // filter out blacklisted platforms:
        if (!cgClIsPlatformSupported(ctx, pid))
            continue;

        // retrieve the ID of the primary CPU device for the platform.
        clGetDeviceIDs(pid, CL_DEVICE_TYPE_CPU, 1, &did, &num);
        if (num > 0)
        {   // ensure that the device meets the minimum OpenCL version requirement.
            int majorv = 0;
            int minorv = 0;

            // skip devices that don't support OpenCL 1.2 or later:
            if (!cgClDeviceVersion(ctx, did, majorv, minorv))
            {   // couldn't parse the version information; skip.
                continue;
            }
            if (!cgClIsVersionSupported(majorv, minorv))
            {   // OpenCL exposed by the device is an unsupported version; skip.
                continue;
            }

            // determine whether it's Intel, AMD or something else.
            char *vendor = cgClPlatformString(ctx, pid, CL_PLATFORM_VENDOR, CG_ALLOCATION_TYPE_TEMP);
            if (cgStristr(vendor, "Intel") != NULL)
                intel_pid = pid;
            else if (cgStristr(vendor, "AMD") != NULL)
                amd_pid = pid;

            cgClFreeString(ctx, vendor, CG_ALLOCATION_TYPE_TEMP);
            clReleaseDevice(did);
            platform = pid;
        }
    }
    if (info.PreferIntel && intel_pid != NULL) platform = intel_pid;
    if (info.PreferAMD   &&   amd_pid != NULL) platform = amd_pid;
    if (platform == NULL)
    {   // the system does not expose any OpenCL-capable CPU devices.
        return CG_SUCCESS;
    }

    // retrieve the ID of the un-partitioned CPU device on the preferred platform.
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &cpu_device, NULL);

    // figure out how the CPU device(s) will be partitioned.
    if (cpu_partition->PartitionType == CG_CPU_PARTITION_NONE)
    {
        if (cgClSupportsPartitionType(cpu_device, CL_DEVICE_PARTITION_BY_COUNTS, max_subdevices))
        {   // partition into a single logical device. some threads may be reserved for application use.
            size_t                threads_enabled = info.HardwareThreads - cpu_partition->ReserveThreads;
            cl_device_partition_property parts[4] = 
            {
                (cl_device_partition_property) CL_DEVICE_PARTITION_BY_COUNTS, 
                (cl_device_partition_property) threads_enabled, 
                (cl_device_partition_property) CL_DEVICE_PARTITION_BY_COUNTS_LIST_END, 
                (cl_device_partition_property) 0
            };
            // allocate memory for the list of logical device identifiers.
            device_count     =  1;
            if ((device_list = (cl_device_id*) cgAllocateHostMemory(&ctx->HostAllocator, device_count * sizeof(cl_device_id), 0, CG_ALLOCATION_TYPE_TEMP)) == NULL)
            {   // unable to allocate the required memory.
                result = CG_OUT_OF_MEMORY;
                goto error_cleanup;
            }
            // partition the master CPU device into sub-devices.
            memset(device_list, 0, device_count * sizeof(cl_device_id));
            if ((clres = clCreateSubDevices(cpu_device, parts, cl_uint(device_count), device_list, &num_subdevices)) != CL_SUCCESS)
            {   // unable to create the device partition.
                switch (clres)
                {
                case CL_INVALID_DEVICE                : result = CG_INVALID_VALUE; break;
                case CL_INVALID_VALUE                 : result = CG_INVALID_VALUE; break;
                case CL_DEVICE_PARTITION_FAILED       : result = CG_INVALID_VALUE; break;
                case CL_INVALID_DEVICE_PARTITION_COUNT: result = CG_INVALID_VALUE; break;
                case CL_OUT_OF_RESOURCES              : result = CG_OUT_OF_MEMORY; break;
                case CL_OUT_OF_HOST_MEMORY            : result = CG_OUT_OF_MEMORY; break;
                default                               : result = CG_ERROR;         break;
                }
                goto error_cleanup;
            }
        }
        else
        {   // the required partition type is not supported by the runtime.
            result = CG_UNSUPPORTED;
            goto error_cleanup;
        }
    }
    else if (cpu_partition->PartitionType == CG_CPU_PARTITION_PER_CORE)
    {
        if (cgClSupportsPartitionType(cpu_device, CL_DEVICE_PARTITION_BY_COUNTS, max_subdevices))
        {   // partition the device into one or more logical sub-devices.
            cl_device_partition_property *parts = NULL;
            size_t            available_threads = info.HardwareThreads - cpu_partition->ReserveThreads;
            size_t            number_of_devices =(size_t) ceilf((float ) available_threads /  (float) info.ThreadsPerCore);
            size_t               part_list_size =(number_of_devices + 3) * sizeof(cl_device_partition_property);
            size_t                   part_index = 0;
            if ((parts = (cl_device_partition_property*) cgAllocateHostMemory(&ctx->HostAllocator, part_list_size, 0, CG_ALLOCATION_TYPE_TEMP)) == NULL)
            {   // unable to allocate memory for the partition list.
                result = CG_OUT_OF_MEMORY;
                goto error_cleanup;
            }
            parts[part_index++] = (cl_device_partition_property) CL_DEVICE_PARTITION_BY_COUNTS;
            while (available_threads != 0)
            {
                size_t threads_for_device = min(available_threads, info.ThreadsPerCore);
                parts[part_index++] = (cl_device_partition_property) threads_for_device;
                available_threads  -=  threads_for_device;
            }
            parts[part_index++] = (cl_device_partition_property) CL_DEVICE_PARTITION_BY_COUNTS_LIST_END;
            parts[part_index++] =  0;
            // allocate memory for the list of logical device identifiers.
            device_count     =  number_of_devices;
            if ((device_list = (cl_device_id*) cgAllocateHostMemory(&ctx->HostAllocator, device_count * sizeof(cl_device_id), 0, CG_ALLOCATION_TYPE_TEMP)) == NULL)
            {   // unable to allocate the required memory for the device ID list.
                cgFreeHostMemory(&ctx->HostAllocator, parts, part_list_size, 0, CG_ALLOCATION_TYPE_TEMP);
                result = CG_OUT_OF_MEMORY;
                goto error_cleanup;
            }
            // partition the master CPU device into sub-devices.
            memset(device_list, 0, device_count * sizeof(cl_device_id));
            if ((clres = clCreateSubDevices(cpu_device, parts, cl_uint(device_count), device_list, &num_subdevices)) != CL_SUCCESS)
            {   // unable to create the device partition.
                switch (clres)
                {
                case CL_INVALID_DEVICE                : result = CG_INVALID_VALUE; break;
                case CL_INVALID_VALUE                 : result = CG_INVALID_VALUE; break;
                case CL_DEVICE_PARTITION_FAILED       : result = CG_INVALID_VALUE; break;
                case CL_INVALID_DEVICE_PARTITION_COUNT: result = CG_INVALID_VALUE; break;
                case CL_OUT_OF_RESOURCES              : result = CG_OUT_OF_MEMORY; break;
                case CL_OUT_OF_HOST_MEMORY            : result = CG_OUT_OF_MEMORY; break;
                default                               : result = CG_ERROR;         break;
                }
                cgFreeHostMemory(&ctx->HostAllocator, parts, part_list_size, 0, CG_ALLOCATION_TYPE_TEMP);
                goto error_cleanup;
            }
        }
        else
        {   // the required partition type is not supported by the runtime.
            result = CG_UNSUPPORTED;
            goto error_cleanup;
        }
    }
    else if (cpu_partition->PartitionType == CG_CPU_PARTITION_PER_NODE)
    {   // allocate info.NUMANodes device IDs, partition by affinity.
        // for each returned sub-device, partition by counts a single device:
        // threads_per_device = (info.HardwareThreads / info.NUMANodes) - cpu_partition->ReserveThreads
        if (cgClSupportsPartitionType(cpu_device, CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN, max_subdevices))
        {
            cl_device_partition_property parts[4] = 
            {
                (cl_device_partition_property) CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN, 
                (cl_device_partition_property) CL_DEVICE_AFFINITY_DOMAIN_NUMA, 
                (cl_device_partition_property) 0, 
                (cl_device_partition_property) 0
            };
            // allocate memory for the list of logical device identifiers.
            device_count     =  info.NUMANodes;
            if ((device_list = (cl_device_id*) cgAllocateHostMemory(&ctx->HostAllocator, device_count * sizeof(cl_device_id), 0, CG_ALLOCATION_TYPE_TEMP)) == NULL)
            {   // unable to allocate the required memory.
                result = CG_OUT_OF_MEMORY;
                goto error_cleanup;
            }
            // partition the master CPU device into sub-devices.
            memset(device_list, 0, device_count * sizeof(cl_device_id));
            if ((clres = clCreateSubDevices(cpu_device, parts, cl_uint(device_count), device_list, &num_subdevices)) != CL_SUCCESS)
            {   // unable to create the device partition.
                switch (clres)
                {
                case CL_INVALID_DEVICE                : result = CG_INVALID_VALUE; break;
                case CL_INVALID_VALUE                 : result = CG_INVALID_VALUE; break;
                case CL_DEVICE_PARTITION_FAILED       : result = CG_INVALID_VALUE; break;
                case CL_INVALID_DEVICE_PARTITION_COUNT: result = CG_INVALID_VALUE; break;
                case CL_OUT_OF_RESOURCES              : result = CG_OUT_OF_MEMORY; break;
                case CL_OUT_OF_HOST_MEMORY            : result = CG_OUT_OF_MEMORY; break;
                default                               : result = CG_ERROR;         break;
                }
                goto error_cleanup;
            }
            // we now have one device per-NUMA node.
            if (cpu_partition->ReserveThreads > 0)
            {   // reserve the specified number of threads on each node.
                for (size_t node_index = 0, node_count = info.NUMANodes; node_index < node_count; ++node_index)
                {
                    size_t threads_per_node = info.HardwareThreads / info.NUMANodes;
                    size_t threads_enabled  = threads_per_node - cpu_partition->ReserveThreads;
                    cl_device_id     devid  = device_list[node_index];
                    cl_uint        subdevs  = 0;
                    if (!cgClSupportsPartitionType(devid, CL_DEVICE_PARTITION_BY_COUNTS, subdevs))
                    {   // the node cannot be partitioned further.
                        result = CG_UNSUPPORTED;
                        goto error_cleanup;
                    }
                    // build the partition list for this node.
                    parts[0] = (cl_device_partition_property) CL_DEVICE_PARTITION_BY_COUNTS;
                    parts[1] = (cl_device_partition_property) threads_enabled;
                    parts[2] = (cl_device_partition_property) CL_DEVICE_PARTITION_BY_COUNTS_LIST_END;
                    parts[3] = (cl_device_partition_property) 0;
                    // partition the node into a single sub-device.
                    if ((clres = clCreateSubDevices(devid, parts, 1, &device_list[node_index], &subdevs)) != CL_SUCCESS)
                    {   // unable to create the device partition.
                        switch (clres)
                        {
                        case CL_INVALID_DEVICE                : result = CG_INVALID_VALUE; break;
                        case CL_INVALID_VALUE                 : result = CG_INVALID_VALUE; break;
                        case CL_DEVICE_PARTITION_FAILED       : result = CG_INVALID_VALUE; break;
                        case CL_INVALID_DEVICE_PARTITION_COUNT: result = CG_INVALID_VALUE; break;
                        case CL_OUT_OF_RESOURCES              : result = CG_OUT_OF_MEMORY; break;
                        case CL_OUT_OF_HOST_MEMORY            : result = CG_OUT_OF_MEMORY; break;
                        default                               : result = CG_ERROR;         break;
                        }
                        goto error_cleanup;
                    }
                }
            }
        }
        else
        {   // the required partition type is not supported by the runtime.
            result = CG_UNSUPPORTED;
            goto error_cleanup;
        }
    }
    else if (cpu_partition->PartitionType == CG_CPU_PARTITION_EXPLICIT)
    {
        if (cgClSupportsPartitionType(cpu_device, CL_DEVICE_PARTITION_BY_COUNTS, max_subdevices))
        {   // partition the device into one or more logical sub-devices.
            cl_device_partition_property *parts = NULL;
            size_t            threads_available = info.HardwareThreads - cpu_partition->ReserveThreads;
            size_t            number_of_devices = cpu_partition->PartitionCount;
            size_t               part_list_size =(number_of_devices + 3) * sizeof(cl_device_partition_property);
            size_t                   part_index = 0;
            if ((parts = (cl_device_partition_property*) cgAllocateHostMemory(&ctx->HostAllocator, part_list_size, 0, CG_ALLOCATION_TYPE_TEMP)) == NULL)
            {   // unable to allocate memory for the partition list.
                result = CG_OUT_OF_MEMORY;
                goto error_cleanup;
            }
            parts[part_index++]  = (cl_device_partition_property) CL_DEVICE_PARTITION_BY_COUNTS;
            for (size_t i = 0, x = 1; i < number_of_devices; ++i, ++x)
            {
                size_t thread_count = (size_t) cpu_partition->ThreadCounts[i];
                if (thread_count < 0)
                {   // negative values mean 'all remaining threads'.
                    thread_count = threads_available;
                }
                parts[part_index++] = (cl_device_partition_property) thread_count;
                threads_available  -=  thread_count;
            }
            parts[part_index++]  = (cl_device_partition_property) CL_DEVICE_PARTITION_BY_COUNTS_LIST_END;
            parts[part_index++]  =  0;
            // allocate memory for the list of logical device identifiers.
            device_count     =  number_of_devices;
            if ((device_list = (cl_device_id*) cgAllocateHostMemory(&ctx->HostAllocator, device_count * sizeof(cl_device_id), 0, CG_ALLOCATION_TYPE_TEMP)) == NULL)
            {   // unable to allocate the required memory for the device ID list.
                cgFreeHostMemory(&ctx->HostAllocator, parts, part_list_size, 0, CG_ALLOCATION_TYPE_TEMP);
                result = CG_OUT_OF_MEMORY;
                goto error_cleanup;
            }
            // partition the master CPU device into sub-devices.
            memset(device_list, 0, device_count * sizeof(cl_device_id));
            if ((clres = clCreateSubDevices(cpu_device, parts, cl_uint(device_count), device_list, &num_subdevices)) != CL_SUCCESS)
            {   // unable to create the device partition.
                switch (clres)
                {
                case CL_INVALID_DEVICE                : result = CG_INVALID_VALUE; break;
                case CL_INVALID_VALUE                 : result = CG_INVALID_VALUE; break;
                case CL_DEVICE_PARTITION_FAILED       : result = CG_INVALID_VALUE; break;
                case CL_INVALID_DEVICE_PARTITION_COUNT: result = CG_INVALID_VALUE; break;
                case CL_OUT_OF_RESOURCES              : result = CG_OUT_OF_MEMORY; break;
                case CL_OUT_OF_HOST_MEMORY            : result = CG_OUT_OF_MEMORY; break;
                default                               : result = CG_ERROR;         break;
                }
                cgFreeHostMemory(&ctx->HostAllocator, parts, part_list_size, 0, CG_ALLOCATION_TYPE_TEMP);
                goto error_cleanup;
            }
        }
        else
        {   // the required partition type is not supported by the runtime.
            result = CG_UNSUPPORTED;
            goto error_cleanup;
        }
    }
    else
    {   // the requested partition type is not supported by CGFX.
        result = CG_UNSUPPORTED;
        goto error_cleanup;
    }

    // generate the CGFX device objects for the CPU partitions.
    for (size_t device_index = 0; device_index < device_count; ++device_index)
    {
        cl_platform_id pid   = platform;
        cl_device_id   devid = device_list[device_index];
        CG_DEVICE    sub_dev;

        sub_dev.ExecutionGroup = CG_INVALID_HANDLE;

        sub_dev.PlatformId     = pid;
        sub_dev.DeviceId       = devid;
        sub_dev.MasterDeviceId = cpu_device;
        sub_dev.Type           = CL_DEVICE_TYPE_CPU;

        sub_dev.Name           = cgClDeviceString  (ctx, devid, CL_DEVICE_NAME      , CG_ALLOCATION_TYPE_INTERNAL);
        sub_dev.Platform       = cgClPlatformString(ctx, pid  , CL_PLATFORM_NAME    , CG_ALLOCATION_TYPE_INTERNAL);
        sub_dev.Version        = cgClDeviceString  (ctx, devid, CL_DEVICE_VERSION   , CG_ALLOCATION_TYPE_INTERNAL);
        sub_dev.Driver         = cgClDeviceString  (ctx, devid, CL_DRIVER_VERSION   , CG_ALLOCATION_TYPE_INTERNAL);
        sub_dev.Extensions     = cgClDeviceString  (ctx, devid, CL_DEVICE_EXTENSIONS, CG_ALLOCATION_TYPE_INTERNAL);

        // the following are setup during display enumeration.
        sub_dev.DisplayCount   = 0;
        sub_dev.DisplayRC      = NULL;
        memset(sub_dev.AttachedDisplays, 0, CG_OPENGL_MAX_ATTACHED_DISPLAYS * sizeof(CG_DISPLAY*));
        memset(sub_dev.DisplayDC       , 0, CG_OPENGL_MAX_ATTACHED_DISPLAYS * sizeof(HDC));

        // query and cache all of the device capabilities.
        cgClDeviceCapsQuery(ctx, &sub_dev.Capabilities, devid);

        // insert the logical device record into the device table.
        cg_handle_t handle = cgObjectTableAdd(&ctx->DeviceTable, sub_dev);
        if (handle == CG_INVALID_HANDLE)
        {   // free all of the device memory; the table is full.
            cgClDeviceCapsFree(ctx, &sub_dev.Capabilities);
            cgClFreeString(ctx, sub_dev.Extensions, CG_ALLOCATION_TYPE_INTERNAL);
            cgClFreeString(ctx, sub_dev.Driver    , CG_ALLOCATION_TYPE_INTERNAL);
            cgClFreeString(ctx, sub_dev.Version   , CG_ALLOCATION_TYPE_INTERNAL);
            cgClFreeString(ctx, sub_dev.Platform  , CG_ALLOCATION_TYPE_INTERNAL);
            cgClFreeString(ctx, sub_dev.Name      , CG_ALLOCATION_TYPE_INTERNAL);
        }
    }

    // clean up temporary resources.
    cgFreeHostMemory(&ctx->HostAllocator, device_list, device_count * sizeof(cl_device_id), 0, CG_ALLOCATION_TYPE_TEMP);
    return CG_SUCCESS;

error_cleanup:
    for (size_t i = 0; i < device_count; ++i)
    {
        if (device_list[i] != NULL)
            clReleaseDevice(device_list[i]);
    }
    cgFreeHostMemory(&ctx->HostAllocator, device_list, device_count * sizeof(cl_device_id), 0, CG_ALLOCATION_TYPE_TEMP);
    if (cpu_device != NULL) clReleaseDevice(cpu_device);
    return result;
}

/// @summary Enumerate all OpenCL platforms and devices available on the system.
/// @param ctx The CGFX context to populate with device information.
/// @param cpu_partition Information about how CPU devices should be partitioned.
/// @return CG_SUCCESS, CG_NO_OPENCL, CG_OUT_OF_MEMORY, CG_UNSUPPORTED or CG_INVALID_VALUE.
internal_function int
cgClEnumerateDevices
(
    CG_CONTEXT               *ctx, 
    cg_cpu_partition_t const *cpu_partition
)
{
    cl_uint   total_platforms = 0;
    cl_uint   valid_platforms = 0;
    cl_platform_id *platforms = NULL;
    int                   res = CG_SUCCESS;

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

    // identify and partition the CPU device (if present) first.
    if ((res = cgClPartitionCpuDevice(ctx, cpu_partition, total_platforms, platforms)) != CG_SUCCESS)
    {   // the CPU device couldn't be partitioned.
        cgFreeHostMemory(&ctx->HostAllocator, platforms, total_platforms * sizeof(cl_platform_id), 0, CG_ALLOCATION_TYPE_TEMP);
        return res;
    }

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

        // query for the number of available GPU and accelerator devices.
        clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR, 0, NULL, &total_devices);
        if (total_devices == 0)
            continue;
        if ((devices = (cl_device_id*) cgAllocateHostMemory(&ctx->HostAllocator, total_devices * sizeof(cl_device_id), 0, CG_ALLOCATION_TYPE_TEMP)) == NULL)
        {   // unable to allocate memory for device IDs.
            res = CG_OUT_OF_MEMORY;
            goto cleanup;
        }
        clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR, total_devices, devices, NULL);

        // validate each device and create CG_DEVICE objects.
        for (size_t device_index = 0, device_count = size_t(total_devices);
                    device_index <    device_count;
                  ++device_index)
        {   cl_device_id  device =    devices[device_index];
            size_t      existing =    0;
            int           majorv =    0;
            int           minorv =    0;

            // skip devices that are already known:
            if (cgClDoesDeviceExist(ctx, platform, device, existing, false))
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
            dev.ExecutionGroup   = CG_INVALID_HANDLE;

            clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(cl_device_type), &dev.Type, NULL);
            dev.PlatformId       = platform;
            dev.DeviceId         = device;
            dev.MasterDeviceId   = device;
            dev.Name             = cgClDeviceString  (ctx, device  , CL_DEVICE_NAME      , CG_ALLOCATION_TYPE_INTERNAL);
            dev.Platform         = cgClPlatformString(ctx, platform, CL_PLATFORM_NAME    , CG_ALLOCATION_TYPE_INTERNAL);
            dev.Version          = cgClDeviceString  (ctx, device  , CL_DEVICE_VERSION   , CG_ALLOCATION_TYPE_INTERNAL);
            dev.Driver           = cgClDeviceString  (ctx, device  , CL_DRIVER_VERSION   , CG_ALLOCATION_TYPE_INTERNAL);
            dev.Extensions       = cgClDeviceString  (ctx, device  , CL_DEVICE_EXTENSIONS, CG_ALLOCATION_TYPE_INTERNAL);

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
    if (!cgGlInitializeWGLEW(display, x, y, w, h))
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
    HINSTANCE       instance = (HINSTANCE)&__ImageBase;
    WNDCLASSEX      wndclass = {};
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

/// @summary Given an OpenGL data type value, calculates the corresponding size.
/// @param data_type The OpenGL data type value, for example, GL_UNSIGNED_BYTE.
/// @return The size of a single element of the specified type, in bytes.
internal_function size_t
cgGlDataSize
(
    GLenum data_type
)
{
    switch (data_type)
    {
    case GL_UNSIGNED_BYTE:               return sizeof(GLubyte);
    case GL_FLOAT:                       return sizeof(GLfloat);
    case GL_FLOAT_VEC2:                  return sizeof(GLfloat) *  2;
    case GL_FLOAT_VEC3:                  return sizeof(GLfloat) *  3;
    case GL_FLOAT_VEC4:                  return sizeof(GLfloat) *  4;
    case GL_INT:                         return sizeof(GLint);
    case GL_INT_VEC2:                    return sizeof(GLint)   *  2;
    case GL_INT_VEC3:                    return sizeof(GLint)   *  3;
    case GL_INT_VEC4:                    return sizeof(GLint)   *  4;
    case GL_BOOL:                        return sizeof(GLint);
    case GL_BOOL_VEC2:                   return sizeof(GLint)   *  2;
    case GL_BOOL_VEC3:                   return sizeof(GLint)   *  3;
    case GL_BOOL_VEC4:                   return sizeof(GLint)   *  4;
    case GL_FLOAT_MAT2:                  return sizeof(GLfloat) *  4;
    case GL_FLOAT_MAT3:                  return sizeof(GLfloat) *  9;
    case GL_FLOAT_MAT4:                  return sizeof(GLfloat) * 16;
    case GL_FLOAT_MAT2x3:                return sizeof(GLfloat) *  6;
    case GL_FLOAT_MAT2x4:                return sizeof(GLfloat) *  8;
    case GL_FLOAT_MAT3x2:                return sizeof(GLfloat) *  6;
    case GL_FLOAT_MAT3x4:                return sizeof(GLfloat) * 12;
    case GL_FLOAT_MAT4x2:                return sizeof(GLfloat) *  8;
    case GL_FLOAT_MAT4x3:                return sizeof(GLfloat) * 12;
    case GL_BYTE:                        return sizeof(GLbyte);
    case GL_UNSIGNED_SHORT:              return sizeof(GLushort);
    case GL_SHORT:                       return sizeof(GLshort);
    case GL_UNSIGNED_INT:                return sizeof(GLuint);
    case GL_UNSIGNED_SHORT_5_6_5:        return sizeof(GLushort);
    case GL_UNSIGNED_SHORT_5_6_5_REV:    return sizeof(GLushort);
    case GL_UNSIGNED_SHORT_4_4_4_4:      return sizeof(GLushort);
    case GL_UNSIGNED_SHORT_4_4_4_4_REV:  return sizeof(GLushort);
    case GL_UNSIGNED_SHORT_5_5_5_1:      return sizeof(GLushort);
    case GL_UNSIGNED_SHORT_1_5_5_5_REV:  return sizeof(GLushort);
    case GL_UNSIGNED_INT_8_8_8_8:        return sizeof(GLubyte);
    case GL_UNSIGNED_INT_8_8_8_8_REV:    return sizeof(GLubyte);
    case GL_UNSIGNED_INT_10_10_10_2:     return sizeof(GLuint);
    case GL_UNSIGNED_INT_2_10_10_10_REV: return sizeof(GLuint);
    case GL_UNSIGNED_BYTE_3_3_2:         return sizeof(GLubyte);
    case GL_UNSIGNED_BYTE_2_3_3_REV:     return sizeof(GLubyte);
    default:                             break;
    }
    return 0;
}

/// @summary Given an OpenGL block-compressed internal format identifier, determine the size of each compressed block, in pixels. For non block-compressed formats, the block size is defined to be 1.
/// @param internal_format The OpenGL internal format value.
/// @return The dimension of a block, in pixels.
internal_function size_t
cgGlBlockDimension
(
    GLenum internal_format
)
{
    switch (internal_format)
    {
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:        return 4;
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:       return 4;
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:       return 4;
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:       return 4;
        case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:       return 4;
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT: return 4;
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT: return 4;
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT: return 4;
        default:
            break;
    }
    return 1;
}

/// @summary Given an OpenGL block-compressed internal format identifier, determine the size of each compressed block of pixels.
/// @param internal_format The OpenGL internal format value. RGB/RGBA/SRGB and SRGBA S3TC/DXT format identifiers are the only values currently accepted.
/// @return The number of bytes per compressed block of pixels, or zero.
internal_function size_t
cgGlBytesPerBlock
(
    GLenum internal_format
)
{
    switch (internal_format)
    {
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:        return 8;
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:       return 8;
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:       return 16;
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:       return 16;
        case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:       return 8;
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT: return 8;
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT: return 16;
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT: return 16;
        default:
            break;
    }
    return 0;
}

/// @summary Given an OpenGL internal format value, calculates the number of
/// bytes per-element (or per-block, for block-compressed formats).
/// @param internal_format The OpenGL internal format, for example, GL_RGBA.
/// @param data_type The OpenGL data type, for example, GL_UNSIGNED_BYTE.
/// @return The number of bytes per element (pixel or block), or zero.
internal_function size_t
cgGlBytesPerElement
(
    GLenum internal_format,
    GLenum data_type
)
{
    switch (internal_format)
    {
        case GL_DEPTH_COMPONENT:
        case GL_DEPTH_STENCIL:
        case GL_RED:
        case GL_R8:
        case GL_R8_SNORM:
        case GL_R16:
        case GL_R16_SNORM:
        case GL_R16F:
        case GL_R32F:
        case GL_R8I:
        case GL_R8UI:
        case GL_R16I:
        case GL_R16UI:
        case GL_R32I:
        case GL_R32UI:
        case GL_R3_G3_B2:
        case GL_RGB4:
        case GL_RGB5:
        case GL_RGB10:
        case GL_RGB12:
        case GL_RGBA2:
        case GL_RGBA4:
        case GL_RGB9_E5:
        case GL_R11F_G11F_B10F:
        case GL_RGB5_A1:
        case GL_RGB10_A2:
        case GL_RGB10_A2UI:
            return (cgGlDataSize(data_type) * 1);

        case GL_RG:
        case GL_RG8:
        case GL_RG8_SNORM:
        case GL_RG16:
        case GL_RG16_SNORM:
        case GL_RG16F:
        case GL_RG32F:
        case GL_RG8I:
        case GL_RG8UI:
        case GL_RG16I:
        case GL_RG16UI:
        case GL_RG32I:
        case GL_RG32UI:
            return (cgGlDataSize(data_type) * 2);

        case GL_RGB:
        case GL_RGB8:
        case GL_RGB8_SNORM:
        case GL_RGB16_SNORM:
        case GL_SRGB8:
        case GL_RGB16F:
        case GL_RGB32F:
        case GL_RGB8I:
        case GL_RGB8UI:
        case GL_RGB16I:
        case GL_RGB16UI:
        case GL_RGB32I:
        case GL_RGB32UI:
            return (cgGlDataSize(data_type) * 3);

        case GL_RGBA:
        case GL_RGBA8:
        case GL_RGBA8_SNORM:
        case GL_SRGB8_ALPHA8:
        case GL_RGBA16F:
        case GL_RGBA32F:
        case GL_RGBA8I:
        case GL_RGBA8UI:
        case GL_RGBA16I:
        case GL_RGBA16UI:
        case GL_RGBA32I:
        case GL_RGBA32UI:
            return (cgGlDataSize(data_type) * 4);

        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
            return cgGlBytesPerBlock(internal_format);

        default:
            break;
    }
    return 0;
}

/// @summary Given an OpenGL internal_format value and a width, calculates the number of bytes between rows in a 2D image slice.
/// @param internal_format The OpenGL internal format, for example, GL_RGBA.
/// @param data_type The OpenGL data type, for example, GL_UNSIGNED_BYTE.
/// @param width The row width, in pixels.
/// @param alignment The alignment requirement of the OpenGL implementation, corresponding to the pname of GL_PACK_ALIGNMENT or GL_UNPACK_ALIGNMENT for the glPixelStorei function. The specification default is 4.
/// @return The number of bytes per-row, or zero.
internal_function size_t
cgGlBytesPerRow
(
    GLenum internal_format,
    GLenum data_type,
    size_t width,
    size_t alignment
)
{
    if (width == 0)  width = 1;
    switch (internal_format)
    {
        case GL_DEPTH_COMPONENT:
        case GL_DEPTH_STENCIL:
        case GL_RED:
        case GL_R8:
        case GL_R8_SNORM:
        case GL_R16:
        case GL_R16_SNORM:
        case GL_R16F:
        case GL_R32F:
        case GL_R8I:
        case GL_R8UI:
        case GL_R16I:
        case GL_R16UI:
        case GL_R32I:
        case GL_R32UI:
        case GL_R3_G3_B2:
        case GL_RGB4:
        case GL_RGB5:
        case GL_RGB10:
        case GL_RGB12:
        case GL_RGBA2:
        case GL_RGBA4:
        case GL_RGB9_E5:
        case GL_R11F_G11F_B10F:
        case GL_RGB5_A1:
        case GL_RGB10_A2:
        case GL_RGB10_A2UI:
            return align_up(width * cgGlDataSize(data_type), alignment);

        case GL_RG:
        case GL_RG8:
        case GL_RG8_SNORM:
        case GL_RG16:
        case GL_RG16_SNORM:
        case GL_RG16F:
        case GL_RG32F:
        case GL_RG8I:
        case GL_RG8UI:
        case GL_RG16I:
        case GL_RG16UI:
        case GL_RG32I:
        case GL_RG32UI:
            return align_up(width * cgGlDataSize(data_type) * 2, alignment);

        case GL_RGB:
        case GL_RGB8:
        case GL_RGB8_SNORM:
        case GL_RGB16_SNORM:
        case GL_SRGB8:
        case GL_RGB16F:
        case GL_RGB32F:
        case GL_RGB8I:
        case GL_RGB8UI:
        case GL_RGB16I:
        case GL_RGB16UI:
        case GL_RGB32I:
        case GL_RGB32UI:
            return align_up(width * cgGlDataSize(data_type) * 3, alignment);

        case GL_RGBA:
        case GL_RGBA8:
        case GL_RGBA8_SNORM:
        case GL_SRGB8_ALPHA8:
        case GL_RGBA16F:
        case GL_RGBA32F:
        case GL_RGBA8I:
        case GL_RGBA8UI:
        case GL_RGBA16I:
        case GL_RGBA16UI:
        case GL_RGBA32I:
        case GL_RGBA32UI:
            return align_up(width * cgGlDataSize(data_type) * 4, alignment);

        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
            return align_up(((width + 3) >> 2) * cgGlBytesPerBlock(internal_format), alignment);

        default:
            break;
    }
    return 0;
}

/// @summary Calculates the size of the buffer required to store an image with the specified attributes.
/// @param internal_format The OpenGL internal format value, for example, GL_RGBA. The most common compressed formats are supported (DXT/S3TC).
/// @param data_type The data type identifier, for example, GL_UNSIGNED_BYTE.
/// @param width The width of the image, in pixels.
/// @param height The height of the image, in pixels.
/// @param alignment The alignment requirement of the OpenGL implementation, corresponding to the pname of GL_PACK_ALIGNMENT or GL_UNPACK_ALIGNMENT for the glPixelStorei function. The specification default is 4.
/// @return The number of bytes required to store the image data.
internal_function size_t
cgGlBytesPerSlice
(
    GLenum internal_format,
    GLenum data_type,
    size_t width,
    size_t height,
    size_t alignment
)
{
    if (width  == 0) width  = 1;
    if (height == 0) height = 1;
    switch (internal_format)
    {
        case GL_DEPTH_COMPONENT:
        case GL_DEPTH_STENCIL:
        case GL_RED:
        case GL_R8:
        case GL_R8_SNORM:
        case GL_R16:
        case GL_R16_SNORM:
        case GL_R16F:
        case GL_R32F:
        case GL_R8I:
        case GL_R8UI:
        case GL_R16I:
        case GL_R16UI:
        case GL_R32I:
        case GL_R32UI:
        case GL_R3_G3_B2:
        case GL_RGB4:
        case GL_RGB5:
        case GL_RGB10:
        case GL_RGB12:
        case GL_RGBA2:
        case GL_RGBA4:
        case GL_RGB9_E5:
        case GL_R11F_G11F_B10F:
        case GL_RGB5_A1:
        case GL_RGB10_A2:
        case GL_RGB10_A2UI:
            return align_up(width * cgGlDataSize(data_type), alignment) * height;

        case GL_RG:
        case GL_RG8:
        case GL_RG8_SNORM:
        case GL_RG16:
        case GL_RG16_SNORM:
        case GL_RG16F:
        case GL_RG32F:
        case GL_RG8I:
        case GL_RG8UI:
        case GL_RG16I:
        case GL_RG16UI:
        case GL_RG32I:
        case GL_RG32UI:
            return align_up(width * cgGlDataSize(data_type) * 2, alignment) * height;

        case GL_RGB:
        case GL_RGB8:
        case GL_RGB8_SNORM:
        case GL_RGB16_SNORM:
        case GL_SRGB8:
        case GL_RGB16F:
        case GL_RGB32F:
        case GL_RGB8I:
        case GL_RGB8UI:
        case GL_RGB16I:
        case GL_RGB16UI:
        case GL_RGB32I:
        case GL_RGB32UI:
            return align_up(width * cgGlDataSize(data_type) * 3, alignment) * height;

        case GL_RGBA:
        case GL_RGBA8:
        case GL_RGBA8_SNORM:
        case GL_SRGB8_ALPHA8:
        case GL_RGBA16F:
        case GL_RGBA32F:
        case GL_RGBA8I:
        case GL_RGBA8UI:
        case GL_RGBA16I:
        case GL_RGBA16UI:
        case GL_RGBA32I:
        case GL_RGBA32UI:
            return align_up(width * cgGlDataSize(data_type) * 4, alignment) * height;

        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
            // these formats operate on 4x4 blocks of pixels, so if a dimension
            // is not evenly divisible by four, it needs to be rounded up.
            return align_up(((width + 3) >> 2) * cgGlBytesPerBlock(internal_format), alignment) * ((height + 3) >> 2);

        default:
            break;
    }
    return 0;
}

/// @summary Calculates the dimension of an image (width, height, etc.) rounded up to the next alignment boundary based on the internal format.
/// @summary internal_format The OpenGL internal format value, for example, GL_RGBA. The most common compressed formats are supported (DXT/S3TC).
/// @param dimension The dimension value (width, height, etc.), in pixels.
/// @return The dimension value padded up to the next alignment boundary. The returned value is always specified in pixels.
internal_function size_t
cgGlImageDimension
(
    GLenum internal_format,
    size_t dimension
)
{
    switch (internal_format)
    {
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
            // these formats operate on 4x4 blocks of pixels, so if a dimension
            // is not evenly divisible by four, it needs to be rounded up.
            return (((dimension + 3) >> 2) * cgGlBlockDimension(internal_format));

        default:
            break;
    }
    return dimension;
}

/// @summary Given an OpenGL internal format type value, determines the corresponding pixel layout.
/// @param internal_format The OpenGL internal format value. See the documentation for glTexImage2D(), internalFormat argument.
/// @return The OpenGL base internal format values. See the documentation for glTexImage2D(), format argument.
internal_function GLenum
cgGlPixelLayout
(
    GLenum internal_format
)
{
    switch (internal_format)
    {
        case GL_DEPTH_COMPONENT:                     return GL_DEPTH_COMPONENT;
        case GL_DEPTH_STENCIL:                       return GL_DEPTH_STENCIL;
        case GL_RED:                                 return GL_RED;
        case GL_RG:                                  return GL_RG;
        case GL_RGB:                                 return GL_RGB;
        case GL_RGBA:                                return GL_BGRA;
        case GL_R8:                                  return GL_RED;
        case GL_R8_SNORM:                            return GL_RED;
        case GL_R16:                                 return GL_RED;
        case GL_R16_SNORM:                           return GL_RED;
        case GL_RG8:                                 return GL_RG;
        case GL_RG8_SNORM:                           return GL_RG;
        case GL_RG16:                                return GL_RG;
        case GL_RG16_SNORM:                          return GL_RG;
        case GL_R3_G3_B2:                            return GL_RGB;
        case GL_RGB4:                                return GL_RGB;
        case GL_RGB5:                                return GL_RGB;
        case GL_RGB8:                                return GL_RGB;
        case GL_RGB8_SNORM:                          return GL_RGB;
        case GL_RGB10:                               return GL_RGB;
        case GL_RGB12:                               return GL_RGB;
        case GL_RGB16_SNORM:                         return GL_RGB;
        case GL_RGBA2:                               return GL_RGB;
        case GL_RGBA4:                               return GL_RGB;
        case GL_RGB5_A1:                             return GL_RGBA;
        case GL_RGBA8:                               return GL_BGRA;
        case GL_RGBA8_SNORM:                         return GL_BGRA;
        case GL_RGB10_A2:                            return GL_RGBA;
        case GL_RGB10_A2UI:                          return GL_RGBA;
        case GL_RGBA12:                              return GL_RGBA;
        case GL_RGBA16:                              return GL_BGRA;
        case GL_SRGB8:                               return GL_RGB;
        case GL_SRGB8_ALPHA8:                        return GL_BGRA;
        case GL_R16F:                                return GL_RED;
        case GL_RG16F:                               return GL_RG;
        case GL_RGB16F:                              return GL_RGB;
        case GL_RGBA16F:                             return GL_BGRA;
        case GL_R32F:                                return GL_RED;
        case GL_RG32F:                               return GL_RG;
        case GL_RGB32F:                              return GL_RGB;
        case GL_RGBA32F:                             return GL_BGRA;
        case GL_R11F_G11F_B10F:                      return GL_RGB;
        case GL_RGB9_E5:                             return GL_RGB;
        case GL_R8I:                                 return GL_RED;
        case GL_R8UI:                                return GL_RED;
        case GL_R16I:                                return GL_RED;
        case GL_R16UI:                               return GL_RED;
        case GL_R32I:                                return GL_RED;
        case GL_R32UI:                               return GL_RED;
        case GL_RG8I:                                return GL_RG;
        case GL_RG8UI:                               return GL_RG;
        case GL_RG16I:                               return GL_RG;
        case GL_RG16UI:                              return GL_RG;
        case GL_RG32I:                               return GL_RG;
        case GL_RG32UI:                              return GL_RG;
        case GL_RGB8I:                               return GL_RGB;
        case GL_RGB8UI:                              return GL_RGB;
        case GL_RGB16I:                              return GL_RGB;
        case GL_RGB16UI:                             return GL_RGB;
        case GL_RGB32I:                              return GL_RGB;
        case GL_RGB32UI:                             return GL_RGB;
        case GL_RGBA8I:                              return GL_BGRA;
        case GL_RGBA8UI:                             return GL_BGRA;
        case GL_RGBA16I:                             return GL_BGRA;
        case GL_RGBA16UI:                            return GL_BGRA;
        case GL_RGBA32I:                             return GL_BGRA;
        case GL_RGBA32UI:                            return GL_BGRA;
        case GL_COMPRESSED_RED:                      return GL_RED;
        case GL_COMPRESSED_RG:                       return GL_RG;
        case GL_COMPRESSED_RGB:                      return GL_RGB;
        case GL_COMPRESSED_RGBA:                     return GL_RGBA;
        case GL_COMPRESSED_SRGB:                     return GL_RGB;
        case GL_COMPRESSED_SRGB_ALPHA:               return GL_RGBA;
        case GL_COMPRESSED_RED_RGTC1:                return GL_RED;
        case GL_COMPRESSED_SIGNED_RED_RGTC1:         return GL_RED;
        case GL_COMPRESSED_RG_RGTC2:                 return GL_RG;
        case GL_COMPRESSED_SIGNED_RG_RGTC2:          return GL_RG;
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:        return GL_RGB;
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:       return GL_RGBA;
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:       return GL_RGBA;
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:       return GL_RGBA;
        case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:       return GL_RGB;
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT: return GL_RGBA;
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT: return GL_RGBA;
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT: return GL_RGBA;
        default:
            break;
    }
    return GL_NONE;
}

/// @summary Given an OpenGL sampler type value, determines the corresponding texture bind target identifier.
/// @param sampler_type The OpenGL sampler type, for example, GL_SAMPLER_2D.
/// @return The corresponding bind target, for example, GL_TEXTURE_2D.
internal_function GLenum
cgGlTextureTarget
(
    GLenum sampler_type
)
{
    switch (sampler_type)
    {
        case GL_SAMPLER_1D:
        case GL_INT_SAMPLER_1D:
        case GL_UNSIGNED_INT_SAMPLER_1D:
        case GL_SAMPLER_1D_SHADOW:
            return GL_TEXTURE_1D;

        case GL_SAMPLER_2D:
        case GL_INT_SAMPLER_2D:
        case GL_UNSIGNED_INT_SAMPLER_2D:
        case GL_SAMPLER_2D_SHADOW:
            return GL_TEXTURE_2D;

        case GL_SAMPLER_3D:
        case GL_INT_SAMPLER_3D:
        case GL_UNSIGNED_INT_SAMPLER_3D:
            return GL_TEXTURE_3D;

        case GL_SAMPLER_CUBE:
        case GL_INT_SAMPLER_CUBE:
        case GL_UNSIGNED_INT_SAMPLER_CUBE:
        case GL_SAMPLER_CUBE_SHADOW:
            return GL_TEXTURE_CUBE_MAP;

        case GL_SAMPLER_1D_ARRAY:
        case GL_SAMPLER_1D_ARRAY_SHADOW:
        case GL_INT_SAMPLER_1D_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
            return GL_TEXTURE_1D_ARRAY;

        case GL_SAMPLER_2D_ARRAY:
        case GL_SAMPLER_2D_ARRAY_SHADOW:
        case GL_INT_SAMPLER_2D_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
            return GL_TEXTURE_2D_ARRAY;

        case GL_SAMPLER_BUFFER:
        case GL_INT_SAMPLER_BUFFER:
        case GL_UNSIGNED_INT_SAMPLER_BUFFER:
            return GL_TEXTURE_BUFFER;

        case GL_SAMPLER_2D_RECT:
        case GL_SAMPLER_2D_RECT_SHADOW:
        case GL_INT_SAMPLER_2D_RECT:
        case GL_UNSIGNED_INT_SAMPLER_2D_RECT:
            return GL_TEXTURE_RECTANGLE;

        case GL_SAMPLER_2D_MULTISAMPLE:
        case GL_INT_SAMPLER_2D_MULTISAMPLE:
        case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
            return GL_TEXTURE_2D_MULTISAMPLE;

        case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
        case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
        case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
            return GL_TEXTURE_2D_MULTISAMPLE_ARRAY;

        default:
            break;
    }
    return GL_TEXTURE_1D;
}

/// @summary Given a value from the DXGI_FORMAT enumeration, determine the appropriate OpenGL format, base format and data type values. This is useful when loading texture data from a DDS container.
/// @param dxgi A value of the DXGI_FORMAT or dxgi_format_e enumeration.
/// @param out_internalformat On return, stores the corresponding OpenGL internal format.
/// @param out_baseformat On return, stores the corresponding OpenGL base format (layout).
/// @param out_datatype On return, stores the corresponding OpenGL data type.
/// @return true if the input format could be mapped to OpenGL.
internal_function bool
cgGlDxgiFormatToGL
(
    uint32_t dxgi,
    GLenum  &out_internalformat,
    GLenum  &out_format,
    GLenum  &out_datatype
)
{
    switch (dxgi)
    {
        case DXGI_FORMAT_UNKNOWN:
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case DXGI_FORMAT_R32G32B32_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R32G32_TYPELESS:
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R16G16_TYPELESS:
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_R8G8_TYPELESS:
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case DXGI_FORMAT_R1_UNORM:
        case DXGI_FORMAT_R8G8_B8G8_UNORM:
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_AYUV:
        case DXGI_FORMAT_Y410:
        case DXGI_FORMAT_Y416:
        case DXGI_FORMAT_NV12:
        case DXGI_FORMAT_P010:
        case DXGI_FORMAT_P016:
        case DXGI_FORMAT_420_OPAQUE:
        case DXGI_FORMAT_YUY2:
        case DXGI_FORMAT_Y210:
        case DXGI_FORMAT_Y216:
        case DXGI_FORMAT_NV11:
        case DXGI_FORMAT_AI44:
        case DXGI_FORMAT_IA44:
            break;
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            out_internalformat = GL_RGBA32F;
            out_format         = GL_BGRA;
            out_datatype       = GL_FLOAT;
            return true;
        case DXGI_FORMAT_R32G32B32A32_UINT:
            out_internalformat = GL_RGBA32UI;
            out_format         = GL_BGRA_INTEGER;
            out_datatype       = GL_UNSIGNED_INT;
            return true;
        case DXGI_FORMAT_R32G32B32A32_SINT:
            out_internalformat = GL_RGBA32I;
            out_format         = GL_BGRA_INTEGER;
            out_datatype       = GL_INT;
            return true;
        case DXGI_FORMAT_R32G32B32_FLOAT:
            out_internalformat = GL_RGB32F;
            out_format         = GL_BGR;
            out_datatype       = GL_FLOAT;
            return true;
        case DXGI_FORMAT_R32G32B32_UINT:
            out_internalformat = GL_RGB32UI;
            out_format         = GL_BGR_INTEGER;
            out_datatype       = GL_UNSIGNED_INT;
            return true;
        case DXGI_FORMAT_R32G32B32_SINT:
            out_internalformat = GL_RGB32I;
            out_format         = GL_BGR_INTEGER;
            out_datatype       = GL_INT;
            return true;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            out_internalformat = GL_RGBA16F;
            out_format         = GL_BGRA;
            out_datatype       = GL_HALF_FLOAT;
            return true;
        case DXGI_FORMAT_R16G16B16A16_UNORM:
            out_internalformat = GL_RGBA16;
            out_format         = GL_BGRA_INTEGER;
            out_datatype       = GL_UNSIGNED_SHORT;
            return true;
        case DXGI_FORMAT_R16G16B16A16_UINT:
            out_internalformat = GL_RGBA16UI;
            out_format         = GL_BGRA_INTEGER;
            out_datatype       = GL_UNSIGNED_SHORT;
            return true;
        case DXGI_FORMAT_R16G16B16A16_SNORM:
            out_internalformat = GL_RGBA16_SNORM;
            out_format         = GL_BGRA_INTEGER;
            out_datatype       = GL_SHORT;
            return true;
        case DXGI_FORMAT_R16G16B16A16_SINT:
            out_internalformat = GL_RGBA16I;
            out_format         = GL_BGRA_INTEGER;
            out_datatype       = GL_SHORT;
            return true;
        case DXGI_FORMAT_R32G32_FLOAT:
            out_internalformat = GL_RG32F;
            out_format         = GL_RG;
            out_datatype       = GL_FLOAT;
            return true;
        case DXGI_FORMAT_R32G32_UINT:
            out_internalformat = GL_RG32UI;
            out_format         = GL_RG;
            out_datatype       = GL_UNSIGNED_INT;
            return true;
        case DXGI_FORMAT_R32G32_SINT:
            out_internalformat = GL_RG32I;
            out_format         = GL_RG;
            out_datatype       = GL_INT;
            return true;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            out_internalformat = GL_DEPTH_STENCIL;
            out_format         = GL_DEPTH_STENCIL;
            out_datatype       = GL_FLOAT; // ???
            return true;
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            out_internalformat = GL_RG32F;
            out_format         = GL_RG;
            out_datatype       = GL_FLOAT;
            return true;
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            return true;
        case DXGI_FORMAT_R10G10B10A2_UNORM:
            out_internalformat = GL_RGB10_A2;
            out_format         = GL_BGRA;
            out_datatype       = GL_UNSIGNED_INT_2_10_10_10_REV;
            return true;
        case DXGI_FORMAT_R10G10B10A2_UINT:
            out_internalformat = GL_RGB10_A2UI;
            out_format         = GL_BGRA;
            out_datatype       = GL_UNSIGNED_INT_2_10_10_10_REV;
            return true;
        case DXGI_FORMAT_R11G11B10_FLOAT:
            out_internalformat = GL_R11F_G11F_B10F;
            out_format         = GL_BGR;
            out_datatype       = GL_FLOAT; // ???
            return true;
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            out_internalformat = GL_RGBA8;
            out_format         = GL_BGRA;
            out_datatype       = GL_UNSIGNED_INT_8_8_8_8_REV;
            return true;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            out_internalformat = GL_SRGB8_ALPHA8;
            out_format         = GL_BGRA;
            out_datatype       = GL_UNSIGNED_INT_8_8_8_8_REV;
            return true;
        case DXGI_FORMAT_R8G8B8A8_UINT:
            out_internalformat = GL_RGBA8UI;
            out_format         = GL_BGRA;
            out_datatype       = GL_UNSIGNED_INT_8_8_8_8_REV;
            return true;
        case DXGI_FORMAT_R8G8B8A8_SNORM:
            out_internalformat = GL_RGBA8_SNORM;
            out_format         = GL_BGRA;
            out_datatype       = GL_UNSIGNED_INT_8_8_8_8_REV;
            return true;
        case DXGI_FORMAT_R8G8B8A8_SINT:
            out_internalformat = GL_RGBA8I;
            out_format         = GL_BGRA;
            out_datatype       = GL_BYTE;
            return true;
        case DXGI_FORMAT_R16G16_FLOAT:
            out_internalformat = GL_RG16F;
            out_format         = GL_RG;
            out_datatype       = GL_HALF_FLOAT;
            return true;
        case DXGI_FORMAT_R16G16_UNORM:
            out_internalformat = GL_RG16;
            out_format         = GL_RG;
            out_datatype       = GL_UNSIGNED_SHORT;
            return true;
        case DXGI_FORMAT_R16G16_UINT:
            out_internalformat = GL_RG16UI;
            out_format         = GL_RG;
            out_datatype       = GL_UNSIGNED_SHORT;
            return true;
        case DXGI_FORMAT_R16G16_SNORM:
            out_internalformat = GL_RG16_SNORM;
            out_format         = GL_RG;
            out_datatype       = GL_SHORT;
            return true;
        case DXGI_FORMAT_R16G16_SINT:
            out_internalformat = GL_RG16I;
            out_format         = GL_RG;
            out_datatype       = GL_SHORT;
            return true;
        case DXGI_FORMAT_D32_FLOAT:
            out_internalformat = GL_DEPTH_COMPONENT;
            out_format         = GL_DEPTH_COMPONENT;
            out_datatype       = GL_FLOAT;
            return true;
        case DXGI_FORMAT_R32_FLOAT:
            out_internalformat = GL_R32F;
            out_format         = GL_RED;
            out_datatype       = GL_FLOAT;
            return true;
        case DXGI_FORMAT_R32_UINT:
            out_internalformat = GL_R32UI;
            out_format         = GL_RED;
            out_datatype       = GL_UNSIGNED_INT;
            return true;
        case DXGI_FORMAT_R32_SINT:
            out_internalformat = GL_R32I;
            out_format         = GL_RED;
            out_datatype       = GL_INT;
            return true;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            out_internalformat = GL_DEPTH_STENCIL;
            out_format         = GL_DEPTH_STENCIL;
            out_datatype       = GL_UNSIGNED_INT;
            return true;
        case DXGI_FORMAT_R8G8_UNORM:
            out_internalformat = GL_RG8;
            out_format         = GL_RG;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case DXGI_FORMAT_R8G8_UINT:
            out_internalformat = GL_RG8UI;
            out_format         = GL_RG;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case DXGI_FORMAT_R8G8_SNORM:
            out_internalformat = GL_RG8_SNORM;
            out_format         = GL_RG;
            out_datatype       = GL_BYTE;
            return true;
        case DXGI_FORMAT_R8G8_SINT:
            out_internalformat = GL_RG8I;
            out_format         = GL_RG;
            out_datatype       = GL_BYTE;
            return true;
        case DXGI_FORMAT_R16_FLOAT:
            out_internalformat = GL_R16F;
            out_format         = GL_RED;
            out_datatype       = GL_HALF_FLOAT;
            return true;
        case DXGI_FORMAT_D16_UNORM:
            out_internalformat = GL_DEPTH_COMPONENT;
            out_format         = GL_DEPTH_COMPONENT;
            out_datatype       = GL_UNSIGNED_SHORT;
            return true;
        case DXGI_FORMAT_R16_UNORM:
            out_internalformat = GL_R16;
            out_format         = GL_RED;
            out_datatype       = GL_UNSIGNED_SHORT;
            return true;
        case DXGI_FORMAT_R16_UINT:
            out_internalformat = GL_R16UI;
            out_format         = GL_RED;
            out_datatype       = GL_UNSIGNED_SHORT;
            return true;
        case DXGI_FORMAT_R16_SNORM:
            out_internalformat = GL_R16_SNORM;
            out_format         = GL_RED;
            out_datatype       = GL_SHORT;
            return true;
        case DXGI_FORMAT_R16_SINT:
            out_internalformat = GL_R16I;
            out_format         = GL_RED;
            out_datatype       = GL_SHORT;
            return true;
        case DXGI_FORMAT_R8_UNORM:
            out_internalformat = GL_R8;
            out_format         = GL_RED;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case DXGI_FORMAT_R8_UINT:
            out_internalformat = GL_R8UI;
            out_format         = GL_RED;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case DXGI_FORMAT_R8_SNORM:
            out_internalformat = GL_R8_SNORM;
            out_format         = GL_RED;
            out_datatype       = GL_BYTE;
            return true;
        case DXGI_FORMAT_R8_SINT:
            out_internalformat = GL_R8I;
            out_format         = GL_RED;
            out_datatype       = GL_BYTE;
            return true;
        case DXGI_FORMAT_A8_UNORM:
            out_internalformat = GL_R8;
            out_format         = GL_RED;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
            out_internalformat = GL_RGB9_E5;
            out_format         = GL_RGB;
            out_datatype       = GL_UNSIGNED_INT;
            return true;
        case DXGI_FORMAT_BC1_UNORM:
            out_internalformat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            out_format         = GL_RGBA;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case DXGI_FORMAT_BC1_UNORM_SRGB:
            out_internalformat = 0x8C4D;
            out_format         = GL_RGBA;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case DXGI_FORMAT_BC3_UNORM:
            out_internalformat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            out_format         = GL_RGBA;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case DXGI_FORMAT_BC3_UNORM_SRGB:
            out_internalformat = 0x8C4E;
            out_format         = GL_RGBA;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case DXGI_FORMAT_BC5_UNORM:
            out_internalformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            out_format         = GL_RGBA;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case DXGI_FORMAT_B5G6R5_UNORM:
            out_internalformat = GL_RGB;
            out_format         = GL_BGR;
            out_datatype       = GL_UNSIGNED_SHORT_5_6_5_REV;
            return true;
        case DXGI_FORMAT_B5G5R5A1_UNORM:
            out_internalformat = GL_RGBA;
            out_format         = GL_BGRA;
            out_datatype       = GL_UNSIGNED_SHORT_1_5_5_5_REV;
            return true;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
            out_internalformat = GL_RGBA8;
            out_format         = GL_BGRA;
            out_datatype       = GL_UNSIGNED_INT_8_8_8_8_REV;
            return true;
        case DXGI_FORMAT_B8G8R8X8_UNORM:
            out_internalformat = GL_RGBA8;
            out_format         = GL_BGR;
            out_datatype       = GL_UNSIGNED_INT_8_8_8_8_REV;
            return true;
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
            out_internalformat = GL_RGB10_A2;
            out_format         = GL_BGRA;
            out_datatype       = GL_UNSIGNED_INT_2_10_10_10_REV;
            return true;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            out_internalformat = GL_SRGB8_ALPHA8;
            out_format         = GL_BGRA;
            out_datatype       = GL_UNSIGNED_INT_8_8_8_8_REV;
            return true;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            out_internalformat = GL_SRGB8_ALPHA8;
            out_format         = GL_BGR;
            out_datatype       = GL_UNSIGNED_INT_8_8_8_8_REV;
            return true;
        case DXGI_FORMAT_BC6H_UF16:
            out_internalformat = 0x8E8F;
            out_format         = GL_RGB;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case DXGI_FORMAT_BC6H_SF16:
            out_internalformat = 0x8E8E;
            out_format         = GL_RGB;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case DXGI_FORMAT_BC7_UNORM:
            out_internalformat = 0x8E8C;
            out_format         = GL_RGBA;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            out_internalformat = 0x8E8D;
            out_format         = GL_RGBA;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case DXGI_FORMAT_P8:
            out_internalformat = GL_R8;
            out_format         = GL_RED;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case DXGI_FORMAT_A8P8:
            out_internalformat = GL_RG8;
            out_format         = GL_RG;
            out_datatype       = GL_UNSIGNED_BYTE;
            return true;
        case DXGI_FORMAT_B4G4R4A4_UNORM:
            out_internalformat = GL_RGBA4;
            out_format         = GL_BGRA;
            out_datatype       = GL_UNSIGNED_SHORT_4_4_4_4_REV;
            return true;
        default:
            break;
    }
    return false;
}

/// @summary Computes the number of levels in a mipmap chain given the dimensions of the highest resolution level.
/// @param width The width of the highest resolution level, in pixels.
/// @param height The height of the highest resolution level, in pixels.
/// @param slice_count The number of slices of the highest resolution level. For everything except a 3D image, this value should be specified as 1.
/// @param max_levels The maximum number of levels in the mipmap chain. If there is no limit, this value should be specified as 0.
internal_function size_t
cgGlLevelCount
(
    size_t width,
    size_t height,
    size_t slice_count,
    size_t max_levels
)
{
    size_t levels = 0;
    size_t major  = 0;

    // select the largest of (width, height, slice_count):
    major = (width > height)      ? width : height;
    major = (major > slice_count) ? major : slice_count;
    // calculate the number of levels down to 1 in the largest dimension.
    while (major > 0)
    {
        major >>= 1;
        levels += 1;
    }
    if (max_levels == 0) return levels;
    else return (max_levels < levels) ? max_levels : levels;
}

/// @summary Computes the dimension (width, height or number of slices) of a particular level in a mipmap chain given the dimension for the highest resolution image.
/// @param dimension The dimension in the highest resolution image.
/// @param level_index The index of the target mip-level, with index 0 representing the highest resolution image.
/// @return The dimension in the specified mip level.
internal_function size_t
cgGlLevelDimension
(
    size_t dimension,
    size_t level_index
)
{
    size_t  l_dimension  = dimension >> level_index;
    return (l_dimension == 0) ? 1 : l_dimension;
}

/// @summary Given basic image attributes, builds a complete description of the levels in a mipmap chain.
/// @param internal_format The OpenGL internal format, for example GL_RGBA. See the documentation for glTexImage2D(), internalFormat argument.
/// @param data_type The OpenGL data type, for example, GL_UNSIGNED_BYTE.
/// @param width The width of the highest resolution image, in pixels.
/// @param height The height of the highest resolution image, in pixels.
/// @param slice_count The number of slices in the highest resolution image. For all image types other than 3D images, specify 1 for this value.
/// @param alignment The alignment requirement of the OpenGL implementation, corresponding to the pname of GL_PACK_ALIGNMENT or GL_UNPACK_ALIGNMENT for the glPixelStorei function. The specification default is 4.
/// @param max_levels The maximum number of levels in the mipmap chain. To describe all possible levels, specify 0 for this value.
/// @param level_desc The array of level descriptors to populate.
/*internal_function void
cgGlDescribeMipmaps
(
    GLenum         internal_format,
    GLenum         data_type,
    size_t         width,
    size_t         height,
    size_t         slice_count,
    size_t         alignment,
    size_t         max_levels,
    CG_LEVEL_DESC *level_desc
)
{
    size_t slices      = slice_count;
    GLenum type        = data_type;
    GLenum format      = internal_format;
    GLenum layout      = cgGlPixelLayout(format);
    size_t bytes_per_e = cgGlBytesPerElement(format, type);
    size_t num_levels  = cgGlLevelCount(width, height, slices, max_levels);
    for (size_t i = 0; i < num_levels; ++i)
    {
        size_t lw = cgGlLevelDimension(width,  i);
        size_t lh = cgGlLevelDimension(height, i);
        size_t ls = cgGlLevelDimension(slices, i);
        level_desc[i].Index           = i;
        level_desc[i].Width           = cgGlImageDimension(format, lw);
        level_desc[i].Height          = cgGlImageDimension(format, lh);
        level_desc[i].Slices          = ls;
        level_desc[i].BytesPerElement = bytes_per_e;
        level_desc[i].BytesPerRow     = cgGlBytesPerRow(format, type, level_desc[i].Width, alignment);
        level_desc[i].BytesPerSlice   = level_desc[i].BytesPerRow * level_desc[i].Height;
        level_desc[i].Layout          = layout;
        level_desc[i].Format          = internal_format;
        level_desc[i].DataType        = data_type;
    }
}*/

/// @summary Given basic texture attributes, allocates storage for all levels of a texture, such that the texture is said to be complete. This should only be performed once per-texture. After calling this function, the texture object attributes should be considered immutable. Transfer data to the texture using the gl_transfer_pixels_h2d() function. The wrapping modes are always set to GL_CLAMP_TO_EDGE. The caller is responsible for creating and binding the texture object prior to calling this function.
/// @param display The display managing the rendering context.
/// @param target The OpenGL texture target, defining the texture type.
/// @param internal_format The OpenGL internal format, for example GL_RGBA8.
/// @param data_type The OpenGL data type, for example, GL_UNSIGNED_INT_8_8_8_8_REV.
/// @param min_filter The minification filter to use.
/// @param mag_filter The magnification filter to use.
/// @param width The width of the highest resolution image, in pixels.
/// @param height The height of the highest resolution image, in pixels.
/// @param slice_count The number of slices in the highest resolution image. If the @a target argument specifies an array target, this represents the number of items in the texture array. For 3D textures, it represents the number of slices in the image. For all other types, this value must be 1.
/// @param max_levels The maximum number of levels in the mipmap chain. To define all possible levels, specify 0 for this value.
internal_function void
cgGlTextureStorage
(
    CG_DISPLAY *display,
    GLenum      target,
    GLenum      internal_format,
    GLenum      data_type,
    GLenum      min_filter,
    GLenum      mag_filter,
    size_t      width,
    size_t      height,
    size_t      slice_count,
    size_t      max_levels
)
{
    GLenum layout = cgGlPixelLayout(internal_format);

    if (max_levels == 0)
    {
        // specify mipmaps all the way down to 1x1x1.
        max_levels  = cgGlLevelCount(width, height, slice_count, max_levels);
    }

    // make sure that no PBO is bound as the unpack target.
    // we don't want to copy data to the texture now.
    glBindBuffer (GL_PIXEL_UNPACK_BUFFER, 0);

    // specify the maximum number of mipmap levels.
    if (target != GL_TEXTURE_RECTANGLE)
    {
        glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(target, GL_TEXTURE_MAX_LEVEL,  GLint(max_levels - 1));
    }
    else
    {
        // rectangle textures don't support mipmaps.
        glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(target, GL_TEXTURE_MAX_LEVEL,  0);
    }

    // specify the filtering and wrapping modes.
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, mag_filter);
    glTexParameteri(target, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_R,     GL_CLAMP_TO_EDGE);

    // use glTexImage to allocate storage for each mip-level of the texture.
    switch (target)
    {
    case GL_TEXTURE_1D:
        {
            for (size_t lod = 0; lod < max_levels; ++lod)
            {
                size_t lw = cgGlLevelDimension(width, lod);
                glTexImage1D(target, GLint(lod), internal_format, GLsizei(lw), 0, layout, data_type, NULL);
            }
        }
        break;

    case GL_TEXTURE_1D_ARRAY:
        {
            // 1D array textures specify slice_count for height.
            for (size_t lod = 0; lod < max_levels; ++lod)
            {
                size_t lw = cgGlLevelDimension(width, lod);
                glTexImage2D(target, GLint(lod), internal_format, GLsizei(lw), GLsizei(slice_count), 0, layout, data_type, NULL);
            }
        }
        break;

    case GL_TEXTURE_RECTANGLE:
        {
            // rectangle textures don't support mipmaps.
            glTexImage2D(target, 0, internal_format, GLsizei(width), GLsizei(height), 0, layout, data_type, NULL);
        }
        break;

    case GL_TEXTURE_2D:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        {
            for (size_t lod = 0; lod < max_levels; ++lod)
            {
                size_t lw = cgGlLevelDimension(width,  lod);
                size_t lh = cgGlLevelDimension(height, lod);
                glTexImage2D(target, GLint(lod), internal_format, GLsizei(lw), GLsizei(lh), 0, layout, data_type, NULL);
            }
        }
        break;

    case GL_TEXTURE_2D_ARRAY:
    case GL_TEXTURE_CUBE_MAP_ARRAY:
        {
            // 2D array texture specify slice_count as the number of
            // items; the number of items is not decreased with LOD.
            for (size_t lod = 0; lod < max_levels; ++lod)
            {
                size_t lw = cgGlLevelDimension(width,  lod);
                size_t lh = cgGlLevelDimension(height, lod);
                glTexImage3D(target, GLint(lod), internal_format, GLsizei(lw), GLsizei(lh), GLsizei(slice_count), 0, layout, data_type, NULL);
            }
        }
        break;

    case GL_TEXTURE_3D:
        {
            for (size_t lod = 0; lod < max_levels; ++lod)
            {
                size_t lw = cgGlLevelDimension(width,       lod);
                size_t lh = cgGlLevelDimension(height,      lod);
                size_t ls = cgGlLevelDimension(slice_count, lod);
                glTexImage3D(target, GLint(lod), internal_format, GLsizei(lw), GLsizei(lh), GLsizei(ls), 0, layout, data_type, NULL);
            }
        }
        break;
    }
}
/*
/// @summary Copies pixel data from the device (GPU) to the host (CPU). The pixel data consists of the framebuffer contents, or the contents of a single mip-level of a texture image.
/// @param display The display managing the rendering context.
/// @param transfer An object describing the transfer operation to execute.
internal_function void
cgGlTransferPixelsDeviceToHost
(
    CG_DISPLAY            *display,
    CG_PIXEL_TRANSFER_D2H *transfer
)
{
    if (transfer->PackBuffer != 0)
    {
        // select the PBO as the target of the pack operation.
        glBindBuffer(GL_PIXEL_PACK_BUFFER, transfer->PackBuffer);
    }
    else
    {
        // select the client memory as the target of the pack operation.
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }
    if (transfer->TargetWidth != transfer->TransferWidth)
    {
        // transferring into a sub-rectangle of the image; tell GL how
        // many pixels are in a single row of the target image.
        glPixelStorei(GL_PACK_ROW_LENGTH,   GLint(transfer->TargetWidth));
        glPixelStorei(GL_PACK_IMAGE_HEIGHT, GLint(transfer->TargetHeight));
    }

    // perform the setup necessary to have GL calculate any byte offsets.
    if (transfer->TargetX != 0) glPixelStorei(GL_PACK_SKIP_PIXELS, GLint(transfer->TargetX));
    if (transfer->TargetY != 0) glPixelStorei(GL_PACK_SKIP_ROWS,   GLint(transfer->TargetY));
    if (transfer->TargetZ != 0) glPixelStorei(GL_PACK_SKIP_IMAGES, GLint(transfer->TargetZ));

    if (cgGlBytesPerBlock(transfer->Format) > 0)
    {
        // the texture image is compressed; use glGetCompressedTexImage.
        switch (transfer->Target)
        {
            case GL_TEXTURE_1D:
            case GL_TEXTURE_2D:
            case GL_TEXTURE_3D:
            case GL_TEXTURE_CUBE_MAP_ARRAY:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
                {
                    glGetCompressedTexImage(transfer->Target, GLint(transfer->SourceIndex), transfer->TransferBuffer);
                }
                break;
        }
    }
    else
    {
        // the image is not compressed; read the framebuffer with
        // glReadPixels or the texture image using glGetTexImage.
        switch (transfer->Target)
        {
            case GL_READ_FRAMEBUFFER:
                {
                    // remember, x and y identify the lower-left corner.
                    glReadPixels(GLint(transfer->TransferX), GLint(transfer->TransferY), GLint(transfer->TransferWidth), GLint(transfer->TransferHeight), transfer->Layout, transfer->DataType, transfer->TransferBuffer);
                }
                break;

            case GL_TEXTURE_1D:
            case GL_TEXTURE_2D:
            case GL_TEXTURE_3D:
            case GL_TEXTURE_1D_ARRAY:
            case GL_TEXTURE_2D_ARRAY:
            case GL_TEXTURE_RECTANGLE:
            case GL_TEXTURE_CUBE_MAP_ARRAY:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
                {
                    glGetTexImage(transfer->Target, GLint(transfer->SourceIndex), transfer->Layout, transfer->DataType, transfer->TransferBuffer);
                }
                break;
        }
    }

    // restore the pack state values to their defaults.
    if (transfer->PackBuffer  != 0)
        glBindBuffer(GL_PIXEL_PACK_BUFFER,  0);
    if (transfer->TargetWidth != transfer->TransferWidth)
    {
        glPixelStorei(GL_PACK_ROW_LENGTH,   0);
        glPixelStorei(GL_PACK_IMAGE_HEIGHT, 0);
    }
    if (transfer->TargetX != 0) glPixelStorei(GL_PACK_SKIP_PIXELS,  0);
    if (transfer->TargetY != 0) glPixelStorei(GL_PACK_SKIP_ROWS,    0);
    if (transfer->TargetZ != 0) glPixelStorei(GL_PACK_SKIP_IMAGES,  0);
}

/// @summary Copies pixel data from the host (CPU) to the device (GPU). The pixel data is copied to a single mip-level of a texture image.
/// @param display The display managing the rendering context.
/// @param transfer An object describing the transfer operation to execute.
internal_function void
cgGlPixelTransferHostToDevice
(
    CG_DISPLAY            *display,
    CG_PIXEL_TRANSFER_H2D *transfer
)
{
    if (transfer->UnpackBuffer != 0)
    {   // select the PBO as the source of the unpack operation.
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, transfer->UnpackBuffer);
    }
    else
    {   // select the client memory as the source of the unpack operation.
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }
    if (transfer->SourceWidth != transfer->TransferWidth)
    {   // transferring a sub-rectangle of the image; tell GL how many
        // pixels are in a single row of the source image.
        glPixelStorei(GL_UNPACK_ROW_LENGTH, GLint(transfer->SourceWidth));
    }
    if (transfer->TransferSlices > 1)
    {   // transferring an image volume; tell GL how many rows per-slice
        // in the source image, since we may only be transferring a sub-volume.
        glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, GLint(transfer->SourceHeight));
    }

    // perform the setup necessary to have GL calculate any byte offsets.
    if (transfer->SourceX != 0) glPixelStorei(GL_UNPACK_SKIP_PIXELS, GLint(transfer->SourceX));
    if (transfer->SourceY != 0) glPixelStorei(GL_UNPACK_SKIP_ROWS,   GLint(transfer->SourceY));
    if (transfer->SourceZ != 0) glPixelStorei(GL_UNPACK_SKIP_IMAGES, GLint(transfer->SourceZ));

    if (cgGlBytesPerBlock(transfer->Format) > 0)
    {
        // the image is compressed; use glCompressedTexSubImage to transfer.
        switch (transfer->Target)
        {
            case GL_TEXTURE_1D:
                {
                    glCompressedTexSubImage1D(transfer->Target, GLint(transfer->TargetIndex), GLint(transfer->TargetX), GLsizei(transfer->TransferWidth), transfer->Format, GLsizei(transfer->TransferSize), transfer->TransferBuffer);
                }
                break;

            case GL_TEXTURE_2D:
            case GL_TEXTURE_1D_ARRAY:
            case GL_TEXTURE_RECTANGLE:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
                {
                    glCompressedTexSubImage2D(transfer->Target, GLint(transfer->TargetIndex), GLint(transfer->TargetX), GLint(transfer->TargetY), GLsizei(transfer->TransferWidth), GLsizei(transfer->TransferHeight), transfer->Format, GLsizei(transfer->TransferSize), transfer->TransferBuffer);
                }
                break;

            case GL_TEXTURE_3D:
            case GL_TEXTURE_2D_ARRAY:
            case GL_TEXTURE_CUBE_MAP_ARRAY:
                {
                    glCompressedTexSubImage3D(transfer->Target, GLint(transfer->TargetIndex), GLint(transfer->TargetX), GLint(transfer->TargetY), GLint(transfer->TargetZ), GLsizei(transfer->TransferWidth), GLsizei(transfer->TransferHeight), GLsizei(transfer->TransferSlices), transfer->Format, GLsizei(transfer->TransferSize), transfer->TransferBuffer);
                }
                break;
        }
    }
    else
    {
        // the image is not compressed, use glTexSubImage to transfer data.
        switch (transfer->Target)
        {
            case GL_TEXTURE_1D:
                {
                    glTexSubImage1D(transfer->Target, GLint(transfer->TargetIndex), GLint(transfer->TargetX), GLsizei(transfer->TransferWidth), transfer->Format, transfer->DataType, transfer->TransferBuffer);
                }
                break;

            case GL_TEXTURE_2D:
            case GL_TEXTURE_1D_ARRAY:
            case GL_TEXTURE_RECTANGLE:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
                {
                    glTexSubImage2D(transfer->Target, GLint(transfer->TargetIndex), GLint(transfer->TargetX), GLint(transfer->TargetY), GLsizei(transfer->TransferWidth), GLsizei(transfer->TransferHeight), transfer->Format, transfer->DataType, transfer->TransferBuffer);
                }
                break;

            case GL_TEXTURE_3D:
            case GL_TEXTURE_2D_ARRAY:
            case GL_TEXTURE_CUBE_MAP_ARRAY:
                {
                    glTexSubImage3D(transfer->Target, GLint(transfer->TargetIndex), GLint(transfer->TargetX), GLint(transfer->TargetY), GLint(transfer->TargetZ), GLsizei(transfer->TransferWidth), GLsizei(transfer->TransferHeight), GLsizei(transfer->TransferSlices), transfer->Format, transfer->DataType, transfer->TransferBuffer);
                }
                break;
        }
    }

    // restore the unpack state values to their defaults.
    if (transfer->UnpackBuffer  != 0)
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER,  0);
    if (transfer->SourceWidth   != transfer->TransferWidth)
        glPixelStorei(GL_UNPACK_ROW_LENGTH,   0);
    if (transfer->TransferSlices > 1)
        glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
    if (transfer->SourceX != 0)
        glPixelStorei(GL_UNPACK_SKIP_PIXELS,  0);
    if (transfer->SourceY != 0)
        glPixelStorei(GL_UNPACK_SKIP_ROWS,    0);
    if (transfer->SourceZ != 0)
        glPixelStorei(GL_UNPACK_SKIP_IMAGES,  0);
}*/

/// @summary Convert a CGFX blend function value to the corresponding OpenGL enum.
/// @param factor The CGFX blend function value, one of cg_blend_function_e.
/// @return The corresponding OpenGL enum value, or GL_INVALID_ENUM.
internal_function GLenum
cgGlBlendFunction
(
    int func
)
{
    switch (func)
    {
        case CG_BLEND_FUNCTION_ADD             : return GL_FUNC_ADD;
        case CG_BLEND_FUNCTION_SUBTRACT        : return GL_FUNC_SUBTRACT;
        case CG_BLEND_FUNCTION_REVERSE_SUBTRACT: return GL_FUNC_REVERSE_SUBTRACT;
        case CG_BLEND_FUNCTION_MIN             : return GL_MIN;
        case CG_BLEND_FUNCTION_MAX             : return GL_MAX;
        default: break;
    }
    return GL_INVALID_ENUM;
}

/// @summary Convert a CGFX blend factor value to the corresponding OpenGL enum.
/// @param factor The CGFX blend factor value, one of cg_blend_factor_e.
/// @return The corresponding OpenGL enum value, or GL_INVALID_ENUM.
internal_function GLenum
cgGlBlendFactor
(
    int factor
)
{
    switch (factor)
    {
        case CG_BLEND_FACTOR_ZERO           : return GL_ZERO;
        case CG_BLEND_FACTOR_ONE            : return GL_ONE;
        case CG_BLEND_FACTOR_SRC_COLOR      : return GL_SRC_COLOR;
        case CG_BLEND_FACTOR_INV_SRC_COLOR  : return GL_ONE_MINUS_SRC_COLOR;
        case CG_BLEND_FACTOR_DST_COLOR      : return GL_DST_COLOR;
        case CG_BLEND_FACTOR_INV_DST_COLOR  : return GL_ONE_MINUS_DST_COLOR;
        case CG_BLEND_FACTOR_SRC_ALPHA      : return GL_SRC_ALPHA;
        case CG_BLEND_FACTOR_INV_SRC_ALPHA  : return GL_ONE_MINUS_SRC_ALPHA;
        case CG_BLEND_FACTOR_DST_ALPHA      : return GL_DST_ALPHA;
        case CG_BLEND_FACTOR_INV_DST_ALPHA  : return GL_ONE_MINUS_DST_ALPHA;
        case CG_BLEND_FACTOR_CONST_COLOR    : return GL_CONSTANT_COLOR;
        case CG_BLEND_FACTOR_INV_CONST_COLOR: return GL_ONE_MINUS_CONSTANT_COLOR;
        case CG_BLEND_FACTOR_CONST_ALPHA    : return GL_CONSTANT_ALPHA;
        case CG_BLEND_FACTOR_INV_CONST_ALPHA: return GL_ONE_MINUS_CONSTANT_ALPHA;
        case CG_BLEND_FACTOR_SRC_ALPHA_SAT  : return GL_SRC_ALPHA_SATURATE;
        default: break;
    }
    return GL_INVALID_ENUM;
}

/// @summary Convert a CGFX comparison function value to the corresponding OpenGL enum.
/// @param mode The CGFX comparison function, one of cg_compare_function_e.
/// @return The corresponding OpenGL enum value, or GL_INVALID_ENUM.
internal_function GLenum
cgGlCompareFunction
(
    int func
)
{
    switch (func)
    {
        case CG_COMPARE_NEVER        : return GL_NEVER;
        case CG_COMPARE_LESS         : return GL_LESS;
        case CG_COMPARE_EQUAL        : return GL_EQUAL;
        case CG_COMPARE_LESS_EQUAL   : return GL_LEQUAL;
        case CG_COMPARE_GREATER      : return GL_GREATER;
        case CG_COMPARE_NOT_EQUAL    : return GL_NOTEQUAL;
        case CG_COMPARE_GREATER_EQUAL: return GL_GEQUAL;
        case CG_COMPARE_ALWAYS       : return GL_ALWAYS;
        default: break;
    }
    return GL_INVALID_ENUM;
}

/// @summary Convert a CGFX fill mode value to the corresponding OpenGL enum.
/// @param mode The CGFX fill mode, one of cg_fill_mode_e.
/// @return The corresponding OpenGL enum value, or GL_INVALID_ENUM.
internal_function GLenum
cgGlFillMode
(
    int mode
)
{
    switch (mode)
    {
        case CG_FILL_SOLID    : return GL_FILL;
        case CG_FILL_WIREFRAME: return GL_LINE;
        default: break;
    }
    return GL_INVALID_ENUM;
}

/// @summary Convert a CGFX cull mode value to the corresponding OpenGL enum.
/// @param mode The CGFX culling mode, one of cg_cull_mode_e.
/// @return The corresponding OpenGL enum value, or GL_INVALID_ENUM.
internal_function GLenum
cgGlCullMode
(
    int mode
)
{
    switch (mode)
    {
        case CG_CULL_NONE : return GL_NONE;
        case CG_CULL_FRONT: return GL_FRONT;
        case CG_CULL_BACK : return GL_BACK;
        default: break;
    }
    return GL_INVALID_ENUM;
}

/// @summary Convert a CGFX winding order value to the corresponding OpenGL enum.
/// @param winding The CGFX winding order value, one of cg_winding_e.
/// @return The corresponding OpenGL enum value, or GL_INVALID_ENUM.
internal_function GLenum
cgGlWindingOrder
(
    int winding
)
{
    switch (winding)
    {
        case CG_WINDING_CCW: return GL_CCW;
        case CG_WINDING_CW : return GL_CW;
        default: break;
    }
    return GL_INVALID_ENUM;
}

/// @summary Convert a CGFX primitive topology value to the corresponding OpenGL enum.
/// @param topology The CGFX primitive topology value, one of cg_primitive_topology_e.
/// @return The corresponding OpenGL enum value, or GL_INVALID_ENUM.
internal_function GLenum
cgGlPrimitiveTopology
(
    int topology
)
{
    switch (topology)
    {
        case CG_PRIMITIVE_POINT_LIST    : return GL_POINTS;
        case CG_PRIMITIVE_LINE_LIST     : return GL_LINES;
        case CG_PRIMITIVE_LINE_STRIP    : return GL_LINE_STRIP;
        case CG_PRIMITIVE_TRIANGLE_LIST : return GL_TRIANGLES;
        case CG_PRIMITIVE_TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
        default: break;
    }
    return GL_INVALID_ENUM;
}

/// @summary Convert a CGFX stencil operation value to the corresponding OpenGL enum.
/// @param op The CGFX stencil operation, one of cg_stencil_operation_e.
/// @return The corresponding OpenGL enum value, or GL_INVALID_ENUM.
internal_function GLenum
cgGlStencilOp
(
    int op
)
{
    switch (op)
    {
        case CG_STENCIL_OP_KEEP     : return GL_KEEP;
        case CG_STENCIL_OP_ZERO     : return GL_ZERO;
        case CG_STENCIL_OP_REPLACE  : return GL_REPLACE;
        case CG_STENCIL_OP_INC_CLAMP: return GL_INCR;
        case CG_STENCIL_OP_DEC_CLAMP: return GL_DECR;
        case CG_STENCIL_OP_INVERT   : return GL_INVERT;
        case CG_STENCIL_OP_INC_WRAP : return GL_INCR_WRAP;
        case CG_STENCIL_OP_DEC_WRAP : return GL_DECR_WRAP;
        default: break;
    }
    return GL_INVALID_ENUM;
}

/// @summary Convert a boolean value into either GL_TRUE or GL_FALSE.
/// @param value The value to convert.
/// @return One of GL_TRUE or GL_FALSE.
internal_function GLboolean
cgGlBoolean
(
    bool value
)
{
    return value ? GL_TRUE : GL_FALSE;
}

/// @summary Determines whether an identifier would be considered a GLSL built- in value; that is, whether the identifier starts with 'gl_'.
/// @param name A NULL-terminated ASCII string identifier.
/// @return true if @a name starts with 'gl_'.
internal_function bool
cgGlslBuiltIn
(
    char const *name
)
{
    char prefix[4] = {'g','l','_','\0'};
    return (strncmp(name, prefix, 3) == 0);
}

/// @summary Counts the number of active vertex attribues, texture samplers and uniform values defined in a shader program.
/// @param display The display managing the rendering context.
/// @param program The OpenGL program object to query.
/// @param buffer A temporary buffer used to hold attribute and uniform names.
/// @param buffer_size The maximum number of bytes that can be written to the temporary name buffer.
/// @param include_builtins Specify true to include GLSL builtin values in the returned vertex attribute count.
/// @param out_num_attribs On return, this address is updated with the number of active vertex attribute values.
/// @param out_num_samplers On return, this address is updated with the number of active texture sampler values.
/// @param out_num_uniforms On return, this address is updated with the number of active uniform values.
internal_function void
cgGlslReflectProgramCounts
(
    CG_DISPLAY *display,
    GLuint      program,
    char       *buffer,
    size_t      buffer_size,
    bool        include_builtins,
    size_t     &out_num_attribs,
    size_t     &out_num_samplers,
    size_t     &out_num_uniforms
)
{
    size_t  num_attribs  = 0;
    GLint   attrib_count = 0;
    GLsizei buf_size     = (GLsizei) buffer_size;

    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &attrib_count);
    for (GLint i = 0; i < attrib_count; ++i)
    {
        GLenum type = GL_FLOAT;
        GLuint idx  = (GLuint) i;
        GLint  len  = 0;
        GLint  sz   = 0;
        glGetActiveAttrib(program, idx, buf_size, &len, &sz, &type, buffer);
        if (cgGlslBuiltIn(buffer) && !include_builtins)
            continue;
        num_attribs++;
    }

    size_t num_samplers  = 0;
    size_t num_uniforms  = 0;
    GLint  uniform_count = 0;

    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniform_count);
    for (GLint i = 0; i < uniform_count; ++i)
    {
        GLenum type = GL_FLOAT;
        GLuint idx  = (GLuint) i;
        GLint  len  = 0;
        GLint  sz   = 0;
        glGetActiveUniform(program, idx, buf_size, &len, &sz, &type, buffer);
        if (cgGlslBuiltIn (buffer) && !include_builtins)
            continue;

        switch (type)
        {
            case GL_SAMPLER_1D:
            case GL_INT_SAMPLER_1D:
            case GL_UNSIGNED_INT_SAMPLER_1D:
            case GL_SAMPLER_1D_SHADOW:
            case GL_SAMPLER_2D:
            case GL_INT_SAMPLER_2D:
            case GL_UNSIGNED_INT_SAMPLER_2D:
            case GL_SAMPLER_2D_SHADOW:
            case GL_SAMPLER_3D:
            case GL_INT_SAMPLER_3D:
            case GL_UNSIGNED_INT_SAMPLER_3D:
            case GL_SAMPLER_CUBE:
            case GL_INT_SAMPLER_CUBE:
            case GL_UNSIGNED_INT_SAMPLER_CUBE:
            case GL_SAMPLER_CUBE_SHADOW:
            case GL_SAMPLER_1D_ARRAY:
            case GL_SAMPLER_1D_ARRAY_SHADOW:
            case GL_INT_SAMPLER_1D_ARRAY:
            case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
            case GL_SAMPLER_2D_ARRAY:
            case GL_SAMPLER_2D_ARRAY_SHADOW:
            case GL_INT_SAMPLER_2D_ARRAY:
            case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
            case GL_SAMPLER_BUFFER:
            case GL_INT_SAMPLER_BUFFER:
            case GL_UNSIGNED_INT_SAMPLER_BUFFER:
            case GL_SAMPLER_2D_RECT:
            case GL_SAMPLER_2D_RECT_SHADOW:
            case GL_INT_SAMPLER_2D_RECT:
            case GL_UNSIGNED_INT_SAMPLER_2D_RECT:
            case GL_SAMPLER_2D_MULTISAMPLE:
            case GL_INT_SAMPLER_2D_MULTISAMPLE:
            case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
            case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
            case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
            case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
                num_samplers++;
                break;

            default:
                num_uniforms++;
                break;
        }
    }

    // set the output values for the caller.
    out_num_attribs  = num_attribs;
    out_num_samplers = num_samplers;
    out_num_uniforms = num_uniforms;
}

/// @summary Retrieve information about the active vertex attribues, texture samplers and uniform values defined in a shader program.
/// @param display The display managing the rendering context.
/// @param program The OpenGL program object to query.
/// @param buffer A temporary buffer used to hold attribute and uniform names.
/// @param buffer_size The maximum number of bytes that can be written to the temporary name buffer.
/// @param include_builtins Specify true to include GLSL builtin values in the returned vertex attribute count.
/// @param glsl The GLSL program descriptor to populate with data.
internal_function void
cgGlslReflectProgramMetadata
(
    CG_DISPLAY      *display,
    GLuint           program,
    char            *buffer,
    size_t           buffer_size,
    bool             include_builtins,
    CG_GLSL_PROGRAM *glsl
)
{
    uint32_t          *attrib_names  = glsl->AttributeNames;
    uint32_t          *sampler_names = glsl->SamplerNames;
    uint32_t          *uniform_names = glsl->UniformNames;
    CG_GLSL_ATTRIBUTE *attrib_info   = glsl->Attributes;
    CG_GLSL_SAMPLER   *sampler_info  = glsl->Samplers;
    CG_GLSL_UNIFORM   *uniform_info  = glsl->Uniforms;
    size_t             num_attribs   = 0;
    GLint              attrib_count  = 0;
    GLsizei            buf_size      =(GLsizei) buffer_size;

    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &attrib_count);
    for (GLint i = 0; i < attrib_count; ++i)
    {
        GLenum type = GL_FLOAT;
        GLuint idx  = (GLuint) i;
        GLint  len  = 0;
        GLint  loc  = 0;
        GLint  sz   = 0;
        glGetActiveAttrib(program, idx, buf_size, &len, &sz, &type, buffer);
        if (cgGlslBuiltIn(buffer) && !include_builtins)
            continue;

        CG_GLSL_ATTRIBUTE va;
        loc             = glGetAttribLocation(program, buffer);
        va.DataType     =(GLenum)   type;
        va.Location     =(GLint)    loc;
        va.DataSize     =(size_t)   cgGlDataSize(type) * sz;
        va.DataOffset   =(size_t)   0; // for application use only
        va.Dimension    =(size_t)   sz;
        attrib_names[num_attribs] = cgHashName(buffer);
        attrib_info [num_attribs] = va;
        num_attribs++;
    }

    size_t num_samplers  = 0;
    size_t num_uniforms  = 0;
    GLint  uniform_count = 0;
    GLint  texture_unit  = 0;

    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniform_count);
    for (GLint i = 0; i < uniform_count; ++i)
    {
        GLenum type = GL_FLOAT;
        GLuint idx  = (GLuint) i;
        GLint  len  = 0;
        GLint  loc  = 0;
        GLint  sz   = 0;
        glGetActiveUniform(program, idx, buf_size, &len, &sz, &type, buffer);
        if (cgGlslBuiltIn (buffer) && !include_builtins)
            continue;

        switch (type)
        {
            case GL_SAMPLER_1D:
            case GL_INT_SAMPLER_1D:
            case GL_UNSIGNED_INT_SAMPLER_1D:
            case GL_SAMPLER_1D_SHADOW:
            case GL_SAMPLER_2D:
            case GL_INT_SAMPLER_2D:
            case GL_UNSIGNED_INT_SAMPLER_2D:
            case GL_SAMPLER_2D_SHADOW:
            case GL_SAMPLER_3D:
            case GL_INT_SAMPLER_3D:
            case GL_UNSIGNED_INT_SAMPLER_3D:
            case GL_SAMPLER_CUBE:
            case GL_INT_SAMPLER_CUBE:
            case GL_UNSIGNED_INT_SAMPLER_CUBE:
            case GL_SAMPLER_CUBE_SHADOW:
            case GL_SAMPLER_1D_ARRAY:
            case GL_SAMPLER_1D_ARRAY_SHADOW:
            case GL_INT_SAMPLER_1D_ARRAY:
            case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
            case GL_SAMPLER_2D_ARRAY:
            case GL_SAMPLER_2D_ARRAY_SHADOW:
            case GL_INT_SAMPLER_2D_ARRAY:
            case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
            case GL_SAMPLER_BUFFER:
            case GL_INT_SAMPLER_BUFFER:
            case GL_UNSIGNED_INT_SAMPLER_BUFFER:
            case GL_SAMPLER_2D_RECT:
            case GL_SAMPLER_2D_RECT_SHADOW:
            case GL_INT_SAMPLER_2D_RECT:
            case GL_UNSIGNED_INT_SAMPLER_2D_RECT:
            case GL_SAMPLER_2D_MULTISAMPLE:
            case GL_INT_SAMPLER_2D_MULTISAMPLE:
            case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
            case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
            case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
            case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
                {
                    CG_GLSL_SAMPLER  ts;
                    loc            = glGetUniformLocation(program, buffer);
                    ts.SamplerType =(GLenum)      type;
                    ts.BindTarget  =(GLenum)      cgGlTextureTarget(type);
                    ts.Location    =(GLint)       loc;
                    ts.ImageUnit   =(GLint)       texture_unit++;
                    sampler_names[num_samplers] = cgHashName(buffer);
                    sampler_info [num_samplers] = ts;
                    num_samplers++;
                }
                break;

            default:
                {
                    CG_GLSL_UNIFORM  uv;
                    loc            = glGetUniformLocation(program, buffer);
                    uv.DataType    =(GLenum)      type;
                    uv.Location    =(GLint)       loc;
                    uv.DataSize    =(size_t)      cgGlDataSize(type) * sz;
                    uv.DataOffset  =(size_t)      0; // for application use only
                    uv.Dimension   =(size_t)      sz;
                    uniform_names[num_uniforms] = cgHashName(buffer);
                    uniform_info [num_uniforms] = uv;
                    num_uniforms++;
                }
                break;
        }
    }
}

/// @summary Fills a memory buffer with a checkerboard pattern. This is useful for indicating uninitialized textures and for testing. The image internal format is expected to be GL_RGBA, data type GL_UNSIGNED_INT_8_8_8_8_REV, and the data is written using the native system byte ordering (GL_BGRA).
/// @param width The image width, in pixels.
/// @param height The image height, in pixels.
/// @param alpha The value to write to the alpha channel, in [0, 1].
/// @param buffer The buffer to which image data will be written.
internal_function void
cgCheckerFillRGBA
(
    size_t width,
    size_t height,
    float  alpha,
    void  *buffer
)
{
    uint8_t  a = (uint8_t)((alpha - 0.5f) * 255.0f);
    uint8_t *p = (uint8_t*) buffer;
    for (size_t i = 0;  i < height; ++i)
    {
        for (size_t j = 0; j < width; ++j)
        {
            uint8_t v = ((((i & 0x8) == 0)) ^ ((j & 0x8) == 0)) ? 1 : 0;
            *p++  = v * 0xFF;
            *p++  = v ? 0x00 : 0xFF;
            *p++  = v * 0xFF;
            *p++  = a;
        }
    }
}

/// @summary Determine whether a device should be included in the device list for an execution group.
/// @param root_device A reference to the root device, which defines the share group.
/// @param check_device A reference to the device to check for inclusion in the device list.
/// @param check_handle The handle used to refer to @a check_device.
/// @param create_flags A combination of cg_execution_group_flags_e used to define the devices to include in the implicit device list.
/// @param device_list A list of handles of devices explicitly specified by the user for inclusion.
/// @param device_count The number of handles in the explicit device list.
/// @param explicit_count A counter to increment if @a check_handle appears in @a device_list.
/// @return true if @a check_device should be included in the device list.
internal_function bool
cgShouldIncludeDeviceInGroup
(
    CG_DEVICE   *root_device,
    CG_DEVICE   *check_device,
    cg_handle_t  check_handle,
    uint32_t     create_flags,
    cg_handle_t *device_list,
    size_t       device_count,
    size_t      &explicit_count
)
{
    if (check_device->ExecutionGroup != CG_INVALID_HANDLE)
        return false;    // devices must not already be assigned to an execution group
    if (check_device == root_device)
        return true;     // always include the root device
    if (check_device->PlatformId != root_device->PlatformId)
        return false;    // devices must be in the same share group
    for (size_t i = 0; i  < device_count; ++i)
    {   if (check_handle == device_list[i])
        {   explicit_count++;
            return true; // the device is in the explicit device list
        }
    }
    switch (root_device->Type)
    {
    case CL_DEVICE_TYPE_CPU:
        {
            if (check_device->Type == CL_DEVICE_TYPE_CPU && (create_flags & CG_EXECUTION_GROUP_CPUS))
                return true; // CPU devices may always be used together.
            if (check_device->Type == CL_DEVICE_TYPE_GPU && (create_flags & CG_EXECUTION_GROUP_GPUS))
                return check_device->Capabilities.UnifiedMemory == CL_TRUE; // GPUs can be paired if unified memory.
            if (check_device->Type == CL_DEVICE_TYPE_ACCELERATOR && (create_flags & CG_EXECUTION_GROUP_ACCELERATORS))
                return check_device->Capabilities.UnifiedMemory == CL_TRUE; // accelerators can be paired if unified memory.
        }
        return false;

    case CL_DEVICE_TYPE_GPU:
        {
            if (check_device->Type == CL_DEVICE_TYPE_CPU && (create_flags & CG_EXECUTION_GROUP_CPUS))
                return (root_device->Capabilities.UnifiedMemory == CL_TRUE); // CPUs can be paired if unified memory.
            if (check_device->Type == CL_DEVICE_TYPE_GPU && (create_flags & CG_EXECUTION_GROUP_GPUS))
                return (create_flags & CG_EXECUTION_GROUP_DISPLAY_OUTPUT) == 0; // GPUs can be paired if not used for display output.
            if (check_device->Type == CL_DEVICE_TYPE_ACCELERATOR && (create_flags & CG_EXECUTION_GROUP_ACCELERATORS))
                return (root_device->Capabilities.UnifiedMemory == CL_TRUE && check_device->Capabilities.UnifiedMemory == CL_TRUE); // pairing allowed if both devices support unified memory.
        }
        return false;

    case CL_DEVICE_TYPE_ACCELERATOR:
        {
            if (check_device->Type == CL_DEVICE_TYPE_CPU && (create_flags & CG_EXECUTION_GROUP_CPUS))
                return (root_device->Capabilities.UnifiedMemory == CL_TRUE); // CPUs can be paired if unified memory.
            if (check_device->Type == CL_DEVICE_TYPE_GPU && (create_flags & CG_EXECUTION_GROUP_GPUS))
                return (root_device->Capabilities.UnifiedMemory == CL_TRUE && check_device->Capabilities.UnifiedMemory == CL_TRUE); // pairing allowed if both devices support unified memory.
            if (check_device->Type == CL_DEVICE_TYPE_ACCELERATOR && (create_flags & CG_EXECUTION_GROUP_ACCELERATORS))
                return true; // accelerator devices may always be used together.
        }
        return false;

    default:
        return false;
    }
}

/// @summary Create a complete list of device handles for an execution group and perform partitioning of any CPU devices.
/// @param ctx The CGFX context managing the device list.
/// @param root_device The handle of any device in the share group.
/// @param create_flags A combination of cg_execution_group_flags_e specifying what to include in the device list.
/// @param device_list A list of handles of devices explicitly specified by the user for inclusion.
/// @param device_count The number of handles in the explicit device list.
/// @param num_devices On return, stores the number of devices in the output device list.
/// @param result On return, stores CG_SUCCESS or an error code.
/// @return An array of num_devices handles to the devices used to create the execution group (including the root device), or NULL.
internal_function cg_handle_t*
cgCreateExecutionGroupDeviceList
(
    CG_CONTEXT  *ctx,
    cg_handle_t  root_device,
    uint32_t     create_flags,
    cg_handle_t *device_list,
    size_t       device_count,
    size_t      &num_devices,
    int         &result
)
{
    CG_DEVICE     *root_dev     = cgObjectTableGet(&ctx->DeviceTable, root_device);
    cg_handle_t   *devices      = NULL;
    size_t         num_cpu      = 0; // number of CPU devices
    size_t         num_gpu      = 0; // number of GPU devices
    size_t         num_acl      = 0; // number of accelerator devices
    size_t         num_total    = 0;
    size_t         num_explicit = 0;

    // initialize output parameters:
    num_devices = 0;
    result = CG_SUCCESS;

    // count the number of devices to be included in the list.
    for (size_t i = 0, n = ctx->DeviceTable.ObjectCount; i < n; ++i)
    {
        CG_DEVICE *check =&ctx->DeviceTable.Objects[i];
        cg_handle_t hand = cgMakeHandle(&ctx->DeviceTable, i);
        if (cgShouldIncludeDeviceInGroup(root_dev, check, hand, create_flags, device_list, device_count, num_explicit))
        {   // increment per-type counts:
            switch (check->Type)
            {
            case CL_DEVICE_TYPE_CPU        : num_cpu++; break;
            case CL_DEVICE_TYPE_GPU        : num_gpu++; break;
            case CL_DEVICE_TYPE_ACCELERATOR: num_acl++; break;
            default: break;
            }
        }
    }
    if (num_explicit != device_count)
    {   // the explicit device list contains one or more invalid handles.
        result = CG_INVALID_VALUE;
        goto error_return;
    }
    if (num_gpu > 1 && (create_flags & CG_EXECUTION_GROUP_DISPLAY_OUTPUT))
    {   // only one GPU device allowed in groups used for display output.
        result = CG_INVALID_VALUE;
        goto error_return;
    }

    // calculate the total number of items in the device list:
    num_explicit = 0;
    num_total = num_cpu + num_gpu + num_acl;
    assert(num_total >= 1);

    // allocate storage for the device handles:
    if ((devices = (cg_handle_t*) cgAllocateHostMemory(&ctx->HostAllocator, num_total * sizeof(cg_handle_t), 0, CG_ALLOCATION_TYPE_TEMP)) == NULL)
    {   // unable to allocate the necessary memory.
        result = CG_OUT_OF_MEMORY;
        goto error_return;
    }

    // run through the device table again to populate the device list.
    for (size_t i = 0, n = ctx->DeviceTable.ObjectCount; i < n; ++i)
    {
        CG_DEVICE *check =&ctx->DeviceTable.Objects[i];
        cg_handle_t hand = cgMakeHandle(&ctx->DeviceTable, i);
        if (cgShouldIncludeDeviceInGroup(root_dev, check, hand, create_flags, device_list, device_count, num_explicit))
        {   // save the handle for the caller.
            devices[num_devices++] = hand;
        }
    }
    return devices;

error_return:
    num_devices = 0;
    return NULL;
}

/// @summary Frees the memory allocated for a device list created with cgCreateExecutionGroupDeviceList.
/// @param ctx The CGFX context that allocated the device list.
/// @param devices The device list returned by cgCreateExecutionGroupDeviceList.
/// @param num_devices The number of devices returned by cgCreateExecutionGroupDeviceList.
internal_function inline void
cgFreeExecutionGroupDeviceList
(
    CG_CONTEXT  *ctx,
    cg_handle_t *devices,
    size_t       num_devices
)
{
    cgFreeHostMemory(&ctx->HostAllocator, devices, num_devices * sizeof(cg_handle_t), 0, CG_ALLOCATION_TYPE_TEMP);
}

/// @summary Calculate the number of bytes allocated for an image on its associated heap.
/// @param image The image to query. All image attributes (dimensions, format, etc.) must be set.
/// @return The number of bytes allocated for image data storage on the image source heap.
internal_function size_t
cgCalculateImageAllocatedSize
(
    CG_IMAGE *image
)
{
    size_t allocated_size = 0;
    size_t nslice_or_item = image->ArrayCount > 1 ? image->ArrayCount : image->SliceCount;
    for (size_t i = 0, n  = nslice_or_item; i < n; ++i)
    {
        for (size_t j = 0; j < image->LevelCount; ++j)
        {
            size_t  w = cgGlLevelDimension(image->PaddedWidth , j);
            size_t  h = cgGlLevelDimension(image->PaddedHeight, j);
            size_t ss = cgGlBytesPerSlice (image->InternalFormat, image->DataType, w, h, 4);
            allocated_size += ss;
        }
    }
    return align_up(allocated_size, image->SourceHeap->DeviceAlignment);
}

/// @summary Provides a default teardown callback for a pipeline that doesn't need to perform any cleanup.
/// @param ctx A CGFX context returned by cgEnumerateDevices.
/// @param pipeline A pointer to the CG_PIPELINE object.
/// @param opaque The opaque state data supplied when the pipeline was created.
internal_function void
cgPipelineTeardownNoOp
(
    uintptr_t  ctx, 
    uintptr_t  pipeline, 
    void      *opaque
)
{
    UNREFERENCED_PARAMETER(ctx);
    UNREFERENCED_PARAMETER(pipeline);
    UNREFERENCED_PARAMETER(opaque);
}

/// @summary Implements the DEVICE_FENCE command, inserting a fence in an in-order graphics queue or an out-of-order compute or transfer queue.
/// The fence ensures that some or all of the commands inserted into the queue prior to the fence have completed before executing commands enqueued after the fence.
/// @param ctx The CGFX context returned by cgEnumerateDevices.
/// @param queue The command queue into which the fence command is being inserted.
/// @param cmdbuf The command buffer defining the fence command.
/// @param cmd The fence command and any associated data.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_INVALID_STATE, CG_BAD_CLCONTEXT, CG_UNSUPPORTED, CG_OUT_OF_MEMORY, CG_OUT_OF_OBJECTS or CG_ERROR.
internal_function int
cgExecuteDeviceFence
(
    CG_CONTEXT    *ctx, 
    CG_QUEUE      *queue, 
    CG_CMD_BUFFER *cmdbuf, 
    cg_command_t  *cmd
)
{   UNREFERENCED_PARAMETER(cmdbuf);
    // proceed with command execution. a fence can either be a graphics 
    // fence or a compute/transfer fence. a graphics fence could be linked
    // to an OpenCL event from a clReleaseGLObjects command. since graphics
    // queues are always in-order, a fence blocks execution until all prior 
    // commands have completed. a compute or transfer queue fence, on the 
    // other hand, supports out-of-order execution and can support waiting 
    // for a specific set of events to become signaled.
    cg_device_fence_cmd_data_t *ddp = (cg_device_fence_cmd_data_t*) cmd->Data;
    CG_FENCE *fence = cgObjectTableGet(&ctx->FenceTable, ddp->FenceObject);
    if (fence == NULL || (queue->QueueType & fence->QueueType) == 0)
    {   // invalid fence object, or queue type doesn't match fence.
        return CG_INVALID_VALUE;
    }

    if (fence->QueueType == CG_QUEUE_TYPE_GRAPHICS)
    {   // create and enqueue an OpenGL fence sync object.
        CG_DISPLAY *display = fence->AttachedDisplay;
        GLsync      sync    = NULL;

        // if there's an existing sync object associated with the fence, delete it.
        if (fence->GraphicsFence != NULL)
        {
            glDeleteSync(fence->GraphicsFence);
            fence->GraphicsFence  = NULL;
        }

        // generate a new sync object and insert the fence command in the OpenGL context's command queue.
        if (fence->LinkedEvent != CG_INVALID_HANDLE)
        {   // the fence is linked to an OpenCL event, so use GL_ARB_cl_event.
            CG_EVENT *event = cgObjectTableGet(&ctx->EventTable, fence->LinkedEvent);
            if (event == NULL || event->ComputeEvent == NULL)
            {   // the compute or transfer queue command associated with the event hasn't
                // yet been submitted to a command queue, so the operation is invalid.
                return CG_INVALID_STATE;
            }
            if (GLEW_ARB_cl_event)
            {   // create the OpenGL sync object and insert it into the command stream.
                if ((sync = glCreateSyncFromCLeventARB(queue->ComputeContext, event->ComputeEvent, 0)) == NULL)
                {
                    GLenum  glerr = glGetError();
                    switch (glerr)
                    {
                    case GL_INVALID_VALUE    : return CG_BAD_CLCONTEXT;
                    case GL_INVALID_OPERATION: return CG_INVALID_STATE;
                    default: break;
                    }
                    return CG_ERROR;
                }
            }
            else
            {   // GL_ARB_cl_event is not supported by the runtime. fail.
                return CG_UNSUPPORTED;
            }
        }
        else
        {   // the fence is not linked to an OpenCL event.
            if ((sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0)) == NULL)
            {
                GLenum  glerr = glGetError();
                switch (glerr)
                {
                case GL_INVALID_ENUM : return CG_INVALID_VALUE;
                case GL_INVALID_VALUE: return CG_INVALID_VALUE;
                default              : break;
                }
                return CG_ERROR;
            }
        }
        // insert a wait command to act as a barrier in the GPU command queue.
        // issue a glFlush prior to the barrier to ensure that the sync object
        // has been flushed to the GPU command queue by the driver.
        glFlush();  glWaitSync(sync, 0, GL_TIMEOUT_IGNORED);
        fence->GraphicsFence = sync;
        return cgSetupCompleteEvent(ctx, queue, ddp->CompleteEvent, NULL, sync, CG_SUCCESS);
    }
    else
    {   // enqueue an OpenCL barrier. there could be a wait list.
        int       result     = CG_SUCCESS;
        cl_int    clres      = CL_SUCCESS;
        cl_uint   wait_count = 0;
        cl_event  cl_fence   = NULL;
        cl_event *wait_list  = NULL;
        bool     *wait_free  = NULL;
        cl_event  wait_stack[16];
        bool      free_stack[16];

        // if there's an existing sync object associated with this fence, delete it.
        if (fence->ComputeFence != NULL)
        {
            clReleaseEvent(fence->ComputeFence);
            fence->ComputeFence  = NULL;
        }

        // generate the event wait list. it's possible that there is no wait list, 
        // in which case the fence waits for all prior commands in the queue to be
        // completed, or it's possible that a specific list of events is specified 
        // to satisfy the barrier condition.
        if (ddp->WaitListCount == 0)
        {   // simple case, this is a fence for all prior commands in the queue.
            wait_count = 0; wait_list = NULL;
        }
        else
        {   // there is a wait list involved. convert the wait handles into 
            // an array of cl_event instances to wait on. in most cases we 
            // should just be able to use stack memory.
            if (ddp->WaitListCount <= 16)
            {   // just use the stack arrays; no dynamic allocation.
                wait_count = 0;
                wait_list  = wait_stack;
                wait_free  = free_stack;
            }
            else
            {   // need to wait on many individual events. allocate temporary memory.
                wait_list  = (cl_event*) cgAllocateHostMemory(&ctx->HostAllocator, ddp->WaitListCount * sizeof(cl_event), 1, CG_ALLOCATION_TYPE_TEMP);
                wait_free  = (bool    *) cgAllocateHostMemory(&ctx->HostAllocator, ddp->WaitListCount * sizeof(bool)    , 1, CG_ALLOCATION_TYPE_TEMP);
                if (wait_list == NULL || wait_free == NULL)
                {   // unable to allocate the required memory.
                    result = CG_OUT_OF_MEMORY;
                    goto opencl_cleanup;
                }
            }
            for (size_t i = 0, n = ddp->WaitListCount; i < n; ++i)
            {
                if ((result = cgGetWaitEvent(ctx, queue, ddp->WaitList[i], wait_list, wait_count, cl_uint(n), wait_free[wait_count])) != CG_SUCCESS)
                {   // unable to retrieve the OpenCL event object to wait on.
                    goto opencl_cleanup;
                }
            }
        }

        // insert the barrier command in the OpenCL command queue.
        if ((clres = clEnqueueBarrierWithWaitList(queue->CommandQueue, wait_count, wait_list, &cl_fence)) != CL_SUCCESS)
        {   
            switch (clres)
            {
            case CL_INVALID_COMMAND_QUEUE  : result = CG_BAD_CLCONTEXT; break;
            case CL_INVALID_EVENT_WAIT_LIST: result = CG_INVALID_VALUE; break;
            case CL_OUT_OF_RESOURCES       : result = CG_OUT_OF_MEMORY; break;
            case CL_OUT_OF_HOST_MEMORY     : result = CG_OUT_OF_MEMORY; break;
            default                        : result = CG_ERROR;         break;
            }
            goto opencl_cleanup;
        }
        fence->ComputeFence = cl_fence;
        result = CG_SUCCESS;
        /* fall through to opencl_cleanup */

    opencl_cleanup:
        for (size_t wait_i = 0, wait_n = wait_count; wait_i < wait_n; ++wait_i)
        {   // free temporary event objects.
            if (wait_free[wait_i])
                clReleaseEvent(wait_list[wait_i]);
        }
        if (wait_list != &wait_stack[0])
        {   // free the temporary wait list memory.
            if (wait_free != NULL) cgFreeHostMemory(&ctx->HostAllocator, wait_free, ddp->WaitListCount * sizeof(bool)    , 1, CG_ALLOCATION_TYPE_TEMP);
            if (wait_list != NULL) cgFreeHostMemory(&ctx->HostAllocator, wait_list, ddp->WaitListCount * sizeof(cl_event), 1, CG_ALLOCATION_TYPE_TEMP);
        }
        return cgSetupCompleteEvent(ctx, queue, ddp->CompleteEvent, cl_fence, NULL, result);
    }
}

/// @summary Implements the PIPELINE_DISPATCH command for a compute command queue.
/// @param ctx The CGFX context returned by cgEnumerateDevices.
/// @param queue The command queue into which the command is being inserted.
/// @param cmdbuf The command buffer defining the command.
/// @param cmd The command and any associated data.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_INVALID_STATE, CG_BAD_CLCONTEXT, CG_UNSUPPORTED, CG_OUT_OF_MEMORY, CG_OUT_OF_OBJECTS or CG_ERROR.
internal_function int
cgExecuteComputePipelineDispatch
(
    CG_CONTEXT    *ctx, 
    CG_QUEUE      *queue, 
    CG_CMD_BUFFER *cmdbuf, 
    cg_command_t  *cmd
)
{
    CG_PIPELINE         *pipeline =  NULL;
    cg_pipeline_cmd_base_t *cdata = (cg_pipeline_cmd_base_t*) cmd->Data;
    cgPipelineExecute_fn    cfunc =  cgGetComputePipelineCallback(cdata->PipelineId);
    if (cfunc == NULL)
    {   // this pipeline hasn't been registered yet.
        return CG_INVALID_VALUE;
    }
    if ((pipeline = cgObjectTableGet(&ctx->PipelineTable, cdata->Pipeline)) == NULL)
    {   // the pipeline ID is valid, but the pipeline state handle is not valid.
        return CG_INVALID_VALUE;
    }
    return cfunc(ctx, queue, cmdbuf, pipeline, cmd);
}

/// @summary Implements the PIPELINE_DISPATCH command for a graphics command queue.
/// @param ctx The CGFX context returned by cgEnumerateDevices.
/// @param queue The command queue into which the command is being inserted.
/// @param cmdbuf The command buffer defining the command.
/// @param cmd The command and any associated data.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_INVALID_STATE, CG_BAD_CLCONTEXT, CG_UNSUPPORTED, CG_OUT_OF_MEMORY, CG_OUT_OF_OBJECTS or CG_ERROR.
internal_function int
cgExecuteGraphicsPipelineDispatch
(
    CG_CONTEXT    *ctx, 
    CG_QUEUE      *queue, 
    CG_CMD_BUFFER *cmdbuf, 
    cg_command_t  *cmd
)
{
    CG_PIPELINE         *pipeline =  NULL;
    cg_pipeline_cmd_base_t *cdata = (cg_pipeline_cmd_base_t*) cmd->Data;
    cgPipelineExecute_fn    cfunc =  cgGetGraphicsPipelineCallback(cdata->PipelineId);
    if (cfunc == NULL)
    {   // this pipeline hasn't been registered yet.
        return CG_INVALID_VALUE;
    }
    if ((pipeline = cgObjectTableGet(&ctx->PipelineTable, cdata->Pipeline)) == NULL)
    {   // the pipeline ID is valid, but the pipeline state handle is not valid.
        return CG_INVALID_VALUE;
    }
    return cfunc(ctx, queue, cmdbuf, pipeline, cmd);
}

/// @summary Implements the COPY_BUFFER command, which copies data from one buffer to another.
/// @param ctx The CGFX context returned by cgEnumerateDevices.
/// @param queue The command queue into which the command is being inserted.
/// @param cmdbuf The command buffer defining the command.
/// @param cmd The command and any associated data.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_INVALID_STATE, CG_BAD_CLCONTEXT, CG_UNSUPPORTED, CG_OUT_OF_MEMORY, CG_OUT_OF_OBJECTS or CG_ERROR.
internal_function int
cgExecuteCopyBufferRegion
(
    CG_CONTEXT    *ctx, 
    CG_QUEUE      *queue, 
    CG_CMD_BUFFER *cmdbuf, 
    cg_command_t  *cmd
)
{   UNREFERENCED_PARAMETER(cmdbuf);
    cg_copy_buffer_cmd_t *ddp  = (cg_copy_buffer_cmd_t*) cmd->Data;
    CG_BUFFER         *srcbuf  =  cgObjectTableGet(&ctx->BufferTable, ddp->SourceBuffer);
    CG_BUFFER         *dstbuf  =  cgObjectTableGet(&ctx->BufferTable, ddp->TargetBuffer);
    int                result  =  CG_SUCCESS;
    size_t            nmemrefs =  0;
    cl_uint           nwaitevt =  0;
    cl_event          acquire  =  NULL;
    cl_mem            memrefs[2];
    cgMemRefListAddBuffer(srcbuf, memrefs, nmemrefs, 2, false);
    cgMemRefListAddBuffer(dstbuf, memrefs, nmemrefs, 2, false);
    if ((result = cgAcquireMemoryObjects(ctx, queue, memrefs, nmemrefs, ddp->WaitEvent, &acquire, nwaitevt, 1)) == CL_SUCCESS)
    {
        cl_event  cl_done = NULL;
        cl_int    cl_res  = clEnqueueCopyBuffer(
            queue->CommandQueue, 
            memrefs[0], /* src */
            memrefs[1], /* dst */
            ddp->SourceOffset, 
            ddp->TargetOffset, 
            ddp->CopyAmount, 
            nwaitevt, 
            CG_OPENCL_WAIT_LIST(nwaitevt, &acquire),
            &cl_done);
        if  (cl_res != CL_SUCCESS)
        {
            switch (cl_res)
            {
            case CL_INVALID_COMMAND_QUEUE        : result = CG_BAD_CLCONTEXT; break;
            case CL_INVALID_CONTEXT              : result = CG_BAD_CLCONTEXT; break;
            case CL_INVALID_MEM_OBJECT           : result = CG_BAD_CLCONTEXT; break;
            case CL_INVALID_VALUE                : result = CG_INVALID_VALUE; break;
            case CL_INVALID_EVENT_WAIT_LIST      : result = CG_INVALID_VALUE; break;
            case CL_MISALIGNED_SUB_BUFFER_OFFSET : result = CG_INVALID_VALUE; break;
            case CL_MEM_COPY_OVERLAP             : result = CG_INVALID_VALUE; break;
            case CL_MEM_OBJECT_ALLOCATION_FAILURE: result = CG_OUT_OF_MEMORY; break;
            case CL_OUT_OF_RESOURCES             : result = CG_OUT_OF_MEMORY; break;
            case CL_OUT_OF_HOST_MEMORY           : result = CG_OUT_OF_MEMORY; break;
            default                              : result = CG_ERROR;         break;
            }
            cgReleaseMemoryObjects(ctx, queue, memrefs, nmemrefs, NULL, 0, CG_INVALID_HANDLE);
            return result;
        }
        return cgReleaseMemoryObjects(ctx, queue, memrefs, nmemrefs, &cl_done, 1, ddp->CompleteEvent);
    }
    return result;
}

/// @summary Implements the COPY_IMAGE command, which copies data from one image to another.
/// @param ctx The CGFX context returned by cgEnumerateDevices.
/// @param queue The command queue into which the command is being inserted.
/// @param cmdbuf The command buffer defining the command.
/// @param cmd The command and any associated data.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_INVALID_STATE, CG_BAD_CLCONTEXT, CG_UNSUPPORTED, CG_OUT_OF_MEMORY, CG_OUT_OF_OBJECTS or CG_ERROR.
internal_function int
cgExecuteCopyImageRegion
(
    CG_CONTEXT    *ctx, 
    CG_QUEUE      *queue, 
    CG_CMD_BUFFER *cmdbuf, 
    cg_command_t  *cmd
)
{   UNREFERENCED_PARAMETER(cmdbuf);
    cg_copy_image_cmd_t  *ddp  = (cg_copy_image_cmd_t*) cmd->Data;
    CG_IMAGE          *srcimg  =  cgObjectTableGet(&ctx->ImageTable, ddp->SourceImage);
    CG_IMAGE          *dstimg  =  cgObjectTableGet(&ctx->ImageTable, ddp->TargetImage);
    int                result  =  CG_SUCCESS;
    size_t            nmemrefs =  0;
    cl_uint           nwaitevt =  0;
    cl_event          acquire  =  NULL;
    cl_mem            memrefs[2];
    cgMemRefListAddImage(srcimg, memrefs, nmemrefs, 2, false);
    cgMemRefListAddImage(dstimg, memrefs, nmemrefs, 2, false);
    if ((result = cgAcquireMemoryObjects(ctx, queue, memrefs, nmemrefs, ddp->WaitEvent, &acquire, nwaitevt, 1)) == CL_SUCCESS)
    {
        cl_event  cl_done = NULL;
        cl_int    cl_res  = clEnqueueCopyImage(
            queue->CommandQueue, 
            memrefs[0], /* src */
            memrefs[1], /* dst */
            ddp->SourceOrigin, 
            ddp->TargetOrigin, 
            ddp->Dimensions, 
            nwaitevt, 
            CG_OPENCL_WAIT_LIST(nwaitevt, &acquire),
            &cl_done);
        if  (cl_res != CL_SUCCESS)
        {
            switch (cl_res)
            {
            case CL_INVALID_COMMAND_QUEUE        : result = CG_BAD_CLCONTEXT; break;
            case CL_INVALID_CONTEXT              : result = CG_BAD_CLCONTEXT; break;
            case CL_INVALID_MEM_OBJECT           : result = CG_BAD_CLCONTEXT; break;
            case CL_IMAGE_FORMAT_MISMATCH        : result = CG_INVALID_VALUE; break;
            case CL_INVALID_IMAGE_SIZE           : result = CG_INVALID_VALUE; break;
            case CL_INVALID_VALUE                : result = CG_INVALID_VALUE; break;
            case CL_INVALID_EVENT_WAIT_LIST      : result = CG_INVALID_VALUE; break;
            case CL_MEM_COPY_OVERLAP             : result = CG_INVALID_VALUE; break;
            case CL_MEM_OBJECT_ALLOCATION_FAILURE: result = CG_OUT_OF_MEMORY; break;
            case CL_OUT_OF_RESOURCES             : result = CG_OUT_OF_MEMORY; break;
            case CL_OUT_OF_HOST_MEMORY           : result = CG_OUT_OF_MEMORY; break;
            default                              : result = CG_ERROR;         break;
            }
            cgReleaseMemoryObjects(ctx, queue, memrefs, nmemrefs, NULL, 0, CG_INVALID_HANDLE);
            return result;
        }
        return cgReleaseMemoryObjects(ctx, queue, memrefs, nmemrefs, &cl_done, 1, ddp->CompleteEvent);
    }
    return result;
}

/// @summary Implements the COPY_BUFFER_TO_IMAGE command, which copies data from one image to another.
/// @param ctx The CGFX context returned by cgEnumerateDevices.
/// @param queue The command queue into which the command is being inserted.
/// @param cmdbuf The command buffer defining the command.
/// @param cmd The command and any associated data.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_INVALID_STATE, CG_BAD_CLCONTEXT, CG_UNSUPPORTED, CG_OUT_OF_MEMORY, CG_OUT_OF_OBJECTS, CG_UNSUPPORTED or CG_ERROR.
internal_function int
cgExecuteCopyBufferToImage
(
    CG_CONTEXT    *ctx, 
    CG_QUEUE      *queue, 
    CG_CMD_BUFFER *cmdbuf, 
    cg_command_t  *cmd
)
{   UNREFERENCED_PARAMETER(cmdbuf);
    cg_copy_buffer_to_image_cmd_t  *ddp  = (cg_copy_buffer_to_image_cmd_t*) cmd->Data;
    CG_BUFFER         *srcbuf  =  cgObjectTableGet(&ctx->BufferTable, ddp->SourceBuffer);
    CG_IMAGE          *dstimg  =  cgObjectTableGet(&ctx->ImageTable , ddp->TargetImage);
    int                result  =  CG_SUCCESS;
    size_t            nmemrefs =  0;
    cl_uint           nwaitevt =  0;
    cl_event          acquire  =  NULL;
    cl_mem            memrefs[2];
    cgMemRefListAddBuffer(srcbuf, memrefs, nmemrefs, 2, false);
    cgMemRefListAddImage (dstimg, memrefs, nmemrefs, 2, false);
    if ((result = cgAcquireMemoryObjects(ctx, queue, memrefs, nmemrefs, ddp->WaitEvent, &acquire, nwaitevt, 1)) == CL_SUCCESS)
    {
        cl_event  cl_done = NULL;
        cl_int    cl_res  = clEnqueueCopyBufferToImage(
            queue->CommandQueue, 
            memrefs[0], /* src */
            memrefs[1], /* dst */
            ddp->SourceOffset, 
            ddp->TargetOrigin, 
            ddp->Dimensions, 
            nwaitevt, 
            CG_OPENCL_WAIT_LIST(nwaitevt, &acquire),
            &cl_done);
        if  (cl_res != CL_SUCCESS)
        {
            switch (cl_res)
            {
            case CL_INVALID_OPERATION            : result = CG_UNSUPPORTED;   break;
            case CL_INVALID_COMMAND_QUEUE        : result = CG_BAD_CLCONTEXT; break;
            case CL_INVALID_CONTEXT              : result = CG_BAD_CLCONTEXT; break;
            case CL_INVALID_MEM_OBJECT           : result = CG_BAD_CLCONTEXT; break;
            case CL_INVALID_IMAGE_SIZE           : result = CG_UNSUPPORTED;   break;
            case CL_MISALIGNED_SUB_BUFFER_OFFSET : result = CG_INVALID_VALUE; break;
            case CL_INVALID_VALUE                : result = CG_INVALID_VALUE; break;
            case CL_INVALID_EVENT_WAIT_LIST      : result = CG_INVALID_VALUE; break;
            case CL_MEM_COPY_OVERLAP             : result = CG_INVALID_VALUE; break;
            case CL_MEM_OBJECT_ALLOCATION_FAILURE: result = CG_OUT_OF_MEMORY; break;
            case CL_OUT_OF_RESOURCES             : result = CG_OUT_OF_MEMORY; break;
            case CL_OUT_OF_HOST_MEMORY           : result = CG_OUT_OF_MEMORY; break;
            default                              : result = CG_ERROR;         break;
            }
            cgReleaseMemoryObjects(ctx, queue, memrefs, nmemrefs, NULL, 0, CG_INVALID_HANDLE);
            return result;
        }
        return cgReleaseMemoryObjects(ctx, queue, memrefs, nmemrefs, &cl_done, 1, ddp->CompleteEvent);
    }
    return result;
}

/// @summary Implements the COPY_IMAGE_TO_BUFFER command, which copies data from one image to another.
/// @param ctx The CGFX context returned by cgEnumerateDevices.
/// @param queue The command queue into which the command is being inserted.
/// @param cmdbuf The command buffer defining the command.
/// @param cmd The command and any associated data.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_INVALID_STATE, CG_BAD_CLCONTEXT, CG_UNSUPPORTED, CG_OUT_OF_MEMORY, CG_OUT_OF_OBJECTS, CG_UNSUPPORTED or CG_ERROR.
internal_function int
cgExecuteCopyImageToBuffer
(
    CG_CONTEXT    *ctx, 
    CG_QUEUE      *queue, 
    CG_CMD_BUFFER *cmdbuf, 
    cg_command_t  *cmd
)
{   UNREFERENCED_PARAMETER(cmdbuf);
    cg_copy_image_to_buffer_cmd_t  *ddp  = (cg_copy_image_to_buffer_cmd_t*) cmd->Data;
    CG_IMAGE          *srcimg  =  cgObjectTableGet(&ctx->ImageTable , ddp->SourceImage);
    CG_BUFFER         *dstbuf  =  cgObjectTableGet(&ctx->BufferTable, ddp->TargetBuffer);
    int                result  =  CG_SUCCESS;
    size_t            nmemrefs =  0;
    cl_uint           nwaitevt =  0;
    cl_event          acquire  =  NULL;
    cl_mem            memrefs[2];
    cgMemRefListAddImage (srcimg, memrefs, nmemrefs, 2, false);
    cgMemRefListAddBuffer(dstbuf, memrefs, nmemrefs, 2, false);
    if ((result = cgAcquireMemoryObjects(ctx, queue, memrefs, nmemrefs, ddp->WaitEvent, &acquire, nwaitevt, 1)) == CL_SUCCESS)
    {
        cl_event  cl_done = NULL;
        cl_int    cl_res  = clEnqueueCopyImageToBuffer(
            queue->CommandQueue, 
            memrefs[0], /* src */
            memrefs[1], /* dst */
            ddp->SourceOrigin, 
            ddp->Dimensions, 
            ddp->TargetOffset, 
            nwaitevt, 
            CG_OPENCL_WAIT_LIST(nwaitevt, &acquire),
            &cl_done);
        if  (cl_res != CL_SUCCESS)
        {
            switch (cl_res)
            {
            case CL_INVALID_OPERATION            : result = CG_UNSUPPORTED;   break;
            case CL_INVALID_COMMAND_QUEUE        : result = CG_BAD_CLCONTEXT; break;
            case CL_INVALID_CONTEXT              : result = CG_BAD_CLCONTEXT; break;
            case CL_INVALID_MEM_OBJECT           : result = CG_BAD_CLCONTEXT; break;
            case CL_INVALID_IMAGE_SIZE           : result = CG_UNSUPPORTED;   break;
            case CL_MISALIGNED_SUB_BUFFER_OFFSET : result = CG_INVALID_VALUE; break;
            case CL_INVALID_VALUE                : result = CG_INVALID_VALUE; break;
            case CL_INVALID_EVENT_WAIT_LIST      : result = CG_INVALID_VALUE; break;
            case CL_MEM_COPY_OVERLAP             : result = CG_INVALID_VALUE; break;
            case CL_MEM_OBJECT_ALLOCATION_FAILURE: result = CG_OUT_OF_MEMORY; break;
            case CL_OUT_OF_RESOURCES             : result = CG_OUT_OF_MEMORY; break;
            case CL_OUT_OF_HOST_MEMORY           : result = CG_OUT_OF_MEMORY; break;
            default                              : result = CG_ERROR;         break;
            }
            cgReleaseMemoryObjects(ctx, queue, memrefs, nmemrefs, NULL, 0, CG_INVALID_HANDLE);
            return result;
        }
        return cgReleaseMemoryObjects(ctx, queue, memrefs, nmemrefs, &cl_done, 1, ddp->CompleteEvent);
    }
    return result;
}

/// @summary Executes a command buffer against an out-of-order transfer queue.
/// @param ctx The CGFX context defining the command queue.
/// @param queue The CGFX command queue, which must be a transfer queue.
/// @param cmdbuf The CGFX command buffer to submit to the device queue.
/// @return CG_SUCCESS, CG_COMMAND_NOT_IMPLEMENTED, or another result code.
internal_function int
cgExecuteTransferCommandBuffer
(
    CG_CONTEXT    *ctx, 
    CG_QUEUE      *queue, 
    CG_CMD_BUFFER *cmdbuf
)
{
    cg_command_t  *cmd = NULL;
    size_t         ofs = 0;
    int            res = CG_SUCCESS;

    while ((cmd = cgCmdBufferCommandAt(cmdbuf, ofs, res)) != NULL && res == CG_SUCCESS)
    {
        switch (cmd->CommandId)
        {
        case CG_COMMAND_DEVICE_FENCE:
            res = cgExecuteDeviceFence(ctx, queue, cmdbuf, cmd);
            break;
        case CG_COMMAND_COPY_BUFFER:
            res = cgExecuteCopyBufferRegion(ctx, queue, cmdbuf, cmd);
            break;
        case CG_COMMAND_COPY_IMAGE:
            res = cgExecuteCopyImageRegion(ctx, queue, cmdbuf, cmd);
            break;
        case CG_COMMAND_COPY_BUFFER_TO_IMAGE:
            res = cgExecuteCopyImageToBuffer(ctx, queue, cmdbuf, cmd);
            break;
        case CG_COMMAND_COPY_IMAGE_TO_BUFFER:
            res = cgExecuteCopyBufferToImage(ctx, queue, cmdbuf, cmd);
            break;
        default:
            res = CG_COMMAND_NOT_IMPLEMENTED;
            break;
        }
    }
    if (res == CG_END_OF_BUFFER)
        res  = CG_SUCCESS;

    return res;
}

/// @summary Executes a command buffer against an out-of-order compute queue.
/// @param ctx The CGFX context defining the command queue.
/// @param queue The CGFX command queue, which must be a compute queue.
/// @param cmdbuf The CGFX command buffer to submit to the device queue.
/// @return CG_SUCCESS, CG_COMMAND_NOT_IMPLEMENTED, or another result code.
internal_function int
cgExecuteComputeCommandBuffer
(
    CG_CONTEXT    *ctx, 
    CG_QUEUE      *queue, 
    CG_CMD_BUFFER *cmdbuf
)
{
    cg_command_t *cmd = NULL;
    size_t        ofs = 0;
    int           res = CG_SUCCESS;

    while ((cmd = cgCmdBufferCommandAt(cmdbuf, ofs, res)) != NULL && res == CG_SUCCESS)
    {
        switch (cmd->CommandId)
        {
        case CG_COMMAND_DEVICE_FENCE:
            res = cgExecuteDeviceFence(ctx, queue, cmdbuf, cmd);
            break;
        case CG_COMMAND_COPY_BUFFER:
            res = cgExecuteCopyBufferRegion(ctx, queue, cmdbuf, cmd);
            break;
        case CG_COMMAND_COPY_IMAGE:
            res = cgExecuteCopyImageRegion(ctx, queue, cmdbuf, cmd);
            break;
        case CG_COMMAND_COPY_BUFFER_TO_IMAGE:
            res = cgExecuteCopyImageToBuffer(ctx, queue, cmdbuf, cmd);
            break;
        case CG_COMMAND_COPY_IMAGE_TO_BUFFER:
            res = cgExecuteCopyBufferToImage(ctx, queue, cmdbuf, cmd);
            break;
        case CG_COMMAND_PIPELINE_DISPATCH:
            res = cgExecuteComputePipelineDispatch(ctx, queue, cmdbuf, cmd);
            break;
        default:
            res = CG_COMMAND_NOT_IMPLEMENTED;
            break;
        }
    }
    if (res == CG_END_OF_BUFFER)
        res  = CG_SUCCESS;

    return res;
}

/// @summary Executes a command buffer against an in-order graphics queue.
/// @param ctx The CGFX context defining the command queue.
/// @param queue The CGFX command queue, which must be a graphics queue.
/// @param cmdbuf The CGFX command buffer to submit to the device queue.
/// @return CG_SUCCESS, CG_COMMAND_NOT_IMPLEMENTED, or another result code.
internal_function int
cgExecuteGraphicsCommandBuffer
(
    CG_CONTEXT    *ctx, 
    CG_QUEUE      *queue, 
    CG_CMD_BUFFER *cmdbuf
)
{
    cg_command_t  *cmd = NULL;
    size_t         ofs = 0;
    int            res = CG_SUCCESS;

    while ((cmd = cgCmdBufferCommandAt(cmdbuf, ofs, res)) != NULL && res == CG_SUCCESS)
    {
        switch (cmd->CommandId)
        {
        case CG_COMMAND_DEVICE_FENCE:
            res = cgExecuteDeviceFence(ctx, queue, cmdbuf, cmd);
            break;
        case CG_COMMAND_PIPELINE_DISPATCH:
            res = cgExecuteGraphicsPipelineDispatch(ctx, queue, cmdbuf, cmd);
            break;
        default:
            res = CG_COMMAND_NOT_IMPLEMENTED;
            break;
        }
    }
    if (res == CG_END_OF_BUFFER)
        res  = CG_SUCCESS;

    return res;
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
    case CG_ERROR:                   return "Generic error (CG_ERROR)";
    case CG_INVALID_VALUE:           return "One or more arguments are invalid (CG_INVALID_VALUE)";
    case CG_OUT_OF_MEMORY:           return "Unable to allocate required memory (CG_OUT_OF_MEMORY)";
    case CG_NO_PIXELFORMAT:          return "No accelerated pixel format could be found (CG_NO_PIXELFORMAT)";
    case CG_BAD_PIXELFORMAT:         return "Unable to set the display pixel format (CG_BAD_PIXELFORMAT)";
    case CG_NO_GLCONTEXT:            return "Unable to create an OpenGL rendering context (CG_NO_GLCONTEXT)";
    case CG_BAD_GLCONTEXT:           return "The OpenGL rendering context is invalid (CG_BAD_GLCONTEXT)";
    case CG_NO_CLCONTEXT:            return "Unable to create an OpenCL resource context (CG_NO_CLCONTEXT)";
    case CG_BAD_CLCONTEXT:           return "The OpenCL resource context is invalid (CG_BAD_CLCONTEXT)";
    case CG_OUT_OF_OBJECTS:          return "The maximum number of objects has been exceeded (CG_OUT_OF_OBJECTS)";
    case CG_UNKNOWN_GROUP:           return "The object is not associated with an execution group (CG_UNKNOWN_GROUP)";
    case CG_INVALID_STATE:           return "The object is in an invalid state for the operation (CG_INVALID_OBJECT)";
    case CG_COMPILE_FAILED:          return "The kernel program could not be compiled (CG_COMPILE_FAILED)";
    case CG_LINK_FAILED:             return "The kernel program could not be linked (CG_LINK_FAILED)";
    case CG_TOO_MANY_MEMREFS:        return "The command references too many memory objects (CG_TOO_MANY_MEMREFS)";
    case CG_COMMAND_NOT_IMPLEMENTED: return "A command is not implemented or is not supported on the queue (CG_COMMAND_NOT_IMPLEMENTED)";
    case CG_SUCCESS:                 return "Success (CG_SUCCESS)";
    case CG_UNSUPPORTED:             return "Unsupported operation (CG_UNSUPPORTED)";
    case CG_NOT_READY:               return "Result not ready (CG_NOT_READY)";
    case CG_TIMEOUT:                 return "Wait completed due to timeout (CG_TIMEOUT)";
    case CG_SIGNALED:                return "Wait completed due to event signal (CG_SIGNALED)";
    case CG_BUFFER_TOO_SMALL:        return "Buffer too small (CG_BUFFER_TOO_SMALL)";
    case CG_NO_OPENCL:               return "No compatible OpenCL platforms or devices available (CG_NO_OPENCL)";
    case CG_NO_OPENGL:               return "No compatible OpenGL devices are available (CG_NO_OPENGL)";
    case CG_NO_GLSHARING:            return "OpenGL rendering context created but OpenCL interop unavailable (CG_NO_GLSHARING)";
    case CG_NO_QUEUE_OF_TYPE:        return "The object does not have a command queue of the specified type (CG_NO_QUEUE_OF_TYPE)";
    case CG_END_OF_BUFFER:           return "The end of the buffer has been reached (CG_END_OF_BUFFER)";
    default:                         break;
    }
    if (result <= CG_RESULT_FAILURE_EXT || result >= CG_RESULT_NON_FAILURE_EXT)
        return cgResultStringEXT(result);
    else
        return "Unknown Result (CORE)";
}

/// @summary Enumerate all CPU resources available in the system.
/// @param cpu_info On return, stores information about the available CPU resources in the system.
/// @return CG_SUCCESS or CG_OUT_OF_MEMORY.
library_function int
cgGetCpuInfo
(
    cg_cpu_info_t *cpu_info
)
{
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *lpibuf = NULL;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *info   = NULL;
    size_t   smt_count   = 0;
    uint8_t *bufferp     = NULL;
    uint8_t *buffere     = NULL;
    DWORD    buffer_size = 0;
    int         regs[4]  ={0, 0, 0, 0};

    // zero out the CPU information returned to the caller.
    memset(cpu_info, 0, sizeof(cg_cpu_info_t));
    
    // retrieve the CPU vendor string using the __cpuid intrinsic.
    __cpuid(regs  , 0); // CPUID function 0
    *((int*)&cpu_info->VendorName[0]) = regs[1]; // EBX
    *((int*)&cpu_info->VendorName[4]) = regs[3]; // ECX
    *((int*)&cpu_info->VendorName[8]) = regs[2]; // EDX
         if (!strcmp(cpu_info->VendorName, "AuthenticAMD")) cpu_info->PreferAMD        = true;
    else if (!strcmp(cpu_info->VendorName, "GenuineIntel")) cpu_info->PreferIntel      = true;
    else if (!strcmp(cpu_info->VendorName, "KVMKVMKVMKVM")) cpu_info->IsVirtualMachine = true;
    else if (!strcmp(cpu_info->VendorName, "Microsoft Hv")) cpu_info->IsVirtualMachine = true;
    else if (!strcmp(cpu_info->VendorName, "VMwareVMware")) cpu_info->IsVirtualMachine = true;
    else if (!strcmp(cpu_info->VendorName, "XenVMMXenVMM")) cpu_info->IsVirtualMachine = true;

    // figure out the amount of space required, and allocate a temporary buffer:
    GetLogicalProcessorInformationEx(RelationAll, NULL, &buffer_size);
    if ((lpibuf = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*) malloc(size_t(buffer_size))) == NULL)
    {   // unable to allocate the required memory:
        cpu_info->NUMANodes       = 1;
        cpu_info->PhysicalCPUs    = 1;
        cpu_info->PhysicalCores   = 1;
        cpu_info->HardwareThreads = 1;
        cpu_info->ThreadsPerCore  = 1;
        return CG_OUT_OF_MEMORY;
    }
    GetLogicalProcessorInformationEx(RelationAll, lpibuf, &buffer_size);

    // initialize the output counts:
    cpu_info->NUMANodes       = 0;
    cpu_info->PhysicalCPUs    = 0;
    cpu_info->PhysicalCores   = 0;
    cpu_info->HardwareThreads = 0;
    cpu_info->ThreadsPerCore  = 0;

    // step through the buffer and update counts:
    bufferp = (uint8_t*) lpibuf;
    buffere =((uint8_t*) lpibuf) + size_t(buffer_size);
    while (bufferp < buffere)
    {
        info = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*) bufferp;
        switch (info->Relationship)
        {
        case RelationNumaNode:
            cpu_info->NUMANodes++;
            break;
        case RelationProcessorPackage:
            cpu_info->PhysicalCPUs++;
            break;
        case RelationProcessorCore:
            cpu_info->PhysicalCores++;
            if (info->Processor.Flags == LTP_PC_SMT)
                smt_count++;
            break;
        default: // RelationGroup, RelationCache - don't care.
            break;
        }
        bufferp += size_t(info->Size);
    }
    // free the temporary buffer:
    free(lpibuf);

    // determine the total number of logical processors in the system.
    // use this value to figure out the number of threads per-core.
    if (smt_count > 0)
    {   // determine the number of logical processors in the system and
        // use this value to figure out the number of threads per-core.
        SYSTEM_INFO sysinfo;
        GetNativeSystemInfo(&sysinfo);
        cpu_info->ThreadsPerCore = size_t(sysinfo.dwNumberOfProcessors) / smt_count;
    }
    else
    {   // there are no SMT-enabled CPUs in the system, so 1 thread per-core.
        cpu_info->ThreadsPerCore = 1;
    }

    // calculate the total number of available hardware threads.
    cpu_info->HardwareThreads = (smt_count * cpu_info->ThreadsPerCore) + (cpu_info->PhysicalCores - smt_count);
    return CG_SUCCESS;
}

/// @summary Initialize the fields of a CPU partition layout to default values. All hardware threads are configured for use executing OpenCL kernels.
/// @param cpu_partition The CPU device parition layout to initialize.
/// @return CG_SUCCESS.
library_function int
cgDefaultCpuPartition
(
    cg_cpu_partition_t *cpu_partition
)
{
    cpu_partition->PartitionType  = CG_CPU_PARTITION_NONE;
    cpu_partition->ReserveThreads = 0;
    cpu_partition->PartitionCount = 0;
    cpu_partition->ThreadCounts   = NULL;
    return CG_SUCCESS;
}

/// @summary Performs basic validation on a CPU partition layout.
/// @param cpu_partition The CPU partition layout.
/// @param cpu_info The CPU information to validate against.
/// @return CG_SUCCESS if the partitioning is likely to be successful.
library_function int
cgValidateCpuPartition
(
    cg_cpu_partition_t const *cpu_partition, 
    cg_cpu_info_t      const *cpu_info
)
{   // perform validation according to device partition type.
    if (cpu_partition->PartitionType == CG_CPU_PARTITION_NONE)
    {
        if (cpu_partition->ReserveThreads >= cpu_info->HardwareThreads)
            return CG_INVALID_VALUE;
    }
    else if (cpu_partition->PartitionType == CG_CPU_PARTITION_PER_CORE)
    {
        if (cpu_partition->ReserveThreads >= cpu_info->HardwareThreads)
            return CG_INVALID_VALUE;
    }
    else if (cpu_partition->PartitionType == CG_CPU_PARTITION_PER_NODE)
    {   // for device per-node, threads are also reserved per-node.
        if ((cpu_partition->ReserveThreads * cpu_info->NUMANodes) >= cpu_info->HardwareThreads)
            return CG_INVALID_VALUE;
    }
    else if (cpu_partition->PartitionType == CG_CPU_PARTITION_EXPLICIT)
    {   // at most one negative value is allowed meaning 'all remaining threads'.
        if (cpu_partition->PartitionCount == 0)
            return CG_INVALID_VALUE; // no partitions defined
        if (cpu_partition->ThreadCounts == NULL)
            return CG_INVALID_VALUE; // PartitionCount > 0, but no partition values

        size_t negative_count = 0; 
        size_t thread_count   = cpu_partition->ReserveThreads;
        for (size_t i = 0 , n = cpu_partition->PartitionCount; i < n; ++i)
        {
            if (cpu_partition->ThreadCounts[i] == 0)
                return CG_INVALID_VALUE; // cannot have a partition with no resources
            else if (cpu_partition->ThreadCounts[i] < 0)
                negative_count++;
            else
                thread_count += cpu_partition->ThreadCounts[i];
        }
        if (negative_count > 1)
            return CG_INVALID_VALUE; // only one entry can mean 'all remaining threads'
        if (thread_count > cpu_info->HardwareThreads)
            return CG_INVALID_VALUE; // cannot subscribe more threads than are available
        if (negative_count > 0 && thread_count == cpu_info->HardwareThreads)
            return CG_INVALID_VALUE; // cannot have a partition with no resources
    }
    else
    {   // the partition type is invalid.
        return CG_INVALID_VALUE;
    }
    return CG_SUCCESS;
}

/// @summary Register the application with CGFX and enumerate all compute resources and displays in the system.
/// @param app_info Information about the application using CGFX services.
/// @param alloc_cb A custom host memory allocator, or NULL to use the default host memory allocator.
/// @param cpu_partition Information about how CPU devices should be partitioned, or NULL to use all CPU resources for data-parallel kernel execution.
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
    cg_cpu_partition_t const    *cpu_partition,
    size_t                      &device_count,
    cg_handle_t                 *device_list,
    size_t                       max_devices,
    uintptr_t                   &context
)
{
    cg_cpu_partition_t cpu_default;
    int           res = CG_SUCCESS;
    size_t heap_count = 0;
    CG_HEAP    *heaps = NULL;
    bool      new_ctx = false;
    CG_CONTEXT   *ctx = NULL;

    // configure the default CPU partitioning, which uses all CPU hardware 
    // threads for data-parallel work distribution driven by a single queue.
    cpu_default.PartitionType  = CG_CPU_PARTITION_NONE;
    cpu_default.ReserveThreads = 0;
    cpu_default.PartitionCount = 0;
    cpu_default.ThreadCounts   = NULL;
    if (cpu_partition == NULL)
        cpu_partition = &cpu_default;

    if (context != 0)
    {   // the caller is calling with an existing context to be updated.
        ctx      =(CG_CONTEXT*) context;
        new_ctx  = false;
    }
    else
    {   // allocate a new, empty context for the caller.
        if ((ctx = cgCreateContext(app_info, alloc_cb, res)) == NULL)
        {   // the CGFX context object could not be allocated.
            device_count = 0;
            context = 0;
            return res;
        }
        new_ctx  = true;
    }

    // count the number of CPU resources available in the system.
    if ((res = cgGetCpuInfo(&ctx->CpuInfo)) != CG_SUCCESS)
    {   // typically, memory allocation failed.
        goto error_cleanup;
    }
    // perform basic validation of the desired CPU partition scheme.
    if ((res = cgValidateCpuPartition(cpu_partition, &ctx->CpuInfo)) != CG_SUCCESS)
    {   // the basic partition values are invalid or incomplete.
        goto error_cleanup;
    }

    // enumerate all displays attached to the system.
    if ((res = cgGlEnumerateDisplays(ctx)) != CG_SUCCESS)
    {   // several things could have gone wrong.
        goto error_cleanup;
    }

    // enumerate all OpenCL platforms and devices in the system.
    if ((res = cgClEnumerateDevices(ctx, cpu_partition)) != CG_SUCCESS)
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

    // count device heaps and gather heap information.
    heap_count =  cgClCountUniqueHeaps(ctx);
    if ((heaps = (CG_HEAP*) cgAllocateHostMemory(&ctx->HostAllocator, heap_count * sizeof(CG_HEAP), 1, CG_ALLOCATION_TYPE_INTERNAL)) == NULL)
    {   // unable to allocate the required memory.
        goto error_cleanup;
    }
    ctx->HeapCount = cgClGetHeapInformation(ctx, heaps, heap_count);
    ctx->HeapList  = heaps;

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

/// @summary Frees all resources associated with a CGFX context and invalidates the context.
/// @param context A CGFX context returned by cgEnumerateDevices().
/// @return CG_SUCCESS.
library_function int
cgDestroyContext
(
    uintptr_t context
)
{
    CG_CONTEXT *ctx = (CG_CONTEXT*) context;
    if (ctx != NULL)   cgDeleteContext(ctx);
    return CG_SUCCESS;
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
#define BUFFER_SET_SCALAR(type, value) \
    *((type*)buffer) = (value)
    // ==========================================================
    // ==========================================================

    // retrieve parameter data:
    switch (param)
    {
    case CG_CONTEXT_CPU_COUNTS:
        {   BUFFER_CHECK_TYPE(cg_cpu_info_t);
            memcpy(buffer, &ctx->CpuInfo, sizeof(cg_cpu_info_t));
        }
        return CG_SUCCESS;

    case CG_CONTEXT_DEVICE_COUNT:
        {   BUFFER_CHECK_TYPE(size_t);
            BUFFER_SET_SCALAR(size_t, ctx->DeviceTable.ObjectCount);
        }
        return CG_SUCCESS;

    case CG_CONTEXT_DISPLAY_COUNT:
        {   BUFFER_CHECK_TYPE(size_t);
            BUFFER_SET_SCALAR(size_t, ctx->DisplayTable.ObjectCount);
        }
        return CG_SUCCESS;

    default:
        {
            if (bytes_needed != NULL) *bytes_needed = 0;
            return CG_INVALID_VALUE;
        }
    }

#undef  BUFFER_SET_SCALAR
#undef  BUFFER_CHECK_SIZE
#undef  BUFFER_CHECK_TYPE
}

/// @summary Retrieve the number of heaps available to compute devices in the system.
/// @param context The CGFX context returned by a prior call to cgEnumerateDevices().
/// @return The number of heaps available in the system.
library_function size_t
cgGetHeapCount
(
    uintptr_t context
)
{
    CG_CONTEXT *ctx = (CG_CONTEXT*) context;
    return ctx->HeapCount;
}

/// @summary Retrieve the properties of a memory heap.
/// @param context A CGFX context returned by cgEnumerateDevices().
/// @param heap_ordinal The zero-based ordinal of the heap to query, in [0, cgGetHeapCount(context)).
/// @param heap_info On return, this structure stores heap attributes.
/// @return CG_SUCCESS or CG_INVALID_VALUE.
library_function int
cgGetHeapProperties
(
    uintptr_t       context, 
    size_t          heap_ordinal, 
    cg_heap_info_t &heap_info
)
{
    CG_CONTEXT  *ctx = (CG_CONTEXT*) context;

    // zero out the heap data for the caller.
    memset(&heap_info, 0, sizeof(cg_heap_info_t));
    heap_info.Ordinal   = heap_ordinal;

    // validate the heap index.
    if (heap_ordinal   >= ctx->HeapCount)
        return CG_INVALID_VALUE;

    // copy over the heap attributes.
    heap_info.Type            = ctx->HeapList[heap_ordinal].Type;
    heap_info.Flags           = ctx->HeapList[heap_ordinal].Flags;
    heap_info.HeapSize        = ctx->HeapList[heap_ordinal].HeapSizeTotal;
    heap_info.PinnableSize    = ctx->HeapList[heap_ordinal].PinnableTotal;
    heap_info.DeviceAlignment = ctx->HeapList[heap_ordinal].DeviceAlignment;
    heap_info.UserAlignment   = ctx->HeapList[heap_ordinal].UserAlignment;
    heap_info.UserSizeAlign   = ctx->HeapList[heap_ordinal].UserSizeAlign;
    return CG_SUCCESS;
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
    void       *buffer,
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
#define BUFFER_SET_SCALAR(type, value) \
    *((type*)buffer) = (value)
    // ==========================================================
    // ==========================================================

    // retrieve parameter data:
    switch (param)
    {
    case CG_DEVICE_CL_PLATFORM_ID:
        {   BUFFER_CHECK_TYPE(cl_platform_id);
            BUFFER_SET_SCALAR(cl_platform_id, device->PlatformId);
        }
        return CG_SUCCESS;

    case CG_DEVICE_CL_DEVICE_ID:
        {   BUFFER_CHECK_TYPE(cl_device_id);
            BUFFER_SET_SCALAR(cl_device_id, device->DeviceId);
        }
        return CG_SUCCESS;

    case CG_DEVICE_CL_DEVICE_TYPE:
        {   BUFFER_CHECK_TYPE(cl_device_type);
            BUFFER_SET_SCALAR(cl_device_type, device->Type);
        }
        return CG_SUCCESS;

    case CG_DEVICE_WINDOWS_HGLRC:
        {   BUFFER_CHECK_TYPE(HGLRC);
            BUFFER_SET_SCALAR(HGLRC, device->DisplayRC);
        }
        return CG_SUCCESS;

    case CG_DEVICE_DISPLAY_COUNT:
        {   BUFFER_CHECK_TYPE(size_t);
            BUFFER_SET_SCALAR(size_t, device->DisplayCount);
        }
        return CG_SUCCESS;

    case CG_DEVICE_ATTACHED_DISPLAYS:
        {   BUFFER_CHECK_SIZE(device->DisplayCount * sizeof(cg_handle_t));
            memcpy(buffer, device->AttachedDisplays, device->DisplayCount * sizeof(cg_handle_t));
        }
        return CG_SUCCESS;

    case CG_DEVICE_PRIMARY_DISPLAY:
        {   BUFFER_CHECK_TYPE(cg_handle_t);
            for (size_t i = 0, n = ctx->DisplayTable.ObjectCount; i < n; ++i)
            {
                CG_DISPLAY *display = &ctx->DisplayTable.Objects[i];
                if (display->DisplayInfo.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
                {
                    cg_handle_t handle = cgMakeHandle(&ctx->DisplayTable, i);
                    BUFFER_SET_SCALAR(cg_handle_t, handle);
                    return CG_SUCCESS;
                }
            }
            BUFFER_SET_SCALAR(cg_handle_t, CG_INVALID_HANDLE);
        }
        return CG_SUCCESS;

    default:
        {
            if (bytes_needed != NULL) *bytes_needed = 0;
            return CG_INVALID_VALUE;
        }
    }

#undef  BUFFER_SET_SCALAR
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
    void       *buffer,
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
#define BUFFER_SET_SCALAR(type, value) \
    *((type*)buffer) = (value)
    // ==========================================================
    // ==========================================================

    // retrieve parameter data:
    switch (param)
    {
    case CG_DISPLAY_DEVICE:
        {   BUFFER_CHECK_TYPE(cg_handle_t);
            if (display->DisplayDevice != NULL)
            {
                cg_handle_t handle = cgMakeHandle(display->DisplayDevice->ObjectId, CG_OBJECT_DEVICE, CG_DEVICE_TABLE_ID);
                BUFFER_SET_SCALAR(cg_handle_t, handle);
            }
            else BUFFER_SET_SCALAR(cg_handle_t, CG_INVALID_HANDLE);
        }
        return CG_SUCCESS;

    case CG_DISPLAY_CL_PLATFORM_ID:
        {   BUFFER_CHECK_TYPE(cl_platform_id);
            if (display->DisplayDevice != NULL) BUFFER_SET_SCALAR(cl_platform_id, display->DisplayDevice->PlatformId);
            else BUFFER_SET_SCALAR(cl_platform_id, CG_INVALID_HANDLE);
        }
        return CG_SUCCESS;

    case CG_DISPLAY_CL_DEVICE_ID:
        {   BUFFER_CHECK_TYPE(cl_device_id);
            if (display->DisplayDevice != NULL) BUFFER_SET_SCALAR(cl_device_id, display->DisplayDevice->DeviceId);
            else BUFFER_SET_SCALAR(cl_device_id, CG_INVALID_HANDLE);
        }
        return CG_SUCCESS;

    case CG_DISPLAY_WINDOWS_HDC:
        {   BUFFER_CHECK_TYPE(HDC);
            BUFFER_SET_SCALAR(HDC, display->DisplayDC);
        }
        return CG_SUCCESS;

    case CG_DISPLAY_WINDOWS_HGLRC:
        {   BUFFER_CHECK_TYPE(HGLRC);
            BUFFER_SET_SCALAR(HGLRC, display->DisplayRC);
        }
        return CG_SUCCESS;

    case CG_DISPLAY_POSITION:
        {   BUFFER_CHECK_SIZE(2 * sizeof(int));
            int xy[2] = { display->DisplayX, display->DisplayY };
            memcpy(buffer, xy, 2 * sizeof(int));
        }
        return CG_SUCCESS;

    case CG_DISPLAY_SIZE:
        {   BUFFER_CHECK_SIZE(2 * sizeof(size_t));
            size_t wh[2] = { display->DisplayWidth, display->DisplayHeight };
            memcpy(buffer, wh, 2 * sizeof(size_t));
        }
        return CG_SUCCESS;

    case CG_DISPLAY_ORIENTATION:
        {   BUFFER_CHECK_TYPE(cg_display_orientation_e);
            DEVMODE dm; dm.dmSize = sizeof(DEVMODE);
            EnumDisplaySettingsEx(display->DisplayInfo.DeviceName, ENUM_CURRENT_SETTINGS, &dm, 0);
            if (dm.dmPelsWidth >= dm.dmPelsHeight) BUFFER_SET_SCALAR(cg_display_orientation_e, CG_DISPLAY_ORIENTATION_LANDSCAPE);
            else BUFFER_SET_SCALAR(cg_display_orientation_e, CG_DISPLAY_ORIENTATION_PORTRAIT);
        }
        return CG_SUCCESS;

    case CG_DISPLAY_REFRESH_RATE:
        {   BUFFER_CHECK_TYPE(float);
            float refresh_rate_hz = (float) GetDeviceCaps(display->DisplayDC, VREFRESH);
            BUFFER_SET_SCALAR(float, refresh_rate_hz);
        }
        return CG_SUCCESS;

    default:
        {
            if (bytes_needed != NULL) *bytes_needed = 0;
            return CG_INVALID_VALUE;
        }
    }

#undef  BUFFER_SET_SCALAR
#undef  BUFFER_CHECK_SIZE
#undef  BUFFER_CHECK_TYPE
}

/// @summary Retrieve the GPU compute device that drives the specified display.
/// @param context A CGFX context returned by cgEnumerateDevices().
/// @param display_handle The handle of the display to query.
/// @return The handle of the GPU device driving the display, or CG_INVALID_HANDLE.
library_function cg_handle_t
cgGetDisplayDevice
(
    uintptr_t   context,
    cg_handle_t display_handle
)
{
    CG_CONTEXT *ctx     = (CG_CONTEXT*) context;
    CG_DISPLAY *display =  NULL;
    if ((display = cgObjectTableGet(&ctx->DisplayTable, display_handle)) == NULL)
    {   // the specified display handle is invalid.
        return CG_INVALID_HANDLE;
    }
    if (display->DisplayDevice != NULL)
    {   // return the handle of the associated device.
        return cgMakeHandle(display->DisplayDevice->ObjectId, CG_OBJECT_DEVICE, CG_DEVICE_TABLE_ID);
    }
    else return CG_INVALID_HANDLE;
}

/// @summary Retrieves all CPU devices that can share resources with a given device.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param cpu_count On return, stores the number of CPU devices that can share resources with the specified device.
/// @param max_devices The maximum number of device handles to write to @a cpu_handles.
/// @param cpu_handles An array of device handles indicating the CPU devices in the sharegroup.
/// @return CG_SUCCESS.
library_function int
cgGetCPUDevices
(
    uintptr_t     context,
    size_t       &cpu_count,
    size_t const  max_devices,
    cg_handle_t  *cpu_handles
)
{
    cpu_count = 0;
    CG_CONTEXT *ctx = (CG_CONTEXT*) context;
    for (size_t i = 0, n = ctx->DeviceTable.ObjectCount; i < n; ++i)
    {
        CG_DEVICE &dev = ctx->DeviceTable.Objects[i];
        if (dev.Type  == CL_DEVICE_TYPE_CPU)
        {
            if (cpu_count < max_devices)
                cpu_handles[cpu_count] = cgMakeHandle(&ctx->DeviceTable, i);
            cpu_count++;
        }
    }
    return CG_SUCCESS;
}

/// @summary Retrieves all GPU devices that can share resources with a given device.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param gpu_count On return, stores the number of GPU devices that can share resources with the specified device.
/// @param max_devices The maximum number of device handles to write to @a gpu_handles.
/// @param gpu_handles An array of device handles indicating the GPU devices in the sharegroup.
/// @return CG_SUCCESS.
library_function int
cgGetGPUDevices
(
    uintptr_t     context,
    size_t       &gpu_count,
    size_t const  max_devices,
    cg_handle_t  *gpu_handles
)
{
    gpu_count = 0;
    CG_CONTEXT *ctx = (CG_CONTEXT*) context;
    for (size_t i = 0, n = ctx->DeviceTable.ObjectCount; i < n; ++i)
    {
        CG_DEVICE &dev = ctx->DeviceTable.Objects[i];
        if (dev.Type  == CL_DEVICE_TYPE_GPU)
        {
            if (gpu_count < max_devices)
                gpu_handles[gpu_count] = cgMakeHandle(&ctx->DeviceTable, i);
            gpu_count++;
        }
    }
    return CG_SUCCESS;
}

/// @summary Retrieves all accelerator devices that can share resources with a given device.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param acl_count On return, stores the number of accelerator devices that can share resources with the specified device.
/// @param max_devices The maximum number of device handles to write to @a acl_handles.
/// @param acl_handles An array of device handles indicating the accelerator devices in the sharegroup.
/// @return CG_SUCCESS.
library_function int
cgGetAcceleratorDevices
(
    uintptr_t     context,
    size_t       &acl_count,
    size_t const  max_devices,
    cg_handle_t  *acl_handles
)
{
    acl_count = 0;
    CG_CONTEXT *ctx = (CG_CONTEXT*) context;
    for (size_t i = 0, n = ctx->DeviceTable.ObjectCount; i < n; ++i)
    {
        CG_DEVICE &dev = ctx->DeviceTable.Objects[i];
        if (dev.Type  == CL_DEVICE_TYPE_ACCELERATOR)
        {
            if (acl_count < max_devices)
                acl_handles[acl_count] = cgMakeHandle(&ctx->DeviceTable, i);
            acl_count++;
        }
    }
    return CG_SUCCESS;
}

/// @summary Retrieves all CPU devices that can share resources with a given device.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param group_device The handle of the device to check.
/// @param cpu_count On return, stores the number of CPU devices that can share resources with the specified device.
/// @param max_devices The maximum number of device handles to write to @a cpu_handles.
/// @param cpu_handles An array of device handles indicating the CPU devices in the sharegroup.
/// @return CG_SUCCESS or CG_INVALID_VALUE.
library_function int
cgGetCPUDevicesInShareGroup
(
    uintptr_t     context,
    cg_handle_t   group_device,
    size_t       &cpu_count,
    size_t const  max_devices,
    cg_handle_t  *cpu_handles
)
{
    cpu_count = 0;
    CG_CONTEXT *ctx = (CG_CONTEXT*) context;
    CG_DEVICE  *dev =  cgObjectTableGet(&ctx->DeviceTable, group_device);
    if (dev == NULL)   return CG_INVALID_VALUE;
    for (size_t i = 0, n = ctx->DeviceTable.ObjectCount; i < n; ++i)
    {
        CG_DEVICE *device  =&ctx->DeviceTable.Objects[i];
        if (device->Type  == CL_DEVICE_TYPE_CPU && device->PlatformId == dev->PlatformId && device != dev)
        {
            if (cpu_count < max_devices)
                cpu_handles[cpu_count] = cgMakeHandle(&ctx->DeviceTable, i);
            cpu_count++;
        }
    }
    return CG_SUCCESS;
}

/// @summary Retrieves all GPU devices that can share resources with a given device.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param group_device The handle of the device to check.
/// @param gpu_count On return, stores the number of GPU devices that can share resources with the specified device.
/// @param max_devices The maximum number of device handles to write to @a gpu_handles.
/// @param gpu_handles An array of device handles indicating the GPU devices in the sharegroup.
/// @return CG_SUCCESS or CG_INVALID_VALUE.
library_function int
cgGetGPUDevicesInShareGroup
(
    uintptr_t     context,
    cg_handle_t   group_device,
    size_t       &gpu_count,
    size_t const  max_devices,
    cg_handle_t  *gpu_handles
)
{
    gpu_count = 0;
    CG_CONTEXT *ctx = (CG_CONTEXT*) context;
    CG_DEVICE  *dev =  cgObjectTableGet(&ctx->DeviceTable, group_device);
    if (dev == NULL)   return CG_INVALID_VALUE;
    for (size_t i = 0, n = ctx->DeviceTable.ObjectCount; i < n; ++i)
    {
        CG_DEVICE *device  =&ctx->DeviceTable.Objects[i];
        if (device->Type  == CL_DEVICE_TYPE_GPU && device->PlatformId == dev->PlatformId && device != dev)
        {
            if (gpu_count < max_devices)
                gpu_handles[gpu_count] = cgMakeHandle(&ctx->DeviceTable, i);
            gpu_count++;
        }
    }
    return CG_SUCCESS;
}

/// @summary Create an execution group out of a set of devices.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param config A description of the execution group to create.
/// @param result On return, set to CG_SUCCESS or another result code.
library_function cg_handle_t
cgCreateExecutionGroup
(
    uintptr_t                   context,
    cg_execution_group_t const *config,
    int                        &result
)
{
    size_t       device_count = 0;
    size_t        queue_index = 0;
    cg_handle_t *devices      = NULL;
    cg_handle_t  group_handle = CG_INVALID_HANDLE;
    CG_CONTEXT  *ctx          =(CG_CONTEXT*)context;
    uint32_t     flags        = config->CreateFlags;
    // perform basic validation of the remaining configuration parameters.
    if (config->RootDevice == NULL)
    {   // the root device *must* be specified. it defines the share group ID.
        goto error_invalid_value;
    }
    if (config->DeviceCount > 0 && config->DeviceList == NULL)
    {   // no devices were specified in the device list.
        goto error_invalid_value;
    }
    if (config->ExtensionCount > 0 && config->ExtensionNames == NULL)
    {   // no extension name list was specified.
        goto error_invalid_value;
    }

    // construct the complete list of devices in the execution group.
    if ((devices = cgCreateExecutionGroupDeviceList(ctx, config->RootDevice, flags, config->DeviceList, config->DeviceCount, device_count, result)) == NULL)
    {   // result has been set to the reason for the failure.
        return CG_INVALID_HANDLE;
    }

    // initialize the execution group object.
    CG_EXEC_GROUP group;
    HDC           display_dc = NULL;
    if ((result = cgAllocExecutionGroup(ctx, &group, config->RootDevice, devices, device_count)) != CG_SUCCESS)
    {   // result has been set to the reason for the failure.
        cgFreeExecutionGroupDeviceList(ctx, devices, device_count);
        return CG_INVALID_HANDLE;
    }

    // if the group is to be used for display output, retrieve the rendering context.
    if (flags & CG_EXECUTION_GROUP_DISPLAY_OUTPUT)
    {   // find the GPU device and save off the rendering context.
        for (size_t i = 0; i < device_count; ++i)
        {
            if (group.DeviceList[i]->Type == CL_DEVICE_TYPE_GPU && group.DeviceList[i]->DisplayRC != NULL)
            {
                group.RenderingContext = group.DeviceList[i]->DisplayRC;
                group.AttachedDisplay  = group.DeviceList[i]->AttachedDisplays[0];
                display_dc             = group.DeviceList[i]->AttachedDisplays[0]->DisplayDC;
                break;
            }
        }
        if (group.RenderingContext == NULL)
        {   // display output is not available because there's no OpenGL context in the group.
            result = CG_NO_OPENGL;
            cgDeleteExecutionGroup(ctx, &group);
            cgFreeExecutionGroupDeviceList(ctx, devices, device_count);
            return CG_INVALID_HANDLE;
        }
    }
    else
    {   // if not used for display output, only OpenCL kernels may be executed.
        group.RenderingContext = NULL;
        display_dc             = NULL;
    }

    // create a single context that's shared between all devices. if OpenGL interop 
    // is required, we have to create the context with different properties.
    // NOTE(rlk): Intel OpenCL compatibility - 
    // - it seems that you cannot have any CPU devices in your share group
    // - it seems that you must make the GL context current on the thread
    cl_context_properties *props = NULL;
    cl_context_properties  props_glshare[] = 
    {
        (cl_context_properties) CL_CONTEXT_PLATFORM, 
        (cl_context_properties) group.PlatformId, 
        (cl_context_properties) CL_GL_CONTEXT_KHR, 
        (cl_context_properties) group.RenderingContext, 
        (cl_context_properties) CL_WGL_HDC_KHR, 
        (cl_context_properties) display_dc, 
        (cl_context_properties) 0
    };
    cl_context_properties  props_noshare[] =
    {
        (cl_context_properties) CL_CONTEXT_PLATFORM,
        (cl_context_properties) group.PlatformId,
        (cl_context_properties) 0
    };
    if (group.RenderingContext != NULL)
    {   // use the interop-capable context properties.
        props = &props_glshare[0];
        wglMakeCurrent(display_dc, group.RenderingContext);
    }
    else
    {   // use the standard, non-interop context properties.
        props = &props_noshare[0];
    }
    cl_int     cl_error = CL_SUCCESS;
    cl_context cl_ctx   = clCreateContext(props, cl_uint(group.DeviceCount), group.DeviceIds, NULL, NULL, &cl_error);
    if (cl_ctx == NULL)
    {   cgDeleteExecutionGroup(ctx, &group);
        cgFreeExecutionGroupDeviceList(ctx, devices, device_count);
        switch (cl_error)
        {
        case CL_DEVICE_NOT_AVAILABLE: result = CG_NO_CLCONTEXT;  break;
        case CL_OUT_OF_HOST_MEMORY  : result = CG_OUT_OF_MEMORY; break;
        default:                      result = CG_INVALID_VALUE; break;
        }
        return CG_INVALID_HANDLE;
    }
    
    // save the global OpenCL resource context reference:
    group.ComputeContext = cl_ctx;

    // create compute and transfer queues. each device gets its own unique
    // compute queue, and each GPU or accelerator device that doesn't have
    // its unified memory capability set gets its own transfer queue.
    for (size_t i = 0; i < device_count; ++i)
    {   // create the command queue used for submitting compute dispatch operations.
        // TODO(rlk): in OpenCL 2.0, clCreateCommandQueue is marked deprecated.
        // it has been replaced by clCreateCommandQueueWithProperties.
        cl_int       cl_err = CL_SUCCESS;
        cl_device_id cl_dev = group.DeviceIds[i];
        cl_command_queue cq = NULL; // compute queue
        cl_command_queue tq = NULL; // transfer-only queue
        cg_handle_t  cq_hnd = CG_INVALID_HANDLE;
        cg_handle_t  tq_hnd = CG_INVALID_HANDLE;
        CG_QUEUE    *cq_ref = NULL;
        CG_QUEUE    *tq_ref = NULL;
        cl_command_queue_properties flags  = CL_QUEUE_PROFILING_ENABLE;

        if (group.DeviceList[i]->Capabilities.CmdQueueConfig & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE)
        {   // prefer out-of-order execution if supported by the device.
            flags |= CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE;
        }

        if ((cq = clCreateCommandQueue(cl_ctx, cl_dev, flags, &cl_err)) == NULL)
        {   // unable to create the command queue for some reason.
            switch (cl_err)
            {
            case CL_INVALID_CONTEXT          : result = CG_BAD_CLCONTEXT; break;
            case CL_INVALID_DEVICE           : result = CG_BAD_CLCONTEXT; break;
            case CL_INVALID_VALUE            : result = CG_INVALID_VALUE; break;
            case CL_INVALID_QUEUE_PROPERTIES : result = CG_INVALID_VALUE; break;
            case CL_OUT_OF_RESOURCES         : result = CG_OUT_OF_MEMORY; break;
            case CL_OUT_OF_HOST_MEMORY       : result = CG_OUT_OF_MEMORY; break;
            default: result = CG_ERROR; break;
            }
            cgDeleteExecutionGroup(ctx, &group);
            cgFreeExecutionGroupDeviceList(ctx, devices, device_count);
            return CG_INVALID_HANDLE;
        }
        else
        {   // create the CG_QUEUE, add it to the table, save the reference.
            CG_QUEUE queue;
            queue.QueueType       = CG_QUEUE_TYPE_COMPUTE_OR_TRANSFER;
            queue.ComputeContext  = cl_ctx;
            queue.CommandQueue    = cq;
            queue.AttachedDisplay = group.AttachedDisplay;
            queue.DisplayDC       = NULL;
            queue.DisplayRC       = NULL;
            cq_hnd = cgObjectTableAdd(&ctx->QueueTable, queue);
            cq_ref = cgObjectTableGet(&ctx->QueueTable, cq_hnd);
            group.ComputeQueues[i]         = cq_ref;
            group.QueueList[queue_index++] = cq_ref;
        }

        // create the command queue used for submitting data transfer operations.
        if (group.DeviceList[i]->Capabilities.UnifiedMemory == CL_FALSE)
        {   // also create a transfer queue for the device.
            if ((tq = clCreateCommandQueue(cl_ctx, cl_dev, flags, &cl_err)) == NULL)
            {
                switch (cl_err)
                {
                case CL_INVALID_CONTEXT          : result = CG_BAD_CLCONTEXT; break;
                case CL_INVALID_DEVICE           : result = CG_BAD_CLCONTEXT; break;
                case CL_INVALID_VALUE            : result = CG_INVALID_VALUE; break;
                case CL_INVALID_QUEUE_PROPERTIES : result = CG_INVALID_VALUE; break;
                case CL_OUT_OF_RESOURCES         : result = CG_OUT_OF_MEMORY; break;
                case CL_OUT_OF_HOST_MEMORY       : result = CG_OUT_OF_MEMORY; break;
                default: result = CG_ERROR; break;
                }
                cgDeleteExecutionGroup(ctx, &group);
                cgFreeExecutionGroupDeviceList(ctx, devices, device_count);
                return CG_INVALID_HANDLE;
            }
            else
            {   // create the CG_QUEUE, add it to the table, save the reference.
                CG_QUEUE queue;
                queue.QueueType       = CG_QUEUE_TYPE_TRANSFER;
                queue.ComputeContext  = cl_ctx;
                queue.CommandQueue    = tq;
                queue.AttachedDisplay = group.AttachedDisplay;
                queue.DisplayDC       = NULL;
                queue.DisplayRC       = NULL;
                tq_hnd = cgObjectTableAdd(&ctx->QueueTable, queue);
                tq_ref = cgObjectTableGet(&ctx->QueueTable, tq_hnd);
                group.TransferQueues[i]        = tq_ref;
                group.QueueList[queue_index++] = tq_ref;
            }
        }
        else
        {   // use the existing compute queue for transfers, but create a distinct CG_QUEUE.
            // the compute queue object needs to be retained as it will be released twice.
            CG_QUEUE queue;
            queue.QueueType           = CG_QUEUE_TYPE_TRANSFER;
            queue.ComputeContext      = cl_ctx;
            queue.CommandQueue        = cq;
            queue.AttachedDisplay     = group.AttachedDisplay;
            queue.DisplayDC           = NULL;
            queue.DisplayRC           = NULL;
            tq_hnd = cgObjectTableAdd(&ctx->QueueTable, queue);
            tq_ref = cgObjectTableGet(&ctx->QueueTable, tq_hnd);
            group.TransferQueues[i]        = tq_ref;
            group.QueueList[queue_index++] = tq_ref;
            clRetainCommandQueue(cq);
        }
    }

    // create (logical) graphics queues for each attached display.
    for (size_t i = 0; i < group.DisplayCount; ++i)
    {
        if (group.AttachedDisplays[i]->DisplayRC != NULL)
        {
            CG_QUEUE    queue;
            CG_QUEUE   *queue_ref = NULL;
            cg_handle_t handle    = CG_INVALID_HANDLE;
            queue.QueueType       = CG_QUEUE_TYPE_GRAPHICS;
            queue.ComputeContext  = NULL;
            queue.CommandQueue    = NULL;
            queue.AttachedDisplay = group.AttachedDisplay;
            queue.DisplayDC       = group.AttachedDisplays[i]->DisplayDC;
            queue.DisplayRC       = group.AttachedDisplays[i]->DisplayRC;
            handle    = cgObjectTableAdd(&ctx->QueueTable, queue);
            queue_ref = cgObjectTableGet(&ctx->QueueTable, handle);
            group.GraphicsQueues[i]        = queue_ref;
            group.QueueList[queue_index++] = queue_ref;
        }
        else
        {   // there's no OpenGL available on this display, so no presentation queue.
            group.GraphicsQueues[i] = NULL;
        }
    }

    // if we created an interop-capable OpenCL context, deactivate the OpenGL context.
    if (group.RenderingContext != NULL)
    {
        wglMakeCurrent(NULL, NULL);
    }

    // insert the execution group into the object table.
    if ((group_handle = cgObjectTableAdd(&ctx->ExecGroupTable, group)) != CG_INVALID_HANDLE)
    {   // update each device in the group with the execution group handle.
        for (size_t i = 0; i < device_count; ++i)
        {
            group.DeviceList[i]->ExecutionGroup = group_handle;
        }
        result = CG_SUCCESS;
        return group_handle;
    }
    else
    {   // the object table is full. clean up.
        cgDeleteExecutionGroup(ctx, &group);
        cgFreeExecutionGroupDeviceList(ctx, devices, device_count);
        result = CG_OUT_OF_OBJECTS;
        return CG_INVALID_HANDLE;
    }

error_invalid_value:
    result = CG_INVALID_VALUE;
    return CG_INVALID_HANDLE;
}

/// @summary Query execution group information.
/// @param context A CGFX context returned by cgEnumerateDevices().
/// @param group_handle The handle of the execution group to query.
/// @param param One of cg_execution_group_info_param_e specifying the data to return.
/// @param buffer A caller-managed buffer to receive the data.
/// @param buffer_size The maximum number of bytes that can be written to @a buffer.
/// @param bytes_needed On return, stores the number of bytes copied to the buffer, or the number of bytes required to store the data.
/// @return CG_SUCCESS, CG_BUFFER_TOO_SMALL, CG_INVALID_VALUE or CG_OUT_OF_MEMORY.
library_function int
cgGetExecutionGroupInfo
(
    uintptr_t   context,
    cg_handle_t group_handle,
    int         param,
    void       *buffer,
    size_t      buffer_size,
    size_t     *bytes_needed
)
{
    CG_CONTEXT    *ctx   = (CG_CONTEXT*) context;
    CG_EXEC_GROUP *group =  NULL;

    if ((group = cgObjectTableGet(&ctx->ExecGroupTable, group_handle)) == NULL)
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
#define BUFFER_SET_SCALAR(type, value) \
    *((type*)buffer) = (value)
    // ==========================================================
    // ==========================================================

    // retrieve parameter data:
    switch (param)
    {
    case CG_EXEC_GROUP_DEVICE_COUNT:
        {   BUFFER_CHECK_TYPE(size_t);
            BUFFER_SET_SCALAR(size_t, group->DeviceCount);
        }
        return CG_SUCCESS;

    case CG_EXEC_GROUP_DEVICES:
        {   BUFFER_CHECK_SIZE(group->DeviceCount * sizeof(cg_handle_t));
            cg_handle_t *handles = (cg_handle_t*)  buffer;
            for (size_t i = 0, n = group->DeviceCount; i < n; ++i)
            {
                handles[i] = cgMakeHandle(group->DeviceList[i]->ObjectId, CG_OBJECT_DEVICE, CG_DEVICE_TABLE_ID);
            }
        }
        return CG_SUCCESS;

    case CG_EXEC_GROUP_CPU_COUNT:
        {   BUFFER_CHECK_TYPE(size_t);
            size_t num_devices = 0;
            for (size_t i = 0, n = group->DeviceCount; i < n; ++i)
            {
                if (group->DeviceList[i]->Type == CL_DEVICE_TYPE_CPU)
                    num_devices++;
            }
            BUFFER_SET_SCALAR(size_t, num_devices);
        }
        return CG_SUCCESS;

    case CG_EXEC_GROUP_CPU_DEVICES:
        {   size_t num_devices = 0;
            for (size_t i = 0, n = group->DeviceCount; i < n; ++i)
            {
                if (group->DeviceList[i]->Type == CL_DEVICE_TYPE_CPU)
                    num_devices++;
            }
            BUFFER_CHECK_SIZE(num_devices * sizeof(cg_handle_t));
            cg_handle_t *handles = (cg_handle_t*)  buffer;
            for (size_t i = 0, n = group->DeviceCount, o = 0; i < n; ++i)
            {
                if (group->DeviceList[i]->Type == CL_DEVICE_TYPE_CPU)
                    handles[o++] = cgMakeHandle(group->DeviceList[i]->ObjectId, CG_OBJECT_DEVICE, CG_DEVICE_TABLE_ID);
            }
        }
        return CG_SUCCESS;

    case CG_EXEC_GROUP_GPU_COUNT:
        {   BUFFER_CHECK_TYPE(size_t);
            size_t num_devices = 0;
            for (size_t i = 0, n = group->DeviceCount; i < n; ++i)
            {
                if (group->DeviceList[i]->Type == CL_DEVICE_TYPE_GPU)
                    num_devices++;
            }
            BUFFER_SET_SCALAR(size_t, num_devices);
        }
        return CG_SUCCESS;

    case CG_EXEC_GROUP_GPU_DEVICES:
        {   size_t num_devices = 0;
            for (size_t i = 0, n = group->DeviceCount; i < n; ++i)
            {
                if (group->DeviceList[i]->Type == CL_DEVICE_TYPE_GPU)
                    num_devices++;
            }
            BUFFER_CHECK_SIZE(num_devices * sizeof(cg_handle_t));
            cg_handle_t *handles = (cg_handle_t*)  buffer;
            for (size_t i = 0, n = group->DeviceCount, o = 0; i < n; ++i)
            {
                if (group->DeviceList[i]->Type == CL_DEVICE_TYPE_GPU)
                    handles[o++] = cgMakeHandle(group->DeviceList[i]->ObjectId, CG_OBJECT_DEVICE, CG_DEVICE_TABLE_ID);
            }
        }
        return CG_SUCCESS;

    case CG_EXEC_GROUP_ACCELERATOR_COUNT:
        {   BUFFER_CHECK_TYPE(size_t);
            size_t num_devices = 0;
            for (size_t i = 0, n = group->DeviceCount; i < n; ++i)
            {
                if (group->DeviceList[i]->Type == CL_DEVICE_TYPE_ACCELERATOR)
                    num_devices++;
            }
            BUFFER_SET_SCALAR(size_t, num_devices);
        }
        return CG_SUCCESS;

    case CG_EXEC_GROUP_ACCELERATOR_DEVICES:
        {   size_t num_devices = 0;
            for (size_t i = 0, n = group->DeviceCount; i < n; ++i)
            {
                if (group->DeviceList[i]->Type == CL_DEVICE_TYPE_ACCELERATOR)
                    num_devices++;
            }
            BUFFER_CHECK_SIZE(num_devices * sizeof(cg_handle_t));
            cg_handle_t *handles = (cg_handle_t*)  buffer;
            for (size_t i = 0, n = group->DeviceCount, o = 0; i < n; ++i)
            {
                if (group->DeviceList[i]->Type == CL_DEVICE_TYPE_ACCELERATOR)
                    handles[o++] = cgMakeHandle(group->DeviceList[i]->ObjectId, CG_OBJECT_DEVICE, CG_DEVICE_TABLE_ID);
            }
        }
        return CG_SUCCESS;

    case CG_EXEC_GROUP_DISPLAY_COUNT:
        {   BUFFER_CHECK_TYPE(size_t);
            BUFFER_SET_SCALAR(size_t, group->DisplayCount);
        }
        return CG_SUCCESS;

    case CG_EXEC_GROUP_ATTACHED_DISPLAYS:
        {   BUFFER_CHECK_SIZE(group->DisplayCount * sizeof(cg_handle_t));
            cg_handle_t *handles = (cg_handle_t*) buffer;
            for (size_t i = 0, n = group->DisplayCount; i < n; ++i)
            {
                handles[i] = cgMakeHandle(group->AttachedDisplays[i]->ObjectId, CG_OBJECT_DISPLAY, CG_DISPLAY_TABLE_ID);
            }
        }
        return CG_SUCCESS;

    case CG_EXEC_GROUP_QUEUE_COUNT:
        {   BUFFER_CHECK_TYPE(size_t);
            BUFFER_SET_SCALAR(size_t, group->QueueCount);
        }
        return CG_SUCCESS;

    case CG_EXEC_GROUP_QUEUES:
        {   BUFFER_CHECK_SIZE(group->QueueCount * sizeof(cg_handle_t));
            cg_handle_t *handles = (cg_handle_t*) buffer;
            for (size_t i = 0, n = group->QueueCount; i < n; ++i)
            {
                handles[i] = cgMakeHandle(group->QueueList[i]->ObjectId, CG_OBJECT_QUEUE, CG_QUEUE_TABLE_ID);
            }
        }
        return CG_SUCCESS;

    case CG_EXEC_GROUP_COMPUTE_QUEUE_COUNT:
        {   BUFFER_CHECK_TYPE(size_t);
            size_t num_queues = 0;
            for (size_t i = 0, n = group->QueueCount; i < n; ++i)
            {
                if (group->QueueList[i]->QueueType == CG_QUEUE_TYPE_COMPUTE)
                    num_queues++;
            }
            BUFFER_SET_SCALAR(size_t, num_queues);
        }
        return CG_SUCCESS;

    case CG_EXEC_GROUP_COMPUTE_QUEUES:
        {   size_t num_queues = 0;
            for (size_t i = 0, n = group->QueueCount; i < n; ++i)
            {
                if (group->QueueList[i]->QueueType == CG_QUEUE_TYPE_COMPUTE)
                    num_queues++;
            }
            BUFFER_CHECK_SIZE(num_queues * sizeof(cg_handle_t));
            cg_handle_t *handles = (cg_handle_t*) buffer;
            for (size_t i = 0, n = group->QueueCount, o = 0; i < n; ++i)
            {
                if (group->QueueList[i]->QueueType == CG_QUEUE_TYPE_COMPUTE)
                    handles[o++] = cgMakeHandle(group->QueueList[i]->ObjectId, CG_OBJECT_QUEUE, CG_QUEUE_TABLE_ID);
            }
        }
        return CG_SUCCESS;

    case CG_EXEC_GROUP_TRANSFER_QUEUE_COUNT:
        {   BUFFER_CHECK_TYPE(size_t);
            size_t num_queues = 0;
            for (size_t i = 0, n = group->QueueCount; i < n; ++i)
            {
                if (group->QueueList[i]->QueueType == CG_QUEUE_TYPE_TRANSFER)
                    num_queues++;
            }
            BUFFER_SET_SCALAR(size_t, num_queues);
        }
        return CG_SUCCESS;

    case CG_EXEC_GROUP_TRANSFER_QUEUES:
        {   size_t num_queues = 0;
            for (size_t i = 0, n = group->QueueCount; i < n; ++i)
            {
                if (group->QueueList[i]->QueueType == CG_QUEUE_TYPE_TRANSFER)
                    num_queues++;
            }
            BUFFER_CHECK_SIZE(num_queues * sizeof(cg_handle_t));
            cg_handle_t *handles = (cg_handle_t*) buffer;
            for (size_t i = 0, n = group->QueueCount, o = 0; i < n; ++i)
            {
                if (group->QueueList[i]->QueueType == CG_QUEUE_TYPE_TRANSFER)
                    handles[o++] = cgMakeHandle(group->QueueList[i]->ObjectId, CG_OBJECT_QUEUE, CG_QUEUE_TABLE_ID);
            }
        }
        return CG_SUCCESS;

    case CG_EXEC_GROUP_GRAPHICS_QUEUE_COUNT:
        {   BUFFER_CHECK_TYPE(size_t);
            size_t num_queues = 0;
            for (size_t i = 0, n = group->QueueCount; i < n; ++i)
            {
                if (group->QueueList[i]->QueueType == CG_QUEUE_TYPE_GRAPHICS)
                    num_queues++;
            }
            BUFFER_SET_SCALAR(size_t, num_queues);
        }
        return CG_SUCCESS;

    case CG_EXEC_GROUP_GRAPHICS_QUEUES:
        {   size_t num_queues = 0;
            for (size_t i = 0, n = group->QueueCount; i < n; ++i)
            {
                if (group->QueueList[i]->QueueType == CG_QUEUE_TYPE_GRAPHICS)
                    num_queues++;
            }
            BUFFER_CHECK_SIZE(num_queues * sizeof(cg_handle_t));
            cg_handle_t *handles = (cg_handle_t*) buffer;
            for (size_t i = 0, n = group->QueueCount, o = 0; i < n; ++i)
            {
                if (group->QueueList[i]->QueueType == CG_QUEUE_TYPE_GRAPHICS)
                    handles[o++] = cgMakeHandle(group->QueueList[i]->ObjectId, CG_OBJECT_QUEUE, CG_QUEUE_TABLE_ID);
            }
        }
        return CG_SUCCESS;

    case CG_EXEC_GROUP_CL_CONTEXT:
        {   BUFFER_CHECK_TYPE(cl_context);
            BUFFER_SET_SCALAR(cl_context, group->ComputeContext);
        }
        return CG_SUCCESS;

    case CG_EXEC_GROUP_WINDOWS_HGLRC:
        {   BUFFER_CHECK_TYPE(HGLRC);
            BUFFER_SET_SCALAR(HGLRC, group->RenderingContext);
        }
        return CG_SUCCESS;

    default:
        {
            if (bytes_needed != NULL) *bytes_needed = 0;
            return CG_INVALID_VALUE;
        }
    }

#undef  BUFFER_SET_SCALAR
#undef  BUFFER_CHECK_SIZE
#undef  BUFFER_CHECK_TYPE
}

/// @summary Retrieve a command queue associated with a device.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param device_handle The handle of the device to query.
/// @param queue_type One of cg_queue_type_e identifying the queue to retrieve.
/// @param result On return, set to CG_SUCCESS or another result code.
/// @return The handle of the queue, or CG_INVALID_HANDLE.
library_function cg_handle_t
cgGetQueueForDevice
(
    uintptr_t   context,
    cg_handle_t device_handle,
    int         queue_type,
    int        &result
)
{
    CG_CONTEXT *ctx = (CG_CONTEXT*) context;
    CG_DEVICE  *dev =  cgObjectTableGet(&ctx->DeviceTable, device_handle);
    if (dev == NULL)
    {   // an invalid device handle was supplied.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }
    CG_EXEC_GROUP *grp = cgObjectTableGet(&ctx->ExecGroupTable, dev->ExecutionGroup);
    if (grp == NULL)
    {   // there's no execution group associated with this device.
        result = CG_UNKNOWN_GROUP;
        return CG_INVALID_HANDLE;
    }
    if (queue_type == CG_QUEUE_TYPE_COMPUTE)
    {   // search the device list to retrieve the queue.
        for (size_t i = 0, n = grp->DeviceCount; i < n; ++i)
        {
            if (grp->DeviceList[i] == dev)
            {   result = CG_SUCCESS;
                return cgMakeHandle(grp->ComputeQueues[i]->ObjectId, CG_OBJECT_QUEUE, CG_QUEUE_TABLE_ID);
            }
        }
    }
    else if (queue_type == CG_QUEUE_TYPE_TRANSFER)
    {   // search the device list to retrieve the queue.
        for (size_t i = 0, n = grp->DeviceCount; i < n; ++i)
        {
            if (grp->DeviceList[i] == dev)
            {   result = CG_SUCCESS;
                return cgMakeHandle(grp->TransferQueues[i]->ObjectId, CG_OBJECT_QUEUE, CG_QUEUE_TABLE_ID);
            }
        }
    }
    else if (queue_type == CG_QUEUE_TYPE_GRAPHICS)
    {   // search the display list for the device.
        for (size_t i = 0, n = grp->DisplayCount; i < n; ++i)
        {
            if (grp->AttachedDisplays[i]->DisplayDevice == dev)
            {
                if (grp->GraphicsQueues[i] != NULL)
                {   result = CG_SUCCESS;
                    return cgMakeHandle(grp->GraphicsQueues[i]->ObjectId, CG_OBJECT_QUEUE, CG_QUEUE_TABLE_ID);
                }
                else break; // no presentation queue because required OpenGL is not available.
            }
        }
    }
    result = CG_NO_QUEUE_OF_TYPE;
    return CG_INVALID_HANDLE;
}

/// @summary Retrieve a command queue associated with a display.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param display_handle The handle of the display to query.
/// @param queue_type One of cg_queue_type_e identifying the queue to retrieve.
/// @param result On return, set to CG_SUCCESS or another result code.
/// @return The handle of the queue, or CG_INVALID_HANDLE.
library_function cg_handle_t
cgGetQueueForDisplay
(
    uintptr_t   context,
    cg_handle_t display_handle,
    int         queue_type,
    int        &result
)
{
    CG_CONTEXT *ctx = (CG_CONTEXT*) context;
    CG_DISPLAY *dsp =  cgObjectTableGet(&ctx->DisplayTable, display_handle);
    if (dsp == NULL)
    {   // an invalid display handle was supplied.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }
    CG_DEVICE     *dev = dsp->DisplayDevice;
    CG_EXEC_GROUP *grp = cgObjectTableGet(&ctx->ExecGroupTable, dev->ExecutionGroup);
    if (grp == NULL)
    {   // there's no execution group associated with this device.
        result = CG_UNKNOWN_GROUP;
        return CG_INVALID_HANDLE;
    }
    if (queue_type == CG_QUEUE_TYPE_COMPUTE)
    {   // search the device list to retrieve the queue.
        for (size_t i = 0, n = grp->DeviceCount; i < n; ++i)
        {
            if (grp->DeviceList[i] == dev)
            {   result = CG_SUCCESS;
                return cgMakeHandle(grp->ComputeQueues[i]->ObjectId, CG_OBJECT_QUEUE, CG_QUEUE_TABLE_ID);
            }
        }
    }
    else if (queue_type == CG_QUEUE_TYPE_TRANSFER)
    {   // search the device list to retrieve the queue.
        for (size_t i = 0, n = grp->DeviceCount; i < n; ++i)
        {
            if (grp->DeviceList[i] == dev)
            {   result = CG_SUCCESS;
                return cgMakeHandle(grp->TransferQueues[i]->ObjectId, CG_OBJECT_QUEUE, CG_QUEUE_TABLE_ID);
            }
        }
    }
    else if (queue_type == CG_QUEUE_TYPE_GRAPHICS)
    {   // search the display list for the device.
        for (size_t i = 0, n = grp->DisplayCount; i < n; ++i)
        {
            if (grp->AttachedDisplays[i] == dsp)
            {
                if (grp->GraphicsQueues[i] != NULL)
                {   result = CG_SUCCESS;
                    return cgMakeHandle(grp->GraphicsQueues[i]->ObjectId, CG_OBJECT_QUEUE, CG_QUEUE_TABLE_ID);
                }
                else break; // no presentation queue because required OpenGL is not available.
            }
        }
    }
    result = CG_NO_QUEUE_OF_TYPE;
    return CG_INVALID_HANDLE;
}

/// @summary Deletes an object and invalidates its handle.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param object The handle of the object to delete.
/// @return CG_SUCCESS or CG_INVALID_VALUE.
library_function int
cgDeleteObject
(
    uintptr_t   context,
    cg_handle_t object
)
{
    CG_CONTEXT *ctx = (CG_CONTEXT*) context;
    switch (cgGetObjectType(object))
    {
    case CG_OBJECT_COMMAND_BUFFER:
        {
            CG_CMD_BUFFER buf;
            if (cgObjectTableRemove(&ctx->CmdBufferTable, object, buf))
            {
                cgDeleteCmdBuffer(ctx, &buf);
                return CG_SUCCESS;
            }
        }
        break;

    case CG_OBJECT_KERNEL:
        {
            CG_KERNEL kernel;
            if (cgObjectTableRemove(&ctx->KernelTable, object, kernel))
            {
                cgDeleteKernel(ctx, &kernel);
                return CG_SUCCESS;
            }
        }
        break;

    case CG_OBJECT_PIPELINE:
        {
            CG_PIPELINE pipeline;
            if (cgObjectTableRemove(&ctx->PipelineTable, object, pipeline))
            {
                cgDeletePipeline(ctx, &pipeline);
                return CG_SUCCESS;
            }
        }
        break;

    case CG_OBJECT_BUFFER:
        {
            CG_BUFFER buffer;
            if (cgObjectTableRemove(&ctx->BufferTable, object, buffer))
            {
                cgDeleteBuffer(ctx, &buffer);
                return CG_SUCCESS;
            }
        }
        break;

    case CG_OBJECT_FENCE:
        {
            CG_FENCE fence;
            if (cgObjectTableRemove(&ctx->FenceTable, object, fence))
            {
                cgDeleteFence(ctx, &fence);
                return CG_SUCCESS;
            }
        }
        break;

    case CG_OBJECT_EVENT:
        {
            CG_EVENT event;
            if (cgObjectTableRemove(&ctx->EventTable, object, event))
            {
                cgDeleteEvent(ctx, &event);
                return CG_SUCCESS;
            }
        }
        break;

    case CG_OBJECT_IMAGE:
        {
            CG_IMAGE image;
            if (cgObjectTableRemove(&ctx->ImageTable, object, image))
            {
                cgDeleteImage(ctx, &image);
                return CG_SUCCESS;
            }
        }
        break;

    case CG_OBJECT_SAMPLER:
        {
            CG_SAMPLER sampler;
            if (cgObjectTableRemove(&ctx->SamplerTable, object, sampler))
            {
                cgDeleteSampler(ctx, &sampler);
                return CG_SUCCESS;
            }
        }
        break;

    case CG_OBJECT_VERTEX_DATA_SOURCE:
        {
            CG_VERTEX_DATA_SOURCE source;
            if (cgObjectTableRemove(&ctx->VertexSourceTable, object, source))
            {
                cgDeleteVertexDataSource(ctx, &source);
                return CG_SUCCESS;
            }
        }
        break;

    default:
        break;
    }
    return CG_INVALID_VALUE;
}

/// @summary Allocates and initializes a new command buffer.
/// @param context A CGFX context returned from cgEnumerateDevices.
/// @param queue_type One of cg_queue_type_e specifying the destination queue type.
/// @param result On return, set to CG_SUCCESS, CG_OUT_OF_MEMORY or CG_OUT_OF_OBJECTS.
library_function cg_handle_t
cgCreateCommandBuffer
(
    uintptr_t  context,
    int        queue_type,
    int       &result
)
{
    CG_CONTEXT   *ctx = (CG_CONTEXT*) context;
    CG_CMD_BUFFER buf;

    cgCmdBufferSetTypeAndState(&buf, queue_type, CG_CMD_BUFFER::UNINITIALIZED);
    buf.BytesTotal       =  0;
    buf.BytesUsed        =  0;
    buf.CommandCount     =  0;
    if ((buf.CommandData = (uint8_t*) VirtualAlloc(NULL, CG_CMD_BUFFER::MAX_SIZE, MEM_RESERVE, PAGE_READWRITE)) == NULL)
    {   // unable to reserve the required virtual address space.
        result = CG_OUT_OF_MEMORY;
        return CG_INVALID_HANDLE;
    }
    cg_handle_t cmdbuf = cgObjectTableAdd(&ctx->CmdBufferTable, buf);
    if (cmdbuf == CG_INVALID_HANDLE)
    {   // the object table is full. free resources.
        VirtualFree(buf.CommandData, 0, MEM_RELEASE);
        result = CG_OUT_OF_OBJECTS;
        return CG_INVALID_HANDLE;
    }
    result = CG_SUCCESS;
    return cmdbuf;
}

/// @summary Prepares a command buffer for writing.
/// @param context A CGFX context returned from cgEnumerateDevices.
/// @param cmd_buffer The handle of the command buffer.
/// @param flags Flags used to optimize command buffer submission.
/// @return CG_SUCCESS, CG_INVALID_VALUE or CG_INVALID_STATE.
library_function int
cgBeginCommandBuffer
(
    uintptr_t   context,
    cg_handle_t cmd_buffer,
    uint32_t    flags
)
{   UNREFERENCED_PARAMETER(flags);
    CG_CONTEXT    *ctx    =(CG_CONTEXT*) context;
    CG_CMD_BUFFER *cmdbuf = cgObjectTableGet(&ctx->CmdBufferTable, cmd_buffer);
    if (cmdbuf == NULL)
    {   // an invalid handle was supplied.
        return CG_INVALID_VALUE;
    }
    uint32_t state = cgCmdBufferGetState(cmdbuf);
    if (state != CG_CMD_BUFFER::UNINITIALIZED &&
        state != CG_CMD_BUFFER::SUBMIT_READY)
    {   // the command buffer is in an invalid state for this call.
        return CG_INVALID_STATE;
    }
    cmdbuf->BytesUsed     = 0;
    cmdbuf->CommandCount  = 0;
    cgCmdBufferSetState(cmdbuf, CG_CMD_BUFFER::BUILDING);
    return CG_SUCCESS;
}

/// @summary Resets a command buffer regardless of its current state.
/// @param context A CGFX context returned from cgEnumerateDevices.
/// @param cmd_buffer The handle of the command buffer to reset.
/// @return CG_SUCCESS or CG_INVALID_VALUE.
library_function int
cgResetCommandBuffer
(
    uintptr_t   context,
    cg_handle_t cmd_buffer
)
{
    CG_CONTEXT    *ctx    =(CG_CONTEXT*) context;
    CG_CMD_BUFFER *cmdbuf = cgObjectTableGet(&ctx->CmdBufferTable, cmd_buffer);
    if (cmdbuf == NULL)
    {   // an invalid handle was supplied.
        return CG_INVALID_VALUE;
    }
    cmdbuf->BytesUsed     = 0;
    cmdbuf->CommandCount  = 0;
    cgCmdBufferSetState(cmdbuf, CG_CMD_BUFFER::UNINITIALIZED);
    return CG_SUCCESS;
}

/// @summary Appends a fixed-length command to a command buffer.
/// @param context A CGFX context returned from cgEnumerateDevices.
/// @param cmd_buffer The handle of the command buffer to write.
/// @param cmd_type The command identifier.
/// @param data_size The size of the command data, in bytes. The maximum size is 64KB.
/// @param cmd_data The command data to copy to the command buffer.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_INVALID_STATE, CG_BUFFER_TOO_SMALL or CG_OUT_OF_MEMORY.
library_function int
cgCommandBufferAppend
(
    uintptr_t   context,
    cg_handle_t cmd_buffer,
    uint16_t    cmd_type,
    size_t      data_size,
    void const *cmd_data
)
{
    if (data_size > CG_CMD_BUFFER::MAX_CMD_SIZE)
    {   // this command is too large.
        return CG_INVALID_VALUE;
    }
    CG_CONTEXT    *ctx    =(CG_CONTEXT*) context;
    CG_CMD_BUFFER *cmdbuf = cgObjectTableGet(&ctx->CmdBufferTable, cmd_buffer);
    if (cmdbuf == NULL)
    {   // an invalid handle was supplied.
        return CG_INVALID_VALUE;
    }
    uint32_t state = cgCmdBufferGetState(cmdbuf);
    if (state != CG_CMD_BUFFER::BUILDING)
    {   // the command buffer is in an invalid state for this call.
        return CG_INVALID_STATE;
    }
    size_t total_size = CG_CMD_BUFFER::CMD_HEADER_SIZE + data_size;
    if (cmdbuf->BytesUsed + total_size > CG_CMD_BUFFER::MAX_SIZE)
    {   // the command buffer is too large.
        cgCmdBufferSetState(cmdbuf, CG_CMD_BUFFER::INCOMPLETE);
        return CG_BUFFER_TOO_SMALL;
    }
    if (cmdbuf->BytesUsed + total_size > cmdbuf->BytesTotal)
    {   // commit additional address space.
        size_t cs  = align_up(cmdbuf->BytesUsed + total_size, CG_CMD_BUFFER::ALLOCATION_GRANULARITY);
        void *buf  = VirtualAlloc(cmdbuf->CommandData, cmdbuf->BytesUsed+total_size, MEM_COMMIT, PAGE_READWRITE);
        if   (buf == NULL)
        {   // unable to commit the additional address space.
            cgCmdBufferSetState(cmdbuf, CG_CMD_BUFFER::INCOMPLETE);
            return CG_OUT_OF_MEMORY;
        }
        cmdbuf->BytesTotal  =  cs;
        cmdbuf->CommandData = (uint8_t*) buf;
    }
    cg_command_t *cmd  = (cg_command_t*) (cmdbuf->CommandData + cmdbuf->BytesUsed);
    cmd->CommandId     =  cmd_type;
    cmd->DataSize      = (uint16_t) data_size;
    memcpy(cmd->Data, cmd_data, data_size);
    cmdbuf->BytesUsed +=  total_size;
    cmdbuf->CommandCount++;
    return CG_SUCCESS;
}

/// @summary Begins writing a variable-length command.
/// @param context A CGFX context returned from cgEnumerateDevices.
/// @param cmd_buffer The handle of the command buffer to modify.
/// @param reserve_size The maximum number of data bytes that will be written for the command.
/// @param command On return, points to the command structure to populate.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_INVALID_STATE, CG_BUFFER_TOO_SMALL or CG_OUT_OF_MEMORY.
library_function int
cgCommandBufferMapAppend
(
    uintptr_t      context,
    cg_handle_t    cmd_buffer,
    size_t         reserve_size,
    cg_command_t **command
)
{
    CG_CONTEXT    *ctx    =(CG_CONTEXT*) context;
    CG_CMD_BUFFER *cmdbuf = cgObjectTableGet(&ctx->CmdBufferTable, cmd_buffer);
    if (cmdbuf == NULL)
    {   // an invalid handle was supplied.
        return CG_INVALID_VALUE;
    }
    uint32_t state = cgCmdBufferGetState(cmdbuf);
    if (state != CG_CMD_BUFFER::BUILDING)
    {   // the command buffer is in an invalid state for this call.
        return CG_INVALID_STATE;
    }
    size_t total_size = CG_CMD_BUFFER::CMD_HEADER_SIZE + reserve_size;
    if (cmdbuf->BytesUsed + total_size > CG_CMD_BUFFER::MAX_SIZE)
    {   // the command is too large.
        cgCmdBufferSetState(cmdbuf, CG_CMD_BUFFER::INCOMPLETE);
        return CG_BUFFER_TOO_SMALL;
    }
    if (cmdbuf->BytesUsed + total_size > cmdbuf->BytesTotal)
    {   // commit additional address space.
        size_t cs  = align_up(cmdbuf->BytesUsed + total_size, CG_CMD_BUFFER::ALLOCATION_GRANULARITY);
        void *buf  = VirtualAlloc(cmdbuf->CommandData, cmdbuf->BytesUsed+total_size, MEM_COMMIT, PAGE_READWRITE);
        if   (buf == NULL)
        {   // unable to commit the additional address space.
            cgCmdBufferSetState(cmdbuf, CG_CMD_BUFFER::INCOMPLETE);
            return CG_OUT_OF_MEMORY;
        }
        cmdbuf->BytesTotal  =  cs;
        cmdbuf->CommandData = (uint8_t*) buf;
    }
    cgCmdBufferSetState(cmdbuf, CG_CMD_BUFFER::MAP_APPEND);
    *command = (cg_command_t*) (cmdbuf->CommandData + cmdbuf->BytesUsed);
    return CG_SUCCESS;
}

/// @summary Finishes writing a variable-length command.
/// @param context A CGFX context returned from cgEnumerateDevices.
/// @param cmd_buffer The handle of the command buffer.
/// @param data_written The number of data bytes written for the command.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_INVALID_STATE or CG_BUFFER_TOO_SMALL.
library_function int
cgCommandBufferUnmapAppend
(
    uintptr_t      context,
    cg_handle_t    cmd_buffer,
    size_t         data_written
)
{
    CG_CONTEXT    *ctx    =(CG_CONTEXT*) context;
    CG_CMD_BUFFER *cmdbuf = cgObjectTableGet(&ctx->CmdBufferTable, cmd_buffer);
    if (cmdbuf == NULL)
    {   // an invalid handle was supplied.
        return CG_INVALID_VALUE;
    }
    uint32_t state = cgCmdBufferGetState(cmdbuf);
    if (state != CG_CMD_BUFFER::MAP_APPEND)
    {   // the command buffer is in an invalid state for this call.
        return CG_INVALID_STATE;
    }
    size_t total_size = CG_CMD_BUFFER::CMD_HEADER_SIZE + data_written;
    if (cmdbuf->BytesUsed + total_size > cmdbuf->BytesTotal)
    {   // something seriously wrong; the user wrote past the end of the buffer.
        // user must use cgCommandBufferReset to fix.
        cgCmdBufferSetState(cmdbuf, CG_CMD_BUFFER::INCOMPLETE);
        return CG_BUFFER_TOO_SMALL;
    }
    cgCmdBufferSetState(cmdbuf, CG_CMD_BUFFER::BUILDING);
    cmdbuf->BytesUsed += total_size;
    cmdbuf->CommandCount++;
    return CG_SUCCESS;
}

/// @summary Indicates completion of command buffer construction.
/// @param context A CGFX context returned from cgEnumerateDevices.
/// @param cmd_buffer The handle of the command buffer.
/// @return CG_SUCCESS, CG_INVALID_VALUE or CG_INVALID_STATE.
library_function int
cgEndCommandBuffer
(
    uintptr_t   context,
    cg_handle_t cmd_buffer
)
{
    CG_CONTEXT    *ctx    =(CG_CONTEXT*) context;
    CG_CMD_BUFFER *cmdbuf = cgObjectTableGet(&ctx->CmdBufferTable, cmd_buffer);
    if (cmdbuf == NULL)
    {   // an invalid handle was supplied.
        return CG_INVALID_VALUE;
    }
    uint32_t state = cgCmdBufferGetState(cmdbuf);
    if (state != CG_CMD_BUFFER::BUILDING)
    {   // the command buffer is in an invalid state for this call.
        return CG_INVALID_STATE;
    }
    cgCmdBufferSetState(cmdbuf, CG_CMD_BUFFER::SUBMIT_READY);
    return CG_SUCCESS;
}

/// @summary Determine whether a command buffer is in a readable state.
/// @param context A CGFX context returned from cgEnumerateDevices.
/// @param cmd_buffer The handle of the command buffer to query.
/// @param bytes_total On return, set to the number of bytes used in the command buffer.
/// @return CG_SUCCESS, CG_INVALID_VALUE or CG_INVALID_STATE.
library_function int
cgCommandBufferCanRead
(
    uintptr_t   context,
    cg_handle_t cmd_buffer,
    size_t     &bytes_total
)
{
    CG_CONTEXT    *ctx    =(CG_CONTEXT*) context;
    CG_CMD_BUFFER *cmdbuf = cgObjectTableGet(&ctx->CmdBufferTable, cmd_buffer);
    if (cmdbuf == NULL)
    {   // an invalid handle was supplied.
        bytes_total = 0;
        return CG_INVALID_VALUE;
    }
    uint32_t state = cgCmdBufferGetState(cmdbuf);
    if (state != CG_CMD_BUFFER::SUBMIT_READY)
    {   // the command buffer is in an invalid state for this call.
        bytes_total = 0;
        return CG_INVALID_STATE;
    }
    bytes_total = cmdbuf->BytesUsed;
    return CG_SUCCESS;
}

/// @summary Retrieve the command located at a specified byte offset.
/// @param context A CGFX context returned from cgEnumerateDevices.
/// @param cmd_buffer The handle of the command buffer to read.
/// @param cmd_offset The byte offset of the command to read. On return, this is set to the byte offset of the next command.
/// @param result On return, set to CG_SUCCESS or CG_END_OF_BUFFER.
/// @return A pointer to the command, or NULL.
library_function cg_command_t*
cgCommandBufferCommandAt
(
    uintptr_t   context,
    cg_handle_t cmd_buffer,
    size_t     &cmd_offset,
    int        &result
)
{
    CG_CONTEXT    *ctx    =(CG_CONTEXT*) context;
    CG_CMD_BUFFER *cmdbuf = cgObjectTableGet(&ctx->CmdBufferTable, cmd_buffer);
    if (cmd_offset >= cmdbuf->BytesUsed)
    {   // the offset is invalid.
        result = CG_END_OF_BUFFER;
        return NULL;
    }
    cg_command_t *cmd = (cg_command_t*) (cmdbuf->CommandData + cmd_offset);
    cmd_offset += cmd->DataSize + CG_CMD_BUFFER::CMD_HEADER_SIZE;
    result = CG_SUCCESS;
    return cmd;
}

/// @summary Create a new fence object.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param exec_group The execution group managing the queues that will pass the fence.
/// @param queue_type One of cg_queue_type_e specifying the type of queue associated with the fence.
/// @param result On return, set to CG_SUCCESS, CG_INVALID_VALUE or CG_OUT_OF_OBJECTS.
/// @return A handle to the new fence object, or CG_INVALID_HANDLE.
library_function cg_handle_t
cgCreateFence
(
    uintptr_t   context, 
    cg_handle_t exec_group, 
    int         queue_type, 
    int        &result
)
{
    CG_CONTEXT    *ctx = (CG_CONTEXT*) context;
    CG_EXEC_GROUP *group = cgObjectTableGet(&ctx->ExecGroupTable, exec_group);
    if (group == NULL)
    {   // an invalid execution group was specified.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }

    CG_FENCE fence;
    fence.QueueType       = queue_type;
    fence.ComputeFence    = NULL;
    fence.GraphicsFence   = NULL;
    fence.LinkedEvent     = CG_INVALID_HANDLE;
    fence.AttachedDisplay = group->AttachedDisplay;
    cg_handle_t handle    = cgObjectTableAdd(&ctx->FenceTable, fence);
    if (handle == CG_INVALID_HANDLE)
    {   // the fence table is full.
        result = CG_OUT_OF_OBJECTS;
        return CG_INVALID_HANDLE;
    }
    return handle;
}

/// @summary Create a new graphics queue fence that becomes passable when a specified compute or transfer event becomes signaled.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param exec_group The execution group managing the graphics queue that will pass the fence and the linked event.
/// @param event_handle The handle of the linked compute or transfer queue event. When this event becomes signaled, the graphics queue may pass the fence.
/// @param result On return, set to CG_SUCCESS, CG_INVALID_VALUE or CG_OUT_OF_OBJECTS.
/// @return A handle to the new fence object, or CG_INVALID_HANDLE.
library_function cg_handle_t
cgCreateFenceForEvent
(
    uintptr_t   context, 
    cg_handle_t exec_group, 
    cg_handle_t event_handle, 
    int        &result
)
{
    CG_CONTEXT    *ctx = (CG_CONTEXT*) context;
    CG_EXEC_GROUP *group = cgObjectTableGet(&ctx->ExecGroupTable, exec_group);
    if (group == NULL)
    {   // an invalid execution group was specified.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }
    CG_EVENT *ev = cgObjectTableGet(&ctx->EventTable, event_handle);
    if (ev == NULL)
    {   // an invalid event handle was specified.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }

    CG_FENCE fence;
    fence.QueueType       = CG_QUEUE_TYPE_GRAPHICS;
    fence.ComputeFence    = NULL;
    fence.GraphicsFence   = NULL;
    fence.LinkedEvent     = event_handle;
    fence.AttachedDisplay = group->AttachedDisplay;
    cg_handle_t handle    = cgObjectTableAdd(&ctx->FenceTable, fence);
    if (handle == CG_INVALID_HANDLE)
    {   // the fence table is full.
        result = CG_OUT_OF_OBJECTS;
        return CG_INVALID_HANDLE;
    }
    return handle;
}

/// @summary Create a new event object in the unsignaled state.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param exec_group The execution group that will signal the event.
/// @param result On return, set to CG_SUCCESS, CG_INVALID_VALUE or CG_OUT_OF_OBJECTS.
/// @return A handle to the new event object, or CG_INVALID_HANDLE.
library_function cg_handle_t
cgCreateEvent
(
    uintptr_t   context, 
    cg_handle_t exec_group,
    int        &result
)
{
    CG_CONTEXT    *ctx = (CG_CONTEXT*) context;
    CG_EXEC_GROUP *group = cgObjectTableGet(&ctx->ExecGroupTable, exec_group);
    if (group == NULL)
    {   // an invalid execution group was specified.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }

    CG_EVENT evt;
    evt.ComputeEvent    = NULL;
    cg_handle_t handle  = cgObjectTableAdd(&ctx->EventTable, evt);
    if (handle == CG_INVALID_HANDLE)
    {   // the event object table is full.
        result = CG_OUT_OF_OBJECTS;
        return CG_INVALID_HANDLE;
    }
    return handle;
}

/// @summary Blocks the calling host thread until a device event becomes signaled. The command buffer that causes the event to become signaled must be submitted to a command queue before calling this function.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param wait_handle The handle of the event object to wait on.
/// @return CG_SUCCESS, CG_INVALID_VALUE or CG_INVALID_STATE.
library_function int
cgHostWaitForEvent
(
    uintptr_t   context, 
    cg_handle_t wait_handle
)
{
    CG_CONTEXT *ctx = (CG_CONTEXT*) context;
    CG_EVENT   *evt =  cgObjectTableGet(&ctx->EventTable, wait_handle);
    if (evt == NULL)   return CG_INVALID_VALUE;
    if (evt->ComputeEvent == NULL) return CG_INVALID_STATE;
    cl_int clres = clWaitForEvents(1, &evt->ComputeEvent);
    return clres == CL_SUCCESS ? CG_SUCCESS : CG_INVALID_VALUE;
}

/// @summary Buffer a command that blocks execution of subsequent commands until all prior commands have completed.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param cmd_buffer The handle of the destination command buffer.
/// @param fence_handle The handle of the fence object.
/// @param done_event The handle of the event object to signal when the fence has been passed, or CG_INVALID_HANDLE. This is useful if the host needs to wait until the fence is passed.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_INVALID_STATE or another result code.
library_function int
cgDeviceFence
(
    uintptr_t   context, 
    cg_handle_t cmd_buffer, 
    cg_handle_t fence_handle, 
    cg_handle_t done_event
)
{
    int result = CG_SUCCESS;

    if (fence_handle  == CG_INVALID_HANDLE)
        return CG_INVALID_VALUE;

    cg_command_t *cmd      = NULL;
    size_t const  cmd_size = sizeof(cg_device_fence_cmd_base_t);
    if ((result = cgCommandBufferMapAppend(context, cmd_buffer, cmd_size, &cmd)) != CG_SUCCESS)
        return result;

    cg_device_fence_cmd_data_t *bdp = (cg_device_fence_cmd_data_t*) cmd->Data;
    cmd->CommandId     = CG_COMMAND_DEVICE_FENCE;
    cmd->DataSize      = uint16_t(cmd_size);
    bdp->CompleteEvent = done_event;
    bdp->FenceObject   = fence_handle;
    bdp->WaitListCount = 0;
    return cgCommandBufferUnmapAppend(context, cmd_buffer, cmd_size);
}

/// @summary Buffer a command that blocks execution of subsequent commands until a specific set of commands have completed. This is only supported for compute and transfer queues.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param cmd_buffer The handle of the destination command buffer.
/// @param fence_handle The handle of the fence object.
/// @param wait_count The number of event handles specified in the @a wait_handles list.
/// @param wait_handles An array of @a wait_count event handles to wait on before allowing the fence to be passed.
/// @param done_event The handle of the event object to signal when the fence has been passed, or CG_INVALID_HANDLE. This is useful if the host needs to wait until the fence is passed.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_INVALID_STATE or another result code.
library_function int
cgDeviceFenceWithWaitList
(
    uintptr_t          context, 
    cg_handle_t        cmd_buffer,
    cg_handle_t        fence_handle, 
    size_t             wait_count, 
    cg_handle_t const *wait_handles,
    cg_handle_t        done_event
)
{
    CG_CONTEXT *ctx    =(CG_CONTEXT*) context;
    int         result = CG_SUCCESS;

    if (fence_handle  == CG_INVALID_HANDLE)
        return CG_INVALID_VALUE;
    if (wait_count > 0 && wait_handles == NULL)
        return CG_INVALID_VALUE;

    CG_CMD_BUFFER *cmdbuf = cgObjectTableGet(&ctx->CmdBufferTable, cmd_buffer);
    if (cmdbuf == NULL || cgCmdBufferGetQueueType(cmdbuf) == CG_QUEUE_TYPE_GRAPHICS)
        return CG_INVALID_VALUE;

    cg_command_t *cmd      = NULL;
    size_t const  cmd_size = sizeof(cg_device_fence_cmd_base_t) + (wait_count * sizeof(cg_handle_t));
    if ((result = cgCommandBufferMapAppend(context, cmd_buffer, cmd_size, &cmd)) != CG_SUCCESS)
        return result; // wait lists are not supported on in-order queues.

    cg_device_fence_cmd_data_t *bdp = (cg_device_fence_cmd_data_t*) cmd->Data;
    cmd->CommandId      = CG_COMMAND_DEVICE_FENCE;
    cmd->DataSize       = uint16_t(cmd_size);
    bdp->CompleteEvent  = done_event;
    bdp->FenceObject    = fence_handle;
    bdp->WaitListCount  = wait_count;
    memcpy(bdp->WaitList, wait_handles, wait_count * sizeof(cg_handle_t));
    return cgCommandBufferUnmapAppend(context, cmd_buffer, cmd_size);
}

/// @summary Create a vertex data source object that can be used to configure the input assembler stage of a graphics pipeline.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param exec_group The handle of the execution group that owns the layout object.
/// @param num_buffers The number of data buffers expected to be used for providing vertex data.
/// @param buffer_list An array of @a num_buffers data buffer object handles specifying the data buffers that contain the vertex data.
/// @param index_buffer A data buffer object handle specifying the data buffer that contains primitive indices, or CG_INVALID_HANDLE if indexed rendering is not used.
/// @param attrib_counts An array of @a num_buffers counts, each specifying the number of vertex attributes in each buffer.
/// @param attrib_data An array of @a num_buffers arrays, each specifying the vertex attribute definitions for the corresponding buffer.
/// @param result On return, set to CG_SUCCESS, CG_INVALID_VALUE, ...
library_function cg_handle_t
cgCreateVertexDataSource
(
    uintptr_t                     context,
    cg_handle_t                   exec_group, 
    size_t                        num_buffers, 
    cg_handle_t                  *buffer_list,
    cg_handle_t                   index_buffer,
    size_t const                 *attrib_counts, 
    cg_vertex_attribute_t const **attrib_data,
    int                          &result
)
{
    CG_CONTEXT    *ctx   = (CG_CONTEXT*) context;
    CG_EXEC_GROUP *group =  cgObjectTableGet(&ctx->ExecGroupTable, exec_group);
    if (group == NULL)
    {   // an invalid execution group was specified.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }
    if (num_buffers > 16)
    {   // implementations typically only support 16 vertex attributes.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }

    // retrieve pointers to all of the referenced buffer objects.
    CG_BUFFER *indexbuf = NULL;
    if (index_buffer != CG_INVALID_HANDLE)
    {   // indexed rendering will be used; retrieve the index buffer object.
        if ((indexbuf = cgObjectTableGet(&ctx->BufferTable, index_buffer)) == NULL)
        {   // the supplied buffer object handle is not valid.
            result = CG_INVALID_VALUE;
            return CG_INVALID_HANDLE;
        }
        if  (indexbuf->GraphicsBuffer == 0)
        {   // no OpenGL buffer object is associated with the specified data buffer.
            result = CG_INVALID_VALUE;
            return CG_INVALID_HANDLE;
        }
    }

    CG_BUFFER *buffers[16];
    for (size_t i  = 0, n = num_buffers; i < n; ++i)
    {
        buffers[i] = cgObjectTableGet(&ctx->BufferTable, buffer_list[i]);
        if (buffers[i] == NULL || buffers[i]->GraphicsBuffer == 0)
        {   // an invalid buffer handle was specified.
            result = CG_INVALID_VALUE;
            return CG_INVALID_HANDLE;
        }
    }

    // calculate the total number of vertex attributes.
    size_t  num_attribs  =  0;
    for (size_t i = 0, n = num_buffers, a = 0; i < n; ++i)
    {
        cg_vertex_attribute_t const *attrib_list = attrib_data[i];
        if (attrib_counts[i] == 0)
        {   // each buffer must define at least one vertex attribute.
            result = CG_INVALID_VALUE;
            return CG_INVALID_HANDLE;
        }
        // run through the vertex attribute definitions and perform basic validation.
        for (size_t j = 0, m = attrib_counts[i]; j < m; ++j, ++a)
        {
            if (attrib_list[j].BufferIndex != i)
            {   // the buffer index is specified incorrectly.
                result = CG_INVALID_VALUE;
                return CG_INVALID_HANDLE;
            }
        }
        num_attribs += attrib_counts[i];
    }

    // allocate memory for the vertex attribute metadata.
    GLuint                 buf      =  0;
    GLuint                 vao      =  0;
    size_t                *strides  = (size_t              *) cgAllocateHostMemory(&ctx->HostAllocator, num_buffers * sizeof(size_t)             , 0, CG_ALLOCATION_TYPE_OBJECT);
    size_t                *indices  = (size_t              *) cgAllocateHostMemory(&ctx->HostAllocator, num_attribs * sizeof(size_t)             , 0, CG_ALLOCATION_TYPE_OBJECT);
    GLuint                *regs     = (GLuint              *) cgAllocateHostMemory(&ctx->HostAllocator, num_attribs * sizeof(GLuint)             , 0, CG_ALLOCATION_TYPE_OBJECT);
    CG_VERTEX_ATTRIBUTE   *attribs  = (CG_VERTEX_ATTRIBUTE *) cgAllocateHostMemory(&ctx->HostAllocator, num_attribs * sizeof(CG_VERTEX_ATTRIBUTE), 0, CG_ALLOCATION_TYPE_OBJECT);
    CG_DISPLAY            *display  =  group->AttachedDisplay;
    cg_handle_t            handle   =  CG_INVALID_HANDLE;
    CG_VERTEX_DATA_SOURCE  source;
    if (strides == NULL || indices == NULL || regs == NULL || attribs == NULL)
    {   // unable to allocate the required memory.
        result = CG_OUT_OF_MEMORY;
        goto error_cleanup;
    }
    glGenVertexArrays(1, &vao);
    if (vao == 0)
    {   // unable to allocate a vertex array object name.
        result = CG_BAD_GLCONTEXT;
        goto error_cleanup;
    }

    // calculate OpenGL types and compute stride values.
    for (size_t buffer_index = 0, global_attrib = 0; buffer_index < num_buffers; ++buffer_index)
    {
        cg_vertex_attribute_t const *local_attribs = attrib_data  [buffer_index];
        size_t                       local_count   = attrib_counts[buffer_index];

        // loop over all of the vertex attributes in the buffer. convert to OpenGL types and update the stride.
        strides[buffer_index] = 0;
        for (size_t local_attrib  = 0; local_attrib < local_count; ++local_attrib, ++global_attrib)
        {
            GLenum  type = GL_NONE;
            GLsizei dim  = 0;
            GLuint  reg  = 0;
            size_t  ofs  = 0;
            size_t  siz  = 0;
            GLboolean n  = GL_FALSE;

            switch (local_attribs[local_attrib].ComponentType)
            {
            case CG_ATTRIBUTE_FORMAT_FLOAT32:
                type = GL_FLOAT;
                dim  =(GLsizei) local_attribs[local_attrib].ComponentCount;
                reg  =(GLuint ) local_attribs[local_attrib].ShaderLocation;
                ofs  =(size_t ) local_attribs[local_attrib].ByteOffset;
                siz  = sizeof(float);
                n    = GL_FALSE;
                break;
            case CG_ATTRIBUTE_FORMAT_FLOAT16:
                type = GL_HALF_FLOAT;
                dim  =(GLsizei) local_attribs[local_attrib].ComponentCount;
                reg  =(GLuint ) local_attribs[local_attrib].ShaderLocation;
                ofs  =(size_t ) local_attribs[local_attrib].ByteOffset;
                siz  = sizeof(uint16_t);
                n    = GL_FALSE;
                break;
            case CG_ATTRIBUTE_FORMAT_SINT8:
                type = GL_BYTE;
                dim  =(GLsizei) local_attribs[local_attrib].ComponentCount;
                reg  =(GLuint ) local_attribs[local_attrib].ShaderLocation;
                ofs  =(size_t ) local_attribs[local_attrib].ByteOffset;
                siz  = sizeof(int8_t);
                n    = local_attribs[local_attrib].Normalized ? GL_TRUE : GL_FALSE;
                break;
            case CG_ATTRIBUTE_FORMAT_UINT8:
                type = GL_UNSIGNED_BYTE;
                dim  =(GLsizei) local_attribs[local_attrib].ComponentCount;
                reg  =(GLuint ) local_attribs[local_attrib].ShaderLocation;
                ofs  =(size_t ) local_attribs[local_attrib].ByteOffset;
                siz  = sizeof(uint8_t);
                n    = local_attribs[local_attrib].Normalized ? GL_TRUE : GL_FALSE;
                break;
            case CG_ATTRIBUTE_FORMAT_SINT16:
                type = GL_SHORT;
                dim  =(GLsizei) local_attribs[local_attrib].ComponentCount;
                reg  =(GLuint ) local_attribs[local_attrib].ShaderLocation;
                ofs  =(size_t ) local_attribs[local_attrib].ByteOffset;
                siz  = sizeof(int16_t);
                n    = local_attribs[local_attrib].Normalized ? GL_TRUE : GL_FALSE;
                break;
            case CG_ATTRIBUTE_FORMAT_UINT16:
                type = GL_UNSIGNED_SHORT;
                dim  =(GLsizei) local_attribs[local_attrib].ComponentCount;
                reg  =(GLuint ) local_attribs[local_attrib].ShaderLocation;
                ofs  =(size_t ) local_attribs[local_attrib].ByteOffset;
                siz  = sizeof(uint16_t);
                n    = local_attribs[local_attrib].Normalized ? GL_TRUE : GL_FALSE;
                break;
            case CG_ATTRIBUTE_FORMAT_SINT32:
                type = GL_INT;
                dim  =(GLsizei) local_attribs[local_attrib].ComponentCount;
                reg  =(GLuint ) local_attribs[local_attrib].ShaderLocation;
                ofs  =(size_t ) local_attribs[local_attrib].ByteOffset;
                siz  = sizeof(int32_t);
                n    = local_attribs[local_attrib].Normalized ? GL_TRUE : GL_FALSE;
                break;
            case CG_ATTRIBUTE_FORMAT_UINT32:
                type = GL_UNSIGNED_INT;
                dim  =(GLsizei) local_attribs[local_attrib].ComponentCount;
                reg  =(GLuint ) local_attribs[local_attrib].ShaderLocation;
                ofs  =(size_t ) local_attribs[local_attrib].ByteOffset;
                siz  = sizeof(uint32_t);
                n    = local_attribs[local_attrib].Normalized ? GL_TRUE : GL_FALSE;
                break;
            default:
                break;
            }
            if (dim < 1 || dim > 4 || siz == 0)
            {   // the type was invalid, or the dimension was invalid.
                result = CG_INVALID_VALUE;
                goto error_cleanup;
            }

            // copy data out into our object-local storage.
            strides[buffer_index ]           += siz * dim;
            indices[global_attrib]            = buffer_index;
            regs   [global_attrib]            = reg;
            attribs[global_attrib].DataType   = type;
            attribs[global_attrib].Dimension  = dim;
            attribs[global_attrib].ByteOffset = ofs;
            attribs[global_attrib].Normalized = n;
        }
    }

    // set up the OpenGL vertex array object. the VAO stores which attributes 
    // are enabled, where they come from (buffer bindings, types, offsets, strides)
    // and also which index array is active. OpenGL 3.3+ also stores instancing state.
    glBindVertexArray(vao);
    if  (indexbuf != NULL)
    {   // the VAO captures the element array buffer binding immediately.
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuf->GraphicsBuffer);
    }
    for (size_t i  = 0, n = num_attribs; i < n; ++i)
    {
        CG_BUFFER *attrib_source = buffers[indices[i]];
        if (buf != attrib_source->GraphicsBuffer)
        {   // bind the buffer containing the vertex attribute data. note that 
            // the VAO doesn't capture this binding until glVertexAttribPointer.
            glBindBuffer(GL_ARRAY_BUFFER, attrib_source->GraphicsBuffer);
            buf = attrib_source->GraphicsBuffer;
        }
        glEnableVertexAttribArray(regs[i]);
        glVertexAttribPointer
        (   regs   [i], 
            attribs[i].Dimension, 
            attribs[i].DataType, 
            attribs[i].Normalized, 
            strides[indices[i]], 
            CG_GL_BUFFER_OFFSET(attribs[i].ByteOffset)
        );
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // fill out and add the new vertex layout object.
    source.VertexArray    = vao;
    source.BufferCount    = num_buffers;
    source.AttributeCount = num_attribs;
    source.BufferStrides  = strides;
    source.BufferIndices  = indices;
    source.ShaderLocations= regs;
    source.AttributeData  = attribs;
    source.AttachedDisplay= display;
    if ((handle = cgObjectTableAdd(&ctx->VertexSourceTable, source)) == CG_INVALID_HANDLE)
    {   // the object table is full.
        result = CG_OUT_OF_OBJECTS;
        goto error_cleanup;
    }
    result = CG_SUCCESS;
    return handle;

error_cleanup:
    if (vao != 0)
    {
        glBindVertexArray(0);
        glDeleteVertexArrays(1, &vao);
    }
    cgFreeHostMemory(&ctx->HostAllocator, attribs, num_attribs * sizeof(CG_VERTEX_ATTRIBUTE), 0, CG_ALLOCATION_TYPE_OBJECT);
    cgFreeHostMemory(&ctx->HostAllocator, regs   , num_attribs * sizeof(GLuint)             , 0, CG_ALLOCATION_TYPE_OBJECT);
    cgFreeHostMemory(&ctx->HostAllocator, indices, num_attribs * sizeof(size_t)             , 0, CG_ALLOCATION_TYPE_OBJECT);
    cgFreeHostMemory(&ctx->HostAllocator, strides, num_buffers * sizeof(size_t)             , 0, CG_ALLOCATION_TYPE_OBJECT);
    return CG_INVALID_HANDLE;
}

/// @summary Create a kernel object and load graphics or compute shader code into it.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param exec_group The handle of the execution group that owns the kernel object.
/// @param code A description of the code to load into the kernel object.
/// @param result On return, set to CG_SUCCESS, CG_INVALID_VALUE, CG_UNSUPPORTED, CG_OUT_OF_MEMORY, CG_BAD_GLCONTEXT, CG_BAD_CLCONTEXT, CG_COMPILE_FAILED, CG_ERROR or CG_OUT_OF_OBJECTS.
library_function cg_handle_t
cgCreateKernel
(
    uintptr_t               context,
    cg_handle_t             exec_group,
    cg_kernel_code_t const *code,
    int                    &result
)
{
    CG_CONTEXT    *ctx   = (CG_CONTEXT*) context;
    CG_EXEC_GROUP *group =  cgObjectTableGet(&ctx->ExecGroupTable, exec_group);
    if (group == NULL)
    {   // an invalid execution group was specified.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }

    // proceed with kernel creation.
    CG_KERNEL kernel;
    switch (code->Type)
    {
    case CG_KERNEL_TYPE_GRAPHICS_VERTEX:
    case CG_KERNEL_TYPE_GRAPHICS_FRAGMENT:
    case CG_KERNEL_TYPE_GRAPHICS_PRIMITIVE:
        {
            if ((code->Flags & CG_KERNEL_FLAGS_SOURCE) == 0)
            {   // constructing a shader from binary is not supported.
                result = CG_UNSUPPORTED;
                return CG_INVALID_HANDLE;
            }
            if (group->RenderingContext == NULL)
            {   // there's no OpenGL available on this execution group.
                result = CG_UNSUPPORTED;
                return CG_INVALID_HANDLE;
            }
            // allocate storage for the shader object handles.
            kernel.KernelType       = code->Type;
            kernel.ComputeContext   = NULL;
            kernel.ComputeProgram   = NULL;
            kernel.AttachedDisplay  = group->AttachedDisplay;
            kernel.RenderingContext = group->RenderingContext;
            kernel.GraphicsShader   = 0;

            // convert from CG_KERNEL_GRAPHICS_xxx => OpenGL shader type.
            GLenum shader_type   = 0;
                 if (code->Type == CG_KERNEL_TYPE_GRAPHICS_VERTEX   ) shader_type = GL_VERTEX_SHADER;
            else if (code->Type == CG_KERNEL_TYPE_GRAPHICS_FRAGMENT ) shader_type = GL_FRAGMENT_SHADER;
            else if (code->Type == CG_KERNEL_TYPE_GRAPHICS_PRIMITIVE) shader_type = GL_GEOMETRY_SHADER;

            // compile the kernel for the OpenGL rendering context.
            CG_DISPLAY *display  = group->AttachedDisplay;
            GLuint      shader   = glCreateShader(shader_type);
            GLint       clres    = GL_FALSE;
            GLsizei     log_size = 0;

            // compile the shader source code.
            if (shader == 0)
            {   // unable to create the shader object.
                cgDeleteKernel(ctx, &kernel);
                result = CG_BAD_GLCONTEXT;
                return CG_INVALID_HANDLE;
            }
            glShaderSource (shader, 1, (GLchar const* const*) &code->Code, (GLint const*) &code->CodeSize);
            glCompileShader(shader);
            glGetShaderiv  (shader, GL_COMPILE_STATUS , &clres);
            glGetShaderiv  (shader, GL_INFO_LOG_LENGTH, &log_size);
            glGetError();
            if (clres != GL_TRUE)
            {
#ifdef _DEBUG
                GLsizei len = 0;
                GLchar *buf = (GLchar*) cgAllocateHostMemory(&ctx->HostAllocator, log_size+1, 0, CG_ALLOCATION_TYPE_TEMP);
                glGetShaderInfoLog(shader, log_size+1, &len, buf);
                buf[len]    = '\0';
                OutputDebugString(_T("OpenGL shader compilation failed: \n"));
                OutputDebugString(_T("**** Source Code: \n"));
                OutputDebugStringA((char const*) code->Code);
                OutputDebugString(_T("**** Compile Log: \n"));
                OutputDebugStringA(buf);
                OutputDebugString(_T("\n\n"));
                cgFreeHostMemory(&ctx->HostAllocator, buf, log_size+1, 0, CG_ALLOCATION_TYPE_TEMP);
#endif
                glDeleteShader(shader);
                cgDeleteKernel(ctx, &kernel);
                result = CG_COMPILE_FAILED;
                return CG_INVALID_HANDLE;
            }

            // save the shader object handle.
            kernel.GraphicsShader = shader;
        }
        break;

    case CG_KERNEL_TYPE_COMPUTE:
        {   // allocate storage for the program object handles.
            kernel.KernelType       = code->Type;
            kernel.ComputeContext   = group->ComputeContext;
            kernel.ComputeProgram   = NULL;
            kernel.AttachedDisplay  = NULL;
            kernel.RenderingContext = NULL;
            kernel.GraphicsShader   = 0;

            // OpenCL drivers support kernel program loading from source or binary.
            if (code->Flags & CG_KERNEL_FLAGS_SOURCE)
            {   // load the source code into the program object.
                cl_int clres = CL_SUCCESS;
                cl_program p = clCreateProgramWithSource(group->ComputeContext, 1, (char const**) &code->Code, &code->CodeSize, &clres);
                if (p == NULL)
                {
                    switch (clres)
                    {
                    case CL_INVALID_CONTEXT   : result = CG_BAD_CLCONTEXT; break;
                    case CL_INVALID_VALUE     : result = CG_INVALID_VALUE; break;
                    case CL_OUT_OF_RESOURCES  : result = CG_OUT_OF_MEMORY; break;
                    case CL_OUT_OF_HOST_MEMORY: result = CG_OUT_OF_MEMORY; break;
                    default: result = CG_ERROR; break;
                    }
                    cgDeleteKernel(ctx, &kernel);
                    return CG_INVALID_HANDLE;
                }
                // compile the program for each device attached to the context.
                // TODO(rlk): support passing compiler options and defines.
                if ((clres = clBuildProgram(p, cl_uint(group->DeviceCount), group->DeviceIds, NULL, NULL, NULL)) != CL_SUCCESS)
                {
#ifdef _DEBUG
                    if (clres == CL_BUILD_PROGRAM_FAILURE)
                    {
                        size_t log_size = 0;
                        clGetProgramBuildInfo(p, group->DeviceIds[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
                        size_t len = 0;
                        char  *buf = (char*) cgAllocateHostMemory(&ctx->HostAllocator, log_size+1, 0, CG_ALLOCATION_TYPE_TEMP);
                        clGetProgramBuildInfo(p, group->DeviceIds[0], CL_PROGRAM_BUILD_LOG, log_size+1, buf, &len);
                        buf[len]   = '\0';
                        OutputDebugString(_T("OpenCL program compilation failed: \n"));
                        OutputDebugString(_T("**** Source Code: \n"));
                        OutputDebugStringA((char const*) code->Code);
                        OutputDebugString(_T("**** Compile Log: \n"));
                        OutputDebugStringA(buf);
                        OutputDebugString(_T("\n\n"));
                        cgFreeHostMemory(&ctx->HostAllocator, buf, log_size+1, 0, CG_ALLOCATION_TYPE_TEMP);
                    }
#endif
                    switch (clres)
                    {
                    case CL_INVALID_PROGRAM       : result = CG_BAD_CLCONTEXT; break;
                    case CL_INVALID_VALUE         : result = CG_INVALID_VALUE; break;
                    case CL_INVALID_DEVICE        : result = CG_BAD_CLCONTEXT; break;
                    case CL_INVALID_BINARY        : result = CG_INVALID_VALUE; break;
                    case CL_INVALID_BUILD_OPTIONS : result = CG_INVALID_VALUE; break;
                    case CL_INVALID_OPERATION     : result = CG_INVALID_STATE; break;
                    case CL_COMPILER_NOT_AVAILABLE: result = CG_UNSUPPORTED;   break;
                    case CL_BUILD_PROGRAM_FAILURE : result = CG_COMPILE_FAILED;break;
                    case CL_OUT_OF_HOST_MEMORY    : result = CG_OUT_OF_MEMORY; break;
                    default                       : result = CG_ERROR;         break;
                    }
                    cgDeleteKernel(ctx, &kernel);
                    return CG_INVALID_HANDLE;
                }

                // save the compute kernel handle.
                kernel.ComputeProgram = p;
            }
            else
            {   // TODO(rlk): support loading of binaries.
                // this requires having a list of devices per-context.
                // also, the kernel code structure must store a pointer to the binary for each device.
                cgDeleteKernel(ctx, &kernel);
                result = CG_UNSUPPORTED;
                return CG_INVALID_HANDLE;
            }
        }
        break;

    default:
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }

    cg_handle_t handle = cgObjectTableAdd(&ctx->KernelTable, kernel);
    if (handle == CG_INVALID_HANDLE)
    {
        cgDeleteKernel(ctx, &kernel);
        result = CG_OUT_OF_OBJECTS;
        return CG_INVALID_HANDLE;
    }
    return handle;
}

/// @summary Create a new compute pipeline object to execute an OpenCL compute kernel.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param exec_group The handle of the execution group defining the devices the kernel may execute on.
/// @param create Information about the pipeline configuration.
/// @param opaque Opaque state internal to the pipeline implementation.
/// @param teardown The callback function to invoke when the pipeline is destroyed.
/// @param result On return, set to CG_SUCCESS, ...
/// @return The handle of the compute pipeline, or CG_INVALID_HANDLE.
library_function cg_handle_t
cgCreateComputePipeline
(
    uintptr_t                    context,
    cg_handle_t                  exec_group,
    cg_compute_pipeline_t const *create,
    void                        *opaque,
    cgPipelineTeardown_fn        teardown,
    int                         &result
)
{
    cl_uint    arg_count =  0;
    cg_handle_t   handle =  CG_INVALID_HANDLE;
    CG_CONTEXT    *ctx   = (CG_CONTEXT*) context;
    CG_EXEC_GROUP *group =  cgObjectTableGet(&ctx->ExecGroupTable, exec_group);
    if (group == NULL)
    {   // an invalid execution group was specified.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }

    CG_KERNEL *kernel = cgObjectTableGet(&ctx->KernelTable, create->KernelProgram);
    if (kernel == NULL || kernel->KernelType != CG_KERNEL_TYPE_COMPUTE)
    {   // an invalid kernel program handle was specified.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }

    // use the no-op teardown callback if none is supplied.
    if (teardown == NULL)
    {   // using the no-op prevents having to NULL-check.
        teardown  = cgPipelineTeardownNoOp;
    }

    // allocate resources for the compute pipeline description:
    CG_PIPELINE          pipe;
    CG_COMPUTE_PIPELINE &cp= pipe.Compute;
    memset(&pipe.Compute, 0, sizeof(CG_COMPUTE_PIPELINE));
    pipe.PipelineType   = CG_PIPELINE_TYPE_COMPUTE;
    pipe.PrivateState   = opaque;
    pipe.DestroyState   = teardown;
    cp.ComputeContext   = group->ComputeContext;
    cp.ComputeKernel    = NULL;

    cp.DeviceCount      = group->DeviceCount;
    cp.DeviceList       = group->DeviceList;
    cp.DeviceIds        = group->DeviceIds;
    cp.ComputeQueues    = group->ComputeQueues;
    cp.DeviceKernelInfo =(CG_CL_WORKGROUP_INFO*) cgAllocateHostMemory(&ctx->HostAllocator, group->DeviceCount * sizeof(CG_CL_WORKGROUP_INFO), 0, CG_ALLOCATION_TYPE_OBJECT);
    if (cp.DeviceKernelInfo == NULL)
    {   // unable to allocate required memory.
        result = CG_OUT_OF_MEMORY;
        goto error_cleanup;
    }
    memset(cp.DeviceKernelInfo, 0, group->DeviceCount  * sizeof(CG_CL_WORKGROUP_INFO));

    // create the cl_kernel object for each context:
    cl_int clres = 0;
    cl_kernel  k = clCreateKernel(kernel->ComputeProgram, create->KernelName, &clres);
    if (k == NULL)
    {   // the kernel couldn't be created.
        switch (clres)
        {
        case CL_INVALID_PROGRAM           : result = CG_ERROR; break;
        case CL_INVALID_PROGRAM_EXECUTABLE: result = CG_ERROR; break;
        case CL_INVALID_KERNEL_NAME       : result = CG_ERROR; break;
        case CL_INVALID_KERNEL_DEFINITION : result = CG_ERROR; break;
        case CL_INVALID_VALUE             : result = CG_ERROR; break;
        case CL_OUT_OF_HOST_MEMORY        : result = CG_OUT_OF_MEMORY; break;
        default: result = CG_ERROR;  break;
        }
        goto error_cleanup;
    }
    else
    {   // link the kernel to all devices that share the context.
        for (size_t device_index = 0, device_count = group->DeviceCount; device_index < device_count; ++device_index)
        {
            clGetKernelWorkGroupInfo(k, group->DeviceIds[device_index], CL_KERNEL_WORK_GROUP_SIZE        , sizeof(size_t) * 1, &cp.DeviceKernelInfo[device_index].WorkGroupSize , NULL);
            clGetKernelWorkGroupInfo(k, group->DeviceIds[device_index], CL_KERNEL_COMPILE_WORK_GROUP_SIZE, sizeof(size_t) * 3,  cp.DeviceKernelInfo[device_index].FixedGroupSize, NULL);
            clGetKernelWorkGroupInfo(k, group->DeviceIds[device_index], CL_KERNEL_LOCAL_MEM_SIZE         , sizeof(cl_ulong)  , &cp.DeviceKernelInfo[device_index].LocalMemory   , NULL);
        }
        cp.ComputeKernel = k;
    }

    // retrieve argument information. this is consistent across kernels.
    clGetKernelInfo(cp.ComputeKernel, CL_KERNEL_NUM_ARGS, sizeof(cl_uint), &arg_count, NULL);
    cp.ArgumentCount = size_t(arg_count);
    cp.ArgumentNames =(uint32_t        *) cgAllocateHostMemory(&ctx->HostAllocator, arg_count * sizeof(uint32_t)        , 0, CG_ALLOCATION_TYPE_OBJECT);
    cp.Arguments     =(CG_CL_KERNEL_ARG*) cgAllocateHostMemory(&ctx->HostAllocator, arg_count * sizeof(CG_CL_KERNEL_ARG), 0, CG_ALLOCATION_TYPE_OBJECT);
    if (cp.ArgumentNames == NULL || cp.Arguments == NULL)
    {   // unable to allocate the required memory.
        result = CG_OUT_OF_MEMORY;
        goto error_cleanup;
    }
    for (size_t i = 0; i < size_t(arg_count); ++i)
    {
        char          *name = cgClKernelArgName(ctx, cp.ComputeKernel, cl_uint(i), CG_ALLOCATION_TYPE_TEMP);
        cp.ArgumentNames[i] = cgHashName(name);
        cp.Arguments[i].Index  = cl_uint(i);
        clGetKernelArgInfo(cp.ComputeKernel, cl_uint(i), CL_KERNEL_ARG_ACCESS_QUALIFIER , sizeof(cl_kernel_arg_access_qualifier) , &cp.Arguments[i].ImageAccess  , NULL);
        clGetKernelArgInfo(cp.ComputeKernel, cl_uint(i), CL_KERNEL_ARG_ADDRESS_QUALIFIER, sizeof(cl_kernel_arg_address_qualifier), &cp.Arguments[i].MemoryType   , NULL);
        clGetKernelArgInfo(cp.ComputeKernel, cl_uint(i), CL_KERNEL_ARG_TYPE_QUALIFIER   , sizeof(cl_kernel_arg_type_qualifier)   , &cp.Arguments[i].TypeQualifier, NULL);
        cgClFreeString(ctx, name, CG_ALLOCATION_TYPE_TEMP);
    }

    // insert the pipeline into the object table.
    if ((handle = cgObjectTableAdd(&ctx->PipelineTable, pipe)) == CG_INVALID_HANDLE)
    {   // the object table is full.
        result = CG_OUT_OF_OBJECTS;
        goto error_cleanup;
    }
    return handle;

error_cleanup:
    cgDeleteComputePipeline(ctx, &pipe.Compute);
    return CG_INVALID_HANDLE;
}

/// @summary Create a new graphics pipeline object to execute an OpenGL shader program.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param exec_group The handle of the execution group defining the rendering contexts.
/// @param create Information about the pipeline configuration.
/// @param opaque Opaque state internal to the pipeline implementation.
/// @param teardown The callback function to invoke when the pipeline is destroyed.
/// @param result On return, set to CG_SUCCESS, CG_UNSUPPORTED, ...
/// @return The handle of the graphics pipeline, or CG_INVALID_HANDLE.
library_function cg_handle_t
cgCreateGraphicsPipeline
(
    uintptr_t                     context,
    cg_handle_t                   exec_group,
    cg_graphics_pipeline_t const *create,
    void                         *opaque, 
    cgPipelineTeardown_fn         teardown,
    int                          &result
)
{
    cg_handle_t   handle =  CG_INVALID_HANDLE;
    CG_CONTEXT    *ctx   = (CG_CONTEXT*) context;
    CG_EXEC_GROUP *group =  cgObjectTableGet(&ctx->ExecGroupTable, exec_group);
    if (group == NULL)
    {   // an invalid execution group was specified.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }
    if (group->RenderingContext == NULL)
    {   // this execution group has no OpenGL support.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }

    // resolve references to kernel objects and link them into an OpenGL program.
    CG_KERNEL *vs_kernel = cgObjectTableGet(&ctx->KernelTable, create->VertexShader);
    if (vs_kernel == NULL || vs_kernel->KernelType != CG_KERNEL_TYPE_GRAPHICS_VERTEX)
    {   // an invalid kernel program handle was specified.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }
    CG_KERNEL *fs_kernel = cgObjectTableGet(&ctx->KernelTable, create->FragmentShader);
    if (fs_kernel == NULL || fs_kernel->KernelType != CG_KERNEL_TYPE_GRAPHICS_FRAGMENT)
    {   // an invalid kernel program handle was specified.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }
    CG_KERNEL *gs_kernel = cgObjectTableGet(&ctx->KernelTable, create->GeometryShader);
    if (gs_kernel == NULL && create->GeometryShader != CG_INVALID_HANDLE)
    {   // an invalid kernel program handle was specified.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }
    if (gs_kernel != NULL && gs_kernel->KernelType != CG_KERNEL_TYPE_GRAPHICS_PRIMITIVE)
    {   // an invalid kernel program handle was specified.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }

    // use the no-op teardown callback if none is supplied.
    if (teardown == NULL)
    {   // using the no-op prevents having to NULL-check.
        teardown  = cgPipelineTeardownNoOp;
    }

    // allocate resources for the graphics pipeline description:
    CG_DISPLAY      *display= group->AttachedDisplay;
    CG_PIPELINE         pipe;
    CG_GRAPHICS_PIPELINE &gp= pipe.Graphics;
    memset(&pipe.Graphics, 0, sizeof(CG_GRAPHICS_PIPELINE));
    pipe.PipelineType       = CG_PIPELINE_TYPE_GRAPHICS;
    pipe.PrivateState       = opaque;
    pipe.DestroyState       = teardown;
    gp.AttachedDisplay      = group->AttachedDisplay;
    gp.DeviceCount          = group->DeviceCount;
    gp.DeviceList           = group->DeviceList;
    gp.DisplayCount         = group->DisplayCount;
    gp.AttachedDisplays     = group->AttachedDisplays;
    gp.GraphicsQueues       = group->GraphicsQueues;

    // link all of the shaders into a program object.
    CG_GLSL_PROGRAM  &glsl  = gp.ShaderProgram;
    GLuint         program  = glCreateProgram();
    GLuint              vs  = vs_kernel != NULL ? vs_kernel->GraphicsShader : 0; // vertex shader object
    GLuint              fs  = fs_kernel != NULL ? fs_kernel->GraphicsShader : 0; // fragment shader object
    GLuint              gs  = gs_kernel != NULL ? gs_kernel->GraphicsShader : 0; // geometry shader object
    GLint          linkres  = GL_FALSE;
    GLsizei       log_size  = 0;
    GLint            a_max  = 0; // length of longest active vertex attribute name
    GLint            u_max  = 0; // length of longest active uniform name
    size_t        name_max  = 0; // length of longest string name
    size_t     num_attribs  = 0;
    size_t    num_samplers  = 0;
    size_t    num_uniforms  = 0;
    char         *name_buf  = NULL;

    if (program == 0)
    {   // unable to create the program object.
        result = CG_BAD_GLCONTEXT;
        goto error_cleanup;
    }
    if (vs == 0 || fs == 0) // gs is optional.
    {   // required shaders are missing.
        glDeleteProgram(program);
        result = CG_LINK_FAILED;
        goto error_cleanup;
    }

    // attach the shader objects to the program object.
    if (vs) glAttachShader(program, vs);
    if (gs) glAttachShader(program, gs);
    if (fs) glAttachShader(program, fs);

    // set vertex attribute locations pre-link.
    for (size_t i = 0, n = create->AttributeCount; i < n; ++i)
    {
        cg_shader_binding_t const &binding = create->AttributeBindings[i];
        glBindAttribLocation(program, (GLuint) binding.Location, (GLchar const*) binding.Name);
    }
    // set fragment output locations pre-link.
    for (size_t i = 0, n = create->OutputCount; i < n; ++i)
    {
        cg_shader_binding_t const &binding = create->OutputBindings[i];
        glBindFragDataLocation(program, (GLuint) binding.Location, (GLchar const*) binding.Name);
    }

    // link the shaders into an executable program.
    glLinkProgram (program);
    glGetProgramiv(program, GL_LINK_STATUS , &linkres);
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_size);
    if (linkres != GL_TRUE)
    {
#ifdef _DEBUG
        GLsizei len = 0;
        GLchar *buf = (GLchar*) cgAllocateHostMemory(&ctx->HostAllocator, log_size+1, 0, CG_ALLOCATION_TYPE_TEMP);
        glGetProgramInfoLog(program, log_size+1, &len, buf);
        buf[len] = '\0';
        OutputDebugString(_T("OpenGL shader linking failed: \n"));
        OutputDebugString(_T("**** Linker Log: \n"));
        OutputDebugStringA(buf);
        OutputDebugString(_T("\n\n"));
        cgFreeHostMemory(&ctx->HostAllocator, buf, log_size+1, 0, CG_ALLOCATION_TYPE_TEMP);
#endif
        glDeleteProgram(program);
        result = CG_LINK_FAILED;
        goto error_cleanup;
    }

    // grab the length of the longest attribute and uniform name.
    // we'll use these values to allocate a single temp buffer for strings.
    glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH  , &u_max);
    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &a_max);
    name_max = (size_t) (u_max > a_max ? u_max + 1 : a_max + 1);
    name_buf = (char *)  cgAllocateHostMemory(&ctx->HostAllocator, name_max, 0, CG_ALLOCATION_TYPE_TEMP);
    if (name_buf == NULL)
    {   // unable to allocate the required memory.
        glDeleteProgram(program);
        result = CG_OUT_OF_MEMORY;
        goto error_cleanup;
    }
    cgGlslReflectProgramCounts(display, program, name_buf, name_max, false, num_attribs, num_samplers, num_uniforms);

    // allocate storage for the attribute, sampler and uniform metadata.
    glsl.AttributeCount = num_attribs;
    glsl.AttributeNames =(uint32_t         *) cgAllocateHostMemory(&ctx->HostAllocator, num_attribs  * sizeof(uint32_t)         , 0, CG_ALLOCATION_TYPE_OBJECT);
    glsl.Attributes     =(CG_GLSL_ATTRIBUTE*) cgAllocateHostMemory(&ctx->HostAllocator, num_attribs  * sizeof(CG_GLSL_ATTRIBUTE), 0, CG_ALLOCATION_TYPE_OBJECT);
    glsl.SamplerCount   = num_samplers;
    glsl.SamplerNames   =(uint32_t         *) cgAllocateHostMemory(&ctx->HostAllocator, num_samplers * sizeof(uint32_t)         , 0, CG_ALLOCATION_TYPE_OBJECT);
    glsl.Samplers       =(CG_GLSL_SAMPLER  *) cgAllocateHostMemory(&ctx->HostAllocator, num_samplers * sizeof(CG_GLSL_SAMPLER)  , 0, CG_ALLOCATION_TYPE_OBJECT);
    glsl.UniformCount   = num_uniforms;
    glsl.UniformNames   =(uint32_t         *) cgAllocateHostMemory(&ctx->HostAllocator, num_uniforms * sizeof(uint32_t)         , 0, CG_ALLOCATION_TYPE_OBJECT);
    glsl.Uniforms       =(CG_GLSL_UNIFORM  *) cgAllocateHostMemory(&ctx->HostAllocator, num_uniforms * sizeof(CG_GLSL_UNIFORM)  , 0, CG_ALLOCATION_TYPE_OBJECT);
    if (glsl.AttributeNames == NULL || glsl.Attributes == NULL ||
        glsl.SamplerNames   == NULL || glsl.Samplers   == NULL ||
        glsl.UniformNames   == NULL || glsl.Uniforms   == NULL)
    {   // unable to allocate the required memory.
        cgFreeHostMemory(&ctx->HostAllocator, name_buf, name_max, 0, CG_ALLOCATION_TYPE_TEMP);
        glDeleteProgram(program);
        result = CG_OUT_OF_MEMORY;
        goto error_cleanup;
    }
    cgGlslReflectProgramMetadata(display, program, name_buf, name_max, false, &glsl);
    cgFreeHostMemory(&ctx->HostAllocator, name_buf, name_max, 0, CG_ALLOCATION_TYPE_TEMP);

    // all data has been retrieved, so store the program reference.
    glsl.Program = program;

    // convert the fixed-function state from CGFX enums to their OpenGL equivalents.
    CG_DEPTH_STENCIL_STATE &dss = pipe.Graphics.DepthStencilState;
    dss.DepthTestEnable     = cgGlBoolean(create->DepthStencilState.DepthTestEnable);
    dss.DepthWriteEnable    = cgGlBoolean(create->DepthStencilState.DepthWriteEnable);
    dss.DepthBoundsEnable   = cgGlBoolean(create->DepthStencilState.DepthBoundsEnable);
    dss.DepthTestFunction   = cgGlCompareFunction(create->DepthStencilState.DepthTestFunction);
    dss.DepthMin            = create->DepthStencilState.DepthMin;
    dss.DepthMax            = create->DepthStencilState.DepthMax;
    dss.StencilTestEnable   = cgGlBoolean(create->DepthStencilState.StencilTestEnable);
    dss.StencilTestFunction = cgGlCompareFunction(create->DepthStencilState.StencilTestFunction);
    dss.StencilFailOp       = cgGlStencilOp(create->DepthStencilState.StencilFailOp);
    dss.StencilPassZPassOp  = cgGlStencilOp(create->DepthStencilState.StencilPassZPassOp);
    dss.StencilPassZFailOp  = cgGlStencilOp(create->DepthStencilState.StencilPassZFailOp);
    dss.StencilReadMask     = create->DepthStencilState.StencilReadMask;
    dss.StencilWriteMask    = create->DepthStencilState.StencilWriteMask;
    dss.StencilReference    = create->DepthStencilState.StencilReference;

    CG_RASTER_STATE        &rs = pipe.Graphics.RasterizerState;
    rs.FillMode             = cgGlFillMode(create->RasterizerState.FillMode);
    rs.CullMode             = cgGlCullMode(create->RasterizerState.CullMode);
    rs.FrontFace            = cgGlWindingOrder(create->RasterizerState.FrontFace);
    rs.DepthBias            = create->RasterizerState.DepthBias;
    rs.SlopeScaledDepthBias = create->RasterizerState.SlopeScaledDepthBias;

    CG_BLEND_STATE         &bs = pipe.Graphics.BlendState;
    bs.BlendEnabled         = cgGlBoolean(create->BlendState.BlendEnabled);
    bs.SrcBlendColor        = cgGlBlendFactor(create->BlendState.SrcBlendColor);
    bs.DstBlendColor        = cgGlBlendFactor(create->BlendState.DstBlendColor);
    bs.ColorBlendFunction   = cgGlBlendFunction(create->BlendState.ColorBlendFunction);
    bs.SrcBlendAlpha        = cgGlBlendFactor(create->BlendState.SrcBlendAlpha);
    bs.DstBlendAlpha        = cgGlBlendFactor(create->BlendState.DstBlendAlpha);
    bs.AlphaBlendFunction   = cgGlBlendFunction(create->BlendState.AlphaBlendFunction);
    bs.ConstantRGBA[0]      = create->BlendState.ConstantRGBA[0];
    bs.ConstantRGBA[1]      = create->BlendState.ConstantRGBA[1];
    bs.ConstantRGBA[2]      = create->BlendState.ConstantRGBA[2];
    bs.ConstantRGBA[3]      = create->BlendState.ConstantRGBA[3];

    pipe.Graphics.Topology  = cgGlPrimitiveTopology(create->PrimitiveType);

    // insert the pipeline into the object table.
    if ((handle = cgObjectTableAdd(&ctx->PipelineTable, pipe)) == CG_INVALID_HANDLE)
    {   // the object table is full.
        result = CG_OUT_OF_OBJECTS;
        goto error_cleanup;
    }
    return handle;

error_cleanup:
    cgDeleteGraphicsPipeline(ctx, &pipe.Graphics);
    return CG_INVALID_HANDLE;
}

/// @summary Creates a new buffer object and allocates, but does not initialize, the backing memory.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param exec_group The execution group that will read or write the data buffer.
/// @param buffer_size The desired size of the data buffer, in bytes.
/// @param kernel_types One or more of cg_memory_object_kernel_e specifying the types of kernels that will access the buffer.
/// @param kernel_access One or more of cg_memory_access_e specifying how the kernel(s) will access the buffer.
/// @param host_access One or more of cg_memory_access_e specifying how the host will access the buffer.
/// @param placement_hint One of cg_memory_placement_e specifying the heap the buffer should be allocated from. This is only a hint.
/// @param frequency_hint One of cg_memory_update_frequency_e specifying the expected buffer update frequency.
/// @param result On return, set to CG_SUCCESS, CG_NO_OPENGL, CG_BAD_GLCONTEXT, CG_BAD_CLCONTEXT, CG_OUT_OF_MEMORY, CG_INVALID_VALUE or CG_ERROR.
/// @return A handle to the new buffer object, or CG_INVALID_HANDLE.
library_function cg_handle_t
cgCreateDataBuffer
(
    uintptr_t    context,
    cg_handle_t  exec_group,
    size_t       buffer_size,
    uint32_t     kernel_types,
    uint32_t     kernel_access,
    uint32_t     host_access,
    int          placement_hint,
    int          frequency_hint,
    int         &result
)
{
    CG_CONTEXT    *ctx   = (CG_CONTEXT*) context;
    CG_EXEC_GROUP *group =  cgObjectTableGet(&ctx->ExecGroupTable, exec_group);
    if (group == NULL)
    {   // invalid execution group handle.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }
    if ((group->RenderingContext == NULL) && (kernel_types & CG_MEMORY_OBJECT_KERNEL_GRAPHICS))
    {   // want OpenGL interop, but group has no OpenGL capability.
        result = CG_NO_OPENGL;
        return CG_INVALID_HANDLE;
    }
    if (host_access == CG_MEMORY_ACCESS_NONE)
    {   // the host will not map the buffer, so it should be placed in device memory.
        placement_hint = CG_MEMORY_PLACEMENT_DEVICE;
    }

    if (kernel_types & CG_MEMORY_OBJECT_KERNEL_GRAPHICS)
    {   // the buffer must be created in OpenGL first. the OpenGL driver 
        // determines the buffer placement based on hints we provide. 
        GLenum  gl_flags = 0;
        switch (frequency_hint)
        {
        case CG_MEMORY_UPDATE_ONCE:
            {   // the data stored in the buffer is largely static.
                     if (host_access == CG_MEMORY_ACCESS_NONE ) gl_flags = GL_STATIC_COPY;
                else if (host_access  & CG_MEMORY_ACCESS_WRITE) gl_flags = GL_STATIC_DRAW;
                else if (host_access  & CG_MEMORY_ACCESS_READ ) gl_flags = GL_STATIC_READ;
                else
                {   // invalid host access flags specified.
                    result = CG_INVALID_VALUE;
                    return CG_INVALID_HANDLE;
                }
            }
            break;

        case CG_MEMORY_UPDATE_PER_FRAME:
            {   // the data stored in the buffer is updated on the order of once per-frame.
                     if (host_access == CG_MEMORY_ACCESS_NONE ) gl_flags = GL_DYNAMIC_COPY;
                else if (host_access  & CG_MEMORY_ACCESS_WRITE) gl_flags = GL_DYNAMIC_DRAW;
                else if (host_access  & CG_MEMORY_ACCESS_READ ) gl_flags = GL_DYNAMIC_READ;
                else
                {   // invalid host access flags specified.
                    result = CG_INVALID_VALUE;
                    return CG_INVALID_HANDLE;
                }
            }
            break;

        case CG_MEMORY_UPDATE_PER_DISPATCH:
            {   // the data stored in the buffer is updated very frequently.
                     if (host_access == CG_MEMORY_ACCESS_NONE ) gl_flags = GL_STREAM_COPY;
                else if (host_access  & CG_MEMORY_ACCESS_WRITE) gl_flags = GL_STREAM_DRAW;
                else if (host_access  & CG_MEMORY_ACCESS_READ ) gl_flags = GL_STREAM_READ;
                else
                {   // invalid host access flags specified.
                    result = CG_INVALID_VALUE;
                    return CG_INVALID_HANDLE;
                }
            }
            break;

        default:
            result = CG_INVALID_VALUE;
            return CG_INVALID_HANDLE;
        }

        CG_DISPLAY *display   = group->AttachedDisplay;
        GLuint      gl_buffer = 0;
        glGenBuffers(1, &gl_buffer);
        if (gl_buffer == 0)
        {
            result = CG_BAD_GLCONTEXT;
            return CG_INVALID_HANDLE;
        }
        glBindBuffer(GL_ARRAY_BUFFER, gl_buffer);
        glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, gl_flags);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        CG_BUFFER   buffer;
        buffer.KernelTypes     = kernel_types;
        buffer.KernelAccess    = kernel_access;
        buffer.HostAccess      = host_access;
        buffer.AttachedDisplay = group->AttachedDisplay;
        buffer.SourceHeap      = cgGlFindHeapForUsage(ctx, gl_flags);
        buffer.ComputeContext  = NULL;
        buffer.ComputeBuffer   = NULL;
        buffer.ComputeUsage    = 0;
        buffer.AllocatedSize   = align_up(buffer_size, buffer.SourceHeap->DeviceAlignment);
        buffer.RequestedSize   = buffer_size;
        buffer.ExecutionGroup  = exec_group;
        buffer.GraphicsBuffer  = gl_buffer;
        buffer.GraphicsUsage   = gl_flags;

        // set up OpenCL sharing, if requested.
        if (kernel_types & CG_MEMORY_OBJECT_KERNEL_COMPUTE)
        {
            cl_mem_flags cl_flags = 0;
            cl_mem       clmem    = NULL;
            cl_int       clres    = CL_SUCCESS;
            uint32_t     access   =(kernel_access & ~CG_MEMORY_ACCESS_PRESERVE);

            // convert kernel access flags into cl_mem_flags.
            // because OpenGL is responsible for allocating the buffer, 
            // we never specifying anything like CL_MEM_ALLOC_HOST_PTR.
                 if (access == CG_MEMORY_ACCESS_READ      ) cl_flags = CL_MEM_READ_ONLY;
            else if (access == CG_MEMORY_ACCESS_WRITE     ) cl_flags = CL_MEM_WRITE_ONLY;
            else if (access == CG_MEMORY_ACCESS_READ_WRITE) cl_flags = CL_MEM_READ_WRITE;
            else
            {   // CG_MEMORY_ACCESS_NONE is not valid.
                glDeleteBuffers(1, &gl_buffer);
                result = CG_INVALID_VALUE;
                return CG_INVALID_HANDLE;
            }
            if ((clmem = clCreateFromGLBuffer(group->ComputeContext, cl_flags, gl_buffer, &clres)) == NULL)
            {
                switch (clres)
                {
                case CL_INVALID_CONTEXT   : result = CG_BAD_CLCONTEXT; break;
                case CL_INVALID_VALUE     : result = CG_INVALID_VALUE; break;
                case CL_INVALID_GL_OBJECT : result = CG_INVALID_VALUE; break;
                case CL_OUT_OF_HOST_MEMORY: result = CG_OUT_OF_MEMORY; break;
                default: result = CG_ERROR; break;
                }
                glDeleteBuffers(1, &gl_buffer);
                return CG_INVALID_HANDLE;
            }
            buffer.ComputeContext = group->ComputeContext;
            buffer.ComputeBuffer  = clmem;
            buffer.ComputeUsage   = cl_flags;
        }

        cg_handle_t handle = cgObjectTableAdd(&ctx->BufferTable, buffer);
        if (handle == CG_INVALID_HANDLE)
        {
            if (buffer.ComputeBuffer  != NULL) clReleaseMemObject(buffer.ComputeBuffer);
            glDeleteBuffers(1, &buffer.GraphicsBuffer);
            result = CG_OUT_OF_OBJECTS;
            return CG_INVALID_HANDLE;
        }
        // TODO(rlk): update CG_HEAP::HeapSizeUsed and CG_HEAP::PinnedUsed.
        result = CG_SUCCESS;
        return handle;
    }
    else if (kernel_types & CG_MEMORY_OBJECT_KERNEL_COMPUTE)
    {   // compute kernel data only.
        cl_mem_flags cl_flags = 0;
        cl_mem       clmem    = NULL;
        cl_int       clres    = CL_SUCCESS;
        uint32_t     access   =(kernel_access & ~CG_MEMORY_ACCESS_PRESERVE);

        // convert kernel access flags into cl_mem_flags.
        // because OpenGL is responsible for allocating the buffer, 
        // we never specifying anything like CL_MEM_ALLOC_HOST_PTR.
             if (access == CG_MEMORY_ACCESS_READ      ) cl_flags = CL_MEM_READ_ONLY;
        else if (access == CG_MEMORY_ACCESS_WRITE     ) cl_flags = CL_MEM_WRITE_ONLY;
        else if (access == CG_MEMORY_ACCESS_READ_WRITE) cl_flags = CL_MEM_READ_WRITE;
        else
        {   // CG_MEMORY_ACCESS_NONE is not valid.
            result = CG_INVALID_VALUE;
            return CG_INVALID_HANDLE;
        }
        if (placement_hint != CG_MEMORY_PLACEMENT_DEVICE)
        {   // prefer to allocate the buffer in host or pinned memory.
            // there are limitations on buffer size with pinned memory.
            cl_flags |= CL_MEM_ALLOC_HOST_PTR;
        }
        if (host_access == CG_MEMORY_ACCESS_NONE)
        {   // the host promises not to map the memory. attempts to do so will fail.
            cl_flags |= CL_MEM_HOST_NO_ACCESS;
        }
        if ((clmem = clCreateBuffer(group->ComputeContext, cl_flags, buffer_size, NULL, &clres)) == NULL)
        {
            switch (clres)
            {
            case CL_INVALID_CONTEXT              : result = CG_BAD_CLCONTEXT; break;
            case CL_INVALID_VALUE                : result = CG_INVALID_VALUE; break;
            case CL_INVALID_BUFFER_SIZE          : result = CG_INVALID_VALUE; break;
            case CL_INVALID_HOST_PTR             : result = CG_INVALID_VALUE; break;
            case CL_MEM_OBJECT_ALLOCATION_FAILURE: result = CG_OUT_OF_MEMORY; break;
            case CL_OUT_OF_HOST_MEMORY           : result = CG_OUT_OF_MEMORY; break;
            default                              : result = CG_ERROR;         break;
            }
            return CG_INVALID_HANDLE;
        }

        CG_BUFFER   buffer;
        buffer.KernelTypes     = kernel_types;
        buffer.KernelAccess    = kernel_access;
        buffer.HostAccess      = host_access;
        buffer.AttachedDisplay = NULL;
        buffer.SourceHeap      = cgFindHeapForPlacement(ctx, placement_hint);
        buffer.ComputeContext  = group->ComputeContext;
        buffer.ComputeBuffer   = clmem;
        buffer.ComputeUsage    = cl_flags;
        buffer.AllocatedSize   = align_up(buffer_size, buffer.SourceHeap->DeviceAlignment);
        buffer.RequestedSize   = buffer_size;
        buffer.ExecutionGroup  = exec_group;
        buffer.GraphicsBuffer  = 0;
        buffer.GraphicsUsage   = 0;

        cg_handle_t handle     = cgObjectTableAdd(&ctx->BufferTable, buffer);
        if (handle == CG_INVALID_HANDLE)
        {
            clReleaseMemObject(buffer.ComputeBuffer);
            result = CG_OUT_OF_OBJECTS;
            return CG_INVALID_HANDLE;
        }
        // TODO(rlk): update CG_HEAP::HeapSizeUsed and CG_HEAP::PinnedUsed.
        result = CG_SUCCESS;
        return handle;
    }
    else
    {   // invalid kernel_types.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }
}

/// @summary Query data buffer information.
/// @param context A CGFX context returned by cgEnumerateDevices().
/// @param buffer_handle The handle of the data buffer to query.
/// @param param One of cg_data_buffer_info_param_e specifying the data to return.
/// @param buffer A caller-managed buffer to receive the data.
/// @param buffer_size The maximum number of bytes that can be written to @a buffer.
/// @param bytes_needed On return, stores the number of bytes copied to the buffer, or the number of bytes required to store the data.
/// @return CG_SUCCESS, CG_BUFFER_TOO_SMALL, CG_INVALID_VALUE or CG_OUT_OF_MEMORY.
library_function int
cgGetDataBufferInfo
(
    uintptr_t                     context,
    cg_handle_t                   buffer_handle,
    int                           param,
    void                         *buffer,
    size_t                        buffer_size,
    size_t                       *bytes_needed
)
{
    CG_CONTEXT *ctx    = (CG_CONTEXT*) context;
    CG_BUFFER  *object =  NULL;

    if ((object = cgObjectTableGet(&ctx->BufferTable, buffer_handle)) == NULL)
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
#define BUFFER_SET_SCALAR(type, value) \
    *((type*)buffer) = (value)
    // ==========================================================
    // ==========================================================

    // retrieve parameter data:
    switch (param)
    {
    case CG_DATA_BUFFER_HEAP_ORDINAL:
        {   BUFFER_CHECK_TYPE(size_t);
            BUFFER_SET_SCALAR(size_t, object->SourceHeap->Ordinal);
        }
        return CG_SUCCESS;

    case CG_DATA_BUFFER_HEAP_TYPE:
        {   BUFFER_CHECK_TYPE(int);
            BUFFER_SET_SCALAR(int, object->SourceHeap->Type);
        }
        return CG_SUCCESS;

    case CG_DATA_BUFFER_HEAP_FLAGS:
        {   BUFFER_CHECK_TYPE(uint32_t);
            BUFFER_SET_SCALAR(uint32_t, object->SourceHeap->Flags);
        }
        return CG_SUCCESS;

    case CG_DATA_BUFFER_ALLOCATED_SIZE:
        {   BUFFER_CHECK_TYPE(size_t);
            BUFFER_SET_SCALAR(size_t, object->AllocatedSize);
        }
        return CG_SUCCESS;

    default:
        {
            if (bytes_needed != NULL) *bytes_needed = 0;
            return CG_INVALID_VALUE;
        }
    }

#undef  BUFFER_SET_SCALAR
#undef  BUFFER_CHECK_SIZE
#undef  BUFFER_CHECK_TYPE
}

/// @summary Map a region of a buffer object into the host address space. If the host is reading the data, a data transfer from device to host may be performed. Map operations always block until the required data is available.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param queue_handle The queue on which the transfer will be performed.
/// @param buffer_handle The handle of the buffer object to map.
/// @param wait_handle The handle of a fence or event object to wait on before mapping the buffer, or CG_INVALID_HANDLE if the caller asserts that no device is currently accessing the memory object.
/// @param offset The zero-based byte offset defining the start of the region to map.
/// @param amount The number of bytes to map into the host address space.
/// @param flags A combination of cg_memory_access_e specifying mapping options. CG_MEMORY_ACCESS_NONE is not valid.
/// @param result On return, set to CG_SUCCESS, CG_INVALID_VALUE, CG_INVALID_STATE, CG_OUT_OF_MEMORY, CG_BAD_CLCONTEXT or CG_ERROR.
/// @return A pointer to host-accessible memory representing the buffer contents, or NULL.
library_function void*
cgMapDataBuffer
(
    uintptr_t   context,
    cg_handle_t queue_handle,
    cg_handle_t buffer_handle,
    cg_handle_t wait_handle,
    size_t      offset,
    size_t      amount,
    uint32_t    flags,
    int         &result
)
{
    CG_CONTEXT *ctx   = (CG_CONTEXT*) context;
    CG_QUEUE   *queue = (CG_QUEUE  *) cgObjectTableGet(&ctx->QueueTable , queue_handle);
    CG_BUFFER  *obj   = (CG_BUFFER *) cgObjectTableGet(&ctx->BufferTable, buffer_handle);
    if (obj == NULL || queue == NULL || flags == CG_MEMORY_ACCESS_NONE)
    {   // one or more of the supplied handles is invalid.
        result = CG_INVALID_VALUE;
        return NULL;
    }
    // mapping the buffer for read access performs a blocking transfer to the host.
    // mapping the buffer for write access returns a pointer to pinned memory.
    if (queue->QueueType & CG_QUEUE_TYPE_TRANSFER)
    {   // map the buffer using OpenCL.
        cl_map_flags map_flags  = 0;
        cl_event     wait_event = NULL;
        cl_uint      wait_count = 0;
        cl_int       clres      = CL_SUCCESS;
        void        *mapped     = NULL;
        bool         release_ev = false;

        if ((result = cgGetWaitEvent(ctx, queue, wait_handle, &wait_event, wait_count, 1, release_ev)) != CG_SUCCESS)
        {   // some problem with the event or fence we're told to wait on.
            return NULL;
        }

        if (flags & CG_MEMORY_ACCESS_READ)
            map_flags |= CL_MAP_READ;
        if (flags & CG_MEMORY_ACCESS_WRITE)
            map_flags |= CL_MAP_WRITE;
        if ((flags & CG_MEMORY_ACCESS_READ) == 0 && (flags & CG_MEMORY_ACCESS_PRESERVE) == 0)
            map_flags  = CL_MAP_WRITE_INVALIDATE_REGION;

        if (obj->GraphicsBuffer != 0)
        {   // the buffer first needs to be acquired by OpenCL for use.
            cl_event gl_wait = NULL;
            if ((clres = clEnqueueAcquireGLObjects(queue->CommandQueue, 1, &obj->ComputeBuffer, wait_count, CG_OPENCL_WAIT_LIST(wait_count, &wait_event), &gl_wait)) != CL_SUCCESS)
            {
                switch (clres)
                {
                case CL_INVALID_VALUE          : result = CG_INVALID_VALUE; break;
                case CL_INVALID_MEM_OBJECT     : result = CG_INVALID_VALUE; break;
                case CL_INVALID_COMMAND_QUEUE  : result = CG_INVALID_VALUE; break;
                case CL_INVALID_CONTEXT        : result = CG_BAD_CLCONTEXT; break;
                case CL_INVALID_GL_OBJECT      : result = CG_INVALID_VALUE; break;
                case CL_INVALID_EVENT_WAIT_LIST: result = CG_INVALID_VALUE; break;
                case CL_OUT_OF_RESOURCES       : result = CG_OUT_OF_MEMORY; break;
                case CL_OUT_OF_HOST_MEMORY     : result = CG_OUT_OF_MEMORY; break;
                default                        : result = CG_ERROR;         break;
                }
                if (release_ev)
                {   // release any temporary event object.
                    clReleaseEvent(wait_event);
                }
                return NULL;
            }
            // the acquire request was enqueued successfully. overwrite the wait_event 
            // so that the map request waits until the acquire has completed.
            if (release_ev)
            {   // release any temporary event object.
                clReleaseEvent(wait_event);
            }
            wait_count = 1;
            wait_event = gl_wait;
            release_ev = true; // release gl_wait after the buffer map is enqueued.
        }

        if ((mapped = clEnqueueMapBuffer(queue->CommandQueue, obj->ComputeBuffer, CL_TRUE, map_flags, offset, amount, wait_count, CG_OPENCL_WAIT_LIST(wait_count, &wait_event), NULL, &clres)) == NULL)
        {
            switch (clres)
            {
            case CL_INVALID_COMMAND_QUEUE                    : result = CG_INVALID_VALUE; break;
            case CL_INVALID_CONTEXT                          : result = CG_BAD_CLCONTEXT; break;
            case CL_INVALID_MEM_OBJECT                       : result = CG_INVALID_VALUE; break;
            case CL_INVALID_VALUE                            : result = CG_INVALID_VALUE; break;
            case CL_INVALID_EVENT_WAIT_LIST                  : result = CG_INVALID_VALUE; break;
            case CL_MISALIGNED_SUB_BUFFER_OFFSET             : result = CG_INVALID_VALUE; break;
            case CL_MAP_FAILURE                              : result = CG_ERROR;         break;
            case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST: result = CG_INVALID_VALUE; break;
            case CL_MEM_OBJECT_ALLOCATION_FAILURE            : result = CG_OUT_OF_MEMORY; break;
            case CL_INVALID_OPERATION                        : result = CG_INVALID_STATE; break;
            case CL_OUT_OF_RESOURCES                         : result = CG_OUT_OF_MEMORY; break;
            case CL_OUT_OF_HOST_MEMORY                       : result = CG_OUT_OF_MEMORY; break;
            default                                          : result = CG_ERROR;         break;
            }
            if (release_ev)
            {   // release any temporary event object.
                clReleaseEvent(wait_event);
            }
            return NULL;
        }
        if (release_ev)
        {   // release any temporary event object.
            clReleaseEvent(wait_event);
        }
        result = CG_SUCCESS;
        return mapped;
    }
    else
    {   // the queue type is not valid.
        result = CG_INVALID_VALUE;
        return NULL;
    }
}

/// @summary Unmap a region of a mapped buffer. If the host was writing data to the buffer, a data transfer from host to device may be performed. Unmap operations are always non-blocking.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param queue_handle The queue on which the transfer will be performed. This must be the same queue that was supplied to cgMapDataBuffer.
/// @param buffer_handle The handle of the buffer object to map.
/// @param mapped_region The pointer returned by a call to cgMapDataBuffer.
/// @param event_handle On return, if non-NULL, stores a handle to an event object that can be used to block the host or device until the data transfer has completed.
library_function int
cgUnmapDataBuffer
(
    uintptr_t    context,
    cg_handle_t  queue_handle,
    cg_handle_t  buffer_handle,
    void        *mapped_region,
    cg_handle_t *event_handle
)
{
    CG_CONTEXT *ctx   = (CG_CONTEXT*) context;
    CG_QUEUE   *queue = (CG_QUEUE  *) cgObjectTableGet(&ctx->QueueTable , queue_handle);
    CG_BUFFER  *obj   = (CG_BUFFER *) cgObjectTableGet(&ctx->BufferTable, buffer_handle);
    int        result =  CG_SUCCESS;
    if (obj == NULL || queue == NULL)
    {   // one or more of the supplied handles is invalid.
        if (event_handle != NULL) 
        {   // no completion event will be returned since no command was submitted.
           *event_handle = CG_INVALID_HANDLE;
        }
        return CG_INVALID_VALUE;
    }

    if (queue->QueueType & CG_QUEUE_TYPE_TRANSFER)
    {   
        cl_int        clres = CL_SUCCESS;
        cl_event  evt_unmap = NULL;
        cl_event  evt_relgl = NULL;
        cl_event  event     = NULL;
        
        // unmap the buffer from the host address space. if the buffer was mapped for write access, 
        // a data transfer from host to device may be performed (hopefully asynchronously.)
        if ((clres = clEnqueueUnmapMemObject(queue->CommandQueue, obj->ComputeBuffer, mapped_region, 0, NULL, &evt_unmap)) != CL_SUCCESS)
        {
            switch (clres)
            {
            case CL_INVALID_VALUE          : result = CG_INVALID_VALUE; break;
            case CL_INVALID_MEM_OBJECT     : result = CG_INVALID_VALUE; break;
            case CL_INVALID_COMMAND_QUEUE  : result = CG_INVALID_VALUE; break;
            case CL_INVALID_CONTEXT        : result = CG_BAD_CLCONTEXT; break;
            case CL_INVALID_EVENT_WAIT_LIST: result = CG_INVALID_VALUE; break;
            case CL_OUT_OF_RESOURCES       : result = CG_OUT_OF_MEMORY; break;
            case CL_OUT_OF_HOST_MEMORY     : result = CG_OUT_OF_MEMORY; break;
            default                        : result = CG_ERROR;         break;
            }
            return cgSetupNewCompleteEvent(ctx, queue, event_handle, NULL, NULL, result);
        }

        // set the compute event handle to the unmap completion event.
        event = evt_unmap;

        if (clres == CL_SUCCESS && obj->GraphicsBuffer != 0)
        {   // release the buffer so it can be used by OpenGL again. this must wait for the data 
            // transfer, possibly performed by the unmap operation, to complete.
            if ((clres = clEnqueueReleaseGLObjects(queue->CommandQueue, 1, &obj->ComputeBuffer, 1, &evt_unmap, &evt_relgl)) != CL_SUCCESS)
            {
                switch (clres)
                {
                case CL_INVALID_VALUE          : result = CG_INVALID_VALUE; break;
                case CL_INVALID_MEM_OBJECT     : result = CG_INVALID_VALUE; break;
                case CL_INVALID_COMMAND_QUEUE  : result = CG_INVALID_VALUE; break;
                case CL_INVALID_CONTEXT        : result = CG_BAD_CLCONTEXT; break;
                case CL_INVALID_GL_OBJECT      : result = CG_INVALID_VALUE; break;
                case CL_INVALID_EVENT_WAIT_LIST: result = CG_INVALID_VALUE; break;
                case CL_OUT_OF_RESOURCES       : result = CG_OUT_OF_MEMORY; break;
                case CL_OUT_OF_HOST_MEMORY     : result = CG_OUT_OF_MEMORY; break;
                default                        : result = CG_ERROR;         break;
                }
                clReleaseEvent(evt_unmap);
                return cgSetupNewCompleteEvent(ctx, queue, event_handle, NULL, NULL, result);
            }
            else
            {   // the returned event will only be signaled when the data becomes available to OpenGL.
                // release the unmap event in this case, as the user will not wait on it at all.
                event = evt_relgl;
                clReleaseEvent(evt_unmap);
            }
        }
        return cgSetupNewCompleteEvent(ctx, queue, event_handle, event, NULL, CG_SUCCESS);
    }
    else
    {   // the queue type is not valid.
        if (event_handle != NULL)
        {   // no event object will be created or returned.
           *event_handle = CG_INVALID_HANDLE;
        }
        return CG_INVALID_VALUE;
    }
}

/// @summary Creates a new image object and allocates, but does not initialize, the backing memory.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param exec_group The execution group that will read or write the image.
/// @param pixel_width The width of the image, in pixels.
/// @param pixel_height The height of the image, in pixels.
/// @param slice_count The number of slices in the image. For 1D and 2D images, specify 1.
/// @param array_count The number of array elements in the image array. For non-array images, specify 1.
/// @param level_count The number of mipmap levels. For no mipmaps, specify 1. For all mipmap levels, specify 0.
/// @param pixel_format One of dxgi_format_e or DXGI_FORMAT specifying the data storage format.
/// @param kernel_types One or more of cg_memory_object_kernel_e specifying the types of kernels that will access the image.
/// @param kernel_access One or more of cg_memory_access_e specifying how the kernel(s) will access the image.
/// @param host_access One or more of cg_memory_access_e specifying how the host will access the image.
/// @param placement_hint One of cg_memory_placement_e specifying the heap the image should be allocated from. This is only a hint.
/// @param frequency_hint One of cg_memory_update_frequency_e specifying the expected image update frequency.
/// @param result On return, set to CG_SUCCESS, CG_NO_OPENGL, CG_BAD_GLCONTEXT, CG_BAD_CLCONTEXT, CG_OUT_OF_MEMORY, CG_INVALID_VALUE or CG_ERROR.
/// @return A handle to the new buffer object, or CG_INVALID_HANDLE.
library_function cg_handle_t
cgCreateImage
(
    uintptr_t    context,
    cg_handle_t  exec_group, 
    size_t       pixel_width, 
    size_t       pixel_height, 
    size_t       slice_count, 
    size_t       array_count, 
    size_t       level_count, 
    uint32_t     pixel_format,
    uint32_t     kernel_types,
    uint32_t     kernel_access,
    uint32_t     host_access,
    int          placement_hint,
    int          frequency_hint,
    int         &result
)
{   UNREFERENCED_PARAMETER(frequency_hint);
    CG_CONTEXT    *ctx   = (CG_CONTEXT*) context;
    CG_EXEC_GROUP *group =  cgObjectTableGet(&ctx->ExecGroupTable, exec_group);
    if (group == NULL)
    {   // invalid execution group handle.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }
    if ((group->RenderingContext == NULL) && (kernel_types & CG_MEMORY_OBJECT_KERNEL_GRAPHICS))
    {   // want OpenGL interop, but group has no OpenGL capability.
        result = CG_NO_OPENGL;
        return CG_INVALID_HANDLE;
    }
    if (slice_count < 1 || array_count < 1)
    {   // both of these values must be at least 1.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }
    if (slice_count > 1 && array_count > 1)
    {   // arrays of 3D images are not supported.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }
    if (pixel_format == DXGI_FORMAT_UNKNOWN)
    {   // the pixel format must be specified.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }
    if (host_access  == CG_MEMORY_ACCESS_NONE)
    {   // images with no need for host access are always placed in device memory.
        placement_hint = CG_MEMORY_PLACEMENT_DEVICE;
    }

    size_t max_mip_levels = cgGlLevelCount(pixel_width, pixel_height, slice_count, level_count);
    if (level_count == 0)   level_count  = max_mip_levels;
    if (level_count  > max_mip_levels)
    {   // the number of mip-levels is not valid.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }

    GLenum internal_format = GL_NONE;
    GLenum base_format     = GL_NONE;
    GLenum data_type       = GL_NONE;
    if (!cgGlDxgiFormatToGL(pixel_format, internal_format, base_format, data_type))
    {   // the pixel format is not valid.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }

    cl_image_desc            cl_desc;
    cl_image_format          cl_format;
    GLenum default_target  = GL_NONE;
    size_t padded_width    = cgGlImageDimension(internal_format, pixel_width);
    size_t padded_height   = cgGlImageDimension(internal_format, pixel_height);
    size_t row_pitch       = cgGlBytesPerRow(internal_format, data_type, padded_width, 4);
    size_t slice_pitch     = cgGlBytesPerSlice(internal_format, data_type, padded_width, padded_height, 4);
    if (!cgClMakeImageDesc(cl_desc, cl_format, default_target, pixel_format, padded_width, padded_height, slice_count, array_count, level_count, row_pitch, slice_pitch))
    {   // one or more arguments are invalid.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }

    if (kernel_types & CG_MEMORY_OBJECT_KERNEL_GRAPHICS)
    {   // the texture must be created in OpenGL first.
        CG_DISPLAY *display = group->AttachedDisplay;
        GLuint      texture = 0;
        size_t      nslices = 0;

        if (array_count > 1)
        {   // specify array_count as the slice_count argument to cgGlTextureStorage.
            nslices = array_count;
        }
        else
        {   // not an array texture, so use slice_count as-is.
            nslices = slice_count;
        }

        // generate an OpenGL texture object name.
        glGenTextures(1, &texture);
        if (texture == 0)
        {   // unable to allocate an OpenGL texture object name.
            result = CG_BAD_GLCONTEXT;
            return CG_INVALID_HANDLE;
        }

        // bind the texture to the default target, and allocate storage.
        // the texture is always allocated in device memory for OpenGL.
        glBindTexture(default_target, texture);
        if (level_count > 1)
        {   // default filtering with mipmaps.
            cgGlTextureStorage(
                display, default_target , internal_format, data_type, 
                GL_NEAREST_MIPMAP_LINEAR, /* minification filter  */ 
                GL_LINEAR_MIPMAP_LINEAR , /* magnification filter */
                padded_width, padded_height, nslices, level_count);
        }
        else
        {   // default filtering without mipmaps.
            cgGlTextureStorage(
                display, default_target, internal_format, data_type, 
                GL_NEAREST, /* minification filter  */
                GL_LINEAR , /* magnification filter */
                padded_width, padded_height, nslices, level_count);
        }
        glBindTexture(default_target, 0);

        // populate the image description with OpenGL data.
        // OpenGL textures are always allocated in device memory.
        CG_IMAGE image;
        image.KernelTypes      = kernel_types;
        image.KernelAccess     = kernel_access;
        image.HostAccess       = host_access;
        image.AttachedDisplay  = NULL;
        image.SourceHeap       = cgFindHeapForPlacement(ctx, CG_MEMORY_PLACEMENT_DEVICE);
        image.ExecutionGroup   = exec_group;
        image.ComputeContext   = NULL;
        image.ComputeImage     = NULL;
        image.ComputeUsage     = 0;
        image.ImageWidth       = pixel_width;
        image.ImageHeight      = pixel_height;
        image.SliceCount       = slice_count;
        image.ArrayCount       = array_count;
        image.LevelCount       = level_count;
        image.PaddedWidth      = padded_width;
        image.PaddedHeight     = padded_height;
        image.RowPitch         = row_pitch;
        image.SlicePitch       = slice_pitch;
        image.GraphicsImage    = texture;
        image.DefaultTarget    = default_target;
        image.InternalFormat   = internal_format;
        image.BaseFormat       = base_format;
        image.DataType         = data_type;
        image.DxgiFormat       = pixel_format;

        // set up OpenCL sharing, if requested.
        if (kernel_types & CG_MEMORY_OBJECT_KERNEL_COMPUTE)
        {
            cl_mem_flags  cl_flags = 0;
            cl_mem        clmem    = NULL;
            cl_int        clres    = CL_SUCCESS;
            uint32_t      access   =(kernel_access & ~CG_MEMORY_ACCESS_PRESERVE);

            // convert kernel access flags into cl_mem_flags.
            // OpenCL images created from OpenGL textures cannot specify ALLOC_HOST_PTR, etc.
                 if (access == CG_MEMORY_ACCESS_READ      ) cl_flags = CL_MEM_READ_ONLY;
            else if (access == CG_MEMORY_ACCESS_WRITE     ) cl_flags = CL_MEM_WRITE_ONLY;
            else if (access == CG_MEMORY_ACCESS_READ_WRITE) cl_flags = CL_MEM_READ_WRITE;
            else
            {   // CG_MEMORY_ACCESS_NONE is not valid.
                glDeleteTextures(1, &texture);
                result = CG_INVALID_VALUE;
                return CG_INVALID_HANDLE;
            }
            if ((clmem = clCreateFromGLTexture(group->ComputeContext, cl_flags, default_target, 0, texture, &clres)) == NULL)
            {
                switch (clres)
                {
                case CL_INVALID_CONTEXT                : result = CG_BAD_CLCONTEXT; break;
                case CL_INVALID_VALUE                  : result = CG_INVALID_VALUE; break;
                case CL_INVALID_MIP_LEVEL              : result = CG_INVALID_VALUE; break;
                case CL_INVALID_GL_OBJECT              : result = CG_BAD_GLCONTEXT; break;
                case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR: result = CG_INVALID_VALUE; break;
                case CL_OUT_OF_RESOURCES               : result = CG_OUT_OF_MEMORY; break;
                case CL_OUT_OF_HOST_MEMORY             : result = CG_OUT_OF_MEMORY; break;
                default                                : result = CG_ERROR;         break;
                }
                glDeleteTextures(1, &texture);
                return CG_INVALID_HANDLE;
            }
            image.ComputeContext = group->ComputeContext;
            image.ComputeImage   = clmem;
            image.ComputeUsage   = cl_flags;
        }

        cg_handle_t handle = cgObjectTableAdd(&ctx->ImageTable, image);
        if (handle == CG_INVALID_HANDLE)
        {
            if (image.ComputeImage != NULL) clReleaseMemObject(image.ComputeImage);
            glDeleteBuffers(1, &image.GraphicsImage);
            result = CG_OUT_OF_OBJECTS;
            return CG_INVALID_HANDLE;
        }
        // TODO(rlk): update CG_HEAP::HeapSizeUsed.
        result = CG_SUCCESS;
        return handle;
    }
    else if (kernel_types & CG_MEMORY_OBJECT_KERNEL_COMPUTE)
    {   // compute kernel data only.
        cl_mem_flags  cl_flags = 0;
        cl_mem        clmem    = NULL;
        cl_int        clres    = CL_SUCCESS;
        uint32_t      access   =(kernel_access & ~CG_MEMORY_ACCESS_PRESERVE);

        // convert kernel access flags into cl_mem_flags.
             if (access == CG_MEMORY_ACCESS_READ      ) cl_flags = CL_MEM_READ_ONLY;
        else if (access == CG_MEMORY_ACCESS_WRITE     ) cl_flags = CL_MEM_WRITE_ONLY;
        else if (access == CG_MEMORY_ACCESS_READ_WRITE) cl_flags = CL_MEM_READ_WRITE;
        else
        {   // CG_MEMORY_ACCESS_NONE is not valid.
            result = CG_INVALID_VALUE;
            return CG_INVALID_HANDLE;
        }
        if (placement_hint != CG_MEMORY_PLACEMENT_DEVICE)
        {   // prefer to allocate the buffer in host or pinned memory.
            // there are limitations on buffer size with pinned memory.
            cl_flags |= CL_MEM_ALLOC_HOST_PTR;
        }
        if (host_access == CG_MEMORY_ACCESS_NONE)
        {   // the host promises not to map the memory. attempts to do so will fail.
            cl_flags |= CL_MEM_HOST_NO_ACCESS;
        }
        if ((clmem = clCreateImage(group->ComputeContext, cl_flags, &cl_format, &cl_desc, NULL, &clres)) == NULL)
        {
            switch (clres)
            {
            case CL_INVALID_CONTEXT              : result = CG_BAD_CLCONTEXT; break;
            case CL_INVALID_VALUE                : result = CG_INVALID_VALUE; break;
            case CL_INVALID_BUFFER_SIZE          : result = CG_INVALID_VALUE; break;
            case CL_INVALID_HOST_PTR             : result = CG_INVALID_VALUE; break;
            case CL_MEM_OBJECT_ALLOCATION_FAILURE: result = CG_OUT_OF_MEMORY; break;
            case CL_OUT_OF_HOST_MEMORY           : result = CG_OUT_OF_MEMORY; break;
            default                              : result = CG_ERROR;         break;
            }
            return CG_INVALID_HANDLE;
        }

        CG_IMAGE image;
        image.KernelTypes      = kernel_types;
        image.KernelAccess     = kernel_access;
        image.HostAccess       = host_access;
        image.AttachedDisplay  = NULL;
        image.SourceHeap       = cgFindHeapForPlacement(ctx, placement_hint);
        image.ExecutionGroup   = exec_group;
        image.ComputeContext   = group->ComputeContext;
        image.ComputeImage     = clmem;
        image.ComputeUsage     = cl_flags;
        image.ImageWidth       = pixel_width;
        image.ImageHeight      = pixel_height;
        image.SliceCount       = slice_count;
        image.ArrayCount       = array_count;
        image.LevelCount       = level_count;
        image.PaddedWidth      = padded_width;
        image.PaddedHeight     = padded_height;
        image.RowPitch         = row_pitch;
        image.SlicePitch       = slice_pitch;
        image.GraphicsImage    = 0;
        image.DefaultTarget    = default_target;
        image.InternalFormat   = internal_format;
        image.BaseFormat       = base_format;
        image.DataType         = data_type;
        image.DxgiFormat       = pixel_format;

        cg_handle_t handle     = cgObjectTableAdd(&ctx->ImageTable, image);
        if (handle == CG_INVALID_HANDLE)
        {
            clReleaseMemObject(image.ComputeImage);
            result = CG_OUT_OF_OBJECTS;
            return CG_INVALID_HANDLE;
        }
        // TODO(rlk): update CG_HEAP::HeapSizeUsed.
        result = CG_SUCCESS;
        return handle;
    }
    else
    {   // invalid kernel_types.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }
}

/// @summary Query image information.
/// @param context A CGFX context returned by cgEnumerateDevices().
/// @param image_handle The handle of the image to query.
/// @param param One of cg_image_info_param_e specifying the data to return.
/// @param buffer A caller-managed buffer to receive the data.
/// @param buffer_size The maximum number of bytes that can be written to @a buffer.
/// @param bytes_needed On return, stores the number of bytes copied to the buffer, or the number of bytes required to store the data.
/// @return CG_SUCCESS, CG_BUFFER_TOO_SMALL, CG_INVALID_VALUE or CG_OUT_OF_MEMORY.
library_function int
cgGetImageInfo
(
    uintptr_t   context,
    cg_handle_t image_handle,
    int         param,
    void       *buffer,
    size_t      buffer_size,
    size_t     *bytes_needed
)
{
    CG_CONTEXT *ctx    = (CG_CONTEXT*) context;
    CG_IMAGE   *object =  NULL;

    if ((object = cgObjectTableGet(&ctx->ImageTable, image_handle)) == NULL)
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
#define BUFFER_SET_SCALAR(type, value) \
    *((type*)buffer) = (value)
    // ==========================================================
    // ==========================================================

    // retrieve parameter data:
    switch (param)
    {
    case CG_IMAGE_HEAP_ORDINAL:
        {   BUFFER_CHECK_TYPE(size_t);
            BUFFER_SET_SCALAR(size_t, object->SourceHeap->Ordinal);
        }
        return CG_SUCCESS;

    case CG_IMAGE_HEAP_TYPE:
        {   BUFFER_CHECK_TYPE(int);
            BUFFER_SET_SCALAR(int, object->SourceHeap->Type);
        }
        return CG_SUCCESS;

    case CG_IMAGE_HEAP_FLAGS:
        {   BUFFER_CHECK_TYPE(uint32_t);
            BUFFER_SET_SCALAR(uint32_t, object->SourceHeap->Flags);
        }
        return CG_SUCCESS;

    case CG_IMAGE_ALLOCATED_SIZE:
        {   BUFFER_CHECK_TYPE(size_t);
            BUFFER_SET_SCALAR(size_t, cgCalculateImageAllocatedSize(object));
        }
        return CG_SUCCESS;

    case CG_IMAGE_DIMENSIONS:
        {   BUFFER_CHECK_SIZE(sizeof(size_t) * 3);
            size_t whd[3] = { object->PaddedWidth, object->PaddedHeight, object->SliceCount };
            memcpy(buffer, whd, sizeof(size_t) * 3);
        }
        return CG_SUCCESS;

    case CG_IMAGE_ARRAY_SIZE:
        {   BUFFER_CHECK_TYPE(size_t);
            BUFFER_SET_SCALAR(size_t, object->ArrayCount);
        }
        return CG_SUCCESS;

    case CG_IMAGE_MIPMAP_LEVELS:
        {   BUFFER_CHECK_TYPE(size_t);
            BUFFER_SET_SCALAR(size_t, object->LevelCount);
        }
        return CG_SUCCESS;

    case CG_IMAGE_PIXEL_FORMAT:
        {   BUFFER_CHECK_TYPE(uint32_t);
            BUFFER_SET_SCALAR(uint32_t, object->DxgiFormat);
        }
        return CG_SUCCESS;

    case CG_IMAGE_DDS_HEADER:
        {   BUFFER_CHECK_TYPE(dds_header_t);
            // TODO(rlk): implement this.
        }
        return CG_SUCCESS;

    case CG_IMAGE_DDS_HEADER_DX10:
        {   BUFFER_CHECK_TYPE(dds_header_dxt10_t);
            // TODO(rlk): implement this.
        }
        return CG_SUCCESS;

    default:
        {
            if (bytes_needed != NULL) *bytes_needed = 0;
            return CG_INVALID_VALUE;
        }
    }

#undef  BUFFER_SET_SCALAR
#undef  BUFFER_CHECK_SIZE
#undef  BUFFER_CHECK_TYPE
}

/// @summary Map a region of an image object into the host address space. If the host is reading the data, a data transfer from device to host may be performed. Map operations always block until the required data is available.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param queue_handle The queue on which the transfer will be performed.
/// @param image_handle The handle of the image object to map.
/// @param wait_handle The handle of a fence or event object to wait on before mapping the image, or CG_INVALID_HANDLE if the caller asserts that no device is currently accessing the memory object.
/// @param xyz The x-coordinate, y-coordinate and slice or image index of the upper-left corner of the region to map.
/// @param whd The width (in pixels), height (in pixels) and depth (in slices or images) of the region to map.
/// @param flags A combination of cg_memory_access_e specifying mapping options. CG_MEMORY_ACCESS_NONE is not valid.
/// @param row_pitch On return, specifies the number of bytes between successive rows in the mapped region.
/// @param slice_pitch On return, specifies the number of bytes between successive slices in the mapped region.
/// @param result On return, set to CG_SUCCESS, CG_INVALID_VALUE, CG_INVALID_STATE, CG_OUT_OF_MEMORY, CG_BAD_CLCONTEXT or CG_ERROR.
/// @return A host-accessible pointer to the specified region of the image, or NULL.
library_function void*
cgMapImageRegion
(
    uintptr_t   context,
    cg_handle_t queue_handle,
    cg_handle_t image_handle,
    cg_handle_t wait_handle,
    size_t      xyz[3], 
    size_t      whd[3],
    uint32_t    flags,
    size_t     &row_pitch, 
    size_t     &slice_pitch,
    int        &result
)
{
    CG_CONTEXT *ctx   = (CG_CONTEXT*) context;
    CG_QUEUE   *queue = (CG_QUEUE  *) cgObjectTableGet(&ctx->QueueTable, queue_handle);
    CG_IMAGE   *obj   = (CG_IMAGE  *) cgObjectTableGet(&ctx->ImageTable, image_handle);
    if (obj == NULL || queue == NULL || flags == CG_MEMORY_ACCESS_NONE)
    {   // one or more of the supplied handles is invalid.
        result = CG_INVALID_VALUE;
        return NULL;
    }
    // mapping the buffer for read access performs a blocking transfer to the host.
    // mapping the buffer for write access returns a pointer to pinned memory.
    if (queue->QueueType & CG_QUEUE_TYPE_TRANSFER)
    {   // map the buffer using OpenCL.
        cl_map_flags map_flags  = 0;
        cl_event     wait_event = NULL;
        cl_uint      wait_count = 0;
        cl_int       clres      = CL_SUCCESS;
        void        *mapped     = NULL;
        bool         release_ev = false;

        if ((result = cgGetWaitEvent(ctx, queue, wait_handle, &wait_event, wait_count, 1, release_ev)) != CG_SUCCESS)
        {   // some problem with the event or fence we're told to wait on.
            return NULL;
        }

        if (flags & CG_MEMORY_ACCESS_READ)
            map_flags |= CL_MAP_READ;
        if (flags & CG_MEMORY_ACCESS_WRITE)
            map_flags |= CL_MAP_WRITE;
        if ((flags & CG_MEMORY_ACCESS_READ) == 0 && (flags & CG_MEMORY_ACCESS_PRESERVE) == 0)
            map_flags  = CL_MAP_WRITE_INVALIDATE_REGION;

        // fix up the values in xyz and whd to match what OpenCL expects.
        switch (obj->DefaultTarget)
        {
        case GL_TEXTURE_1D:
            xyz[1] = xyz[2] = 0;
            whd[1] = whd[2] = 1;
            break;
        case GL_TEXTURE_1D_ARRAY:
            xyz[1] = xyz[2]; // set xyz[1] to the array index
            xyz[2] = 0;      // OpenCL expects xyz[2] to be 0
            whd[2] = 1;      // OpenCL expects whd[2] to be 1
            break;
        case GL_TEXTURE_2D:
            xyz[2] = 0;      // OpenCL expects xyz[2] to be 0
            whd[2] = 1;      // OpenCL expects whd[2] to be 1
            break;
        default:
            break;
        }

        if (obj->GraphicsImage != 0)
        {   // the buffer first needs to be acquired by OpenCL for use.
            cl_event gl_wait = NULL;
            if ((clres = clEnqueueAcquireGLObjects(queue->CommandQueue, 1, &obj->ComputeImage, wait_count, CG_OPENCL_WAIT_LIST(wait_count, &wait_event), &gl_wait)) != CL_SUCCESS)
            {
                switch (clres)
                {
                case CL_INVALID_VALUE          : result = CG_INVALID_VALUE; break;
                case CL_INVALID_MEM_OBJECT     : result = CG_INVALID_VALUE; break;
                case CL_INVALID_COMMAND_QUEUE  : result = CG_INVALID_VALUE; break;
                case CL_INVALID_CONTEXT        : result = CG_BAD_CLCONTEXT; break;
                case CL_INVALID_GL_OBJECT      : result = CG_INVALID_VALUE; break;
                case CL_INVALID_EVENT_WAIT_LIST: result = CG_INVALID_VALUE; break;
                case CL_OUT_OF_RESOURCES       : result = CG_OUT_OF_MEMORY; break;
                case CL_OUT_OF_HOST_MEMORY     : result = CG_OUT_OF_MEMORY; break;
                default                        : result = CG_ERROR;         break;
                }
                if (release_ev)
                {   // release any temporary event object.
                    clReleaseEvent(wait_event);
                }
                return NULL;
            }
            // the acquire request was enqueued successfully. overwrite the wait_event 
            // so that the map request waits until the acquire has completed.
            if (release_ev)
            {   // release any temporary event object.
                clReleaseEvent(wait_event);
            }
            wait_count = 1;
            wait_event = gl_wait;
            release_ev = true; // release gl_wait after the image map is enqueued.
        }

        if ((mapped = clEnqueueMapImage(queue->CommandQueue, obj->ComputeImage, CL_TRUE, map_flags, xyz, whd, &row_pitch, &slice_pitch, wait_count, CG_OPENCL_WAIT_LIST(wait_count, &wait_event), NULL, &clres)) == NULL)
        {
            switch (clres)
            {
            case CL_INVALID_COMMAND_QUEUE                    : result = CG_INVALID_VALUE; break;
            case CL_INVALID_CONTEXT                          : result = CG_BAD_CLCONTEXT; break;
            case CL_INVALID_MEM_OBJECT                       : result = CG_INVALID_VALUE; break;
            case CL_INVALID_VALUE                            : result = CG_INVALID_VALUE; break;
            case CL_INVALID_EVENT_WAIT_LIST                  : result = CG_INVALID_VALUE; break;
            case CL_INVALID_IMAGE_SIZE                       : result = CG_INVALID_VALUE; break;
            case CL_MAP_FAILURE                              : result = CG_ERROR;         break;
            case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST: result = CG_INVALID_VALUE; break;
            case CL_MEM_OBJECT_ALLOCATION_FAILURE            : result = CG_OUT_OF_MEMORY; break;
            case CL_INVALID_OPERATION                        : result = CG_INVALID_STATE; break;
            case CL_OUT_OF_RESOURCES                         : result = CG_OUT_OF_MEMORY; break;
            case CL_OUT_OF_HOST_MEMORY                       : result = CG_OUT_OF_MEMORY; break;
            default                                          : result = CG_ERROR;         break;
            }
            if (release_ev)
            {   // release any temporary event object.
                clReleaseEvent(wait_event);
            }
            return NULL;
        }
        if (release_ev)
        {   // release any temporary event object.
            clReleaseEvent(wait_event);
        }
        result = CG_SUCCESS;
        return mapped;
    }
    else
    {   // the queue type is not valid.
        result = CG_INVALID_VALUE;
        return NULL;
    }
}

/// @summary Unmap a region of a mapped image. If the host was writing data to the image, a data transfer from host to device may be performed. Unmap operations are always non-blocking.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param queue_handle The queue on which the transfer will be performed. This must be the same queue that was supplied to cgMapDataBuffer.
/// @param image_handle The handle of the mapped image object.
/// @param mapped_region The pointer returned by a call to cgMapImageRegion.
/// @param event_handle On return, if non-NULL, stores a handle to an event object that can be used to block the host or device until the data transfer has completed.
library_function int
cgUnmapImageRegion
(
    uintptr_t    context,
    cg_handle_t  queue_handle,
    cg_handle_t  image_handle,
    void        *mapped_region,
    cg_handle_t *event_handle
)
{
    CG_CONTEXT *ctx   = (CG_CONTEXT*) context;
    CG_QUEUE   *queue = (CG_QUEUE  *) cgObjectTableGet(&ctx->QueueTable, queue_handle);
    CG_IMAGE   *obj   = (CG_IMAGE  *) cgObjectTableGet(&ctx->ImageTable, image_handle);
    int        result =  CG_SUCCESS;
    if (obj == NULL || queue == NULL)
    {   // one or more of the supplied handles is invalid.
        if (event_handle != NULL)
        {   // no command was submitted, so there's no completion event to return.
           *event_handle = CG_INVALID_HANDLE;
        }
        return CG_INVALID_VALUE;
    }

    if (queue->QueueType & CG_QUEUE_TYPE_TRANSFER)
    {   
        cl_int        clres = CL_SUCCESS;
        cl_event  evt_unmap = NULL;
        cl_event  evt_relgl = NULL;
        cl_event  event     = NULL;
        
        // unmap the image region from the host address space. if the region was mapped for write 
        // access, a data transfer from host to device may be performed (hopefully asynchronously.)
        if ((clres = clEnqueueUnmapMemObject(queue->CommandQueue, obj->ComputeImage, mapped_region, 0, NULL, &evt_unmap)) != CL_SUCCESS)
        {
            switch (clres)
            {
            case CL_INVALID_VALUE          : result = CG_INVALID_VALUE; break;
            case CL_INVALID_MEM_OBJECT     : result = CG_INVALID_VALUE; break;
            case CL_INVALID_COMMAND_QUEUE  : result = CG_INVALID_VALUE; break;
            case CL_INVALID_CONTEXT        : result = CG_BAD_CLCONTEXT; break;
            case CL_INVALID_EVENT_WAIT_LIST: result = CG_INVALID_VALUE; break;
            case CL_OUT_OF_RESOURCES       : result = CG_OUT_OF_MEMORY; break;
            case CL_OUT_OF_HOST_MEMORY     : result = CG_OUT_OF_MEMORY; break;
            default                        : result = CG_ERROR;         break;
            }
            return cgSetupNewCompleteEvent(ctx, queue, event_handle, NULL, NULL, result);
        }

        // set the compute event handle to the unmap completion event.
        event = evt_unmap;

        if (clres == CL_SUCCESS && obj->GraphicsImage != 0)
        {   // release the buffer so it can be used by OpenGL again. this must wait for the data 
            // transfer, possibly performed by the unmap operation, to complete.
            if ((clres = clEnqueueReleaseGLObjects(queue->CommandQueue, 1, &obj->ComputeImage, 1, &evt_unmap, &evt_relgl)) != CL_SUCCESS)
            {
                switch (clres)
                {
                case CL_INVALID_VALUE          : result = CG_INVALID_VALUE; break;
                case CL_INVALID_MEM_OBJECT     : result = CG_INVALID_VALUE; break;
                case CL_INVALID_COMMAND_QUEUE  : result = CG_INVALID_VALUE; break;
                case CL_INVALID_CONTEXT        : result = CG_BAD_CLCONTEXT; break;
                case CL_INVALID_GL_OBJECT      : result = CG_INVALID_VALUE; break;
                case CL_INVALID_EVENT_WAIT_LIST: result = CG_INVALID_VALUE; break;
                case CL_OUT_OF_RESOURCES       : result = CG_OUT_OF_MEMORY; break;
                case CL_OUT_OF_HOST_MEMORY     : result = CG_OUT_OF_MEMORY; break;
                default                        : result = CG_ERROR;         break;
                }
                clReleaseEvent(evt_unmap);
                return cgSetupNewCompleteEvent(ctx, queue, event_handle, NULL, NULL, result);
            }
            else
            {   // the returned event will only be signaled when the data becomes available to OpenGL.
                // release the unmap event in this case, as the user will not wait on it at all.
                event = evt_relgl;
                clReleaseEvent(evt_unmap);
            }
        }
        return cgSetupNewCompleteEvent(ctx, queue, event_handle, event, NULL, CG_SUCCESS);
    }
    else
    {   // the queue type is not valid.
        if (event_handle != NULL)
        {   // no event object will be created or returned.
           *event_handle = CG_INVALID_HANDLE;
        }
        return CG_INVALID_VALUE;
    }
}

/// @summary Creates a new image sampler object.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param exec_group The handle of the execution group defining devices that will access the sampler.
/// @param create_info Attributes specifying the image sampler configuration.
/// @param result On return, set to CG_SUCCESS or ...
/// @return A handle to the new image sampler object, or CG_INVALID_HANDLE.
library_function cg_handle_t
cgCreateImageSampler
(
    uintptr_t                 context, 
    uintptr_t                 exec_group, 
    cg_image_sampler_t const *create_info, 
    int                      &result
)
{
    CG_CONTEXT    *ctx   = (CG_CONTEXT*) context;
    CG_EXEC_GROUP *group =  cgObjectTableGet(&ctx->ExecGroupTable, exec_group);
    if (group == NULL)
    {   // invalid execution group handle.
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }

    CG_DISPLAY        *display     = group->AttachedDisplay;
    GLenum             gl_target   = GL_NONE;
    GLenum             gl_minfilter= GL_NEAREST;
    GLenum             gl_magfilter= GL_LINEAR;
    GLenum             gl_address  = GL_CLAMP_TO_EDGE;
    cl_bool            cl_norm     = CL_TRUE;
    cl_filter_mode     cl_filter   = CL_FILTER_LINEAR;
    cl_addressing_mode cl_address  = CL_ADDRESS_CLAMP_TO_EDGE;
    uint32_t           flags       = create_info->Flags;
    CG_SAMPLER         sampler;

    if (flags == CG_IMAGE_SAMPLER_FLAGS_NONE)
    {   // use the default configuration, compatible with both OpenCL and OpenGL.
        flags  = CG_IMAGE_SAMPLER_FLAG_COMPUTE | CG_IMAGE_SAMPLER_FLAG_GRAPHICS;
    }

    // figure out the OpenGL and OpenCL enum values.
    if ((create_info->Flags & CG_IMAGE_SAMPLER_FLAG_GRAPHICS) == 0)
    {   // the sampler will be used for compute only.
        switch (create_info->Type)
        {
        case CG_IMAGE_SAMPLER_1D:
            gl_target = GL_TEXTURE_1D;
            cl_norm   = CL_TRUE;
            break;
        case CG_IMAGE_SAMPLER_1D_ARRAY:
            gl_target = GL_TEXTURE_1D_ARRAY;
            cl_norm   = CL_TRUE;
            break;
        case CG_IMAGE_SAMPLER_2D:
            gl_target = GL_TEXTURE_2D;
            cl_norm   = CL_TRUE;
            break;
        case CG_IMAGE_SAMPLER_2D_RECT:
            gl_target = GL_TEXTURE_RECTANGLE;
            cl_norm   = CL_FALSE;
            break;
        case CG_IMAGE_SAMPLER_2D_ARRAY:
            gl_target = GL_TEXTURE_2D_ARRAY;
            cl_norm   = CL_TRUE;
            break;
        case CG_IMAGE_SAMPLER_3D:
            gl_target = GL_TEXTURE_3D;
            cl_norm   = CL_TRUE;
            break;
        default:
            result = CG_INVALID_VALUE;
            return CG_INVALID_HANDLE;
        }
    }
    else
    {   // the sampler will be used for graphics, so enable support for additional sampler types.
        switch (create_info->Type)
        {
        case CG_IMAGE_SAMPLER_1D:
            gl_target = GL_TEXTURE_1D;
            cl_norm   = CL_TRUE;
            break;
        case CG_IMAGE_SAMPLER_1D_ARRAY:
            gl_target = GL_TEXTURE_1D_ARRAY;
            cl_norm   = CL_TRUE;
            break;
        case CG_IMAGE_SAMPLER_2D:
            gl_target = GL_TEXTURE_2D;
            cl_norm   = CL_TRUE;
            break;
        case CG_IMAGE_SAMPLER_2D_RECT:
            gl_target = GL_TEXTURE_RECTANGLE;
            cl_norm   = CL_FALSE;
            break;
        case CG_IMAGE_SAMPLER_2D_ARRAY:
            gl_target = GL_TEXTURE_2D_ARRAY;
            cl_norm   = CL_TRUE;
            break;
        case CG_IMAGE_SAMPLER_3D:
            gl_target = GL_TEXTURE_3D;
            cl_norm   = CL_TRUE;
            break;
        case CG_IMAGE_SAMPLER_CUBEMAP:
            gl_target = GL_TEXTURE_CUBE_MAP;
            cl_norm   = CL_TRUE;
            break;
#if CG_OPENGL_VERSION >= CG_OPENGL_VERSION_40
        case CG_IMAGE_SAMPLER_CUBEMAP_ARRAY:
            gl_target = GL_TEXTURE_CUBE_MAP_ARRAY;
            cl_norm   = CL_TRUE;
            break;
#else
        case CG_IMAGE_SAMPLER_CUBEMAP_ARRAY:
            if (GLEW_ARB_texture_cube_map_array)
            {
                gl_target = GL_TEXTURE_CUBE_MAP_ARRAY_ARB;
                cl_norm   = CL_TRUE;
            }
            else
            {   // the required extension is not supported by the current context.
                result = CG_INVALID_VALUE;
                return CG_INVALID_HANDLE;
            }
            break;
#endif
        case CG_IMAGE_SAMPLER_BUFFER:
            gl_target = GL_TEXTURE_BUFFER;
            cl_norm   = CL_FALSE;
            break;
        default:
            result = CG_INVALID_VALUE;
            return CG_INVALID_HANDLE;
        }
    }

    switch (create_info->FilterMode)
    {
    case CG_IMAGE_FILTER_NEAREST:
        if (create_info->Flags & CG_IMAGE_SAMPLER_FLAG_MIPMAPPED)
        {
            gl_minfilter = GL_NEAREST_MIPMAP_LINEAR;
            gl_magfilter = GL_NEAREST;
            cl_filter    = CL_FILTER_NEAREST;
        }
        else
        {
            gl_minfilter = GL_NEAREST;
            gl_magfilter = GL_NEAREST;
            cl_filter    = CL_FILTER_NEAREST;
        }
        break;
    case CG_IMAGE_FILTER_LINEAR:
        if (create_info->Flags & CG_IMAGE_SAMPLER_FLAG_MIPMAPPED)
        {
            gl_minfilter = GL_NEAREST_MIPMAP_LINEAR;
            gl_magfilter = GL_LINEAR_MIPMAP_LINEAR;
            cl_filter    = CL_FILTER_LINEAR;
        }
        else
        {
            gl_minfilter = GL_NEAREST;
            gl_magfilter = GL_LINEAR;
            cl_filter    = CL_FILTER_LINEAR;
        }
        break;
    default:
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }

    switch (create_info->AddressMode)
    {
    case CG_IMAGE_ADDRESS_UNDEFINED:
        gl_address = GL_CLAMP;
        cl_address = CL_ADDRESS_NONE;
        break;
    case CG_IMAGE_ADDRESS_BORDER_COLOR:
        gl_address = GL_CLAMP_TO_BORDER;
        cl_address = CL_ADDRESS_CLAMP;
        break;
    case CG_IMAGE_ADDRESS_CLAMP_TO_EDGE:
        gl_address = GL_CLAMP_TO_EDGE;
        cl_address = CL_ADDRESS_CLAMP_TO_EDGE;
        break;
    case CL_IMAGE_ADDRESS_REPEAT:
        gl_address = GL_REPEAT;
        cl_address = CL_ADDRESS_REPEAT;
        break;
    case CL_IMAGE_ADDRESS_REFLECT:
        gl_address = GL_MIRRORED_REPEAT;
        cl_address = CL_ADDRESS_MIRRORED_REPEAT;
        break;
    default:
        result = CG_INVALID_VALUE;
        return CG_INVALID_HANDLE;
    }

    // fill out the basic sampler attributes.
    sampler.GraphicsSampler = 0;
    sampler.TextureTarget   = gl_target;
    sampler.MinFilter       = gl_minfilter;
    sampler.MagFilter       = gl_magfilter;
    sampler.AddressMode     = gl_address;
    sampler.AttachedDisplay = display;
    sampler.ComputeSampler  = NULL;

    // create the OpenCL sampler object.
    if (create_info->Flags & CG_IMAGE_SAMPLER_FLAG_COMPUTE)
    {   // TODO(rlk): In OpenCL 2.0, clCreateSampler has been marked deprecated.
        // it has been replaced with clCreateSamplerWithProperties.
        cl_int clres = CL_SUCCESS;
        if ((sampler.ComputeSampler = clCreateSampler(group->ComputeContext, cl_norm, cl_address, cl_filter, &clres)) == NULL)
        {
            switch (clres)
            {
            case CL_INVALID_CONTEXT   : result = CG_BAD_CLCONTEXT; break;
            case CL_INVALID_VALUE     : result = CG_INVALID_VALUE; break;
            case CL_INVALID_OPERATION : result = CG_UNSUPPORTED;   break;
            case CL_OUT_OF_RESOURCES  : result = CG_OUT_OF_MEMORY; break;
            case CL_OUT_OF_HOST_MEMORY: result = CG_OUT_OF_MEMORY; break;
            default: result = CG_ERROR; break;
            }
        }
    }
    if (create_info->Flags & CG_IMAGE_SAMPLER_FLAG_GRAPHICS)
    {   // this requires ARB_sampler_objects support. if this extension is not 
        // available, we'll fall back and just set the sampler properties each 
        // time the associated texture is bound.
        if (GLEW_ARB_sampler_objects)
        {
            GLuint glsamp = 0;
            glGenSamplers(1, &glsamp);
            if (glsamp == 0)
            {
                if (sampler.ComputeSampler != NULL) clReleaseSampler(sampler.ComputeSampler);
                result = CG_BAD_GLCONTEXT;
                return CG_INVALID_HANDLE;
            }
            glSamplerParameteri(glsamp, GL_TEXTURE_MIN_FILTER, gl_minfilter);
            glSamplerParameteri(glsamp, GL_TEXTURE_MAG_FILTER, gl_magfilter);
            glSamplerParameteri(glsamp, GL_TEXTURE_WRAP_S    , gl_address);
            glSamplerParameteri(glsamp, GL_TEXTURE_WRAP_T    , gl_address);
            glSamplerParameteri(glsamp, GL_TEXTURE_WRAP_R    , gl_address);
            sampler.GraphicsSampler   = glsamp;
        }
    }

    cg_handle_t handle = cgObjectTableAdd(&ctx->SamplerTable, sampler);
    if (handle == CG_INVALID_HANDLE)
    {
        if (sampler.GraphicsSampler != 0   ) glDeleteSamplers(1, &sampler.GraphicsSampler);
        if (sampler.ComputeSampler  != NULL) clReleaseSampler(sampler.ComputeSampler);
        result = CG_OUT_OF_OBJECTS;
        return CG_INVALID_HANDLE;
    }
    return handle;
}

/// @summary Copies the entire contents of one buffer to another.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param cmd_buffer The handle of the destination command buffer.
/// @param dst_buffer The handle of the target buffer.
/// @param dst_offset The offset of the first byte to write in the target buffer.
/// @param src_buffer The handle of the source buffer.
/// @param done_event The handle of an event to signal when the transfer has completed, or CG_INVALID_HANDLE.
/// @param wait_event The handle of an event to wait for before beginning the transfer, or CG_INVALID_HANDLE.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_INVALID_STATE, CG_BUFFER_TOO_SMALL or CG_OUT_OF_MEMORY.
library_function int
cgCopyBuffer
(
    uintptr_t   context, 
    cg_handle_t cmd_buffer, 
    cg_handle_t dst_buffer, 
    size_t      dst_offset,
    cg_handle_t src_buffer, 
    cg_handle_t done_event, 
    cg_handle_t wait_event
)
{
    CG_CONTEXT    *ctx    = (CG_CONTEXT*) context;
    CG_BUFFER     *dstbuf =  cgObjectTableGet(&ctx->BufferTable, dst_buffer);
    CG_BUFFER     *srcbuf =  cgObjectTableGet(&ctx->BufferTable, src_buffer);
    if ((srcbuf == NULL) || (dstbuf == NULL) || (srcbuf == dstbuf))
    {   // one or both buffer objects are invalid, or the same buffer.
        return CG_INVALID_VALUE;
    }
    if ((dst_offset + srcbuf->RequestedSize) > dstbuf->RequestedSize)
    {   // the destination buffer is not large enough, or the offset is invalid.
        return CG_INVALID_VALUE;
    }

    cg_copy_buffer_cmd_t cmd;
    cmd.WaitEvent      = wait_event;
    cmd.CompleteEvent  = done_event;
    cmd.SourceBuffer   = src_buffer;
    cmd.TargetBuffer   = dst_buffer;
    cmd.SourceOffset   = 0;
    cmd.TargetOffset   = dst_offset;
    cmd.CopyAmount     = srcbuf->RequestedSize;
    return cgCommandBufferAppend(context, cmd_buffer, CG_COMMAND_COPY_BUFFER, sizeof(cmd), &cmd);
}

/// @summary Copies a region of one buffer to another.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param dst_buffer The handle of the target buffer.
/// @param dst_offset The offset of the first byte to write in the target buffer.
/// @param src_buffer The handle of the source buffer.
/// @param src_offset The offset of the first byte to read from the source buffer.
/// @param copy_amount The number of bytes to copy.
/// @param done_event The handle of an event to signal when the transfer has completed, or CG_INVALID_HANDLE.
/// @param wait_event The handle of an event to wait for before beginning the transfer, or CG_INVALID_HANDLE.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_INVALID_STATE, CG_BUFFER_TOO_SMALL or CG_OUT_OF_MEMORY.
library_function int
cgCopyBufferRegion
(
    uintptr_t   context, 
    cg_handle_t cmd_buffer, 
    cg_handle_t dst_buffer, 
    size_t      dst_offset,
    cg_handle_t src_buffer,
    size_t      src_offset, 
    size_t      copy_amount,
    cg_handle_t done_event, 
    cg_handle_t wait_event
)
{
    CG_CONTEXT *ctx    = (CG_CONTEXT*) context;
    CG_BUFFER  *dstbuf =  cgObjectTableGet(&ctx->BufferTable, dst_buffer);
    CG_BUFFER  *srcbuf =  cgObjectTableGet(&ctx->BufferTable, src_buffer);
    if ((srcbuf == NULL) || (dstbuf == NULL) || (srcbuf == dstbuf))
    {   // one or both buffer objects are invalid, or the same buffer.
        return CG_INVALID_VALUE;
    }
    if ((src_offset + copy_amount) > srcbuf->RequestedSize)
    {   // the offset or amount is invalid.
        return CG_INVALID_VALUE;
    }
    if ((dst_offset + copy_amount) > dstbuf->RequestedSize)
    {   // the destination buffer is not large enough, or the offset is invalid.
        return CG_INVALID_VALUE;
    }

    cg_copy_buffer_cmd_t cmd;
    cmd.WaitEvent      = wait_event;
    cmd.CompleteEvent  = done_event;
    cmd.SourceBuffer   = src_buffer;
    cmd.TargetBuffer   = dst_buffer;
    cmd.SourceOffset   = src_offset;
    cmd.TargetOffset   = dst_offset;
    cmd.CopyAmount     = copy_amount;
    return cgCommandBufferAppend(context, cmd_buffer, CG_COMMAND_COPY_BUFFER, sizeof(cmd), &cmd);
}

/// @summary Copies the entire contents of one image to another.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param cmd_buffer The handle of the destination command buffer.
/// @param dst_image The handle of the target image.
/// @param dst_origin The x-coordinate, y-coordinate and slice or array index on the target image.
/// @param src_image The handle of the source image.
/// @param done_event The handle of an event to signal when the transfer has completed, or CG_INVALID_HANDLE.
/// @param wait_event The handle of an event to wait for before beginning the transfer, or CG_INVALID_HANDLE.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_INVALID_STATE, CG_BUFFER_TOO_SMALL or CG_OUT_OF_MEMORY.
library_function int
cgCopyImage
(
    uintptr_t   context, 
    cg_handle_t cmd_buffer, 
    cg_handle_t dst_image, 
    size_t      dst_origin[3],
    cg_handle_t src_image, 
    cg_handle_t done_event, 
    cg_handle_t wait_event
)
{
    CG_CONTEXT    *ctx    = (CG_CONTEXT*) context;
    CG_IMAGE      *dstimg =  cgObjectTableGet(&ctx->ImageTable, dst_image);
    CG_IMAGE      *srcimg =  cgObjectTableGet(&ctx->ImageTable, src_image);
    if ((srcimg == NULL) || (dstimg == NULL) || (srcimg == dstimg))
    {   // one or both image objects are invalid, or the same image.
        return CG_INVALID_VALUE;
    }
    if ((dst_origin[0] + srcimg->ImageWidth ) > dstimg->PaddedWidth)
    {   // the origin is invalid - it will write off the edge of the destination.
        return CG_INVALID_VALUE;
    }
    if ((dst_origin[1] + srcimg->ImageHeight) > dstimg->PaddedHeight)
    {   // the origin is invalid - it will write off the edge of the destination.
        return CG_INVALID_VALUE;
    }

    cg_copy_image_cmd_t cmd;
    cmd.WaitEvent       = wait_event;
    cmd.CompleteEvent   = done_event;
    cmd.SourceImage     = src_image;
    cmd.TargetImage     = dst_image;
    cmd.SourceOrigin[0] = 0;
    cmd.SourceOrigin[1] = 0;
    cmd.SourceOrigin[2] = 0;
    cmd.TargetOrigin[0] = dst_origin[0];
    cmd.TargetOrigin[1] = dst_origin[1];
    cmd.TargetOrigin[2] = dst_origin[2];
    cmd.Dimensions[0]   = srcimg->ImageWidth;
    cmd.Dimensions[1]   = srcimg->ImageHeight;
    cmd.Dimensions[2]   = max(srcimg->SliceCount, srcimg->ArrayCount);
    return cgCommandBufferAppend(context, cmd_buffer, CG_COMMAND_COPY_IMAGE, sizeof(cmd), &cmd);
}

/// @summary Copies a subregion of one image to another.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param cmd_buffer The handle of the destination command buffer.
/// @param dst_image The handle of the target image.
/// @param dst_origin The x-coordinate, y-coordinate and slice or array index on the target image.
/// @param src_image The handle of the source image.
/// @param done_event The handle of an event to signal when the transfer has completed, or CG_INVALID_HANDLE.
/// @param wait_event The handle of an event to wait for before beginning the transfer, or CG_INVALID_HANDLE.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_INVALID_STATE, CG_BUFFER_TOO_SMALL or CG_OUT_OF_MEMORY.
library_function int
cgCopyImageRegion
(
    uintptr_t   context, 
    cg_handle_t cmd_buffer, 
    cg_handle_t dst_image, 
    size_t      dst_origin[3],
    cg_handle_t src_image, 
    size_t      src_origin[3], 
    size_t      dimension[3],
    cg_handle_t done_event, 
    cg_handle_t wait_event
)
{
    CG_CONTEXT    *ctx    = (CG_CONTEXT*) context;
    CG_IMAGE      *dstimg =  cgObjectTableGet(&ctx->ImageTable, dst_image);
    CG_IMAGE      *srcimg =  cgObjectTableGet(&ctx->ImageTable, src_image);
    if ((srcimg == NULL) || (dstimg == NULL) || (srcimg == dstimg))
    {   // one or both image objects are invalid, or the same image.
        return CG_INVALID_VALUE;
    }
    if ((dst_origin[0] + dimension[0]) > dstimg->PaddedWidth)
    {   // the origin is invalid - it will write off the edge of the destination.
        return CG_INVALID_VALUE;
    }
    if ((dst_origin[1] + dimension[1]) > dstimg->PaddedHeight)
    {   // the origin is invalid - it will write off the edge of the destination.
        return CG_INVALID_VALUE;
    }
    if ((src_origin[0] + dimension[0]) > srcimg->PaddedWidth)
    {   // the origin is invalid - it will read off the edge of the source.
        return CG_INVALID_VALUE;
    }
    if ((src_origin[1] + dimension[1]) > srcimg->PaddedHeight)
    {   // the origin is invalid - it will read off the edge of the source.
        return CG_INVALID_VALUE;
    }

    cg_copy_image_cmd_t cmd;
    cmd.WaitEvent       = wait_event;
    cmd.CompleteEvent   = done_event;
    cmd.SourceImage     = src_image;
    cmd.TargetImage     = dst_image;
    cmd.SourceOrigin[0] = src_origin[0];
    cmd.SourceOrigin[1] = src_origin[1];
    cmd.SourceOrigin[2] = src_origin[2];
    cmd.TargetOrigin[0] = dst_origin[0];
    cmd.TargetOrigin[1] = dst_origin[1];
    cmd.TargetOrigin[2] = dst_origin[2];
    cmd.Dimensions[0]   = dimension[0];
    cmd.Dimensions[1]   = dimension[1];
    cmd.Dimensions[2]   = dimension[2];
    return cgCommandBufferAppend(context, cmd_buffer, CG_COMMAND_COPY_IMAGE, sizeof(cmd), &cmd);
}

/// @summary Copies the contents of a buffer object to an image object.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param cmd_buffer The handle of the destination command buffer.
/// @param dst_image The handle of the target image.
/// @param dst_origin The x-coordinate, y-coordinate and slice or array index on the target image.
/// @param dst_dimension The width (in pixels), height (in pixels) and number of slices to copy.
/// @param src_buffer The handle of the source buffer object.
/// @param src_offset The offset of the first byte to read from the source buffer.
/// @param done_event The handle of the event to signal when the transfer has completed, or CG_INVALID_HANDLE.
/// @param wait_event The handle of the event to wait on before proceeding with the transfer, or CG_INVALID_HANDLE.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_INVALID_STATE, CG_BUFFER_TOO_SMALL or CG_OUT_OF_MEMORY.
library_function int
cgCopyBufferToImage
(
    uintptr_t   context,
    cg_handle_t cmd_buffer,
    cg_handle_t dst_image,
    size_t      dst_origin[3],
    size_t      dst_dimension[3],
    cg_handle_t src_buffer,
    size_t      src_offset,
    cg_handle_t done_event,
    cg_handle_t wait_event
)
{
    CG_CONTEXT    *ctx    = (CG_CONTEXT*) context;
    CG_IMAGE      *dstimg =  cgObjectTableGet(&ctx->ImageTable , dst_image);
    CG_BUFFER     *srcbuf =  cgObjectTableGet(&ctx->BufferTable, src_buffer);
    if ((srcbuf == NULL) || (dstimg == NULL))
    {   // one or both objects are invalid.
        return CG_INVALID_VALUE;
    }
    // TODO(rlk): additional argument validation.

    cg_copy_buffer_to_image_cmd_t cmd;
    cmd.WaitEvent       = wait_event;
    cmd.CompleteEvent   = done_event;
    cmd.SourceBuffer    = src_buffer;
    cmd.TargetImage     = dst_image;
    cmd.SourceOffset    = src_offset;
    cmd.TargetOrigin[0] = dst_origin[0];
    cmd.TargetOrigin[1] = dst_origin[1];
    cmd.TargetOrigin[2] = dst_origin[2];
    cmd.Dimensions[0]   = dst_dimension[0];
    cmd.Dimensions[1]   = dst_dimension[1];
    cmd.Dimensions[2]   = dst_dimension[2];
    return cgCommandBufferAppend(context, cmd_buffer, CG_COMMAND_COPY_BUFFER_TO_IMAGE, sizeof(cmd), &cmd);
}

/// @summary Copies a region from an image object into a buffer object.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param cmd_buffer The handle of the destination command buffer.
/// @param dst_buffer The handle of the target buffer.
/// @param dst_offset The offset of the first byte to write in the target buffer.
/// @param src_image The handle of the source image.
/// @param src_origin The x-coordinate, y-coordinate and slice or array index on the target image.
/// @param src_dimension The width (in pixels), height (in pixels) and number of slices to copy.
/// @param done_event The handle of the event to signal when the transfer has completed, or CG_INVALID_HANDLE.
/// @param wait_event The handle of the event to wait on before proceeding with the transfer, or CG_INVALID_HANDLE.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_INVALID_STATE, CG_BUFFER_TOO_SMALL or CG_OUT_OF_MEMORY.
library_function int
cgCopyImageToBuffer
(
    uintptr_t   context,
    cg_handle_t cmd_buffer,
    cg_handle_t dst_buffer,
    size_t      dst_offset,
    cg_handle_t src_image,
    size_t      src_origin[3],
    size_t      src_dimension[3],
    cg_handle_t done_event,
    cg_handle_t wait_event
)
{
    CG_CONTEXT    *ctx    = (CG_CONTEXT*) context;
    CG_BUFFER     *dstbuf =  cgObjectTableGet(&ctx->BufferTable, dst_buffer);
    CG_IMAGE      *srcimg =  cgObjectTableGet(&ctx->ImageTable , src_image);
    if ((srcimg == NULL) || (dstbuf == NULL))
    {   // one or both objects are invalid.
        return CG_INVALID_VALUE;
    }
    // TODO(rlk): additional argument validation.

    cg_copy_image_to_buffer_cmd_t cmd;
    cmd.WaitEvent       = wait_event;
    cmd.CompleteEvent   = done_event;
    cmd.SourceImage    = src_image;
    cmd.TargetBuffer    = dst_buffer;
    cmd.SourceOrigin[0] = src_origin[0];
    cmd.SourceOrigin[1] = src_origin[1];
    cmd.SourceOrigin[2] = src_origin[2];
    cmd.Dimensions[0]   = src_dimension[0];
    cmd.Dimensions[1]   = src_dimension[1];
    cmd.Dimensions[2]   = src_dimension[2];
    cmd.TargetOffset    = dst_offset;
    return cgCommandBufferAppend(context, cmd_buffer, CG_COMMAND_COPY_IMAGE_TO_BUFFER, sizeof(cmd), &cmd);
}

/// @summary Submits a command buffer for asynchronous execution.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param queue_handle The target command queue.
/// @param cmd_buffer The command buffer to submit.
/// @return One of CG_SUCCESS, CG_INVALID_VALUE, CG_INVALID_STATE, CG_UNSUPPORTED or another result code.
library_function int
cgExecuteCommandBuffer
(
    uintptr_t   context,  
    cg_handle_t queue_handle, 
    cg_handle_t cmd_buffer
)
{
    CG_CONTEXT    *ctx    =(CG_CONTEXT*) context;
    CG_QUEUE      *fifo   = cgObjectTableGet(&ctx->QueueTable, queue_handle);
    CG_CMD_BUFFER *cmdbuf = cgObjectTableGet(&ctx->CmdBufferTable, cmd_buffer);
    size_t         nbytes = 0;
    if (fifo == NULL || cmdbuf == NULL)
    {
        return CG_INVALID_VALUE;
    }
    int  queue_type  = cgCmdBufferGetQueueType(cmdbuf);
    if ((fifo->QueueType & queue_type) == 0)
    {
        return CG_INVALID_VALUE;
    }
    if (cgCmdBufferCanRead(cmdbuf, nbytes) != CG_SUCCESS)
    {
        return CG_INVALID_STATE;
    }

    // submit commands to the associated command queues.
    switch (queue_type)
    {
    case CG_QUEUE_TYPE_COMPUTE:
        return cgExecuteComputeCommandBuffer (ctx, fifo, cmdbuf);
    case CG_QUEUE_TYPE_GRAPHICS:
        return cgExecuteGraphicsCommandBuffer(ctx, fifo, cmdbuf);
    case CG_QUEUE_TYPE_TRANSFER:
        return cgExecuteTransferCommandBuffer(ctx, fifo, cmdbuf);
    default:
        break;
    }
    return CG_UNSUPPORTED;
}

/// @summary Initialize a blend state descriptor such that alpha blending is disabled.
/// @param state The fixed-function blend state descriptor to configure.
/// @return CG_SUCCESS.
library_function int
cgBlendStateInitNone
(
    cg_blend_state_t &state
)
{
    state.BlendEnabled       = false;
    state.SrcBlendColor      = CG_BLEND_FACTOR_ONE;
    state.DstBlendColor      = CG_BLEND_FACTOR_ZERO;
    state.ColorBlendFunction = CG_BLEND_FUNCTION_ADD;
    state.SrcBlendAlpha      = CG_BLEND_FACTOR_ONE;
    state.DstBlendAlpha      = CG_BLEND_FACTOR_ZERO;
    state.AlphaBlendFunction = CG_BLEND_FUNCTION_ADD;
    state.ConstantRGBA[0]    = 0.0f; // R
    state.ConstantRGBA[1]    = 0.0f; // G
    state.ConstantRGBA[2]    = 0.0f; // B
    state.ConstantRGBA[3]    = 0.0f; // A
    return CG_SUCCESS;
}

/// @summary Initialize a blend state descriptor such that standard texture transparency is enabled.
/// @param state The fixed-function blend state descriptor to configure.
/// @return CG_SUCCESS.
library_function int
cgBlendStateInitAlpha
(
    cg_blend_state_t &state
)
{
    state.BlendEnabled       = true;
    state.SrcBlendColor      = CG_BLEND_FACTOR_SRC_COLOR;
    state.DstBlendColor      = CG_BLEND_FACTOR_INV_SRC_ALPHA;
    state.ColorBlendFunction = CG_BLEND_FUNCTION_ADD;
    state.SrcBlendAlpha      = CG_BLEND_FACTOR_SRC_ALPHA;
    state.DstBlendAlpha      = CG_BLEND_FACTOR_INV_SRC_ALPHA;
    state.AlphaBlendFunction = CG_BLEND_FUNCTION_ADD;
    state.ConstantRGBA[0]    = 0.0f; // R
    state.ConstantRGBA[1]    = 0.0f; // G
    state.ConstantRGBA[2]    = 0.0f; // B
    state.ConstantRGBA[3]    = 0.0f; // A
    return CG_SUCCESS;
}

/// @summary Initialize a blend state descriptor such that additive alpha blending is enabled.
/// @param state The fixed-function blend state descriptor to configure.
/// @return CG_SUCCESS.
library_function int
cgBlendStateInitAdditive
(
    cg_blend_state_t &state
)
{
    state.BlendEnabled       = true;
    state.SrcBlendColor      = CG_BLEND_FACTOR_SRC_COLOR;
    state.DstBlendColor      = CG_BLEND_FACTOR_ONE;
    state.ColorBlendFunction = CG_BLEND_FUNCTION_ADD;
    state.SrcBlendAlpha      = CG_BLEND_FACTOR_SRC_ALPHA;
    state.DstBlendAlpha      = CG_BLEND_FACTOR_ONE;
    state.AlphaBlendFunction = CG_BLEND_FUNCTION_ADD;
    state.ConstantRGBA[0]    = 0.0f; // R
    state.ConstantRGBA[1]    = 0.0f; // G
    state.ConstantRGBA[2]    = 0.0f; // B
    state.ConstantRGBA[3]    = 0.0f; // A
    return CG_SUCCESS;
}

/// @sumary Initialize a blend state descriptor such that premultiplied alpha blending is enabled.
/// @param state The fixed-function blend state descriptor to configure.
/// @return CG_SUCCESS.
library_function int
cgBlendStateInitPremultiplied
(
    cg_blend_state_t &state
)
{
    state.BlendEnabled       = true;
    state.SrcBlendColor      = CG_BLEND_FACTOR_ONE;
    state.DstBlendColor      = CG_BLEND_FACTOR_INV_SRC_ALPHA;
    state.ColorBlendFunction = CG_BLEND_FUNCTION_ADD;
    state.SrcBlendAlpha      = CG_BLEND_FACTOR_ONE;
    state.DstBlendAlpha      = CG_BLEND_FACTOR_INV_SRC_ALPHA;
    state.AlphaBlendFunction = CG_BLEND_FUNCTION_ADD;
    state.ConstantRGBA[0]    = 0.0f; // R
    state.ConstantRGBA[1]    = 0.0f; // G
    state.ConstantRGBA[2]    = 0.0f; // B
    state.ConstantRGBA[3]    = 0.0f; // A
    return CG_SUCCESS;
}

/// @summary Initialize a rasterizer state descriptor to the default values.
/// @param state The fixed-function rasterizer state descriptor to configure.
/// @return CG_SUCCESS.
library_function int
cgRasterStateInitDefault
(
    cg_raster_state_t &state
)
{
    state.FillMode  = CG_FILL_SOLID;
    state.CullMode  = CG_CULL_BACK;
    state.FrontFace = CG_WINDING_CCW;
    state.DepthBias = 0;
    state.SlopeScaledDepthBias = 0.0f;
    return CG_SUCCESS;
}

/// @summary Initialize a depth-stencil state descriptor to the default values.
/// @param state The fixed-function depth and stencil test state descriptor to configure.
/// @return CG_SUCCESS.
library_function int
cgDepthStencilStateInitDefault
(
    cg_depth_stencil_state_t &state
)
{
    state.DepthTestEnable     = false;
    state.DepthWriteEnable    = false;
    state.DepthBoundsEnable   = false;
    state.DepthTestFunction   = CG_COMPARE_LESS;
    state.DepthMin            = 0.0f;
    state.DepthMax            = 1.0f;
    state.StencilTestEnable   = false;
    state.StencilTestFunction = CG_COMPARE_ALWAYS;
    state.StencilFailOp       = CG_STENCIL_OP_KEEP;
    state.StencilPassZPassOp  = CG_STENCIL_OP_KEEP;
    state.StencilPassZFailOp  = CG_STENCIL_OP_KEEP;
    state.StencilReadMask     = 0xFF;
    state.StencilWriteMask    = 0xFF;
    state.StencilReference    = 0x00;
    return CG_SUCCESS;
}

/// @summary Determines if a DXGI format value is a block-compressed format.
/// @param format One of dxgi_format_e or DXGI_FORMAT.
/// @return true if format is one of DXGI_FORMAT_BCn.
library_function bool
cgPixelFormatIsBlockCompressed
(
    uint32_t format
)
{
    switch (format)
    {
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return true;

        default:
            break;
    }
    return false;
}

/// @summary Determines if a DXGI format value specifies a packed format.
/// @param format One of dxgi_format_e or DXGI_FORMAT.
/// @return true if format is one of DXGI_FORMAT_R8G8_B8G8_UNORM or DXGI_FORMAT_G8R8_G8B8_UNORM.
library_function bool
cgPixelFormatIsPacked
(
    uint32_t format
)
{
    if (format == DXGI_FORMAT_R8G8_B8G8_UNORM ||
        format == DXGI_FORMAT_G8R8_G8B8_UNORM)
    {
        return true;
    }
    else return false;
}

/// @summary Calculate the number of bits-per-pixel for a given format. Block-
/// compressed formats are supported as well.
/// @param format One of dxgi_format_e.
/// @return The number of bits per-pixel.
library_function size_t
cgPixelFormatBitsPerPixel
(
    uint32_t format
)
{
    switch (format)
    {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return 128;

        case DXGI_FORMAT_R32G32B32_TYPELESS:
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
            return 96;

        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R32G32_TYPELESS:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
            return 64;

        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R11G11B10_FLOAT:
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_R16G16_TYPELESS:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        case DXGI_FORMAT_R8G8_B8G8_UNORM:
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
            return 32;

        case DXGI_FORMAT_R8G8_TYPELESS:
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_SINT:
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_B5G6R5_UNORM:
        case DXGI_FORMAT_B5G5R5A1_UNORM:
        case DXGI_FORMAT_B4G4R4A4_UNORM:
            return 16;

        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_A8_UNORM:
            return 8;

        case DXGI_FORMAT_R1_UNORM:
            return 1;

        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
            return 4;

        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return 8;

        default:
            return 0;
    }
}

/// @summary Calculate the number of bytes per 4x4-pixel block.
/// @param format One of dxgi_format_e.
/// @return The number of bytes in a 4x4 pixel block, or 0 for non-block-compressed formats.
library_function size_t
cgPixelFormatBytesPerBlock
(
    uint32_t format
)
{
    switch (format)
    {
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
            return 8;

        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return 16;

        default:
            break;
    }
    return 0;
}

/// @summary Calculates the dimension of an image, in pixels, and accounting for block compression. Note that only width and height should be calculated using this logic as block compression is 2D-only.
/// @param format One of dxgi_format_e or DXGI_FORMAT.
/// @param pixel_dimension The unpadded width or height of an image.
/// @return The width or height, in pixels.
library_function size_t
cgImageDimension
(
    uint32_t format,
    size_t   pixel_dimension
)
{
    if (cgPixelFormatIsBlockCompressed(format))
    {   // all BC formats encode 4x4 blocks.
        size_t dim = ((pixel_dimension + 3) / 4) * 4;
        return dim > 0 ? dim : 1;
    }
    else return pixel_dimension > 0 ? pixel_dimension : 1;
}

/// @summary Calculates the dimension of a mipmap level. This function may be used to calculate the width, height or depth dimensions.
/// @param format One of dxgi_format_e or DXGI_FORMAT.
/// @param base_dimension The unpadded dimension of the highest-resolution level (level 0.)
/// @param level_index The zero-based index of the mip-level to compute.
/// @return The corresponding unpadded dimension of the specified mipmap level.
library_function size_t
cgImageLevelDimension
(
    uint32_t format,
    size_t   base_dimension,
    size_t   level_index
)
{   UNREFERENCED_PARAMETER(format);
    size_t level_dimension = base_dimension >> level_index;
    return(level_dimension > 0 ? level_dimension : 1);
}

/// @summary Calculates the correct pitch value for a scanline, based on the data format and width of the surface. This is necessary because many DDS writers do not correctly compute the pitch value. See MSDN documentation at:
/// http://msdn.microsoft.com/en-us/library/windows/desktop/bb943991(v=vs.85).aspx
/// @param format One of the values of the dxgi_format_e enumeration.
/// @param pixel_width The unpadded width of a single scanline, in pixels.
/// @return The number of bytes per-scanline.
library_function size_t
cgImageRowPitch
(
    uint32_t format,
    size_t   pixel_width
)
{
    if (cgPixelFormatIsBlockCompressed(format))
    {
        size_t bw =(pixel_width + 3) / 4; bw = bw > 0 ? bw : 1;
        return bw * cgPixelFormatBytesPerBlock(format);
    }
    if (cgPixelFormatIsPacked(format))
    {
        return ((pixel_width + 1) >> 1) * 4;
    }
    return (pixel_width * cgPixelFormatBitsPerPixel(format) + 7) / 8;
}

/// @summary Calculates the number of bytes required to store a single slice with the specified dimensions.
/// @param format One of the values of the dxgi_format_e or DXGI_FORMAT enumeration.
/// @param pixel_width The unpadded width of a single scanline, in pixels.
/// @param pixel_height The unpadded height of the image, in pixels.
/// @return The number of bytes required to store a single slice.
library_function size_t
cgImageSlicePitch
(
    uint32_t format,
    size_t   pixel_width,
    size_t   pixel_height
)
{
    return cgImageRowPitch (format, pixel_width) * 
           cgImageDimension(format, pixel_height);
}

/// @summary Determines if a DDS describes a cubemap surface.
/// @param header The base surface header of the DDS.
/// @param header_ex The extended surface header of the DDS, or NULL.
/// @return true if the DDS describes a cubemap.
library_function bool
cgIsCubemapImageDDS
(
    dds_header_t       const *header,
    dds_header_dxt10_t const *header_ex
)
{
    if (header_ex != NULL)
    {
        if (header_ex->Dimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D &&
            header_ex->Flags     == D3D11_RESOURCE_MISC_TEXTURECUBE)
        {
            return true;
        }
        // else fall through and look at the dds_header_t.
    }
    if (header != NULL)
    {
        if ((header->Caps  & DDSCAPS_COMPLEX) == 0)
            return false;
        if ((header->Caps2 & DDSCAPS2_CUBEMAP) == 0)
            return false;
        if ((header->Caps2 & DDSCAPS2_CUBEMAP_POSITIVEX) ||
            (header->Caps2 & DDSCAPS2_CUBEMAP_NEGATIVEX) ||
            (header->Caps2 & DDSCAPS2_CUBEMAP_POSITIVEY) ||
            (header->Caps2 & DDSCAPS2_CUBEMAP_NEGATIVEY) ||
            (header->Caps2 & DDSCAPS2_CUBEMAP_POSITIVEZ) ||
            (header->Caps2 & DDSCAPS2_CUBEMAP_NEGATIVEZ))
            return true;
    }
    return false;
}

/// @summary Determines if a DDS describes a volume surface.
/// @param header The base surface header of the DDS.
/// @param header_ex The extended surface header of the DDS, or NULL.
/// @return true if the DDS describes a volume.
library_function bool
cgIsVolumeImageDDS
(
    dds_header_t       const *header,
    dds_header_dxt10_t const *header_ex
)
{
    if (header_ex != NULL)
    {
        if (header_ex->ArraySize != 1)
        {   // arrays of volumes are not supported.
            return false;
        }
    }
    if (header != NULL)
    {
        if ((header->Caps  & DDSCAPS_COMPLEX) == 0)
            return false;
        if ((header->Caps2 & DDSCAPS2_VOLUME) == 0)
            return false;
        if ((header->Flags & DDSD_DEPTH) == 0)
            return false;
        return header->Depth > 1;
    }
    return false;
}

/// @summary Determines if a DDS describes a surface array. Note that a volume is not considered to be the same as a surface array.
/// @param header The base surface header of the DDS.
/// @param header_ex The extended surface header of the DDS, or NULL.
/// @return true if the DDS describes a surface array.
library_function bool
cgIsArrayImageDDS
(
    dds_header_t       const *header,
    dds_header_dxt10_t const *header_ex
)
{
    if (header != NULL && header_ex != NULL)
    {
        return header_ex->ArraySize > 1;
    }
    return false;
}

/// @summary Determines if a DDS describes a mipmap chain.
/// @param header The base surface header of the DDS.
/// @param header_ex The extended surface header of the DDS, or NULL.
/// @return true if the DDS describes a mipmap chain.
library_function bool
cgHasMipmapsDDS
(
    dds_header_t       const *header,
    dds_header_dxt10_t const *header_ex
)
{
    if (header_ex != NULL)
    {
        if (header_ex->Dimension != D3D11_RESOURCE_DIMENSION_TEXTURE1D &&
            header_ex->Dimension != D3D11_RESOURCE_DIMENSION_TEXTURE2D &&
            header_ex->Dimension != D3D11_RESOURCE_DIMENSION_TEXTURE3D)
            return false;
    }
    if (header != NULL)
    {
        if (header->Caps & DDSCAPS_MIPMAP)
            return true;
        if (header->Flags & DDSD_MIPMAPCOUNT)
            return true;
        if (header->Levels > 0)
            return true;
    }
    return false;
}

/// @summary Determines the number of elements in a surface array.
/// @param header The base surface header of the DDS.
/// @param header_ex The extended surface header of the DDS, or NULL.
/// @return The number of elements in the surface array, or 1 if the DDS does not describe an array.
library_function size_t
cgImageArrayCountDDS
(
    dds_header_t       const *header,
    dds_header_dxt10_t const *header_ex
)
{
    if (header != NULL && header_ex != NULL)
    {
        size_t multiplier = 1;
        if (header->Caps2 & DDSCAPS2_CUBEMAP)
        {   // DX10+ cubemaps must specify all faces.
            multiplier = 6;
        }
        // DX10 extended header is required for surface arrays.
        return size_t(header_ex->ArraySize) * multiplier;
    }
    else if (header != NULL)
    {
        size_t nfaces = 1;
        if (header->Caps2 & DDSCAPS2_CUBEMAP)
        {   // non-DX10 cubemaps may specify only some faces.
            if (header->Caps2 & DDSCAPS2_CUBEMAP_POSITIVEX) nfaces++;
            if (header->Caps2 & DDSCAPS2_CUBEMAP_NEGATIVEX) nfaces++;
            if (header->Caps2 & DDSCAPS2_CUBEMAP_POSITIVEY) nfaces++;
            if (header->Caps2 & DDSCAPS2_CUBEMAP_NEGATIVEY) nfaces++;
            if (header->Caps2 & DDSCAPS2_CUBEMAP_POSITIVEZ) nfaces++;
            if (header->Caps2 & DDSCAPS2_CUBEMAP_NEGATIVEZ) nfaces++;
        }
        return nfaces;
    }
    else return 0;
}

/// @summary Determines the number of levels in the mipmap chain.
/// @param header The base surface header of the DDS.
/// @param header_ex The extended surface header of the DDS, or NULL.
/// @return The number of levels in the mipmap chain, or 1 if the DDS describes the top level only.
library_function size_t
cgImageLevelCountDDS
(
    dds_header_t       const *header,
    dds_header_dxt10_t const *header_ex
)
{
    if (cgHasMipmapsDDS(header, header_ex))
    {
        return header->Levels;
    }
    else if (header != NULL) return 1;
    else return 0;
}

/// @summary Determines the dxgi_format value based on data in dds headers.
/// @param header the base surface header of the dds.
/// @param header_ex the extended surface header of the dds, or NULL.
/// @return One of the values of the dxgi_format_e enumeration.
library_function uint32_t
cgPixelFormatForDDS
(
    dds_header_t       const *header,
    dds_header_dxt10_t const *header_ex
)
{
    if (header_ex != NULL)
    {
        return header_ex->Format;
    }
    if (header == NULL)
    {
        return DXGI_FORMAT_UNKNOWN;
    }

    dds_pixelformat_t const &pf =  header->Format;
    #define ISBITMASK(r, g, b, a) (pf.BitMaskR == (r) && pf.BitMaskG == (g) && pf.BitMaskB == (b) && pf.BitMaskA == (a))

    if (pf.Flags & DDPF_FOURCC)
    {
        if (pf.FourCC == cgFourCC_le('D','X','T','1'))
        {
            return DXGI_FORMAT_BC1_UNORM;
        }
        if (pf.FourCC == cgFourCC_le('D','X','T','2'))
        {
            return DXGI_FORMAT_BC2_UNORM;
        }
        if (pf.FourCC == cgFourCC_le('D','X','T','3'))
        {
            return DXGI_FORMAT_BC2_UNORM;
        }
        if (pf.FourCC == cgFourCC_le('D','X','T','4'))
        {
            return DXGI_FORMAT_BC3_UNORM;
        }
        if (pf.FourCC == cgFourCC_le('D','X','T','5'))
        {
            return DXGI_FORMAT_BC3_UNORM;
        }
        if (pf.FourCC == cgFourCC_le('A','T','I','1'))
        {
            return DXGI_FORMAT_BC4_UNORM;
        }
        if (pf.FourCC == cgFourCC_le('A','T','I','2'))
        {
            return DXGI_FORMAT_BC5_UNORM;
        }
        if (pf.FourCC == cgFourCC_le('B','C','4','U'))
        {
            return DXGI_FORMAT_BC4_UNORM;
        }
        if (pf.FourCC == cgFourCC_le('B','C','4','S'))
        {
            return DXGI_FORMAT_BC4_SNORM;
        }
        if (pf.FourCC == cgFourCC_le('B','C','5','U'))
        {
            return DXGI_FORMAT_BC5_UNORM;
        }
        if (pf.FourCC == cgFourCC_le('B','C','5','S'))
        {
            return DXGI_FORMAT_BC5_SNORM;
        }
        switch (pf.FourCC)
        {
            case 36: // D3DFMT_A16B16G16R16
                return DXGI_FORMAT_R16G16B16A16_UNORM;

            case 110: // D3DFMT_Q16W16V16U16
                return DXGI_FORMAT_R16G16B16A16_SNORM;

            case 111: // D3DFMT_R16F
                return DXGI_FORMAT_R16_FLOAT;

            case 112: // D3DFMT_G16R16F
                return DXGI_FORMAT_R16G16_FLOAT;

            case 113: // D3DFMT_A16B16G16R16F
                return DXGI_FORMAT_R16G16B16A16_FLOAT;

            case 114: // D3DFMT_R32F
                return DXGI_FORMAT_R32_FLOAT;

            case 115: // D3DFMT_G32R32F
                return DXGI_FORMAT_R32G32_FLOAT;

            case 116: // D3DFMT_A32B32G32R32F
                return DXGI_FORMAT_R32G32B32A32_FLOAT;

            default:
                break;
        }
        return DXGI_FORMAT_UNKNOWN;
    }
    if (pf.Flags & DDPF_RGB)
    {
        switch (pf.RGBBitCount)
        {
            case 32:
                {
                    if (ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
                    {
                        return DXGI_FORMAT_R8G8B8A8_UNORM;
                    }
                    if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000))
                    {
                        return DXGI_FORMAT_B8G8R8A8_UNORM;
                    }
                    if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000))
                    {
                        return DXGI_FORMAT_B8G8R8X8_UNORM;
                    }

                    // No DXGI format maps to ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000) aka D3DFMT_X8B8G8R8
                    // Note that many common DDS reader/writers (including D3DX) swap the the RED/BLUE masks for 10:10:10:2
                    // formats. We assumme below that the 'backwards' header mask is being used since it is most likely
                    // written by D3DX. The more robust solution is to use the 'DX10' header extension and specify the
                    // DXGI_FORMAT_R10G10B10A2_UNORM format directly.

                    // For 'correct' writers, this should be 0x000003ff, 0x000ffc00, 0x3ff00000 for RGB data.
                    if (ISBITMASK(0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000))
                    {
                        return DXGI_FORMAT_R10G10B10A2_UNORM;
                    }
                    // No DXGI format maps to ISBITMASK(0x000003ff, 0x000ffc00, 0x3ff00000, 0xc0000000) aka D3DFMT_A2R10G10B10.
                    if (ISBITMASK(0x0000ffff, 0xffff0000, 0x00000000, 0x00000000))
                    {
                        return DXGI_FORMAT_R16G16_UNORM;
                    }
                    if (ISBITMASK(0xffffffff, 0x00000000, 0x00000000, 0x00000000))
                    {
                        // Only 32-bit color channel format in D3D9 was R32F
                        return DXGI_FORMAT_R32_FLOAT; // D3DX writes this out as a FourCC of 114.
                    }
                }
                break;

            case 24:
                // No 24bpp DXGI formats aka D3DFMT_R8G8B8
                break;

            case 16:
                {
                    if (ISBITMASK(0x7c00, 0x03e0, 0x001f, 0x8000))
                    {
                        return DXGI_FORMAT_B5G5R5A1_UNORM;
                    }
                    if (ISBITMASK(0xf800, 0x07e0, 0x001f, 0x0000))
                    {
                        return DXGI_FORMAT_B5G6R5_UNORM;
                    }
                    // No DXGI format maps to ISBITMASK(0x7c00, 0x03e0, 0x001f, 0x0000) aka D3DFMT_X1R5G5B5.
                    if (ISBITMASK(0x0f00, 0x00f0, 0x000f, 0xf000))
                    {
                        return DXGI_FORMAT_B4G4R4A4_UNORM;
                    }
                    // No DXGI format maps to ISBITMASK(0x0f00, 0x00f0, 0x000f, 0x0000) aka D3DFMT_X4R4G4B4.
                    // No 3:3:2, 3:3:2:8, or paletted DXGI formats aka D3DFMT_A8R3G3B2, D3DFMT_R3G3B2, D3DFMT_P8, D3DFMT_A8P8, etc.
                }
                break;
        }
    }
    if (pf.Flags & DDPF_ALPHA)
    {
        if (pf.RGBBitCount == 8)
        {
            return DXGI_FORMAT_A8_UNORM;
        }
    }
    if (pf.Flags & DDPF_LUMINANCE)
    {
        if (pf.RGBBitCount == 8)
        {
            if (ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x00000000))
            {
                // D3DX10/11 writes this out as DX10 extension.
                return DXGI_FORMAT_R8_UNORM;
            }

            // No DXGI format maps to ISBITMASK(0x0f, 0x00, 0x00, 0xf0) aka D3DFMT_A4L4
        }
        if (pf.RGBBitCount == 16)
        {
            if (ISBITMASK(0x0000ffff, 0x00000000, 0x00000000, 0x00000000))
            {
                // D3DX10/11 writes this out as DX10 extension.
                return DXGI_FORMAT_R16_UNORM;
            }
            if (ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x0000ff00))
            {
                // D3DX10/11 writes this out as DX10 extension
                return DXGI_FORMAT_R8G8_UNORM;
            }
        }
    }
    #undef ISBITMASK
    return DXGI_FORMAT_UNKNOWN;
}

/// @summary Given a baseline DDS header, generates a corresponding DX10 extended header.
/// @param dx10 The DX10 extended DDS header to populate.
/// @param header base DDS header describing the image.
library_function void
cgMakeDx10HeaderDDS
(
    dds_header_dxt10_t       *dx10,
    dds_header_t       const *header
)
{
    dx10->Format    = cgPixelFormatForDDS(header, NULL);
    dx10->Dimension = D3D11_RESOURCE_DIMENSION_TEXTURE1D;
    dx10->Flags     = 0; // determined below
    dx10->ArraySize =(uint32_t) cgImageArrayCountDDS(header, NULL);
    dx10->Flags2    = DDS_ALPHA_MODE_UNKNOWN;

    // determine a correct resource dimension:
    if (cgIsCubemapImageDDS(header, NULL) && dx10->ArraySize == 6)
    {   // DX10+ cubemaps must specify all six faces.
        dx10->Dimension = D3D11_RESOURCE_DIMENSION_TEXTURE2D;
        dx10->Flags    |= D3D11_RESOURCE_MISC_TEXTURECUBE;
    }
    else if (cgIsVolumeImageDDS(header, NULL))
    {
        dx10->Dimension = D3D11_RESOURCE_DIMENSION_TEXTURE3D;
    }
    else if ((header->Flags & DDSD_WIDTH ) != 0 && header->Width  > 1 && 
             (header->Flags & DDSD_HEIGHT) != 0 && header->Height > 1)
    {
        dx10->Dimension = D3D11_RESOURCE_DIMENSION_TEXTURE2D;
    }
}

// A headless configuration is going to find CPUs first, including any GPUs in the share group.
// It would then create additional execution groups containing any discrete GPU devices.
// Headless is CPU-driven, with *optional* GPUs for accelerators.
//
// A display configuration is going to create an execution group based on a display first, including any CPUs in the share group.
// It would then create additional execution groups containing any CPUs or discrete GPU devices.
// Display is GPU-driven; a presentation queue/OpenGL support is *required*.
//
// There are only two types of resources that allocate memory - images and buffers.
