#ifndef _COMMON_H
#define _COMMON_H
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#define function static
#define persistent static
#define global static
#define exported extern __declspec(dllexport)

#ifndef COMPILER_MSVC
#define COMPILER_MSVC 0
#endif

#ifndef COMPILER_MSVC
#ifdef _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#endif
#endif

#ifdef COMPILER_MSVC
#include <intrin.h>
#pragma intrinsic(_BitScanForward)
#endif


#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#define BYTES_PER_PIXEL 4

#define KILOBYTES(Value) ((Value)*1024ULL)
#define MEGABYTES(Value) ((KILOBYTES(Value)*1024ULL))
#define GIGABYTES(Value) ((MEGABYTES(Value)*1024ULL))
#define TERABYTES(Value) ((GIGABYTES(Value)*1024ULL))

#define RED 0x00ff0000
#define GREEN 0x0000ff00
#define BLUE 0x000000ff
#define BLACK 0x00000000

#define ArrayCount(arr) (sizeof(arr) / sizeof(arr[0]))
#define fori(n) for(int i = 0; i < (n); i++)
#define forix(n, x) \
for(int i = 0; i < (n); i += (x))

#define forj(n) for(int j = 0; j < (n); j++)
#define forui(n) for(u32 i = 0; i < (n); i++)
#define foruj(n) for(u32 j = 0; j < (n); j++)
#define foriEach(arr) fori(ArrayCount((arr)))
#define forjEach(arr) forj(ArrayCount((arr)))

#define ClearStruct(s) _ClearStruct_(s, sizeof(*s))

#define PushSize(stack, size, type) ((type *)PushSize_(stack, size))  
#define PushStruct(arena, type) ((type *)PushSize_(arena, sizeof(type)))
#define PushArray(arena, type, count) ((type *)PushSize_(arena, (count)*sizeof(type)))

#define Cast(type, var)   ((type)(var))
#define Implies(p, q) \
(!(p) || (q))
#define enum(name) \
enum class name : u32

#define Used if(1)
#define NotUsed if(0)

#ifdef _DEBUG
#define TestBool(t) { b32 _isSuccess_ = (t); Assert(_isSuccess_); }
#define TestPointer(t) { void* _isSuccess_ = (t); Assert(_isSuccess_); }
#define Assert(cond) do { if (!(cond)) DebugBreak(); } while(0)
#define ExitIf(cond) do { if (!(cond)) Com_Quit(); } while(0)
#define StaticAssert(cond, error) \
do { \
    static const char error[(cond)?1:-1];\
} while(0)
#define InvalidCodePath(m) do { MessageBoxA(0, "Invalid code path: " ##m, 0, 0); DebugBreak(); } while(0)
#define CheckMemory(cond) do { if (!(cond)) { MessageBoxA(0, "Out of memory in: " ##__FILE__, 0, 0); DebugBreak(); } } while(0)
#define EventOverflow do { MessageBoxA(0, "Event overflow", 0, 0); Assert(0); } while(0)
#else
#define Assert(e) (e)
#define TestBool(t) (t)
#define TestPointer(t) (t)
#define InvalidCodePath(m) 
#define CheckMemory(cond) 
#define EventOverflow 
#endif

#define OffsetOf(s, m) (size_t)&reinterpret_cast<const volatile char&>((((s *)0)->m))
#define Defer(begin, end) int (_i_); for((_i_) = (begin, 0); !(_i_); ++(_i_), end)
#define Ensure(begin, end) int (_i_); assert(begin); for((_i_) = ((begin), 0); !(_i_); ++(_i_), assert(end))
#define IsZero(x) (!(x))

typedef unsigned char byte;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef uint32_t b32;

typedef float f32;
typedef double f64;

typedef size_t memory_index;

// Similar convention to RECT in WinDef.h (left, top are inclusive -- right, bottom are exclusive)
typedef struct Rectangle2
{
	s32 left;
	s32 top;
	s32 right;
	s32 bottom;
} Rectangle2;

function void
Printf(const char* format, ...)
{
	static char buffer[1024];
	va_list argList;
	va_start(argList, format);
	vsprintf_s(buffer, sizeof(buffer), format, argList);
	va_end(argList);
	OutputDebugString(buffer);
}


#endif   // COMMON_H
