// TODO(jp): Rotations
// TODO(jp): Finish up the fonts situation

#include "csteroids_math.h"
#include "csteroids.h"
#include "csteroids_entity.cpp"
#include "csteroids_draw_shapes.cpp"

#define STB_TRUETYPE_IMPLEMENTATION 1 
#include "stb_truetype.h"

inline bool32
DoRectanglesCollide(real32 RectAMinX, real32 RectAMinY, real32 RectAMaxX, real32 RectAMaxY,
                    real32 RectBMinX, real32 RectBMinY, real32 RectBMaxX, real32 RectBMaxY)
{
    bool32 Result = !((RectAMinX > RectBMaxX) ||
                      (RectAMaxX < RectBMinX) ||
                      (RectAMinY > RectBMaxY) ||
                      (RectAMaxY < RectBMinY));
    return(Result);
}

inline bool32
DoRectanglesCollide(v2 RectAMin, v2 RectAMax, v2 RectBMin, v2 RectBMax)
{
    bool32 Result = DoRectanglesCollide(RectAMin.X, RectAMin.Y, RectAMax.X, RectAMax.Y,
                                        RectBMin.X, RectBMin.Y, RectBMax.X, RectBMax.Y);
    return(Result);
}

inline bool32
DoRectanglesCollide(rectangle2 RectA, rectangle2 RectB)
{
    bool32 Result = DoRectanglesCollide(RectA.Min, RectA.Max, RectB.Min, RectB.Max);
    return(Result);
}

inline v2
GetPixelCoord(game_offscreen_buffer *Buffer, v2 MetersCoord, real32 MetersToPixels)
{
    v2 Result = {};
    Result.X = MetersCoord.X*MetersToPixels;
    Result.Y = (real32)Buffer->Height - MetersCoord.Y*MetersToPixels;

    return(Result);
}

#pragma pack(push, 1)
struct bmp_header
{
    uint16 Type;
    uint32 FileSize;
    uint16 Reserved1;
    uint16 Reserved2;
    uint32 BitmapOffset;

    uint32 Size;
    int32 Width;
    int32 Height;
    uint16 Planes;
    uint16 BitsPerPixel;

    uint32 Compression;
    uint32 SizeOfBitmap;
    int32 HorzResolution;
    int32 VertResolution;
    uint32 ColorsUsed;
    uint32 ColorsImportant;

    uint32 RedMask;
    uint32 GreenMask;
    uint32 BlueMask;
};
#pragma pack(pop)

struct find_least_significant_set_bit_result
{
    bool32 Found;
    uint32 Index;
};
internal find_least_significant_set_bit_result
FindLeastSignificantSetBit(uint32 Mask)
{
    find_least_significant_set_bit_result Result = {};
    for(uint32 BitIndex = 0;
        BitIndex < 32;
        ++BitIndex)
    {
        uint32 Bit = (1 << BitIndex);
        if((Mask & Bit) == Bit)
        {
            Result.Index = BitIndex;
            Result.Found = true;
            break;
        }
    }

    return(Result);
}

internal loaded_bitmap
LoadBMP(memory_arena *Arena, char *FileName)
{
    loaded_bitmap Result = {};
    read_entire_file_result BMPFile = PlatformReadEntireFile(FileName, Arena);
    if(BMPFile.FileMemory)
    {
        bmp_header *BMPHeader = (bmp_header *)BMPFile.FileMemory;
        Assert(BMPHeader->Compression == 3);

        uint32 AlphaMask = ~(BMPHeader->RedMask | BMPHeader->GreenMask | BMPHeader->BlueMask);        
        find_least_significant_set_bit_result AlphaShiftResult =
            FindLeastSignificantSetBit(AlphaMask);
        
        find_least_significant_set_bit_result RedShiftResult =
            FindLeastSignificantSetBit(BMPHeader->RedMask);
        find_least_significant_set_bit_result GreenShiftResult =
            FindLeastSignificantSetBit(BMPHeader->GreenMask);
        find_least_significant_set_bit_result BlueShiftResult =
            FindLeastSignificantSetBit(BMPHeader->BlueMask);
        
        if(AlphaShiftResult.Found && RedShiftResult.Found &&
           GreenShiftResult.Found && BlueShiftResult.Found)
        {
            Result.Memory = (uint8 *)BMPFile.FileMemory + BMPHeader->BitmapOffset;
            Result.Width = BMPHeader->Width;
            Result.Height = BMPHeader->Height;
            
            uint32 AlphaShift = AlphaShiftResult.Index;
            uint32 RedShift = RedShiftResult.Index;
            uint32 GreenShift = GreenShiftResult.Index;
            uint32 BlueShift = BlueShiftResult.Index;
            
            uint32 *SourceDest = (uint32 *)Result.Memory;
            for(uint32 Y = 0;
                Y < Result.Height;
                ++Y)
            {
                for(uint32 X = 0;
                    X < Result.Width;
                    ++X)
                {
                    *SourceDest++ = ((((*SourceDest >> AlphaShift) & 0xFF) << 24) |
                                     (((*SourceDest >> RedShift) & 0xFF) << 16) |
                                     (((*SourceDest >> GreenShift) & 0xFF) << 8) |
                                     (((*SourceDest >> BlueShift) & 0xFF) << 0));
                }
            }
        }
    }

    return(Result);
}

internal void
DrawBitmap(game_offscreen_buffer *Buffer, loaded_bitmap *Bitmap, real32 RealMinX, real32 RealMinY)
{
    int32 MinX = RoundReal32ToInt32(RealMinX);
    int32 MinY = RoundReal32ToInt32(RealMinY);        
    int32 MaxX = RoundReal32ToInt32(RealMinX + (real32)Bitmap->Width);
    int32 MaxY = RoundReal32ToInt32(RealMinY + (real32)Bitmap->Height);

    int OffsetX = 0;
    int OffsetY = 0;
    if(MinX < 0)
    {
        OffsetX = -MinX;
        MinX = 0;
    }
    if(MinY < 0)
    {
        OffsetY = -MinY;
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

    int BytesPerPixel = 4;
    int BitmapPitch = BytesPerPixel*Bitmap->Width;

    uint8 *SourceRow = ((uint8 *)Bitmap->Memory +
                        Bitmap->Width*(Bitmap->Height - 1)*BytesPerPixel +
                        OffsetX*BytesPerPixel - OffsetY*BitmapPitch);

    uint8 *DestRow = (uint8 *)Buffer->Memory + MinY*Buffer->Pitch + MinX*Buffer->BytesPerPixel;
    for(int32 Y = MinY;
        Y < MaxY;
        ++Y)
    {
        uint32 *SourcePixel = (uint32 *)SourceRow;
        uint32 *DestPixel= (uint32 *)DestRow;
        for(int32 X = MinX;
            X < MaxX;
            ++X)
        {
            uint8 SourceA = ((*SourcePixel >> 24) & 0xFF);
            uint8 SourceR = ((*SourcePixel >> 16) & 0xFF);
            uint8 SourceG = ((*SourcePixel >> 8) & 0xFF);
            uint8 SourceB = ((*SourcePixel >> 0) & 0xFF);

            real32 t = (real32)SourceA / 255.0f;
            
            uint8 DestR = ((*DestPixel >> 16) & 0xFF);
            uint8 DestG = ((*DestPixel >> 8) & 0xFF);
            uint8 DestB = ((*DestPixel >> 0) & 0xFF);

            real32 RealR = (real32)DestR*(1.0f - t) + t*(real32)SourceR;
            real32 RealG = (real32)DestG*(1.0f - t) + t*(real32)SourceG;
            real32 RealB = (real32)DestB*(1.0f - t) + t*(real32)SourceB;
            
            *DestPixel = ((RoundReal32ToInt32(RealR) << 16) |
                          (RoundReal32ToInt32(RealG) << 8) |
                          (RoundReal32ToInt32(RealB) << 0));
            ++DestPixel;
            ++SourcePixel;
        }
        SourceRow -= BitmapPitch;
        DestRow += Buffer->Pitch;
    }
}

inline void
DEBUGDrawBitmapBox(game_offscreen_buffer *Buffer, v2 P, loaded_bitmap *Bitmap,
                   real32 R, real32 G, real32 B, real32 Alpha = 1.0f)
{
    rectangle2 BitmapRect = RectCenterDim(P, V2((real32)Bitmap->Width, (real32)Bitmap->Height));
    DrawRectangle(Buffer, BitmapRect.Min, BitmapRect.Max, R, G, B, Alpha);
}

internal void
DEBUGDrawCollisionBox(game_state *GameState, game_offscreen_buffer *Buffer,
                      collision_box *CollisionBox, uint32 CollisionBoxCount,
                      real32 R, real32 G, real32 B, real32 Alpha = 1.0f)
{    
    for(uint32 CollisionBoxIndex = 0;
        CollisionBoxIndex < CollisionBoxCount;
        ++CollisionBoxIndex)
    {
        collision_box *ThisBox = CollisionBox + CollisionBoxIndex;
        v2 CollisionPInPixels = GetPixelCoord(Buffer, ThisBox->Center, GameState->MetersToPixels);
        rectangle2 CollisionRect =
            RectCenterDim(CollisionPInPixels, ThisBox->Dimension*GameState->MetersToPixels);

        DrawRectangle(Buffer, CollisionRect.Min, CollisionRect.Max, R, G, B, Alpha);
    }
}

internal loaded_bitmap 
LoadFontCodePointBitmap(game_state *GameState, game_offscreen_buffer *Buffer,
                        void *TTFFile, char CodePoint, real32 HeightInPixels)
{
    loaded_bitmap Result = {};
    
    stbtt_fontinfo FontInfo = {};
    stbtt_InitFont(&FontInfo, (uint8 *)TTFFile,
                   stbtt_GetFontOffsetForIndex((uint8 *)TTFFile, 0));

    int CodePointBitmapWidth;
    int CodePointBitmapHeight;

    uint8 *CodePointBitmapMemory =
        stbtt_GetCodepointBitmap(&FontInfo, 0, stbtt_ScaleForPixelHeight(&FontInfo, HeightInPixels),
                                 CodePoint, &CodePointBitmapWidth, &CodePointBitmapHeight, 0,0);
        
    int CodePointBitmapPitch = CodePointBitmapWidth*Buffer->BytesPerPixel;
    
    Result.Width = CodePointBitmapWidth;
    Result.Height = CodePointBitmapHeight;
    int ResultMemorySize = (Result.Width*Result.Height)*Buffer->BytesPerPixel;
    Result.Memory = PushArray(&GameState->MemoryArena, uint32, ResultMemorySize);

    uint8 *Source = (uint8 *)CodePointBitmapMemory;
    uint8 *Dest = (uint8 *)Result.Memory + Result.Height*CodePointBitmapPitch;
                            
    for(uint32 Y = 0;
        Y < Result.Height;
        ++Y)
    {
        uint32 *Pixel = (uint32 *)Dest;
        for(uint32 X = 0;
            X < Result.Width;
            ++X)
        {
            uint8 Alpha = *Source++;
            *Pixel++ = ((Alpha << 24) | (Alpha << 16) | (Alpha << 8) | Alpha);
        }
        Dest -= CodePointBitmapPitch;
    }
    
    stbtt_FreeBitmap(CodePointBitmapMemory, 0);
    
    return(Result);
}

internal real32
GetHalfCircleX(real32 Radians)
{
    real32 Radius = 1.0f;
    real32 Result = 0.0f;

    int HalfTurns = (int)(Radians / Pi32);
    real32 RunningX = 2.0f*Radius*(real32)HalfTurns;
    
    real32 Sine = sinf(Radians);
    real32 Cosine = cosf(Radians);
    
    int SineSign = (*(uint32 *)&Sine & (1 << 31));

    real32 X = 0.0f;
    // NOTE(jp): We multiply the cosine with the radius in case it's not a unit circle
    if(Sine >= 0.0)
    {
        X = Radius - (Cosine*Radius);
    }
    else
    {
        X = Radius + (Cosine*Radius);
    }

    Result = RunningX + X;

    return(Result);
}

internal real32
GetHalfCircleRadiansFromX(real32 RunningX)
{
    real32 Result = 0.0f;
    int HalfCirclesIn = (int)(RunningX / 2.0f);
    real32 X = RunningX - (real32)HalfCirclesIn*2.0f;
    Assert(X <= 2.0f);
    Assert(X >= 0.0f);
    real32 Normal = 1.0f - X;
    real32 Cosine = 0.0f;
    if(HalfCirclesIn % 2)
    {
        Cosine = -Normal;
        Result = (2.0f*Pi32) - acosf(Cosine);
    }
    else
    {
        Cosine = Normal;
        Result = acosf(Cosine);
    }
    return(Result);
}

internal int16
GetSampleValue(sound_wave *Wave)
{
    int16 ToneVolume = 3000;
    int16 SampleValue = 0;

    // square wave
    if(Wave->Type == 0)
    {
        int QuarterOfWavePeriod = ((Wave->RunningSampleIndex++ / (Wave->Period/4)) % 4);
        if(QuarterOfWavePeriod == 0)
        {
            SampleValue = ToneVolume;
        }
        else if(QuarterOfWavePeriod == 1)
        {
            SampleValue = ToneVolume;
        }
        else if(QuarterOfWavePeriod == 2)
        {
            SampleValue = -ToneVolume;                
        }
        else if(QuarterOfWavePeriod == 3)
        {
            SampleValue = -ToneVolume;                
        }
    }
    else if(Wave->Type == 1) // sine wave
    {
        real32 SineValue = sinf(Wave->t);
        SampleValue = (int16)(SineValue*(real32)ToneVolume);
        Wave->t += (2.0f*Pi32) * 1.0f / (real32)Wave->Period;
        if(Wave->t >= 2.0f*Pi32)
        {
            Wave->t -= 2.0f*Pi32;
        }
    }
    else if(Wave->Type == 2) // half circle wave
    {
        real32 HalfCircleRadians = GetHalfCircleRadiansFromX(Wave->t);
        real32 HalfCircleValue = sinf(HalfCircleRadians);
        SampleValue = (int16)(HalfCircleValue*(real32)ToneVolume);
        Wave->t += (4.0f * (1.0f / (real32)Wave->Period));
        if(Wave->t >= 4.0f)
        {
            Wave->t -= 4.0f;
        }
    }
    
    return(SampleValue);
}

internal void
WriteWaveInSoundBuffer(sound_wave *Wave, game_sound_output *SoundOutput)
{    
    int16 *SampleOut = SoundOutput->Samples1;    
    for(uint32 SampleIndex = 0;
        SampleIndex < SoundOutput->SampleCount1;
        ++SampleIndex)
    {
        int16 SampleValue = GetSampleValue(Wave);

        int16 Temp = 
        *SampleOut++ = (*SampleOut + SampleValue);
        *SampleOut++ = (*SampleOut + SampleValue);
    }

    SampleOut = SoundOutput->Samples2;
    for(uint32 SampleIndex = 0;
        SampleIndex < SoundOutput->SampleCount2;
        ++SampleIndex)
    {
        int16 SampleValue = GetSampleValue(Wave);
        
        *SampleOut++ = (*SampleOut + SampleValue);
        *SampleOut++ = (*SampleOut + SampleValue);
    }
}

internal void
ClearSoundBuffer(game_sound_output *SoundOutput)
{    
    int16 *SampleOut = SoundOutput->Samples1;    
    for(uint32 SampleIndex = 0;
        SampleIndex < SoundOutput->SampleCount1;
        ++SampleIndex)
    {
        *SampleOut++ = 0;
        *SampleOut++ = 0;
    }

    SampleOut = SoundOutput->Samples2;
    for(uint32 SampleIndex = 0;
        SampleIndex < SoundOutput->SampleCount2;
        ++SampleIndex)
    {
        *SampleOut++ = 0;
        *SampleOut++ = 0;
    }
}

internal void
CopyWAVToSoundBuffer(game_sound_output *SoundOutput, loaded_wav *WAVFile)
{
    uint32 *WAVPos = (uint32 *)WAVFile->Samples + WAVFile->SampleIndex;
    int16 *Source = (int16 *)WAVPos;
    
    int16 *Dest = SoundOutput->Samples1;    
    for(uint32 SampleIndex = 0;
        SampleIndex < SoundOutput->SampleCount1;
        ++SampleIndex)
    {
        *Dest++ = *Source++;
        *Dest++ = *Source++;

        ++WAVFile->SampleIndex;
        if(WAVFile->SampleIndex >= WAVFile->SampleCount)
        {
            Source = WAVFile->Samples;
            WAVFile->SampleIndex = 0;
        }
    }

    Dest = SoundOutput->Samples2;
    for(uint32 SampleIndex = 0;
        SampleIndex < SoundOutput->SampleCount2;
        ++SampleIndex)
    {
        *Dest++ = *Source++;
        *Dest++ = *Source++;

        ++WAVFile->SampleIndex;
        if(WAVFile->SampleIndex >= WAVFile->SampleCount)
        {
            Source = WAVFile->Samples;
            WAVFile->SampleIndex = 0;
        }
    }
}

inline uint32
Power(uint32 Number, uint32 Exponent)
{
    int Result = 1;
    
    for(uint32 ExponentIndex = 0;
        ExponentIndex < Exponent;
        ++ExponentIndex)
    {
        Result *= Number;
    }
    return(Result);
}

internal void
GameUpdateAndRender(game_offscreen_buffer *Buffer, game_input *Input, game_memory *Memory, game_sound_output *Sound)
{
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);    
    
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    v2 InitialPlayerP = {};

    if(!Memory->IsInitialized)
    {        
        InitializeArena(&GameState->MemoryArena,
                        (uint8 *)Memory->PermanentStorage + sizeof(game_state),
                        Memory->PermanentStorageSize - sizeof(game_state));

        sound_wave *Wave = &GameState->ShootingSound;
        Wave->Type = 0;
        // middle f = 349.228
        Wave->ToneHz = 87; // f,,
        Wave->Period = Sound->SamplesPerSecond / Wave->ToneHz;
        Wave->t = 0.0f;
        
        char *TTFFileName = "c:/windows/fonts/times.ttf";
        read_entire_file_result TTFFile = PlatformReadEntireFile(TTFFileName, &GameState->MemoryArena);

        char CodePointsToLoad[] = "0123456789";
        for(uint32 CodePointIndex = 0;
            CodePointIndex < ArrayCount(GameState->FontGlyphs);
            ++CodePointIndex)
        {
            char CodePoint = CodePointsToLoad[CodePointIndex];
            GameState->FontGlyphs[CodePointIndex] =
                LoadFontCodePointBitmap(GameState, Buffer, TTFFile.FileMemory, CodePoint, 32.0f);
        }
        // TODO(jp): Free the ttf file memory

        read_entire_file_result WAVFile = PlatformReadEntireFile("square_waves.wav", &GameState->MemoryArena);        
        wav_header *WAVHeader = (wav_header *)WAVFile.FileMemory;
        GameState->WAVFile = {};
        GameState->WAVFile.Samples = &WAVHeader->Samples;
        GameState->WAVFile.SampleCount = WAVHeader->dataChunkSize / (sizeof(int16)*2);
        GameState->WAVFile.SampleIndex = 0;
        
        GameState->BackgroundBitmap = LoadBMP(&GameState->MemoryArena, "background.bmp");
        
        GameState->SpaceshipBitmaps[0] = LoadBMP(&GameState->MemoryArena, "../data/spaceship_up.bmp");
        GameState->SpaceshipBitmaps[1] = LoadBMP(&GameState->MemoryArena, "spaceship_left.bmp");
        GameState->SpaceshipBitmaps[2] = LoadBMP(&GameState->MemoryArena, "spaceship_down.bmp");
        GameState->SpaceshipBitmaps[3] = LoadBMP(&GameState->MemoryArena, "spaceship_right.bmp");
        
        GameState->SpaceshipUpSmallBitmap = LoadBMP(&GameState->MemoryArena, "spaceship_up_small.bmp");
        
        GameState->SaucerBitmap = LoadBMP(&GameState->MemoryArena, "saucer.bmp");
        GameState->AsteroidBigBitmap = LoadBMP(&GameState->MemoryArena, "asteroid_big.bmp");
        GameState->AsteroidMediumBitmap = LoadBMP(&GameState->MemoryArena, "asteroid_medium.bmp");
        GameState->AsteroidSmallBitmap = LoadBMP(&GameState->MemoryArena, "asteroid_small.bmp");
        GameState->BombBitmap = LoadBMP(&GameState->MemoryArena, "bomb.bmp");
        GameState->CrateBitmap = LoadBMP(&GameState->MemoryArena, "crate.bmp");
        GameState->Missile1Bitmap = LoadBMP(&GameState->MemoryArena, "missile1.bmp");
        GameState->Missile2Bitmap = LoadBMP(&GameState->MemoryArena, "missile2.bmp");
        
        // NOTE(jp): Coordinates and distances in meters are bottom-up, origin in the bottom-left corner,
        // coordinates in pixels are top-down, origin in the top-left corner.  Everything in the game
        // is in meters, pixels are just for drawing.
        real32 SpaceshipBitmapWidthInMeters = 2.0f;
        real32 SpaceshipBitmapWidthInPixels = (real32)GameState->SpaceshipBitmaps[0].Width;
        
        GameState->MetersToPixels = SpaceshipBitmapWidthInPixels / SpaceshipBitmapWidthInMeters;        
        GameState->PixelsToMeters = SpaceshipBitmapWidthInMeters / SpaceshipBitmapWidthInPixels;
        GameState->BufferWidthInMeters = (real32)Buffer->Width*GameState->PixelsToMeters;
        GameState->BufferHeightInMeters = (real32)Buffer->Height*GameState->PixelsToMeters;

        AddNullEntity(GameState);
        InitialPlayerP = {GameState->BufferWidthInMeters*0.5f, GameState->BufferHeightInMeters*0.25f};
        GameState->PlayerEntityIndex = AddPlayerEntity(GameState, InitialPlayerP);

        // NOTE(jp): These should all be normalized before use.
        int EnemiesDirectionsIndex = 0;
        v2 EnemiesDirections[] =
            {
                {0.7f, -0.3f},
                {-1.0f, -0.9f},
                {-1.0f, 1.2f},
                {1.0f, 0.2f},
                {1.0f, -0.6f},
                {1.0f, 3.0f},
                {-1.0f, -1.0f},
                {0.0f, 1.0f},
                {0.0f, -1.0f},
                {0.5f, -2.0f},
            };
        
        int AsteroidCount = 2;
        int SaucerCount = 1;
        
        Assert(ArrayCount(EnemiesDirections) >= (AsteroidCount + SaucerCount));
        for(int AsteroidIndex = 0;
            AsteroidIndex < AsteroidCount;
            ++AsteroidIndex)
        {
            v2 AsteroidP = {GameState->BufferWidthInMeters*0.15f*AsteroidIndex +
                            GameState->AsteroidBigBitmap.Width*GameState->PixelsToMeters,
                            GameState->BufferHeightInMeters*0.5f};
            v2 AsteroidMovementDirection = NormalizeV2(EnemiesDirections[EnemiesDirectionsIndex++]);
            entity *Asteroid = AddAsteroidEntity(GameState, AsteroidP, AsteroidMovementDirection);
        }

        for(int SaucerIndex = 0;
            SaucerIndex < SaucerCount;
            ++SaucerIndex)
        {
            v2 SaucerP = {GameState->BufferWidthInMeters*0.2f*SaucerIndex +
                          GameState->SaucerBitmap.Width*GameState->PixelsToMeters,
                          GameState->BufferHeightInMeters*0.75f};
            v2 SaucerMovementDirection = NormalizeV2(EnemiesDirections[EnemiesDirectionsIndex++]);
            entity *Saucer = AddSaucerEntity(GameState, SaucerP, SaucerMovementDirection);
        }

        GameState->RemainingEnemyCount = AsteroidCount + SaucerCount;

        Memory->IsInitialized = true;
    }

    // TODO(jp): should I just make sure the platform provides a cleared buffer?
    ClearSoundBuffer(Sound);
    CopyWAVToSoundBuffer(Sound, &GameState->WAVFile);
//    WriteWaveInSoundBuffer(&GameState->ShootingSound, Sound);
    
    entity *Player = GetEntity(GameState, GameState->PlayerEntityIndex);
    {
        //
        // NOTE(jp): Entity movement
        //
        
        game_controller *Controller = &Input->Controller;
        v2 ddPlayerP = {}; // m/s^2

        if(Controller->MoveUp.IsDown)
        {
            ddPlayerP.Y = 1.0f;
        }
        if(Controller->MoveLeft.IsDown)
        {
            ddPlayerP.X = -1.0f;
        }
        if(Controller->MoveDown.IsDown)
        {
            ddPlayerP.Y = -1.0f;
        }
        if(Controller->MoveRight.IsDown)
        {
            ddPlayerP.X = 1.0f;
        }

        if((ddPlayerP.X != 0) && (ddPlayerP.Y != 0))
        {
            ddPlayerP = NormalizeV2(ddPlayerP);
        }
        
        if((ddPlayerP.X != 0) || (ddPlayerP.Y != 0))
        {
            Player->Direction = ddPlayerP;
        }

#if 1
        // NOTE(jp): Movement with booster
        real32 PlayerSpeed = 60.0f; // m/s^2
        ddPlayerP *= PlayerSpeed;
        if(Controller->ActionDown.IsDown)
        {
            Player->ddP = Player->Direction*PlayerSpeed;
        }
        else
        {
            Player->ddP = {};
        }
        Player->ddP += -0.7f*Player->dP;
#else
        // NOTE(jp): Direct movement
        real32 PlayerSpeed = 250.0f; // m/s^2
        Player->ddP = ddPlayerP*PlayerSpeed;
        Player->ddP += -5.5f*Player->dP;
#endif

        real32 PlayerBulletsPerSecond = 3.8f;
        real32 SecondsBetweenPlayerShots = 1.0f / PlayerBulletsPerSecond;

        real32 SaucerBulletsPerSecond = 0.5f;
        real32 SecondsBetweenSaucerShots = 1.0f / SaucerBulletsPerSecond;
        
        if(Player->SecondsUntilShotIsReady > 0.0f)
        {
            Player->SecondsUntilShotIsReady -= Input->dtForFrame;
            if(Player->SecondsUntilShotIsReady <= 0.0f)
            {
                Player->SecondsUntilShotIsReady = 0.0f;
            }
        }

        Player->TookAShot = false;
        if(Controller->ActionLeft.IsDown)
        {
//            WriteWaveInSoundBuffer(&GameState->ShootingSound, Sound);
            if(Player->SecondsUntilShotIsReady == 0.0f)
            {
                WriteWaveInSoundBuffer(&GameState->ShootingSound, Sound);
                Player->TookAShot = true;
                Player->SecondsUntilShotIsReady = SecondsBetweenPlayerShots;
            }
        }

        // NOTE(jp): The bullets' positions are correctly updated with their parent entities
        // because they're always after them in the Entities array.
        for(uint32 EntityIndex = 1;
            EntityIndex < GameState->EntityCount;
            ++EntityIndex)
        {
            entity *Entity = GetEntity(GameState, EntityIndex);

            if(Entity->Lifes > 0)
            {
                if(Entity->Type == EntityType_Player)
                {
                    Entity->P = (0.5f*Player->ddP*Square(Input->dtForFrame) +
                                  Entity->dP*Input->dtForFrame + 
                                  Entity->P);
                    Entity->dP = Player->ddP*Input->dtForFrame + Entity->dP;
                    Entity->P = GetWrappedEntityP(GameState, Entity);
                }            
                else if(Entity->Type == EntityType_Saucer)
                {
                    entity *Saucer = Entity;
                    if(Saucer->SecondsUntilShotIsReady > 0.0f)
                    {
                        Saucer->SecondsUntilShotIsReady -= Input->dtForFrame;
                        if(Saucer->SecondsUntilShotIsReady < 0.0f)
                        {
                            Saucer->SecondsUntilShotIsReady = 0.0f;
                        }
                    }
                
                    Saucer->TookAShot = false;
                    if(Saucer->SecondsUntilShotIsReady == 0.0f)
                    {
                        Saucer->TookAShot = true;
                        Saucer->SecondsUntilShotIsReady = SecondsBetweenSaucerShots;
                    }
                    
                    Entity->P += Entity->dP*Input->dtForFrame;
                    Entity->P = GetWrappedEntityP(GameState, Entity);
                }
                else if(Entity->Type == EntityType_Asteroid)
                {
                    Entity->P += Entity->dP*Input->dtForFrame;
                    Entity->P = GetWrappedEntityP(GameState, Entity);
                }
                else if((Entity->Type == EntityType_PlayerBullet) ||
                        (Entity->Type == EntityType_EnemyBullet))
                {
                    Assert((Entity->ParentEntity->Type == EntityType_Player) ||
                           (Entity->ParentEntity->Type == EntityType_Saucer));
                    if(IsBulletAvailableForShooting(Entity))
                    {
                        Entity->P = GetWrappedEntityP(GameState, Entity->ParentEntity);

                        if(Entity->ParentEntity->TookAShot)
                        {
                            if(Entity->Type == EntityType_PlayerBullet)
                            {
                                Entity->Direction = Entity->ParentEntity->Direction;
                                real32 PlayerBulletSpeed = 30.0f;
#if 0
                                PlayerBulletSpeed = 100.0f; // m/s^2
                                Entity->ddP = Entity->Direction*PlayerBulletSpeed;
                                Entity->dP = Entity->ddP*Input->dtForFrame + Entity->dP;
#else
                                // NOTE(jp): Bullets seem to be more fun when they have immediate
                                // and fixed velocity
                                Entity->ddP = {};
                                Entity->dP = Entity->Direction*PlayerBulletSpeed;
#endif
                                Entity->ParentEntity->TookAShot = false;
                            }
                            else if(Entity->Type == EntityType_EnemyBullet)
                            {
                                v2 TowardsPlayer = Player->P - Entity->ParentEntity->P;
                                Entity->Direction = NormalizeV2(TowardsPlayer);
                                real32 SaucerBulletSpeed = 7.0f;  
#if 0
                                SaucerBulletSpeed = 100.0f; // m/s^2
                                Entity->ddP = Entity->Direction*SaucerBulletSpeed;
                                Entity->dP = Entity->ddP*Input->dtForFrame + Entity->dP;
#else
                                Entity->ddP = {};
                                Entity->dP = Entity->Direction*SaucerBulletSpeed;
#endif
                                Entity->ParentEntity->TookAShot = false;
                            }
                            else
                            {
                                Assert(!"No other kind of bullet");
                            }
                        }
                    }
                    else
                    {
                        Entity->P = (0.5f*Entity->ddP*Square(Input->dtForFrame) +
                                     Entity->dP*Input->dtForFrame +
                                     Entity->P);
                        Entity->dP = Entity->ddP*Input->dtForFrame + Entity->dP;

                        if(DidEntityWrapped(GameState, Entity))
                        {
                            MakeBulletAvailableForShooting(Entity, Entity->ParentEntity);
                        }
                    }
                }
                else
                {
                    Assert(!"Unknown entity type!");
                }
            }
            
            for(uint32 CollisionBoxIndex = 0;
                CollisionBoxIndex < Entity->CollisionBoxCount;
                ++CollisionBoxIndex)
            {
                collision_box *CollisionBox = Entity->CollisionBoxes + CollisionBoxIndex;
                CollisionBox->Center = Entity->P + CollisionBox->OffsetFromEntityP;
            }

        }
                
        // NOTE(jp): This gives Y precedence over X when they're both 1.0f!
        v2 TestPlayerP = Player->P;
        if(Player->Direction.Y == 1.0f)
        {
            Player->Bitmap = &GameState->SpaceshipBitmaps[0];
            collision_box *CollisionBox =  &Player->CollisionBoxes[0];
            
            CollisionBox->Center = TestPlayerP + CollisionBox->OffsetFromEntityP;
            CollisionBox->Dimension = V2(34.0f, 116.0f)*GameState->PixelsToMeters;

            ++CollisionBox;
            CollisionBox->Center = TestPlayerP + CollisionBox->OffsetFromEntityP;
            CollisionBox->Dimension = V2(67.0f, 87.0f)*GameState->PixelsToMeters;

            Player->CollisionMaxWidthHeight = GetCollisionBoxesMaxWidthHeight(Player->CollisionBoxes,
                                                                              Player->CollisionBoxCount);
        }
        else if(Player->Direction.Y == -1.0f)
        {
            Player->Bitmap = &GameState->SpaceshipBitmaps[2];
            collision_box *CollisionBox =  &Player->CollisionBoxes[0];

            CollisionBox->Center = TestPlayerP + CollisionBox->OffsetFromEntityP;
            CollisionBox->Dimension = V2(34.0f, 116.0f)*GameState->PixelsToMeters;

            ++CollisionBox;
            v2 NewOffsetFromEntityP = {};
            NewOffsetFromEntityP.Y = -CollisionBox->OffsetFromEntityP.Y;
            CollisionBox->Center = TestPlayerP + NewOffsetFromEntityP;
            CollisionBox->Dimension = V2(67.0f, 87.0f)*GameState->PixelsToMeters;

            Player->CollisionMaxWidthHeight = GetCollisionBoxesMaxWidthHeight(Player->CollisionBoxes,
                                                                              Player->CollisionBoxCount);
        }
        else if(Player->Direction.X == 1.0f)
        {
            Player->Bitmap = &GameState->SpaceshipBitmaps[3];
            collision_box *CollisionBox =  &Player->CollisionBoxes[0];
            
            CollisionBox->Center = TestPlayerP + CollisionBox->OffsetFromEntityP;
            CollisionBox->Dimension = V2(116.0f, 34.0f)*GameState->PixelsToMeters;

            ++CollisionBox;
            v2 NewOffsetFromEntityP = {};
            NewOffsetFromEntityP.X = CollisionBox->OffsetFromEntityP.Y;
            CollisionBox->Center = TestPlayerP + NewOffsetFromEntityP;
            CollisionBox->Dimension = V2(87.0f, 67.0f)*GameState->PixelsToMeters;

            Player->CollisionMaxWidthHeight = GetCollisionBoxesMaxWidthHeight(Player->CollisionBoxes,
                                                                              Player->CollisionBoxCount);
        }
        else if(Player->Direction.X == -1.0f)
        {
            Player->Bitmap = &GameState->SpaceshipBitmaps[1];
            collision_box *CollisionBox =  &Player->CollisionBoxes[0];
            
            CollisionBox->Center = TestPlayerP + CollisionBox->OffsetFromEntityP;
            CollisionBox->Dimension = V2(116.0f, 34.0f)*GameState->PixelsToMeters;

            ++CollisionBox;
            v2 NewOffsetFromEntityP = {};
            NewOffsetFromEntityP.X = -CollisionBox->OffsetFromEntityP.Y;
            CollisionBox->Center = TestPlayerP + NewOffsetFromEntityP;
            CollisionBox->Dimension = V2(87.0f, 67.0f)*GameState->PixelsToMeters;

            Player->CollisionMaxWidthHeight = GetCollisionBoxesMaxWidthHeight(Player->CollisionBoxes,
                                                                              Player->CollisionBoxCount);
        }
        else
        {
            // TODO(jp): Waiting for rotations.
        }

        //
        // NOTE(jp): Collision detection
        //
        
        for(uint32 FriendlyEntityIndex = 1;
            FriendlyEntityIndex < GameState->EntityCount;
            ++FriendlyEntityIndex)
        {
            entity *FriendlyEntity = GetEntity(GameState, FriendlyEntityIndex);

            if((FriendlyEntity->Type == EntityType_Player) || 
               ((FriendlyEntity->Type == EntityType_PlayerBullet) &&
                !IsBulletAvailableForShooting(FriendlyEntity)))
            {
                for(uint32 EnemyEntityIndex = 1;
                    EnemyEntityIndex < GameState->EntityCount;
                    ++EnemyEntityIndex)
                {
                    entity *EnemyEntity = GetEntity(GameState, EnemyEntityIndex);

                    if((EnemyEntity->Type == EntityType_Asteroid) ||
                       (EnemyEntity->Type == EntityType_Saucer) ||
                       (EnemyEntity->Type == EntityType_EnemyBullet))
                    {
                        if(EnemyEntity->Lifes > 0)
                        {
                            bool32 AFriendlyEntityBoxCollidedThisFrame = false;
                
                            for(uint32  FriendlyEntityCollisionBoxIndex = 0;
                                FriendlyEntityCollisionBoxIndex < FriendlyEntity->CollisionBoxCount;
                                ++FriendlyEntityCollisionBoxIndex)
                            {
                                collision_box *FriendlyEntityCollisionBox =
                                    &FriendlyEntity->CollisionBoxes[FriendlyEntityCollisionBoxIndex];
                                rectangle2 FriendlyEntityCollisionRect =
                                    RectCenterDim(FriendlyEntityCollisionBox->Center,
                                                  FriendlyEntityCollisionBox->Dimension);

                                for(uint32 EnemyEntityCollisionBoxIndex = 0;
                                    EnemyEntityCollisionBoxIndex < EnemyEntity->CollisionBoxCount;
                                    ++EnemyEntityCollisionBoxIndex)
                                {                            
                                    collision_box *EnemyEntityCollisionBox =
                                        &EnemyEntity->CollisionBoxes[EnemyEntityCollisionBoxIndex];
                                    rectangle2 EnemyEntityCollisionRect =
                                        RectCenterDim(EnemyEntityCollisionBox->Center,
                                                      EnemyEntityCollisionBox->Dimension);

                                    bool32 CollisionRectsCollided =
                                        DoRectanglesCollide(FriendlyEntityCollisionRect, EnemyEntityCollisionRect);
                                    AFriendlyEntityBoxCollidedThisFrame =
                                        (AFriendlyEntityBoxCollidedThisFrame || CollisionRectsCollided);
                                }
                            }
                            
                            FriendlyEntity->CollidedThisFrame = AFriendlyEntityBoxCollidedThisFrame;
                            FriendlyEntity->CollidedLastFrame = EnemyEntity->CollidedLastFrame;

                            EnemyEntity->CollidedThisFrame = AFriendlyEntityBoxCollidedThisFrame;
                            EnemyEntity->CollidedLastFrame = FriendlyEntity->CollidedThisFrame;
                            if(FriendlyEntity->CollidedThisFrame)
                            {                                
                                FriendlyEntity->CollidedWith = EnemyEntity;
                                break;
                            }
                        }
                    }
                }
            }
        }

        //
        // NOTE(jp): Game updating
        //

        uint32 PointsPerEnemyKilled = 150;
        for(uint32 EntityIndex = 1;
            EntityIndex < GameState->EntityCount;
            ++EntityIndex)
        {
            entity *Entity = GetEntity(GameState, EntityIndex);
            if(Entity->Type == EntityType_Player)
            {
                if(Entity->CollidedThisFrame && 
                   !Entity->CollidedLastFrame)
                {
                  --Player->Lifes;
                  if(Entity->CollidedWith->Type == EntityType_EnemyBullet)
                  {
                      MakeBulletAvailableForShooting(Entity->CollidedWith,
                                                     Entity->CollidedWith->ParentEntity);
                  }

                  if(Player->Lifes == 0)
                  {
                      // TODO(jp): This is the "You lose" case, handle it more properly than starting over.
                      Player->Lifes = 4;
                  }                    
                }
            }
            else if(Entity->Type == EntityType_PlayerBullet)
            {
                if(Entity->CollidedThisFrame && !IsBulletAvailableForShooting(Entity))
                {
                    entity *Enemy = Entity->CollidedWith;
                    Assert(Enemy->Lifes > 0);
                    --Enemy->Lifes;
                    if(Enemy->Type == EntityType_Asteroid)
                    {
                        if(Enemy->Lifes == 2)
                        {
                            Enemy->Bitmap = &GameState->AsteroidMediumBitmap;
                        }
                        else if(Enemy->Lifes == 1)
                        {
                            Enemy->Bitmap = &GameState->AsteroidSmallBitmap;
                        }
                        Enemy->CollisionBoxes[0].Dimension *= 0.6f;
                        Enemy->dP *= 1.5f;
                    }
                        
                    if(Enemy->Lifes == 0)
                    {
                        // TODO(jp): Decide what is reasonable to clear here to get a good "dead entity"

                        if(Enemy->Type == EntityType_EnemyBullet)
                        {
                            MakeBulletAvailableForShooting(Enemy, Enemy->ParentEntity);
                        }
                        else if((Enemy->Type == EntityType_Asteroid) ||
                                (Enemy->Type == EntityType_Saucer))
                        {
                            // TODO(jp): We don't kill the child bullets when we kill the saucer!
                            // We just take them it  of the way later when their parent's HellP
                            entity_type Type = Enemy->Type;
                            *Enemy = {};
                            v2 SpaceHellCoordinates = {666.0f, 666.0f};
                            Enemy->Type = Type;
                            Enemy->P = SpaceHellCoordinates;
                            --GameState->RemainingEnemyCount;
                            GameState->Score += PointsPerEnemyKilled;
                        }
                        
                        if(GameState->RemainingEnemyCount == 0)
                        {
                            // TODO(jp): This is the "You Win" condition, handle it more properly.
                            int YouWinPlaceholder = 5;
                        }
                    }

                    MakeBulletAvailableForShooting(Entity, Entity->ParentEntity);                    
                }                   
            }
        }
    }
    
    //
    // NOTE(jp): Rendering
    //
    
    // NOTE(jp): Clear the screen to red for rendering testing purposes
    {
        uint8 *Row = (uint8 *)Buffer->Memory;
        for(int Y = 0;
            Y < Buffer->Height;
            ++Y)
        {
            uint32 *Pixel = (uint32 *)Row;
            for(int X = 0;
                X < Buffer->Width;
                ++X)
            {
                uint8 R = 255;
                uint8 G = 0;
                uint8 B = 0;
                *Pixel++ = ((R << 16) | (G << 8) | (B << 0));
            }
            Row += Buffer->Pitch;
        }
    }
    
    DrawBitmap(Buffer, &GameState->BackgroundBitmap, 0.0f, 0.0f);

    for(uint32 EntityIndex = 1;
        EntityIndex < GameState->EntityCount;
        ++EntityIndex)
    {
        entity *Entity = GetEntity(GameState, EntityIndex);
        if(Entity->Lifes > 0)
        {
            if((Entity->Type == EntityType_PlayerBullet) ||
               (Entity->Type == EntityType_EnemyBullet))
            {
                if(IsBulletAvailableForShooting(Entity))
                {
                    continue;
                }
#if CSTEROIDS_INTERNAL                
                if(EntityIndex == Entity->ParentEntity->FirstBulletEntityIndex)
                {
                    if(Entity->Type == EntityType_PlayerBullet)
                    {
                        Entity->Bitmap = &GameState->CrateBitmap;
                    }
                    else if(Entity->Type == EntityType_EnemyBullet)
                    {
                        Entity->Bitmap = &GameState->Missile2Bitmap;
                    }
                }
#endif
            }
        
            if((Entity->Type == EntityType_Player) ||
               (Entity->Type == EntityType_Asteroid) ||
               (Entity->Type == EntityType_Saucer))
            {
            }
            
            v2 EntityPInPixels = GetPixelCoord(Buffer, Entity->P, GameState->MetersToPixels);
            rectangle2 EntityBitmapRect =
                RectCenterDim(EntityPInPixels, V2((real32)Entity->Bitmap->Width, (real32)Entity->Bitmap->Height));

            DrawBitmap(Buffer, Entity->Bitmap, EntityBitmapRect.Min.X, EntityBitmapRect.Min.Y);
        
#if 0
            DEBUGDrawBitmapBox(Buffer, EntityPInPixels, Entity->Bitmap, 0.0f, 0.0f, 1.0f, 0.5f);
            DEBUGDrawCollisionBox(GameState, Buffer, Entity->CollisionBoxes,
                                  Entity->CollisionBoxCount, 1.0f, 1.0f, 0.0f, 0.6f);
#endif
        }
    }
    
    // NOTE(jp): Font test
    {            
        int32 Number = GameState->Score;
        
        int DecomposedNumber[10] = {};
        uint32 DecomposedNumberCount = 0;
        // Decompose number
        {
            bool32 LeadingZero = true;
            uint32 Accumulation = 0;
            uint32 MaxExponent = 10;

            int DecomposedNumberIndex = 0;
            for(uint32 ExponentIndex = 0;
                ExponentIndex <= MaxExponent;
                ++ExponentIndex)
            {
                int Exponent = MaxExponent - ExponentIndex;

                int Raised = Power(10, Exponent);
                int Decomposed = ((Number - Accumulation) / Raised);

                if((Decomposed != 0) && LeadingZero)
                {
                    LeadingZero = false;
                }

                if(!LeadingZero)
                {
                    DecomposedNumber[DecomposedNumberIndex] =  Decomposed;
                    Accumulation += (DecomposedNumber[DecomposedNumberIndex]*Raised);
                    ++DecomposedNumberIndex;
                }
            }
            DecomposedNumberCount = DecomposedNumberIndex;
        }

        real32 TextMinX = 100.0f;
        real32 TextMinY = 50.0f;
        
        real32 DistanceBetween = 50.0f;
        real32 X = TextMinX;
        real32 Y = TextMinY;
        
        for(uint32 DecomposedNumberIndex = 0;
            DecomposedNumberIndex < DecomposedNumberCount;
            ++DecomposedNumberIndex)
        {
            uint32 CurrentNumber = DecomposedNumber[DecomposedNumberIndex];
            loaded_bitmap *GlyphBitmap = GameState->FontGlyphs + CurrentNumber;
            
            DrawBitmap(Buffer, GlyphBitmap, X, Y);
            X += GlyphBitmap->Width;
        }
    }
    
    {
        real32 LifeIndicatorWidth = (real32)GameState->SpaceshipUpSmallBitmap.Width;
        real32 LifeIndicatorHeight = (real32)GameState->SpaceshipUpSmallBitmap.Height;

        real32 LifeIndicatorXOffset = LifeIndicatorWidth*2.0f;
        real32 LifeIndicatorYOffset = LifeIndicatorHeight*0.5f;

        real32 DistanceBetween = LifeIndicatorWidth*0.25f;

        for(uint32 LifeIndex = 0;
            LifeIndex < Player->Lifes;
            ++LifeIndex)
        {        
            real32 LifeIndicatorMinX = (LifeIndicatorXOffset +
                                        (LifeIndicatorWidth + DistanceBetween)*(real32)LifeIndex);
            real32 LifeIndicatorMinY = LifeIndicatorYOffset + LifeIndicatorHeight;
            DrawBitmap(Buffer, &GameState->SpaceshipUpSmallBitmap, LifeIndicatorMinX, LifeIndicatorMinY);
        }
    }
}
