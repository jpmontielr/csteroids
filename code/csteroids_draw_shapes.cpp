
enum isosceles_tip_location
{
    IsoscelesTipLocation_Top,
    IsoscelesTipLocation_Left,
    IsoscelesTipLocation_Bottom,
    IsoscelesTipLocation_Right,
};

enum right_angle_location
{
    RightAngleLocation_BottomRight,
    RightAngleLocation_BottomLeft,
    RightAngleLocation_TopRight,
    RightAngleLocation_TopLeft,
};

internal void
DrawRectangle(game_offscreen_buffer *Buffer,
              real32 RealMinX, real32 RealMinY,
              real32 RealMaxX, real32 RealMaxY,
              real32 R, real32 G, real32 B, real32 Alpha = 1.0f)
{
    int32 MinX = RoundReal32ToInt32(RealMinX);
    int32 MinY = RoundReal32ToInt32(RealMinY);        
    int32 MaxX = RoundReal32ToInt32(RealMaxX);
    int32 MaxY = RoundReal32ToInt32(RealMaxY);

    if(MinX < 0)
    {
        MinX = 0;
    }
    if(MinY < 0)
    {
        MinY = 0;
    }

    if(MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }
    if(MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }

    uint32 SourceR = RoundReal32ToUInt32(R * 255.0f);
    uint32 SourceG = RoundReal32ToUInt32(G * 255.0f);
    uint32 SourceB = RoundReal32ToUInt32(B * 255.0f);
    real32 t = Alpha;
    
    uint8 *Row = (uint8 *)Buffer->Memory + MinY*Buffer->Pitch + MinX*Buffer->BytesPerPixel;
    for(int Y = MinY;
        Y < MaxY;
        ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for(int X = MinX;
            X < MaxX;
            ++X)
        {            
            uint8 DestR = ((*Pixel >> 16) & 0xFF);
            uint8 DestG = ((*Pixel >> 8) & 0xFF);
            uint8 DestB = ((*Pixel >> 0) & 0xFF);

            real32 RealR = (real32)DestR*(1.0f - t) + t*(real32)SourceR;
            real32 RealG = (real32)DestG*(1.0f - t) + t*(real32)SourceG;
            real32 RealB = (real32)DestB*(1.0f - t) + t*(real32)SourceB;

            *Pixel = ((RoundReal32ToInt32(RealR) << 16) |
                      (RoundReal32ToInt32(RealG) << 8) |
                      (RoundReal32ToInt32(RealB) << 0));
            ++Pixel;
        }
        Row += Buffer->Pitch;
    }
}

internal void
DrawRectangle(game_offscreen_buffer *Buffer, v2 vMin, v2 vMax,
              real32 R, real32 G, real32 B, real32 Alpha = 1.0f)
{
    DrawRectangle(Buffer, vMin.X, vMin.Y, vMax.X, vMax.Y, R, G, B, Alpha);
}

internal void
DrawLine(game_offscreen_buffer *Buffer, real32 RealX1, real32 RealY1, real32 RealX2, real32 RealY2, uint32 Color)
{
    real32 LineRise = RealY2 - RealY1;
    real32 LineRun = RealX2 - RealX1;

    if(LineRun != 0.0f)
    {
        real32 LineSlope = LineRise / LineRun;
        real32 b = RealY1 - RealX1*LineSlope;

        int32 X1 = RoundReal32ToInt32(RealX1);
        int32 Y1 = RoundReal32ToInt32(RealY1);
        
        int32 X2 = RoundReal32ToInt32(RealX2);
        int32 Y2 = RoundReal32ToInt32(RealY2);

        int32 MinX = Minimum(X1, X2);
        int32 MinY = Minimum(Y1, Y2);
        int32 MaxX = Maximum(X1, X2);
        int32 MaxY = Maximum(Y1, Y2);

        if(MinX < 0)
        {
            MinX = 0;
        }
        if(MinY < 0)
        {
            MinY = 0;
        }

        if(MaxX > Buffer->Width)
        {
            MaxX = Buffer->Width;
        }
        if(MaxY > Buffer->Height)
        {
            MaxY = Buffer->Height;
        }
        
        uint8 *Row = (uint8 *)Buffer->Memory + MinY*Buffer->Pitch + MinX*Buffer->BytesPerPixel;
        for(int Y = MinY;
            Y < MaxY;
            ++Y)
        {
            uint32 *Pixel = (uint32 *)Row;
            for(int X = MinX;
                X < MaxX;
                ++X)
            {
                if((real32)Y == (LineSlope*(real32)X + b))
                {
                    *Pixel = Color;
                }
                ++Pixel;
            }
            Row += Buffer->Pitch;
        }
    }
}

internal void
DrawRightTriangle(game_offscreen_buffer *Buffer,
                  real32 RealMinX, real32 RealMinY, real32 RealMaxX, real32 RealMaxY,
                  uint32 Color, right_angle_location RightAngleLocation = RightAngleLocation_BottomRight)
{
    real32 RealY1 = 0;
    real32 RealY2 = 0;
    if((RightAngleLocation == RightAngleLocation_TopRight) ||
       (RightAngleLocation == RightAngleLocation_BottomLeft))
    {
        RealY1 = Minimum(RealMinY, RealMaxY);
        RealY2 = Maximum(RealMinY, RealMaxY);
    }
    else if((RightAngleLocation == RightAngleLocation_TopLeft) ||
            (RightAngleLocation == RightAngleLocation_BottomRight))
    {
        RealY1 = Maximum(RealMinY, RealMaxY);
        RealY2 = Minimum(RealMinY, RealMaxY);
    }
    else
    {
        Assert(!"Invalid right angle location");
    }
    
    real32 DiagonalRise = RealY2 - RealY1;
    real32 DiagonalRun = RealMaxX - RealMinX;

    if(DiagonalRun != 0.0f)
    {
        real32 DiagonalSlope = DiagonalRise / DiagonalRun;
        real32 b = RealY1 - RealMinX*DiagonalSlope;

        int32 MinX = (int32)RoundReal32ToInt32(RealMinX);
        int32 MinY = (int32)RoundReal32ToInt32(RealMinY);
        
        int32 MaxX = (int32)RoundReal32ToInt32(RealMaxX);
        int32 MaxY = (int32)RoundReal32ToInt32(RealMaxY);

        if(MinX < 0)
        {
            MinX = 0;
        }
        if(MinY < 0)
        {
            MinY = 0;
        }

        if(MaxX > Buffer->Width)
        {
            MaxX = Buffer->Width;
        }
        if(MaxY > Buffer->Height)
        {
            MaxY = Buffer->Height;
        }
        
        uint8 *Row = (uint8 *)Buffer->Memory + MinY*Buffer->Pitch + MinX*Buffer->BytesPerPixel;
        for(int Y = MinY;
            Y < MaxY;
            ++Y)
        {
            uint32 *Pixel = (uint32 *)Row;
            for(int X = MinX;
                X < MaxX;
                ++X)
            {
                if((RightAngleLocation == RightAngleLocation_BottomLeft) ||
                   (RightAngleLocation == RightAngleLocation_BottomRight))
                {
                    if((real32)Y >= (DiagonalSlope*(real32)X + b))
                    {
                        *Pixel = Color;
                    }
                }
                else if((RightAngleLocation == RightAngleLocation_TopRight) ||
                        (RightAngleLocation == RightAngleLocation_TopLeft))
                {
                    if((real32)Y <= (DiagonalSlope*(real32)X + b))
                    {
                        *Pixel = Color;
                    }
                }
                ++Pixel;
            }
            Row += Buffer->Pitch;
        }
    }
}

internal void
DrawIsoscelesTriangle(game_offscreen_buffer *Buffer, real32 MinX, real32 MinY, real32 MaxX, real32 MaxY,
                      uint32 Color, isosceles_tip_location TipLocation = IsoscelesTipLocation_Top)
{
    right_angle_location Triangle1AngleLocation = {};
    right_angle_location Triangle2AngleLocation = {};
    
    if(TipLocation == IsoscelesTipLocation_Top)
    {
        Triangle1AngleLocation = RightAngleLocation_BottomRight;
        Triangle2AngleLocation = RightAngleLocation_BottomLeft;
    }
    else if(TipLocation == IsoscelesTipLocation_Bottom)
    {
        Triangle1AngleLocation = RightAngleLocation_TopRight;
        Triangle2AngleLocation = RightAngleLocation_TopLeft;
    }
    else if(TipLocation == IsoscelesTipLocation_Left)
    {
        Triangle1AngleLocation = RightAngleLocation_BottomRight;
        Triangle2AngleLocation = RightAngleLocation_TopRight;
    }
    else if(TipLocation == IsoscelesTipLocation_Right)
    {
        Triangle1AngleLocation = RightAngleLocation_BottomLeft;
        Triangle2AngleLocation = RightAngleLocation_TopLeft;
    }

    if((TipLocation == IsoscelesTipLocation_Top) ||
       (TipLocation == IsoscelesTipLocation_Bottom))
    {
        right_angle_location LeftSideTriangleAngleLocation = Triangle1AngleLocation;
        right_angle_location RightSideTriangleAngleLocation = Triangle2AngleLocation;
        
        real32 LeftSideTriangleMinX = MinX;
        real32 LeftSideTriangleMinY = MinY;
        real32 LeftSideTriangleMaxX = LeftSideTriangleMinX + (0.5f*(MaxX - MinX));
        real32 LeftSideTriangleMaxY = MaxY;
        DrawRightTriangle(Buffer,
                          LeftSideTriangleMinX, LeftSideTriangleMinY,
                          LeftSideTriangleMaxX, LeftSideTriangleMaxY,
                          Color, LeftSideTriangleAngleLocation);
        real32 RightSideTriangleMinX = LeftSideTriangleMaxX;
        real32 RightSideTriangleMinY = MinY;
        real32 RightSideTriangleMaxX = MaxX;
        real32 RightSideTriangleMaxY = MaxY;
        DrawRightTriangle(Buffer,
                          RightSideTriangleMinX, RightSideTriangleMinY,
                          RightSideTriangleMaxX, RightSideTriangleMaxY,
                          Color, RightSideTriangleAngleLocation);
    }
    else if((TipLocation == IsoscelesTipLocation_Left) ||
            (TipLocation == IsoscelesTipLocation_Right))
    {
        right_angle_location TopTriangleAngleLocation = Triangle1AngleLocation;
        right_angle_location BottomTriangleAngleLocation = Triangle2AngleLocation;

        real32 TopTriangleMinX = MinX;
        real32 TopTriangleMinY = MinY;
        real32 TopTriangleMaxX = MaxX;
        real32 TopTriangleMaxY = TopTriangleMinY + (0.5f*(MaxY - MinY));
        DrawRightTriangle(Buffer,
                          TopTriangleMinX, TopTriangleMinY,
                          TopTriangleMaxX, TopTriangleMaxY,
                          Color, TopTriangleAngleLocation);
        real32 BottomTriangleMinX = MinX;
        real32 BottomTriangleMinY = TopTriangleMaxY;
        real32 BottomTriangleMaxX = MaxX;
        real32 BottomTriangleMaxY = MaxY;
        DrawRightTriangle(Buffer,
                          BottomTriangleMinX, BottomTriangleMinY,
                          BottomTriangleMaxX, BottomTriangleMaxY,
                          Color, BottomTriangleAngleLocation);        
    }
    else
    {
        // ???
    }
}

#if 0

internal void
DrawTest(game_offscreen_buffer *Buffer)
{
    // NOTE(jp): Draw line test
    real32 X1 = 100.0f;
    real32 Y1 = 100.0f;
                
    real32 X2 = 200.0f;
    real32 Y2 = 200.0f;

    DrawLine(Buffer, X1, Y1, X2, Y2, 0xFF0000);
    DrawLine(Buffer, X2, Y2, X2*2.0f - X1, Y1, 0x00FF00);
    DrawLine(Buffer, 300.0f, 300.0f, 200.0f, 500.0f, 0x0000FF);
    // NOTE(jp): Draw triangle test
    real32 TriangleBase = 200.0f;
    real32 TriangleHeight = 500.0f;
    real32 TriangleMinX = 200.0f;
    real32 TriangleMinY = 200.0f;
    real32 TriangleMaxX = TriangleMinX + TriangleBase;
    real32 TriangleMaxY = TriangleMinY + TriangleHeight;

    DrawRightTriangle(Buffer,
                      TriangleMinX, TriangleMinY,
                      TriangleMaxX, TriangleMaxY,
                      0xFF0000, RightAngleLocation_TopRight);
    DrawRightTriangle(Buffer,
                      TriangleMaxX, TriangleMinY,
                      TriangleMaxX*2.0f - TriangleMinX, TriangleMaxY,
                      0x0000FF, RightAngleLocation_TopLeft);
    DrawRightTriangle(Buffer,
                      TriangleMinX, TriangleMinY - (TriangleMaxY - TriangleMinY),
                      TriangleMaxX, TriangleMinY,
                      0x00FF00, RightAngleLocation_BottomRight);
    DrawRightTriangle(Buffer,
                      TriangleMaxX, TriangleMinY - (TriangleMaxY - TriangleMinY),
                      TriangleMaxX*2.0f - TriangleMinX, TriangleMinY,
                      0xFFFFFF, RightAngleLocation_BottomLeft);
}

// NOTE(jp): :')
internal void
DrawPlayer(game_offscreen_buffer *Buffer, real32 PlayerMinX, real32 PlayerMinY, real32 PlayerMaxX, real32 PlayerMaxY,
           isosceles_tip_location TipLocation = IsoscelesTipLocation_Top)
{
    real32 OuterTriangleBase = PlayerMaxX - PlayerMinX;
    real32 OuterTriangleHeight = PlayerMaxY - PlayerMinY;
    
    real32 OuterTriangleMinX = PlayerMinX;
    real32 OuterTriangleMinY = PlayerMinY;
    real32 OuterTriangleMaxX = OuterTriangleMinX + OuterTriangleBase;
    real32 OuterTriangleMaxY = OuterTriangleMinY + OuterTriangleHeight;
                
    DrawIsoscelesTriangle(Buffer,
                          OuterTriangleMinX, OuterTriangleMinY,
                          OuterTriangleMaxX, OuterTriangleMaxY,
                          0xFFFFFF, TipLocation);

    real32 InnerTriangleMinX = 0;
    real32 InnerTriangleMaxX = 0;
    real32 InnerTriangleMinY = 0;
    real32 InnerTriangleMaxY = 0;
    if((TipLocation == IsoscelesTipLocation_Top) ||
       (TipLocation == IsoscelesTipLocation_Bottom))
    {
        real32 Rise = OuterTriangleMaxY - OuterTriangleMinY;
        real32 Run = 0.5f*(OuterTriangleMaxX - OuterTriangleMinX);
        real32 XDelta = (OuterTriangleMaxX - OuterTriangleMinX)*0.08f;
        real32 InnerTriangleHeight = OuterTriangleHeight - XDelta*(Rise/Run);

        InnerTriangleMinX = OuterTriangleMinX + XDelta;
        InnerTriangleMaxX = OuterTriangleMaxX - XDelta;
        if(TipLocation == IsoscelesTipLocation_Top)
        {
            InnerTriangleMaxY = OuterTriangleMaxY;
            InnerTriangleMinY = InnerTriangleMaxY - InnerTriangleHeight;
        }
        else if(TipLocation == IsoscelesTipLocation_Bottom)
        {
            InnerTriangleMinY = OuterTriangleMinY;
            InnerTriangleMaxY = OuterTriangleMinY + InnerTriangleHeight;
        }
    }
    else if((TipLocation == IsoscelesTipLocation_Right) ||
            (TipLocation == IsoscelesTipLocation_Left))
    {   
        real32 Rise = 0.5f*(OuterTriangleMaxY - OuterTriangleMinY);
        real32 Run = (OuterTriangleMaxX - OuterTriangleMinX);
        real32 YDelta = (OuterTriangleMaxY - OuterTriangleMinY)*0.08f;
        real32 InnerTriangleWidth = OuterTriangleBase - YDelta*(Run/Rise);

        InnerTriangleMinY = OuterTriangleMinY + YDelta;
        InnerTriangleMaxY = OuterTriangleMaxY - YDelta;
        if(TipLocation == IsoscelesTipLocation_Right)
        {        
            InnerTriangleMinX = OuterTriangleMinX;
            InnerTriangleMaxX = OuterTriangleMinX + InnerTriangleWidth;
        }
        else if(TipLocation == IsoscelesTipLocation_Left)
        {
            InnerTriangleMinX = OuterTriangleMaxX - InnerTriangleWidth;
            InnerTriangleMaxX = OuterTriangleMaxX;
        }
    }
    
    DrawIsoscelesTriangle(Buffer,
                          InnerTriangleMinX, InnerTriangleMinY,
                          InnerTriangleMaxX, InnerTriangleMaxY,
                          0x000000, TipLocation);
}

inline void
DrawPlayer(game_offscreen_buffer *Buffer, v2 PlayerMin, v2 PlayerMax, isosceles_tip_location TipLocation = IsoscelesTipLocation_Top)
{
    DrawPlayer(Buffer, PlayerMin.X, PlayerMin.Y, PlayerMax.X, PlayerMax.Y, TipLocation);
}

#endif

