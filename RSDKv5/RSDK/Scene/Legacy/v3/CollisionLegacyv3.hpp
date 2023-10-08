
namespace Legacy
{

namespace v3
{

#define COLSTORE_COUNT (2)

enum CollisionSides { 
    CSIDE_FLOOR  = 0, 
    CSIDE_LWALL  = 1, 
    CSIDE_RWALL  = 2, 
    CSIDE_ROOF   = 3, 
    CSIDE_ENTITY = 4, // Introduced in Origins Plus
};

enum CollisionModes {
    CMODE_FLOOR = 0,
    CMODE_LWALL = 1,
    CMODE_ROOF  = 2,
    CMODE_RWALL = 3,
};

enum CollisionSolidity {
    SOLID_ALL  = 0,
    SOLID_TOP  = 1,
    SOLID_LRB  = 2,
    SOLID_NONE = 3,
};

enum ObjectCollisionTypes {
    C_TOUCH    = 0,
    C_BOX      = 1,
    C_BOX2     = 2,
    C_PLATFORM = 3,
    C_BOX3     = 4, // Introduced in Origins Plus
    C_ENEMY    = 5, // Introduced in Origins Plus
};

struct CollisionSensor {
    int32 XPos;
    int32 YPos;
    int32 angle;
    bool32 collided;
};

extern int32 collisionLeft;
extern int32 collisionTop;
extern int32 collisionRight;
extern int32 collisionBottom;

extern CollisionSensor sensors[6];

// Introduced in Origins Plus
struct CollisionStore {
    int32 entityNo;
    int8 type;
    int32 left;
    int32 right;
    int32 top;
    int32 bottom;
};
extern CollisionStore collisionStorage[2];

enum EntityCollisionEffects {
    ECEFFECT_NONE         = 0,
    ECEFFECT_RESETSTORAGE = 1,
    ECEFFECT_BOXCOL3      = 2,
};

#if !RETRO_USE_ORIGINAL_CODE
int32 AddDebugHitbox(uint8 type, Entity *entity, int32 left, int32 top, int32 right, int32 bottom);
#endif

Hitbox *GetPlayerHitbox(Player *player);

void FindFloorPosition(Player *player, CollisionSensor *sensor, int32 startYPos);
void FindLWallPosition(Player *player, CollisionSensor *sensor, int32 startXPos);
void FindRoofPosition(Player *player, CollisionSensor *sensor, int32 startYPos);
void FindRWallPosition(Player *player, CollisionSensor *sensor, int32 startXPos);
void FloorCollision(Player *player, CollisionSensor *sensor);
void LWallCollision(Player *player, CollisionSensor *sensor);
void RoofCollision(Player *player, CollisionSensor *sensor);
void RWallCollision(Player *player, CollisionSensor *sensor);
void SetPathGripSensors(Player *player);

void ProcessPathGrip(Player *player);
void ProcessAirCollision(Player *player);

void ProcessPlayerTileCollisions(Player *player);

void TouchCollision(int32 left, int32 top, int32 right, int32 bottom);
void BoxCollision(int32 left, int32 top, int32 right, int32 bottom);  // Standard
void BoxCollision2(int32 left, int32 top, int32 right, int32 bottom); // Updated (?)
void PlatformCollision(int32 left, int32 top, int32 right, int32 bottom);
void BoxCollision3(int32 left, int32 top, int32 right, int32 bottom); // Added in Origins Plus
void EnemyCollision(int32 left, int32 top, int32 right, int32 bottom); // Added in Origins Plus

void ObjectFloorCollision(int32 xOffset, int32 yOffset, int32 cPath);
void ObjectLWallCollision(int32 xOffset, int32 yOffset, int32 cPath);
void ObjectRoofCollision(int32 xOffset, int32 yOffset, int32 cPath);
void ObjectRWallCollision(int32 xOffset, int32 yOffset, int32 cPath);

void ObjectFloorGrip(int32 xOffset, int32 yOffset, int32 cPath);
void ObjectLWallGrip(int32 xOffset, int32 yOffset, int32 cPath);
void ObjectRoofGrip(int32 xOffset, int32 yOffset, int32 cPath);
void ObjectRWallGrip(int32 xOffset, int32 yOffset, int32 cPath);
void ObjectEntityGrip(int32 direction, int32 extendBottomCol, int32 effect); // Added in Origins Plus

} // namespace v3

} // namespace Legacy