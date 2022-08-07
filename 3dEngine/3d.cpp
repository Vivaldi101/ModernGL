#ifndef _3d_H
#define _3d_H

#include "vector.h"
#include "matrix.h"
#include "geometry.h"


///////////////////////////////////////////////////////////////////////////////
// return a perspective frustum with 6 params similar to glFrustum()
// (left, right, bottom, top, near, far)
///////////////////////////////////////////////////////////////////////////////
function
Matrix4 setFrustum(float l, float r, float b, float t, float n, float f)
{
    Matrix4 matrix;

    matrix[0]  =  2 * n / (r - l);
    matrix[5]  =  2 * n / (t - b);
    matrix[8]  =  (r + l) / (r - l);
    matrix[9]  =  (t + b) / (t - b);
    matrix[10] = -(f + n) / (f - n);
    matrix[11] = -1;
    matrix[14] = -(2 * f * n) / (f - n);
    matrix[15] =  0;

    return matrix;
}

///////////////////////////////////////////////////////////////////////////////
// return a symmetric perspective frustum with 4 params similar to
// gluPerspective() (vertical field of view, aspect ratio, near, far)
///////////////////////////////////////////////////////////////////////////////
function
Matrix4 setFrustum(float fovY, float aspectRatio, float front, float back)
{
    float tangent = tanf(DegreesToRadians(fovY / 2));   // tangent of half fovY
    float height = front * tangent;           // half height of near plane
    float width = height * aspectRatio;       // half width of near plane

    // params: left, right, bottom, top, near, far
    return setFrustum(-width, width, -height, height, front, back);
}

#if 0
function Vec2
MakeVec2(Point2d p0, Point2d p1)
{
	Vec2 result = {0};
	result.x = p1.x - p0.x;
	result.y = p1.y - p0.y;

	return result;
}

function f32
Dot2(Vec2 a, Vec2 b)
{
	f32 result = 0;
	result = a.x * b.x + a.y * b.y;

	return result;
}

function f32 
Length2(Vec2 v)
{
	f32 result = sqrtf(Dot2(v, v));

	return result;
}

function Vec2 
Normalize2(Vec2 v)
{
	Vec2 result = {0};
	f32 length = Length2(v);

	result.x = v.x / length;
	result.y = v.y / length;

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
RoundReal32ToS32(f32 value)
{
    s32 result;
    f32 add = (value >= 0.0f) ? 0.5f: -0.5f;
    
    result = (s32)(value + add);
    return result;
}
#endif

#if 0
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
Min(f32 a, f32 b)
{
	return (a < b) ? a : b;
}

#if 1
// TODO: Optimize :)
function void
ClipToDimensions(Point2d* p0, Point2d* p1, s32 width, s32 height)
{
	if(p0->x < 0)
	{
		p0->x = 0;
	}
	if(p1->x < 0)
	{
		p1->x = 0;
	}
	if(p0->y < 0)
	{
		p0->y = 0;
	}
	if(p1->y < 0)
	{
		p1->y = 0;
	}
	if(p0->x > width)
	{
		p0->x = (f32)width;
	}
	if(p1->x > width)
	{
		p1->x = (f32)width;
	}
	if(p0->y > height)
	{
		p0->y = (f32)height;
	}
	if(p1->y > height)
	{
		p1->y = (f32)height;
	}
}
#endif

static void
PutPixel(u32 x, u32 y, u32 packedColor, Bitmap* texture)
{
	byte *pixels = (byte *)texture->memory + y*texture->pitch + x*BYTES_PER_PIXEL;
	u32 *pixel = (u32 *)pixels;
	*pixel = packedColor;
}

function void 
DrawLine(Bitmap* texture, f32 xa, f32 ya, f32 xb, f32 yb, V4 color)
{
    float len, stepx, stepy, x, y;
    int i, xlen, ylen;

	Point2d p0 = {xa, ya};
	Point2d p1 = {xb, yb};
	u32 packedColor = PackRGBA(color);

	//ClipToDimensions(&p0, &p1, texture->width, texture->height);

    x = (float)xa;
    y = (float)ya;
    xlen = xa - xb;
    ylen = ya - yb;
    len = (float)fabs((float)((fabs((float)ylen) > fabs((float)xlen))? ylen : xlen));
    stepx = xlen / len;
    stepy = ylen / len;

    for(i = 0; i < (int)len; i ++)
	{
		PutPixel(x, y, packedColor, texture);
        x -= stepx;
        y -= stepy;
    }
}
#endif

#if 0
// In screen coordinates.
function void
DrawLineIntoTexture(Bitmap* texture, f32 x0, f32 y0, f32 x1, f32 y1, V4 color)
{
	Point2d p0 = {x0, y0};
	Point2d p1 = {x1, y1};

	ClipToDimensions(&p0, &p1, (f32)texture->width, (f32)texture->height);

	f32 aspectRatio = (f32)texture->width / texture->height;
	Vec2 vn = Normalize2(MakeVec2(p0, p1));
	u32 packedColor = PackRGBA(color);

	f32 slope = (p1.y - p0.y) / (p1.x - p0.x);

	u32 pitch = texture->pitch;
	s32 x = (s32)p0.x; 
	s32 y = (s32)p0.y;
	assert(p1.x >= p0.x); // Right going lines for now.
	assert(0.0f <= slope && slope <= 1.0f); // 1st quadrant.
	//assert(0.0f <= slope && slope <= 1.0f); // 2st quadrant.
	while(x < (s32)p1.x)
	{
		assert(x >= 0 && y >= 0);

		PutPixel((s32)x, (s32)y, packedColor, texture);

		x++;
		Vec2 vne = {((x + 0.5f) - (p0.x + 0.5f)), ((y + 0.5f) - (p0.y + 0.5f)) + 1.0f};
		Vec2 vm = {((x + 0.5f) - (p0.x + 0.5f)), ((y + 0.5f) - (p0.y + 0.5f))};
		Vec2 vse = {((x + 0.5f) - (p0.x + 0.5f)), ((y + 0.5f) - (p0.y + 0.5f)) - 1.0f};

		f32 rne = GetLineRejection(vn, vne);
		f32 rm = GetLineRejection(vn, vm);
		f32 rse = GetLineRejection(vn, vse);

		assert(vne.y > vm.y);
		assert(vm.y > vse.y);
		assert(rne >= 0 && rse >= 0);

		f32 min = Min(Min(rne, rse), rm);
		if(min == rne)
		{
			y++;
		}
		else if(min == rse)
		{
			y--;
		}
	}
}


#define SUB_PIXEL_STEP 18
#define SUB_PIXEL_POW2 (1<<SUB_PIXEL_STEP)

#define FIXP_DIV(x, y) (((s64)(x) << SUB_PIXEL_STEP) / (y))
#define SCALE_DOWN(x) (((x) + (SUB_PIXEL_POW2 >> 1)) >> SUB_PIXEL_STEP)

function void 
DrawLineFlipCode(Bitmap* texture, f32 x1, f32 y1, f32 x2, f32 y2, V4 color)
{  
	Point2d p1 = {x1, y1};
	Point2d p2 = {x2, y2};

	ClipToDimensions(&p1, &p2, texture->width, texture->height);

	// TODO: Derive overflow conditions.
	s32 deltaXFixedP = (s32)(SUB_PIXEL_POW2*(p2.x - p1.x));
	s32 deltaYFixedP = (s32)(SUB_PIXEL_POW2*(p2.y - p1.y));
	u32 hl = abs(deltaXFixedP);
	u32 vl = abs(deltaYFixedP);

	u32 length = (hl >= vl) ? hl : vl;
	u32 deltaXFixedPDivided = FIXP_DIV(deltaXFixedP, length);
	u32 deltaYFixedPDivided = FIXP_DIV(deltaYFixedP, length);
	u32 packedColor = PackRGBA(color);

	u32 x = (u32)(SUB_PIXEL_POW2*p1.x);
	u32 y = (u32)(SUB_PIXEL_POW2*p1.y);
	for (u32 i = 0; i < SCALE_DOWN(length); i++)
	{  
		PutPixel(SCALE_DOWN(x), SCALE_DOWN(y), packedColor, texture);
		x += deltaXFixedPDivided;
		y += deltaYFixedPDivided;
	}
}

#endif
#endif

