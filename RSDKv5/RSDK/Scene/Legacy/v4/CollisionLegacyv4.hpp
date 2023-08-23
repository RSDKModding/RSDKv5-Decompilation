
namespace Legacy
{

namespace v4
{

enum CollisionSides {
    CSIDE_FLOOR = 0,
    CSIDE_LWALL = 1,
    CSIDE_RWALL = 2,
    CSIDE_ROOF  = 3,
};

enum CollisionModes {
    CMODE_FLOOR = 0,
    CMODE_LWALL = 1,
    CMODE_ROOF  = 2,
    CMODE_RWALL = 3,
};

enum CollisionSolidity {
    SOLID_ALL        = 0,
    SOLID_TOP        = 1,
    SOLID_LRB        = 2,
    SOLID_NONE       = 3,
    SOLID_TOP_NOGRIP = 4,
};

enum ObjectCollisionTypes {
    C_TOUCH    = 0,
    C_SOLID    = 1,
    C_SOLID2   = 2,
    C_PLATFORM = 3,
};

enum ObjectCollisionFlags {
    C_BOX = 0x10000,
};

struct CollisionSensor {
    int32 xpos;
    int32 ypos;
    int32 angle;
    bool32 collided;
};

extern int32 collisionLeft;
extern int32 collisionTop;
extern int32 collisionRight;
extern int32 collisionBottom;

extern int32 collisionTolerance;

extern CollisionSensor sensors[7];

#if !RETRO_USE_ORIGINAL_CODE
int32 AddDebugHitbox(uint8 type, Entity *entity, int32 left, int32 top, int32 right, int32 bottom);
#endif

Hitbox *GetHitbox(Entity *entity);

void FindFloorPosition(Entity *player, CollisionSensor *sensor, int32 startYPos);
void FindLWallPosition(Entity *player, CollisionSensor *sensor, int32 startXPos);
void FindRoofPosition(Entity *player, CollisionSensor *sensor, int32 startYPos);
void FindRWallPosition(Entity *player, CollisionSensor *sensor, int32 startXPos);

void FloorCollision(Entity *player, CollisionSensor *sensor);
void LWallCollision(Entity *player, CollisionSensor *sensor);
void RoofCollision(Entity *player, CollisionSensor *sensor);
void RWallCollision(Entity *player, CollisionSensor *sensor);

void SetPathGripSensors(Entity *player);
void ProcessPathGrip(Entity *player);
void ProcessAirCollision(Entity *player);

void ProcessTileCollisions(Entity *player);

void TouchCollision(Entity *thisEntity, int32 thisLeft, int32 thisTop, int32 thisRight, int32 thisBottom, Entity *otherEntity, int32 otherLeft,
                    int32 otherTop, int32 otherRight, int32 otherBottom);
void BoxCollision(Entity *thisEntity, int32 thisLeft, int32 thisTop, int32 thisRight, int32 thisBottom, Entity *otherEntity, int32 otherLeft,
                  int32 otherTop, int32 otherRight, int32 otherBottom); // Standard
void BoxCollision2(Entity *thisEntity, int32 thisLeft, int32 thisTop, int32 thisRight, int32 thisBottom, Entity *otherEntity, int32 otherLeft,
                   int32 otherTop, int32 otherRight, int32 otherBottom); // Updated (?)
void PlatformCollision(Entity *thisEntity, int32 thisLeft, int32 thisTop, int32 thisRight, int32 thisBottom, Entity *otherEntity, int32 otherLeft,
                       int32 otherTop, int32 otherRight, int32 otherBottom);

void ObjectFloorCollision(int32 xOffset, int32 yOffset, int32 cPath);
void ObjectLWallCollision(int32 xOffset, int32 yOffset, int32 cPath);
void ObjectRoofCollision(int32 xOffset, int32 yOffset, int32 cPath);
void ObjectRWallCollision(int32 xOffset, int32 yOffset, int32 cPath);

void ObjectFloorGrip(int32 xOffset, int32 yOffset, int32 cPath);
void ObjectLWallGrip(int32 xOffset, int32 yOffset, int32 cPath);
void ObjectRoofGrip(int32 xOffset, int32 yOffset, int32 cPath);
void ObjectRWallGrip(int32 xOffset, int32 yOffset, int32 cPath);
void ObjectLEntityGrip(int32 xOffset, int32 yOffset, int32 cPath);
void ObjectREntityGrip(int32 xOffset, int32 yOffset, int32 cPath);

} // namespace v4

} // namespace Legacy