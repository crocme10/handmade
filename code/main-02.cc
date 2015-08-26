#include <windows.h>
#include <strsafe.h>

void ErrorExit(LPTSTR lpszFunction) 
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
MainWindowCallback (HWND Window, UINT Message,
                    WPARAM wParam, LPARAM lParam)
{
  LRESULT Result = 0;
  switch (Message)
  {
  case WM_SIZE:
    {
      OutputDebugString ("WM_SIZE\n");
    } break;

  case WM_DESTROY:
    {
      OutputDebugString ("WM_DESTROY\n");
    } break;

  case WM_CLOSE:
    {
      OutputDebugString ("WM_CLOSE\n");
    } break;

  case WM_PAINT:
    {
      PAINTSTRUCT Paint = {};
      HDC DeviceContext = BeginPaint (Window, &Paint);
      int X = Paint.rcPaint.left;
      int Y = Paint.rcPaint.top;
      int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
      int Width = Paint.rcPaint.right - Paint.rcPaint.left;
      PatBlt (DeviceContext, X, Y, Width, Height, WHITENESS);
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
  WindowClass.lpfnWndProc = MainWindowCallback;
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
      MSG Message;
      for (;;)
      {
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


