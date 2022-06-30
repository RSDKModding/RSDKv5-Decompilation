#include "v3/SceneLegacyv3.hpp"

#include "v4/SceneLegacyv4.hpp"

namespace Legacy
{
#define LEGACY_LAYER_COUNT    (9)
#define LEGACY_DEFORM_STORE   (256)
#define LEGACY_DEFORM_SIZE    (320)
#define LEGACY_DEFORM_COUNT   (LEGACY_DEFORM_STORE + LEGACY_DEFORM_SIZE)
#define LEGACY_PARALLAX_COUNT (0x100)

#define LEGACY_TILE_COUNT    (0x400)
#define LEGACY_TILE_SIZE     (0x10)
#define LEGACY_CHUNK_SIZE    (0x80)
#define LEGACY_TILE_DATASIZE (LEGACY_TILE_SIZE * LEGACY_TILE_SIZE)
#define LEGACY_TILESET_SIZE  (LEGACY_TILE_COUNT * LEGACY_TILE_DATASIZE)

#define LEGACY_TILELAYER_CHUNK_W          (0x100)
#define LEGACY_TILELAYER_CHUNK_H          (0x100)
#define LEGACY_TILELAYER_CHUNK_COUNT      (LEGACY_TILELAYER_CHUNK_W * LEGACY_TILELAYER_CHUNK_H)
#define LEGACY_TILELAYER_LINESCROLL_COUNT (LEGACY_TILELAYER_CHUNK_H * LEGACY_CHUNK_SIZE)

#define LEGACY_CHUNKTILE_COUNT (0x200 * (8 * 8))

#define LEGACY_CPATH_COUNT (2)

enum StageListNames {
    STAGELIST_PRESENTATION,
    STAGELIST_REGULAR,
    STAGELIST_SPECIAL,
    STAGELIST_BONUS,
    STAGELIST_MAX, // StageList size
};

enum TileLayerTypes {
    LAYER_NOSCROLL,
    LAYER_HSCROLL,
    LAYER_VSCROLL,
    LAYER_3DFLOOR,
    LAYER_3DSKY,
};

enum TileInfo {
    TILEINFO_INDEX,
    TILEINFO_DIRECTION,
    TILEINFO_VISUALPLANE,
    TILEINFO_SOLIDITYA,
    TILEINFO_SOLIDITYB,
    TILEINFO_FLAGSA,
    TILEINFO_ANGLEA,
    TILEINFO_FLAGSB,
    TILEINFO_ANGLEB,
};

enum DeformationModes {
    DEFORM_FG,
    DEFORM_FG_WATER,
    DEFORM_BG,
    DEFORM_BG_WATER,
};

enum CameraStyles {
    CAMERASTYLE_FOLLOW,
    CAMERASTYLE_EXTENDED,
    CAMERASTYLE_EXTENDED_OFFSET_L,
    CAMERASTYLE_EXTENDED_OFFSET_R,
    CAMERASTYLE_HLOCKED,
    CAMERASTYLE_FIXED,
    CAMERASTYLE_STATIC,
};

enum StageModes {
    STAGEMODE_LOAD,
    STAGEMODE_NORMAL,
    STAGEMODE_PAUSED,

    STAGEMODE_STEPOVER = 8,
};

struct Camera {
    int32 xpos;
    int32 ypos;
    uint32 target;
    int32 adjustY;
    uint8 enabled;
    uint8 unknown1;
    uint8 unknown2;
    uint8 style;
};

struct CollisionMasks {
    int8 floorMasks[LEGACY_TILE_COUNT * LEGACY_TILE_SIZE];
    int8 lWallMasks[LEGACY_TILE_COUNT * LEGACY_TILE_SIZE];
    int8 rWallMasks[LEGACY_TILE_COUNT * LEGACY_TILE_SIZE];
    int8 roofMasks[LEGACY_TILE_COUNT * LEGACY_TILE_SIZE];
    uint32 angles[LEGACY_TILE_COUNT];
    uint8 flags[LEGACY_TILE_COUNT];
};

struct Tiles128x128 {
    int32 gfxDataPos[LEGACY_CHUNKTILE_COUNT];
    uint16 tileIndex[LEGACY_CHUNKTILE_COUNT];
    uint8 direction[LEGACY_CHUNKTILE_COUNT];
    uint8 visualPlane[LEGACY_CHUNKTILE_COUNT];
    uint8 collisionFlags[LEGACY_CPATH_COUNT][LEGACY_CHUNKTILE_COUNT];
};

struct TileLayer {
    uint16 tiles[LEGACY_TILELAYER_CHUNK_COUNT];
    uint8 lineScroll[LEGACY_TILELAYER_LINESCROLL_COUNT];
    int32 parallaxFactor;
    int32 scrollSpeed;
    int32 scrollPos;
    int32 angle;
    int32 xpos;
    int32 ypos;
    int32 zpos;
    int32 deformationOffset;
    int32 deformationOffsetW;
    uint8 type;
    uint8 xsize;
    uint8 ysize;
};

struct LineScroll {
    int32 parallaxFactor[LEGACY_PARALLAX_COUNT];
    int32 scrollSpeed[LEGACY_PARALLAX_COUNT];
    int32 scrollPos[LEGACY_PARALLAX_COUNT];
    int32 linePos[LEGACY_PARALLAX_COUNT];
    int32 deform[LEGACY_PARALLAX_COUNT];
    uint8 entryCount;
};

extern int32 stageMode;

extern Camera cameras[2];
extern Camera *currentCamera;

extern int32 xScrollOffset;
extern int32 yScrollOffset;
extern int32 cameraShift;
extern int32 cameraLockedY;
extern int32 cameraShakeX;
extern int32 cameraShakeY;
extern int32 cameraLag;
extern int32 cameraLagStyle;

extern int32 curXBoundary1;
extern int32 newXBoundary1;
extern int32 curYBoundary1;
extern int32 newYBoundary1;
extern int32 curXBoundary2;
extern int32 curYBoundary2;
extern int32 waterLevel;
extern int32 waterDrawPos;
extern int32 newXBoundary2;
extern int32 newYBoundary2;

extern int32 SCREEN_SCROLL_LEFT;
extern int32 SCREEN_SCROLL_RIGHT;
#define LEGACY_SCREEN_SCROLL_UP   ((SCREEN_YSIZE / 2) - 16)
#define LEGACY_SCREEN_SCROLL_DOWN ((SCREEN_YSIZE / 2) + 16)

extern int32 lastXSize;
extern int32 lastYSize;

extern bool32 pauseEnabled;
extern bool32 timeEnabled;
extern bool32 debugMode;
extern int32 frameCounter;
extern int32 stageMilliseconds;
extern int32 stageSeconds;
extern int32 stageMinutes;

extern bool32 anyPress;

// Category and Scene IDs
extern char currentStageFolder[0x100];
extern int32 actID;

extern char titleCardText[0x100];
extern uint8 titleCardWord2;

extern uint8 activeTileLayers[4];
extern uint8 tLayerMidPoint;
extern TileLayer stageLayouts[LEGACY_LAYER_COUNT];

extern int32 bgDeformationData0[LEGACY_DEFORM_COUNT];
extern int32 bgDeformationData1[LEGACY_DEFORM_COUNT];
extern int32 bgDeformationData2[LEGACY_DEFORM_COUNT];
extern int32 bgDeformationData3[LEGACY_DEFORM_COUNT];

extern LineScroll hParallax;
extern LineScroll vParallax;

extern Tiles128x128 tiles128x128;
extern CollisionMasks collisionMasks[2];

extern uint16 tile3DFloorBuffer[0x100 * 0x100];

extern uint8 tilesetGFXData[LEGACY_TILESET_SIZE];

inline void ResetCurrentStageFolder() { strcpy(currentStageFolder, ""); }
inline bool CheckCurrentStageFolder()
{
    if (strcmp(currentStageFolder, sceneInfo.listData[sceneInfo.listPos].folder) == 0) {
        return true;
    }
    else {
        strcpy(currentStageFolder, sceneInfo.listData[sceneInfo.listPos].folder);
        return false;
    }
}

void GetStageFilepath(char *dest, const char *filePath);
int32 LoadStageFile(const char *filePath, FileInfo *info);
int32 LoadActFile(const char *ext, FileInfo *info);

void LoadStageChunks();
void LoadStageCollisions();
void LoadStageGIFFile();

void ProcessInput();
void ProcessSceneTimer();

void ResetBackgroundSettings();
void SetLayerDeformation(int32 selectedDef, int32 waveLength, int32 waveWidth, int32 waveType, int32 y, int32 waveSize);

inline void Init3DFloorBuffer(int32 layerID)
{
    for (int32 y = 0; y < LEGACY_TILELAYER_CHUNK_H; ++y) {
        for (int32 x = 0; x < LEGACY_TILELAYER_CHUNK_W; ++x) {
            int32 c                         = stageLayouts[layerID].tiles[(x >> 3) + (y >> 3 << 8)] << 6;
            int32 tx                        = x & 7;
            tile3DFloorBuffer[x + (y << 8)] = c + tx + ((y & 7) << 3);
        }
    }
}

inline void Copy16x16Tile(uint16 dest, uint16 src)
{
    uint8 *destPtr = &tilesetGFXData[LEGACY_TILELAYER_CHUNK_W * dest];
    uint8 *srcPtr  = &tilesetGFXData[LEGACY_TILELAYER_CHUNK_W * src];

    int32 cnt      = LEGACY_TILE_DATASIZE;
    while (cnt--) *destPtr++ = *srcPtr++;
}

void InitCameras();

}