
namespace Legacy
{

#define LEGACY_ANIFILE_COUNT     (0x100)
#define LEGACY_ANIMATION_COUNT   (0x400)
#define LEGACY_SPRITEFRAME_COUNT (0x1000)

#define LEGACY_HITBOX_COUNT     (0x20)
#define LEGACY_HITBOX_DIR_COUNT (0x8)

enum AnimRotationFlags { ROTSTYLE_NONE, ROTSTYLE_FULL, ROTSTYLE_45DEG, ROTSTYLE_STATICFRAMES };

struct AnimationFile {
    char fileName[0x20];
    int32 animCount;
    int32 aniListOffset;
    int32 hitboxListOffset;
};

struct SpriteAnimation {
    char name[16];
    uint8 frameCount;
    uint8 speed;
    uint8 loopPoint;
    uint8 rotationStyle;
    int32 frameListOffset;
};

struct SpriteFrame {
    int32 sprX;
    int32 sprY;
    int32 width;
    int32 height;
    int32 pivotX;
    int32 pivotY;
    uint8 sheetID;
    uint8 hitboxID;
};

struct Hitbox {
    int8 left[LEGACY_HITBOX_DIR_COUNT];
    int8 top[LEGACY_HITBOX_DIR_COUNT];
    int8 right[LEGACY_HITBOX_DIR_COUNT];
    int8 bottom[LEGACY_HITBOX_DIR_COUNT];
};

extern AnimationFile animationFileList[LEGACY_ANIFILE_COUNT];
extern int32 animationFileCount;

extern SpriteFrame scriptFrames[LEGACY_SPRITEFRAME_COUNT];
extern int32 scriptFrameCount;

extern SpriteFrame animFrames[LEGACY_SPRITEFRAME_COUNT];
extern int32 animFrameCount;
extern SpriteAnimation animationList[LEGACY_ANIMATION_COUNT];
extern int32 animationCount;
extern Hitbox hitboxList[LEGACY_HITBOX_COUNT];
extern int32 hitboxCount;

void LoadAnimationFile(char *filePath);
void ClearAnimationData();

AnimationFile *AddAnimationFile(char *filePath);

inline AnimationFile *GetDefaultAnimationRef() { return &animationFileList[0]; }

namespace v3
{
void ProcessObjectAnimation(void *objScr, void *ent);
}

namespace v4
{
void ProcessObjectAnimation(void *objScr, void *ent);
}

} // namespace Legacy