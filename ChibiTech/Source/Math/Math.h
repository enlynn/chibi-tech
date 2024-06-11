//
// Header-Only Math Library for common Vector, Matrix, and Quaternion Math
//
//------------------------------------------
// Vector Math
//
// f32x2, f32x3, and f32x4 implements the usual math operators (+ - * /).
//
// f32x2 Additions Functions:
//
// f32x3 Additions Functions:
//
// f32x4 Additions Functions:
//
//------------------------------------------
// Matrix Math
//
// Matrices do not implement the math operator overloads since, I want it to be explicit
// if the operation is a Left or Right Hand.
//
// f32x44 F32x44MulRH(f32x44 Left, f32x44 Right);
// f32x44 InvertMatrix(f32x44 Matrix);
// f32x44 ScaleMatrix(f32 ScaleX, f32 ScaleY, f32 ScaleZ);
// f32x44 TransposeMatrix(f32x44 InMatrix);
// f32x4 MatrixTranslatePoint(f32x44 Matrix, f32x4 Point);
// f32x44 TranslateMatrix(f32x3 TranslateVector);
// f32x44 LookAtMatrixRH(f32x3 EyeVector, f32x3 CenterPoint, f32x3 UpVector);
// f32x44 PerspectiveMatrixRH(f32 FieldOfView, f32 AspectRatio, f32 NearPlane, f32 FarPlane);
// f32x44 RotateXMatrix(f32 Theta);
// f32x44 RotateYMatrix(f32 Theta);
// f32x44 RotateZMatrix(f32 Theta);
// f32x44 RotateMatrix(f32 Theta, f32x3 RotationAxis);
//
//------------------------------------------
// Quaternion Math
//
// f32x44 QuaternionToRotationMatrix(quaternion Quaternion)
//
//------------------------------------------
// More Misc. / Geometric Functions
//
// s32   S32RandomClamped(s32 Min, s32 Max)
// f32   F32Clamp(f32 Min, f32 Max, f32 Value)
// f32   F32Random()
// f32   F32RandomClamped(f32 Min, f32 Max)
// f32   Smoothstep(f32 Point0, f32 Point1, f32 Factor)
// f32   Smootherstep(f32 Point0, f32 Point1, f32 Factor)
// f32x3 F32x3ComputeNormal(f32x3 Point0, f32x3 Point1, f32x3 Point2)
// f32x3 F32x3Random()
// f32x3 F32x3RandomClamped(f32 Min, f32 Max)
// f32x3 F32x3RandomInUnitSphere()
// f32x3 F32x3RandomInHemisphere(f32x3 Normal)
// f32x3 F32x3RandomUnitVector()
// f32x3 F32x3RandomInUnitDisc()
// f32x3 ReflectVector(f32x3 Vector, f32x3 Normal)
// f32x3 RefractVector(f32x3 IncidentVector, f32x3 Normal, f32 IndicesOfRefraction)
//
// Approximates the contribution of Fresnel's Factor to Specular Reflected Light
// f32 SchlickApproximation(f32 Cosine, f32 RefractionIndex)
//

#pragma once

#include <Types.h>

#include <cmath>
#include <limits>
#include <numbers> // requires c++ 20

constexpr f32 F32_EPSILON = std::numeric_limits<float>::epsilon() * 0.5f;
constexpr f32 F32_PI      = std::numbers::pi_v<float>;
constexpr f32 F32_PIDIV2  = F32_PI / 2.0f;
constexpr f32 F32_2PI     = F32_PI * 2.0f;


struct f32x2
{
    union
    {
        struct { f32 X, Y; };
        f32 Ptr[2];
    };

    inline f32    Length();
    inline f32    LengthSq();
    inline f32x2& Norm();

    inline f32x2& operator+=(f32x2 Other);
    inline f32x2& operator+=(f32   Other);

    inline f32x2& operator-=(f32x2 Other);
    inline f32x2& operator-=(f32   Other);

    inline f32x2& operator*=(f32x2 Other);
    inline f32x2& operator*=(f32   Other);

    inline f32x2& operator/=(f32x2 Other);
    inline f32x2& operator/=(f32   Other);
};

struct f32x3
{
    union
    {
        struct { f32 X, Y, Z;         };
        struct { f32x2 XY; f32 Pad0;  };
        struct { f32 Pad1; f32x2 YZ;  };
        f32 Ptr[3];
    };

    inline f32    Length();
    inline f32    LengthSq();
    inline f32x3& Norm();

    inline f32x3& operator+=(f32x3 Other);
    inline f32x3& operator+=(f32x2 Other);
    inline f32x3& operator+=(f32   Other);

    inline f32x3& operator-=(f32x3 Other);
    inline f32x3& operator-=(f32x2 Other);
    inline f32x3& operator-=(f32   Other);

    inline f32x3& operator*=(f32x3 Other);
    inline f32x3& operator*=(f32x2 Other);
    inline f32x3& operator*=(f32   Other);

    inline f32x3& operator/=(f32x3 Other);
    inline f32x3& operator/=(f32x2 Other);
    inline f32x3& operator/=(f32   Other);
};

struct f32x4
{
    union
    {
        struct { f32 X, Y, Z, W;         };
        struct { f32x2 XY;   f32x2 ZW;   };
        struct { f32x3 XYZ;  f32   Pad0; };
        struct { f32   Pad1; f32x3 YZW;  };
        f32 Ptr[4];
    };

    inline f32    Length();
    inline f32    LengthSq();
    inline f32x4& Norm();

    // The Per-Component Operators only apply to the maximum width of a vector. For example, if you add a
    // f32x4 and f32x2, then only the XY components are added.

    // Per Component Add
    inline f32x4& operator+=(f32x4 Other);
    inline f32x4& operator+=(f32x3 Other);
    inline f32x4& operator+=(f32x2 Other);

    // Per Component Subtract
    inline f32x4& operator-=(f32x4 Other);
    inline f32x4& operator-=(f32x3 Other);
    inline f32x4& operator-=(f32x2 Other);

    // Per Component Multiply
    inline f32x4& operator*=(f32x4 Other);
    inline f32x4& operator*=(f32x3 Other);
    inline f32x4& operator*=(f32x2 Other);

    // Divide - this is a little special since this is the one function that can crash a program
    // If a component gets a div-by-zero, the result will be 0.
    inline f32x4& operator/=(f32x4 Other);
    inline f32x4& operator/=(f32x3 Other);
    inline f32x4& operator/=(f32x2 Other);

    // The following operators apply to the entire vector.
    inline f32x4& operator+=(f32 Other);
    inline f32x4& operator-=(f32 Other);
    inline f32x4& operator*=(f32 Other);
    inline f32x4& operator/=(f32 Other);
};

struct quaternion
{
    union
    {
        struct { f32 X, Y, Z, Theta; };
        struct { f32x3 XYZ; f32 Pad0; };
        f32x4 Ptr;
    };

    quaternion(f32 AxisX, f32 AxisY, f32 AxisZ, f32 Angle)
        : X(AxisX), Y(AxisY), Z(AxisZ), Theta(Angle) {}

    quaternion(f32x3 Axis, f32 Angle) : quaternion(Axis.X, Axis.Y, Axis.Z, Angle)                {}
    quaternion(f32 Array[4])          : quaternion(Array[0], Array[1], Array[2], Array[3]) {}

    quaternion() : quaternion(0.0f, 0.0f, 0.0f, 0.0f) {}

    inline quaternion& Norm();
};

struct f32x44
{
    union
    {
        struct { f32x4 C0, C1, C2, C3; };
        f32 Ptr[4][4];
    };

    f32x44() : f32x44(1.0f) {}

    f32x44(f32 Value)
    {
        C0 = { Value,    0.0f, 0.0f, 0.0f  };
        C1 = { 0.0f, Value,    0.0f, 0.0f  };
        C2 = { 0.0f, 0.0f, Value   , 0.0f  };
        C3 = { 0.0f, 0.0f, 0.0f, Value     };
    }
};

using vec2 = f32x2;
using vec3 = f32x3;
using vec4 = f32x4;
using mat4 = f32x44;

static constexpr f32x2 f32x2_zero = { .Ptr = { 0, 0               } };
static constexpr f32x3 f32x3_zero = { .Ptr = { 0, 0, 0        } };
static constexpr f32x4 f32x4_zero = { .Ptr = { 0, 0, 0, 0 } };

static constexpr f32x2 f32x2_one = { .Ptr = { 1, 1               } };
static constexpr f32x3 f32x3_one = { .Ptr = { 1, 1, 1        } };
static constexpr f32x4 f32x4_one = { .Ptr = { 1, 1, 1, 1 } };

//
// Misc. Functions
//

inline bool F32IsNan(f32 Value)
{
    return std::isnan(Value);
}

inline bool F32IsInf(f32 Value)
{
    return std::isinf(Value);
}

inline f32 DegreesToRadians(f32 Degrees)
{
    return Degrees * (F32_PI / 180.0f);
}

// --------------------------------------------------------------------
// NOTE(enlynn): This is a lazy attempt at addressing FP-error
//
// Source: https://isocpp.org/wiki/faq/newbie#floating-point-arith

// This function is not symmetrical - F32IsEqual(Left, Right) might not always equal F32IsEqual(Right, Left)
// TODO(enlynn): properly handle floating point error.
inline bool F32IsEqual(f32 Left, f32 Right)
{
    return std::abs(Left - Right) <= F32_EPSILON * std::abs(Left);
}

inline bool F32IsZero(f32 Value)
{
    return F32IsEqual(Value, 0.0f);
}

// --------------------------------------------------------------------

inline f32 F32FusedMultiplyAdd(f32 A, f32 B, f32 C)
{ // A * B + C
    return std::fma(A, B, C);
}

inline f32 Lerp(f32 Min, f32 Max, f32 Factor)
{
    return F32FusedMultiplyAdd(Factor, Max - Min, Min);
}

//
// Boilerplate Math Operator Overloads: f32x2
//

inline f32x2& f32x2::operator+=(f32x2 Other)
{
    X += Other.X;
    Y += Other.Y;
    return *this;
}

inline f32x2& f32x2::operator+=(f32 Other)
{
    X += Other;
    Y += Other;
    return *this;
}

inline f32x2& f32x2::operator-=(f32x2 Other)
{
    X -= Other.X;
    Y -= Other.Y;
    return *this;
}

inline f32x2& f32x2::operator-=(f32 Other)
{
    X -= Other;
    Y -= Other;
    return *this;
}

inline f32x2& f32x2::operator*=(f32x2 Other)
{
    X *= Other.X;
    Y *= Other.Y;
    return *this;
}

inline f32x2& f32x2::operator*=(f32 Other)
{
    X *= Other;
    Y *= Other;
    return *this;
}

inline f32x2& f32x2::operator/=(f32x2 Other)
{
    X = F32IsZero(Other.X) ? 0.0f : X / Other.X;
    Y = F32IsZero(Other.Y) ? 0.0f : Y / Other.Y;
    return *this;
}

inline f32x2& f32x2::operator/=(f32 Other)
{
    X = F32IsZero(Other) ? 0.0f : X / Other;
    Y = F32IsZero(Other) ? 0.0f : Y / Other;
    return *this;
}

inline f32x2 operator+(f32x2 Left, f32x2 Right)
{
    f32x2 Result = f32x2_zero;
    Result.X = Left.X + Right.X;
    Result.Y = Left.Y + Right.Y;
    return Result;
}

inline f32x2 operator+(f32x2 Left, f32 Right)
{
    f32x2 Result = f32x2_zero;
    Result.X = Left.X + Right;
    Result.Y = Left.Y + Right;
    return Result;
}

inline f32x2 operator-(f32x2 Left, f32x2 Right)
{
    f32x2 Result = f32x2_zero;
    Result.X = Left.X - Right.X;
    Result.Y = Left.Y - Right.Y;
    return Result;
}

inline f32x2 operator-(f32x2 Left, f32 Right)
{
    f32x2 Result = f32x2_zero;
    Result.X = Left.X - Right;
    Result.Y = Left.Y - Right;
    return Result;
}

inline f32x2 operator*(f32x2 Left, f32x2 Right)
{
    f32x2 Result = f32x2_zero;
    Result.X = Left.X * Right.X;
    Result.Y = Left.Y * Right.Y;
    return Result;
}

inline f32x2 operator*(f32x2 Left, f32 Right)
{
    f32x2 Result = f32x2_zero;
    Result.X = Left.X * Right;
    Result.Y = Left.Y * Right;
    return Result;
}

inline f32x2 operator/(f32x2 Left, f32x2 Right)
{
    f32x2 Result = f32x2_zero;
    Result.X = F32IsZero(Right.X) ? 0.0f : Left.X / Right.X;
    Result.Y = F32IsZero(Right.Y) ? 0.0f : Left.Y / Right.Y;
    return Result;
}

inline f32x2 operator/(f32x2 Left, f32 Right)
{
    f32x2 Result = f32x2_zero;
    Result.X = F32IsZero(Right) ? 0.0f : Left.X / Right;
    Result.Y = F32IsZero(Right) ? 0.0f : Left.Y / Right;
    return Result;
}

inline f32 f32x2::Length()   { return sqrt(X * X + Y * Y); }
inline f32 f32x2::LengthSq() { return X * X + Y * Y;          }

inline f32x2& f32x2::Norm()
{
    f32 Len = Length();
    if (!F32IsZero(Len))
    {
        X /= Len;
        Y /= Len;
    }
    return *this;
}

inline f32 Dot(f32x2 Left, f32x2 Right)
{
    return Left.X * Right.X + Left.Y * Right.Y;
}

//
// Boilerplate Math Operator Overloads: f32x3


inline f32x3& f32x3::operator+=(f32x3 Other)
{
    X += Other.X;
    Y += Other.Y;
    Z += Other.Z;
    return *this;
}

inline f32x3& f32x3::operator+=(f32x2 Other)
{
    X += Other.X;
    Y += Other.Y;
    return *this;
}

inline f32x3& f32x3::operator+=(f32 Other)
{
    X += Other;
    Y += Other;
    Z += Other;
    return *this;
}

inline f32x3& f32x3::operator-=(f32x3 Other)
{
    X -= Other.X;
    Y -= Other.Y;
    Z -= Other.Z;
    return *this;
}

inline f32x3& f32x3::operator-=(f32x2 Other)
{
    X -= Other.X;
    Y -= Other.Y;
    return *this;
}

inline f32x3& f32x3::operator-=(f32   Other)
{
    X -= Other;
    Y -= Other;
    Z -= Other;
    return *this;
}

inline f32x3& f32x3::operator*=(f32x3 Other)
{
    X *= Other.X;
    Y *= Other.Y;
    Z *= Other.Z;
    return *this;
}

inline f32x3& f32x3::operator*=(f32x2 Other)
{
    X *= Other.X;
    Y *= Other.Y;
    return *this;
}

inline f32x3& f32x3::operator*=(f32 Other)
{
    X *= Other;
    Y *= Other;
    Z *= Other;
    return *this;
}

inline f32x3& f32x3::operator/=(f32x3 Other)
{
    X = F32IsZero(Other.X) ? 0.0f : X / Other.X;
    Y = F32IsZero(Other.Y) ? 0.0f : Y / Other.Y;
    Z = F32IsZero(Other.Z) ? 0.0f : Z / Other.Z;
    return *this;
}

inline f32x3& f32x3::operator/=(f32x2 Other)
{
    X = F32IsZero(Other.X) ? 0.0f : X / Other.X;
    Y = F32IsZero(Other.Y) ? 0.0f : Y / Other.Y;
    return *this;
}

inline f32x3& f32x3::operator/=(f32 Other)
{
    X = F32IsZero(Other) ? 0.0f : X / Other;
    Y = F32IsZero(Other) ? 0.0f : Y / Other;
    Z = F32IsZero(Other) ? 0.0f : Z / Other;
    return *this;
}

inline f32x3 operator+(f32x3 Left, f32x3 Right)
{
    f32x3 Result = f32x3_zero;
    Result.X = Left.X + Right.X;
    Result.Y = Left.Y + Right.Y;
    Result.Z = Left.Z + Right.Z;
    return Result;
}

inline f32x3 operator+(f32x3 Left, f32x2 Right)
{
    f32x3 Result = f32x3_zero;
    Result.X = Left.X + Right.X;
    Result.Y = Left.Y + Right.Y;
    return Result;
}

inline f32x3 operator+(f32x2 Left, f32x3 Right)
{
    f32x3 Result = f32x3_zero;
    Result.X = Left.X + Right.X;
    Result.Y = Left.Y + Right.Y;
    return Result;
}

inline f32x3 operator+(f32x3 Left, f32 Right)
{
    f32x3 Result = f32x3_zero;
    Result.X = Left.X + Right;
    Result.Y = Left.Y + Right;
    Result.Z = Left.Z + Right;
    return Result;
}

inline f32x3 operator+(f32 Left, f32x3 Right)
{
    f32x3 Result = f32x3_zero;
    Result.X = Left + Right.X;
    Result.Y = Left + Right.Y;
    Result.Z = Left + Right.Z;
    return Result;
}

// Subtract
inline f32x3 operator-(f32x3 Left, f32x3 Right)
{
    f32x3 Result = f32x3_zero;
    Result.X = Left.X - Right.X;
    Result.Y = Left.Y - Right.Y;
    Result.Z = Left.Z - Right.Z;
    return Result;
}

inline f32x3 operator-(f32x3 Left, f32x2 Right)
{
    f32x3 Result = f32x3_zero;
    Result.X = Left.X - Right.X;
    Result.Y = Left.Y - Right.Y;
    return Result;
}

inline f32x3 operator-(f32x2 Left, f32x3 Right)
{
    f32x3 Result = f32x3_zero;
    Result.X = Left.X - Right.X;
    Result.Y = Left.Y - Right.Y;
    return Result;
}

inline f32x3 operator-(f32x3 Left, f32 Right)
{
    f32x3 Result = f32x3_zero;
    Result.X = Left.X - Right;
    Result.Y = Left.Y - Right;
    Result.Z = Left.Z - Right;
    return Result;
}

inline f32x3 operator-(f32 Left, f32x3 Right)
{
    f32x3 Result = f32x3_zero;
    Result.X = Left - Right.X;
    Result.Y = Left - Right.Y;
    Result.Z = Left - Right.Z;
    return Result;
}

// Multiply
inline f32x3 operator*(f32x3 Left, f32x3 Right)
{
    f32x3 Result = f32x3_zero;
    Result.X = Left.X * Right.X;
    Result.Y = Left.Y * Right.Y;
    Result.Z = Left.Z * Right.Z;
    return Result;
}

inline f32x3 operator*(f32x3 Left, f32x2 Right)
{
    f32x3 Result = f32x3_zero;
    Result.X = Left.X * Right.X;
    Result.Y = Left.Y * Right.Y;
    return Result;
}

inline f32x3 operator*(f32x2 Left, f32x3 Right)
{
    f32x3 Result = f32x3_zero;
    Result.X = Left.X * Right.X;
    Result.Y = Left.Y * Right.Y;
    return Result;
}

inline f32x3 operator*(f32x3 Left, f32 Right)
{
    f32x3 Result = f32x3_zero;
    Result.X = Left.X * Right;
    Result.Y = Left.Y * Right;
    Result.Z = Left.Z * Right;
    return Result;
}

inline f32x3 operator*(f32 Left, f32x3 Right)
{
    f32x3 Result = f32x3_zero;
    Result.X = Left * Right.X;
    Result.Y = Left * Right.Y;
    Result.Z = Left * Right.Z;
    return Result;
}

// Divide
inline f32x3 operator/(f32x3 Left, f32x3 Right)
{
    f32x3 Result = f32x3_zero;
    Result.X = F32IsZero(Right.X) ? 0.0f : Left.X / Right.X;
    Result.Y = F32IsZero(Right.Y) ? 0.0f : Left.Y / Right.Y;
    Result.Z = F32IsZero(Right.Z) ? 0.0f : Left.Z / Right.Z;
    return Result;
}

inline f32x3 operator/(f32x3 Left, f32x2 Right)
{
    f32x3 Result = f32x3_zero;
    Result.X = F32IsZero(Right.X) ? 0.0f : Left.X / Right.X;
    Result.Y = F32IsZero(Right.Y) ? 0.0f : Left.Y / Right.Y;
    return Result;
}

inline f32x3 operator/(f32x2 Left, f32x3 Right)
{
    f32x3 Result = f32x3_zero;
    Result.X = F32IsZero(Right.X) ? 0.0f : Left.X / Right.X;
    Result.Y = F32IsZero(Right.Y) ? 0.0f : Left.Y / Right.Y;
    return Result;
}

inline f32x3 operator/(f32x3 Left, f32 Right)
{
    f32x3 Result = f32x3_zero;
    Result.X = F32IsZero(Right) ? 0.0f : Left.X / Right;
    Result.Y = F32IsZero(Right) ? 0.0f : Left.Y / Right;
    Result.Z = F32IsZero(Right) ? 0.0f : Left.Z / Right;
    return Result;
}

inline f32 f32x3::Length()   { return sqrt(X * X + Y * Y + Z * Z); }
inline f32 f32x3::LengthSq() { return X * X + Y * Y + Z * Z;          }

inline f32x3& f32x3::Norm()
{
    f32 Len = Length();
    if (!F32IsZero(Len))
    {
        X /= Len;
        Y /= Len;
        Z /= Len;
    }
    return *this;
}

inline f32 Dot(f32x3 Left, f32x3 Right)
{
    return Left.X * Right.X + Left.Y * Right.Y + Left.Z * Right.Z;
}

//
// f32x3 Cross(f32x3 Left, f32x3 Right)
//
//

inline f32x3 Cross(f32x3 Left, f32x3 Right)
{
    f32x3 Result;

    Result.X = (Left.Y * Right.Z) - (Left.Z * Right.Y);
    Result.Y = (Left.Z * Right.X) - (Left.X * Right.Z);
    Result.Z = (Left.X * Right.Y) - (Left.Y * Right.X);

    return Result;
}

//
// Boilerplate Math Operator Overloads: f32x4
//

// Per component add-equals
inline f32x4& f32x4::operator+=(f32x4 Other)
{
    X += Other.X;
    Y += Other.Y;
    Z += Other.Z;
    W += Other.W;
    return *this;
}

inline f32x4& f32x4::operator+=(f32x3 Other)
{
    X += Other.X;
    Y += Other.Y;
    Z += Other.Z;
    return *this;
}

inline f32x4& f32x4::operator+=(f32x2 Other)
{
    X += Other.X;
    Y += Other.Y;
    return *this;
}

// Per Component Subtract-equals
inline f32x4& f32x4::operator-=(f32x4 Other)
{
    X -= Other.X;
    Y -= Other.Y;
    Z -= Other.Z;
    W -= Other.W;
    return *this;
}

inline f32x4& f32x4::operator-=(f32x3 Other)
{
    X -= Other.X;
    Y -= Other.Y;
    Z -= Other.Z;
    return *this;
}

inline f32x4& f32x4::operator-=(f32x2 Other)
{
    X -= Other.X;
    Y -= Other.Y;
    return *this;
}

// Per Component Multiply-equals
inline f32x4& f32x4::operator*=(f32x4 Other)
{
    X *= Other.X;
    Y *= Other.Y;
    Z *= Other.Z;
    W *= Other.W;
    return *this;
}

inline f32x4& f32x4::operator*=(f32x3 Other)
{
    X *= Other.X;
    Y *= Other.Y;
    Z *= Other.Z;
    return *this;
}

inline f32x4& f32x4::operator*=(f32x2 Other)
{
    X *= Other.X;
    Y *= Other.Y;
    return *this;
}

// Per Component Multiply-equals
inline f32x4& f32x4::operator/=(f32x4 Other)
{
    X = (F32IsZero(Other.X)) ? 0.0f : X / Other.X;
    Y = (F32IsZero(Other.Y)) ? 0.0f : Y / Other.Y;
    Z = (F32IsZero(Other.Z)) ? 0.0f : Z / Other.Z;
    W = (F32IsZero(Other.W)) ? 0.0f : W / Other.W;
}

inline f32x4& f32x4::operator/=(f32x3 Other)
{
    X = (F32IsZero(Other.X)) ? 0.0f : X / Other.X;
    Y = (F32IsZero(Other.Y)) ? 0.0f : Y / Other.Y;
    Z = (F32IsZero(Other.Z)) ? 0.0f : Z / Other.Z;
}

inline f32x4& f32x4::operator/=(f32x2 Other)
{
    X = (F32IsZero(Other.X)) ? 0.0f : X / Other.X;
    Y = (F32IsZero(Other.Y)) ? 0.0f : Y / Other.Y;
}

// The following operators apply to the entire vector.
inline f32x4& f32x4::operator+=(f32 Other)
{
    X += Other;
    Y += Other;
    Z += Other;
    W += Other;
}

inline f32x4& f32x4::operator-=(f32 Other)
{
    X -= Other;
    Y -= Other;
    Z -= Other;
    W -= Other;
}

inline f32x4& f32x4::operator*=(f32 Other)
{
    X *= Other;
    Y *= Other;
    Z *= Other;
    W *= Other;
}

inline f32x4& f32x4::operator/=(f32 Other)
{
    X = (F32IsZero(Other)) ? 0.0f : X / Other;
    Y = (F32IsZero(Other)) ? 0.0f : Y / Other;
    Z = (F32IsZero(Other)) ? 0.0f : Z / Other;
    W = (F32IsZero(Other)) ? 0.0f : W / Other;
}

// Add Operator
inline f32x4 operator+(f32x4 Left, f32x4 Right)
{
    f32x4 Result = f32x4_zero;

    Result.X = Left.X + Right.X;
    Result.Y = Left.Y + Right.Y;
    Result.Z = Left.Z + Right.Z;
    Result.W = Left.W + Right.W;

    return Result;
}

inline f32x4 operator+(f32x4 Left, f32x3 Right)
{
    f32x4 Result = f32x4_zero;

    Result.X = Left.X + Right.X;
    Result.Y = Left.Y + Right.Y;
    Result.Z = Left.Z + Right.Z;

    return Result;
}

inline f32x4 operator+(f32x3 Left, f32x4 Right)
{
    f32x4 Result = f32x4_zero;

    Result.X = Left.X + Right.X;
    Result.Y = Left.Y + Right.Y;
    Result.Z = Left.Z + Right.Z;

    return Result;
}

inline f32x4 operator+(f32x4 Left, f32x2 Right)
{
    f32x4 Result = f32x4_zero;

    Result.X = Left.X + Right.X;
    Result.Y = Left.Y + Right.Y;

    return Result;
}

inline f32x4 operator+(f32x2 Left, f32x4 Right)
{
    f32x4 Result = f32x4_zero;

    Result.X = Left.X + Right.X;
    Result.Y = Left.Y + Right.Y;

    return Result;
}

inline f32x4 operator+(f32x4 Left, f32 Right)
{
    f32x4 Result = f32x4_zero;

    Result.X = Left.X + Right;
    Result.Y = Left.Y + Right;
    Result.Z = Left.Z + Right;
    Result.W = Left.W + Right;

    return Result;
}

// Sub Operator
inline f32x4 operator-(f32x4 Left, f32x4 Right)
{
    f32x4 Result = f32x4_zero;

    Result.X = Left.X - Right.X;
    Result.Y = Left.Y - Right.Y;
    Result.Z = Left.Z - Right.Z;
    Result.W = Left.W - Right.W;

    return Result;
}

inline f32x4 operator-(f32x4 Left, f32x3 Right)
{
    f32x4 Result = f32x4_zero;

    Result.X = Left.X - Right.X;
    Result.Y = Left.Y - Right.Y;
    Result.Z = Left.Z - Right.Z;

    return Result;
}

inline f32x4 operator-(f32x3 Left, f32x4 Right)
{
    f32x4 Result = f32x4_zero;

    Result.X = Left.X - Right.X;
    Result.Y = Left.Y - Right.Y;

    return Result;
}

inline f32x4 operator-(f32x4 Left, f32x2 Right)
{
    f32x4 Result = f32x4_zero;

    Result.X = Left.X - Right.X;
    Result.Y = Left.Y - Right.Y;

    return Result;
}

inline f32x4 operator-(f32x2 Left, f32x4 Right)
{
    f32x4 Result = f32x4_zero;

    Result.X = Left.X - Right.X;
    Result.Y = Left.Y - Right.Y;

    return Result;
}

inline f32x4 operator-(f32x4 Left, f32 Right)
{
    f32x4 Result = f32x4_zero;

    Result.X = Left.X - Right;
    Result.Y = Left.Y - Right;
    Result.Z = Left.Z - Right;
    Result.W = Left.W - Right;

    return Result;
}

// Multiplication Operator
inline f32x4 operator*(f32x4 Left, f32x4 Right)
{
    f32x4 Result = f32x4_zero;

    Result.X = Left.X * Right.X;
    Result.Y = Left.Y * Right.Y;
    Result.Z = Left.Z * Right.Z;
    Result.W = Left.W * Right.W;

    return Result;
}

inline f32x4 operator*(f32x4 Left, f32x3 Right)
{
    f32x4 Result = f32x4_zero;

    Result.X = Left.X * Right.X;
    Result.Y = Left.Y * Right.Y;
    Result.Z = Left.Z * Right.Z;

    return Result;
}

inline f32x4 operator*(f32x3 Left, f32x4 Right)
{
    f32x4 Result = f32x4_zero;

    Result.X = Left.X * Right.X;
    Result.Y = Left.Y * Right.Y;
    Result.Z = Left.Z * Right.Z;

    return Result;
}

inline f32x4 operator*(f32x4 Left, f32x2 Right)
{
    f32x4 Result = f32x4_zero;

    Result.X = Left.X * Right.X;
    Result.Y = Left.Y * Right.Y;

    return Result;
}

inline f32x4 operator*(f32x2 Left, f32x4 Right)
{
    f32x4 Result = f32x4_zero;

    Result.X = Left.X * Right.X;
    Result.Y = Left.Y * Right.Y;

    return Result;
}

inline f32x4 operator*(f32x4 Left, f32 Right)
{
    f32x4 Result = f32x4_zero;

    Result.X = Left.X * Right;
    Result.Y = Left.Y * Right;
    Result.Z = Left.Z * Right;
    Result.W = Left.W * Right;

    return Result;
}

// Division Operator
inline f32x4 operator/(f32x4 Left, f32x4 Right)
{
    f32x4 Result = f32x4_zero;

    Result.X = F32IsZero(Right.X) ? 0.0f : Left.X / Right.X;
    Result.Y = F32IsZero(Right.Y) ? 0.0f : Left.Y / Right.Y;
    Result.Z = F32IsZero(Right.Z) ? 0.0f : Left.Z / Right.Z;
    Result.W = F32IsZero(Right.W) ? 0.0f : Left.W / Right.W;

    return Result;
}

inline f32x4 operator/(f32x4 Left, f32x3 Right)
{
    f32x4 Result = f32x4_zero;

    Result.X = F32IsZero(Right.X) ? 0.0f : Left.X / Right.X;
    Result.Y = F32IsZero(Right.Y) ? 0.0f : Left.Y / Right.Y;
    Result.Z = F32IsZero(Right.Z) ? 0.0f : Left.Z / Right.Z;

    return Result;
}

inline f32x4 operator/(f32x3 Left, f32x4 Right)
{
    f32x4 Result = f32x4_zero;

    Result.X = F32IsZero(Right.X) ? 0.0f : Left.X / Right.X;
    Result.Y = F32IsZero(Right.Y) ? 0.0f : Left.Y / Right.Y;
    Result.Z = F32IsZero(Right.Z) ? 0.0f : Left.Z / Right.Z;

    return Result;
}

inline f32x4 operator/(f32x4 Left, f32x2 Right)
{
    f32x4 Result = f32x4_zero;

    Result.X = F32IsZero(Right.X) ? 0.0f : Left.X / Right.X;
    Result.Y = F32IsZero(Right.Y) ? 0.0f : Left.Y / Right.Y;

    return Result;
}

inline f32x4 operator/(f32x2 Left, f32x4 Right)
{
    f32x4 Result = f32x4_zero;

    Result.X = F32IsZero(Right.X) ? 0.0f : Left.X / Right.X;
    Result.Y = F32IsZero(Right.Y) ? 0.0f : Left.Y / Right.Y;

    return Result;
}

inline f32x4 operator/(f32x4 Left, f32 Right)
{
    f32x4 Result = f32x4_zero;

    Result.X = F32IsZero(Right) ? 0.0f : Left.X / Right;
    Result.Y = F32IsZero(Right) ? 0.0f : Left.Y / Right;
    Result.Z = F32IsZero(Right) ? 0.0f : Left.Z / Right;
    Result.W = F32IsZero(Right) ? 0.0f : Left.W / Right;

    return Result;
}

// f32    f32x4::LengthSq()
// f32    f32x4::Length()
// f32x4& f32x4::Norm()
// f32    Dot(f32x4 Left, f32x4 Right)
// f32x4  Cross(f32x4 Left, f32x4 Right)

inline f32 f32x4::Length()   { return sqrt(X * X + Y * Y + Z * Z + W * W); }
inline f32 f32x4::LengthSq() { return X * X + Y * Y + Z * Z + W * W;          }

inline f32x4& f32x4::Norm()
{
    f32 Len = Length();
    if (!F32IsZero(Len))
    {
        X /= Len;
        Y /= Len;
        Z /= Len;
        W /= Len;
    }
    return *this;
}

inline f32 Dot(f32x4 Left, f32x4 Right)
{
    return Left.X * Right.X + Left.Y * Right.Y + Left.Z * Right.Z + Left.W * Right.W;
}

inline f32x4 Cross(f32x4 Left, f32x4 Right)
{
    f32x4 Result;
    Result.XYZ = Cross(Left.XYZ, Right.XYZ);
    Result.W   = 1.0f;
    return Result;
}

//
// f32x44 Functions
//

inline f32x44 F32x44MulRH(f32x44 Left, f32x44 Right)
{
    f32x44 Result;

    f32x4 lr0 = { Left.Ptr[0][0], Left.Ptr[1][0], Left.Ptr[2][0], Left.Ptr[3][0] };
    f32x4 lr1 = { Left.Ptr[0][1], Left.Ptr[1][1], Left.Ptr[2][1], Left.Ptr[3][1] };
    f32x4 lr2 = { Left.Ptr[0][2], Left.Ptr[1][2], Left.Ptr[2][2], Left.Ptr[3][2] };
    f32x4 lr3 = { Left.Ptr[0][3], Left.Ptr[1][3], Left.Ptr[2][3], Left.Ptr[3][3] };

    Result.Ptr[0][0] = Dot(lr0, Right.C0);
    Result.Ptr[0][1] = Dot(lr1, Right.C0);
    Result.Ptr[0][2] = Dot(lr2, Right.C0);
    Result.Ptr[0][3] = Dot(lr3, Right.C0);

    Result.Ptr[1][0] = Dot(lr0, Right.C1);
    Result.Ptr[1][1] = Dot(lr1, Right.C1);
    Result.Ptr[1][2] = Dot(lr2, Right.C1);
    Result.Ptr[1][3] = Dot(lr3, Right.C1);

    Result.Ptr[2][0] = Dot(lr0, Right.C2);
    Result.Ptr[2][1] = Dot(lr1, Right.C2);
    Result.Ptr[2][2] = Dot(lr2, Right.C2);
    Result.Ptr[2][3] = Dot(lr3, Right.C2);

    Result.Ptr[3][0] = Dot(lr0, Right.C3);
    Result.Ptr[3][1] = Dot(lr1, Right.C3);
    Result.Ptr[3][2] = Dot(lr2, Right.C3);
    Result.Ptr[3][3] = Dot(lr3, Right.C3);

    return Result;
}

// https://gist.github.com/mattatz/86fff4b32d198d0928d0fa4ff32cf6fa
inline f32x44 InvertMatrix(f32x44 Matrix)
{
    float n11 = Matrix.Ptr[0][0], n12 = Matrix.Ptr[1][0], n13 = Matrix.Ptr[2][0], n14 = Matrix.Ptr[3][0];
    float n21 = Matrix.Ptr[0][1], n22 = Matrix.Ptr[1][1], n23 = Matrix.Ptr[2][1], n24 = Matrix.Ptr[3][1];
    float n31 = Matrix.Ptr[0][2], n32 = Matrix.Ptr[1][2], n33 = Matrix.Ptr[2][2], n34 = Matrix.Ptr[3][2];
    float n41 = Matrix.Ptr[0][3], n42 = Matrix.Ptr[1][3], n43 = Matrix.Ptr[2][3], n44 = Matrix.Ptr[3][3];

    float t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44;
    float t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44;
    float t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44;
    float t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34;

    float det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;
    float idet = 1.0f / det;

    f32x44 Result = f32x44();

    Result.Ptr[0][0] = t11 * idet;
    Result.Ptr[0][1] = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * idet;
    Result.Ptr[0][2] = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * idet;
    Result.Ptr[0][3] = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * idet;

    Result.Ptr[1][0] = t12 * idet;
    Result.Ptr[1][1] = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * idet;
    Result.Ptr[1][2] = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * idet;
    Result.Ptr[1][3] = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * idet;

    Result.Ptr[2][0] = t13 * idet;
    Result.Ptr[2][1] = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * idet;
    Result.Ptr[2][2] = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * idet;
    Result.Ptr[2][3] = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * idet;

    Result.Ptr[3][0] = t14 * idet;
    Result.Ptr[3][1] = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * idet;
    Result.Ptr[3][2] = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * idet;
    Result.Ptr[3][3] = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * idet;

    return Result;
}

// Creates a scaling matrix
inline f32x44 ScaleMatrix(f32 ScaleX, f32 ScaleY, f32 ScaleZ)
{
    f32x44 Result = f32x44();

    Result.Ptr[0][0] = ScaleX;
    Result.Ptr[1][1] = ScaleY;
    Result.Ptr[2][2] = ScaleZ;

    return Result;
}

inline f32x44 TransposeMatrix(f32x44 InMatrix)
{
    f32x44 Result = f32x44();

    Result.Ptr[0][0] = InMatrix.Ptr[0][0];
    Result.Ptr[0][1] = InMatrix.Ptr[1][0];
    Result.Ptr[0][2] = InMatrix.Ptr[2][0];
    Result.Ptr[0][3] = InMatrix.Ptr[3][0];

    Result.Ptr[1][0] = InMatrix.Ptr[0][1];
    Result.Ptr[1][1] = InMatrix.Ptr[1][1];
    Result.Ptr[1][2] = InMatrix.Ptr[2][1];
    Result.Ptr[1][3] = InMatrix.Ptr[3][1];

    Result.Ptr[2][0] = InMatrix.Ptr[0][2];
    Result.Ptr[2][1] = InMatrix.Ptr[1][2];
    Result.Ptr[2][2] = InMatrix.Ptr[2][2];
    Result.Ptr[2][3] = InMatrix.Ptr[3][2];

    Result.Ptr[3][0] = InMatrix.Ptr[0][3];
    Result.Ptr[3][1] = InMatrix.Ptr[1][3];
    Result.Ptr[3][2] = InMatrix.Ptr[2][3];
    Result.Ptr[3][3] = InMatrix.Ptr[3][3];

    return Result;
}

inline f32x4 MatrixTranslatePoint(f32x44 Matrix, f32x4 Point)
{
    f32x4 r0 = { Matrix.Ptr[0][0], Matrix.Ptr[1][0], Matrix.Ptr[2][0], Matrix.Ptr[3][0] };
    f32x4 r1 = { Matrix.Ptr[0][1], Matrix.Ptr[1][1], Matrix.Ptr[2][1], Matrix.Ptr[3][1] };
    f32x4 r2 = { Matrix.Ptr[0][2], Matrix.Ptr[1][2], Matrix.Ptr[2][2], Matrix.Ptr[3][2] };
    f32x4 r3 = { Matrix.Ptr[0][3], Matrix.Ptr[1][3], Matrix.Ptr[2][3], Matrix.Ptr[3][3] };

    f32x4 Result = {};
    Result.X = Dot(Point, r0);
    Result.Y = Dot(Point, r1);
    Result.Z = Dot(Point, r2);
    Result.W = Dot(Point, r3);
    return Result;
}

// Creates a translation matrix
inline f32x44 TranslateMatrix(f32x3 TranslateVector)
{
    f32x44 Result = f32x44();

    Result.Ptr[3][0] = TranslateVector.X;
    Result.Ptr[3][1] = TranslateVector.Y;
    Result.Ptr[3][2] = TranslateVector.Z;

    return Result;
}

inline f32x44 LookAtMatrixRH(f32x3 EyePosition, f32x3 EyeLookAtPoint, f32x3 UpVector)
{
    f32x44 Result = f32x44();

    f32x3 f = EyeLookAtPoint - EyePosition;
    f.Norm();

    UpVector.Norm(); // just to be safe

    f32x3 s = Cross(f, UpVector);
    s.Norm();

    f32x3 u = Cross(s, f);

    Result.Ptr[0][0] = s.X;
    Result.Ptr[0][1] = u.X;
    Result.Ptr[0][2] = -f.X;
    Result.Ptr[0][3] = 0.0f;

    Result.Ptr[1][0] = s.Y;
    Result.Ptr[1][1] = u.Y;
    Result.Ptr[1][2] = -f.Y;
    Result.Ptr[1][3] = 0.0f;

    Result.Ptr[2][0] = s.Z;
    Result.Ptr[2][1] = u.Z;
    Result.Ptr[2][2] = -f.Z;
    Result.Ptr[2][3] = 0.0f;

    Result.Ptr[3][0] = -Dot(s, EyePosition);
    Result.Ptr[3][1] = -Dot(u, EyePosition);
    Result.Ptr[3][2] = Dot(f, EyePosition);
    Result.Ptr[3][3] = 1.0f;

    return Result;
}

inline f32x44 PerspectiveMatrixRH(f32 FieldOfView, f32 AspectRatio, f32 NearPlane, f32 FarPlane)
{
    f32x44 Result = f32x44(0.0f);

    f32 Radians = DegreesToRadians(FieldOfView);
    f32 Cotangent = 1.0f / tanf(Radians * 0.5f);

    Result.Ptr[0][0] = Cotangent / AspectRatio;
    Result.Ptr[1][1] = Cotangent;
    Result.Ptr[2][3] = -1.0f;
    Result.Ptr[2][2] = (NearPlane + FarPlane) / (NearPlane - FarPlane);
    Result.Ptr[3][2] = (2.0f * NearPlane * FarPlane) / (NearPlane - FarPlane);
    Result.Ptr[3][3] = 0.0f;

    return Result;
}

inline f32x44 RotateXMatrix(f32 Theta)
{
    Theta = DegreesToRadians(Theta);

    f32 c = cosf(Theta);
    f32 s = sinf(Theta);

    f32x44 Result;

    Result.Ptr[0][0] = 1.0f;
    Result.Ptr[0][1] = 0.0f;
    Result.Ptr[0][2] = 0.0f;
    Result.Ptr[0][3] = 0.0f;

    Result.Ptr[1][0] = 0.0f;
    Result.Ptr[1][1] = c;
    Result.Ptr[1][2] = s;
    Result.Ptr[1][3] = 0.0f;

    Result.Ptr[2][0] = 0.0f;
    Result.Ptr[2][1] = -s;
    Result.Ptr[2][2] = c;
    Result.Ptr[2][3] = 0.0f;

    Result.Ptr[3][0] = 0.0f;
    Result.Ptr[3][1] = 0.0f;
    Result.Ptr[3][2] = 0.0f;
    Result.Ptr[3][3] = 1.0f;

    return Result;
}

inline f32x44 RotateYMatrix(f32 Theta)
{
    Theta = DegreesToRadians(Theta);

    f32 c = cosf(Theta);
    f32 s = sinf(Theta);

    f32x44 Result;

    Result.Ptr[0][0] = c;
    Result.Ptr[0][1] = 0.0f;
    Result.Ptr[0][2] = -s;
    Result.Ptr[0][3] = 0.0f;

    Result.Ptr[1][0] = 0.0f;
    Result.Ptr[1][1] = 1.0f;
    Result.Ptr[1][2] = 0.0f;
    Result.Ptr[1][3] = 0.0f;

    Result.Ptr[2][0] = s;
    Result.Ptr[2][1] = 0.0f;
    Result.Ptr[2][2] = c;
    Result.Ptr[2][3] = 0.0f;

    Result.Ptr[3][0] = 0.0f;
    Result.Ptr[3][1] = 0.0f;
    Result.Ptr[3][2] = 0.0f;
    Result.Ptr[3][3] = 1.0f;

    return Result;
}

inline f32x44 RotateZMatrix(f32 Theta)
{
    Theta = DegreesToRadians(Theta);

    f32 c = cosf(Theta);
    f32 s = sinf(Theta);

    f32x44 Result;

    Result.Ptr[0][0] = c;
    Result.Ptr[0][1] = s;
    Result.Ptr[0][2] = 0.0f;
    Result.Ptr[0][3] = 0.0f;

    Result.Ptr[1][0] = -s;
    Result.Ptr[1][1] = c;
    Result.Ptr[1][2] = 0.0f;
    Result.Ptr[1][3] = 0.0f;

    Result.Ptr[2][0] = 0.0f;
    Result.Ptr[2][1] = 0.0f;
    Result.Ptr[2][2] = 1.0f;
    Result.Ptr[2][3] = 0.0f;

    Result.Ptr[3][0] = 0.0f;
    Result.Ptr[3][1] = 0.0f;
    Result.Ptr[3][2] = 0.0f;
    Result.Ptr[3][3] = 1.0f;

    return Result;
}

inline f32x44 RotateMatrix(f32 Theta, f32x3 RotationAxis)
{
    Theta = DegreesToRadians(Theta);
    RotationAxis.Norm();

    f32 c = cosf(Theta);
    f32 s = sinf(Theta);
    f32 d = 1.0f - c;

    f32 x = RotationAxis.X * d;
    f32 y = RotationAxis.Y * d;
    f32 z = RotationAxis.Z * d;
    f32 axay = x * RotationAxis.Y;
    f32 axaz = x * RotationAxis.Z;
    f32 ayaz = y * RotationAxis.Z;

    f32x44 Result;

    Result.Ptr[0][0] = c    + x * RotationAxis.X;
    Result.Ptr[0][1] = axay + s * RotationAxis.Z;
    Result.Ptr[0][2] = axaz - s * RotationAxis.Y;
    Result.Ptr[0][3] = 0.0f;

    Result.Ptr[1][0] = axay - s * RotationAxis.Z;
    Result.Ptr[1][1] = c    + y * RotationAxis.Y;
    Result.Ptr[1][2] = ayaz + s * RotationAxis.X;
    Result.Ptr[1][3] = 0.0f;

    Result.Ptr[2][0] = axaz + s * RotationAxis.Y;
    Result.Ptr[2][1] = ayaz - s * RotationAxis.X;
    Result.Ptr[2][2] = c    + z * RotationAxis.Z;
    Result.Ptr[2][3] = 0.0f;

    Result.Ptr[3][0] = 0.0f;
    Result.Ptr[3][1] = 0.0f;
    Result.Ptr[3][2] = 0.0f;
    Result.Ptr[3][3] = 1.0f;

    return Result;
}

//
// quaternion Functions
//

inline quaternion& quaternion::Norm()
{
    f32 d = 1.0f / sqrtf(X * X + Y * Y + Z * Z + Theta * Theta);

    Theta = Theta * d;
    X     = X * d;
    Y     = Y * d;
    Z     = Z * d;

    return *this;
}

inline quaternion EulerToQuaternion(f32 Roll, f32 Pitch, f32 Yaw)
{
    Roll  = DegreesToRadians(Roll);
    Pitch = DegreesToRadians(Pitch);
    Yaw   = DegreesToRadians(Yaw);

    f32 cy = cosf(Yaw * 0.5f);
    f32 sy = sinf(Yaw * 0.5f);
    f32 cp = cosf(Pitch * 0.5f);
    f32 sp = sinf(Pitch * 0.5f);
    f32 cr = cosf(Roll * 0.5f);
    f32 sr = sinf(Roll * 0.5f);

    quaternion Result = {};

    Result.Theta = cr * cp * cy + sr * sp * sy;
    Result.X = sr * cp * cy - cr * sp * sy;
    Result.Y = cr * sp * cy + sr * cp * sy;
    Result.Z = cr * cp * sy - sr * sp * cy;

    return Result.Norm();
}

inline quaternion EulerToQuaternion(f32 Axis[3])
{
    return EulerToQuaternion(Axis[0], Axis[1], Axis[2]);
}

inline f32x44 QuaternionToRotationMatrix(quaternion Quaternion)
{
    f32 X2 = Quaternion.X     * Quaternion.X;
    f32 Y2 = Quaternion.Y     * Quaternion.Y;
    f32 Z2 = Quaternion.Z     * Quaternion.Z;
    f32 XY = Quaternion.X     * Quaternion.Y;
    f32 XZ = Quaternion.X     * Quaternion.Z;
    f32 YZ = Quaternion.Y     * Quaternion.Z;
    f32 WX = Quaternion.Theta * Quaternion.X;
    f32 WY = Quaternion.Theta * Quaternion.Y;
    f32 WZ = Quaternion.Theta * Quaternion.Z;

    f32x44 Result = {};

    // 3x3 Rotation Matrix
    Result.C0 = { 1 - 2.0f * (Y2 + Z2),        2.0f * (XY + WZ),        2.0f * (XZ - WY), 0.0f };
    Result.C1 = {     2.0f * (XY - WZ), 1.0f - 2.0f * (X2 + Z2),        2.0f * (YZ - WX), 0.0f };
    Result.C2 = {        2.0f * (XZ + WY),     2.0f * (YZ - WX), 1.0f - 2.0f * (X2 + Y2), 0.0f };
    Result.C3 = {                       0,                    0,                       0,    1 };

    return Result;
}

//
// More Misc. / Geometric Functions
//

inline f32 F32Clamp(f32 Min, f32 Max, f32 Value)
{
    return (Value < Min) ? Min : (Value > Max) ? Max : Value;
}

inline f32 Smoothstep(f32 Point0, f32 Point1, f32 Factor)
{
    Factor = F32Clamp(0.0f, 1.0f, (Factor - Point0) / (Point1, - Point0));
    return Factor * Factor * (3 - 2 * Factor);
}

inline f32 Smootherstep(f32 Point0, f32 Point1, f32 Factor)
{
    Factor = F32Clamp(0.0f, 1.0f, (Factor - Point0) / (Point1 - Point0));
    return Factor * Factor * Factor * (Factor * (Factor * 6 - 15) + 10);
}

inline f32x3 F32x3ComputeNormal(f32x3 Point0, f32x3 Point1, f32x3 Point2)
{
    f32x3 Point10      = Point1 - Point0;
    f32x3 Point20      = Point2 - Point0;
    f32x3 NormalVector = Cross(Point10, Point20);
    return NormalVector.Norm();
}

inline f32 F32Random()
{
    return f32(rand()) / (RAND_MAX + 1.0f);
}

inline f32 F32RandomClamped(f32 Min, f32 Max)
{
    return Min + (Max - Min) * F32Random();
}

inline s32 S32RandomClamped(s32 Min, s32 Max)
{
    s32 Rand = rand();
    return Min + Rand % (Max - Min);
}


inline f32x3 F32x3Random()
{
    f32x3 Result;

    Result.X = F32Random();
    Result.Y = F32Random();
    Result.Z = F32Random();

    return Result;
}

inline f32x3 F32x3RandomClamped(f32 Min, f32 Max)
{
    f32x3 Result;

    Result.X = F32RandomClamped(Min, Max);
    Result.Y = F32RandomClamped(Min, Max);
    Result.Z = F32RandomClamped(Min, Max);

    return Result;
}

inline f32x3 F32x3RandomInUnitSphere()
{
    while (true)
    {
        f32x3 Ran = F32x3RandomClamped(-1.0f, 1.0f);
        if (Ran.LengthSq() >= 1.0f) continue;
        return Ran;
    }
}

inline f32x3 F32x3RandomInHemisphere(f32x3 Normal)
{
    f32x3 RandomInSphere = F32x3RandomInUnitSphere();
    if (Dot(RandomInSphere, Normal) > 0.0f)
    {
        return RandomInSphere;
    }
    else
    {
        return RandomInSphere * -1.0f;
    }
}

inline f32x3 F32x3RandomUnitVector()
{
    f32 A = F32RandomClamped(0, 2 * F32_PI);
    f32 Z = F32RandomClamped(-1, 1);
    f32 R = sqrtf(1 - Z * Z);
    return { R * cosf(A), R * sinf(A), Z };
}

inline f32x3  F32x3RandomInUnitDisc()
{
    while (true)
    {
        f32x3 Ran = { F32RandomClamped(-1, 1), F32RandomClamped(-1, 1), 0};
        if (Ran.LengthSq() >= 1.0f) continue;
        return Ran;
    }
}

// Assumes the Normal Vector is Normalized.
inline f32x3 ReflectVector(f32x3 Vector, f32x3 Normal)
{
    // ReflectedVector = Vector - 2 * (Vector dot Normal) * Normal
    f32 CosTheta = Dot(Vector, Normal);
    f32x3 Result = Vector - 2.0f * CosTheta * Normal;
}

inline f32x3 RefractVector(f32x3 IncidentVector, f32x3 Normal, f32 IndicesOfRefraction)
{
    // Flip the incident vector and find the cos(theta) between the Incident and Normal Vectors
    f32 CosTheta = Dot(IncidentVector * -1.0f, Normal);

    f32x3 ScaledNormal        = Normal * CosTheta;
    f32x3 ParallelVector      = (IncidentVector + ScaledNormal) * IndicesOfRefraction;
    f32x3 PerpendicularVector = Normal * (-1 * sqrtf(1.0f - ParallelVector.LengthSq()));

    return ParallelVector + PerpendicularVector;
}

// Approximates the contribution of Fresnel's Factor to Specular Reflected Light
inline f32 SchlickApproximation(f32 Cosine, f32 RefractionIndex)
{
    f32 R0 = (1 - RefractionIndex) / (1 + RefractionIndex);
    R0     = R0 * R0;

    return R0 + (1 - R0) * powf((1 - Cosine), 5);
}