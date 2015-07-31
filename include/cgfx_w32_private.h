/*/////////////////////////////////////////////////////////////////////////////
/// @summary Define the private constants, data types and functions exposed by 
/// the unified Compute and Graphics library for Microsoft Windows platforms.
///////////////////////////////////////////////////////////////////////////80*/

#ifndef LIB_CGFX_W32_PRIVATE_H
#define LIB_CGFX_W32_PRIVATE_H

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

#include "CL/cl.h"
#include "CL/cl_gl.h"
#include "CL/cl_gl_ext.h"

#include "intrinsics.h"
#include "atomicfifo.h"

/*////////////////////////////
//   Forward Declarations   //
////////////////////////////*/
struct CG_QUEUE;
struct CG_DEVICE;
struct CG_KERNEL;
struct CG_DISPLAY;
struct CG_CONTEXT;
struct CG_CMD_BUFFER;
struct CG_EXEC_GROUP;

/*/////////////////
//   Constants   //
/////////////////*/
// Handle Layout:
// I = object ID (32 bits)
// Y = object type (24 bits)
// T = table index (8 bits)
// 63                                                             0
// ................................................................
// TTTTTTTTYYYYYYYYYYYYYYYYYYYYYYYYIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
// 
// CG_HANDLE_MASK_x_P => Mask in a packed value
// CG_HANDLE_MASK_x_U => Mask in an unpacked value
#define CG_HANDLE_MASK_I_P                       (0x00000000FFFFFFFFULL)
#define CG_HANDLE_MASK_I_U                       (0x00000000FFFFFFFFULL)
#define CG_HANDLE_MASK_Y_P                       (0x00FFFFFF00000000ULL)
#define CG_HANDLE_MASK_Y_U                       (0x0000000000FFFFFFULL)
#define CG_HANDLE_MASK_T_P                       (0xFF00000000000000ULL)
#define CG_HANDLE_MASK_T_U                       (0x00000000000000FFULL)
#define CG_HANDLE_SHIFT_I                        (0)
#define CG_HANDLE_SHIFT_Y                        (32)
#define CG_HANDLE_SHIFT_T                        (56)

/// @summary Define object table indicies within a context. Used when building handles.
#define CG_DEVICE_TABLE_ID                       (0)
#define CG_DISPLAY_TABLE_ID                      (1)
#define CG_QUEUE_TABLE_ID                        (2)
#define CG_CMD_BUFFER_TABLE_ID                   (3)
#define CG_EXEC_GROUP_TABLE_ID                   (4)
#define CG_KERNEL_TABLE_ID                       (5)
#define CG_PIPELINE_TABLE_ID                     (6)
#define CG_BUFFER_TABLE_ID                       (7)

/// @summary Define object table sizes within a context. Different maximum numbers of objects help control memory usage.
/// Each size value must be a power-of-two, and the maximum number of objects of that type is one less than the stated value.
#define CG_MAX_DEVICES                           (4096)
#define CG_MAX_DISPLAYS                          (32)
#define CG_MAX_QUEUES                            (CG_MAX_DEVICES * 4)
#define CG_MAX_CMD_BUFFERS                       (8192)
#define CG_MAX_EXEC_GROUPS                       (CG_MAX_DEVICES)
#define CG_MAX_KERNELS                           (8192)
#define CG_MAX_PIPELINES                         (4096)
#define CG_MAX_BUFFERS                           (65536)

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

/*////////////////////////////
//  Function Pointer Types  //
////////////////////////////*/

/*//////////////////
//   Data Types   //
//////////////////*/
/// @summary Defines the state data associated with a host memory allocator implementation.
struct CG_HOST_ALLOCATOR
{
    cgMemoryAlloc_fn             Allocate;             /// The host memory allocation callback.
    cgMemoryFree_fn              Release;              /// The host memory free callback.
    uintptr_t                    UserData;             /// Opaque user data to pass through to Allocate and Release.
    bool                         Initialized;          /// true if the allocator has been initialized.
};
#define CG_HOST_ALLOCATOR_STATIC_INIT    {NULL, NULL, 0, false}

/// @summary Describes the location of an object within an object table.
struct CG_OBJECT_INDEX
{
    uint32_t                     Id;                   /// The ID is the index-in-table + generation.
    uint16_t                     Next;                 /// The zero-based index of the next free slot.
    uint16_t                     Index;                /// The zero-based index into the tightly-packed array.
};

/// @summary Defines a table mapping handles to internal data. Each table may be several MB. Do not allocate on the stack.
/// The type T must have a field uint32_t ObjectId.
/// The size N must be a power of two greater than zero. The maximum table capacity is 65536. 
/// The table can hold one item less than the value of N.
template <typename T, size_t N>
struct CG_OBJECT_TABLE
{
    static size_t   const        MAX_OBJECTS             = N;
    static uint32_t const        INDEX_INVALID           = N;
    static uint32_t const        INDEX_MASK              =(N - 1);
    static uint32_t const        NEW_OBJECT_ID_ADD       = N;

    size_t                       ObjectCount;          /// The number of live objects in the table.
    uint16_t                     FreeListTail;         /// The index of the most recently freed item.
    uint16_t                     FreeListHead;         /// The index of the next available item.
    uint32_t                     ObjectType;           /// One of cg_object_e specifying the type of object in this table.
    size_t                       TableIndex;           /// The zero-based index of this table, if there are multiple tables of this type.
    CG_OBJECT_INDEX              Indices[N];           /// The sparse array used to look up the data in the packed array.
    T                            Objects[N];           /// The tightly packed array of object data.
};

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
    uint32_t                     ObjectId;             /// The internal CGFX object identifier.

    cg_handle_t                  ExecutionGroup;       /// The handle of the execution group that owns the device. A device can only belong to one group.

    cl_platform_id               PlatformId;           /// The OpenCL platform identifier. Same-platform devices can share resources.
    cl_device_id                 DeviceId;             /// The OpenCL device identifier, which may or may not be the same as the MasterDeviceId.
    cl_device_id                 MasterDeviceId;       /// The OpenCL device identifier of the parent device.
    cl_device_type               Type;                 /// The OpenCL device type (CPU, GPU, ACCELERATOR or CUSTOM.)

    char                        *Name;                 /// The friendly name of the device.
    char                        *Platform;             /// The friendly name of the platform.
    char                        *Version;              /// The OpenCL version supported by the device.
    char                        *Driver;               /// The version of the OpenCL driver for the device.
    char                        *Extensions;           /// A space-delimited list of extensions supported by the device.

    size_t                       DisplayCount;         /// The number of attached displays.
    CG_DISPLAY                  *AttachedDisplays[AD]; /// The set of attached display objects.
    HDC                          DisplayDC[AD];        /// The set of Windows GDI device context for the attached displays.
    HGLRC                        DisplayRC;            /// The Windows OpenGL rendering context for the attached displays, or NULL.

    CL_DEVICE_CAPS               Capabilities;         /// Cached device capabilities reported by OpenCL.
    #undef  AD
};

/// @summary Define the state associated with an OpenGL 3.2 compatible display.
struct CG_DISPLAY
{
    uint32_t                     ObjectId;             /// The internal CGFX object identifier.
    DWORD                        Ordinal;              /// The zero-based index of the display passed to EnumDisplayDevices.
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

/// @summary Define the state associated with a device memory heap.
struct CG_HEAP
{
    cl_device_id                 ParentDeviceId;       /// The OpenCL device identifier of the device that owns the heap.
    int                          Type;                 /// One of cg_heap_type_e specifying the type of heap, relative to the device.
    uint32_t                     Flags;                /// A combination of cg_heap_flags_e specifying heap attributes.
    cl_device_type               DeviceType;           /// The type of OpenCL device that owns this heap.
    cl_ulong                     HeapSizeTotal;        /// The total size of the heap, in bytes.
    cl_ulong                     HeapSizeUsed;         /// An estimate of the number of bytes allocated from the heap.
    cl_ulong                     PinnableTotal;        /// The number of bytes of pinnable memory.
    cl_ulong                     PinnableUsed;         /// An estimate of the number of bytes of pinnable memory allocated from the heap.
    size_t                       DeviceAlignment;      /// The allocation alignment for device-allocated memory.
    size_t                       UserAlignment;        /// The allocation alignment for user-allocated memory.
    size_t                       UserSizeAlign;        /// The allocation size multiple for user-allocated memory.
};

/// @summary Define the data used to identify a device queue. The queue may be used for compute, graphics, or data transfer.
/// Command buffer construction may be performed from multiple threads, but command buffer submission must be performed from the 
/// thread that called cgEnumerateDevices().
struct CG_QUEUE
{
    uint32_t                     ObjectId;             /// The internal CGFX object identifier.
    int                          QueueType;            /// One of cg_queue_type_e.
    cl_context                   ComputeContext;       /// The OpenCL resource context associated with the queue.
    cl_command_queue             CommandQueue;         /// The OpenCL command queue for COMPUTE and TRANSFER queues. NULL for GRAPHICS queues.
    HDC                          DisplayDC;            /// The Windows GDI device context for GRAPHICS queues. NULL for COMPUTE and TRANSFER queues.
    HGLRC                        DisplayRC;            /// The Windows OpenGL rendering context for GRAPHICS queues. NULL for COMPUTE and TRANSFER queues.
};

/// @summary Define the data representing a command buffer, which is a set of commands and associated data that can be submitted to a queue.
/// Command buffers can be cached and re-used or re-submitted.
struct CG_CMD_BUFFER
{
    static size_t   const        ALLOCATION_GRANULARITY  = 64 * 1024;        // 64KB
    static size_t   const        CMD_HEADER_SIZE         = sizeof(uint32_t); // 4 bytes
    static size_t   const        MAX_CMD_SIZE            = 64 * 1024;        // 64KB
    static size_t   const        MAX_SIZE                = 16 * 1024 * 1024; // 16MB
    static uint32_t const        STATE_MASK_P            = 0x00FFFFFF;
    static uint32_t const        STATE_MASK_U            = 0x00FFFFFF;
    static uint32_t const        STATE_SHIFT             = 0;
    static uint32_t const        TYPE_MASK_P             = 0xFF000000;
    static uint32_t const        TYPE_MASK_U             = 0xFF;
    static uint32_t const        TYPE_SHIFT              = 24;

    enum state_e : uint32_t
    {
        UNINITIALIZED            = 0,                  /// The command buffer has not been initialized.
        BUILDING                 = 1,                  /// The command buffer is being constructed.
        MAP_APPEND               = 2,                  /// The command buffer is currently mapped for write.
        SUBMIT_READY             = 3,                  /// The command buffer is ready for submission.
        INCOMPLETE               = 4,                  /// The command buffer is incomplete (an error occurred during construction.)
    };

    uint32_t                     ObjectId;             /// The internal CGFX object identifier.
    uint32_t                     TypeAndState;         /// The type of commands contained in the buffer.
    size_t                       BytesTotal;           /// The number of bytes reserved for the command list.
    size_t                       BytesUsed;            /// The number of bytes actually used for command data.
    size_t                       CommandCount;         /// The number of buffered commands.
    uint8_t                     *CommandData;          /// The start of the command data buffer.
};

/// @summary Define the state associated with a device execution group.
struct CG_EXEC_GROUP
{
    uint32_t                     ObjectId;             /// The internal CGFX object identifier.
    cl_platform_id               PlatformId;           /// The OpenCL platform ID for all devices in the group.
    size_t                       DeviceCount;          /// The number of devices in the execution group.
    CG_DEVICE                  **DeviceList;           /// The devices making up the execution group.
    cl_device_id                *DeviceIds;            /// The OpenCL device ID for each device in the group.
    cl_context                  *ComputeContexts;      /// The compute context for each device in the group, which may reference the same context.
    CG_QUEUE                   **ComputeQueues;        /// The command queue used to submit compute kernel dispatch operations for each device in the group.
    CG_QUEUE                   **TransferQueues;       /// The command queue used to submit data transfer operations for each device in the group.
    size_t                       DisplayCount;         /// The number of displays attached to the group.
    CG_DISPLAY                 **AttachedDisplays;     /// The displays the execution group can output to.
    CG_QUEUE                   **GraphicsQueues;       /// The command queue used to submit graphics dispatch operations for each display in the group. 
    size_t                       QueueCount;           /// The number of unique queue objects associated with the group.
    CG_QUEUE                   **QueueList;            /// The set of references to queue objects owned by this execution group.
    size_t                       ContextCount;         /// The number of unique OpenCL context objects associated with the group.
    cl_context                  *ContextList;          /// The set of unique OpenCL context objects owned by this execution group.
};

/// @summary Define state associated with a graphics shader or compute kernel.
struct CG_KERNEL
{
    uint32_t                     ObjectId;             /// The internal CGFX object identifier.
    int                          KernelType;           /// One of cg_kernel_type_e specifying the shader stage.
    union
    {
        struct
        {
            size_t               DisplayCount;         /// The number of displays the kernel is compiled for.
            CG_DISPLAY         **DisplayList;          /// The set of OpenGL displays the kernel is compiled for.
            GLuint              *Shader;               /// The OpenGL shader object for each OpenGL display.
        }                        Graphics;             /// State valid if KernelType is CG_KERNEL_TYPE_GRAPHICS_*.
        struct
        {
            size_t               ContextCount;         /// The number of OpenCL contexts the kernel is compiled for.
            cl_context          *ContextList;          /// The set of OpenCL contexts the kernel is compiled for.
            cl_program          *Program;              /// The OpenCL program object for each OpenCL context.
        }                        Compute;              /// State valid if KernelType is CG_KERNEL_TYPE_COMPUTE.
    };
};

/// @summary Describes fixed-function state configuration for the blending unit.
struct CG_BLEND_STATE
{
    GLboolean                    BlendEnabled;         /// GL_TRUE if alpha blending is enabled.
    GLenum                       SrcBlendColor;        /// The source component of the blending equation for color channels.
    GLenum                       DstBlendColor;        /// The destination component of the blending equation for color channels.
    GLenum                       ColorBlendFunction;   /// The blending function to use for the color channels.
    GLenum                       SrcBlendAlpha;        /// The source component of the blending equation for alpha.
    GLenum                       DstBlendAlpha;        /// The destination component of the blending equation for alpha.
    GLenum                       AlphaBlendFunction;   /// The blending function to use for the alpha channel.
    GLfloat                      ConstantRGBA[4];      /// RGBA values in [0, 1] specifying a constant blend color.
};

/// @summary Describes fixed-function state configuration for the rasterizer.
struct CG_RASTER_STATE
{
    GLenum                       FillMode;             /// The primitive fill mode.
    GLenum                       CullMode;             /// The primitive culling mode.
    GLenum                       FrontFace;            /// The winding order used to determine front-facing primitives.
    GLint                        DepthBias;            /// The depth bias value added to fragment depth.
    GLfloat                      SlopeScaledDepthBias; /// The scale of the slope-based value added to fragment depth.
};

/// @summary Describes fixed-function state configuration for depth and stencil testing.
struct CG_DEPTH_STENCIL_STATE
{
    GLboolean                    DepthTestEnable;      /// GL_TRUE if depth testing is enabled.
    GLboolean                    DepthWriteEnable;     /// GL_TRUE if depth buffer writes are enabled.
    GLboolean                    DepthBoundsEnable;    /// GL_TRUE if the depth buffer range is enabled.
    GLenum                       DepthTestFunction;    /// The depth value comparison function.
    GLfloat                      DepthMin;             /// The minimum depth buffer value.
    GLfloat                      DepthMax;             /// The maximum depth buffer value.
    GLboolean                    StencilTestEnable;    /// GL_TRUE if stencil testing is enabled.
    GLenum                       StencilTestFunction;  /// The stencil value comparison function.
    GLenum                       StencilFailOp;        /// The stencil operation to apply when the stencil test fails.
    GLenum                       StencilPassZPassOp;   /// The stencil operation to apply when both the stencil and depth tests pass.
    GLenum                       StencilPassZFailOp;   /// The stencil operation to apply when the stencil test passes and the depth test fails.
    GLubyte                      StencilReadMask;      /// The bitmask to apply to stencil reads.
    GLubyte                      StencilWriteMask;     /// The bitmask to apply to stencil writes.
    GLubyte                      StencilReference;     /// The stencil reference value.
};

/// @summary Describes a GLSL vertex attribute value within a pipeline.
struct CG_GLSL_ATTRIBUTE
{
    GLenum                       DataType;             /// The data type, for example, GL_FLOAT.
    GLenum                       Location;             /// The assigned location within the program.
    size_t                       DataSize;             /// The size of the attribute data, in bytes.
    size_t                       DataOffset;           /// The byte offset of the attribute data.
    GLsizei                      Dimension;            /// The data dimension for array types.
};

/// @summary Describes a GLSL sampler value within a pipeline.
struct CG_GLSL_SAMPLER
{
    GLenum                       SamplerType;          /// The sampler type, for example, GL_SAMPLER_2D.
    GLenum                       BindTarget;           /// The texture bind target, for example, GL_TEXTURE_2D.
    GLint                        Location;             /// The assigned location within the program.
    GLint                        ImageUnit;            /// The assigned texture image unit.
};

/// @summary Describes a GLSL uniform value within a pipeline.
struct CG_GLSL_UNIFORM
{
    GLenum                       DataType;             /// The data type, for example, GL_FLOAT.
    GLenum                       Location;             /// The assigned location within the program.
    size_t                       DataSize;             /// The size of the attribute data, in bytes.
    size_t                       DataOffset;           /// The byte offset of the attribute data.
    GLsizei                      Dimension;            /// The data dimension for array types.
};

/// @summary Represents a GLSL program object post-linking, including reflection data.
struct CG_GLSL_PROGRAM
{
    size_t                       UniformCount;         /// The number of active GLSL uniforms.
    uint32_t                    *UniformNames;         /// Hashed names of the active GLSL uniforms.
    CG_GLSL_UNIFORM             *Uniforms;             /// Information about active GLSL uniforms.
    size_t                       AttributeCount;       /// The number of active vertex attributes.
    uint32_t                    *AttributeNames;       /// Hashed names of the active vertex attributes.
    CG_GLSL_ATTRIBUTE           *Attributes;           /// Information about active vertex attributes.
    size_t                       SamplerCount;         /// The number of active image samplers.
    uint32_t                    *SamplerNames;         /// Hashed names of the active GLSL samplers.
    CG_GLSL_SAMPLER             *Samplers;             /// Information about active GLSL samplers.
    GLuint                       Program;              /// The OpenGL program object generated by linking the kernels.
};

/// @summary Describes an OpenCL kernel argument within a pipeline.
struct CG_CL_KERNEL_ARG
{
    cl_uint                      Index;                /// The zero-based index of the argument.
    cl_uint                      MemoryType;           /// One of cl_kernel_arg_address_qualifier (GLOBAL, LOCAL, CONSTANT, PRIVATE.)
    cl_uint                      ImageAccess;          /// One of cl_kernel_arg_access_qualifier (READ_ONLY, WRITE_ONLY, READ_WRITE, NONE.)
    cl_bitfield                  TypeQualifier;        /// A combination of cl_kernel_arg_type_qualifier (CONST, RESTRICT, VOLATILE, NONE.)
};

/// @summary Describes OpenCL kernel work group information for a specific device.
struct CG_CL_WORKGROUP_INFO
{
    size_t                       WorkGroupSize;        /// The preferred work group size on the device.
    size_t                       FixedGroupSize[3];    /// The compile-time fixed work group size, or 0, 0, 0.
    cl_ulong                     LocalMemory;          /// The amount of local memory the kernel uses on the device, in bytes.
};

/// @summary Defines the data associated with an execution-ready compute pipeline.
struct CG_COMPUTE_PIPELINE
{
    size_t                       ContextCount;         /// The number of OpenCL contexts the kernel is compiled for.
    cl_context                  *ContextList;          /// The set of OpenCL contexts the kernel is compiled for. Reference to CG_EXEC_GROUP.
    cl_kernel                   *KernelList;           /// The set of OpenCL kernel objects, one per-context. Local.

    size_t                       DeviceCount;          /// The number of OpenCL devices the kernel can execute on.
    CG_DEVICE                  **DeviceList;           /// The set of OpenCL devices the kernel can execute on. Reference to CG_EXEC_GROUP.
    cl_device_id                *DeviceIds;            /// The set of OpenCL device IDs the kernel can execute on. Reference to CG_EXEC_GROUP.
    CG_QUEUE                   **ComputeQueues;        /// The handle of the command queue for each device the kernel can execute on. Reference to CG_EXEC_GROUP.
    cl_kernel                   *DeviceKernels;        /// The handle of the kernel program for each device the kernel can execute on. Local.
    CG_CL_WORKGROUP_INFO        *DeviceKernelInfo;     /// The work group and memory usage information for the kernel program on each device. Local.

    size_t                       ArgumentCount;        /// The number of kernel arguments.
    uint32_t                    *ArgumentNames;        /// Hashed names of kernel arguments. Local.
    CG_CL_KERNEL_ARG            *Arguments;            /// Information about kernel arguments. Local.
};

/// @summary Defines the data associated with an execution-read graphics pipeline.
struct CG_GRAPHICS_PIPELINE
{
    CG_DEPTH_STENCIL_STATE       DepthStencilState;    /// The fixed-function depth and stencil testing state to apply.
    CG_RASTER_STATE              RasterizerState;      /// The fixed-function rasterizer state to apply.
    CG_BLEND_STATE               BlendState;           /// The fixed-function blending unit state to apply.
    GLenum                       Topology;             /// The primitive topology, for example, GL_TRIANGLES.

    size_t                       DeviceCount;          /// The number of devices the pipeline can execute on.
    CG_DEVICE                  **DeviceList;           /// The set of devices the pipeline can execute on. Reference to CG_EXEC_GROUP.
    CG_GLSL_PROGRAM             *DevicePrograms;       /// The OpenGL program handle and reflection data for each device. Local.

    size_t                       DisplayCount;         /// The number of displays the pipeline can execute on.
    CG_DISPLAY                 **AttachedDisplays;     /// The set of displays the pipeline can execute on. Reference to CG_EXEC_GROUP.
    CG_QUEUE                   **GraphicsQueues;       /// The set of queues the pipeline can be bound on. Reference to CG_EXEC_GROUP.
    CG_GLSL_PROGRAM            **DisplayPrograms;      /// The set of program handle and reflection data for each display. Local.
};

/// @summary Defines the data associated with a pipeline object, which combines fixed-function and programmable state.
struct CG_PIPELINE
{
    uint32_t                     ObjectId;             /// The CGFX internal object identifier.
    int                          PipelineType;         /// One of cg_pipeline_type_e specifying the type of pipeline data.
    union
    {
        CG_COMPUTE_PIPELINE      Compute;              /// The state for a compute pipeline.
        CG_GRAPHICS_PIPELINE     Graphics;             /// The state for a graphics pipeline.
    };
};

/// @summary Defines the data associated with a data buffer object.
struct CG_BUFFER
{
    uint32_t                     ObjectId;             /// The CGFX internal object identifier.
    uint32_t                     KernelTypes;          /// One or more of cg_memory_object_kernel_e specifying the types of kernels that can access the buffer.
    CG_HEAP                     *SourceHeap;           /// The heap from which buffer memory is allocated.
    cl_context                   SourceContext;        /// The OpenCL context in which the object is allocated.
    cl_mem                       Compute;              /// The handle of the associated compute memory object, or NULL.
    size_t                       WriteOffset;          /// The current write offset, in bytes.
    size_t                       AllocatedSize;        /// The number of bytes actually allocated for the buffer.
    size_t                       RequestedSize;        /// The number of bytes requested for the buffer.
    cg_handle_t                  ExecutionGroup;       /// The handle of the execution group that owns the buffer.
    GLuint                       Graphics;             /// The handle of the OpenGL buffer object, or 0.
};

/// @summary Typedef the object tables held by a context object.
typedef CG_OBJECT_TABLE<CG_DEVICE    , CG_MAX_DEVICES    > CG_DEVICE_TABLE;
typedef CG_OBJECT_TABLE<CG_DISPLAY   , CG_MAX_DISPLAYS   > CG_DISPLAY_TABLE;
typedef CG_OBJECT_TABLE<CG_QUEUE     , CG_MAX_QUEUES     > CG_QUEUE_TABLE;
typedef CG_OBJECT_TABLE<CG_CMD_BUFFER, CG_MAX_CMD_BUFFERS> CG_CMD_BUFFER_TABLE;
typedef CG_OBJECT_TABLE<CG_EXEC_GROUP, CG_MAX_EXEC_GROUPS> CG_EXEC_GROUP_TABLE;
typedef CG_OBJECT_TABLE<CG_KERNEL    , CG_MAX_KERNELS    > CG_KERNEL_TABLE;
typedef CG_OBJECT_TABLE<CG_PIPELINE  , CG_MAX_PIPELINES  > CG_PIPELINE_TABLE;
typedef CG_OBJECT_TABLE<CG_BUFFER    , CG_MAX_BUFFERS    > CG_BUFFER_TABLE;

/// @summary Define the state associated with a CGFX instance, created when devices are enumerated.
struct CG_CONTEXT
{
    CG_HOST_ALLOCATOR            HostAllocator;        /// The allocator implementation used to allocate host memory.

    cg_cpu_counts_t              CPUCounts;            /// Information about the CPU resources available in the local system.

    size_t                       HeapCount;            /// The number of heaps defined by all devices in the local system.
    CG_HEAP                     *HeapList;             /// The set of heaps defined by all devices in the local system.

    CG_DEVICE_TABLE              DeviceTable;          /// The object table of all OpenCL 1.2-capable compute devices.
    CG_DISPLAY_TABLE             DisplayTable;         /// The object table of all OpenGL 3.2-capable display devices.
    CG_QUEUE_TABLE               QueueTable;           /// The object table of all active command queues.
    CG_CMD_BUFFER_TABLE          CmdBufferTable;       /// The object table of all active command buffers.
    CG_EXEC_GROUP_TABLE          ExecGroupTable;       /// The object table of all active execution groups.
    CG_KERNEL_TABLE              KernelTable;          /// The object table of all compiled kernels.
    CG_PIPELINE_TABLE            PipelineTable;        /// The object table of all compiled pipelines.
    CG_BUFFER_TABLE              BufferTable;          /// The object table of all data buffers.
};

/*/////////////////
//   Functions   //
/////////////////*/
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

/// @summary Construct a handle out of its constituent parts.
/// @param object_id The object identifier within the parent object table.
/// @param object_type The object type identifier. One of cg_object_e.
/// @param table_index The zero-based index of the parent object table. Max value 255.
/// @return The unique object handle.
public_function inline cg_handle_t
cgMakeHandle
(
    uint32_t object_id, 
    uint32_t object_type,
    size_t   table_index=0
)
{
    assert(table_index < UINT8_MAX);
    uint64_t I =(uint64_t(object_id  ) & CG_HANDLE_MASK_I_U) << CG_HANDLE_SHIFT_I;
    uint64_t Y =(uint64_t(object_type) & CG_HANDLE_MASK_Y_U) << CG_HANDLE_SHIFT_Y;
    uint64_t T =(uint64_t(table_index) & CG_HANDLE_MASK_T_U) << CG_HANDLE_SHIFT_T;
    return  (I | Y | T);
}

/// @summary Update the object identifier of a handle.
/// @param handle The handle to update.
/// @param object_id The object identifier within the parent object table.
/// @return The modified handle value.
public_function inline cg_handle_t
cgSetHandleId
(
    cg_handle_t handle, 
    uint32_t    object_id
)
{
    return (handle & ~CG_HANDLE_MASK_I_P) | ((uint64_t(object_id  ) & CG_HANDLE_MASK_I_U) << CG_HANDLE_SHIFT_I);
}

/// @summary Update the object type of a handle.
/// @param handle The handle to update.
/// @param object_type The object type identifier. One of cg_object_e.
/// @return The modified handle value.
public_function inline cg_handle_t
cgSetHandleObjectType
(
    cg_handle_t handle, 
    uint32_t    object_type
)
{
    return (handle & ~CG_HANDLE_MASK_Y_P) | ((uint64_t(object_type) & CG_HANDLE_MASK_Y_U) << CG_HANDLE_SHIFT_Y);
}

/// @summary Update the table index of a handle.
/// @param handle The handle to update.
/// @param table_index The zero-based index of the owning object table. Max value 255.
/// @return The modified handle value.
public_function inline cg_handle_t
cgSetHandleTableIndex
(
    cg_handle_t handle, 
    size_t      table_index
)
{
    assert(table_index < UINT8_MAX);
    return (handle & ~CG_HANDLE_MASK_T_P) | ((uint64_t(table_index) & CG_HANDLE_MASK_T_U) << CG_HANDLE_SHIFT_T);
}

/// @summary Break a handle into its constituent parts.
/// @param handle The handle to crack.
/// @param object_id On return, stores the object identifier within the owning object table.
/// @param object_type On return, stores the object type identifier, one of cg_object_e.
/// @param table_index On return, stores the zero-based index of the parent object table.
public_function inline void
cgHandleParts
(
    cg_handle_t handle, 
    uint32_t   &object_id, 
    uint32_t   &object_type,
    size_t     &table_index
)
{
    object_id      = (uint32_t) ((handle & CG_HANDLE_MASK_I_P) >> CG_HANDLE_SHIFT_I);
    object_type    = (uint32_t) ((handle & CG_HANDLE_MASK_Y_P) >> CG_HANDLE_SHIFT_Y);
    table_index    = (size_t  ) ((handle & CG_HANDLE_MASK_T_P) >> CG_HANDLE_SHIFT_T);
}

/// @summary Extract the object identifier from an object handle.
/// @param handle The handle to inspect.
/// @return The object identifier.
public_function inline uint32_t
cgGetObjectId
(
    cg_handle_t handle
)
{
    return (uint32_t) ((handle & CG_HANDLE_MASK_I_P) >> CG_HANDLE_SHIFT_I);
}

/// @summary Extract the object type identifier from an object handle.
/// @param handle The handle to inspect.
/// @return The object type identifier, one of cg_object_e.
public_function inline uint32_t
cgGetObjectType
(
    cg_handle_t handle
)
{
    return (uint32_t) ((handle & CG_HANDLE_MASK_Y_P) >> CG_HANDLE_SHIFT_Y);
}

/// @summary Extract the index of the parent object table from an object handle.
/// @param handle The handle to inspect.
/// @return The zero-based index of the parent object table.
public_function inline size_t
cgGetTableIndex
(
    cg_handle_t handle
)
{
    return (size_t) ((handle & CG_HANDLE_MASK_T_P) >> CG_HANDLE_SHIFT_T);
}

/// @summary Initialize an object table to empty. This touches at least 512KB of memory.
/// @param table The object table to initialize or clear.
/// @param object_type The object type identifier for all objects in the table, one of cg_object_e.
/// @param table_index The zero-based index of the object table. Max value 255.
template <typename T, size_t N>
public_function inline void
cgObjectTableInit
(
    CG_OBJECT_TABLE<T, N> *table, 
    uint32_t               object_type, 
    size_t                 table_index=0
)
{
    table->ObjectCount   = 0;
    table->FreeListTail  = CG_OBJECT_TABLE<T,N>::MAX_OBJECTS - 1;
    table->FreeListHead  = 0;
    table->ObjectType    = object_type;
    table->TableIndex    = table_index;
    for (size_t i = 0; i < CG_OBJECT_TABLE<T,N>::MAX_OBJECTS; ++i)
    {
        table->Indices[i].Id   = uint32_t(i);
        table->Indices[i].Next = uint16_t(i+1);
    }
}

/// @summary Check whether an object table contains a given item.
/// @param table The object table to query.
/// @param handle The handle of the object to query.
/// @return true if the handle references an object within the table.
template <typename T, size_t N>
public_function inline bool
cgObjectTableHas
(
    CG_OBJECT_TABLE<T, N> *table, 
    cg_handle_t            handle
)
{
    uint32_t        const  objid = cgGetObjectId(handle);
    CG_OBJECT_INDEX const &index = table->Indices[objid & CG_OBJECT_TABLE<T,N>::INDEX_MASK];
    return (index.Id == objid && index.Index != CG_OBJECT_TABLE<T,N>::INDEX_INVALID);
}

/// @summary Retrieve an item from an object table.
/// @param table The object table to query.
/// @param handle The handle of the object to query.
/// @return A pointer to the object within the table, or NULL.
template <typename T, size_t N>
public_function inline T* 
cgObjectTableGet
(
    CG_OBJECT_TABLE<T, N> *table,
    cg_handle_t            handle
)
{
    uint32_t        const  objid = cgGetObjectId(handle);
    CG_OBJECT_INDEX const &index = table->Indices[objid & CG_OBJECT_TABLE<T,N>::INDEX_MASK];
    if (index.Id == objid && index.Index != CG_OBJECT_TABLE<T,N>::INDEX_INVALID)
    {
        return &table->Objects[index.Index];
    }
    else return NULL;
}

/// @summary Add a new object to an object table.
/// @param table The object table to update.
/// @param data The object to insert into the table.
/// @return The handle of the object within the table, or CG_INVALID_HANDLE.
template <typename T, size_t N>
public_function inline cg_handle_t
cgObjectTableAdd
(
    CG_OBJECT_TABLE<T, N> *table, 
    T const               &data
)
{
    if (table->ObjectCount < CG_OBJECT_TABLE<T,N>::MAX_OBJECTS-1)
    {
        uint32_t const    type = table->ObjectType;
        size_t   const    tidx = table->TableIndex;
        CG_OBJECT_INDEX &index = table->Indices[table->FreeListHead];     // retrieve the next free object index
        table->FreeListHead    = index.Next;                              // pop the item from the free list
        index.Id              += CG_OBJECT_TABLE<T,N>::NEW_OBJECT_ID_ADD; // assign the new item a unique ID
        index.Index            =(uint16_t) table->ObjectCount;            // allocate the next unused slot in the packed array
        T &object              = table->Objects[index.Index];             // object references the allocated slot
        object                 = data;                                    // copy data into the newly allocated slot
        object.ObjectId        = index.Id;                                // and then store the new object ID in the data
        table->ObjectCount++;                                             // update the number of valid items in the table
        return cgMakeHandle(index.Id, type, tidx);
    }
    else return CG_INVALID_HANDLE;
}

/// @summary Remove an item from an object table.
/// @param table The object table to update.
/// @param handle The handle of the item to remove from the table.
/// @param existing On return, a copy of the removed item is placed in this location. This can be useful if the removed object has memory references that need to be cleaned up.
/// @return true if the item was removed and the data copied to existing.
template <typename T, size_t N>
public_function inline bool
cgObjectTableRemove
(
    CG_OBJECT_TABLE<T, N> *table, 
    cg_handle_t            handle, 
    T                     &existing
)
{
    uint32_t       const  objid = cgGetObjectId(handle);
    CG_OBJECT_INDEX      &index = table->Indices[objid & CG_OBJECT_TABLE<T,N>::INDEX_MASK];
    if (index.Id == objid && index.Index != CG_OBJECT_TABLE<T,N>::INDEX_INVALID)
    {   // object refers to the object being deleted.
        // save the data for the caller in case they need to free memory.
        // copy the data for the last object in the packed array over the item being deleted.
        // then, update the index of the item that was moved to reference its new location.
        T &object = table->Objects[index.Index];
        existing  = object;
        object    = table->Objects[table->ObjectCount-1];
        table->Indices[object.ObjectId & CG_OBJECT_TABLE<T,N>::INDEX_MASK].Index = index.Index;
        table->ObjectCount--;
        // mark the deleted object as being invalid.
        // return the object index to the free list.
        index.Index = CG_OBJECT_TABLE<T,N>::INDEX_INVALID;
        table->Indices[table->FreeListTail].Next = objid & CG_OBJECT_TABLE<T,N>::INDEX_MASK;
        table->FreeListTail = objid & CG_OBJECT_TABLE<T,N>::INDEX_MASK;
        return true;
    }
    else return false;
}

/// @summary Create a handle to the 'i-th' object in a table.
/// @param table The object table to query.
/// @param object_index The zero-based index of the object within the table.
/// @return The handle value used to reference the object externally.
template <typename T, size_t N>
public_function inline cg_handle_t
cgMakeHandle
(
    CG_OBJECT_TABLE<T, N> *table, 
    size_t                 object_index
)
{
    return cgMakeHandle(table->Objects[object_index].ObjectId, table->ObjectType, table->TableIndex);
}

#undef  CGFX_WIN32_INTERNALS_DEFINED
#define CGFX_WIN32_INTERNALS_DEFINED
#endif /* !defined(LIB_CGFX_W32_PRIVATE_H) */
