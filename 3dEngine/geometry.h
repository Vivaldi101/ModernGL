#ifndef GEOMETRY_H
#define GEOMETRY_H

// TODO: SIMD
typedef struct Plane
{
	f32 a;
	f32 b;
	f32 c;
	f32 d;
} Plane;

typedef struct Vertex
{
	f32 x;
	f32 y;
	f32 z;
	f32 u;
	f32 v;
	V3 normal;
} Vertex;

typedef struct Face
{
	Vertex v0;
	Vertex v1;
	Vertex v2;
	Plane normal;
} Face;

typedef struct Pyramid
{
	Plane p0; 
	Plane p1; 
	Plane p2; 
	Plane p4;
} Pyramid;

typedef struct Point2d
{
	f32 x;
	f32 y;
} Point2d;

typedef struct Point2dU32
{
	u32 x;
	u32 y;
} Point2dU32;

typedef struct Point2dS32
{
	s32 x;
	s32 y;
} Point2dS32;

#endif
