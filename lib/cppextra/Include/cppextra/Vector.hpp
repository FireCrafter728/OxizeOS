#pragma once

#include <functional>

template <typename T>
class Vector2
{
public:
    constexpr Vector2() noexcept = default;
    constexpr Vector2(T x, T y) noexcept : x(x), y(y) {}
    constexpr Vector2(const Vector2 &) noexcept = default;
    constexpr Vector2(Vector2 &&) noexcept = default;

    constexpr Vector2 &operator=(const Vector2 &) noexcept = default;
    constexpr Vector2 &operator=(Vector2 &&) noexcept = default;

    // Addition, subtraction (vector only)
    inline constexpr Vector2 operator+(const Vector2 &rhs) const noexcept
    {
        return {x + rhs.x, y + rhs.y};
    }

    inline constexpr Vector2 operator-(const Vector2 &rhs) const noexcept
    {
        return {x - rhs.x, y - rhs.y};
    }

    // Scalar multiplication and division
    inline constexpr Vector2 operator*(T scalar) const noexcept
    {
        return {x * scalar, y * scalar};
    }

    inline constexpr Vector2 operator/(T scalar) const noexcept
    {
        return {x / scalar, y / scalar};
    }

    // Component-wise vector multiplication and division
    inline constexpr Vector2 operator*(const Vector2 &rhs) const noexcept
    {
        return {x * rhs.x, y * rhs.y};
    }

    inline constexpr Vector2 operator/(const Vector2 &rhs) const noexcept
    {
        return {x / rhs.x, y / rhs.y};
    }

    // Compound assignment operators
    inline constexpr Vector2 &operator+=(const Vector2 &rhs) noexcept
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    inline constexpr Vector2 &operator-=(const Vector2 &rhs) noexcept
    {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    inline constexpr Vector2 &operator*=(T scalar) noexcept
    {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    inline constexpr Vector2 &operator/=(T scalar) noexcept
    {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    inline constexpr Vector2 &operator*=(const Vector2 &rhs) noexcept
    {
        x *= rhs.x;
        y *= rhs.y;
        return *this;
    }

    inline constexpr Vector2 &operator/=(const Vector2 &rhs) noexcept
    {
        x /= rhs.x;
        y /= rhs.y;
        return *this;
    }

    inline constexpr bool operator==(const Vector2 &rhs) const noexcept
    {
        return x == rhs.x && y == rhs.y;
    }

    inline constexpr bool operator!=(const Vector2 &rhs) const noexcept
    {
        return !(*this == rhs);
    }

    inline constexpr Vector2 operator-() const noexcept
    {
        return {-x, -y};
    }

    T x{}, y{};
};

template <typename T>
class Vector3
{
public:
    constexpr Vector3() noexcept = default;
    constexpr Vector3(T x, T y, T z) noexcept : x(x), y(y), z(z) {}
    constexpr Vector3(const Vector3 &) noexcept = default;
    constexpr Vector3(Vector3 &&) noexcept = default;

    constexpr Vector3 &operator=(const Vector3 &) noexcept = default;
    constexpr Vector3 &operator=(Vector3 &&) noexcept = default;

    // Addition, subtraction (vector only)
    inline constexpr Vector3 operator+(const Vector3 &rhs) const noexcept
    {
        return {x + rhs.x, y + rhs.y, z + rhs.z};
    }

    inline constexpr Vector3 operator-(const Vector3 &rhs) const noexcept
    {
        return {x - rhs.x, y - rhs.y, z - rhs.z};
    }

    // Scalar multiplication and division
    inline constexpr Vector3 operator*(T scalar) const noexcept
    {
        return {x * scalar, y * scalar, z * scalar};
    }

    inline constexpr Vector3 operator/(T scalar) const noexcept
    {
        return {x / scalar, y / scalar, z / scalar};
    }

    // Component-wise vector multiplication and division
    inline constexpr Vector3 operator*(const Vector3 &rhs) const noexcept
    {
        return {x * rhs.x, y * rhs.y, z * rhs.z};
    }

    inline constexpr Vector3 operator/(const Vector3 &rhs) const noexcept
    {
        return {x / rhs.x, y / rhs.y, z / rhs.z};
    }

    // Compound assignment operators
    inline constexpr Vector3 &operator+=(const Vector3 &rhs) noexcept
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    inline constexpr Vector3 &operator-=(const Vector3 &rhs) noexcept
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    inline constexpr Vector3 &operator*=(T scalar) noexcept
    {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    inline constexpr Vector3 &operator/=(T scalar) noexcept
    {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }

    inline constexpr Vector3 &operator*=(const Vector3 &rhs) noexcept
    {
        x *= rhs.x;
        y *= rhs.y;
        z *= rhs.z;
        return *this;
    }

    inline constexpr Vector3 &operator/=(const Vector3 &rhs) noexcept
    {
        x /= rhs.x;
        y /= rhs.y;
        z /= rhs.z;
        return *this;
    }

    inline constexpr bool operator==(const Vector3 &rhs) const noexcept
    {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }

    inline constexpr bool operator!=(const Vector3 &rhs) const noexcept
    {
        return !(*this == rhs);
    }

    inline constexpr Vector3 operator-() const noexcept
    {
        return {-x, -y, -z};
    }

    T x{}, y{}, z{};
};

template <typename T>
class Vector4
{
public:
    constexpr Vector4() noexcept = default;
    constexpr Vector4(T x, T y, T z, T w) noexcept : x(x), y(y), z(z), w(w) {}
    constexpr Vector4(const Vector4 &) noexcept = default;
    constexpr Vector4(Vector4 &&) noexcept = default;

    constexpr Vector4 &operator=(const Vector4 &) noexcept = default;
    constexpr Vector4 &operator=(Vector4 &&) noexcept = default;

    // Addition, subtraction (vector only)
    inline constexpr Vector4 operator+(const Vector4 &rhs) const noexcept
    {
        return {x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w};
    }

    inline constexpr Vector4 operator-(const Vector4 &rhs) const noexcept
    {
        return {x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w};
    }

    // Scalar multiplication and division
    inline constexpr Vector4 operator*(T scalar) const noexcept
    {
        return {x * scalar, y * scalar, z * scalar, w * scalar};
    }

    inline constexpr Vector4 operator/(T scalar) const noexcept
    {
        return {x / scalar, y / scalar, z / scalar, w / scalar};
    }

    // Component-wise vector multiplication and division
    inline constexpr Vector4 operator*(const Vector4 &rhs) const noexcept
    {
        return {x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w};
    }

    inline constexpr Vector4 operator/(const Vector4 &rhs) const noexcept
    {
        return {x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w};
    }

    // Compound assignment operators
    inline constexpr Vector4 &operator+=(const Vector4 &rhs) noexcept
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        w += rhs.w;
        return *this;
    }

    inline constexpr Vector4 &operator-=(const Vector4 &rhs) noexcept
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        w -= rhs.w;
        return *this;
    }

    inline constexpr Vector4 &operator*=(T scalar) noexcept
    {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        w *= scalar;
        return *this;
    }

    inline constexpr Vector4 &operator/=(T scalar) noexcept
    {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        w /= scalar;
        return *this;
    }

    inline constexpr Vector4 &operator*=(const Vector4 &rhs) noexcept
    {
        x *= rhs.x;
        y *= rhs.y;
        z *= rhs.z;
        w *= rhs.w;
        return *this;
    }

    inline constexpr Vector4 &operator/=(const Vector4 &rhs) noexcept
    {
        x /= rhs.x;
        y /= rhs.y;
        z /= rhs.z;
        w /= rhs.w;
        return *this;
    }

    inline constexpr bool operator==(const Vector4 &rhs) const noexcept
    {
        return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w;
    }

    inline constexpr bool operator!=(const Vector4 &rhs) const noexcept
    {
        return !(*this == rhs);
    }

    inline constexpr Vector4 operator-() const noexcept
    {
        return {-x, -y, -z, -w};
    }

    T x{}, y{}, z{}, w{};
};
