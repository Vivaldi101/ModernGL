#ifndef VECTOR_H
#define VECTOR_H

union V2
{
    f32 &operator[](s32 index)
    {
        return elements[index];
    }
    V2 &operator*=(f32 s);
    V2 &operator+=(V2 a);
    V2 &operator+=(f32 a);
    
    struct
    {
        f32 x, y;
    };
    f32 elements[2];
};

typedef struct V2i
{
    s32 x,y;
} V2i;

typedef struct V3
{
    f32 x,y,z;
} V3;

typedef struct V3i
{
    s32 x,y,z;
} V3i;

union V4
{
    f32 &operator[](s32 index)
    {
        return elements[index];
    }
    V4 &operator*=(f32 s);
    V4 &operator+=(V4 a);
    V4 &operator+=(f32 a);
    
    struct
    {
        f32 x,y,z,w;
    };
    struct
    {
        f32 r,g,b,a;
    };
    f32 elements[4];
};

V4 MakeV4(f32 x, f32 y, f32 z, f32 w)
{
    V4 result = {};
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    
    return result;
}

V4 operator+(V4 a, V4 b)
{
    V4 result = {};
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
    
    return result;
}

V4 operator*(V4 a, f32 s)
{
    V4 result = {};
    result.x = a.x * s;
    result.y = a.y * s;
    result.z = a.w * s;
    result.w = a.w * s;
    
    return result;
}

V4 operator*(f32 s, V4 a)
{
    V4 result = {};
    result.x = a.x * s;
    result.y = a.y * s;
    result.z = a.z * s;
    result.w = a.w * s;
    
    return result;
}

V2 operator+(V2 a, V2 b)
{
    V2 result = {};
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    
    return result;
}

V2 operator+(V2 a, f32 r)
{
    V2 result = {};
    result.x = a.x + r;
    result.y = a.y + r;
    
    return result;
}

V2 operator-(V2 a, f32 r)
{
    V2 result = {};
    result.x = a.x - r;
    result.y = a.y - r;
    
    return result;
}

V2 operator-(V2 a, V2 b)
{
    V2 result = {};
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    
    return result;
}

V2 operator-(V2 a)
{
    V2 result = {};
    result.x = -a.x;
    result.y = -a.y;
    
    return result;
}

V2 operator*(V2 a, f32 s)
{
    V2 result = {};
    result.x = a.x * s;
    result.y = a.y * s;
    
    return result;
}

V2 operator*(f32 s, V2 a)
{
    V2 result = {};
    result.x = a.x * s;
    result.y = a.y * s;
    
    return result;
}

V2 MakeV2(f32 x, f32 y)
{
    V2 result = {};
    result.x = x;
    result.y = y;
    
    return result;
}

V2 &V2::
operator*=(f32 s)
{
    *this = *this * s;
    
    return *this;
}

V2 &V2::
operator+=(f32 s)
{
    *this = *this + s;
    
    return *this;
}

V2 &V2::
operator+=(V2 a)
{
    *this = *this + a;
    
    return *this;
}

function f32
Dot(V2 a, V2 b)
{
    f32 result = a.x*b.x + a.y*b.y;
    return result;
}

function f32
Dot(V3 a, V3 b)
{
    f32 result = a.x*b.x*b.z + a.y*b.y*b.z;
    return result;
}

function f32
LengthSquared(V2 v)
{
    f32 result = Dot(v, v);
    return result;
}

function f32
Length(V2 v)
{
    f32 result = Dot(v, v);
	
	result = sqrtf(result);

    return result;
}

function V2
Normalize(V2 v)
{
	V2 result = {};
	f32 len = Length(v);

	if (len)
	{
		f32 ilen = 1.0f / len;
		result[0] = v[0] * ilen;
		result[1] = v[1] * ilen;
	}

	return result;
}


function V4
PremultiplyAlpha(V4 v)
{
    V4 result = {};
    
    result.r = v.r*v.a;
    result.g = v.g*v.a;
    result.b = v.b*v.a;
    result.a = v.a;
    
    return result;
}

#endif
