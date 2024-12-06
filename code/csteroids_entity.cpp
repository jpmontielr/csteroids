#include "csteroids_entity.h"

// TODO(jp): Does this belong here or in a _collision_ file?
internal v2
GetCollisionBoxesMaxWidthHeight(collision_box *CollisionBoxes, uint32 CollisionBoxCount)
{
    v2 Result = {};
    
    for(uint32 CollisionBoxIndex = 0;
        CollisionBoxIndex < CollisionBoxCount;
        ++CollisionBoxIndex)
    {
        collision_box *CollisionBox = CollisionBoxes + CollisionBoxIndex;
        if(CollisionBox->Dimension.X > Result.X)
        {
            Result.X = CollisionBox->Dimension.X;
        }
        if(CollisionBox->Dimension.Y > Result.Y)
        {
            Result.Y = CollisionBox->Dimension.Y;
        }
    }

    return(Result);
}

internal v2
GetWrappedEntityP(game_state *GameState, entity *Entity)
{
    v2 Result = Entity->P;
    
#if 1
    real32 EntityWidth = Entity->CollisionMaxWidthHeight.X;
    real32 EntityHeight = Entity->CollisionMaxWidthHeight.Y;
#else
    real32 EntityWidth = (real32)Entity->Bitmap->Width*GameState->PixelsToMeters;
    real32 EntityHeight = (real32)Entity->Bitmap->Height*GameState->PixelsToMeters;
#endif
    rectangle2 EntityRect = RectCenterDim(Entity->P, V2(EntityWidth, EntityHeight));

    if(EntityRect.Min.X >= GameState->BufferWidthInMeters)
    {
        Result.X -= (GameState->BufferWidthInMeters + EntityWidth);
    }
    if(EntityRect.Min.X <= -EntityWidth)
    {
        Result.X += (GameState->BufferWidthInMeters + EntityWidth);
    }
    if(EntityRect.Min.Y >= GameState->BufferHeightInMeters)
    {
        Result.Y -= (GameState->BufferHeightInMeters + EntityHeight);
    }
    if(EntityRect.Min.Y <= -EntityHeight)
    {
        Result.Y += (GameState->BufferHeightInMeters + EntityHeight);
    }

    return(Result);
}

inline bool32
DidEntityWrapped(game_state *GameState, entity *Entity)
{
    v2 WrappedEntity = GetWrappedEntityP(GameState, Entity);
    bool32 Result = (WrappedEntity.X != Entity->P.X) || (WrappedEntity.Y != Entity->P.Y);
    return(Result);    
}

internal entity *
AddNullEntity(game_state *GameState)
{
    entity *NullEntity = GameState->Entities + GameState->EntityCount++;
    NullEntity->Type = EntityType_Null;
    return(NullEntity);
}

inline bool32
IsBulletAvailableForShooting(entity *Bullet)
{
    Assert(Bullet->ParentEntity);
    Assert((Bullet->Type == EntityType_PlayerBullet) || (Bullet->Type == EntityType_EnemyBullet));
    
    bool32 Result = false;
    if((Bullet->Type == EntityType_PlayerBullet) || (Bullet->Type == EntityType_EnemyBullet))
    {
        Result = (AreEqualV2(Bullet->dP, V2(0.0f, 0.0f)) &&
//                  AreEqualV2(Bullet->ddP, V2(0.0f, 0.0f)) &&
//                  !AreEqualV2(Bullet->Direction, V2(0.0f, 0.0f)) &&                  
                  (Bullet->Lifes > 0));
    }
    return(Result);
}

inline void
MakeBulletAvailableForShooting(entity *Bullet, entity *ParentEntity)
{
    Bullet->P = ParentEntity->P;
    Bullet->Direction = ParentEntity->Direction;
    Bullet->dP = {};
    Bullet->ddP = {};
    Bullet->Lifes = 1;
}

internal uint32
AddBulletEntity(game_state *GameState, entity *ParentEntity, entity_type EntityType)
{
    uint32 BulletEntityIndex = GameState->EntityCount;

    entity *Bullet = GameState->Entities + GameState->EntityCount++;
    
    *Bullet = {};
    Assert((EntityType == EntityType_PlayerBullet) || (EntityType == EntityType_EnemyBullet));
    Bullet->Type = EntityType;

    Bullet->ParentEntity = ParentEntity;
    MakeBulletAvailableForShooting(Bullet, ParentEntity);

    if(Bullet->Type == EntityType_PlayerBullet)
    {    
        Bullet->Bitmap = &GameState->BombBitmap;
    }
    else if(Bullet->Type == EntityType_EnemyBullet)
    {
        Bullet->Bitmap = &GameState->Missile1Bitmap;
    }

    Bullet->CollisionBoxCount = 1;
    Bullet->CollisionBoxes = PushArray(&GameState->MemoryArena, collision_box, Bullet->CollisionBoxCount);

    collision_box *CollisionBox =  &Bullet->CollisionBoxes[0];

    CollisionBox->OffsetFromEntityP = {0.1f, -0.1f}*GameState->PixelsToMeters;
    CollisionBox->Center = Bullet->P + CollisionBox->OffsetFromEntityP;
    CollisionBox->Dimension = 0.55f*V2((real32)Bullet->Bitmap->Width,
                                      (real32)Bullet->Bitmap->Height)*GameState->PixelsToMeters;
    Bullet->CollisionMaxWidthHeight = GetCollisionBoxesMaxWidthHeight(Bullet->CollisionBoxes,
                                                                      Bullet->CollisionBoxCount);

    return(BulletEntityIndex);
}

inline uint32
AddPlayerBulletEntity(game_state *GameState, entity *ParentEntity)
{
    uint32 PlayerBulletIndex = AddBulletEntity(GameState, ParentEntity, EntityType_PlayerBullet);
    return(PlayerBulletIndex);
}

inline uint32
AddEnemyBulletEntity(game_state *GameState, entity *ParentEntity)
{
    uint32 EnemyBulletIndex = AddBulletEntity(GameState, ParentEntity, EntityType_EnemyBullet);
    return(EnemyBulletIndex);
}

internal uint32
AddPlayerEntity(game_state *GameState, v2 PlayerP)
{
    uint32 PlayerEntityIndex = GameState->EntityCount;
    
    entity *Player = GameState->Entities + GameState->EntityCount++;
        
    *Player = {};    
    Player->Type = EntityType_Player;
    
    Player->P = {PlayerP.X, PlayerP.Y};
    Player->Direction = {0.0f, 1.0f};
    
    Player->Bitmap = &GameState->SpaceshipBitmaps[0];
            
    Player->CollisionBoxCount = 2;
    Player->CollisionBoxes = PushArray(&GameState->MemoryArena, collision_box, Player->CollisionBoxCount);

    collision_box *CollisionBox =  &Player->CollisionBoxes[0];
    CollisionBox->OffsetFromEntityP = {};
    CollisionBox->Center = Player->P + CollisionBox->OffsetFromEntityP;
    CollisionBox->Dimension = V2(34.0f, 116.0f)*GameState->PixelsToMeters;

    ++CollisionBox;
    CollisionBox->OffsetFromEntityP = V2(0.0f, -12.0f)*GameState->PixelsToMeters;
    CollisionBox->Center = Player->P + CollisionBox->OffsetFromEntityP ;
    CollisionBox->Dimension = V2(67.0f, 87.0f)*GameState->PixelsToMeters;
    Player->CollisionMaxWidthHeight = GetCollisionBoxesMaxWidthHeight(Player->CollisionBoxes,
                                                                      Player->CollisionBoxCount);

    Player->Lifes = 4;

    uint32 PlayerBulletCount = 4;
    for(uint32 BulletIndex = 0;
        BulletIndex < PlayerBulletCount;
        ++BulletIndex)
    {
        uint32 PlayerBulletIndex = AddPlayerBulletEntity(GameState, Player);
        if(BulletIndex == 0)
        {
            Player->FirstBulletEntityIndex = PlayerBulletIndex;
        }
    }

    return(PlayerEntityIndex);
}

internal entity *
AddAsteroidEntity(game_state *GameState, v2 P, v2 MovementDirection)
{
    entity *Asteroid = GameState->Entities + GameState->EntityCount++;

    *Asteroid = {};
    Asteroid->Type = EntityType_Asteroid;

    real32 dAsteroidP = 5.0f; // m/s
    Asteroid->P = P;
    Asteroid->dP = MovementDirection*dAsteroidP;

    Asteroid->Bitmap = &GameState->AsteroidBigBitmap;

    Asteroid->CollisionBoxCount = 1;
    Asteroid->CollisionBoxes = PushArray(&GameState->MemoryArena, collision_box, Asteroid->CollisionBoxCount);
        
    collision_box *CollisionBox =  &Asteroid->CollisionBoxes[0];
    CollisionBox->OffsetFromEntityP = {};
    CollisionBox->Center = Asteroid->P + CollisionBox->OffsetFromEntityP;
    CollisionBox->Dimension = V2(112.0f, 43.0f)*GameState->PixelsToMeters;

    Asteroid->CollisionMaxWidthHeight = GetCollisionBoxesMaxWidthHeight(Asteroid->CollisionBoxes,
                                                                        Asteroid->CollisionBoxCount);
            
    Asteroid->Lifes = 3;

    return(Asteroid);
}

internal entity *
AddSaucerEntity(game_state *GameState, v2 P, v2 MovementDirection)
{
    entity *Saucer = GameState->Entities + GameState->EntityCount++;

    *Saucer = {};
    Saucer->Type = EntityType_Saucer;

    real32 SaucerVelocity = 6.0f; // m/s
    Saucer->P = P;
    Saucer->dP = MovementDirection*SaucerVelocity;
    
    Saucer->Bitmap = &GameState->SaucerBitmap;

    Saucer->CollisionBoxCount = 2;
    Saucer->CollisionBoxes = PushArray(&GameState->MemoryArena, collision_box, Saucer->CollisionBoxCount);

    collision_box *CollisionBox =  &Saucer->CollisionBoxes[0];
    CollisionBox->OffsetFromEntityP = V2(0.0f, -6.1f)*GameState->PixelsToMeters;
    CollisionBox->Center = Saucer->P + CollisionBox->OffsetFromEntityP;
    CollisionBox->Dimension = V2(150.0f, 29.0f)*GameState->PixelsToMeters;

    ++CollisionBox;
    CollisionBox->OffsetFromEntityP = V2(0.0f, 24.0f)*GameState->PixelsToMeters;
    CollisionBox->Center = Saucer->P + CollisionBox->OffsetFromEntityP;
    CollisionBox->Dimension = V2(94.0f, 119.0f)*GameState->PixelsToMeters;

    Saucer->CollisionMaxWidthHeight = GetCollisionBoxesMaxWidthHeight(Saucer->CollisionBoxes,
                                                                      Saucer->CollisionBoxCount);
    
    Saucer->Lifes = 3;

    uint32 SaucerBulletCount = 4;
    for(uint32 BulletIndex = 0;
        BulletIndex < SaucerBulletCount;
        ++BulletIndex)
    {
        uint32 SaucerBulletIndex = AddEnemyBulletEntity(GameState, Saucer);
        if(BulletIndex == 0)
        {
            Saucer->FirstBulletEntityIndex = SaucerBulletIndex;
        }
    }

    return(Saucer);
}

inline entity *
GetEntity(game_state *GameState, uint32 EntityIndex)
{
    Assert(EntityIndex < GameState->EntityCount);
    entity *Result = &GameState->Entities[EntityIndex];
    return(Result);
}
