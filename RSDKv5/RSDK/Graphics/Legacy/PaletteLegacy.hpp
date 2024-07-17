
namespace Legacy
{
#define LEGACY_PALETTE_COUNT       (0x8)
#define LEGACY_PALETTE_COLOR_COUNT (0x100)

struct Color {
    uint8 r;
    uint8 g;
    uint8 b;
    uint8 a;
};

// Palettes (as RGB565 Colors)
extern uint16 fullPalette[LEGACY_PALETTE_COUNT][LEGACY_PALETTE_COLOR_COUNT];
extern uint16 *activePalette; // Pointers to the 256 color set thats active

extern uint8 gfxLineBuffer[SCREEN_YSIZE * 2]; // Pointers to active palette
extern int32 GFX_LINESIZE;
extern int32 GFX_LINESIZE_MINUSONE;
extern int32 GFX_LINESIZE_DOUBLE;
extern int32 GFX_FRAMEBUFFERSIZE;
extern int32 GFX_FBUFFERMINUSONE;

extern int32 fadeMode;
extern uint8 fadeA;
extern uint8 fadeR;
extern uint8 fadeG;
extern uint8 fadeB;

extern int32 paletteMode;

void LoadPalette(const char *filePath, int32 paletteID, int32 startPaletteIndex, int32 startIndex, int32 endIndex);

inline void SetActivePalette(uint8 newActivePal, int32 startLine, int32 endLine)
{
    if (newActivePal < LEGACY_PALETTE_COUNT)
        for (int32 l = startLine; l < endLine && l < SCREEN_YSIZE; l++) gfxLineBuffer[l] = newActivePal;

    activePalette = fullPalette[gfxLineBuffer[0]];
}

inline void SetPaletteEntry(uint8 paletteIndex, uint8 index, uint8 r, uint8 g, uint8 b)
{
    if (paletteIndex != 0xFF) {
        fullPalette[paletteIndex][index] = PACK_RGB888(r, g, b);
    }
    else {
        activePalette[index] = PACK_RGB888(r, g, b);
    }
}

inline void SetPaletteEntryPacked(uint8 paletteIndex, uint8 index, uint32 color)
{
    fullPalette[paletteIndex][index] = PACK_RGB888((uint8)(color >> 16), (uint8)(color >> 8), (uint8)(color >> 0));
}

inline uint32 GetPaletteEntryPacked(uint8 bankID, uint8 index)
{
    // 0xF800 = 1111 1000 0000 0000 = R
    // 0x7E0  = 0000 0111 1110 0000 = G
    // 0x1F   = 0000 0000 0001 1111 = B
    uint16 clr = fullPalette[bankID & 7][index];

    int32 R = (clr & 0xF800) << 8;
    int32 G = (clr & 0x7E0) << 5;
    int32 B = (clr & 0x1F) << 3;
    return R | G | B;
}

inline void CopyPalette(uint8 sourcePalette, uint8 srcPaletteStart, uint8 destinationPalette, uint8 destPaletteStart, uint8 count)
{
    if (sourcePalette < LEGACY_PALETTE_COUNT && destinationPalette < LEGACY_PALETTE_COUNT) {
        for (int32 i = 0; i < count; ++i) {
            fullPalette[destinationPalette][destPaletteStart + i] = fullPalette[sourcePalette][srcPaletteStart + i];
        }
    }
}

inline void RotatePalette(int32 palID, uint8 startIndex, uint8 endIndex, bool right)
{
    if (right) {
        uint16 startClr = fullPalette[palID][endIndex];
        for (int32 i = endIndex; i > startIndex; --i) {
            fullPalette[palID][i] = fullPalette[palID][i - 1];
        }
        fullPalette[palID][startIndex] = startClr;
    }
    else {
        uint16 startClr = fullPalette[palID][startIndex];
        for (int32 i = startIndex; i < endIndex; ++i) {
            fullPalette[palID][i] = fullPalette[palID][i + 1];
        }
        fullPalette[palID][endIndex] = startClr;
    }
}

inline void SetFade(uint8 R, uint8 G, uint8 B, uint16 A)
{
    fadeMode = 1;
    fadeR    = R;
    fadeG    = G;
    fadeB    = B;
    fadeA    = A > 0xFF ? 0xFF : A;
}

void SetPaletteFade(uint8 destPaletteID, uint8 srcPaletteA, uint8 srcPaletteB, uint16 blendAmount, int32 startIndex, int32 endIndex);

namespace v3
{

inline void CopyPalette(uint8 sourcePalette, uint8 destinationPalette)
{
    if (sourcePalette < LEGACY_PALETTE_COUNT && destinationPalette < LEGACY_PALETTE_COUNT) {
        for (int32 i = 0; i < LEGACY_PALETTE_COLOR_COUNT; ++i) {
            fullPalette[destinationPalette][i] = fullPalette[sourcePalette][i];
        }
    }
}

inline void RotatePalette(uint8 startIndex, uint8 endIndex, bool right)
{
    if (right) {
        uint16 startClr = activePalette[endIndex];
        for (int32 i = endIndex; i > startIndex; --i) {
            activePalette[i] = activePalette[i - 1];
        }
        activePalette[startIndex] = startClr;
    }
    else {
        uint16 startClr = activePalette[startIndex];
        for (int32 i = startIndex; i < endIndex; ++i) {
            activePalette[i] = activePalette[i + 1];
        }
        activePalette[endIndex] = startClr;
    }
}

void SetLimitedFade(uint8 paletteID, uint8 R, uint8 G, uint8 B, uint16 blendAmount, int32 startIndex, int32 endIndex);
} // namespace v3

} // namespace Legacy