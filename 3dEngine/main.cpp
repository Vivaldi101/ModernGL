#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <gl\GL.h>

#include "common.h"

#include <string.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include "vector.h"
#include "intrinsics.h"
#include "3d.h"

#define TIMER_RESOLUTION 1
#define USEC_PER_SIM (u32)(16.677f*1000.0f)

global u64 globalTimeResidual = 0;
global u64 globalTimeFrequency = 0;
//global DWORD global_time_base = 0;


typedef struct EventType
{
	HWND window;
	UINT message; 
	WPARAM wParam; 
	LPARAM lParam;
} EventType;
#include "ring_buffer.h"

typedef struct AppWindow
{
	Rectangle2 region;
	HWND handle;
} AppWindow;

function void
SysDebugPrintR32(f32 value)
{
    char buffer[256];
    sprintf(buffer, "%f\n", value);
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

// Opengl TODO: Move elsewhere
typedef BOOL WINAPI SwapInterval(int swapInterval);
global SwapInterval *wglSwapInterval;

struct Bitmap
{
    void* memory;
    s32 width;
    s32 height;
    s32 pitch;
};

typedef struct PushBuffer
{
	void* memory;
	size_t maxSize;
	size_t usedSize;
} PushBuffer;

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

#include "renderer.cpp"


typedef struct App
{
	Renderer* renderer;
	RingBuffer* eventQueue;
	BITMAPINFO* dib;
	AppWindow window;
	b32 isQuitting;
} App;

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

function void
InitializeOpenGL(App* application)
{
	Ensure(true, application->renderer->context.handle)
	{
		HDC windowDC = SysGetWindowContextHandle(application->window.handle);
		PIXELFORMATDESCRIPTOR desiredFormat = {};
		PIXELFORMATDESCRIPTOR suggestedFormat = {};
		HGLRC renderContextHandle;
		s32 suggestedFormatIndex;

		desiredFormat.nSize = sizeof(desiredFormat);
		desiredFormat.nVersion = 1;
		desiredFormat.dwFlags = PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
		desiredFormat.cColorBits = 24;
		desiredFormat.cAlphaBits = 8;
		desiredFormat.iLayerType = PFD_MAIN_PLANE;

		suggestedFormatIndex = ChoosePixelFormat(windowDC, &desiredFormat);
		TestBool(DescribePixelFormat(windowDC, suggestedFormatIndex, sizeof(suggestedFormat), &suggestedFormat) 
			&& SetPixelFormat(windowDC, suggestedFormatIndex, &suggestedFormat));

		renderContextHandle = wglCreateContext(windowDC);
		application->renderer->context.handle = renderContextHandle;

		TestBool(wglMakeCurrent(windowDC, application->renderer->context.handle));

		ReleaseDC(application->window.handle, windowDC);
		SysReleaseWindowContextHandle(application->window.handle, windowDC);
		{
			Ensure(true, wglSwapInterval = (SwapInterval*)wglGetProcAddress("wglSwapIntervalEXT"));
			wglSwapInterval(1);
		}
	}
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
        if(frameUpdateCount > 5)
        {
            frameUpdateCount = 5;
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

	u64 lastBlitTime = SysGetMicroseconds();
	u64 debugFrameTimeBegin = SysGetMicroseconds();

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
		while(EventCount(&eventQueue))
		{
			RING_BUFFER_TYPE ev = GetNext(&eventQueue);
			//Printf("Event: %u\n", ev.message);
			if(ev.message == WM_QUIT)
			{
				application.isQuitting = true;
				goto quit;
			}
		}

		{
			// Test code.
			f32 left = (f32)application.window.region.left;
			f32 top = (f32)application.window.region.bottom;
			f32 right = (f32)application.window.region.right;
			f32 bottom = (f32)application.window.region.top;
			SysClearTexture(&application.renderer->activeTexture);

			//PushDrawRectangle(&renderer.renderCommands, left, top, right, bottom, MakeV4(0.5f, 0.5f, 0.5f, 1.0f));

#define DegreesToRadians(degrees) ((f32)M_PI / 180.0f) * degrees

			static f32 degrees = 0.0f;
			V2 xAxis = {cosf(DegreesToRadians(degrees)), sinf(DegreesToRadians(degrees))};
			V2 yAxis = {-sinf(DegreesToRadians(degrees)), cosf(DegreesToRadians(degrees))};
			degrees += 1.0f;

			V2 origin = {100.0f, 100.0f};
			f32 axisScale = 100.0f;
			V2 basisPoint = {10.0f, 10.0f};

			PushBasis2d(&renderer.renderCommands, xAxis, yAxis, origin, basisPoint, axisScale, MakeV4(1.0f, 0.0f, 0.0f, 1.0f));

			V2 offset = {10.0f, 15.0f};
			axisScale *= 2.0f;
			origin += offset;

			//PushBasis2d(&renderer.renderCommands, xAxis, yAxis, origin, basisPoint, axisScale, MakeV4(0.8f, 0.5f, 0.8f, 1.0f));

			axisScale *= 2.0f;
			offset *= 2.0f;
			origin += offset;

			//PushBasis2d(&renderer.renderCommands, xAxis, yAxis, origin, basisPoint, axisScale, MakeV4(0.2f, 0.5f, 0.9f, 1.0f));
		}

		PushEndDraw(&application.renderer->activeTexture, &renderer.renderCommands, &application.window);
		IssueRenderCommands(&application.renderer->activeTexture, &renderer.renderCommands);

		SysWaitForFrame(lastBlitTime);
		lastBlitTime = SysGetMicroseconds();

		u64 debugFrameTimeEnd = SysGetMicroseconds();
		u64 debugFrameTimeDelta = debugFrameTimeEnd - debugFrameTimeBegin;
		debugFrameTimeBegin = debugFrameTimeEnd;
		SysDebugPrintR32((f32)debugFrameTimeDelta/1000.0f);
	}

	quit:

	SysEndTimer();

	return 0;
}
