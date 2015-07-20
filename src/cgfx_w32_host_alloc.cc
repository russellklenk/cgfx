/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implements memory allocation functions routed through the user-
/// defined memory allocator, with fallback to default allocation via malloc 
/// and free with added alignment support.
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/

/*////////////////////
//   Preprocessor   //
////////////////////*/

/*/////////////////
//   Constants   //
/////////////////*/

/*///////////////////
//   Local Types   //
///////////////////*/
/// @summary Forward-declare the default standard C library-based allocation functions.
internal_function void* CG_API cgHostMemAllocStdC(size_t, size_t, int, uintptr_t);
internal_function void* CG_API cgHostMemAllocNoOp(size_t, size_t, int, uintptr_t);
internal_function void  CG_API cgHostMemFreeStdC (void* , size_t, size_t, int, uintptr_t);
internal_function void  CG_API cgHostMemFreeNoOp (void* , size_t, size_t, int, uintptr_t);

/// @summary Defines the state data associated with a host memory allocator implementation.
struct CG_HOST_ALLOCATOR
{
    cgMemoryAlloc_fn    Allocate;    /// The host memory allocation callback.
    cgMemoryFree_fn     Release;     /// The host memory free callback.
    uintptr_t           UserData;    /// Opaque user data to pass through to Allocate and Release.
    bool                Initialized; /// true if the allocator has been initialized.
};
#define CG_HOST_ALLOCATOR_STATIC_INIT    {NULL, NULL, 0, false}

/*///////////////
//   Globals   //
///////////////*/
/// @summary The global memory allocator state.
global_variable CG_HOST_ALLOCATOR cgHostAllocator = 
{
    cgHostMemAllocNoOp, 
    cgHostMemFreeNoOp, 
    (uintptr_t) 0, 
    false
};

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

/*////////////////////////
//   Public Functions   //
////////////////////////*/
/// @summary Validate and initialize a user-defined memory allocator.
/// @param alloc_cb The user-supplied host memory allocation callbacks. Specify NULL to fall back to malloc and free.
/// @param alloc_fn The memory allocator state to initialize. Specify NULL to initialize the default allocator.
/// @return CG_SUCCESS, or CG_INVALID_VALUE if one or more inputs are invalid.
public_function int
cgSetupUserHostAllocator
(
    cg_allocation_callbacks_t *alloc_cb, 
    CG_HOST_ALLOCATOR         *alloc_fn=NULL
)
{
    if (alloc_fn == NULL)
    {   // modify the global memory allocator.
        alloc_fn  = &cgHostAllocator;
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
        cgHostAllocator.Allocate    = cgHostMemAllocStdC;
        cgHostAllocator.Release     = cgHostMemFreeStdC;
        cgHostAllocator.UserData    =(uintptr_t) 0;
        cgHostAllocator.Initialized = true;
        if (alloc_fn != NULL)
        {
            alloc_fn->Allocate      = cgHostMemAllocStdC;
            alloc_fn->Release       = cgHostMemFreeStdC;
            alloc_fn->UserData      =(uintptr_t) 0;
            alloc_fn->Initialized   = true;
        }
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

/// @summary Set the global host allocator to use the same allocator implementation as the given allocator state.
/// @param host The host allocator state to copy to the global allocator.
public_function void
cgSetupGlobalHostAllocator
(
    CG_HOST_ALLOCATOR *host
)
{
    if (cgHostAllocator.Initialized == false)
    {   // copy the host fields up to the global allocator.
        cgHostAllocator.Allocate     = host->Allocate;
        cgHostAllocator.Release      = host->Release;
        cgHostAllocator.UserData     = host->UserData;
        cgHostAllocator.Initialized  = true;
    }
}

/// @summary Delete a host allocator by setting it up to reference the no-op allocator implementation. All calls to allocate memory will fail.
/// @param host The host allocator state to delete.
public_function void
cgDeleteUserHostAllocator
(
    CG_HOST_ALLOCATOR *host
)
{
    host->Allocate = cgHostMemAllocNoOp;
    host->Release  = cgHostMemFreeNoOp;
    host->UserData =(uintptr_t) 0;
}

/// @summary Allocate host memory from the global heap.
/// @param size The desired allocation size, in bytes.
/// @param alignment The required alignment of the returned address, in bytes.
/// @param type One of uiss_allocation_type_e.
/// @return A pointer to the host memory block, or NULL.
public_function inline void*
cgAllocateHostMemoryGlobal
(
    size_t     size, 
    size_t     alignment, 
    int        type
)
{
    return cgHostAllocator.Allocate(size, alignment, type, cgHostAllocator.UserData);
}

/// @summary Allocate host memory from the context-specific heap.
/// @param host The host allocator, initialized with cgUserSetupHostAllocator.
/// @param size The desired allocation size, in bytes.
/// @param alignment The required alignment of the returned address, in bytes.
/// @param type One of uiss_allocation_type_e.
/// @return A pointer to the host memory block, or NULL.
public_function inline void*
cgAllocateHostMemory
(
    CG_HOST_ALLOCATOR *host,
    size_t             size, 
    size_t             alignment, 
    int                type 
)
{
    return host->Allocate(size, alignment, type, host->UserData);
}

/// @summary Free host memory allocated from the global heap.
/// @param address The address of the memory block to free, as returned by the allocation function.
/// @param size The requested size of the allocated block, in bytes.
/// @param alignment The required alignment of the allocated block, in bytes.
/// @param type One of uiss_allocation_type_e.
public_function inline void
cgFreeHostMemoryGlobal
(
    void  *address,
    size_t size, 
    size_t alignment, 
    int    type
)
{
    cgHostAllocator.Release(address, size, alignment, type, cgHostAllocator.UserData);
}

/// @summary Free host memory allocated from a context-specific heap.
/// @param host The host allocator, initialized with cgUserSetupHostAllocator.
/// @param address The address of the memory block to free, as returned by the allocation function.
/// @param size The requested size of the allocated block, in bytes.
/// @param alignment The required alignment of the allocated block, in bytes.
/// @param type One of uiss_allocation_type_e.
public_function inline void
cgFreeHostMemory
(
    CG_HOST_ALLOCATOR *host,
    void              *address,
    size_t             size, 
    size_t             alignment, 
    int                type
)
{
    host->Release(address, size, alignment, type, host->UserData);
}
