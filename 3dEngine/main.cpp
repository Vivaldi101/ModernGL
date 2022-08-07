#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <string.h>
#include <assert.h>

#include "types.h"

#include "bmp.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include "vector.h"
#include "intrinsics.h"

#include "cube.h"

#define TIMER_RESOLUTION 1
#define USEC_FLOAT_PER_SIM_PER_SIM (16.6677f*1000.0f)
#define USEC_PER_SIM (u32)(USEC_FLOAT_PER_SIM_PER_SIM)

volatile global u64 globalTimeResidual = 0;
global u64 globalTimeFrequency = 0;
//global DWORD global_time_base = 0;

// TODO: Move these types/funcs into separate modules

typedef struct EventType
{
	HWND window;
	UINT message; 
	WPARAM wParam; 
	LPARAM lParam;
} EventType;

#include "ring_buffer.h"

function BitScan
FindFirstLSB(u32 value)
{
    BitScan result = {0};
    result.is_found = BitScanForward((DWORD *)&result.shift, value);
    
    return result;
}

typedef struct AppWindow
{
	Rectangle2 region;
	HWND handle;
} AppWindow;

function void
SysDebugPrintR32(f32 value)
{
    char buffer[32];
    sprintf(buffer, "%.2f\n", value);
    OutputDebugStringA(buffer);
}

function void
SysDebugPrintU32(u32 value)
{
    char buffer[32];
    sprintf(buffer, "%u\n", value);
    OutputDebugStringA(buffer);
}

function void
SysDebugPrintFPS(u32 value)
{
    char buffer[32];
    sprintf(buffer, "%u\n", value);
    OutputDebugStringA(buffer);
}

function HDC
SysGetWindowContextHandle(HWND windowHandle)
{
	HDC result = 0;
	Ensure(windowHandle, result = GetDC(windowHandle));

	return result;
}

function void
SysReleaseWindowContextHandle(HWND windowHandle, HDC contextHandle)
{
	Ensure(windowHandle, ReleaseDC(windowHandle, contextHandle));
}

function void
SysFreeFile(FileInfo *fi)
{
	VirtualFree(fi->data, 0, MEM_RELEASE);
	fi->size = 0;
}

function FileInfo
SysReadFile(const char *file_name)
{
    FileInfo fi = {};
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
                    SysFreeFile(&fi);
                    Assert(0);
                }
            }
            else
            {
                Assert(0);
            }
        }
        else
        {
            Assert(0);
        }
        CloseHandle(fh);
    }
    else
    {
		// TODO: Handle invalid asset paths.
        Assert(0);
    }
    
    return fi;
}

struct Bitmap
{
    void* memory;
    s32 width;
    s32 height;
    s32 pitch;
};

struct PushBuffer
{
	void* memory;
	size_t maxSize;
	size_t usedSize;
};

struct RenderState
{
	PushBuffer* renderCommands;
	Bitmap* renderTarget;
};

function Bitmap
LoadBMP(const char *filename)
{
    Bitmap result = {0};
    
    FileInfo file = SysReadFile(filename);
    Assert(file.size);
    
    BmpHeader *header = (BmpHeader *)file.data;
    
    result.memory = ((byte *)file.data + header->bmp_offset);
    result.width = header->width;
    result.height = header->height;
    
    BitScan red = FindFirstLSB(header->red_mask);
    BitScan green = FindFirstLSB(header->green_mask);
    BitScan blue = FindFirstLSB(header->blue_mask);
    BitScan alpha = FindFirstLSB(~(header->red_mask | header->green_mask | header->blue_mask));
    Assert(red.is_found && green.is_found && blue.is_found && alpha.is_found);
    u32 *pixels = (u32*)result.memory;
    
    // Swizzle the colorformat to BGRA in memory
    for(s32 i = 0; i < result.height; i++)
    {
        for(s32 j = 0; j < result.width; j++)
        {
            u32 pixel = *pixels;
            
            u32 salpha_original = (pixel >> alpha.shift) & 0xff;
            r32 salpha = (r32)((pixel >> alpha.shift) & 0xff)/255.0f;
            r32 sred = (r32)((pixel >> red.shift) & 0xff);
            r32 sgreen = (r32)((pixel >> green.shift) & 0xff);
            r32 sblue = (r32)((pixel >> blue.shift) & 0xff);
            
            V4 color = {sred, sgreen, sblue, salpha};
            color = PremultiplyAlpha(color);
            
            u32 calpha = (salpha_original << 24);
            u32 cred = (u32)(color.r + 0.5f) << 16;
            u32 cgreen = (u32)(color.g + 0.5f) << 8;
            u32 cblue = (u32)(color.b + 0.5f) << 0;
            
            *pixels = calpha | cred | cgreen | cblue;
            pixels++;
        }
    }
    
    
    result.pitch = result.width*BYTES_PER_PIXEL;
    // Get us to the first row of the bitmap.
    result.memory = ((byte *)result.memory + result.pitch*(result.height-1));
    
    // Set to negative because bottom up bitmaps and y goes down for our render renderer.
    result.pitch = -result.pitch;
    
    return result;
}

// Move into application layer.
typedef struct App
{
	struct Renderer* renderer;
	RingBuffer* eventQueue;
	BITMAPINFO* dib;
	AppWindow window;
	b32 isQuitting;
} App;

typedef struct WGLContext
{
	HGLRC handle;
} WGLContext;

typedef struct Renderer
{
    Bitmap activeTexture;
    PushBuffer renderCommands;
	WGLContext context;
    b32 isTopDown;
} Renderer;

#include "gl.cpp"
#include "renderer.cpp"
#include "3d.cpp"

function void*
SysAllocateAndCommit(size_t allocationSize)
{
	void* result = 0;
	Ensure(allocationSize > 0, result = VirtualAlloc(0, allocationSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));

	return result;
}

// TODO: Pass an arena to allocate from.
function Bitmap
SysMakeBitmap(u32 width, u32 height)
{
	Bitmap result = {0};

	Ensure(width > 0 && height > 0, true)
	{
		result.width = width;
		result.height = height;
		result.pitch = result.width * BYTES_PER_PIXEL;

		result.memory = SysAllocateAndCommit(result.width * result.pitch);
	}

	return result;
}


function Renderer
SysMakeRenderer(u32 width, u32 height, size_t renderBufferSize)
{
    Renderer result = {0};
	Ensure((width > 0 && height > 0 && renderBufferSize > 0), true)
	{
		result.activeTexture = SysMakeBitmap(width, height);
		result.renderCommands.memory = SysAllocateAndCommit(renderBufferSize);
		result.renderCommands.maxSize = renderBufferSize;
	}
    
    return result;
}


function void
SysClearTexture(Bitmap* texture)
{
	Ensure(texture->height && texture->pitch, true)
	{
		void* data = texture->memory;
		s32 height = texture->height;
		s32 pitch = texture->pitch;
		memset(data, 0, height * pitch);
	}
}

function BITMAPINFO
SysMakeDIB(Renderer* renderer)
{
    BITMAPINFO result = {0};
	Ensure(renderer, true)
	{
		result.bmiHeader.biSize = sizeof(result.bmiHeader);
		result.bmiHeader.biWidth = renderer->activeTexture.width;

		// Top-down bitmap!
		result.bmiHeader.biHeight = -renderer->activeTexture.height;
		result.bmiHeader.biPlanes = 1;
		result.bmiHeader.biBitCount = 32;
		result.bmiHeader.biCompression = BI_RGB;

		renderer->isTopDown = (result.bmiHeader.biHeight < 0) ? 1 : 0;
	}
    
    return result;
}

LRESULT CALLBACK
SysWindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
    App* application = (App*)GetWindowLongPtr(window, GWLP_USERDATA);

	if(application)
	{
		EventType ev = {0};
		ev.window = window;
		ev.message = message;
		ev.wParam = wParam;
		ev.lParam = lParam;
		switch(message)
		{
			case WM_DESTROY:
			{
				PostQuitMessage(0);
				ev.message = WM_QUIT;
			} break;
			default:
			{
				result = DefWindowProc(window, message, wParam, lParam);
				break;
			}
		}
		EnqueueEvent(application->eventQueue, ev);
		return result;
	}
	return DefWindowProc(window, message, wParam, lParam);
}

function WNDCLASS
SysMakeWindowClass(HINSTANCE module)
{
	WNDCLASS result = {0};
	Ensure(module, RegisterClass(&result))
	{
		result.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		result.hInstance = module;
		result.lpfnWndProc = SysWindowProcedure;
		result.lpszClassName = "3dEngine";
		result.hCursor = LoadCursor(NULL, IDC_ARROW);
	}

	return result;
}

function AppWindow
SysMakeWindow(HINSTANCE module, s32 x, s32 y, s32 width, s32 height)
{
	AppWindow window = {0};
	Ensure(width > 0 && height > 0, window.handle)
	{
		WNDCLASS windowClass = SysMakeWindowClass(module);
		RECT windowRectangle = {0};

		windowRectangle.left = 0;
		windowRectangle.top = 0;
		windowRectangle.right = width;
		windowRectangle.bottom = height;

		AdjustWindowRect(&windowRectangle, WS_OVERLAPPEDWINDOW, FALSE);
		window.handle = CreateWindow(windowClass.lpszClassName, "3dEngine", 
			WS_OVERLAPPEDWINDOW | WS_VISIBLE, x, y, 
			windowRectangle.right - windowRectangle.left, windowRectangle.bottom - windowRectangle.top, 
			0, 0, module, 0);
		window.region.left = 0;
		window.region.bottom = 0;
		window.region.top = height;
		window.region.right = width;
	}

	UpdateWindow(window.handle);
	ShowWindow(window.handle, SW_SHOWNORMAL);
	return window;
}

function void
SysSleep(DWORD time)
{
    Sleep(time);
}

function u64
SysGetMicroseconds()
{
    u64 result;
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    result = (s64)(((f64)counter.QuadPart / globalTimeFrequency)*1000000);

    return result;
}

function void
SysBeginTimer()
{
    timeBeginPeriod(TIMER_RESOLUTION);
}

function void
SysEndTimer()
{
    timeEndPeriod(TIMER_RESOLUTION);
}

function s32
SysWaitForFrame(u64 nowTime)
{
    s32 frameUpdateCount = 0;
    u64 lastFrameTime = nowTime;
    for(;;)
    {
        u64 currentFrameTime = SysGetMicroseconds();
        u64 deltaMicroSeconds = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        globalTimeResidual += deltaMicroSeconds;

        for(;;)
        {
            // how much to wait before running the next frame
            if(globalTimeResidual < USEC_PER_SIM)
            {
                break;
            }
            globalTimeResidual -= USEC_PER_SIM;
            frameUpdateCount++;
        }
        if(frameUpdateCount > 2)
        {
            frameUpdateCount = 2;
            break;
        }
        if(frameUpdateCount > 0)
        {
            break;
        }
        SysSleep(0);
    }

    return frameUpdateCount;
}

s32 CALLBACK
WinMain(HINSTANCE appInstance,
        HINSTANCE prevAppInstance,
        LPSTR     cmdLine,
        int       showCmd)
{
	App application = {0};
	RingBuffer eventQueue = {0};

	Renderer renderer = SysMakeRenderer(800, 600, MEGABYTES(256));
	BITMAPINFO dib = SysMakeDIB(&renderer);
	application.window = SysMakeWindow(appInstance, 100, 100, renderer.activeTexture.width, renderer.activeTexture.height);
	application.eventQueue = &eventQueue;
	application.renderer = &renderer;
	application.dib = &dib;

	SetWindowLongPtr(application.window.handle, GWLP_USERDATA, (LONG_PTR)&application);
	InitializeOpenGL(&application);

	SysBeginTimer();
	LARGE_INTEGER perfCounterFrequency;
    QueryPerformanceFrequency(&perfCounterFrequency);
    globalTimeFrequency = perfCounterFrequency.QuadPart;

	volatile u64 lastBlitTime = SysGetMicroseconds();
	u64 debugFrameTimeBegin = SysGetMicroseconds();

	Bitmap testBrickTexture = LoadBMP("data/brick.bmp");
	Bitmap testStoneTexture = LoadBMP("data/stone.bmp");

	HANDLE threadHandle = 0;

	while(!application.isQuitting)
	{
		// Pump the message loop.
		MSG message;
		while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
		// Dequeue the messages sent.
		while(EventCount(&eventQueue) > 0)
		{
			RING_BUFFER_TYPE ev = GetNext(&eventQueue);
			//Printf("Event: %u\n", ev.message);
			if(ev.message == WM_QUIT)
			{
				application.isQuitting = true;
				goto quit;
			}
		}
		f32 left = (f32)application.window.region.left;
		f32 top = (f32)application.window.region.bottom;
		f32 right = (f32)application.window.region.right;
		f32 bottom = (f32)application.window.region.top;
		SysClearTexture(&application.renderer->activeTexture);
		{
			f32 left = (f32)application.window.region.left;
			f32 top = (f32)application.window.region.bottom;
			f32 right = (f32)application.window.region.right;
			f32 bottom = (f32)application.window.region.top;

			SysClearTexture(&application.renderer->activeTexture);

			PushDrawRectangle(&renderer.renderCommands, left, top, right, bottom, MakeV4(1.0f, 0.0f, 0.0f, 1.0f));

			static f32 degrees = 0.0f;
			degrees += 2.0f;

			f32 axisScaleX = 64.0f*2;
			f32 axisScaleY = 64.0f*2;

			V2 xAxis = {cosf(DegreesToRadians(degrees)) * axisScaleX, sinf(DegreesToRadians(degrees)) * axisScaleX};
			V2 yAxis = {-sinf(DegreesToRadians(degrees)) * axisScaleY, cosf(DegreesToRadians(degrees)) * axisScaleY};

			V2 origin = {axisScaleX + 200.0f, axisScaleY + 200.0f};

			//PushDrawBasis2d(&renderer.renderCommands, xAxis, yAxis, origin, basisPoint, MakeV4(1.0f, 0.0f, 0.0f, 1.0f));
			PushDrawBitmap(&renderer.renderCommands, &testBrickTexture, xAxis, yAxis, origin, 0.0f, 0.0f);
			//PushDrawBitmap(&renderer.renderCommands, &testStoneTexture, xAxis, yAxis, origin + MakeV2(20.0f, 20.0f), 0.0f, 0.0f);
			//PushDrawBitmap(&renderer.renderCommands, &testBrickTexture, xAxis, yAxis, origin + MakeV2(30.0f, 30.0f), 0.0f, 0.0f);
		}

		PushEndDraw(&application.renderer->activeTexture, &renderer.renderCommands, &application.window);

		RenderState renderState = {};
		renderState.renderCommands = &application.renderer->renderCommands;
		renderState.renderTarget = &application.renderer->activeTexture;

		//IssueRenderCommands(&application.renderer->activeTexture, &renderer.renderCommands);
		IssueRenderCommands(&renderState);

		SysWaitForFrame(lastBlitTime);
		lastBlitTime = SysGetMicroseconds();

		u64 debugFrameTimeEnd = SysGetMicroseconds();
		u64 debugFrameTimeDelta = debugFrameTimeEnd - debugFrameTimeBegin;
		debugFrameTimeBegin = debugFrameTimeEnd;
		SysDebugPrintR32(debugFrameTimeDelta/1000.0f);
		//SysDebugPrintFPS((u32)((1.0f / ((f32)debugFrameTimeDelta/1000000.0f)) + 0.5f));
	}

	quit:

	SysEndTimer();

	return 0;
}
