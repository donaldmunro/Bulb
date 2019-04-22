#include <iostream>

#include "Win.hh"

#ifdef XLIB
#define MAX_SCREEN_WIDTH 1920
#define MAX_SCREEN_HEIGHT 1080

Win::Win(unsigned w, unsigned h,
         std::function<void(unsigned lastWidth, unsigned lastHeight, unsigned newWidth, unsigned newHeight)>& resizeCallback)
         : width(w), height(h), resizedCallback(resizeCallback)
//----------------------------------------------------
{
   XInitThreads();
   display = XOpenDisplay(nullptr);
   if (display == nullptr)
   {
      std::cerr << "Error connecting to X (XOpenDisplay returned null)\n";
      std::exit(1);
   }
//      long visualMask = VisualScreenMask | VisualDepthMask | VisualClassMask |
//                        VisualRedMaskMask | VisualGreenMaskMask | VisualBlueMaskMask;
   long visualMask = VisualScreenMask | VisualDepthMask | VisualClassMask;
   int numberOfVisuals;
   XVisualInfo visualInfoTemplate={};
   visualInfoTemplate.screen = DefaultScreen(display);
   visualInfoTemplate.depth = 32;
   visualInfoTemplate.c_class = TrueColor;
//      visualInfoTemplate.blue_mask = 0xFF0000;
//      visualInfoTemplate.green_mask = 0xFF00;
//      visualInfoTemplate.red_mask = 0xFF;
   XVisualInfo *visualList = XGetVisualInfo(display, visualMask, &visualInfoTemplate, &numberOfVisuals);
   if ( (visualList == nullptr) || (numberOfVisuals == 0) )
   {
      std::cerr << "Error finding RGB X visual \n";
      std::exit(1);
   }
   int rgbaIndex = -1, bgraIndex = -1;
   for (int i=0; i<numberOfVisuals; i++)
   {
      XVisualInfo vi = visualList[i];
      if ( (bgraIndex < 0) && (vi.red_mask == 0xFF) && (vi.green_mask == 0xFF00) && (vi.blue_mask == 0xFF0000) )
         bgraIndex = i;
      if ( (rgbaIndex < 0) && (vi.red_mask == 0xFF0000) && (vi.green_mask == 0xFF00) && (vi.blue_mask == 0xFF) )
         rgbaIndex = i;
      std::cout << i << ": " << vi.red_mask << " " << vi.green_mask << " " << vi.blue_mask << " " << vi.depth << std::endl;
   }
   int index = (bgraIndex >= 0) ? bgraIndex : rgbaIndex;
   if (index < 0)
   {
      std::cerr << "Error finding RGB X visual \n";
      std::exit(1);
   }
   Colormap colormap = XCreateColormap(display, RootWindow(display, visualList[index].screen),
                                       visualList[index].visual, AllocNone);
   XSetWindowAttributes windowAttributes={};
   windowAttributes.colormap = colormap;
   windowAttributes.background_pixel = 0xFFFFFFFF;
   windowAttributes.border_pixel = 0;
   windowAttributes.event_mask = KeyPressMask | KeyReleaseMask | StructureNotifyMask | ExposureMask;
   xWindow = XCreateWindow(display, RootWindow(display, visualList->screen), 0, 0,
                           width, height, 0, visualList->depth, InputOutput, visualList->visual,
                           CWBackPixel | CWBorderPixel | CWEventMask | CWColormap, &windowAttributes);
   if (IS_BAD_XWINDOW(xWindow))
   {
      std::cerr << "Error creating X Window (XCreateWindow returned " << xWindow << ")\n";
      std::exit(1);
   }
   XMapWindow(display, xWindow);
   XFlush(display);
   wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
}

std::vector<std::pair<unsigned, unsigned>> Win::sizes()
//-----------------------------------------------------
{

   std::vector<std::pair<unsigned int, unsigned int>> vs;
   Display *display = XOpenDisplay(nullptr);
   if ( (display == nullptr) || (ScreenCount(display) < 1) )
   {
      if (display != nullptr) XCloseDisplay(display);
      return vs;
   }
   // TODO: Forced size as X returns sum of displays for width if more than one display connected (how to use libxrandr ?)
   vs.emplace_back(MAX_SCREEN_WIDTH, MAX_SCREEN_HEIGHT);

   for (int screenno=0; screenno<ScreenCount(display); screenno++)
   {
      Screen* screen = ScreenOfDisplay(display, screenno);
      if (screen != nullptr)
         vs.emplace_back(screen->width, screen->height);
   }
   XCloseDisplay(display);
   return vs;
}

bool Win::process_event()
//----------------------
{
   XEvent event;
   while (XPending(display) > 0)
   {
      XNextEvent(display, &event);
      switch(event.type)
      {
         case ClientMessage:
            if ((Atom)event.xclient.data.l[0] == wm_delete_window)
               return false;
            break;
         case KeyPress:
            switch (event.xkey.keycode)
            {
               case 0x9: // Escape
                  return false;
            }
            break;
         case ConfigureNotify:
            if ( (width != event.xconfigure.width) || (height != event.xconfigure.height) )
            {
               width = event.xconfigure.width;
               height = event.xconfigure.height;
            }
            if (resizedCallback)
               resizedCallback(event.xconfigure.width, event.xconfigure.height, width, height);
            break;
      }
   }
   return true;
}

Win::~Win()
{
   XDestroyWindow(display, xWindow);
   XCloseDisplay(display);
}
#endif

#ifdef WIN32
Win::Win(unsigned w, unsigned h)
{
   WNDCLASSEX win_class;
   win_class.cbSize = sizeof(WNDCLASSEX);
   win_class.style = CS_HREDRAW | CS_VREDRAW;
   win_class.lpfnWndProc = WndProc;
   win_class.cbClsExtra = 0;
   win_class.cbWndExtra = 0;
   win_class.hInstance = connection;
   win_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
   win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
   win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
   win_class.lpszMenuName = NULL;
   win_class.lpszClassName = name;
   win_class.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
   if (!RegisterClassEx(&win_class))
   {
     std::cerr << "Unexpected error trying to start the application!\n";
     exit(1);
   }
   RECT wr = {0, 0, width, height};
   AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
   window = CreateWindowEx(0, name, name, WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_SYSMENU, 100, 100,
                               wr.right - wr.left, wr.bottom - wr.top, NULL, NULL, connection, NULL);
   if (!window)
   {
      std::cerr << "Cannot create a window in which to draw!\n";
      exit(1);
   }
   minsize.x = GetSystemMetrics(SM_CXMINTRACK);
   minsize.y = GetSystemMetrics(SM_CYMINTRACK)+1;
}
#endif

