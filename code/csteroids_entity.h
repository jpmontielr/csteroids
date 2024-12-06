#if !defined(CSTEROIDS_ENTITY_H)

enum entity_type
{
    EntityType_Null,
    EntityType_Player,
    EntityType_Asteroid,
    EntityType_Saucer,
    EntityType_PlayerBullet,
    EntityType_EnemyBullet,
};

struct entity
{    
    entity_type Type;
    
    v2 P;
    v2 dP;
    v2 ddP;
    v2 Direction;
    
    uint32 CollisionBoxCount;
    collision_box *CollisionBoxes;
    v2 CollisionMaxWidthHeight;

    loaded_bitmap *Bitmap;
    
    uint32 Lifes;

    bool32 CollidedThisFrame;
    bool32 CollidedLastFrame;
    entity *CollidedWith;
#if CSTEROIDS_INTERNAL
    uint32 FirstBulletEntityIndex;
#endif
    entity *ParentEntity;
    bool32 TookAShot;
    real32 SecondsUntilShotIsReady;
};

#define CSTEROIDS_ENTITY_H
#endif
