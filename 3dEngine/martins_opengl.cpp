// example how to set up OpenGL core context on Windows
// and use basic functionality of OpenGL 4.5 version

// important extension functionality used here:
// (4.3) KHR_debug:                     https://www.khronos.org/registry/OpenGL/extensions/KHR/KHR_debug.txt
// (4.5) ARB_direct_state_access:       https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_direct_state_access.txt
// (4.1) ARB_separate_shader_objects:   https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_separate_shader_objects.txt
// (4.2) ARB_shading_language_420pack:  https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_shading_language_420pack.txt
// (4.3) ARB_explicit_uniform_location: https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_explicit_uniform_location.txt

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <GL/gl.h>
#include "glcorearb.h"  // download from https://www.khronos.org/registry/OpenGL/api/GL/glcorearb.h
#include "wglext.h"     // download from https://www.khronos.org/registry/OpenGL/api/GL/wglext.h
// also download https://www.khronos.org/registry/EGL/api/KHR/khrplatform.h and put in "KHR" folder

#define _USE_MATH_DEFINES
#include <math.h>
#include <stddef.h>
#include <cassert>

// replace this with your favorite Assert() implementation
#include <intrin.h>
#ifdef _DEBUG
#define Assert(cond) do { if (!(cond)) __debugbreak(); } while (0)
#define Implies(a, b) (!(a) || (b))
#define Equals(a, b) Implies((a), (b)) && Implies((b), (a))
#else
#define Assert(cond)
#define Implies(a, b) (!(a) || (b))
#endif

#pragma comment (lib, "gdi32.lib")
#pragma comment (lib, "user32.lib")
#pragma comment (lib, "opengl32.lib")

// make sure you use functions that are valid for selected GL version (specified when context is created)
#define GL_FUNCTIONS(X) \
    X(PFNGLGENBUFFERSPROC,				 glGenBuffers               ) \
    X(PFNGLBINDBUFFERPROC,				 glBindBuffer               ) \
    X(PFNGLCREATEBUFFERSPROC,            glCreateBuffers            ) \
    X(PFNGLNAMEDBUFFERSTORAGEPROC,       glNamedBufferStorage       ) \
    X(PFNGLBUFFERSTORAGEPROC,			 glBufferStorage			) \
    X(PFNGLBINDVERTEXARRAYPROC,          glBindVertexArray          ) \
    X(PFNGLISVERTEXARRAYPROC,			 glIsVertexArray            ) \
    X(PFNGLBINDVERTEXBUFFERPROC,         glBindVertexBuffer         ) \
    X(PFNGLGENVERTEXARRAYSPROC,			 glGenVertexArrays			) \
    X(PFNGLCREATEVERTEXARRAYSPROC,       glCreateVertexArrays       ) \
    X(PFNGLVERTEXARRAYATTRIBBINDINGPROC, glVertexArrayAttribBinding ) \
    X(PFNGLVERTEXATTRIBBINDINGPROC,		 glVertexAttribBinding		) \
    X(PFNGLVERTEXARRAYVERTEXBUFFERPROC,  glVertexArrayVertexBuffer  ) \
    X(PFNGLVERTEXARRAYATTRIBFORMATPROC,  glVertexArrayAttribFormat  ) \
    X(PFNGLVERTEXATTRIBFORMATPROC,		 glVertexAttribFormat		) \
    X(PFNGLENABLEVERTEXARRAYATTRIBPROC,  glEnableVertexArrayAttrib  ) \
    X(PFNGLENABLEVERTEXATTRIBARRAYPROC,	 glEnableVertexAttribArray	) \
    X(PFNGLCREATESHADERPROGRAMVPROC,     glCreateShaderProgramv     ) \
    X(PFNGLGETPROGRAMIVPROC,             glGetProgramiv             ) \
    X(PFNGLGETPROGRAMINFOLOGPROC,        glGetProgramInfoLog        ) \
    X(PFNGLGENPROGRAMPIPELINESPROC,      glGenProgramPipelines      ) \
    X(PFNGLUSEPROGRAMSTAGESPROC,         glUseProgramStages         ) \
    X(PFNGLBINDPROGRAMPIPELINEPROC,      glBindProgramPipeline      ) \
    X(PFNGLPROGRAMUNIFORMMATRIX2FVPROC,  glProgramUniformMatrix2fv  ) \
    X(PFNGLBINDTEXTUREUNITPROC,          glBindTextureUnit          ) \
    X(PFNGLCREATETEXTURESPROC,           glCreateTextures           ) \
    X(PFNGLTEXTUREPARAMETERIPROC,        glTextureParameteri        ) \
    X(PFNGLTEXTURESTORAGE2DPROC,         glTextureStorage2D         ) \
    X(PFNGLTEXTURESUBIMAGE2DPROC,        glTextureSubImage2D        ) \
    X(PFNGLGENFRAMEBUFFERSPROC,			 glGenFramebuffers			) \
    X(PFNGLBINDFRAMEBUFFERPROC,			 glBindFramebuffer			) \
    X(PFNGLFRAMEBUFFERTEXTURE2DPROC,	 glFramebufferTexture2D		) \
    X(PFNGLDRAWBUFFERSPROC,				 glDrawBuffers				) \
    X(PFNGLCHECKFRAMEBUFFERSTATUSPROC,	 glCheckFramebufferStatus   ) \
    X(PFNGLUSEPROGRAMPROC,				 glUseProgram				) \
    X(PFNGLLINKPROGRAMPROC,				 glLinkProgram				) \
    X(PFNGLPROGRAMUNIFORM3FPROC,		 glProgramUniform3f			) \
    X(PFNGLPROGRAMUNIFORM3UIPROC,		 glProgramUniform3ui		) \
    X(PFNGLPROGRAMUNIFORM2UIPROC,		 glProgramUniform2ui		) \
    X(PFNGLPROGRAMUNIFORM1UIPROC,		 glProgramUniform1ui		) \
    X(PFNGLDRAWELEMENTSBASEVERTEXPROC,	 glDrawElementsBaseVertex	) \
    X(PFNGLDEBUGMESSAGECALLBACKPROC,     glDebugMessageCallback     )

#define X(type, name) static type name;
GL_FUNCTIONS(X)
#undef X

#define STR2(x) #x
#define STR(x) STR2(x)

static void PreCondition(...)
{}

static void FatalError(const char* message)
{
    MessageBoxA(NULL, message, "Error", MB_ICONEXCLAMATION);
    ExitProcess(0);
}

#ifndef NDEBUG
static void APIENTRY DebugCallback(
    GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar* message, const void* user)
{
    OutputDebugStringA(message);
    OutputDebugStringA("\n");

    if (severity != GL_DEBUG_SEVERITY_HIGH && severity != GL_DEBUG_SEVERITY_MEDIUM)
		return;

	if (severity == GL_DEBUG_SEVERITY_MEDIUM)
		return;

	if (IsDebuggerPresent())
	{
		Assert(!"OpenGL error - check the callstack in debugger");
	}

	FatalError("OpenGL API usage error! Use debugger to examine call stack!");
}
#endif

static LRESULT CALLBACK WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(wnd, msg, wparam, lparam);
}

struct Position
{
	int x, y;
};

Position GetCursorWindowPosition(HWND window, int windowWidth, int windowHeight)
{
	Position result = {};

	POINT cursorPoint = {};
	GetCursorPos(&cursorPoint);

	ScreenToClient(window, &cursorPoint);

	result.x = cursorPoint.x;
	result.y = cursorPoint.y;

	return result;
}

// compares src string with dstlen characters from dst, returns 1 if they are equal, 0 if not
static int StringsAreEqual(const char* src, const char* dst, size_t dstlen)
{
	PreCondition(src, dst, "notnull");
	PreCondition(dstlen, 100);

	while (*src && dstlen-- && *dst)
    {
        if (*src++ != *dst++)
        {
            return 0;
        }
    }

    return (dstlen && *src == *dst) || (!dstlen && *src == 0);
}

static PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = NULL;
static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = NULL;
static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = NULL;

static void GetWglFunctions(void)
{
    // to get WGL functions we need valid GL context, so create dummy window for dummy GL contetx
    HWND dummy = CreateWindowExW(
        0, L"STATIC", L"DummyWindow", WS_OVERLAPPED,
        100, 100, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, NULL, NULL);
    Assert(dummy && "Failed to create dummy window");

    HDC dc = GetDC(dummy);
    Assert(dc && "Failed to get device context for dummy window");

	PIXELFORMATDESCRIPTOR desc = {};
	desc.nSize = sizeof(desc);
	desc.nVersion = 1;
	desc.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	desc.iPixelType = PFD_TYPE_RGBA;
	desc.cColorBits = 24;

    int format = ChoosePixelFormat(dc, &desc);
    if (!format)
    {
        FatalError("Cannot choose OpenGL pixel format for dummy window!");
    }

    int ok = DescribePixelFormat(dc, format, sizeof(desc), &desc);
    Assert(ok && "Failed to describe OpenGL pixel format");

    // reason to create dummy window is that SetPixelFormat can be called only once for the window
    if (!SetPixelFormat(dc, format, &desc))
    {
        FatalError("Cannot set OpenGL pixel format for dummy window!");
    }

    HGLRC rc = wglCreateContext(dc);
    Assert(rc && "Failed to create OpenGL context for dummy window");

    ok = wglMakeCurrent(dc, rc);
    Assert(ok && "Failed to make current OpenGL context for dummy window");

    // https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_extensions_string.txt
    PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB =
        (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
    if (!wglGetExtensionsStringARB)
    {
        FatalError("OpenGL does not support WGL_ARB_extensions_string extension!");
    }

    const char* ext = wglGetExtensionsStringARB(dc);
    Assert(ext && "Failed to get OpenGL WGL extension string");

    const char* start = ext;
    for (;;)
    {
        while (*ext != 0 && *ext != ' ')
        {
            ext++;
        }

        if (*ext == 0)
        {
            break;
        }

        size_t length = ext - start;
        if (StringsAreEqual("WGL_ARB_pixel_format", start, length))
        {
            // https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_pixel_format.txt
            wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
        }
        else if (StringsAreEqual("WGL_ARB_create_context", start, length))
        {
            // https://www.khronos.org/registry/OpenGL/extensions/ARB/WGL_ARB_create_context.txt
            wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
        }
        else if (StringsAreEqual("WGL_EXT_swap_control", start, length))
        {
            // https://www.khronos.org/registry/OpenGL/extensions/EXT/WGL_EXT_swap_control.txt
            wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
        }

        ext++;
        start = ext;
    }

    if (!wglChoosePixelFormatARB || !wglCreateContextAttribsARB || !wglSwapIntervalEXT)
    {
        FatalError("OpenGL does not support required WGL extensions for modern context!");
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(rc);
    ReleaseDC(dummy, dc);
    DestroyWindow(dummy);
}

struct ShaderContext
{
	GLuint textureBinding;
	GLuint rttBinding;
	int width, height, bpp;
	const void* texture;
};

void loadTexture(ShaderContext* context)
{
	assert(context->width);
	assert(context->height);
	assert(context->texture);

	glCreateTextures(GL_TEXTURE_2D, 1, &context->textureBinding);
	glTextureParameteri(context->textureBinding, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(context->textureBinding, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(context->textureBinding, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteri(context->textureBinding, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTextureStorage2D(context->textureBinding, 1, GL_RGBA8, context->width, context->height);
	glTextureSubImage2D(context->textureBinding, 0, 0, 0, context->width, context->height, GL_RGBA, GL_UNSIGNED_BYTE, context->texture);

	context->texture = NULL;
}

void loadDefaultTexture(ShaderContext* context)
{
	const unsigned int pixels[] =
	{
		0xff00ff00, 0xff000000,
		0xff000000, 0xff00ff00,
	};
	context->width = context->height = 2;
	context->bpp = 4;
	context->texture = pixels;

	loadTexture(context);
}

struct vec3
{
	GLfloat x, y, z;
};

// encode an unique ID into a colour with components in range of 0.0 to 1.0
vec3 encodeID(int id) 
{
	int r = id / 65536;
	int g = (id - r * 65536) / 256;
	int b = (id - r * 65536 - g * 256);

	vec3 result = {(float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f};

	// convert to floats. only divide by 255, because range is 0-255
	return result;
}

int decodeID(int r, int g, int b) 
{
	return b + g * 256 + r * 256 * 256;
}

struct PixelBufferData
{
	unsigned int objectID;
	unsigned int drawID;
	unsigned int primitiveID;
	//unsigned char R;
	//unsigned char G;
	//unsigned char B;
	//unsigned char A;
};

void drawToTexture(int width, int height, GLuint frameBuffer) 
{
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

	// Clear frame buffer texture
	glViewport(0, 0, width, height);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw quad into it
	glDrawArrays(GL_TRIANGLES, 0, 6);

	// Restore default frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

PixelBufferData readFromTexture(int x, int y, int width, int height, GLuint frameBuffer)
{
	PixelBufferData result = {};

	// Read from frame buffer 
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glViewport(0, 0, width, height);

	// Read from color texture
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glFlush();
	glFinish();

	glPixelStorei(GL_PACK_ALIGNMENT, 4);

	glReadPixels(x, height - y, 1, 1, GL_RGB_INTEGER, GL_UNSIGNED_INT, &result);

	// Restore default frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return result;
}

void drawPrimitive(int primitiveID)
{
	glDrawArrays(GL_TRIANGLES, primitiveID * 3, 3);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previnstance, LPSTR cmdline, int cmdshow)
{
	// get WGL functions to be able to create modern GL context
    GetWglFunctions();

    // register window class to have custom WindowProc callback
	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = instance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = L"opengl_window_class";
    ATOM atom = RegisterClassExW(&wc);
    Assert(atom && "Failed to register window class");

    // window properties - width, height and style
    int width = CW_USEDEFAULT;
    int height = CW_USEDEFAULT;
    DWORD exstyle = WS_EX_APPWINDOW;
    DWORD style = WS_OVERLAPPEDWINDOW;

    // uncomment in case you want fixed size window
    style &= ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;
    RECT rect = { 0, 0, 1920, 1080 };
    AdjustWindowRectEx(&rect, style, FALSE, exstyle);
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;

    // create window
    HWND window = CreateWindowExW(
        exstyle, wc.lpszClassName, L"OpenGL Window", style,
        0, 0, width, height,
        NULL, NULL, wc.hInstance, NULL);
    Assert(window && "Failed to create window");

	RECT clientRectangle = {};
	GetClientRect(window, &clientRectangle);

	width = clientRectangle.right - clientRectangle.left;
	height = clientRectangle.bottom - clientRectangle.top;

	Assert(width > 0 && width <= (rect.right - rect.left));
	Assert(height > 0 && height <= (rect.bottom - rect.top));

    HDC dc = GetDC(window);
    Assert(dc && "Failed to window device context");

    // set pixel format for OpenGL context
    {
        int attrib[] =
        {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
            WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB,     24,
            WGL_DEPTH_BITS_ARB,     24,
            WGL_STENCIL_BITS_ARB,   8,

            // uncomment for sRGB framebuffer, from WGL_ARB_framebuffer_sRGB extension
            // https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_framebuffer_sRGB.txt
            //WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,

            // uncomment for multisampeld framebuffer, from WGL_ARB_multisample extension
            // https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_multisample.txt
            //WGL_SAMPLE_BUFFERS_ARB, 1,
            //WGL_SAMPLES_ARB,        4, // 4x MSAA

            0,
        };

        int format;
        UINT formats;
        if (!wglChoosePixelFormatARB(dc, attrib, NULL, 1, &format, &formats) || formats == 0)
        {
            FatalError("OpenGL does not support required pixel format!");
        }

		PIXELFORMATDESCRIPTOR desc = {};
        desc.nSize = sizeof(desc);
        int ok = DescribePixelFormat(dc, format, sizeof(desc), &desc);
        Assert(ok && "Failed to describe OpenGL pixel format");

        if (!SetPixelFormat(dc, format, &desc))
        {
            FatalError("Cannot set OpenGL selected pixel format!");
        }
    }

    // create modern OpenGL context
    {
        int attrib[] =
        {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 5,
            WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#ifndef NDEBUG
            // ask for debug context for non "Release" builds
            // this is so we can enable debug callback
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
            0,
        };

        HGLRC rc = wglCreateContextAttribsARB(dc, NULL, attrib);
        if (!rc)
        {
            FatalError("Cannot create modern OpenGL context! OpenGL version 4.5 not supported?");
        }

        BOOL ok = wglMakeCurrent(dc, rc);
        Assert(ok && "Failed to make current OpenGL context");

        // load OpenGL functions
#define X(type, name) name = (type)wglGetProcAddress(#name); Assert(name);
        GL_FUNCTIONS(X)
#undef X

#ifndef NDEBUG
        // enable debug callback
        glDebugMessageCallback(&DebugCallback, NULL);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif
    }

    struct Vertex
    {
        float position[2];
        float uv[2];
        float color[3];
    };

    // the current vertex attribute object for this program

    GLuint triangleAttributeObject;
	glGenVertexArrays(1, &triangleAttributeObject);

	glBindVertexArray(triangleAttributeObject);

	Assert(glIsVertexArray(triangleAttributeObject));

    // vertex buffer
    GLuint vbo;
    {
        struct Vertex verts[] =
        {
			// position in clip-space 
			// uv				
			// color
            //{ { -0.00f, +0.75f }, { 25.0f, 50.0f }, { 1, 0, 0 } },
            //{ { +0.75f, -0.50f }, {  0.0f,  0.0f }, { 0, 1, 0 } },
            //{ { -0.75f, -0.50f }, { 50.0f,  0.0f }, { 0, 0, 1 } },

			// upper-left
            { { -1.00f, -1.0f },	{ 0.0f*8.0f, 0.0f*8.0f	}, { 1, 0, 0 } },
            { { +1.00f, +1.00f },	{ 1.0f*8.0f, 1.0f*8.0f	}, { 0, 1, 0 } },
            { { -1.00f, +1.00f },	{ 0.0f*8.0f, 1.0f*8.0f	}, { 0, 0, 1 } },

			// lower-right
            { { -1.00f, -1.0f },	{ 0.0f*8.0f, 0.0f*8.0f	}, { 1, 0, 0 } },
            { { +1.00f, -1.00f },	{ 1.0f*8.0f, 0.0f*8.0f	}, { 0, 1, 0 } },
            { { +1.00f, +1.00f },	{ 1.0f*8.0f, 1.0f*8.0f	}, { 0, 0, 1 } },
        };

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferStorage(GL_ARRAY_BUFFER, sizeof(verts), verts, 0);
    }

    // checkerboard texture, with 50% transparency on black colors
	ShaderContext context = {};
    {
        loadDefaultTexture(&context);
    }

	// render to texture
	GLuint rttFramebuffer = 0;
	GLuint rttPipeline = 0;
    GLuint rttVShader, rttFShader;
	{
		glGenFramebuffers(1, &rttFramebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, rttFramebuffer);	

		// create texture to use for rendering second pass
		GLuint rtt = 0;
		glGenTextures(1, &rtt);
		glBindTexture(GL_TEXTURE_2D, rtt);

		context.rttBinding = rtt;

		// make the texture the same size as the viewport
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32UI, width, height, 0, GL_RGB_INTEGER, GL_UNSIGNED_INT, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// attach colour texture to fb
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rtt, 0);

		glBindTexture(GL_TEXTURE_2D, 0);

		// redirect fragment shader output 0 used to the texture that we just bound
		GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, drawBuffers);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			FatalError("Incomplete framebuffer status!");
		}

		// bind default fb (number 0) so that we render normally next time
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		{
			const char* glsl_vshader = 
				"#version 450 core                             \n"
				"#line " STR(__LINE__) "                     \n\n" // actual line number in this file for nicer error messages
				"                                              \n"
				"layout (location=0) in vec2 a_pos;            \n" // position attribute index 0
				"layout (location=0)                           \n" // (from ARB_explicit_uniform_location)
				"uniform mat2 u_matrix;                        \n" // matrix uniform location 0
				"out gl_PerVertex { vec4 gl_Position; };       \n" // required because of ARB_separate_shader_objects
				"void main()                                   \n"
				"{                                             \n"
				"    vec2 pos = u_matrix * a_pos;              \n"
				"    gl_Position = vec4(pos, 0, 1);            \n"
				"}                                             \n";

			const char* glsl_fshader =
				"#version 450 core                             \n"
				"#line " STR(__LINE__) "                     \n\n" // actual line number in this file for nicer error messages
				"layout (location=0)                           \n"
				"out uvec3 o_color;                             \n" // output fragment data location 0
				"layout (location=0)                           \n"
				"uniform uint objectID;                        \n" 
				"layout (location=1)                           \n"
				"uniform uint drawID;                        \n" 
				"                                              \n"
				"void main()                                   \n"
				"{                                             \n"
				"	 if (objectID != 0)										\n"
				"    {											\n"
				"		o_color = uvec3(objectID, drawID, gl_PrimitiveID); \n"
				"	 }											\n"
				"	 else										\n"
				"	 {											\n"
				"		o_color = uvec3(0);						\n"
				"	 }											\n"
				"}                                             \n";

			rttVShader = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &glsl_vshader);
			rttFShader = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &glsl_fshader);

			GLint linked;
			glGetProgramiv(rttVShader, GL_LINK_STATUS, &linked);
			if (!linked)
			{
				char message[1024];
				glGetProgramInfoLog(rttVShader, sizeof(message), NULL, message);
				OutputDebugStringA(message);
				Assert(!"Failed to create vertex shader!");
			}

			glGetProgramiv(rttFShader, GL_LINK_STATUS, &linked);
			if (!linked)
			{
				char message[1024];
				glGetProgramInfoLog(rttFShader, sizeof(message), NULL, message);
				OutputDebugStringA(message);
				Assert(!"Failed to create fragment shader!");
			}

			glGenProgramPipelines(1, &rttPipeline);
			glUseProgramStages(rttPipeline, GL_VERTEX_SHADER_BIT, rttVShader);
			glUseProgramStages(rttPipeline, GL_FRAGMENT_SHADER_BIT, rttFShader);
		}
	}

    // bind and enable vertex buffers to the bind vao
    {
        GLint vbuf_index = 0;
        glBindVertexBuffer(vbuf_index, vbo, 0, sizeof(struct Vertex));

        GLint a_pos = 0;
        glVertexAttribFormat(a_pos, 2, GL_FLOAT, GL_FALSE, offsetof(struct Vertex, position));
        glVertexAttribBinding(a_pos, vbuf_index);
        glEnableVertexAttribArray(a_pos);

        GLint a_uv = 1;
        glVertexAttribFormat(a_uv, 2, GL_FLOAT, GL_FALSE, offsetof(struct Vertex, uv));
        glVertexAttribBinding(a_uv, vbuf_index);
        glEnableVertexAttribArray(a_uv);

        GLint a_color = 2;
        glVertexAttribFormat(a_color, 3, GL_FLOAT, GL_FALSE, offsetof(struct Vertex, color));
        glVertexAttribBinding(a_color, vbuf_index);
        glEnableVertexAttribArray(a_color);
    }

    // fragment & vertex shaders for drawing triangle
    GLuint trianglePipeline, vshader, fshader;
    {
        const char* glsl_vshader =
            "#version 450 core                             \n"
            "#line " STR(__LINE__) "                     \n\n" // actual line number in this file for nicer error messages
            "                                              \n"
            "layout (location=0) in vec2 a_pos;            \n" // position attribute index 0
            "layout (location=1) in vec2 a_uv;             \n" // uv attribute index 1
            "layout (location=2) in vec3 a_color;          \n" // color attribute index 2
            "                                              \n"
            "layout (location=0)                           \n" // (from ARB_explicit_uniform_location)
            "uniform mat2 u_matrix;                        \n" // matrix uniform location 0
            "                                              \n"
            "out gl_PerVertex { vec4 gl_Position; };       \n" // required because of ARB_separate_shader_objects
            "out vec2 uv;                                  \n"
            "out vec4 color;                               \n"
            "                                              \n"
            "void main()                                   \n"
            "{                                             \n"
            "    vec2 pos = u_matrix * a_pos;              \n"
            "    gl_Position = vec4(pos, 0, 1);            \n"
            "    uv = a_uv;                                \n"
            "    color = vec4(a_color, 1);                 \n"
            "}                                             \n"
        ;

        const char* glsl_fshader =
            "#version 450 core                             \n"
            "#line " STR(__LINE__) "                     \n\n" // actual line number in this file for nicer error messages
            "                                              \n"
            "in vec2 uv;                                   \n"
            "in vec4 color;                                \n"
            "                                              \n"
            "layout (binding=0)                            \n" // (from ARB_shading_language_420pack)
            "uniform sampler2D s_texture;                  \n" // texture unit binding 0

            "layout (location=0)                           \n"
            "out vec4 o_color;                             \n" // output fragment data location 0
            "                                              \n"
            "void main()                                   \n"
            "{                                             \n"
			"	 o_color = texture(s_texture, uv); \n"
            "}                                             \n"
        ;

        vshader = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &glsl_vshader);
        fshader = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &glsl_fshader);

        GLint linked;
        glGetProgramiv(vshader, GL_LINK_STATUS, &linked);
        if (!linked)
        {
            char message[1024];
            glGetProgramInfoLog(vshader, sizeof(message), NULL, message);
            OutputDebugStringA(message);
            Assert(!"Failed to create vertex shader!");
        }

        glGetProgramiv(fshader, GL_LINK_STATUS, &linked);
        if (!linked)
        {
            char message[1024];
            glGetProgramInfoLog(fshader, sizeof(message), NULL, message);
            OutputDebugStringA(message);
            Assert(!"Failed to create fragment shader!");
        }

        glGenProgramPipelines(1, &trianglePipeline);
        glUseProgramStages(trianglePipeline, GL_VERTEX_SHADER_BIT, vshader);
        glUseProgramStages(trianglePipeline, GL_FRAGMENT_SHADER_BIT, fshader);
    }

    // fragment & vertex shaders for drawing picked triangle
    GLuint pickedPipeline, pickedVS, pickedFS;
	{        
			const char* glsl_vshader = 
				"#version 450 core                             \n"
				"#line " STR(__LINE__) "                     \n\n" // actual line number in this file for nicer error messages
				"                                              \n"
				"layout (location=0) in vec2 a_pos;            \n" // position attribute index 0
				"layout (location=0)                           \n" // (from ARB_explicit_uniform_location)
				"uniform mat2 u_matrix;                        \n" // matrix uniform location 0
				"out gl_PerVertex { vec4 gl_Position; };       \n" // required because of ARB_separate_shader_objects
				"void main()                                   \n"
				"{                                             \n"
				"    vec2 pos = u_matrix * a_pos;              \n"
				"    gl_Position = vec4(pos, 0, 1);            \n"
				"}                                             \n";

			const char* glsl_fshader =
				"#version 450 core                             \n"
				"#line " STR(__LINE__) "                     \n\n" // actual line number in this file for nicer error messages
				"layout (location=0)                           \n"
				"out vec4 o_color;                             \n" // output fragment data location 0
				"void main()                                   \n"
				"{                                             \n"
				"    o_color = vec4(0.0f, 1.0f, 0.0f, 1.0f); \n"
				"}                                             \n";

		pickedVS = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &glsl_vshader);
		pickedFS = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &glsl_fshader);

		GLint linked;
		glGetProgramiv(pickedVS, GL_LINK_STATUS, &linked);
		if (!linked)
		{
			char message[1024];
			glGetProgramInfoLog(pickedVS, sizeof(message), NULL, message);
			OutputDebugStringA(message);
			Assert(!"Failed to create vertex shader!");
		}

		glGetProgramiv(pickedFS, GL_LINK_STATUS, &linked);
		if (!linked)
		{
			char message[1024];
			glGetProgramInfoLog(pickedFS, sizeof(message), NULL, message);
			OutputDebugStringA(message);
			Assert(!"Failed to create fragment shader!");
		}

		glGenProgramPipelines(1, &pickedPipeline);
		glUseProgramStages(pickedPipeline, GL_VERTEX_SHADER_BIT, pickedVS);
		glUseProgramStages(pickedPipeline, GL_FRAGMENT_SHADER_BIT, pickedFS);
	}

    // setup global GL state
    {
        // enable alpha blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // disble depth testing
        glDisable(GL_DEPTH_TEST);

        // disable culling
        glDisable(GL_CULL_FACE);
    }

    // set to FALSE to disable vsync
    BOOL vsync = TRUE;
    wglSwapIntervalEXT(vsync ? 1 : 0);

    // show the window
    ShowWindow(window, SW_SHOWDEFAULT);

    LARGE_INTEGER freq, c1;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&c1);

    float angle = 0;

    for (;;)
    {
        // process all incoming Windows messages
        MSG msg;
        if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            continue;
        }

        LARGE_INTEGER c2;
        QueryPerformanceCounter(&c2);
        float delta = (float)((double)(c2.QuadPart - c1.QuadPart) / freq.QuadPart);
        c1 = c2;

        // render only if window size is non-zero
        if (width != 0 && height != 0)
        {
			// setup output size covering all client area of window
			glViewport(0, 0, width, height);

			// clear screen
			glClearColor(0.85f, 0.85f, 0.85f, 1.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			// setup rotation matrix in uniform
			{
				angle += delta * 1.0f * (float)M_PI / 20.0f; // full rotation in 20 seconds
				angle = fmodf(angle, 2.0f * (float)M_PI);

				angle = 0;

				float aspect = (float)height / width;
				float matrix[] =
				{
					cosf(angle) * aspect, -sinf(angle),
					sinf(angle) * aspect,  cosf(angle),
				};

				GLint u_matrix = 0;
				glProgramUniformMatrix2fv(vshader, u_matrix, 1, GL_FALSE, matrix);
				glProgramUniformMatrix2fv(rttVShader, u_matrix, 1, GL_FALSE, matrix);
				glProgramUniformMatrix2fv(pickedVS, u_matrix, 1, GL_FALSE, matrix);
			}

			const Position cursorPos = GetCursorWindowPosition(window, width, height);

			glBindProgramPipeline(rttPipeline);

			const GLint objectUniform = 0;
			const GLint drawUniform = 1;

			if (cursorPos.x < 0 || cursorPos.x >= width)
			{
				glProgramUniform1ui(rttFShader, objectUniform, 0);
				glProgramUniform1ui(rttFShader, drawUniform, 0);
			}
			else
			{
				glProgramUniform1ui(rttFShader, objectUniform, 1);
				glProgramUniform1ui(rttFShader, drawUniform, 1);
			}

			drawToTexture(width, height, rttFramebuffer);
			PixelBufferData pixels = readFromTexture(cursorPos.x, cursorPos.y, width, height, rttFramebuffer);

			//glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// draw regular quad
			glBindProgramPipeline(trianglePipeline);

			// bind texture to texture unit
			const GLint s_texture = 0; // texture unit that sampler2D will use in GLSL code
			glBindTextureUnit(s_texture, context.textureBinding);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			if (pixels.objectID != 0)
			{
				// draw selected primitive
				glBindProgramPipeline(pickedPipeline);
				drawPrimitive(pixels.primitiveID);
			}

			// swap the buffers to show output
			if (!SwapBuffers(dc))
			{
				FatalError("Failed to swap OpenGL buffers!");
			}
        }
        else
        {
            // window is minimized, cannot vsync - instead sleep a bit
            if (vsync)
            {
                Sleep(10);
            }
        }
    }
}
