// TODO(jp): Lower latency audio
// TODO(jp): Fullscreen and cursor hiding
// TODO(jp): HOT loading???
// TODO(jp): File writing for max points file.

#define internal static 
#define local_persist static 
#define global_variable static

#include <stdint.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

#define Minimum(A, B) (((A) < (B)) ? (A) : (B))
#define Maximum(A, B) (((A) > (B)) ? (A) : (B))

#define Assert(Expression) if(!(Expression)) { *(int *)0 = 0; }
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define Kilobytes(Value) (Value*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#include "csteroids.cpp"
#include <stdio.h>
#include <windows.h>
#include <dsound.h>

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

struct win32_sound_output
{
    int SamplesPerSecond;
    uint32 RunningSampleIndex;
    int BytesPerSample;
    int SecondaryBufferSize;
    int LatencySampleCount;
};

struct win32_bitmap
{
    int Width;
    int Height;
    int BytesPerPixel;
    int Pitch;
    int MemorySize;
    void *Memory;
            
    BITMAPINFO Info;
};

global_variable bool32 GlobalRunning;
global_variable win32_bitmap GlobalWin32Bitmap;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

internal read_entire_file_result
PlatformReadEntireFile(char *FileName, memory_arena *Arena)
{
    read_entire_file_result Result = {};
    
    HANDLE FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if(GetFileSizeEx(FileHandle, &FileSize))
        {
            Assert(FileSize.QuadPart <= 0xFFFFFFFF);
            uint32 FileSize32 = (uint32)FileSize.QuadPart;
            Result.FileMemory = PushSize_(Arena, FileSize32);
            if(Result.FileMemory)
            {
                DWORD BytesRead;
                if(ReadFile(FileHandle, Result.FileMemory, FileSize32, &BytesRead, 0) &&
                   (BytesRead == FileSize32))
                {
                    Result.FileSize = BytesRead;
                    // NOTE(jp): Success
                }
                else
                {
                    Result = {};
                    // TODO(jp): Logging
                }
            }
            else
            {
                // TODO(jp): Logging
            }
        }
        else
        {
            // TODO(jp): Logging
        }
        
        CloseHandle(FileHandle);
    }
    else
    {
        // TODO(jp): Log it
    }

    return(Result);
}

internal void
Win32BlitBitmap(win32_bitmap *Bitmap, HDC DeviceContext, int ClientWidth, int ClientHeight)
{
    int PadX = 10;
    int PadY = 10;
    PatBlt(DeviceContext, 0, 0, PadX, ClientHeight, WHITENESS);
    PatBlt(DeviceContext, 0, 0, ClientWidth, PadY, WHITENESS);
    PatBlt(DeviceContext, PadX + Bitmap->Width, 0, ClientWidth, ClientHeight, WHITENESS);
    PatBlt(DeviceContext, 0, PadY + Bitmap->Height, ClientWidth, ClientHeight, WHITENESS);
                        
    StretchDIBits(DeviceContext,
                  PadX, PadY, Bitmap->Width, Bitmap->Height, // Dest
                  0, 0, Bitmap->Width, Bitmap->Height,       // Source
                  Bitmap->Memory, &Bitmap->Info, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK
Win32MainWindowCallBack(HWND Window, UINT Message,
                        WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = {};

    switch(Message)
    {
#if 0
        case WM_SIZE:
        {
        } break;
#endif 
        case WM_CLOSE:
        {
            GlobalRunning = false;
        } break;

        case WM_QUIT:
        {
            GlobalRunning = false;
        } break;
        
        case WM_DESTROY:
        {
            GlobalRunning = false;
        } break;

        case WM_ACTIVATEAPP:
        {
        } break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
                                
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            int ClientWidth = ClientRect.right - ClientRect.left;
            int ClientHeight = ClientRect.bottom - ClientRect.top;
            
            Win32BlitBitmap(&GlobalWin32Bitmap, DeviceContext, ClientWidth, ClientHeight);
            EndPaint(Window, &Paint);
        } break;
        
        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }
    
    return(Result);
}

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine, int ShowCode)
{    
    WNDCLASSA WindowClass = {};
    WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallBack;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "CsteroidsClassName";
    
    if(RegisterClassA(&WindowClass))
    {
        HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, "Csteroids",
                                      WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                                      CW_USEDEFAULT, CW_USEDEFAULT,
                                      CW_USEDEFAULT, CW_USEDEFAULT,
                                      0, 0, Instance, 0);
        if(Window)
        {
            GlobalWin32Bitmap.Width = 1280;
            GlobalWin32Bitmap.Height = 720;
            GlobalWin32Bitmap.BytesPerPixel = 4;            
            GlobalWin32Bitmap.Pitch = GlobalWin32Bitmap.Width*GlobalWin32Bitmap.BytesPerPixel;
            GlobalWin32Bitmap.MemorySize = GlobalWin32Bitmap.Width*GlobalWin32Bitmap.Height*GlobalWin32Bitmap.BytesPerPixel;
            GlobalWin32Bitmap.Memory = VirtualAlloc(0, GlobalWin32Bitmap.MemorySize,
                                                    MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            
            GlobalWin32Bitmap.Info.bmiHeader.biSize = sizeof(GlobalWin32Bitmap.Info.bmiHeader);
            GlobalWin32Bitmap.Info.bmiHeader.biWidth = GlobalWin32Bitmap.Width;

            GlobalWin32Bitmap.Info.bmiHeader.biHeight = -GlobalWin32Bitmap.Height;
            GlobalWin32Bitmap.Info.bmiHeader.biPlanes = 1;
            GlobalWin32Bitmap.Info.bmiHeader.biBitCount = 32;
            GlobalWin32Bitmap.Info.bmiHeader.biCompression = BI_RGB;

            // NOTE(jp): Timing
            UINT DesiredSchedulerGranularity = 1;
            bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerGranularity) == TIMERR_NOERROR);
            int MonitorRefreshHz = 60;
            HDC RefreshDC = GetDC(Window);
            int Win32RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
            ReleaseDC(Window, RefreshDC);
            if(Win32RefreshRate > 1)
            {
                MonitorRefreshHz = Win32RefreshRate;
            }
            real32 GameUpdateHz = (real32)MonitorRefreshHz / 2.0f;
            real32 TargetSecondsPerFrame = 1.0f / GameUpdateHz;

            win32_sound_output SoundOutput = {};
            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.RunningSampleIndex = 0;
            SoundOutput.BytesPerSample = sizeof(int16)*2;
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
            SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 11;

            // Init direct sound 
            HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
            if(DSoundLibrary)
            {
                direct_sound_create *DirectSoundCreate = (direct_sound_create *)
                    GetProcAddress(DSoundLibrary, "DirectSoundCreate");

                LPDIRECTSOUND DirectSound;
                if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
                {
                    WAVEFORMATEX WaveFormat = {};
                    WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
                    WaveFormat.nChannels = 2;
                    WaveFormat.nSamplesPerSec = SoundOutput.SamplesPerSecond;
                    WaveFormat.wBitsPerSample = 16;
                    WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8;
                    WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
                    WaveFormat.cbSize = 0;

                    if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
                    {
                        DSBUFFERDESC BufferDescription = {};
                        BufferDescription.dwSize = sizeof(BufferDescription);
                        BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                        LPDIRECTSOUNDBUFFER PrimaryBuffer;
                        if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
                        {
                            if(SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
                            {
                                OutputDebugStringA("Primary buffer format was set.\n");
                            }
                        }
                    }

                    DSBUFFERDESC BufferDescription = {};
                    BufferDescription.dwSize = sizeof(BufferDescription);
                    BufferDescription.dwFlags = 0;
                    BufferDescription.dwBufferBytes = SoundOutput.SecondaryBufferSize;
                    BufferDescription.lpwfxFormat = &WaveFormat;

                    if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0)))
                    {
                        OutputDebugStringA("Secondary buffer created successfully.\n");
                        GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
                    }
                }
            }            
            
            game_memory GameMemory = {};
            GameMemory.PermanentStorageSize = Megabytes(64);
            GameMemory.TransientStorageSize = Megabytes(256);
            uint64 TotalMemorySize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
            // TODO(jp): Once you have it, test the reloading to see what changes with
            // an unknown base address.
#if 1
            uint64 BaseAddress = Terabytes(2);
#else
            uint64 BaseAddress = 0;
#endif
            GameMemory.PermanentStorage = VirtualAlloc((LPVOID)BaseAddress, TotalMemorySize,
                                                       MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            GameMemory.TransientStorage = (uint8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize;
            if(GlobalWin32Bitmap.Memory && GameMemory.PermanentStorage && GameMemory.TransientStorage)
            {                
                game_input GameInput[2] = {};
                game_input *NewInput = &GameInput[0];
                game_input *OldInput = &GameInput[1];

                HDC DeviceContext = GetDC(Window);
                GlobalRunning = true;
                
                LARGE_INTEGER QueryPerformanceFrequencyResult;
                QueryPerformanceFrequency(&QueryPerformanceFrequencyResult);
                uint64 CountsPerSecond = QueryPerformanceFrequencyResult.QuadPart;

                LARGE_INTEGER LastCounter = {};
                QueryPerformanceCounter(&LastCounter);
                uint64 LastCycles = __rdtsc();

                while(GlobalRunning)
                {
                    NewInput->dtForFrame = TargetSecondsPerFrame;
                    game_controller *NewController = &NewInput->Controller;
                    game_controller *OldController = &OldInput->Controller;
                    *NewController = {};
                    for(int ButtonIndex = 0;
                        ButtonIndex < ArrayCount(NewController->Buttons);
                        ++ButtonIndex)
                    {
                        NewController->Buttons[ButtonIndex].IsDown
                            = OldController->Buttons[ButtonIndex].IsDown;
                    }

                    MSG Message;
                    while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
                    {
                        if((Message.message == WM_KEYDOWN) ||
                           (Message.message == WM_KEYUP) ||
                           (Message.message == WM_SYSKEYDOWN) ||
                           (Message.message == WM_SYSKEYUP))
                        {
                            uint32 VKCode = (uint32)Message.wParam;
                            
#define WasKeyDownBit (1 << 30)
#define AltKeyDownBit (1 << 29)
#define IsKeyDownBit (1 << 31)
                            bool32 WasDown = ((Message.lParam & WasKeyDownBit) != 0);
                            bool32 IsDown = ((Message.lParam & IsKeyDownBit) == 0);
                            if(IsDown != WasDown)
                            {
                                switch(VKCode)
                                {
                                    case 'W':
                                    {
                                        NewController->MoveUp.IsDown = IsDown;
                                        NewController->MoveUp.HalfTransitionCount = 1;
                                    } break;
                                    
                                    case 'A':
                                    {
                                        NewController->MoveLeft.IsDown = IsDown;
                                        NewController->MoveLeft.HalfTransitionCount = 1;
                                    } break;
                                    
                                    case 'S':
                                    {
                                        NewController->MoveDown.IsDown = IsDown;
                                        NewController->MoveDown.HalfTransitionCount = 1;
                                    } break;
                                    
                                    case 'D':
                                    {
                                        NewController->MoveRight.IsDown = IsDown;
                                        NewController->MoveRight.HalfTransitionCount = 1;
                                    } break;
                                    
                                    case 'K':
                                    {
                                        NewController->ActionLeft.IsDown = IsDown;
                                        NewController->ActionLeft.HalfTransitionCount = 1;
                                    } break;
                                    
                                    case VK_SPACE:
                                    {
                                        NewController->ActionDown.IsDown = IsDown;
                                        NewController->ActionDown.HalfTransitionCount = 1;
                                    } break;
                                    
                                    case VK_ESCAPE:
                                    {
                                        NewController->Start.IsDown = IsDown;
                                        NewController->Start.HalfTransitionCount = 1;
                                    } break;

                                    case VK_F4:
                                    {
                                        bool32 AltKeyWasDown = (Message.lParam & AltKeyDownBit);
                                        if(AltKeyWasDown)
                                        {
                                            GlobalRunning = false;
                                        }
                                    } break;
                                }
                            }
                        }
                        else if(Message.message == WM_QUIT)
                        {
                            GlobalRunning = false;
                        }
                        else
                        {
                            TranslateMessage(&Message);
                            DispatchMessageA(&Message);
                        }
                    }
                    
                    game_sound_output GameSound = {};
                    GameSound.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                    
                    VOID *Region1 = 0;
                    DWORD Region1Size = 0;
                    VOID *Region2 = 0;
                    DWORD Region2Size = 0;
                    
                    DWORD PlayCursor;
                    DWORD WriteCursor;
                    if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
                    {
                        DWORD ByteToLock = ((SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) %
                                            SoundOutput.SecondaryBufferSize);
                        DWORD TargetCursor = ((PlayCursor +
                                               (SoundOutput.LatencySampleCount*SoundOutput.BytesPerSample)) %
                                              SoundOutput.SecondaryBufferSize);
                        DWORD BytesToWrite;
                        if(ByteToLock == TargetCursor)
                        {
                            BytesToWrite = 0;
                        }
                        else if(ByteToLock > TargetCursor)
                        {
                            BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
                            BytesToWrite += TargetCursor;
                        }
                        else
                        {
                            BytesToWrite = TargetCursor - ByteToLock;
                        }
                        
                        if(SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
                                                                 &Region1, &Region1Size,
                                                                 &Region2, &Region2Size,
                                                                 0)))
                        {                            
                            GameSound.Samples1 = (int16 *)Region1;
                            GameSound.SampleCount1 = Region1Size/SoundOutput.BytesPerSample;
                            GameSound.Samples2 = (int16 *)Region2;
                            GameSound.SampleCount2 = Region2Size/SoundOutput.BytesPerSample;
                        }                    
                    }
                
                    game_offscreen_buffer GameBuffer = {};
                    GameBuffer.Width = GlobalWin32Bitmap.Width;
                    GameBuffer.Height = GlobalWin32Bitmap.Height;
                    GameBuffer.BytesPerPixel = GlobalWin32Bitmap.BytesPerPixel;
                    GameBuffer.Pitch = GlobalWin32Bitmap.Pitch;
                    GameBuffer.Memory = GlobalWin32Bitmap.Memory;

                    GameUpdateAndRender(&GameBuffer, NewInput, &GameMemory, &GameSound);

                    if(Region1 || Region2)
                    {
                        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
                        SoundOutput.RunningSampleIndex += (GameSound.SampleCount1 + GameSound.SampleCount2);
                    }

                    // TODO(jp): Simplify this!
                    {
                        LARGE_INTEGER WorkCounter = {};
                        QueryPerformanceCounter(&WorkCounter);
                        uint64 CountsPerWork = WorkCounter.QuadPart - LastCounter.QuadPart;
                        real32 SecondsPerWork = 1.0f / ((real32)CountsPerSecond / (real32)CountsPerWork);
                        
                        if(SecondsPerWork < TargetSecondsPerFrame)
                        {
                            if(SleepIsGranular)
                            {
                                DWORD MilisecondsToSleep = (DWORD)((TargetSecondsPerFrame - SecondsPerWork) * 1000.0f);
                                Sleep(MilisecondsToSleep);
                            }

                            LARGE_INTEGER TestCounter = {};
                            QueryPerformanceCounter(&TestCounter);
                            uint64 CountsPerTest = TestCounter.QuadPart - LastCounter.QuadPart;
                            real32 SecondsPerTest = 1.0f / ((real32)CountsPerSecond / (real32)CountsPerTest);
                            while(SecondsPerTest < TargetSecondsPerFrame)
                            {
                                QueryPerformanceCounter(&TestCounter);
                                CountsPerTest = TestCounter.QuadPart - LastCounter.QuadPart;
                                SecondsPerTest = 1.0f / ((real32)CountsPerSecond / (real32)CountsPerTest);
                            }
                        }
                        else
                        {
                            // NOTE(jp): Missed framerate.
                        }
                    }

                    {
                        RECT ClientRect;
                        GetClientRect(Window, &ClientRect);
                        int ClientWidth = ClientRect.right - ClientRect.left;
                        int ClientHeight = ClientRect.bottom - ClientRect.top;
                        Win32BlitBitmap(&GlobalWin32Bitmap, DeviceContext, ClientWidth, ClientHeight);
                    }
                    
                    game_input *TempInput = OldInput;
                    OldInput = NewInput;
                    NewInput = TempInput;
                    
                    uint64 EndCycles = __rdtsc();
                    LARGE_INTEGER EndCounter = {};
                    QueryPerformanceCounter(&EndCounter);

                    {
                        uint64 CyclesPerFrame = EndCycles - LastCycles;
                        uint64 CountsPerFrame = EndCounter.QuadPart - LastCounter.QuadPart;
                        real32 MHzPerFrame = (real32)CyclesPerFrame / (1024.0f * 1024.0f);
                            
                        real32 FramesPerSecond = (real32)CountsPerSecond / (real32)CountsPerFrame;
                        real32 MilisecondsPerFrame = (1.0f / FramesPerSecond)*1000.0f;
                        char FramerateInfoBuffer[256];
                        sprintf_s(FramerateInfoBuffer, sizeof(FramerateInfoBuffer),
                                  "%.02f FPS, %.02f ms/f, %.02f MHz/f\n",
                                  FramesPerSecond, MilisecondsPerFrame, MHzPerFrame);
//                        OutputDebugStringA(FramerateInfoBuffer);
                    }

                    LastCycles = EndCycles;
                    LastCounter = EndCounter;
                }
            }
            else
            {
                // TODO(jp): Log that memory allocation (VirtualAlloc) failed
            }
        }
        else
        {
            // TODO(jp): Log that CreateWindowExA failed
        }
    }
    else
    {
        // TODO(jp): Log that RegisterClassA failed
    }
    
    return(0);
}
