#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#include <GL/gl.h>

// From Martins of HMH community fame
#include "glcorearb.h"  // download from https://www.khronos.org/registry/OpenGL/api/GL/glcorearb.h
#include "wglext.h"     // download from https://www.khronos.org/registry/OpenGL/api/GL/wglext.h
// also download https://www.khronos.org/registry/EGL/api/KHR/khrplatform.h and put in "KHR" folder

typedef BOOL WINAPI SwapInterval(int swapInterval);
global SwapInterval* wglSwapInterval = 0;

// make sure you use functions that are valid for selected GL version (specified when context is created)
#define GL_FUNCTIONS(X) \
	X(PFNGLGENBUFFERSPROC,				 glGenBuffers				) \
    X(PFNGLCREATEBUFFERSPROC,            glCreateBuffers            ) \
    X(PFNGLNAMEDBUFFERSTORAGEPROC,       glNamedBufferStorage       ) \
    X(PFNGLBINDVERTEXARRAYPROC,          glBindVertexArray          ) \
    X(PFNGLCREATEVERTEXARRAYSPROC,       glCreateVertexArrays       ) \
    X(PFNGLVERTEXARRAYATTRIBBINDINGPROC, glVertexArrayAttribBinding ) \
    X(PFNGLVERTEXARRAYVERTEXBUFFERPROC,  glVertexArrayVertexBuffer  ) \
    X(PFNGLVERTEXARRAYATTRIBFORMATPROC,  glVertexArrayAttribFormat  ) \
    X(PFNGLENABLEVERTEXARRAYATTRIBPROC,  glEnableVertexArrayAttrib  ) \
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
    X(PFNGLDEBUGMESSAGECALLBACKPROC,     glDebugMessageCallback     )

#define X(type, name) static type name;
GL_FUNCTIONS(X)
#undef X

// TODO: Store in a global GL state
global GLuint vboId = 0u;
global GLuint iboId = 0u;

function void 
GenerateBufferObjectIds()
{
	Ensure(true, vboId != 0 && iboId != 0)
	{
		glGenBuffers(1, &vboId);
		glGenBuffers(1, &iboId);
	}
}

function void
DrawTextureToRegion(void* textureMemory, Rectangle2 drawRegion)
{
	s32 width = drawRegion.right - drawRegion.left;
	s32 height = drawRegion.top - drawRegion.bottom;

	Defer(width >= 2 && height >= 2, true)
	{
		// Test margin.
		int pixelMargin = 0;
		glViewport(0 + pixelMargin, 0 + pixelMargin, width - pixelMargin*1, height - pixelMargin*1);
		{
			// Texturing.
			// TODO: Refactor.
			persistent b32 initialize = false;
			if(!initialize)
			{
				u32 textureHandle = 0;
				glGenTextures(1, &textureHandle);
				initialize = true;
			}
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, textureMemory);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		}
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		{
			f32 a = 2.0f / (width - 1.0f);
			f32 b = 2.0f / (height - 1.0f);
			f32 projectionMatrix[] =
			{
				a, 0, 0, 0,
				0, b, 0, 0,
				0, 0, 1, 0,
			   -1, -1, 0, 1,
			};
			glLoadMatrixf(projectionMatrix);
		}

		glEnable(GL_TEXTURE_2D);
		glBegin(GL_TRIANGLES);
		{
			f32 left = 0.0f;
			f32 top = 0.0f;

			f32 right = width - 1.0f;
			f32 bottom = height - 1.0f;

			// Upper left triangle.
			//glColor3f(1.0f, 0.0f, 0.0f);
			glTexCoord2f(0.0f, 0.0f);
			glVertex2f(left, top);

			//glColor3f(0.0f, 1.0f, 0.0f);
			glTexCoord2f(0.0f, 1.0f);
			glVertex2f(left, bottom);

			//glColor3f(0.0f, 0.0f, 1.0f);
			glTexCoord2f(1.0f, 0.0f);
			glVertex2f(right, top);

			// Lower right triangle.
			//glColor3f(0.0f, 1.0f, 0.0f);
			glTexCoord2f(0.0f, 1.0f);
			glVertex2f(left, bottom);

			//glColor3f(1.0f, 0.0f, 0.0f);
			glTexCoord2f(1.0f, 1.0f);
			glVertex2f(right, bottom);

			//glColor3f(0.0f, 0.0f, 1.0f);
			glTexCoord2f(1.0f, 0.0f);
			glVertex2f(right, top);
		}
		glEnd();
	}
}

function void 
APIENTRY DebugCallback(
    GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar* message, const void* user)
{
    OutputDebugStringA(message);
    OutputDebugStringA("\n");

    if (severity != GL_DEBUG_SEVERITY_HIGH && severity != GL_DEBUG_SEVERITY_MEDIUM)
	{
		return;
	}

	Assert(severity == GL_DEBUG_SEVERITY_HIGH || severity == GL_DEBUG_SEVERITY_MEDIUM);

	if (IsDebuggerPresent())
	{
		Assert(!"OpenGL error - check the callstack in debugger");
	}

	FatalError("OpenGL API usage error! Use debugger to examine call stack!");
}

function void
InitializeOpenGL(App* application)
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

	// Load OpenGL functions
#define X(type, name) name = (type)wglGetProcAddress(#name); Assert(name);
	GL_FUNCTIONS(X)
#undef X

#ifndef NDEBUG
	// Enable debug callback
	glDebugMessageCallback(&DebugCallback, NULL);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif

	GenerateBufferObjectIds();
}
