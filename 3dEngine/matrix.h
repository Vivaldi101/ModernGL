#ifndef MATRIX_H
#define MATRIX_H

struct Matrix4
{
    f32 &operator[](s32 index)
    {
        return elements[index];
    }
	float elements[16];
};

#endif
