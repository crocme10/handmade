#include <windows.h>
#include <strsafe.h>
#include <stdint.h>

#define local_persist static
#define global_variable static
#define internal static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;

global_variable bool Running;
global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int BytesPerPixel = 4;

internal void
RenderWeirdGradient (int XOffset, int YOffset)
{
  /* since we want to do pointer arithmetics */
  int Pitch = BitmapWidth * BytesPerPixel;
  uint8 *Row = (uint8 *)BitmapMemory;

  for (int Y = 0; Y < BitmapHeight; ++Y)
  {
    uint32 *Pixel = (uint32 *)Row;
    for (int X = 0; X < BitmapWidth; ++X)
    {
      uint8 Blue = (X + XOffset);
      uint8 Green = (Y + YOffset); 
      /*  Memory BB GG RR XX */
      *Pixel++ = ((Green << 8) | Blue);
    }
    Row += Pitch;
  }
}

/* allocate a buffer for a bitmap based on the size provided */
/* Device Independant Bitmap to be displayed with GDI */
/* free the previous buffer first, allocate the new one after */ 
internal void
Win32ResizeDIBSection (int Width, int Height)
{
  if (BitmapMemory)
  {
    VirtualFree (BitmapMemory, 0, MEM_RELEASE);
  }

  BitmapWidth = Width;
  BitmapHeight = Height;

  BitmapInfo.bmiHeader.biSize = sizeof (BitmapInfo.bmiHeader);
  BitmapInfo.bmiHeader.biWidth = BitmapWidth;
  BitmapInfo.bmiHeader.biHeight = BitmapHeight;
  BitmapInfo.bmiHeader.biPlanes = 1;
  BitmapInfo.bmiHeader.biBitCount = 32;
  BitmapInfo.bmiHeader.biCompression = BI_RGB;

  //int BytesPerPixel = 4;
  int BitmapMemorySize = BytesPerPixel * Width * Height;
  BitmapMemory = VirtualAlloc (0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

  //RenderWeirdGradient (0, 0);
}

internal void
Win32UpdateWindow (HDC DeviceContext, RECT *WindowRect, int X, int Y, int Width, int Height)
{
  int WindowHeight = WindowRect->bottom - WindowRect->top;
  int WindowWidth = WindowRect->right - WindowRect->left;

  StretchDIBits (DeviceContext,
                 0, 0, BitmapWidth, BitmapHeight,
                 0, 0, WindowWidth, WindowHeight,
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
      RECT WindowRect;
      GetWindowRect (Window, &WindowRect);
      int Width = WindowRect.right - WindowRect.left;
      int Height = WindowRect.bottom - WindowRect.top;
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
      RECT WindowRect;
      GetWindowRect (Window, &WindowRect);
      int Width = WindowRect.right - WindowRect.left;
      int Height = WindowRect.bottom - WindowRect.top;
      int X = Paint.rcPaint.left;
      int Y = Paint.rcPaint.top;
      Win32UpdateWindow (DeviceContext, &WindowRect, X, Y, Width, Height);
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
    HWND Window = CreateWindowEx (0,                                      // DWORD     dwExStyle,
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
                                  0);                                     // LPVOID    lpParam
                                 
    if (Window)
    {
      int XOffset = 0;
      int YOffset = 0;
      Running = true;
      while (Running)
      {
        MSG Message;
        while (PeekMessage (&Message, 0, 0, 0, PM_REMOVE))
        {
          if (Message.message == WM_QUIT)
          {
            Running = false;
          }
          TranslateMessage (&Message);
          DispatchMessage (&Message);
        }
        RenderWeirdGradient (XOffset, YOffset);

        HDC DeviceContext = GetDC (Window);
        RECT WindowRect;
        GetWindowRect (Window, &WindowRect);
        int Width = WindowRect.right - WindowRect.left;
        int Height = WindowRect.bottom - WindowRect.top;
        Win32UpdateWindow (DeviceContext, &WindowRect, 0, 0, Width, Height);
        ReleaseDC (Window, DeviceContext);

        ++XOffset;
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


