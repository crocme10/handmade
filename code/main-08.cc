#include <windows.h>
#include <strsafe.h>
#include <stdint.h>
#include <XInput.h>
#include <DSound.h>

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

internal void ErrorExit (LPTSTR lpszFunction) ;

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD UserIndex, XINPUT_STATE *State)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
  return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state * XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD UserIndex, XINPUT_VIBRATION *Vibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
  return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state * XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void
Win32LoadXInput (void)
{
  HMODULE XInputLibrary = LoadLibrary ("xinput1_4.dll");
  if (!XInputLibrary)
  {
    XInputLibrary = LoadLibrary ("xinput1_3.dll");
  }
  if (XInputLibrary)
  {
    XInputGetState = (x_input_get_state *)GetProcAddress (XInputLibrary, "XInputGetState");
    XInputSetState = (x_input_set_state *)GetProcAddress (XInputLibrary, "XInputSetState");
  }
}


#define DIRECT_SOUND_CREATE(name) HRESULT name(LPCGUID lpcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);
DIRECT_SOUND_CREATE(DirectSoundCreateStub)
{
  return 0;
}
global_variable direct_sound_create * DirectSoundCreate_ = DirectSoundCreateStub;
#define DirectSoundCreate DirectSoundCreate_
                          
global_variable LPDIRECTSOUNDBUFFER SecondaryBuffer;

internal void
Win32LoadDirectSound (HWND Window, int32_t SamplesPerSecond, int32_t BufferSize)
{
  /* load library */
  HMODULE DSoundLibrary = LoadLibrary ("dsound.dll");

  if (DSoundLibrary)
  {
    /* get a direct sound object */
    DirectSoundCreate = (direct_sound_create *)GetProcAddress (DSoundLibrary,"DirectSoundCreate");
    LPDIRECTSOUND DirectSound;
    if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate (0, &DirectSound, 0)))
    {
      if (SUCCEEDED(DirectSound->SetCooperativeLevel (Window, DSSCL_PRIORITY)))
      {
        LPDIRECTSOUNDBUFFER PrimaryBuffer;
        DSBUFFERDESC PrimaryBufferDescription = {};
        PrimaryBufferDescription.dwSize = sizeof(PrimaryBufferDescription);
        PrimaryBufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
        if (SUCCEEDED(DirectSound->CreateSoundBuffer (&PrimaryBufferDescription, &PrimaryBuffer, 0)))
        {

          WAVEFORMATEX WaveFormat = {};
          WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
          //WaveFormat.nChannels = 2;
          //WaveFormat.nSamplesPerSec = SamplesPerSecond;
          //WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
          //WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
          //WaveFormat.wBitsPerSample = 16;
          //WaveFormat.cbSize = 0;
          if (SUCCEEDED (PrimaryBuffer->SetFormat(&WaveFormat)))
          {
            DSBUFFERDESC SecondaryBufferDescription = {};
            SecondaryBufferDescription.dwSize = sizeof(SecondaryBufferDescription);
            SecondaryBufferDescription.lpwfxFormat = &WaveFormat;
            //SecondaryBufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
            if (SUCCEEDED(DirectSound->CreateSoundBuffer (&SecondaryBufferDescription, &SecondaryBuffer, 0)))
            {
              if (!SecondaryBuffer) ErrorExit (TEXT("Invalid Secondary Buffer"));
            }
            else
            {
              ErrorExit (TEXT("CreateSoundBuffer (Secondary)"));
            }
          } 
          else
          {
            ErrorExit (TEXT("SetWaveFormat"));
          }
        }
        else
        {
          ErrorExit (TEXT("CreateSoundBuffer (Primary)"));
        }
      }
      else
      {
        ErrorExit (TEXT("SetCooperativeLevel"));
      }
    } 
    else
    {
      ErrorExit (TEXT("DirectSoundCreate"));
    }
  } /* DSoundLibrary */
}

internal Win32WindowDimension
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

  case WM_SYSKEYDOWN:
  case WM_SYSKEYUP:
  case WM_KEYDOWN:
  case WM_KEYUP:
    {
      uint32 VKCode = wParam; /* What key was pressed */
      bool WasDown = ((lParam & (1 << 30)) != 0);
      bool IsDown = ((lParam & (1 << 31)) == 0);

      if (IsDown != WasDown) /* filter only change of key press */
      {
        if (VKCode == 'W')
        {
        }
        else if (VKCode == 'A')
        {
        }
        else if (VKCode == 'S')
        {
        }
        else if (VKCode == 'D')
        {
        }
        else if (VKCode == 'Q')
        {
        }
        else if (VKCode == 'E')
        {
        }
        else if (VKCode == VK_UP)
        {
        }
        else if (VKCode == VK_DOWN)
        {
        }
        else if (VKCode == VK_LEFT)
        {
        }
        else if (VKCode == VK_RIGHT)
        {
        }
        else if (VKCode == VK_ESCAPE)
        {
          OutputDebugString ("Escape: ");
          if (IsDown)
          {
            OutputDebugString ("IsDown ");
          }
          if (WasDown)
          {
            OutputDebugString ("WasDown");
          }
          OutputDebugString ("\n");
        }
        else if (VKCode == VK_SPACE)
        {
        }
      }
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
  Win32LoadXInput ();

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
      
      /* Sound test */
      int ToneHz = 256; 
      int Volume = 200;
      int SamplesPerSecond = 40000;
      int SquareWavePeriod = SamplesPerSecond / ToneHz;
      int HalfSquareWavePeriod = SquareWavePeriod / 2;
      int BytesPerSample = sizeof (int16_t) * 2;
      int BufferSize = SamplesPerSecond * sizeof(int16_t) * 2;
      uint32_t SampleIndex = 0; /* goes up for ever... */

      Win32LoadDirectSound (Window, SamplesPerSecond, BufferSize);

      SecondaryBuffer->Play (0, 0, DSBPLAY_LOOPING);

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

        /* Poll for input */
        for (DWORD ControllerIndex = 0;
             ControllerIndex < XUSER_MAX_COUNT;
             ++ControllerIndex)
        {
          XINPUT_STATE ControllerState;
          /* if the gamepad is plugged in ... */
          if (XInputGetState (ControllerIndex, &ControllerState) == ERROR_SUCCESS)
          {
            XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
            bool Up = (Pad->wButtons &XINPUT_GAMEPAD_DPAD_UP);
            bool Down = (Pad->wButtons &XINPUT_GAMEPAD_DPAD_DOWN);
            bool Left = (Pad->wButtons &XINPUT_GAMEPAD_DPAD_LEFT);
            bool Right = (Pad->wButtons &XINPUT_GAMEPAD_DPAD_RIGHT);
            bool Start = (Pad->wButtons &XINPUT_GAMEPAD_START);
            bool Back = (Pad->wButtons &XINPUT_GAMEPAD_BACK);
            bool AButton = (Pad->wButtons &XINPUT_GAMEPAD_A);
            bool BButton = (Pad->wButtons &XINPUT_GAMEPAD_B);
            bool XButton = (Pad->wButtons &XINPUT_GAMEPAD_X);
            bool YButton = (Pad->wButtons &XINPUT_GAMEPAD_Y);
          }
          else
          {
          }
        }

        RenderWeirdGradient (&BackBuffer, XOffset, YOffset);

        /* Sound */

        DWORD PlayCursor;
        DWORD WriteCursor;

        if (SUCCEEDED(SecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
        {
          /* We're locking at the sample index */
          DWORD ByteToLock = SampleIndex * BytesPerSample % BufferSize;

          /* We're writing from bytetolock to the playcursor... but it's a
           * circular buffer */
          DWORD BytesToWrite;

          if (ByteToLock > PlayCursor)
          {
            BytesToWrite = BufferSize - ByteToLock;
            BytesToWrite += PlayCursor;
          }
          else
          {
            BytesToWrite = PlayCursor - ByteToLock;
          }

          VOID *Region1;
          DWORD Region1Size;
          VOID *Region2;
          DWORD Region2Size;

          SecondaryBuffer->Lock (ByteToLock, BytesToWrite,
                                 &Region1, &Region1Size,
                                 &Region2, &Region2Size,
                                 0 /* Flags */
                                );
          int16_t *SampleOut = (int16_t *)Region1;
          DWORD Region1SampleCount = Region1Size / BytesPerSample;
          for (DWORD Index = 0; Index < Region1SampleCount; ++Index)
          {
            int16_t SampleValue = (SampleIndex / HalfSquareWavePeriod) % 2 ? Volume : -Volume;
            *SampleOut++ = SampleValue;
            *SampleOut++ = SampleValue;
            ++SampleIndex;
          }
          SampleOut = (int16_t *)Region2;
          DWORD Region2SampleCount = Region2Size / BytesPerSample;
          for (DWORD Index = 0; Index < Region2SampleCount; ++Index)
          {
            int16_t SampleValue = (SampleIndex / HalfSquareWavePeriod) % 2 ? Volume : -Volume;
            *SampleOut++ = SampleValue;
            *SampleOut++ = SampleValue;
            ++SampleIndex;
          }
          SecondaryBuffer->Unlock (Region1, Region1Size, Region2, Region2Size);
        }
        HDC DeviceContext = GetDC (Window);
        Win32WindowDimension Dimension = GetWindowDimension (Window);
        Win32DisplayBuffer (DeviceContext, Dimension.Width, Dimension.Height, &BackBuffer);
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
