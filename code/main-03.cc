#include <windows.h>
#include <strsafe.h>

#define local_persist static
#define global_variable static
#define internal static

global_variable bool Running;
global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable HBITMAP BitmapHandle;
global_variable HDC BitmapDeviceContext;

/* allocate a buffer for a bitmap based on the size provided */
/* Device Independant Bitmap to be displayed with GDI */
/* free the previous buffer first, allocate the new one after */ 
internal void
Win32ResizeDIBSection (int Width, int Height)
{
  if (BitmapHandle)
  {
    DeleteObject (BitmapHandle);
  }
  if (!BitmapDeviceContext)
  {
    BitmapDeviceContext = CreateCompatibleDC (0);
  }

  BitmapInfo.bmiHeader.biSize = sizeof (BitmapInfo.bmiHeader);
  BitmapInfo.bmiHeader.biWidth = Width;
  BitmapInfo.bmiHeader.biHeight = Height;
  BitmapInfo.bmiHeader.biPlanes = 1;
  BitmapInfo.bmiHeader.biBitCount = 32;
  BitmapInfo.bmiHeader.biCompression = BI_RGB;
  BitmapInfo.bmiHeader.biSizeImage = 0;
  BitmapInfo.bmiHeader.biXPelsPerMeter = 0;
  BitmapInfo.bmiHeader.biYPelsPerMeter = 0;
  BitmapInfo.bmiHeader.biClrUsed = 0; /* no color table */
  BitmapInfo.bmiHeader.biClrImportant = 0; /* no color table */

  BitmapHandle = CreateDIBSection (BitmapDeviceContext,
                                   &BitmapInfo,
                                   DIB_RGB_COLORS,
                                   &BitmapMemory,
                                   0, 0);
}

internal void
Win32UpdateWindow (HDC DeviceContext, int X, int Y, int Width, int Height)
{
  StretchDIBits (DeviceContext,
                 X, Y, Width, Height, 
                 X, Y, Width, Height,
                 BitmapMemory,
                 &BitmapInfo,
                 DIB_RGB_COLORS,
                 SRCCOPY);
}

internal
void ErrorExit (LPTSTR lpszFunction) 
{ 
  // Retrieve the system error message for the last-error code

  LPVOID lpMsgBuf;
  LPVOID lpDisplayBuf;
  DWORD dw = GetLastError(); 

  FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL,
                 dw,
                 MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
                 (LPTSTR)
                 &lpMsgBuf,
                 0,
                 NULL
                );

  // Display the error message and exit the process

  lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
                                    (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR)); 
  StringCchPrintf((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR),
                  TEXT("%s failed with error %d: %s"), 
                  lpszFunction,
                  dw,
                  lpMsgBuf); 
  MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

  LocalFree(lpMsgBuf);
  LocalFree(lpDisplayBuf);
  ExitProcess(dw); 
}

LRESULT CALLBACK
Win32MainWindowCallback (HWND Window, UINT Message,
                         WPARAM wParam, LPARAM lParam)
{
  LRESULT Result = 0;
  switch (Message)
  {
  case WM_SIZE:
    {
      /* Recreate the buffer because its size has changed */
      RECT ClientRect;
      GetClientRect (Window, &ClientRect);
      int Height = ClientRect.bottom - ClientRect.top;
      int Width = ClientRect.right - ClientRect.left;
      Win32ResizeDIBSection (Width, Height);
    } break;

  case WM_DESTROY:
    {
      Running = false;
    } break;

  case WM_CLOSE:
    {
      Running = false;
    } break;

  case WM_PAINT:
    {
      /* blit the buffer to the window */
      PAINTSTRUCT Paint = {};
      HDC DeviceContext = BeginPaint (Window, &Paint);
      int X = Paint.rcPaint.left;
      int Y = Paint.rcPaint.top;
      int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
      int Width = Paint.rcPaint.right - Paint.rcPaint.left;
      Win32UpdateWindow (DeviceContext, X, Y, Width, Height);
      //PatBlt (DeviceContext, X, Y, Width, Height, WHITENESS);
      EndPaint (Window, &Paint);
    } break;

  case WM_ACTIVATEAPP:
    {
      OutputDebugString ("WM_ACTIVATEAPP\n");
    } break;

  default:
    {
      Result = DefWindowProc (Window, Message, wParam, lParam);
    }
  }
  return Result;
}

int CALLBACK
WinMain (HINSTANCE Instance, HINSTANCE PrevInstance,
         LPSTR CommandLine, int ShowCode)
{
  WNDCLASS WindowClass = {};
  WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;                       // we won't have to get and release DC since we own it.
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  WindowClass.hInstance = Instance;
  WindowClass.lpszClassName = "HandmadeHeroWindowClass";

  if (RegisterClass (&WindowClass))
  {
    HWND WindowHandle = CreateWindowEx (0,                                      // DWORD     dwExStyle,
                                        WindowClass.lpszClassName,              // LPCTSTR   lpClassName,
                                        "Handmade Hero",                        // LPCTSTR   lpWindowName,
                                        WS_OVERLAPPEDWINDOW | WS_VISIBLE,       // DWORD     dwStyle,
                                        CW_USEDEFAULT,                          // int       x,
                                        CW_USEDEFAULT,                          // int       y,
                                        CW_USEDEFAULT,                          // int       nWidth,
                                        CW_USEDEFAULT,                          // int       nHeight,
                                        0,                                      // HWND      hWndParent,
                                        0,                                      // HMENU     hMenu,
                                        Instance,                               // HINSTANCE hInstance,
                                        0                                       // LPVOID    lpParam
                                       );
    if (WindowHandle)
    {
      Running = true;
      while (Running)
      {
        MSG Message;
        BOOL MessageResult = GetMessage (&Message, 0, 0, 0);
        if (MessageResult > 0)
        {
          TranslateMessage (&Message);
          DispatchMessage (&Message);
        }
        else
        {
          break;
        }
      }
    }
    else
    {
      ErrorExit (TEXT("CreateWindowEx"));
    }
  }
  else
  {
    ErrorExit (TEXT("RegisterClass"));
  }
  return 0;
}


