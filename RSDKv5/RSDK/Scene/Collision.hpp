#ifndef COLLISION_H
#define COLLISION_H

namespace RSDK
{

enum TileCollisionModes {
    TILECOLLISION_NONE, // no tile collisions
    TILECOLLISION_DOWN, // downwards tile collisions
#if RETRO_REV0U
    TILECOLLISION_UP, // upwards tile collisions
#endif
};

enum CSides {
    C_NONE,
    C_TOP,
    C_LEFT,
    C_RIGHT,
    C_BOTTOM,
};

struct CollisionSensor {
    Vector2 position;
    bool32 collided;
    uint8 angle;
};

#if !RETRO_USE_ORIGINAL_CODE
#define DEBUG_HITBOX_COUNT (0x400)

struct DebugHitboxInfo {
    uint8 type;
    uint8 collision;
    uint16 entityID;
    Hitbox hitbox;
    Vector2 pos;
};

enum DebugHitboxTypes { H_TYPE_TOUCH, H_TYPE_CIRCLE, H_TYPE_BOX, H_TYPE_PLAT };

extern bool32 showHitboxes;
extern int32 debugHitboxCount;
extern DebugHitboxInfo debugHitboxList[DEBUG_HITBOX_COUNT];

int32 AddDebugHitbox(uint8 type, uint8 dir, Entity *entity, Hitbox *hitbox);
#endif

extern int32 collisionTolerance;
#if RETRO_REV0U
extern bool32 useCollisionOffset;
#else
extern int32 collisionOffset;
#endif
extern int32 collisionMaskAir;

extern Hitbox collisionOuter;
extern Hitbox collisionInner;

extern Entity *collisionEntity;

extern CollisionSensor sensors[6];

#if RETRO_REV0U
extern int32 collisionMinimumDistance;

extern uint8 lowCollisionTolerance;
extern uint8 highCollisionTolerance;

extern uint8 floorAngleTolerance;
extern uint8 wallAngleTolerance;
extern uint8 roofAngleTolerance;

inline void SetupCollisionConfig(int32 minDistance, uint8 lowTolerance, uint8 highTolerance, uint8 floorAngleTolerance, uint8 wallAngleTolerance,
                                 uint8 roofAngleTolerance)
{
    collisionMinimumDistance = TO_FIXED(minDistance);
    lowCollisionTolerance    = lowTolerance;
    highCollisionTolerance   = highTolerance;
    floorAngleTolerance      = floorAngleTolerance;
    wallAngleTolerance       = wallAngleTolerance;
    roofAngleTolerance       = roofAngleTolerance;
}

void CopyCollisionMask(uint16 dst, uint16 src, uint8 cPlane, uint8 cMode);

inline void GetCollisionInfo(CollisionMask **masks, TileInfo **tileInfo)
{
    if (masks)
        *masks = (RSDK::CollisionMask *)collisionMasks;

    if (tileInfo)
        *tileInfo = (RSDK::TileInfo *)tileInfo;
}
#endif

bool32 CheckObjectCollisionTouch(Entity *thisEntity, Hitbox *thisHitbox, Entity *otherEntity, Hitbox *otherHitbox);
inline bool32 CheckObjectCollisionCircle(Entity *thisEntity, int32 thisRadius, Entity *otherEntity, int32 otherRadius)
{
    int32 x = FROM_FIXED(thisEntity->position.x - otherEntity->position.x);
    int32 y = FROM_FIXED(thisEntity->position.y - otherEntity->position.y);
    int32 r = FROM_FIXED(thisRadius + otherRadius);

#if !RETRO_USE_ORIGINAL_CODE
    if (showHitboxes) {
        bool32 collided = x * x + y * y < r * r;
        Hitbox thisHitbox;
        Hitbox otherHitbox;
        thisHitbox.left  = thisRadius >> 16;
        otherHitbox.left = otherRadius >> 16;

        int32 thisHitboxID  = AddDebugHitbox(H_TYPE_CIRCLE, FLIP_NONE, thisEntity, &thisHitbox);
        int32 otherHitboxID = AddDebugHitbox(H_TYPE_CIRCLE, FLIP_NONE, otherEntity, &otherHitbox);

        if (thisHitboxID >= 0 && collided)
            debugHitboxList[thisHitboxID].collision |= 1 << (collided - 1);
        if (otherHitboxID >= 0 && collided)
            debugHitboxList[otherHitboxID].collision |= 1 << (collided - 1);
    }
#endif

    return x * x + y * y < r * r;
}
uint8 CheckObjectCollisionBox(Entity *thisEntity, Hitbox *thisHitbox, Entity *otherEntity, Hitbox *otherHitbox, bool32 setValues);
bool32 CheckObjectCollisionPlatform(Entity *thisEntity, Hitbox *thisHitbox, Entity *otherEntity, Hitbox *otherHitbox, bool32 setValues);

bool32 ObjectTileCollision(Entity *entity, uint16 cLayers, uint8 cMode, uint8 cPlane, int32 xOffset, int32 yOffset, bool32 setPos);
bool32 ObjectTileGrip(Entity *entity, uint16 cLayers, uint8 cMode, uint8 cPlane, int32 xOffset, int32 yOffset, int32 tolerance);

void ProcessObjectMovement(Entity *entity, Hitbox *outerBox, Hitbox *innerBox);

void ProcessPathGrip();
void ProcessAirCollision_Down();
#if RETRO_REV0U
void ProcessAirCollision_Up();
#endif

void SetPathGripSensors(CollisionSensor *cSensors);

void FindFloorPosition(CollisionSensor *sensor);
void FindLWallPosition(CollisionSensor *sensor);
void FindRoofPosition(CollisionSensor *sensor);
void FindRWallPosition(CollisionSensor *sensor);

void FloorCollision(CollisionSensor *sensor);
void LWallCollision(CollisionSensor *sensor);
void RoofCollision(CollisionSensor *sensor);
void RWallCollision(CollisionSensor *sensor);

#if RETRO_REV0U
#include "Legacy/CollisionLegacy.hpp"
#endif

} // namespace RSDK

#endif // !COLLISION_H
