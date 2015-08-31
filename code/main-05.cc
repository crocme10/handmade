#include <windows.h>
#include <strsafe.h>
#include <stdint.h>

#define local_persist static
#define global_variable static
#define internal static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;


typedef struct
{
  BITMAPINFO Info;
  void *Memory;
  int Width;
  int Height;
  int Pitch;
  int BytesPerPixel;
} Win32OffscreenBuffer;

typedef struct
{
  int Width;
  int Height;
} Win32WindowDimension;

Win32WindowDimension
GetWindowDimension (HWND Window)
{
  Win32WindowDimension Result;

  RECT WindowRect;
  GetWindowRect (Window, &WindowRect);
  Result.Width = WindowRect.right - WindowRect.left;
  Result.Height = WindowRect.bottom - WindowRect.top;

  return Result;
}

global_variable bool Running;
global_variable Win32OffscreenBuffer BackBuffer;

internal void
RenderWeirdGradient (Win32OffscreenBuffer *Buffer, int BlueOffset, int GreenOffset)
{
  /* since we want to do pointer arithmetics */
  uint8 *Row = (uint8 *)Buffer->Memory;

  for (int Y = 0; Y < Buffer->Height; ++Y)
  {
    uint32 *Pixel = (uint32 *)Row;
    for (int X = 0; X < Buffer->Width; ++X)
    {
      uint8 Blue = (X + BlueOffset);
      uint8 Green = (Y + GreenOffset); 
      /*  Memory BB GG RR XX */
      *Pixel++ = ((Green << 8) | Blue);
    }
    Row += Buffer->Pitch;
  }
}

internal void
Win32ResizeDIBSection (Win32OffscreenBuffer *Buffer, int Width, int Height)
{
  if (Buffer->Memory)
  {
    VirtualFree (Buffer->Memory, 0, MEM_RELEASE);
  }

  Buffer->Width = Width;
  Buffer->Height = Height;
  Buffer->BytesPerPixel = 4;

  Buffer->Info.bmiHeader.biSize = sizeof (Buffer->Info.bmiHeader);
  Buffer->Info.bmiHeader.biWidth = Buffer->Width;
  Buffer->Info.bmiHeader.biHeight = Buffer->Height;
  Buffer->Info.bmiHeader.biPlanes = 1;
  Buffer->Info.bmiHeader.biBitCount = 32;
  Buffer->Info.bmiHeader.biCompression = BI_RGB;

  int BitmapMemorySize = Buffer->BytesPerPixel * Width * Height;
  Buffer->Memory = VirtualAlloc (0, BitmapMemorySize, MEM_COMMIT,
                                 PAGE_READWRITE);

  Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;
}

internal void
Win32DisplayBuffer (HDC DeviceContext, int Width, int Height,
                    Win32OffscreenBuffer *Buffer)
{
  StretchDIBits (DeviceContext,
                 0, 0, Width, Height,
                 0, 0, Buffer->Width, Buffer->Height,
                 Buffer->Memory,
                 &Buffer->Info,
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

  FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER 
                 | FORMAT_MESSAGE_FROM_SYSTEM
                 | FORMAT_MESSAGE_IGNORE_INSERTS,
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
                                    (lstrlen((LPCTSTR)lpMsgBuf)
                                     + lstrlen((LPCTSTR)lpszFunction) + 40)
                                    * sizeof(TCHAR)); 
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
      int Width = Paint.rcPaint.right - Paint.rcPaint.left;
      int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
      int X = Paint.rcPaint.left;
      int Y = Paint.rcPaint.top;
      Win32WindowDimension Dimension = GetWindowDimension (Window);
      Win32DisplayBuffer (DeviceContext, Dimension.Width, Dimension.Height,
                          &BackBuffer);
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
  WindowClass.style = CS_HREDRAW | CS_VREDRAW;                                 // We need to repaint the whole window
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  WindowClass.hInstance = Instance;
  WindowClass.lpszClassName = "HandmadeHeroWindowClass";

  Win32ResizeDIBSection (&BackBuffer, 1200, 720);

  if (RegisterClass (&WindowClass))
  {
    HWND Window = CreateWindowEx (0,                                           // DWORD     dwExStyle,
                                  WindowClass.lpszClassName,                   // LPCTSTR   lpClassName,
                                  "Handmade Hero",                             // LPCTSTR   lpWindowName,
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE,            // DWORD     dwStyle,
                                  CW_USEDEFAULT,                               // int       x,
                                  CW_USEDEFAULT,                               // int       y,
                                  CW_USEDEFAULT,                               // int       nWidth,
                                  CW_USEDEFAULT,                               // int       nHeight,
                                  0,                                           // HWND      hWndParent,
                                  0,                                           // HMENU     hMenu,
                                  Instance,                                    // HINSTANCE hInstance,
                                  0);                                          // LPVOID    lpParam
                                 
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
        RenderWeirdGradient (&BackBuffer, XOffset, YOffset);

        HDC DeviceContext = GetDC (Window);
        Win32WindowDimension Dimension = GetWindowDimension (Window);
        Win32DisplayBuffer (DeviceContext, Dimension.Width, Dimension.Height,
                            &BackBuffer);
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
