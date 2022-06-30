
namespace Legacy
{

namespace v3
{

enum CollisionSidess {
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

void ObjectFloorCollision(int32 xOffset, int32 yOffset, int32 cPath);
void ObjectLWallCollision(int32 xOffset, int32 yOffset, int32 cPath);
void ObjectRoofCollision(int32 xOffset, int32 yOffset, int32 cPath);
void ObjectRWallCollision(int32 xOffset, int32 yOffset, int32 cPath);

void ObjectFloorGrip(int32 xOffset, int32 yOffset, int32 cPath);
void ObjectLWallGrip(int32 xOffset, int32 yOffset, int32 cPath);
void ObjectRoofGrip(int32 xOffset, int32 yOffset, int32 cPath);
void ObjectRWallGrip(int32 xOffset, int32 yOffset, int32 cPath);

}

}