/*/////////////////////////////////////////////////////////////////////////////
/// @summary Define the public constants, data types and functions exposed by 
/// the unified Compute and Graphics library extensions for Microsoft Windows.
///////////////////////////////////////////////////////////////////////////80*/

#ifndef LIB_CGFX_EXT_WIN_H
#define LIB_CGFX_EXT_WIN_H

/*////////////////
//   Includes   //
////////////////*/
#include <Windows.h>
#include <windowsx.h>
#include <tchar.h>

/*////////////////////
//   Preprocessor   //
////////////////////*/

/*////////////////////////////
//   Forward Declarations   //
////////////////////////////*/

/*/////////////////
//   Constants   //
/////////////////*/

/*////////////////////////////
//  Function Pointer Types  //
////////////////////////////*/

/*//////////////////
//   Data Types   //
//////////////////*/
/// @summary Define the recognized function return codes (extended).
enum cg_result_ext_e : int
{
    CG_SOME_FAILURE_EXT          = CG_RESULT_FAILURE_EXT    -1, /// 
    CG_WINDOW_RECREATED_EXT      = CG_RESULT_NON_FAILURE_EXT+0, /// The window was destroyed and re-created successfully.
};

/*/////////////////
//   Functions   //
/////////////////*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

char const*
cgResultStringEXT                /// Convert a result code into a string.
(
    int          result          /// One of cg_result_e.
);

cg_handle_t
cgDisplayForWindowEXT            /// Locate the display containing the majority of the area of the specified window.
(
    uintptr_t    context,        /// A CGFX context returned by cgEnumerateDevices.
    HWND         window          /// The handle of the window.
);

HWND
cgMakeWindowOnDisplayEXT         /// Create a new window on the specified display.
(
    uintptr_t    context,        /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t  display,        /// The display where the window should be created.
    TCHAR const *class_name,     /// The name of the window class. You must register the window class. See CreateWindowEx, lpClassName argument.
    TCHAR const *title,          /// The text to set as the window title. See CreateWindowEx, lpWindowName argument.
    DWORD        style,          /// The window style. See CreateWindowEx, dwStyle argument.
    DWORD        style_ex,       /// The extended window style. See CreateWindowEx, dwExStyle argument.
    int          x,              /// The x-coordinate of the upper left corner of the window, in global display coordinates, or CW_USEDEFAULT which maps to the x-coordinate of the upper-left corner of the display.
    int          y,              /// The y-coordinate of the upper left corner of the window, in global display coordinates, or CW_USEDEFAULT which maps to the y-coordinate of the upper-left corner of the display.
    int          w,              /// The window width, in pixels, or CW_USEDEFAULT which maps to the entire width of the display.
    int          h,              /// The window height, in pixels, or CW_USEDEFAULT which maps to the entire height of the display.
    HWND         parent,         /// The parent HWND, or NULL. See CreateWindowEx, hWndParent argument.
    HMENU        menu,           /// The menu to be used with the window, or NULL. See CreateWindowEx, hMenu argument.
    HINSTANCE    instance,       /// The module instance associated with the window, or NULL. See CreateWindowEx, hInstance argument.
    LPVOID       user_data,      /// Passed to the window through the CREATESTRUCT, or NULL. See CreateWindowEx, lpParam argument.
    RECT        *window_rect     /// On return, stores the window bounds in global display coordinates. May be NULL.
);

HWND
cgAttachWindowToDisplayEXT       /// Attach a window to a display, ensuring that the correct pixel format is set. If necessary, the window is destroyed and re-created.
(
    uintptr_t    context,        /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t  display,        /// The display to which the window should be attached.
    HWND         window,         /// The window being attached to the target display.
    int         &result          /// On return, set to CG_SUCCESS, CG_WINDOW_RECREATED_EXT or another result code.
);

int
cgSetDisplayDrawableEXT          /// Set the target drawable for a display. The HDC must come from an HWND returned by cgMakeWindowOnDisplayEXT or cgAttachWindowToDisplayEXT.
(
    uintptr_t    context,        /// A CGFX context returned by cgEnumerateDevices.
    cg_handle_t  display,        /// The display to which the window is attached.
    HDC          drawable        /// The device context of the window representing the drawable, or NULL to use the primary display window.
);

int
cgSetActiveDrawableEXT           /// Set the target drawable. The display is determined automatically. The HDC must come from an HWND returned by cgMakeWindowOnDisplayEXT or cgAttachWindowToDisplayEXT.
(
    uintptr_t    context,        /// A CGFX context returned by cgEnumerateDevices.
    HDC          drawable        /// The device context of the window representing the drawable, or NULL to disable display output.
);

int
cgPresentDrawableEXT             /// Queue presentation of a drawable to the screen directly.
(
    HDC          drawable        /// The device context of the window representing the drawable.
);

#ifdef __cplusplus
};     /* extern "C"  */
#endif /* __cplusplus */

#undef  CGFX_EXT_WINDOWS_DEFINED
#define CGFX_EXT_WINDOWS_DEFINED
#endif /* !defined(LIB_CGFX_EXT_WIN_H) */
