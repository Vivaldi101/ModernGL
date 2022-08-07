#ifndef TYPES_H
#define TYPES_H
#include <stdint.h>
#include <stdio.h>

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

#if 0
#define FULL_SCREEN
#endif

#ifdef FULL_SCREEN
#define FRAME_BUFFER_WIDTH 1920
#define FRAME_BUFFER_HEIGHT 1080
#else
#define FRAME_BUFFER_WIDTH 800
#define FRAME_BUFFER_HEIGHT 600
#endif // FULL_SCREEN

#define BYTES_PER_PIXEL 4

#define KILOBYTES(Value) ((Value)*1024ULL)
#define MEGABYTES(Value) ((KILOBYTES(Value)*1024ULL))
#define GIGABYTES(Value) ((MEGABYTES(Value)*1024ULL))
#define TERABYTES(Value) ((GIGABYTES(Value)*1024ULL))

#define RED 0x00ff0000
#define GREEN 0x0000ff00
#define BLUE 0x000000ff
#define BLACK 0x00000000

#define MOUSE_LEFT_CLICK 0
#define MOUSE_RIGHT_CLICK 1

//#define unused if(0)
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

typedef float r32;
typedef double r64;

typedef size_t memory_index;


typedef struct FileInfo
{
    void *data;
    u32 size;
} FileInfo;

typedef struct ThreadContext
{
    s32 place_holder;
} ThreadContext;

typedef void (*FreeFilePf)(ThreadContext *, FileInfo *fi);
typedef FileInfo (*ReadFilePf)(ThreadContext *, const char *file_name);
typedef b32 (*writeFilePf)(ThreadContext *, const char *file_name, u32 memory_size, void *memory);

typedef void (*PrintU32Pf)(u32);
typedef void (*PrintS32Pf)(s32);
typedef void (*PrintR32Pf)(r32);
typedef void (*PrintMsgPf)(const char *);
typedef struct PlatformApi
{
    ReadFilePf read_file_pf;
    PrintU32Pf printu32_pf;
    PrintS32Pf prints32_pf;
    PrintR32Pf printr32_pf;
    PrintMsgPf printmsg_pf;
    r32 seconds_to_update;
    b32 is_initialized;
} PlatformApi;

struct RenderBuffer 
{
	byte* data;
	size_t max_size;
	size_t used_size;
};

typedef struct MemoryArena
{
    byte *data;
    memory_index size;
    memory_index used;
    s32 num_elements;
} MemoryArena;

typedef struct TemporaryMemory
{
    MemoryArena *arena;
    memory_index used;
} TemporaryMemory;

typedef struct MemoryPool
{
    memory_index permanent_size;
    void *permanent_data;
    memory_index permanent_used;
    
    memory_index transient_size;
    void *transient_data;
    memory_index transient_used;
    
} MemoryPool;

typedef struct Button
{
    s32 half_trans_count;
    b32 ended_down;
} Button;

typedef struct Input
{
    struct
    {
        Button buttons[3];
        s32 x;
        s32 y;
    } mouse;
    
    struct
    {
        Button move_up;
        Button move_down;
        Button move_left;
        Button move_right;
        Button player_speed_boost;
        Button reset_world;
    } keys;
} Input;

typedef struct BitScan
{
    b32 is_found;
    u32 shift;
} BitScan;

// TODO: Just for dev, remove these!
exported void SysFreeFile(ThreadContext *, FileInfo *fi);
exported FileInfo SysReadFile(ThreadContext *, const char *file_name);
exported b32 SysWriteFile(ThreadContext *, const char *file_name, u32 memory_size, void *memory);


// Exported dll functions
//#define DLL_API_UPDATE_AND_RENDER (ThreadContext *, PlatformApi *, Input *, Renderer *, MemoryPool *)
//exported void DLL_UpdateAndRender DLL_API_UPDATE_AND_RENDER;
#endif   // COMMON_H
