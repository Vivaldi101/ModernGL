
//#define WINDOW_CLASS (LPCSTR)"TopWindowClass"
#define USEC_PER_SIM (u32)(16.666677f*1000)
#define TIMER_RESOLUTION 1
#define _CRT_SECURE_NO_WARNINGS

#include "common.h"
#include "win_main.h"

#include "win_assert.h"

USE_ASSERT

volatile global b32 global_is_running = false;

global u64 global_game_time_residual = 0;
global u64 global_game_time_freq = 0;
global DWORD global_time_base = 0;

//#if _DEBUG
internal void
SysDebugPrintMsg(const char *debug_msg)
{
    char buffer[256];
    wsprintf(buffer, "%s\n", debug_msg);
    OutputDebugStringA(debug_msg);
}

internal void
SysDebugPrintS32(s32 value)
{
    char buffer[256];
    wsprintf(buffer, "%d\n", value);
    OutputDebugStringA(buffer);
}

internal void
SysDebugPrintU32(u32 value)
{
    char buffer[256];
    wsprintf(buffer, "%u\n", value);
    OutputDebugStringA(buffer);
}

internal void
SysDebugPrintR32(r32 value)
{
    char buffer[256];
    sprintf(buffer, "%f\n", value);
    OutputDebugStringA(buffer);
}

internal void
SysDebugPrintFPS(u64 value)
{
    char buffer[256];
    sprintf(buffer, "%u\n", (u32)((1.0f / ((r32)value/1000000))));
    OutputDebugStringA(buffer);
}
//#endif   // _DEBUG

void
SysFreeFile(ThreadContext *thread, FileInfo *fi)
{
    //IsMsg(fi->data, "SysFreeFile failed!")
    {
        VirtualFree(fi->data, 0, MEM_RELEASE);
        fi->size = 0;
    }
}

FileInfo
SysReadFile(ThreadContext *thread, const char *file_name)
{
    FileInfo fi = {0};
    HANDLE fh = CreateFile(file_name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    
    if (fh != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER file_size;
        if (GetFileSizeEx(fh, &file_size))
        {
            u32 file_size_32 = (u32)(file_size.QuadPart);
            fi.data = VirtualAlloc(0, file_size_32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (fi.data)
            {
                DWORD bytes_read = 0;
                if (ReadFile(fh, fi.data, file_size_32, &bytes_read, 0) &&
                    (file_size_32 == bytes_read))
                {
                    fi.size = bytes_read;
                }
                else
                {
                    SysFreeFile(thread, &fi);
                    Is(0);
                }
            }
            else
            {
                Is(0);
            }
        }
        else
        {
            Is(0);
        }
        CloseHandle(fh);
    }
    else
    {
        Is(0);
    }
    
    return fi;
}

b32
SysWriteFile(ThreadContext *thread, const char *file_name, u32 memory_size, void *memory)
{
    b32 success = false;
    HANDLE fh = CreateFile(file_name, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (fh != INVALID_HANDLE_VALUE)
    {
        DWORD bytes_written;
        if (WriteFile(fh, memory, memory_size, &bytes_written, 0)) {
            success = (memory_size == bytes_written);
        }
        else
        {
            Is(0);
        }
        CloseHandle(fh);
    }
    else
    {
        Is(0);
    }
    return success;
}

internal void
SysProcessKeyMessage(Button *button, b32 is_down)
{
    if (button->ended_down && button->toggle)
    {
        button->toggle = false;
    }
    else if(button->ended_down && !button->toggle)
    {
        button->toggle = true;
    }
    if(button->ended_down != is_down)
    {
        button->ended_down = is_down;
        ++button->half_trans_count;
    }
}

internal DrawableWindow
SysGetDrawableWindow(HWND window_handle)
{
    DrawableWindow result;
    RECT rectangle;
    GetClientRect(window_handle, &rectangle);
    
    s32 w = rectangle.right - rectangle.left;
    s32 h = rectangle.bottom - rectangle.top;
    
    result.width = w;
    result.height = h;
    
    return result;
}

internal BITMAPINFO
SysMakeDIB(Renderer *renderer)
{
    BITMAPINFO result = {};
    result.bmiHeader.biSize = sizeof(result.bmiHeader);
    result.bmiHeader.biWidth = renderer->width;
    
    // Top-down bitmap!
    result.bmiHeader.biHeight = -renderer->height;
    result.bmiHeader.biPlanes = 1;
    result.bmiHeader.biBitCount = 32;
    result.bmiHeader.biCompression = BI_RGB;
    
    renderer->isTopDown = (result.bmiHeader.biHeight < 0) ? 1 : 0;
    
    return result;
}

#if 0
internal void
SysInitRenderer(Renderer *renderer, s32 x, s32 y, s32 width, s32 height)
{
    renderer->x = x;
    renderer->y = y;
    renderer->width = width;
    renderer->height = height;
    renderer->bpp = BYTES_PER_PIXEL;
    
    u32 frameBuffer_size = width*height*renderer->bpp;
    // TODO: Optimize later?
    u32 renderBuffer_size = MEGABYTES(32);
    renderer->pitch = width*renderer->bpp;
    
    // TODO: Pool the buffers.
    renderer->frameBuffer = VirtualAlloc(NULL, frameBuffer_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    
    renderer->renderBuffer.data = Cast(byte*, VirtualAlloc(NULL, renderBuffer_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    renderer->renderBuffer.maxSize = renderBuffer_size;
    renderer->renderBuffer.usedSize = 0;
    
    renderer->activeTexture.width = renderer->width;
    renderer->activeTexture.height = renderer->height;
    renderer->activeTexture.pitch = renderer->pitch;
    renderer->activeTexture.pixels = (u32 *)renderer->frameBuffer;
    
    // Check the allocated buffers.
    Is(renderer->frameBuffer && renderer->renderBuffer.data);
}
#else
internal Renderer
SysInitRenderer(s32 width, s32 height)
{
    Renderer result = {};
    result.width = width;
    result.height = height;
    result.bpp = BYTES_PER_PIXEL;
    
    u32 frameBuffer_size = width*height*result.bpp;
    // TODO: Optimize later?
    u32 renderBuffer_size = MEGABYTES(32);
    result.pitch = width*result.bpp;
    
    // TODO: Pool the buffers.
    result.frameBuffer = VirtualAlloc(NULL, frameBuffer_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    
    result.renderBuffer.data = Cast(byte*, VirtualAlloc(NULL, renderBuffer_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    result.renderBuffer.maxSize = renderBuffer_size;
    result.renderBuffer.usedSize = 0;
    
    result.activeTexture.width = result.width;
    result.activeTexture.height = result.height;
    result.activeTexture.pitch = result.pitch;
    result.activeTexture.pixels = (u32 *)result.frameBuffer;
    
    // Check the allocated buffers.
    Is(result.frameBuffer && result.renderBuffer.data);
    
    return result;
}
#endif

internal void
SysBlit(HWND window_handle, BITMAPINFO *win32_bmp_info, Renderer *renderer)
{
    HDC dc = GetDC(window_handle);
    
    StretchDIBits(dc, 0, 0, renderer->width, renderer->height, renderer->x, renderer->y, renderer->width, renderer->height,
                  renderer->frameBuffer, win32_bmp_info,  DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(window_handle, dc);
}

internal void
SysClearScreen(Renderer *renderer)
{
    void *data = renderer->frameBuffer;
    s32 w = renderer->width;
    s32 h = renderer->height;
    s32 bpp = renderer->bpp;
    memset(data, 0, w*h*bpp);
}

internal DWORD
SysAbort()
{
    DWORD result = GetLastError();
    global_is_running = false;
    
    return result;
}

LRESULT CALLBACK
SysTopWindowCallback(HWND window_handle, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LRESULT result = 0;
#define PREVIOUS_KEY_STATE (1<<30)
#define TRANSITION_KEY_STATE (1<<31)
    switch(msg)
    {
        case WM_SIZE:
        {
        } break;
        case WM_DESTROY:
        {
        } break;
        case WM_KEYUP:
        {
            u32 vkcode = (u32)wparam;
            b32 was_down = TRANSITION_KEY_STATE & (u32)lparam;
            
            if(was_down && (vkcode == VK_ESCAPE))
            {
                SysAbort();
            }
        } break;
        case WM_CLOSE:
        {
            SysAbort();
        } break;
        case WM_ACTIVATEAPP:
        {
        } break;
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(window_handle, &ps);
            EndPaint(window_handle, &ps);
        } break;
        default:
        {
            result = DefWindowProc(window_handle, msg, wparam, lparam);
        } break;
    }
    
    return result;
}

internal HWND
SysCreateWindow(LPCSTR title, s32 width, s32 height, HINSTANCE program_instance,LPCSTR window_class)
{
    HWND current_window = CreateWindowEx(0,
                                         window_class,
                                         title,
                                         WS_VISIBLE|WS_EX_TOPMOST|WS_POPUP,
                                         0,
                                         0,
                                         width,
                                         height,
                                         0,
                                         0,
                                         program_instance,
                                         0);
    //IsMsg(current_window, "SysCreateWindow failed!")
    {
    }
    
    return current_window;
}

internal HWND
SysRegisterAndCreateWindow(HINSTANCE instance, u32 width, u32 height, WNDPROC window_callback, LPCSTR window_class, u32 class_style, u32 window_style)
{
    HWND result;
    WNDCLASS top_window_class = {0};
    top_window_class.style = class_style;
    top_window_class.hInstance = instance;
    top_window_class.lpfnWndProc = window_callback;
    top_window_class.lpszClassName = window_class;
    
    if(RegisterClass(&top_window_class))
    {
        //TODO: Log registering of window class
    }
    else
    {
        SysAbort();
        //TODO: Log window registeration failed
    }
    
    // Fill only certain necerrary args.
    result = CreateWindowEx(0,
                            window_class,
                            "",
                            window_style,
                            0,
                            0,
                            width,
                            height,
                            0,
                            0,
                            instance,
                            0);
    
    return result;
}

internal void
SysShowWindow(HWND window)
{
    ShowWindow(window, SW_SHOW);
}

internal void
SysHideWindow(HWND window)
{
    ShowWindow(window, SW_HIDE);
}

internal void
SysConsumeMessageQueue(Input *input)
{
    MSG msg;
    while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        switch(msg.message)
        {
            case WM_QUIT:
            {
                global_is_running = false;
            } break;
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                u32 vk_code = (u32)msg.wParam;
                b32 was_down = (msg.lParam & (1<<30)) != 0;
                b32 is_down = (msg.lParam & (1<<31)) == 0;
                
                if(was_down != is_down)
                {
                    if(vk_code == 'W')
                    {
                        SysProcessKeyMessage(&input->keys.move_up, is_down);
                    }
                    else if(vk_code == 'A' || vk_code == VK_LEFT)
                    {
                        SysProcessKeyMessage(&input->keys.move_left, is_down);
                    }
                    else if(vk_code == 'S')
                    {
                        SysProcessKeyMessage(&input->keys.move_down, is_down);
                    }
                    else if(vk_code == 'D')
                    {
                        SysProcessKeyMessage(&input->keys.move_right, is_down);
                    }
                    else if(vk_code == VK_RETURN)
                    {
                        SysProcessKeyMessage(&input->keys.reset_world, is_down);
                    }
                    else if(vk_code == VK_SPACE)
                    {
                        SysProcessKeyMessage(&input->keys.player_speed_boost, is_down);
                    }
                    else if(vk_code == 'F')
                    {
                        SysProcessKeyMessage(&input->keys.debug, is_down);
                    }
                }
            } break;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

inline internal void
SysSleep(DWORD time)
{
    Sleep(time);
}

internal u64
SysGetMicroseconds()
{
    u64 result;
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    
    result = (s64)(((r64)counter.QuadPart / global_game_time_freq)*1000000);
    
    return result;
}

internal void
SysBeginTimer()
{
    timeBeginPeriod(TIMER_RESOLUTION);
}

internal void
SysEndTimer()
{
    timeEndPeriod(TIMER_RESOLUTION);
}

internal s32
SysWaitForFrame(u64 now_time)
{
    s32 num_frames_to_update = 0;
    u64 last_frame_time = now_time;
    for(;;)
    {
        u64 current_frame_time = SysGetMicroseconds();
        u64 delta_micro_seconds = current_frame_time - last_frame_time;
        last_frame_time = current_frame_time;
        
        global_game_time_residual += delta_micro_seconds;
        
        for(;;)
        {
            // how much to wait before running the next frame
            if(global_game_time_residual < USEC_PER_SIM)
            {
                break;
            }
            global_game_time_residual -= USEC_PER_SIM;
            num_frames_to_update++;
        }
        if(num_frames_to_update > 5)
        {
            num_frames_to_update = 5;
        }
        if(num_frames_to_update > 0)
        {
            break;
        }
        SysSleep(0);
    }
    
    return num_frames_to_update;
}

internal MemoryPool
SysMakeMemoryPool(u64 permanent_size, u64 transient_size)
{
    MemoryPool result = {};
    result.permanent_size = permanent_size;
    result.transient_size = transient_size;
    
    byte *start_address = 0;
#if _DEBUG
    start_address = (byte *)GIGABYTES(1);
    result.permanent_data = (byte *)VirtualAlloc(start_address, permanent_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    result.transient_data = (byte *)VirtualAlloc(start_address + permanent_size, transient_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
#else
    result.permanent_data = (byte *)VirtualAlloc(start_address, permanent_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    result.transient_data = (byte *)VirtualAlloc(start_address + permanent_size, transient_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
#endif    // _DEBUG
    
    // Make sure we got the memory we asked for.
    Is(result.permanent_data);
    Is(result.transient_data);
    
    return result;
}

internal void
SysDestroyArena(MemoryArena *arena)
{
    if(arena->data)
    {
        VirtualFree(arena->data, 0, MEM_RELEASE);
        arena->data = NULL;
    }
}

internal void
SysDestroyMemoryPool(MemoryPool *memory_pool)
{
    if(memory_pool->permanent_data)
    {
        VirtualFree(memory_pool->permanent_data, 0, MEM_RELEASE);
        memory_pool->permanent_data = NULL;
    }
    if(memory_pool->transient_data)
    {
        VirtualFree(memory_pool->transient_data, 0, MEM_RELEASE);
        memory_pool->transient_data = NULL;
    }
}

internal GameDLLApi
SysLoadGameDLLApi()
{
    GameDLLApi result = {0};
    // Copy the dll to be loaded into a temp file
    // to avoid debugger pdb locking for hot code reloading purposes
    
    //CopyFile("../libs/Game.dll", "../libs/Game_temp.dll", FALSE);
    result.dll_handle = LoadLibraryA("libs/Game.dll");
    
    //IsMsg(result.dll_handle, "LoadLibraryA() failed")
    {
        UpdateAndRender dll_api_ur = (UpdateAndRender)GetProcAddress(result.dll_handle,
                                                                     "DLL_UpdateAndRender");
        //IsMsg(dll_api_ur, "GetProcAddress() failed")
        if(dll_api_ur)
        {
            result.update_and_render_fp = dll_api_ur;
            result.is_valid = true;
        }
    }
    
    return result;
}

internal void
SysUnloadGameDLLApi(GameDLLApi *api)
{
    if(api->is_valid)
    {
        FreeLibrary(api->dll_handle);
        api->dll_handle = 0;
        api->update_and_render_fp = NULL;
        api->is_valid = false;
    }
}

internal PlatformApi
SysMakePlatformApi()
{
    PlatformApi result = {};
    result.read_file_pf = SysReadFile;
    //#if _DEBUG
    result.printu32_pf = SysDebugPrintU32;
    result.prints32_pf = SysDebugPrintS32;
    result.printr32_pf = SysDebugPrintR32;
    result.printmsg_pf = SysDebugPrintMsg;
    //#else
    //win32_platform->prints32_pf = NULL;
    //win32_platform->printr32_pf = NULL;
    //win32_platform->printmsg_pf = NULL;
    //#endif // _DEBUG
    
    return result;
}

internal void
SysUpdateMouse(HWND top_window_handle, Input *input)
{
    POINT pt;
    if(GetCursorPos(&pt))
    {
        ScreenToClient(top_window_handle, &pt);
        input->mouse.x = pt.x;
        input->mouse.y = pt.y;
    }
    else
    {
        input->mouse.x = 0;
        input->mouse.y = 0;
    }
    
    SysProcessKeyMessage(&input->mouse.buttons[MOUSE_LEFT_CLICK], GetKeyState(VK_LBUTTON) & (1<<15));
    SysProcessKeyMessage(&input->mouse.buttons[MOUSE_RIGHT_CLICK], GetKeyState(VK_RBUTTON) & (1<<15));
}

s32 CALLBACK
WinMain(HINSTANCE instance,
        HINSTANCE prev_instance,
        LPSTR     cmd_line,
        int       show_cmd)
{
    ThreadContext thread = {};
    Input input = {};
    
    Renderer renderer = SysInitRenderer(frameBuffer_WIDTH, frameBuffer_HEIGHT);
    BITMAPINFO win32_bmp_info = SysMakeDIB(&renderer);
    MemoryPool memory_pool = SysMakeMemoryPool(MEGABYTES(128), MEGABYTES(512));
    
    GameDLLApi api = SysLoadGameDLLApi();
    PlatformApi win32_platform = SysMakePlatformApi();
    
    HWND top_window_handle = SysRegisterAndCreateWindow(instance, frameBuffer_WIDTH, frameBuffer_HEIGHT, SysTopWindowCallback, (LPCSTR)"TopWindowClass", CS_OWNDC|CS_HREDRAW|CS_VREDRAW, WS_VISIBLE|WS_EX_TOPMOST|WS_POPUP);
    
    HDC refresh_dc = GetDC(top_window_handle);
    s32 refresh_rate = GetDeviceCaps(refresh_dc, VREFRESH);
    ReleaseDC(top_window_handle, refresh_dc);
    r32 renderer_seconds_per_frame = 1.0f / Cast(r32, refresh_rate);
    win32_platform.seconds_to_update = renderer_seconds_per_frame;
    
    SysShowWindow(top_window_handle);
    
    SysBeginTimer();
    LARGE_INTEGER perf_counter_freq;
    QueryPerformanceFrequency(&perf_counter_freq);
    global_game_time_freq = perf_counter_freq.QuadPart;
    u64 dll_refresh_time_begin = SysGetMicroseconds();
    u64 dll_refresh_time_end = dll_refresh_time_begin;
    
    u64 debug_frame_time_begin = SysGetMicroseconds();
    //u32 num_frames_to_update = 1;
    global_is_running = true;
    u64 last_blit_time = SysGetMicroseconds();
    while(global_is_running)
    {
#if HOT_CODE_RELOAD
        // Load a fresh dll every second for hot code reloading
        if(((dll_refresh_time_end - dll_refresh_time_begin)) >= 1000)
        {
            dll_refresh_time_begin = dll_refresh_time_end;
            SysUnloadGameDLLApi(&api);
            api = SysLoadGameDLLApi();
        }
        dll_refresh_time_end = SysGetMicroseconds();
#endif
        
        SysUpdateMouse(top_window_handle, &input);
        SysConsumeMessageQueue(&input);
        
        SysClearScreen(&renderer);
        
        u64 update_and_render_begin = SysGetMicroseconds();
        
        api.update_and_render_fp(&thread, &win32_platform, &input, &renderer, &memory_pool);
        SysBlit(top_window_handle, &win32_bmp_info,
                &renderer);
        u64 update_and_render_end = SysGetMicroseconds();
        SysWaitForFrame(last_blit_time);
        last_blit_time = SysGetMicroseconds();
        {
            
#if 1
            u64 debug_frame_time_end = SysGetMicroseconds();
            u64 debug_frame_time_delta = debug_frame_time_end - debug_frame_time_begin;
            debug_frame_time_begin = debug_frame_time_end;
            u64 update_and_render_delta = update_and_render_end - update_and_render_begin;
            //SysDebugPrintR32((r32)debug_frame_time_delta/1000.0f);
            SysDebugPrintFPS(debug_frame_time_delta);
#endif // _DEBUG
        }
    }
    
    SysUnloadGameDLLApi(&api);
    SysEndTimer();
    //SysDestroyArena(&main_arena);
    SysDestroyMemoryPool(&memory_pool);
    return 0;
}
