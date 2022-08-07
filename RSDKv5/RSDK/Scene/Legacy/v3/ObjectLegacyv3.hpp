
namespace Legacy
{

namespace v3
{
#define LEGACY_v3_ENTITY_COUNT     (0x4A0)
#define LEGACY_v3_TEMPENTITY_START (LEGACY_v3_ENTITY_COUNT - 0x80)
#define LEGACY_v3_OBJECT_COUNT     (0x100)

struct Entity {
    int32 XPos;
    int32 YPos;
    int32 values[8];
    int32 scale;
    int32 rotation;
    int32 animationTimer;
    int32 animationSpeed;
    uint8 type;
    uint8 propertyValue;
    uint8 state;
    uint8 priority;
    uint8 drawOrder;
    uint8 direction;
    uint8 inkEffect;
    uint8 alpha;
    uint8 animation;
    uint8 prevAnimation;
    uint8 frame;
};

enum ObjectTypes {
    OBJ_TYPE_BLANKOBJECT = 0 // 0 is always blank obj
};

enum ObjectPriority {
    // The entity is active if the entity is on screen or within 128 pixels of the screen borders on any axis
    PRIORITY_BOUNDS,
    // The entity is always active, unless the stage state is PAUSED or FROZEN
    PRIORITY_ACTIVE,
    // Same as PRIORITY_ACTIVE, the entity even runs when the stage state is PAUSED or FROZEN
    PRIORITY_ALWAYS,
    // Same as PRIORITY_BOUNDS, however it only does checks on the x-axis, so when in bounds on the x-axis, the y position doesn't matter
    PRIORITY_XBOUNDS,
    // Same as PRIORITY_BOUNDS, however the entity's type will be set to BLANK OBJECT when it becomes inactive
    PRIORITY_BOUNDS_DESTROY,
    // Never Active.
    PRIORITY_INACTIVE,
};

extern int32 objectLoop;
extern int32 curObjectType;
extern Entity objectEntityList[LEGACY_v3_ENTITY_COUNT];

extern char typeNames[LEGACY_v3_OBJECT_COUNT][0x40];

void ProcessStartupObjects();
void ProcessObjects();
void ProcessPausedObjects();

void SetObjectTypeName(const char *objectName, int32 objectID);
}

}