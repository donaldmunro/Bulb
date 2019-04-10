#ifndef _WIN_HH_
#define _WIN_HH_

#include <vector>
#include <functional>

#define XLIB

#ifdef XLIB
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define IS_BAD_XWINDOW(x)    ( (x == BadAlloc) || (x == BadColor) || \
                               (x == BadCursor) || (x == BadMatch) ||  \
                               (x == BadPixmap) || (x == BadValue) || \
                               (x == BadWindow) )
#endif
#ifdef WIN32
#pragma comment(linker, "/subsystem:windows")
#define APP_NAME_STR_LEN 80
#include <windows.h>
#endif

struct Win
{
   unsigned width, height;
   std::function<void(unsigned lastWidth, unsigned lastHeight, unsigned newWidth, unsigned newHeight)>& resizedCallback;

   Win(unsigned w, unsigned h,
       std::function<void(unsigned, unsigned, unsigned, unsigned)>& resizeCallback);
   static std::vector<std::pair<unsigned, unsigned>> sizes();
   bool process_event();
   virtual ~Win();


#ifdef XLIB
   Window xWindow;
   Display* display;
   Atom wm_delete_window;
   void* native_window() { return (void *)xWindow; }
#endif
#ifdef WIN32
   HINSTANCE connection;         // hInstance - Windows Instance
   char name[APP_NAME_STR_LEN];  // Name to put on the window/icon
   HWND window;                  // hWnd - window handle
   POINT minsize;
   void* native_window() { return (void *)connection; }
#endif
};


#endif //_WIN_HH_
