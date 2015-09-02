/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implements the entry point of the CGFX test application.
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

/// @summary Somewhere in our include chain, someone includes <dxgiformat.h>.
/// Don't re-define the DXGI_FORMAT and D3D11 enumerations.
#define CG_DXGI_ENUMS_DEFINED 1

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
#include <string.h>
#include <stdio.h>

#include "cgfx.h"
#include "cgfx_ext_win.h"
#include "cgfx_kernel_compute.h"
#include "cgfx_kernel_graphics.h"

/*/////////////////
//   Constants   //
/////////////////*/
/// @summary The scale used to convert from seconds into nanoseconds.
static uint64_t const SEC_TO_NANOSEC = 1000000000ULL;

/*///////////////////
//   Local Types   //
///////////////////*/
/// @summary Encapsulate CGFX state for the Window attached to the primary display.
struct cgfx_state_t
{
    uintptr_t   Context;
    cg_handle_t Display;
    cg_handle_t DisplayDevice;
    cg_handle_t GPUGroup;
    cg_handle_t GPUComputeQueue;
    cg_handle_t GPUGraphicsQueue;
    cg_handle_t GPUTransferQueue;
    cg_handle_t CPUGroup;
    cg_handle_t CPUComputeQueue;
    cg_handle_t CPUTransferQueue;
    HWND        Window;
    HDC         Drawable;
};

/*///////////////
//   Globals   //
///////////////*/
/// @summary A flag used to control whether the main application loop is still running.
global_variable bool            Global_IsRunning       = true;

/// @summary The high-resolution timer frequency. This is queried once at startup.
global_variable int64_t         Global_ClockFrequency  = {0};

/// @summary The CGFX state for the primary window and display.
global_variable cgfx_state_t    Global_CGFX            = {0};

/// @summary Information about the window placement saved when entering fullscreen mode.
/// This data is necessary to restore the window to the same position and size when 
/// the user exits fullscreen mode back to windowed mode.
global_variable WINDOWPLACEMENT Global_WindowPlacement = {0};

/*///////////////////////
//   Local Functions   //
///////////////////////*/
/// @summary Format a message and write it to the debug output channel.
/// @param fmt The printf-style format string.
/// ... Substitution arguments.
internal_function void dbg_printf(char const *fmt, ...)
{
    char buffer[1024];
    va_list  args;
    va_start(args, fmt);
    vsnprintf_s(buffer, 1024, fmt, args);
    va_end  (args);
    OutputDebugStringA(buffer);
}

/// @summary Retrieve the current high-resolution timer value. 
/// @return The current time value, specified in system ticks.
internal_function inline int64_t ticktime(void)
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter.QuadPart;
}

/// @summary Retrieve the current high-resolution timer value.
/// @return The current time value, specified in nanoseconds.
internal_function inline uint64_t nanotime(void)
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (SEC_TO_NANOSEC * uint64_t(counter.QuadPart) / uint64_t(Global_ClockFrequency));
}

/// @summary Retrieve the current high-resolution timer value.
/// @param frequency The clock frequency, in ticks-per-second.
/// @return The current time value, specified in nanoseconds.
internal_function inline uint64_t nanotime(int64_t frequency)
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (SEC_TO_NANOSEC * uint64_t(counter.QuadPart) / uint64_t(frequency));
}

/// @summary Calculate the elapsed time in nanoseconds.
/// @param start The nanosecond timestamp at the start of the measured interval.
/// @param end The nanosecond timestamp at the end of the measured interval.
/// @return The elapsed time, specified in nanoseconds.
internal_function inline int64_t elapsed_nanos(uint64_t start, uint64_t end)
{
    return int64_t(end - start);
}

/// @summary Calculate the elapsed time in system ticks.
/// @param start The timestamp at the start of the measured interval.
/// @param end The timestamp at the end of the measured interval.
/// @return The elapsed time, specified in system ticks.
internal_function inline int64_t elapsed_ticks(int64_t start, int64_t end)
{
    return (end - start);
}

/// @summary Convert a time duration specified in system ticks to seconds.
/// @param ticks The duration, specified in system ticks.
/// @return The specified duration, specified in seconds.
internal_function inline float ticks_to_seconds(int64_t ticks)
{
    return ((float) ticks / (float) Global_ClockFrequency);
}

/// @summary Convert a time duration specified in system ticks to seconds.
/// @param ticks The duration, specified in system ticks.
/// @param frequency The clock frequency in ticks-per-second.
/// @return The specified duration, specified in seconds.
internal_function inline float ticks_to_seconds(int64_t ticks, int64_t frequency)
{
    return ((float) ticks / (float) frequency);
}

/// @summary Convert a time duration specified in nanoseconds to seconds.
/// @param nanos The duration, specified in nanoseconds.
/// @return The specified duration, specified in nanoseconds.
internal_function inline float nanos_to_seconds(int64_t nanos)
{
    return ((float) nanos / (float) SEC_TO_NANOSEC);
}

/// @summary Toggles the window styles of a given window between standard 
/// windowed mode and a borderless fullscreen" mode. The display resolution
/// is not changed, so rendering will be performed at the desktop resolution.
/// @param window The window whose styles will be updated.
internal_function void toggle_fullscreen(HWND window)
{
    LONG_PTR style = GetWindowLongPtr(window, GWL_STYLE);
    if (style & WS_OVERLAPPEDWINDOW)
    {   // switch to a fullscreen-style window.
        MONITORINFO monitor_info = { sizeof(MONITORINFO)     };
        WINDOWPLACEMENT win_info = { sizeof(WINDOWPLACEMENT) };
        if (GetWindowPlacement(window, &win_info) && GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &monitor_info))
        {
            RECT rc = monitor_info.rcMonitor;
            Global_WindowPlacement = win_info;
            SetWindowLongPtr(window, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(window, HWND_TOP , rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {   // go back to windowed mode.
        SetWindowLongPtr(window, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(window, &Global_WindowPlacement);
        SetWindowPos(window, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

/// @summary Implement the Windows message callback for the main application window.
/// @param hWnd The handle of the main application window.
/// @param uMsg The identifier of the message being processed.
/// @param wParam An unsigned pointer-size value specifying data associated with the message.
/// @param lParam A signed pointer-size value specifying data associated with the message.
/// @return The return value depends on the message being processed.
internal_function LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        {	// 
            if (hWnd == Global_CGFX.Window && Global_CGFX.Drawable != NULL)
            {   // deactivate the rendering context on the current thread.
                cgSetActiveDrawableEXT(Global_CGFX.Context, NULL);
                ReleaseDC(hWnd, Global_CGFX.Drawable);
                Global_CGFX.Drawable = NULL;
                Global_CGFX.Window   = NULL;
            }
            // post the quit message with the application exit code.
			// this will be picked up back in WinMain and cause the
			// message pump and display update loop to terminate.
            PostQuitMessage(EXIT_SUCCESS);
        }
        return 0;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
        {   // application-level user keyboard input handling.
            uint32_t vk_code  = (uint32_t)  wParam;
            bool     is_down  = ((lParam & (1 << 31)) == 0);
            bool     was_down = ((lParam & (1 << 30)) != 0);
            
            if (is_down)
            {
                bool alt_down = ((lParam & (1 << 29)) != 0);
                if (vk_code == VK_RETURN && alt_down)
				{
                    toggle_fullscreen(hWnd);
                    return 0; // prevent default handling.
                }
            }
        }
        break;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/// @summary Performs one-time setup for CGFX and creates the main application window.
/// @param cgfx On return, the fields of this structure reference CGFX resources.
/// @return true if setup completed successfully.
internal_function bool cgfx_setup(cgfx_state_t *cgfx)
{
    cg_application_info_t app             = {};
    cg_execution_group_t  gpu_group_def   = {};
    cg_execution_group_t  cpu_group_def   = {};
    cg_cpu_partition_t    cpu_partition   = {};
    cg_cpu_info_t         cpu_info        = {};
    cg_handle_t           primary_display = CG_INVALID_HANDLE;
    cg_handle_t           display_device  = CG_INVALID_HANDLE;
    cg_handle_t           primary_cpu     = CG_INVALID_HANDLE;
    cg_handle_t           gpu_group       = CG_INVALID_HANDLE;
    cg_handle_t           cpu_group       = CG_INVALID_HANDLE;
    uintptr_t             ctx             = NULL;
    size_t                num_devices_all = 0;
    size_t                num_devices_cpu = 0;
    HWND                  wnd             = NULL;
    HDC                   dc              = NULL;
    int                   res             = CG_SUCCESS;

    // initialize the output state to invalid everything.
    memset(cgfx, 0, sizeof(cgfx_state_t));

    // retrieve a description of CPU resources on the local system.
    if ((res = cgGetCpuInfo(&cpu_info)) != CG_SUCCESS)
    {
        OutputDebugString(_T("ERROR: Unable to retrieve CPU device information:\n  "));
        OutputDebugStringA(cgResultString(res));
        OutputDebugString(_T("\n"));
        return false;
    }

    // fill our some basic information about our application:
    app.AppName        = "cgfxTest";
    app.AppVersion     =  CG_MAKE_VERSION(1, 0, 0);
    app.DriverName     = "cgfxTest Driver";
    app.DriverVersion  =  CG_MAKE_VERSION(1, 0, 0);
    app.ApiVersion     =  CG_API_VERSION;

    // fill out information describing how the CPU should be partitioned.
    // we'll set it up for data-parallel operation, but with a single 
    // physical reserved for running all application logic.
    cpu_partition.PartitionType  =  CG_CPU_PARTITION_NONE;
    cpu_partition.ReserveThreads =  cpu_info.ThreadsPerCore;
    cpu_partition.PartitionCount =  0;
    cpu_partition.ThreadCounts   =  NULL;

    // ask CGFX to enumerate the devices in the system according to our preferences.
    // this returns a CGFX context that is used during resource creation.
    if ((res = cgEnumerateDevices(&app, NULL, &cpu_partition, num_devices_all, NULL, 0, ctx)) != CG_SUCCESS)
    {
        OutputDebugString(_T("ERROR: Unable to enumerate CGFX devices:\n  "));
        OutputDebugStringA(cgResultString(res));
        OutputDebugString(_T("\n"));
        return false;
    }
    if (num_devices_all == 0)
    {   cgDestroyContext(ctx);
        OutputDebugString(_T("ERROR: No CGFX-compatible devices found.\n"));
        return false;
    }

    // retrieve the default display and ensure there's a device attached.
    // the device will be a GPU device. use this device to create an 
    // execution group, including all available GPUs. the display 
    // device becomes the root device of the execution group, which defines
    // the share group. device resources can be shared amongst all devices 
    // in the share group, but cannot be accessed outside of the group 
    // without host intervention (the host must map and copy data manually.)
    // since we'll use the devices in this group for display purposes, the 
    // CG_EXECUTION_GROUP_DISPLAY_OUTPUT flag must be specified.
    if ((primary_display = cgGetPrimaryDisplay(ctx)) == CG_INVALID_HANDLE)
    {   cgDestroyContext(ctx);
        OutputDebugString(_T("ERROR: No display devices attached to the local system.\n"));
        return false;
    }
    if ((display_device  = cgGetDisplayDevice(ctx, primary_display)) == CG_INVALID_HANDLE)
    {   cgDestroyContext(ctx);
        OutputDebugString(_T("ERROR: No CGFX-compatible device attached to the primary display.\n"));
        return false;
    }
    gpu_group_def.RootDevice      = display_device;   // the GPU defines the share group
    gpu_group_def.DeviceCount     = 0;    // don't explicitly include any other devices
    gpu_group_def.DeviceList      = NULL; // don't explicitly include any other devices
    gpu_group_def.ExtensionCount  = 0;    // we aren't configuring any optional extensions
    gpu_group_def.ExtensionNames  = NULL; // we aren't configuring any optional extensions
    gpu_group_def.CreateFlags     = CG_EXECUTION_GROUP_DISPLAY_OUTPUT;
    gpu_group_def.ValidationLevel = 0;    // don't enable thorough validation
    if ((gpu_group = cgCreateExecutionGroup(ctx, &gpu_group_def, res)) == CG_INVALID_HANDLE)
    {   cgDestroyContext(ctx);
        OutputDebugString(_T("ERROR: Unable to create the GPU execution group:\n  "));
        OutputDebugStringA(cgResultString(res));
        OutputDebugString(_T("\n"));
        return false;
    }

    // create a second execution group containing all of the available CPU devices.
    // while it is possible for CPU and GPU devices to share resources in some cases 
    // (such as Intel Integrated Graphics) this sharing doesn't seem to work if 
    // the GPU device is used for display output. for consistency with discrete GPU 
    // operation CPUs are maintained in a totally separate execution and share group.
    // since we didn't partition the device during enumeration, there will only be 
    // a single CPU device, if any, and thus a single set of queues. had we partitioned
    // the CPU into multiple devices (one per NUMA node, or one per-core) there would
    // be multiple devices, each with their own set of queues.
    cgGetCPUDevices(ctx, num_devices_cpu, 1, &primary_cpu);
    if (num_devices_cpu > 0)
    {   // there's at least one OpenCL-capable CPU present. include all capable CPUs.
        cpu_group_def.RootDevice      = primary_cpu;
        cpu_group_def.DeviceCount     = 0;
        cpu_group_def.DeviceList      = NULL;
        cpu_group_def.ExtensionCount  = 0;
        cpu_group_def.ExtensionNames  = NULL;
        cpu_group_def.CreateFlags     = CG_EXECUTION_GROUP_CPUS;
        cpu_group_def.ValidationLevel = 0;
        if ((cpu_group = cgCreateExecutionGroup(ctx, &cpu_group_def, res)) == CG_INVALID_HANDLE)
        {   cgDestroyContext(ctx);
            OutputDebugString(_T("ERROR: Unable to create the CPU execution group:\n  "));
            OutputDebugStringA(cgResultString(res));
            OutputDebugString(_T("\n"));
            return false;
        }
    }

    // use the Win32 extension API to create and configure an OpenGL display window.
    // note that the application is still responsible for creating the window class 
    // and has full control over the newly-created window. if you have an existing 
    // window that you want to use for display output, you can use cgAttachWindowToDisplayEXT
    // instead, which will ensure that the window can be used for OpenGL output.
    // the cgMakeWindowOnDisplay arguments are equivalent to those for CreateWindowEx.
    DWORD const  style   = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE;
    DWORD const  styleex = 0;
    TCHAR const *clsname = _T("cgfxTestWndClass");
    TCHAR const *wndname = _T("CGFX");
    HINSTANCE    exeinst = GetModuleHandle(NULL);
    WNDCLASSEX   wndcls  = {};
    if (!GetClassInfoEx(exeinst, clsname, &wndcls))
    {   // the window class has not been registered yet.
        wndcls.cbSize         = sizeof(WNDCLASSEX);
        wndcls.cbClsExtra     = 0;
        wndcls.cbWndExtra     = sizeof(void*);
        wndcls.hInstance      = exeinst;
        wndcls.lpszClassName  = clsname;
        wndcls.lpszMenuName   = NULL;
        wndcls.lpfnWndProc    = MainWndProc;
        wndcls.hIcon          = LoadIcon  (0, IDI_APPLICATION);
        wndcls.hIconSm        = LoadIcon  (0, IDI_APPLICATION);
        wndcls.hCursor        = LoadCursor(0, IDC_ARROW);
        wndcls.style          = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
        wndcls.hbrBackground  = NULL;
        if (!RegisterClassEx(&wndcls))
        {   // unable to register the hidden window class - don't proceed.
            return CG_SUCCESS;
        }
    }
    if ((wnd = cgMakeWindowOnDisplayEXT(ctx, primary_display, clsname, wndname, style, styleex, 0, 0, 800, 600, NULL, NULL, exeinst, NULL, NULL)) == NULL)
    {   cgDestroyContext(ctx);
        OutputDebugString(_T("ERROR: Unable to create window on primary display.\n"));
        return false;
    }

    // done with basic initialization:
    cgfx->Context          = ctx;
    cgfx->Display          = primary_display;
    cgfx->DisplayDevice    = display_device;
    cgfx->GPUGroup         = gpu_group;
    cgfx->GPUComputeQueue  = cgGetQueueForDevice(ctx, display_device, CG_QUEUE_TYPE_COMPUTE , res);
    cgfx->GPUGraphicsQueue = cgGetQueueForDevice(ctx, display_device, CG_QUEUE_TYPE_GRAPHICS, res);
    cgfx->GPUTransferQueue = cgGetQueueForDevice(ctx, display_device, CG_QUEUE_TYPE_TRANSFER, res);
    cgfx->Window           = wnd;
    cgfx->Drawable         = GetDC(wnd);
    if (cpu_group != CG_INVALID_HANDLE)
    {   // store the CPU execution group information.
        cgfx->CPUGroup         = cpu_group;
        cgfx->CPUComputeQueue  = cgGetQueueForDevice(ctx, primary_cpu, CG_QUEUE_TYPE_COMPUTE , res);
        cgfx->CPUTransferQueue = cgGetQueueForDevice(ctx, primary_cpu, CG_QUEUE_TYPE_TRANSFER, res);
    }
    return true;
}

/// @summary Clean up CGFX and display resources on application shutdown.
/// @param cgfx The CGFX device state to delete.
internal_function void cgfx_teardown(cgfx_state_t *cgfx)
{   // destroy the window if it hasn't already happened via the WndProc.
    if (cgfx->Window != NULL)
    {   // deactivate the rendering context on the current thread first.
        cgSetActiveDrawableEXT(cgfx->Context, NULL);
        ReleaseDC(cgfx->Window, cgfx->Drawable);
        DestroyWindow(cgfx->Window);
        cgfx->Drawable = NULL;
        cgfx->Window   = NULL;
    }
    // destroy the context itself, which frees all active resources.
    cgDestroyContext(cgfx->Context);
    // zero out all of the pointers and handles.
    memset(cgfx, 0, sizeof(cgfx_state_t));
}

/*////////////////////////
//   Public Functions   //
////////////////////////*/
/// @summary Implements the entry point of the application.
/// @param hInstance A handle to the current instance of the application.
/// @param hPrev A handle to the previous instance of the application. Always NULL.
/// @param lpCmdLine The command line for the application, excluding the program name.
/// @param nCmdShow Controls how the window is to be shown. See MSDN for possible values.
/// @return Zero if the message loop was not entered, or the value of the WM_QUIT wParam.
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow)
{
#ifdef  _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    int cgres  = CG_SUCCESS;
    int result = 0; // return zero if the message loop isn't entered.
    UNREFERENCED_PARAMETER(hPrev);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    // query the clock frequency of the high-resolution timer.
    LARGE_INTEGER clock_frequency;
    QueryPerformanceFrequency(&clock_frequency);
    Global_ClockFrequency = clock_frequency.QuadPart;

    // set the scheduler granularity to 1ms for more accurate Sleep.
    UINT desired_granularity = 1; // millisecond
    BOOL sleep_is_granular   =(timeBeginPeriod(desired_granularity) == TIMERR_NOERROR);

    // create the main application window on the primary display.
    HWND main_window = NULL;
    if (!cgfx_setup(&Global_CGFX))
    {
        return 0;
    }

    cg_handle_t vb = CG_INVALID_HANDLE;
    cg_handle_t ib = CG_INVALID_HANDLE;
    cg_handle_t vs = CG_INVALID_HANDLE;
    cg_handle_t gp = CG_INVALID_HANDLE;
    cg_handle_t cb = CG_INVALID_HANDLE;
    size_t                 attrib_counts[1] = { 2 };
    cg_vertex_attribute_t  attrib_pos       = { 0, 0, CG_ATTRIBUTE_FORMAT_FLOAT32, 3,  0, false };
    cg_vertex_attribute_t  attrib_clr       = { 0, 1, CG_ATTRIBUTE_FORMAT_UINT8  , 4, 12, true  };
    cg_vertex_attribute_t  attrib_buf0[2]   = { attrib_pos, attrib_clr };
    cg_vertex_attribute_t *attrib_data[1]   = { attrib_buf0 };

    // @note
    // cgSetActiveDrawableEXT is necessary to make the context current; otherwise, GL calls will fail.
    // CG_MEMORY_OBJECT_KERNEL_COMPUTE must be specified or the buffers will not be mappable.
    cgSetActiveDrawableEXT(Global_CGFX.Context , Global_CGFX.Drawable);
    vb = cgCreateDataBuffer(Global_CGFX.Context, Global_CGFX.GPUGroup, sizeof(CG_GFX_TEST01_VERTEX) * 4, CG_MEMORY_OBJECT_KERNEL_GRAPHICS | CG_MEMORY_OBJECT_KERNEL_COMPUTE, CG_MEMORY_ACCESS_READ, CG_MEMORY_ACCESS_WRITE, CG_MEMORY_PLACEMENT_DEVICE, CG_MEMORY_UPDATE_ONCE, cgres);
    ib = cgCreateDataBuffer(Global_CGFX.Context, Global_CGFX.GPUGroup, sizeof(uint16_t)             * 6, CG_MEMORY_OBJECT_KERNEL_GRAPHICS | CG_MEMORY_OBJECT_KERNEL_COMPUTE, CG_MEMORY_ACCESS_READ, CG_MEMORY_ACCESS_WRITE, CG_MEMORY_PLACEMENT_DEVICE, CG_MEMORY_UPDATE_ONCE, cgres);
    vs = cgCreateVertexDataSource(Global_CGFX.Context, Global_CGFX.GPUGroup, 1, &vb, ib, attrib_counts ,(cg_vertex_attribute_t const **) attrib_data, cgres);
    gp = cgCreateGraphicsPipelineTest01(Global_CGFX.Context, Global_CGFX.GPUGroup, cgres);
    cb = cgCreateCommandBuffer(Global_CGFX.Context, CG_QUEUE_TYPE_GRAPHICS, cgres);

    // the data transfers are performed using OpenCL. the buffers are mapped to host-accessible memory
    // and the host fills them. when they are unmapped, the DMA engine copies the data to device memory.
    CG_GFX_TEST01_VERTEX *vtx = (CG_GFX_TEST01_VERTEX*) cgMapDataBuffer(Global_CGFX.Context, Global_CGFX.GPUTransferQueue, vb, CG_INVALID_HANDLE, 0, sizeof(CG_GFX_TEST01_VERTEX) * 4, CG_MEMORY_ACCESS_WRITE, cgres);
    if (vtx != NULL)
    {   // the quad is positioned using screen coordinates, with origin at the center.
        // draw a 100x100 quad positioned at the center of the screen.
        vtx[0].Position[0] = 350.0f;
        vtx[0].Position[1] = 250.0f;
        vtx[0].Position[2] = 0.0f;
        vtx[0].RGBA = 0xFF0000FF;

        vtx[1].Position[0] = 450.0f;
        vtx[1].Position[1] = 250.0f;
        vtx[1].Position[2] = 0.0f;
        vtx[1].RGBA = 0xFFFF00FF;

        vtx[2].Position[0] = 450.0f;
        vtx[2].Position[1] = 350.0f;
        vtx[2].Position[2] = 0.0f;
        vtx[2].RGBA = 0xFF00FFFF;

        vtx[3].Position[0] = 350.0f;
        vtx[3].Position[1] = 350.0f;
        vtx[3].Position[2] = 0.0f;
        vtx[3].RGBA = 0xFFFFFFFF;
        cgUnmapDataBuffer(Global_CGFX.Context, Global_CGFX.GPUTransferQueue, vb, vtx, NULL);
    }

    uint16_t *idx = (uint16_t*) cgMapDataBuffer(Global_CGFX.Context, Global_CGFX.GPUTransferQueue, ib, CG_INVALID_HANDLE, 0, sizeof(uint16_t) * 6, CG_MEMORY_ACCESS_WRITE, cgres);
    if (idx != NULL)
    {   // the quad is comprised of 2 triangles with counter-clockwise winding.
        idx[0] = 0; idx[1] = 3; idx[2] = 2;
        idx[3] = 0; idx[4] = 2; idx[5] = 1;
        cgUnmapDataBuffer(Global_CGFX.Context, Global_CGFX.GPUTransferQueue, ib, idx, NULL);
    }

    float  dst16[16];
    float  s_x = 1.0f / (800.0f  * 0.5f);
    float  s_y = 1.0f / (600.0f  * 0.5f);
    dst16[ 0]  = s_x ; dst16[ 1] = 0.0f; dst16[ 2] = 0.0f; dst16[ 3] = 0.0f;
    dst16[ 4]  = 0.0f; dst16[ 5] = -s_y; dst16[ 6] = 0.0f; dst16[ 7] = 0.0f;
    dst16[ 8]  = 0.0f; dst16[ 9] = 0.0f; dst16[10] = 1.0f; dst16[11] = 0.0f;
    dst16[12]  =-1.0f; dst16[13] = 1.0f; dst16[14] = 0.0f; dst16[15] = 1.0f;
    cgBeginCommandBuffer(Global_CGFX.Context, cb, 0);
    cgGraphicsTest01SetViewport(Global_CGFX.Context, cb, gp, 0, 0, 800, 600, CG_INVALID_HANDLE, CG_INVALID_HANDLE);
    cgGraphicsTest01SetProjection(Global_CGFX.Context, cb, gp, dst16, CG_INVALID_HANDLE, CG_INVALID_HANDLE);
    cgGraphicsTest01DrawTriangles(Global_CGFX.Context, cb, gp, vs, 0, 5, 0, 2, CG_INVALID_HANDLE, CG_INVALID_HANDLE);
    cgEndCommandBuffer(Global_CGFX.Context, cb);

    // query the monitor refresh rate and use that as our target frame rate.
    int monitor_refresh_hz =  60;
    HDC monitor_refresh_dc =  GetDC(main_window);
    int refresh_rate_hz    =  GetDeviceCaps(monitor_refresh_dc, VREFRESH);
    if (refresh_rate_hz >  1) monitor_refresh_hz =  refresh_rate_hz;
    float present_rate_hz  =  monitor_refresh_hz /  2.0f;
    float present_rate_sec =  1.0f / present_rate_hz;
    ReleaseDC(main_window, monitor_refresh_dc);

    // initialize timer snapshots used to throttle update rate.
    int64_t last_clock     =  ticktime();
    int64_t flip_clock     =  ticktime();

    int64_t frame_update_time = ticktime();
    size_t  frame_index = 0;

    // run the main window message pump and user interface loop.
    while (Global_IsRunning)
    {   // dispatch Windows messages while messages are waiting.
        // specify NULL as the HWND to retrieve all messages for
        // the current thread, not just messages for the window.
        MSG msg;
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            switch (msg.message)
            {
            case WM_QUIT:
                result = (int) msg.wParam;
                Global_IsRunning = false;
                break;
            default:
                TranslateMessage(&msg);
                DispatchMessage (&msg);
                break;
            }
        }

        // throttle the application update rate.
        int64_t update_ticks = elapsed_ticks(last_clock, ticktime());
        float   update_secs  = ticks_to_seconds(update_ticks);
        if (update_secs < present_rate_sec)
        {   // TODO(rlk): can we do this in all-integer arithmetic?
            float sleep_ms_f =(1000.0f * (present_rate_sec - update_secs));
            if   (sleep_ms_f > desired_granularity)
            {   // should be able to sleep accurately.
                Sleep((DWORD)  sleep_ms_f);
            }
            else
            {   // sleep time is less than scheduler granularity; busy wait.
                while (update_secs < present_rate_sec)
                {
                    update_ticks   = elapsed_ticks(last_clock, ticktime());
                    update_secs    = ticks_to_seconds(update_ticks);
                }
            }
        }
        // else, we missed our target rate, so run full-throttle.
        last_clock = ticktime();

        // now perform the actual presentation of the display buffer to the window.
        // this may take some non-negligible amount of time, as it involves processing
        // queued command buffers to construct the current frame.
        cgSetActiveDrawableEXT(Global_CGFX.Context, Global_CGFX.Drawable);
        cgExecuteCommandBuffer(Global_CGFX.Context, Global_CGFX.GPUGraphicsQueue, cb);
        cgPresentDrawableEXT(Global_CGFX.Drawable);

        // update timestamps to calculate the total presentation time.
        int64_t present_ticks = elapsed_ticks(flip_clock, ticktime());
        float   present_secs  = ticks_to_seconds(present_ticks);
        flip_clock = ticktime();
        result     = 1;
    }

    // clean up application resources, and exit.
    cgfx_teardown(&Global_CGFX);
    return result;
}
