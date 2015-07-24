/*/////////////////////////////////////////////////////////////////////////////
/// @summary Implements the Windows platform extensions for the CGFX API.
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

/*////////////////
//   Includes   //
////////////////*/
#include "cgfx.h"
#include "cgfx_ext_win.h"
#include "cgfx_w32_private.h"

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
#error   Need to define __ImageBase for your compiler in cgfx_ext_win.cc!
#endif

/*///////////////////////
//   Local Functions   //
///////////////////////*/

/*////////////////////////
//   Public Functions   //
////////////////////////*/
/// @summary Convert a cg_result_ext_e to a string description.
/// @param result One of cg_result_ext_e.
/// @return A pointer to a NULL-terminated ASCII string description. Do not attempt to modify or free the returned string.
library_function char const*
cgResultStringEXT
(
    int result
)
{
    switch (result)
    {
    case CG_BAD_WINDOW_EXT      :  return "The window is invalid or cannot be queried (CG_BAD_WINDOW_EXT)";
    case CG_BAD_DRAWABLE_EXT    :  return "The device context is invalid or is not associated with a window (CG_BAD_DRAWABLE_EXT)";
    case CG_WINDOW_RECREATED_EXT:  return "The window has been successfully recreated (CG_WINDOW_RECREATED_EXT)";
    default:                       break;
    }
    return "Unknown Result (EXT)";
}

/// @summary Locate the display containing the majority of the area of the specified window.
/// @param context A context returned by cgEnumerateDevices.
/// @param window The handle of the window.
/// @return The handle of the display containing the specified window, or CG_INVALID_HANDLE.
library_function cg_handle_t
cgDisplayForWindowEXT
(
    uintptr_t context,
    HWND      window
)
{
    CG_CONTEXT *ctx      = (CG_CONTEXT*) context;
    CG_DISPLAY *display  = NULL;
    size_t      max_area = 0;
    RECT        wnd_rect;

    // retrieve the current bounds of the window.
    GetWindowRect(window, &wnd_rect);

    // check each display to determine which contains the majority of the window area.
    for (size_t i = 0, n = ctx->DisplayTable.ObjectCount; i < n; ++i)
    {   // convert the display rectangle from {x,y,w,h} to {l,t,r,b}.
        RECT int_rect;
        RECT mon_rect = 
        {
            (LONG) ctx->DisplayTable.Objects[i].DisplayX, 
            (LONG) ctx->DisplayTable.Objects[i].DisplayY, 
            (LONG)(ctx->DisplayTable.Objects[i].DisplayX + ctx->DisplayTable.Objects[i].DisplayWidth), 
            (LONG)(ctx->DisplayTable.Objects[i].DisplayY + ctx->DisplayTable.Objects[i].DisplayHeight)
        };
        if (IntersectRect(&int_rect, &wnd_rect, &mon_rect))
        {   // the window client area intersects this display. what's the area of intersection?
            size_t int_w  = size_t(int_rect.right  - int_rect.left);
            size_t int_h  = size_t(int_rect.bottom - int_rect.top );
            size_t int_a  = int_w * int_h;
            if (int_a > max_area)
            {   // more of this window is on display 'i' than the previous best match.
                display   =&ctx->DisplayTable.Objects[i];
                max_area  = int_a;
            }
        }
    }
    if (display != NULL) return cgMakeHandle(display->ObjectId, CG_OBJECT_DISPLAY, CG_DISPLAY_TABLE_ID);
    else return CG_INVALID_HANDLE;
}

/// @summary Create a new window on the specified display.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param display_handle The display where the window should be created.
/// @param class_name The name of the window class. You must register the window class. See CreateWindowEx, lpClassName argument.
/// @param title The text to set as the window title. See CreateWindowEx, lpWindowName argument.
/// @param style The window style. See CreateWindowEx, dwStyle argument.
/// @param style_ex The extended window style. See CreateWindowEx, dwExStyle argument.
/// @param x The x-coordinate of the upper left corner of the window, in global display coordinates, or CW_USEDEFAULT which maps to the x-coordinate of the upper-left corner of the display.
/// @param y The y-coordinate of the upper left corner of the window, in global display coordinates, or CW_USEDEFAULT which maps to the y-coordinate of the upper-left corner of the display.
/// @param w The window width, in pixels, or CW_USEDEFAULT which maps to the entire width of the display.
/// @param h The window height, in pixels, or CW_USEDEFAULT which maps to the entire height of the display.
/// @param parent The parent HWND, or NULL. See CreateWindowEx, hWndParent argument.
/// @param menu The menu to be used with the window, or NULL. See CreateWindowEx, hMenu argument.
/// @param instance The module instance associated with the window, or NULL. See CreateWindowEx, hInstance argument.
/// @param user_data Passed to the window through the CREATESTRUCT, or NULL. See CreateWindowEx, lpParam argument.
/// @param window_rect On return, stores the window bounds in global display coordinates. May be NULL.
/// @return The handle of the new window, or NULL.
library_function HWND
cgMakeWindowOnDisplayEXT
(
    uintptr_t    context,
    cg_handle_t  display_handle,
    TCHAR const *class_name,
    TCHAR const *title,
    DWORD        style,
    DWORD        style_ex,
    int          x,
    int          y,
    int          w,
    int          h,
    HWND         parent,
    HMENU        menu,
    HINSTANCE    instance,
    LPVOID       user_data,
    RECT        *window_rect
)
{   
    CG_CONTEXT *ctx     = (CG_CONTEXT*) context;
    CG_DISPLAY *display =  cgObjectTableGet(&ctx->DisplayTable, display_handle);
    if (display == NULL)
    {   // an invalid display handle was specified.
        if (window_rect != NULL) *window_rect = {0,0,0,0};
        return NULL;
    }

    // default to a full screen window if no size is specified.
    RECT display_rect = 
    {
        (LONG) display->DisplayX, 
        (LONG) display->DisplayY, 
        (LONG)(display->DisplayX + display->DisplayWidth), 
        (LONG)(display->DisplayY + display->DisplayHeight)
    };

    // CW_USEDEFAULT x, y => (0, 0)
    // CW_USEDEFAULT w, h => (width, height)
    if (x == CW_USEDEFAULT) x = (int) display->DisplayX;
    if (y == CW_USEDEFAULT) y = (int) display->DisplayY;
    if (w == CW_USEDEFAULT) w = (int) display->DisplayWidth;
    if (h == CW_USEDEFAULT) h = (int) display->DisplayHeight;

    // clip the window bounds to the display bounds.
    if (x + w <= display_rect.left || x >= display_rect.right)   x = display_rect.left;
    if (x + w >  display_rect.right ) w  = display_rect.right  - x;
    if (y + h <= display_rect.top  || y >= display_rect.bottom)  y = display_rect.bottom;
    if (y + h >  display_rect.bottom) h  = display_rect.bottom - y;

    // retrieve the pixel format description for the target display.
    PIXELFORMATDESCRIPTOR pfd_dst;
    HDC                   hdc_dst  = display->DisplayDC;
    int                   fmt_dst  = GetPixelFormat(display->DisplayDC);
    DescribePixelFormat(hdc_dst, fmt_dst, sizeof(PIXELFORMATDESCRIPTOR), &pfd_dst);

    // create the new window on the display.
    HWND src_wnd  = CreateWindowEx(style_ex, class_name, title, style, x, y, w, h, parent, menu, instance, user_data);
    if  (src_wnd == NULL)
    {   // failed to create the window, no point in continuing.
        if (window_rect != NULL) *window_rect = {0,0,0,0};
        return NULL;
    }

    // assign the target display pixel format to the window.
    HDC  hdc_src = GetDC(src_wnd);
    if (!SetPixelFormat (hdc_src, fmt_dst, &pfd_dst))
    {   // cannot set the pixel format; window is unusable.
        if (window_rect != NULL) *window_rect = {0,0,0,0};
        ReleaseDC(src_wnd, hdc_src);
        DestroyWindow(src_wnd);
        return NULL;
    }

    // everything is good; cleanup and set output parameters.
    ReleaseDC(src_wnd, hdc_src);
    if (window_rect != NULL) GetWindowRect(src_wnd, window_rect);
    return src_wnd;
}

/// @summary Attach a window to a display, ensuring that the correct pixel format is set. If necessary, the window is destroyed and re-created.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param display_handle The display to which the window should be attached.
/// @param window The window being attached to the target display.
/// @param result On return, set to CG_SUCCESS, CG_WINDOW_RECREATED_EXT or another result code.
/// @return The handle of the window attached to the display, which may or may not be the same as @a window.
library_function HWND
cgAttachWindowToDisplayEXT
(
    uintptr_t   context,
    cg_handle_t display_handle,
    HWND        window,
    int        &result
)
{
    CG_CONTEXT *ctx     = (CG_CONTEXT*) context;
    CG_DISPLAY *display =  cgObjectTableGet(&ctx->DisplayTable, display_handle);
    if (display == NULL)
    {   // an invalid display handle was specified.
        result = CG_INVALID_VALUE;
        return window;
    }

    PIXELFORMATDESCRIPTOR pfd_dst;
    HDC                   hdc_src  = GetDC(window);
    HDC                   hdc_dst  = display->DisplayDC;
    int                   fmt_dst  = GetPixelFormat(display->DisplayDC);

    DescribePixelFormat(hdc_dst, fmt_dst, sizeof(PIXELFORMATDESCRIPTOR), &pfd_dst);
    if (!SetPixelFormat(hdc_src, fmt_dst, &pfd_dst))
    {   // the window may have already had its pixel format set.
        // we need to destroy and then re-create the window.
        // when re-creating the window, use WS_CLIPCHILDREN | WS_CLIPSIBLINGS.
        DWORD      style_r  =  WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
        HINSTANCE  instance = (HINSTANCE) GetWindowLongPtr(window, GWLP_HINSTANCE);
        DWORD      style    = (DWORD    ) GetWindowLongPtr(window, GWL_STYLE) | style_r;
        DWORD      style_ex = (DWORD    ) GetWindowLongPtr(window, GWL_EXSTYLE);
        HWND       parent   = (HWND     ) GetWindowLongPtr(window, GWLP_HWNDPARENT);
        LONG_PTR   userdata = (LONG_PTR ) GetWindowLongPtr(window, GWLP_USERDATA);
        WNDPROC    wndproc  = (WNDPROC  ) GetWindowLongPtr(window, GWLP_WNDPROC);
        HMENU      wndmenu  = (HMENU    ) GetMenu(window);
        size_t     titlelen = (size_t   ) GetWindowTextLength(window);
        TCHAR     *titlebuf = (TCHAR   *) cgAllocateHostMemory(&ctx->HostAllocator, (titlelen+1) * sizeof(TCHAR), 0, CG_ALLOCATION_TYPE_TEMP);
        BOOL       visible  = IsWindowVisible(window);
        RECT       wndrect; GetWindowRect(window, &wndrect);

        if (titlebuf == NULL)
        {   // unable to allocate the required memory.
            ReleaseDC(window, hdc_src);
            result = CG_OUT_OF_MEMORY;
            return window;
        }
        GetWindowText(window, titlebuf, (int)(titlelen+1));

        TCHAR class_name[256+1]; // see MSDN for WNDCLASSEX lpszClassName field.
        if (!GetClassName(window, class_name, 256+1))
        {   // unable to retrieve the window class name.
            cgFreeHostMemory(&ctx->HostAllocator, titlebuf, (titlelen+1) * sizeof(TCHAR), 0, CG_ALLOCATION_TYPE_TEMP);
            ReleaseDC(window, hdc_src);
            result = CG_BAD_WINDOW_EXT;
            return window;
        }

        int  x = (int) wndrect.left;
        int  y = (int) wndrect.top;
        int  w = (int)(wndrect.right  - wndrect.left);
        int  h = (int)(wndrect.bottom - wndrect.top );
        HWND new_window  = CreateWindowEx(style_ex, class_name, titlebuf, style, x, y, w, h, parent, wndmenu, instance, (LPVOID) userdata);
        if  (new_window == NULL)
        {   // unable to re-create the window.
            cgFreeHostMemory(&ctx->HostAllocator, titlebuf, (titlelen+1) * sizeof(TCHAR), 0, CG_ALLOCATION_TYPE_TEMP);
            ReleaseDC(window, hdc_src);
            result = CG_BAD_WINDOW_EXT;
            return window;
        }
        cgFreeHostMemory(&ctx->HostAllocator, titlebuf, (titlelen+1) * sizeof(TCHAR), 0, CG_ALLOCATION_TYPE_TEMP);
        SetWindowLongPtr(new_window, GWLP_USERDATA, (LONG_PTR) userdata);
        SetWindowLongPtr(new_window, GWLP_WNDPROC , (LONG_PTR) wndproc);
        ReleaseDC(window, hdc_src);
        hdc_src = GetDC(new_window);

        // finally, we can try and set the pixel format again.
        if (!SetPixelFormat(hdc_src, fmt_dst, &pfd_dst))
        {   // still unable to set the pixel format - we're out of options.
            ReleaseDC(new_window, hdc_src);
            DestroyWindow(new_window);
            result = CG_BAD_PIXELFORMAT;
            return window;
        }

        // show the new window, and activate it:
        if (visible) ShowWindow(new_window, SW_SHOW);
        else ShowWindow(new_window, SW_HIDE);

        // everything was successful, so destroy the old window:
        ReleaseDC(new_window, hdc_src);
        DestroyWindow(window);
        result = CG_WINDOW_RECREATED_EXT;
        return new_window;
    }
    else
    {   // the pixel format was set successfully without recreating the window.
        ReleaseDC(window, hdc_src);
        result = CG_SUCCESS;
        return window;
    }
}

/// @summary Set the target drawable for a display. The HDC must come from an HWND returned by cgMakeWindowOnDisplayEXT or cgAttachWindowToDisplayEXT.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param display_handle The display to which the window is attached.
/// @param drawable The device context of the window representing the drawable, or NULL to use the primary display window.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_NO_GLCONTEXT or CG_BAD_GLCONTEXT.
library_function int
cgSetDisplayDrawableEXT
(
    uintptr_t   context,
    cg_handle_t display_handle,
    HDC         drawable
)
{
    CG_CONTEXT *ctx     =(CG_CONTEXT*) context;
    if (display_handle == CG_INVALID_HANDLE && drawable == NULL)
    {   // deactivate the rendering context.
        wglMakeCurrent(NULL, NULL);
        return CG_SUCCESS;
    }

    CG_DISPLAY *display = cgObjectTableGet(&ctx->DisplayTable, display_handle);
    if (display == NULL)
    {   // the display handle is invalid.
        return CG_INVALID_VALUE;
    }
    if (display->DisplayRC == NULL)
    {   // this display has no rendering context.
        return CG_NO_GLCONTEXT;
    }
    if (drawable != NULL)
    {   // use the drawable specified by the caller.
        wglMakeCurrent(drawable, display->DisplayRC);
    }
    else
    {   // use the drawable of the display.
        wglMakeCurrent(display->DisplayDC, display->DisplayRC);
    }
    return CG_SUCCESS;
}

/// @summary Set the target drawable. The display is determined automatically. The HDC must come from an HWND returned by cgMakeWindowOnDisplayEXT or cgAttachWindowToDisplayEXT.
/// @param context A CGFX context returned by cgEnumerateDevices.
/// @param drawable The device context of the window representing the drawable, or NULL to disable display output.
/// @return CG_SUCCESS, CG_INVALID_VALUE, CG_NO_GLCONTEXT or CG_BAD_GLCONTEXT.
library_function int
cgSetActiveDrawableEXT
(
    uintptr_t context,
    HDC       drawable
)
{
    if (drawable == NULL)
    {   // deactivate the current rendering context.
        wglMakeCurrent(NULL, NULL);
        return CG_SUCCESS;
    }
    // locate the display that contains the window.
    HWND window  = WindowFromDC(drawable);
    if  (window == NULL)
    {   // no window is associated with this drawable.
        return CG_BAD_DRAWABLE_EXT;
    }
    cg_handle_t display_handle = cgDisplayForWindowEXT(context, window);
    if (display_handle != CG_INVALID_HANDLE)
    {   // found the display, so make the drawable the active target.
        return cgSetDisplayDrawableEXT(context, display_handle, drawable);
    }
    else
    {   // this window isn't on any known display.
        return CG_BAD_DRAWABLE_EXT;
    }
}

/// @summary Directly queues presentation of all pending display commands targeting a drawable.
/// @param drawable The device context of the window representing the drawable.
/// @return CG_SUCCESS.
library_function int
cgPresentDrawableEXT
(
    HDC          drawable
)
{
    SwapBuffers(drawable);
    return CG_SUCCESS;
}
