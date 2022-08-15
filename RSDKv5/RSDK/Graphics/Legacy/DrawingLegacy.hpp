
namespace Legacy
{

#define LEGACY_SURFACE_COUNT (24)
#define LEGACY_GFXDATA_SIZE  (0x800 * 0x800)

#define LEGACY_DRAWLAYER_COUNT (8)

enum DrawFXFlags { FX_SCALE, FX_ROTATE, FX_ROTOZOOM, FX_INK, FX_TINT, FX_FLIP };

struct DrawListEntry {
    int32 entityRefs[LEGACY_v4_ENTITY_COUNT];
    int32 listSize;
};

struct GFXSurface {
    char fileName[0x40];
    int32 height;
    int32 width;
    int32 widthShift;
    int32 depth;
    int32 dataPosition;
};

extern int32 SCREEN_XSIZE;
extern int32 SCREEN_CENTERX;

extern DrawListEntry drawListEntries[LEGACY_DRAWLAYER_COUNT];

extern int32 gfxDataPosition;
extern GFXSurface gfxSurface[LEGACY_SURFACE_COUNT];
extern uint8 graphicData[LEGACY_GFXDATA_SIZE];

extern uint16 blendLookupTable[0x20 * 0x100];
extern uint16 subtractLookupTable[0x20 * 0x100];
extern uint16 tintLookupTable[0x10000];

void GenerateBlendLookupTable();

inline void ClearGraphicsData()
{
    for (int32 i = 0; i < LEGACY_SURFACE_COUNT; ++i) MEM_ZERO(gfxSurface[i]);
    gfxDataPosition = 0;
}
void ClearScreen(uint8 index);

// TileLayer Drawing
void DrawHLineScrollLayer(int32 layerID);
void DrawVLineScrollLayer(int32 layerID);
void Draw3DFloorLayer(int32 layerID);
void Draw3DSkyLayer(int32 layerID);

// Shape Drawing
void DrawRectangle(int32 XPos, int32 YPos, int32 width, int32 height, int32 R, int32 G, int32 B, int32 A);
void DrawTintRectangle(int32 XPos, int32 YPos, int32 width, int32 height);
void DrawScaledTintMask(int32 direction, int32 XPos, int32 YPos, int32 pivotX, int32 pivotY, int32 scaleX, int32 scaleY, int32 width, int32 height,
                        int32 sprX, int32 sprY, int32 sheetID);

// Sprite Drawing
void DrawSprite(int32 XPos, int32 YPos, int32 width, int32 height, int32 sprX, int32 sprY, int32 sheetID);
void DrawSpriteFlipped(int32 XPos, int32 YPos, int32 width, int32 height, int32 sprX, int32 sprY, int32 direction, int32 sheetID);
void DrawSpriteScaled(int32 direction, int32 XPos, int32 YPos, int32 pivotX, int32 pivotY, int32 scaleX, int32 scaleY, int32 width, int32 height,
                      int32 sprX, int32 sprY, int32 sheetID);
void DrawSpriteRotated(int32 direction, int32 XPos, int32 YPos, int32 pivotX, int32 pivotY, int32 sprX, int32 sprY, int32 width, int32 height,
                       int32 rotation, int32 sheetID);
void DrawSpriteRotozoom(int32 direction, int32 XPos, int32 YPos, int32 pivotX, int32 pivotY, int32 sprX, int32 sprY, int32 width, int32 height,
                        int32 rotation, int32 scale, int32 sheetID);

void DrawBlendedSprite(int32 XPos, int32 YPos, int32 width, int32 height, int32 sprX, int32 sprY, int32 sheetID);
void DrawAlphaBlendedSprite(int32 XPos, int32 YPos, int32 width, int32 height, int32 sprX, int32 sprY, int32 alpha, int32 sheetID);
void DrawAdditiveBlendedSprite(int32 XPos, int32 YPos, int32 width, int32 height, int32 sprX, int32 sprY, int32 alpha, int32 sheetID);
void DrawSubtractiveBlendedSprite(int32 XPos, int32 YPos, int32 width, int32 height, int32 sprX, int32 sprY, int32 alpha, int32 sheetID);

void DrawFace(void *v, uint32 color);
void DrawTexturedFace(void *v, uint8 sheetID);

namespace v4
{
void DrawObjectAnimation(void *objScr, void *ent, int32 XPos, int32 YPos);

void DrawFadedFace(void *v, uint32 color, uint32 fogColor, int32 alpha);
void DrawTexturedFaceBlended(void *v, uint8 sheetID);
} // namespace v4

namespace v3
{
void DrawObjectAnimation(void *objScr, void *ent, int32 XPos, int32 YPos);
}

void DrawTextMenu(void *menu, int32 XPos, int32 YPos);
void DrawTextMenuEntry(void *menu, int32 rowID, int32 XPos, int32 YPos, int32 textHighlight);
void DrawStageTextEntry(void *menu, int32 rowID, int32 XPos, int32 YPos, int32 textHighlight);
void DrawBlendedTextMenuEntry(void *menu, int32 rowID, int32 XPos, int32 YPos, int32 textHighlight);

} // namespace Legacy

#include "v3/DrawingLegacyv3.hpp"
#include "v4/DrawingLegacyv4.hpp"