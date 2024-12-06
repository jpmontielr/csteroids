#if !defined(CSTEROIDS_H)

#define Pi32 3.14159265359f

struct memory_arena
{
    uint8 *Base;
    uint32 Size;
    uint32 Used;
};

inline void *
PushSize_(memory_arena *Arena, uint32 Size)
{
    Assert((Arena->Used + Size) < (Arena->Size));
    void *Result = (void *)(Arena->Base + Arena->Used);
    Arena->Used += Size;
    return(Result);
}

inline void
InitializeArena(memory_arena *Arena, uint8 *Base, uint32 Size)
{
    Arena->Base = Base;
    Arena->Size = Size;
    Arena->Used = 0;
}

#define PushStruct(Arena, Type) (Type *)PushSize_(Arena, sizeof(Type))
#define PushArray(Arena, Type, Count) (Type *)PushSize_(Arena, sizeof(Type)*Count)

// NOTE(jp): Services from the platform layer to the game
struct read_entire_file_result
{
    void *FileMemory;
    uint32 FileSize;
};
internal read_entire_file_result PlatformReadEntireFile(char *FileName, memory_arena *Arena);

// NOTE(jp): Services from the game to the platform layer

struct game_memory
{
    bool32 IsInitialized;

    void *PermanentStorage;
    uint32 PermanentStorageSize;

    void *TransientStorage;
    uint32 TransientStorageSize;
};

struct sound_wave
{
    uint32 Type;
    uint32 RunningSampleIndex;
    int ToneHz;
    int Period;
    real32 t;
};

struct game_sound_output
{
    int32 SamplesPerSecond;
    // NOTE(Jp): We have two possible sound buffers so we can receive ring buffers from the platform.
    int16 *Samples1;
    uint32 SampleCount1;
    int16 *Samples2;
    uint32 SampleCount2;
};

struct game_offscreen_buffer
{
    int32 Width;
    int32 Height;
    int BytesPerPixel;
    int Pitch;
    void *Memory;
};

struct game_controller_button
{
    uint32 HalfTransitionCount;
    bool32 IsDown;
};

struct game_controller
{
    union
    {
        game_controller_button Buttons[9];
        struct
        {
            game_controller_button MoveUp;
            game_controller_button MoveLeft;
            game_controller_button MoveDown;
            game_controller_button MoveRight;

            game_controller_button ActionUp;
            game_controller_button ActionLeft;
            game_controller_button ActionDown;
            game_controller_button ActionRight;

            game_controller_button Start;
        };
    };
};

struct game_input
{
    game_controller Controller;
    real32 dtForFrame;
};

struct loaded_bitmap
{
    uint32 Width;
    uint32 Height;
    void *Memory;    
};

#pragma pack(push, 1)
struct wav_header
{
    char RIFFChunkID[4];
    uint32 RIFFChunkSize;
    
    char WAVEId[4];
    
    char fmtChunkID[4];
    uint32 fmtChunkSize;
    uint16 wFormatTag;
    uint16 nChannels;
    uint32 nSamplesPerSec;
    uint32 nAvgBytesPerSec;
    uint16 nBlockAlign;
    uint16 wBitsPerSample;

    char dataChunkID[4];
    uint32 dataChunkSize;
    int16 Samples; // beginning of the samples
};
#pragma pack(pop)

struct loaded_wav
{
    // 32-bit samples as 16-bit interleaved
    int16 *Samples;
    uint32 SampleCount;
    uint32 SampleIndex;
};

struct collision_box
{
    v2 OffsetFromEntityP;
    v2 Center;
    v2 Dimension;
};

#include "csteroids_entity.h"

struct game_state
{
    memory_arena MemoryArena;
    real32 MetersToPixels;
    real32 PixelsToMeters;
    
    real32 BufferWidthInMeters;
    real32 BufferHeightInMeters;

    loaded_bitmap BackgroundBitmap;

    loaded_bitmap SpaceshipBitmaps[4];
    loaded_bitmap SpaceshipUpSmallBitmap;
    
    loaded_bitmap SaucerBitmap;
    loaded_bitmap AsteroidBigBitmap;
    loaded_bitmap AsteroidMediumBitmap;
    loaded_bitmap AsteroidSmallBitmap;
    
    loaded_bitmap BombBitmap;
    loaded_bitmap CrateBitmap;
    loaded_bitmap Missile1Bitmap;
    loaded_bitmap Missile2Bitmap;

    loaded_bitmap FontGlyphs[10];
    uint32 PlayerEntityIndex;
    
    uint32 EntityCount;
    entity Entities[256];

    sound_wave ShootingSound;
    loaded_wav WAVFile;

    int RemainingEnemyCount;
    uint32 Score;
};

#define CSTEROIDS_H
#endif
