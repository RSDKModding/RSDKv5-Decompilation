
namespace Legacy
{

namespace v3
{
#define LEGACY_v3_PLAYER_COUNT (2)

enum PlayerControlModes {
    CONTROLMODE_NONE     = -1,
    CONTROLMODE_NORMAL   = 0,
    CONTROLMODE_SIDEKICK = 1,
};

struct Player {
    int32 entityNo;
    int32 XPos;
    int32 YPos;
    int32 XVelocity;
    int32 YVelocity;
    int32 speed;
    int32 screenXPos;
    int32 screenYPos;
    int32 angle;
    int32 timer;
    int32 lookPos;
    int32 values[8];
    uint8 collisionMode;
    uint8 skidding;
    uint8 pushing;
    uint8 collisionPlane;
    int8 controlMode;
    uint8 controlLock;
    int32 topSpeed;
    int32 acceleration;
    int32 deceleration;
    int32 airAcceleration;
    int32 airDeceleration;
    int32 gravityStrength;
    int32 jumpStrength;
    int32 jumpCap;
    int32 rollingAcceleration;
    int32 rollingDeceleration;
    uint8 visible;
    uint8 tileCollisions;
    uint8 objectInteractions;
    uint8 left;
    uint8 right;
    uint8 up;
    uint8 down;
    uint8 jumpPress;
    uint8 jumpHold;
    uint8 followPlayer1;
    uint8 trackScroll;
    uint8 gravity;
    uint8 water;
    uint8 flailing[3];
    AnimationFile *animationFile;
    Entity *boundEntity;
};

extern Player playerList[LEGACY_v3_PLAYER_COUNT];
extern int32 playerListPos;
extern int32 activePlayer;
extern int32 activePlayerCount;

extern uint16 upBuffer;
extern uint16 downBuffer;
extern uint16 leftBuffer;
extern uint16 rightBuffer;
extern uint16 jumpPressBuffer;
extern uint16 jumpHoldBuffer;

void ProcessPlayerControl(Player *player);
} // namespace v3

} // namespace Legacy