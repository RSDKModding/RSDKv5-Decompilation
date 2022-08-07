
namespace Legacy
{

namespace v4
{
#define LEGACY_v4_ENTITY_COUNT     (0x4A0)
#define LEGACY_v4_TEMPENTITY_START (LEGACY_v4_ENTITY_COUNT - 0x80)
#define LEGACY_v4_OBJECT_COUNT     (0x100)
#define LEGACY_v4_TYPEGROUP_COUNT  (0x103)

enum ObjectControlModes {
    CONTROLMODE_NONE   = -1,
    CONTROLMODE_NORMAL = 0,
};

struct TypeGroupList {
    int entityRefs[LEGACY_v4_ENTITY_COUNT];
    int listSize;
};

struct Entity {
    int32 xpos;
    int32 ypos;
    int32 xvel;
    int32 yvel;
    int32 speed;
    int32 values[48];
    int32 state;
    int32 angle;
    int32 scale;
    int32 rotation;
    int32 alpha;
    int32 animationTimer;
    int32 animationSpeed;
    int32 lookPosX;
    int32 lookPosY;
    uint16 groupID;
    uint8 type;
    uint8 propertyValue;
    uint8 priority;
    uint8 drawOrder;
    uint8 direction;
    uint8 inkEffect;
    uint8 animation;
    uint8 prevAnimation;
    uint8 frame;
    uint8 collisionMode;
    uint8 collisionPlane;
    int8 controlMode;
    uint8 controlLock;
    uint8 pushing;
    uint8 visible;
    uint8 tileCollisions;
    uint8 objectInteractions;
    uint8 gravity;
    uint8 left;
    uint8 right;
    uint8 up;
    uint8 down;
    uint8 jumpPress;
    uint8 jumpHold;
    uint8 scrollTracking;
    uint8 floorSensors[5];
};

enum ObjectTypes {
    OBJ_TYPE_BLANKOBJECT = 0 // 0 is always blank obj
};

enum ObjectGroups {
    GROUP_ALL = 0 // 0 is always "all"
};

enum ObjectPriority {
    // The entity is active if the entity is on screen or within 128 pixels of the screen borders on any axis
    PRIORITY_BOUNDS,
    // The entity is always active, unless the stage state is PAUSED/FROZEN
    PRIORITY_ACTIVE,
    // Same as PRIORITY_ACTIVE, the entity even runs when the stage state is PAUSED/FROZEN
    PRIORITY_ALWAYS,
    // Same as PRIORITY_BOUNDS, however it only does checks on the x-axis, so when in bounds on the x-axis, the y position doesn't matter
    PRIORITY_XBOUNDS,
    // Same as PRIORITY_XBOUNDS, however the entity's type will be set to BLANK OBJECT when it becomes inactive
    PRIORITY_XBOUNDS_DESTROY,
    // Never Active.
    PRIORITY_INACTIVE,
    // Same as PRIORITY_BOUNDS, but uses the smaller bounds (32px off screen rather than the normal 128)
    PRIORITY_BOUNDS_SMALL,
    // Same as PRIORITY_ACTIVE, but uses the smaller bounds in object.outOfBounds
    PRIORITY_ACTIVE_SMALL
};

// Game Objects
extern int32 objectEntityPos;
extern int32 curObjectType;
extern Entity objectEntityList[LEGACY_v4_ENTITY_COUNT * 2];
extern int32 processObjectFlag[LEGACY_v4_ENTITY_COUNT];
extern TypeGroupList objectTypeGroupList[LEGACY_v4_TYPEGROUP_COUNT];

extern char typeNames[LEGACY_v4_OBJECT_COUNT][0x40];

extern int32 playerListPos;

void ProcessStartupObjects();
void ProcessObjects();
void ProcessPausedObjects();
void ProcessFrozenObjects();
void Process2PObjects();

void SetObjectTypeName(const char *objectName, int32 objectID);

void ProcessObjectControl(Entity *player);
} // namespace v4

} // namespace Legacy