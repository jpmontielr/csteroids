#if !defined(CSTEROIDS_MATH_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   $Notice: (C) Copyright 2014 by Molly Rocket, Inc. All Rights Reserved. $
   ======================================================================== */

#include <math.h>

inline int32
RoundReal32ToInt32(real32 RealValue)
{
    int32 Result = (int32)(RealValue + 0.5f);
    return(Result);
}

inline uint32
RoundReal32ToUInt32(real32 RealValue)
{
    uint32 Result = (uint32)(RealValue + 0.5f);
    return(Result);
}

struct v2
{
    real32 X;
    real32 Y;
};

inline bool32
AreEqualV2(v2 A, v2 B)
{
    bool32 Result = ((A.X == B.X) && (A.Y == B.Y));
    return(Result);
}

inline v2
V2(real32 X, real32 Y)
{
    v2 Result = {};
    Result.X = X;
    Result.Y = Y;
    return(Result);
}

inline v2
operator+(v2 A, v2 B)
{
    v2 Result = {};
    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;
    return(Result);
}

inline v2 &
operator+=(v2 &A, v2 B)
{
    A = A + B;
    return(A);
}

inline v2
operator-(v2 A, v2 B)
{
    v2 Result = {};
    Result.X = A.X - B.X;
    Result.Y = A.Y - B.Y;
    return(Result);
}

inline v2
operator-(v2 A)
{
    v2 Result = {};
    Result.X = -A.X;
    Result.Y = -A.Y;
    return(Result);
}

 v2 &
operator-=(v2 &A, v2 B)
{
    A = A - B;
    return(A);
}

inline v2
operator*(real32 A, v2 B)
{
    v2 Result = {};
    Result.X = A*B.X;
    Result.Y = A*B.Y;
    return(Result);
}

inline v2
operator*(v2 B, real32 A)
{
    v2 Result = A*B;
    return(Result);
}

inline v2 &
operator*=(v2 &B, real32 A)
{
    B = A * B;
    return(B);
}

struct rectangle2
{
    v2 Min;
    v2 Max;
};

inline v2
GetCenter(rectangle2 Rect)
{
    v2 Result = 0.5f*(Rect.Max + Rect.Min);
    return(Result);
}

inline rectangle2
RectCenterHalfDim(v2 Center, v2 HalfDim)
{
    rectangle2 Result = {};
    Result.Min = Center - HalfDim;
    Result.Max = Center + HalfDim;
    return(Result);
}

inline rectangle2
RectCenterDim(v2 Center, v2 Dim)
{
    rectangle2 Result = RectCenterHalfDim(Center, 0.5f*Dim);
    return(Result);
}

inline rectangle2
RectMinDim(v2 Min, v2 Dim)
{
    rectangle2 Result = {};
    Result.Min = Min;
    Result.Max = Min + Dim;
    return(Result);
}

inline bool32
IsInRectangle(rectangle2 Rectangle, v2 Test)
{
    bool32 Result = ((Test.X >= Rectangle.Min.X) &&
                     (Test.X <= Rectangle.Max.X) &&
                     (Test.Y >= Rectangle.Min.Y) &&
                     (Test.Y <= Rectangle.Max.Y));
    return(Result);
}

inline real32
Squared(real32 Real32)
{
    real32 Result = Real32*Real32;
    return(Result);
}

inline real32
Inner(v2 A, v2 B)
{
    real32 Result = 0.0f;

    Result = (A.X*B.X + A.Y*B.Y);
    return(Result);
}

inline real32
LengthSq(v2 A)
{
    real32 Result = Inner(A, A);
    
    return(Result);
}

inline v2
NormalizeV2(v2 A)
{
    real32 C = 1.0f/sqrtf(LengthSq(A));
    v2 Result = A*C;
    
    return(Result);
}

inline real32
Square(real32 A)
{
    real32 Result = A*A;
    return(Result);
}

#define CSTEROIDS_MATH_H
#endif
