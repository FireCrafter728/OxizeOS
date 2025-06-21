#pragma once

class Vector2f
{
public:
    Vector2f() {};
    Vector2f(float x, float y) : x(x), y(y) {}

    float x, y;
};

class Vector2i
{
public:
    Vector2i() {};
    Vector2i(int x, int y) : x(x), y(y) {}

    int x, y;
};

class Vector3f
{
public:
    Vector3f() {};
    Vector3f(float x, float y, float z) : x(x), y(y), z(z) {}

    float x, y, z;
};

class Vector3i
{
public:
    Vector3i() {};
    Vector3i(int x, int y, int z) : x(x), y(y), z(z) {}

    int x, y, z;
};

class Vector4f
{
public:
    Vector4f() {};
    Vector4f(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    float x, y, z, w;
};

class Vector4i
{
public:
    Vector4i() {};
    Vector4i(int x, int y, int z, int w) : x(x), y(y), z(z), w(w) {}

    int x, y, z, w;
};
