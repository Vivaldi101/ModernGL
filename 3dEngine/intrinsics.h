#ifndef INTRINSICS_H
#define INTRINSICS_H

#include "math.h"

function u32
RotateLeft(u32 value, u32 amount)
{
    u32 result = _rotl(value, amount);
    return result;
}

function u32
RotateRight(u32 value, u32 amount)
{
    u32 result = _rotr(value, amount);
    
    return result;
}

function s32
RoundReal32ToS32(f32 value)
{
    s32 result;
    f32 add = (value >= 0.0f) ? 0.5f: -0.5f;
    
    result = (s32)(value + add);
    return result;
}

function u32
RoundReal32ToU32(f32 value)
{
    u32 result;
    f32 add = 0.5f;
    
    result = (u32)(value + add);
    return result;
}

function s32
FloorReal32ToS32(f32 value)
{
    s32 result;
    result = (s32)floorf(value);
    
    return result;
}

function u32
PackRGBA(V4 color)
{
    f32 scale =  255.0f;
	u32 result = (RoundReal32ToU32(color.a * scale) << 24 |
				  RoundReal32ToU32(color.r * scale) << 16 |
				  RoundReal32ToU32(color.g * scale) << 8  |
				  RoundReal32ToU32(color.b * scale));
    
    
	return result;
}

function f32
ATan2(f32 y, f32 x)
{
    f32 result = atan2f(y, x);
    
    return result;
}

function f32
Square(f32 a)
{
    f32 result = a*a;
    
    return result;
}

function f32
GetLineRejection(V2 line, V2 v)
{
	f32 result = 0;
	f32 d = Dot(v, line);
	V2 vd = {line.x * d, line.y * d};
	V2 rv = {v.x - vd.x, v.y - vd.y};

	result = Dot(rv, rv);

	assert(result >= 0.0f);
	return result;
}

const char* StrStr(const char* needle, const char* haystack)
{
	assert(needle != 0 && haystack != 0);
	char* result = 0;
	char* ph = (char*)haystack;
	do
	{
		char* pn = (char*)needle;
		do
		{
			int n = *pn;
			int h = *(ph + (pn - needle));
			result = (n == h) ? ph : (char*)0; 
		} while(result && *(++pn));
	} while(!result && *(++ph));

	return result;
}

#undef Min
#undef Max
#undef Abs
#define Min(a, b) (a) < (b) ? (a) : (b)
#define Max(a, b) (a) > (b) ? (a) : (b)
#define Abs(a) (a) < 0 ? -(a+1) : (a)
#define DegreesToRadians(degrees) ((f32)M_PI / 180.0f) * degrees

#endif