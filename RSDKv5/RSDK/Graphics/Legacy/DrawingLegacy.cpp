

#include "v3/DrawingLegacyv3.cpp"
#include "v4/DrawingLegacyv4.cpp"

int32 RSDK::Legacy::SCREEN_XSIZE   = 424;
int32 RSDK::Legacy::SCREEN_CENTERX = 424 / 2;

RSDK::Legacy::DrawListEntry RSDK::Legacy::drawListEntries[LEGACY_DRAWLAYER_COUNT];

int32 RSDK::Legacy::gfxDataPosition;
RSDK::Legacy::GFXSurface RSDK::Legacy::gfxSurface[LEGACY_SURFACE_COUNT];
uint8 RSDK::Legacy::graphicData[LEGACY_GFXDATA_SIZE];

uint16 RSDK::Legacy::blendLookupTable[0x20 * 0x100];
uint16 RSDK::Legacy::subtractLookupTable[0x20 * 0x100];
uint16 RSDK::Legacy::tintLookupTable[0x10000];

void RSDK::Legacy::GenerateBlendLookupTable()
{
    for (int32 y = 0; y < 0x100; y++) {
        for (int32 x = 0; x < 0x20; x++) {
            blendLookupTable[x + (0x20 * y)]    = y * x >> 8;
            subtractLookupTable[x + (0x20 * y)] = y * (0x1F - x) >> 8;
        }
    }

    for (int32 i = 0; i < 0x10000; i++) {
        int32 tintValue    = ((i & 0x1F) + ((i & 0x7E0) >> 6) + ((i & 0xF800) >> 11)) / 3 + 6;
        tintLookupTable[i] = 0x841 * MIN(tintValue, 0x1F);
    }
}

void RSDK::Legacy::ClearScreen(uint8 index)
{
    uint16 color        = activePalette[index];
    uint16 *framebuffer = currentScreen->frameBuffer;

    int32 cnt = GFX_LINESIZE * SCREEN_YSIZE;
    while (cnt--) {
        *framebuffer = color;
        ++framebuffer;
    }
}

void RSDK::Legacy::DrawHLineScrollLayer(int32 layerID)
{
    TileLayer *layer = &stageLayouts[activeTileLayers[layerID]];
    if (!layer->xsize || !layer->ysize)
        return;

    int32 screenwidth16  = (GFX_LINESIZE >> 4) - 1;
    int32 layerwidth     = layer->xsize;
    int32 layerheight    = layer->ysize;
    bool32 aboveMidPoint = layerID >= tLayerMidPoint;

    uint8 *lineScroll;
    int32 *deformationData;
    int32 *deformationDataW;

    int32 yscrollOffset = 0;
    if (activeTileLayers[layerID]) { // BG Layer
        int32 yScroll    = yScrollOffset * layer->parallaxFactor >> 8;
        int32 fullheight = layerheight << 7;
        layer->scrollPos += layer->scrollSpeed;
        if (layer->scrollPos > fullheight << 16)
            layer->scrollPos -= fullheight << 16;
        yscrollOffset    = (yScroll + (layer->scrollPos >> 16)) % fullheight;
        layerheight      = fullheight >> 7;
        lineScroll       = layer->lineScroll;
        deformationData  = &bgDeformationData2[(uint8)(yscrollOffset + layer->deformationOffset)];
        deformationDataW = &bgDeformationData3[(uint8)(yscrollOffset + waterDrawPos + layer->deformationOffsetW)];
    }
    else { // FG Layer
        lastXSize     = layer->xsize;
        yscrollOffset = yScrollOffset;
        lineScroll    = layer->lineScroll;
        for (int32 i = 0; i < LEGACY_PARALLAX_COUNT; ++i) hParallax.linePos[i] = xScrollOffset;
        deformationData  = &bgDeformationData0[(uint8)(yscrollOffset + layer->deformationOffset)];
        deformationDataW = &bgDeformationData1[(uint8)(yscrollOffset + waterDrawPos + layer->deformationOffsetW)];
    }

    if (layer->type == LAYER_HSCROLL) {
        if (lastXSize != layerwidth) {
            int32 fullLayerwidth = layerwidth << 7;
            for (int32 i = 0; i < hParallax.entryCount; ++i) {
                hParallax.linePos[i] = xScrollOffset * hParallax.parallaxFactor[i] >> 8;
                if (hParallax.scrollPos[i] > fullLayerwidth << 16)
                    hParallax.scrollPos[i] -= fullLayerwidth << 16;
                if (hParallax.scrollPos[i] < 0)
                    hParallax.scrollPos[i] += fullLayerwidth << 16;
                hParallax.linePos[i] += hParallax.scrollPos[i] >> 16;
                hParallax.linePos[i] %= fullLayerwidth;
            }
        }
        int32 w = -1;
        if (activeTileLayers[layerID])
            w = layerwidth;
        lastXSize = w;
    }

    uint16 *frameBuffer = currentScreen->frameBuffer;
    uint8 *lineBuffer   = gfxLineBuffer;
    int32 tileYPos      = yscrollOffset % (layerheight << 7);
    if (tileYPos < 0)
        tileYPos += layerheight << 7;
    uint8 *scrollIndex = &lineScroll[tileYPos];
    int32 tileY16      = tileYPos & 0xF;
    int32 chunkY       = tileYPos >> 7;
    int32 tileY        = (tileYPos & 0x7F) >> 4;

    // Draw Above Water (if applicable)
    int32 drawableLines[2] = { waterDrawPos, SCREEN_YSIZE - waterDrawPos };
    for (int32 i = 0; i < 2; ++i) {
        while (drawableLines[i]--) {
            activePalette = fullPalette[*lineBuffer];
            lineBuffer++;

            int32 chunkX = hParallax.linePos[*scrollIndex];
            if (i == 0) {
                if (hParallax.deform[*scrollIndex])
                    chunkX += *deformationData;
                ++deformationData;
            }
            else {
                if (hParallax.deform[*scrollIndex])
                    chunkX += *deformationDataW;
                ++deformationDataW;
            }
            ++scrollIndex;

            int32 fullLayerwidth = layerwidth << 7;
            if (chunkX < 0)
                chunkX += fullLayerwidth;
            if (chunkX >= fullLayerwidth)
                chunkX -= fullLayerwidth;

            int32 chunkXPos         = chunkX >> 7;
            int32 tilePxXPos        = chunkX & 0xF;
            int32 tileXPxRemain     = TILE_SIZE - tilePxXPos;
            int32 chunk             = (layer->tiles[(chunkX >> 7) + (chunkY << 8)] << 6) + ((chunkX & 0x7F) >> 4) + 8 * tileY;
            int32 tileOffsetY       = TILE_SIZE * tileY16;
            int32 tileOffsetYFlipX  = TILE_SIZE * tileY16 + 0xF;
            int32 tileOffsetYFlipY  = TILE_SIZE * (0xF - tileY16);
            int32 tileOffsetYFlipXY = TILE_SIZE * (0xF - tileY16) + 0xF;
            int32 lineRemain        = GFX_LINESIZE;

            uint8 *pixels       = NULL;
            int32 tilePxLineCnt = tileXPxRemain;

            // Draw the first tile to the left
            if (tiles128x128.visualPlane[chunk] == (uint8)aboveMidPoint) {
                lineRemain -= tilePxLineCnt;
                switch (tiles128x128.direction[chunk]) {
                    case FLIP_NONE:
                        pixels = &tilesetGFXData[tileOffsetY + tiles128x128.gfxDataPos[chunk] + tilePxXPos];
                        while (tilePxLineCnt--) {
                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;
                        }
                        break;

                    case FLIP_X:

                        pixels = &tilesetGFXData[tileOffsetYFlipX + tiles128x128.gfxDataPos[chunk] - tilePxXPos];
                        while (tilePxLineCnt--) {
                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;
                        }
                        break;

                    case FLIP_Y:
                        pixels = &tilesetGFXData[tileOffsetYFlipY + tiles128x128.gfxDataPos[chunk] + tilePxXPos];
                        while (tilePxLineCnt--) {
                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;
                        }
                        break;

                    case FLIP_XY:
                        pixels = &tilesetGFXData[tileOffsetYFlipXY + tiles128x128.gfxDataPos[chunk] - tilePxXPos];
                        while (tilePxLineCnt--) {
                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;
                        }
                        break;

                    default: break;
                }
            }
            else {
                frameBuffer += tilePxLineCnt;
                lineRemain -= tilePxLineCnt;
            }

            // Draw the bulk of the tiles
            int32 chunkTileX   = ((chunkX & 0x7F) >> 4) + 1;
            int32 tilesPerLine = screenwidth16;
            while (tilesPerLine--) {
                if (chunkTileX < 8) {
                    ++chunk;
                }
                else {
                    if (++chunkXPos == layerwidth)
                        chunkXPos = 0;

                    chunkTileX = 0;
                    chunk      = (layer->tiles[chunkXPos + (chunkY << 8)] << 6) + 8 * tileY;
                }
                lineRemain -= TILE_SIZE;

                // Loop Unrolling (faster but messier code)
                if (tiles128x128.visualPlane[chunk] == (uint8)aboveMidPoint) {
                    switch (tiles128x128.direction[chunk]) {
                        case FLIP_NONE:
                            pixels = &tilesetGFXData[tiles128x128.gfxDataPos[chunk] + tileOffsetY];
                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            break;

                        case FLIP_X:
                            pixels = &tilesetGFXData[tiles128x128.gfxDataPos[chunk] + tileOffsetYFlipX];
                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            break;

                        case FLIP_Y:
                            pixels = &tilesetGFXData[tiles128x128.gfxDataPos[chunk] + tileOffsetYFlipY];
                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            ++pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            break;

                        case FLIP_XY:
                            pixels = &tilesetGFXData[tiles128x128.gfxDataPos[chunk] + tileOffsetYFlipXY];
                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            --pixels;

                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++frameBuffer;
                            break;
                    }
                }
                else {
                    frameBuffer += TILE_SIZE;
                }
                ++chunkTileX;
            }

            // Draw any remaining tiles
            while (lineRemain > 0) {
                if (chunkTileX++ < 8) {
                    ++chunk;
                }
                else {
                    chunkTileX = 0;
                    if (++chunkXPos == layerwidth)
                        chunkXPos = 0;

                    chunk = (layer->tiles[chunkXPos + (chunkY << 8)] << 6) + 8 * tileY;
                }

                tilePxLineCnt = lineRemain >= TILE_SIZE ? TILE_SIZE : lineRemain;
                lineRemain -= tilePxLineCnt;
                if (tiles128x128.visualPlane[chunk] == (uint8)aboveMidPoint) {
                    switch (tiles128x128.direction[chunk]) {
                        case FLIP_NONE:
                            pixels = &tilesetGFXData[tiles128x128.gfxDataPos[chunk] + tileOffsetY];
                            while (tilePxLineCnt--) {
                                if (*pixels > 0)
                                    *frameBuffer = activePalette[*pixels];
                                ++frameBuffer;
                                ++pixels;
                            }
                            break;

                        case FLIP_X:
                            pixels = &tilesetGFXData[tiles128x128.gfxDataPos[chunk] + tileOffsetYFlipX];
                            while (tilePxLineCnt--) {
                                if (*pixels > 0)
                                    *frameBuffer = activePalette[*pixels];
                                ++frameBuffer;
                                --pixels;
                            }
                            break;

                        case FLIP_Y:
                            pixels = &tilesetGFXData[tiles128x128.gfxDataPos[chunk] + tileOffsetYFlipY];
                            while (tilePxLineCnt--) {
                                if (*pixels > 0)
                                    *frameBuffer = activePalette[*pixels];
                                ++frameBuffer;
                                ++pixels;
                            }
                            break;

                        case FLIP_XY:
                            pixels = &tilesetGFXData[tiles128x128.gfxDataPos[chunk] + tileOffsetYFlipXY];
                            while (tilePxLineCnt--) {
                                if (*pixels > 0)
                                    *frameBuffer = activePalette[*pixels];
                                ++frameBuffer;
                                --pixels;
                            }
                            break;

                        default: break;
                    }
                }
                else {
                    frameBuffer += tilePxLineCnt;
                }
            }

            if (++tileY16 >= TILE_SIZE) {
                tileY16 = 0;
                ++tileY;
            }

            if (tileY >= 8) {
                if (++chunkY == layerheight) {
                    chunkY = 0;
                    scrollIndex -= 0x80 * layerheight;
                }
                tileY = 0;
            }
        }
    }
}
void RSDK::Legacy::DrawVLineScrollLayer(int32 layerID)
{
    TileLayer *layer = &stageLayouts[activeTileLayers[layerID]];
    if (!layer->xsize || !layer->ysize)
        return;

    int32 layerwidth   = layer->xsize;
    int32 layerheight  = layer->ysize;
    bool aboveMidPoint = layerID >= tLayerMidPoint;

    uint8 *lineScroll;
    int32 *deformationData;

    int32 xscrollOffset = 0;
    if (activeTileLayers[layerID]) { // BG Layer
        int32 xScroll        = xScrollOffset * layer->parallaxFactor >> 8;
        int32 fullLayerwidth = layerwidth << 7;
        layer->scrollPos += layer->scrollSpeed;
        if (layer->scrollPos > fullLayerwidth << 16)
            layer->scrollPos -= fullLayerwidth << 16;
        xscrollOffset   = (xScroll + (layer->scrollPos >> 16)) % fullLayerwidth;
        layerwidth      = fullLayerwidth >> 7;
        lineScroll      = layer->lineScroll;
        deformationData = &bgDeformationData2[(uint8)(xscrollOffset + layer->deformationOffset)];
    }
    else { // FG Layer
        lastYSize            = layer->ysize;
        xscrollOffset        = xScrollOffset;
        lineScroll           = layer->lineScroll;
        vParallax.linePos[0] = yScrollOffset;
        vParallax.deform[0]  = true;
        deformationData      = &bgDeformationData0[(uint8)(xScrollOffset + layer->deformationOffset)];
    }

    if (layer->type == LAYER_VSCROLL) {
        if (lastYSize != layerheight) {
            int32 fullLayerheight = layerheight << 7;
            for (int32 i = 0; i < vParallax.entryCount; ++i) {
                vParallax.linePos[i] = yScrollOffset * vParallax.parallaxFactor[i] >> 8;

                vParallax.scrollPos[i] += vParallax.scrollPos[i] << 16;
                if (vParallax.scrollPos[i] > fullLayerheight << 16)
                    vParallax.scrollPos[i] -= fullLayerheight << 16;

                vParallax.linePos[i] += vParallax.scrollPos[i] >> 16;
                vParallax.linePos[i] %= fullLayerheight;
            }
            layerheight = fullLayerheight >> 7;
        }
        lastYSize = layerheight;
    }

    uint16 *frameBuffer = currentScreen->frameBuffer;
    activePalette       = fullPalette[gfxLineBuffer[0]];
    int32 tileXPos      = xscrollOffset % (layerheight << 7);
    if (tileXPos < 0)
        tileXPos += layerheight << 7;
    uint8 *scrollIndex = &lineScroll[tileXPos];
    int32 chunkX       = tileXPos >> 7;
    int32 tileX16      = tileXPos & 0xF;
    int32 tileX        = (tileXPos & 0x7F) >> 4;

    // Draw Above Water (if applicable)
    int32 drawableLines = SCREEN_XSIZE;
    while (drawableLines--) {
        int32 chunkY = vParallax.linePos[*scrollIndex];
        if (vParallax.deform[*scrollIndex])
            chunkY += *deformationData;
        ++deformationData;
        ++scrollIndex;

        int32 fullLayerHeight = layerheight << 7;
        if (chunkY < 0)
            chunkY += fullLayerHeight;
        if (chunkY >= fullLayerHeight)
            chunkY -= fullLayerHeight;

        int32 chunkYPos         = chunkY >> 7;
        int32 tileY             = chunkY & 0xF;
        int32 tileYPxRemain     = TILE_SIZE - tileY;
        int32 chunk             = (layer->tiles[chunkX + (chunkY >> 7 << 8)] << 6) + tileX + 8 * ((chunkY & 0x7F) >> 4);
        int32 tileOffsetXFlipX  = 0xF - tileX16;
        int32 tileOffsetXFlipY  = tileX16 + SCREEN_YSIZE;
        int32 tileOffsetXFlipXY = 0xFF - tileX16;
        int32 lineRemain        = SCREEN_YSIZE;

        uint8 *pixels       = NULL;
        int32 tilePxLineCnt = tileYPxRemain;

        // Draw the first tile to the left
        if (tiles128x128.visualPlane[chunk] == (uint8)aboveMidPoint) {
            lineRemain -= tilePxLineCnt;
            switch (tiles128x128.direction[chunk]) {
                case FLIP_NONE:
                    pixels = &tilesetGFXData[TILE_SIZE * tileY + tileX16 + tiles128x128.gfxDataPos[chunk]];
                    while (tilePxLineCnt--) {
                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;
                    }
                    break;

                case FLIP_X:
                    pixels = &tilesetGFXData[TILE_SIZE * tileY + tileOffsetXFlipX + tiles128x128.gfxDataPos[chunk]];
                    while (tilePxLineCnt--) {
                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;
                    }
                    break;

                case FLIP_Y:
                    pixels = &tilesetGFXData[tileOffsetXFlipY + tiles128x128.gfxDataPos[chunk] - TILE_SIZE * tileY];
                    while (tilePxLineCnt--) {
                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;
                    }
                    break;

                case FLIP_XY:
                    pixels = &tilesetGFXData[tileOffsetXFlipXY + tiles128x128.gfxDataPos[chunk] - TILE_SIZE * tileY];
                    while (tilePxLineCnt--) {
                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;
                    }
                    break;

                default: break;
            }
        }
        else {
            frameBuffer += GFX_LINESIZE * tileYPxRemain;
            lineRemain -= tilePxLineCnt;
        }

        // Draw the bulk of the tiles
        int32 chunkTileY   = ((chunkY & 0x7F) >> 4) + 1;
        int32 tilesPerLine = (SCREEN_YSIZE >> 4) - 1;

        while (tilesPerLine--) {
            if (chunkTileY < 8) {
                chunk += 8;
            }
            else {
                if (++chunkYPos == layerheight)
                    chunkYPos = 0;

                chunkTileY = 0;
                chunk      = (layer->tiles[chunkX + (chunkYPos << 8)] << 6) + tileX;
            }
            lineRemain -= TILE_SIZE;

            // Loop Unrolling (faster but messier code)
            if (tiles128x128.visualPlane[chunk] == (uint8)aboveMidPoint) {
                switch (tiles128x128.direction[chunk]) {
                    case FLIP_NONE:
                        pixels = &tilesetGFXData[tiles128x128.gfxDataPos[chunk] + tileX16];
                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        break;

                    case FLIP_X:
                        pixels = &tilesetGFXData[tiles128x128.gfxDataPos[chunk] + tileOffsetXFlipX];
                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels += TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        break;

                    case FLIP_Y:
                        pixels = &tilesetGFXData[tiles128x128.gfxDataPos[chunk] + tileOffsetXFlipY];
                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        break;

                    case FLIP_XY:
                        pixels = &tilesetGFXData[tiles128x128.gfxDataPos[chunk] + tileOffsetXFlipXY];
                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        pixels -= TILE_SIZE;

                        if (*pixels > 0)
                            *frameBuffer = activePalette[*pixels];
                        frameBuffer += GFX_LINESIZE;
                        break;
                }
            }
            else {
                frameBuffer += GFX_LINESIZE * TILE_SIZE;
            }
            ++chunkTileY;
        }

        // Draw any remaining tiles
        while (lineRemain > 0) {
            if (chunkTileY < 8) {
                chunk += 8;
            }
            else {
                if (++chunkYPos == layerheight)
                    chunkYPos = 0;

                chunkTileY = 0;
                chunk      = (layer->tiles[chunkX + (chunkYPos << 8)] << 6) + tileX;
            }

            tilePxLineCnt = lineRemain >= TILE_SIZE ? TILE_SIZE : lineRemain;
            lineRemain -= tilePxLineCnt;

            if (tiles128x128.visualPlane[chunk] == (uint8)aboveMidPoint) {
                switch (tiles128x128.direction[chunk]) {
                    case FLIP_NONE:
                        pixels = &tilesetGFXData[tiles128x128.gfxDataPos[chunk] + tileX16];
                        while (tilePxLineCnt--) {
                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            frameBuffer += GFX_LINESIZE;
                            pixels += TILE_SIZE;
                        }
                        break;

                    case FLIP_X:
                        pixels = &tilesetGFXData[tiles128x128.gfxDataPos[chunk] + tileOffsetXFlipX];
                        while (tilePxLineCnt--) {
                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            frameBuffer += GFX_LINESIZE;
                            pixels += TILE_SIZE;
                        }
                        break;

                    case FLIP_Y:
                        pixels = &tilesetGFXData[tiles128x128.gfxDataPos[chunk] + tileOffsetXFlipY];
                        while (tilePxLineCnt--) {
                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            frameBuffer += GFX_LINESIZE;
                            pixels -= TILE_SIZE;
                        }
                        break;

                    case FLIP_XY:
                        pixels = &tilesetGFXData[tiles128x128.gfxDataPos[chunk] + tileOffsetXFlipXY];
                        while (tilePxLineCnt--) {
                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            frameBuffer += GFX_LINESIZE;
                            pixels -= TILE_SIZE;
                        }
                        break;

                    default: break;
                }
            }
            else {
                frameBuffer += GFX_LINESIZE * tilePxLineCnt;
            }
            chunkTileY++;
        }

        if (++tileX16 >= TILE_SIZE) {
            tileX16 = 0;
            ++tileX;
        }

        if (tileX >= 8) {
            if (++chunkX == layerwidth) {
                chunkX = 0;
                scrollIndex -= 0x80 * layerwidth;
            }
            tileX = 0;
        }

        frameBuffer -= GFX_FBUFFERMINUSONE;
    }
}
void RSDK::Legacy::Draw3DFloorLayer(int32 layerID)
{
    TileLayer *layer = &stageLayouts[activeTileLayers[layerID]];
    if (!layer->xsize || !layer->ysize)
        return;

    int32 layerWidth    = layer->xsize << 7;
    int32 layerHeight   = layer->ysize << 7;
    int32 layerYPos     = layer->ypos;
    int32 layerZPos     = layer->zpos;
    int32 sinValue      = sinM7LookupTable[layer->angle];
    int32 cosValue      = cosM7LookupTable[layer->angle];
    uint8 *lineBuffer   = &gfxLineBuffer[(SCREEN_YSIZE / 2) + 12];
    uint16 *frameBuffer = &currentScreen->frameBuffer[((SCREEN_YSIZE / 2) + 12) * GFX_LINESIZE];
    int32 layerXPos     = layer->xpos >> 4;
    int32 ZBuffer       = layerZPos >> 4;

    for (int32 i = 4; i < 112; ++i) {
        if (!(i & 1)) {
            activePalette = fullPalette[*lineBuffer];
            lineBuffer++;
        }
        int32 XBuffer    = layerYPos / (i << 9) * -cosValue >> 8;
        int32 YBuffer    = sinValue * (layerYPos / (i << 9)) >> 8;
        int32 XPos       = layerXPos + (3 * sinValue * (layerYPos / (i << 9)) >> 2) - XBuffer * SCREEN_CENTERX;
        int32 YPos       = ZBuffer + (3 * cosValue * (layerYPos / (i << 9)) >> 2) - YBuffer * SCREEN_CENTERX;
        int32 lineBuffer = 0;
        while (lineBuffer < GFX_LINESIZE) {
            int32 tileX = XPos >> 12;
            int32 tileY = YPos >> 12;
            if (tileX > -1 && tileX < layerWidth && tileY > -1 && tileY < layerHeight) {
                int32 chunk   = tile3DFloorBuffer[(YPos >> 16 << 8) + (XPos >> 16)];
                uint8 *pixels = &tilesetGFXData[tiles128x128.gfxDataPos[chunk]];
                switch (tiles128x128.direction[chunk]) {
                    case FLIP_NONE: pixels += TILE_SIZE * (tileY & 0xF) + (tileX & 0xF); break;
                    case FLIP_X: pixels += TILE_SIZE * (tileY & 0xF) + 15 - (tileX & 0xF); break;
                    case FLIP_Y: pixels += (tileX & 0xF) + SCREEN_YSIZE - TILE_SIZE * (tileY & 0xF); break;
                    case FLIP_XY: pixels += 15 - (tileX & 0xF) + SCREEN_YSIZE - TILE_SIZE * (tileY & 0xF); break;
                    default: break;
                }

                if (*pixels > 0)
                    *frameBuffer = activePalette[*pixels];
            }
            ++frameBuffer;
            ++lineBuffer;

            XPos += XBuffer;
            YPos += YBuffer;
        }
    }
}
void RSDK::Legacy::Draw3DSkyLayer(int32 layerID)
{
    TileLayer *layer = &stageLayouts[activeTileLayers[layerID]];
    if (!layer->xsize || !layer->ysize)
        return;

    int32 layerWidth    = layer->xsize << 7;
    int32 layerHeight   = layer->ysize << 7;
    int32 layerYPos     = layer->ypos;
    int32 sinValue      = sinM7LookupTable[layer->angle & 0x1FF];
    int32 cosValue      = cosM7LookupTable[layer->angle & 0x1FF];
    uint16 *frameBuffer = &currentScreen->frameBuffer[((SCREEN_YSIZE / 2) + 12) * GFX_LINESIZE];
    uint8 *lineBuffer   = &gfxLineBuffer[((SCREEN_YSIZE / 2) + 12)];
    int32 layerXPos     = layer->xpos >> 4;
    int32 layerZPos     = layer->zpos >> 4;
    for (int32 i = TILE_SIZE / 2; i < SCREEN_YSIZE - TILE_SIZE; ++i) {
        if (!(i & 1)) {
            activePalette = fullPalette[*lineBuffer];
            lineBuffer++;
        }

        int32 xBuffer    = layerYPos / (i << 8) * -cosValue >> 9;
        int32 yBuffer    = sinValue * (layerYPos / (i << 8)) >> 9;
        int32 XPos       = layerXPos + (3 * sinValue * (layerYPos / (i << 8)) >> 2) - xBuffer * GFX_LINESIZE;
        int32 YPos       = layerZPos + (3 * cosValue * (layerYPos / (i << 8)) >> 2) - yBuffer * GFX_LINESIZE;
        int32 lineBuffer = 0;

        while (lineBuffer < GFX_LINESIZE * 2) {
            int32 tileX = XPos >> 12;
            int32 tileY = YPos >> 12;
            if (tileX > -1 && tileX < layerWidth && tileY > -1 && tileY < layerHeight) {
                int32 chunk   = tile3DFloorBuffer[(YPos >> 16 << 8) + (XPos >> 16)];
                uint8 *pixels = &tilesetGFXData[tiles128x128.gfxDataPos[chunk]];

                switch (tiles128x128.direction[chunk]) {
                    case FLIP_NONE: pixels += TILE_SIZE * (tileY & 0xF) + (tileX & 0xF); break;
                    case FLIP_X: pixels += TILE_SIZE * (tileY & 0xF) + 0xF - (tileX & 0xF); break;
                    case FLIP_Y: pixels += (tileX & 0xF) + SCREEN_YSIZE - TILE_SIZE * (tileY & 0xF); break;
                    case FLIP_XY: pixels += 0xF - (tileX & 0xF) + SCREEN_YSIZE - TILE_SIZE * (tileY & 0xF); break;
                    default: break;
                }

                if (*pixels > 0)
                    *frameBuffer = activePalette[*pixels];
            }

            if (lineBuffer & 1)
                ++frameBuffer;

            lineBuffer++;
            XPos += xBuffer;
            YPos += yBuffer;
        }

        if (!(i & 1))
            frameBuffer -= GFX_LINESIZE;
    }
}

void RSDK::Legacy::DrawRectangle(int32 XPos, int32 YPos, int32 width, int32 height, int32 R, int32 G, int32 B, int32 A)
{
    if (A > 0xFF)
        A = 0xFF;

    if (width + XPos > GFX_LINESIZE)
        width = GFX_LINESIZE - XPos;
    if (XPos < 0) {
        width += XPos;
        XPos = 0;
    }

    if (height + YPos > SCREEN_YSIZE)
        height = SCREEN_YSIZE - YPos;
    if (YPos < 0) {
        height += YPos;
        YPos = 0;
    }

    if (width <= 0 || height <= 0 || A <= 0)
        return;

    int32 pitch            = GFX_LINESIZE - width;
    uint16 *frameBufferPtr = &currentScreen->frameBuffer[XPos + GFX_LINESIZE * YPos];
    uint16 clr             = PACK_RGB888(R, G, B);

    if (A == 0xFF) {
        int32 h = height;
        while (h--) {
            int32 w = width;
            while (w--) {
                *frameBufferPtr = clr;
                ++frameBufferPtr;
            }
            frameBufferPtr += pitch;
        }
    }
    else {
        uint16 *fbufferBlend = &blendLookupTable[0x20 * (0xFF - A)];
        uint16 *pixelBlend   = &blendLookupTable[0x20 * A];

        int32 h = height;
        while (h--) {
            int32 w = width;
            while (w--) {
                int32 R = (fbufferBlend[(*frameBufferPtr & 0xF800) >> 11] + pixelBlend[(clr & 0xF800) >> 11]) << 11;
                int32 G = (fbufferBlend[(*frameBufferPtr & 0x7E0) >> 6] + pixelBlend[(clr & 0x7E0) >> 6]) << 6;
                int32 B = fbufferBlend[*frameBufferPtr & 0x1F] + pixelBlend[clr & 0x1F];

                *frameBufferPtr = R | G | B;
                ++frameBufferPtr;
            }
            frameBufferPtr += pitch;
        }
    }
}

void RSDK::Legacy::DrawTintRectangle(int32 XPos, int32 YPos, int32 width, int32 height)
{
    if (width + XPos > GFX_LINESIZE)
        width = GFX_LINESIZE - XPos;
    if (XPos < 0) {
        width += XPos;
        XPos = 0;
    }

    if (height + YPos > SCREEN_YSIZE)
        height = SCREEN_YSIZE - YPos;
    if (YPos < 0) {
        height += YPos;
        YPos = 0;
    }
    if (width <= 0 || height <= 0)
        return;
    int32 yOffset = GFX_LINESIZE - width;
    for (uint16 *frameBufferPtr = &currentScreen->frameBuffer[XPos + GFX_LINESIZE * YPos];; frameBufferPtr += yOffset) {
        height--;
        if (!height)
            break;
        int32 w = width;
        while (w--) {
            *frameBufferPtr = tintLookupTable[*frameBufferPtr];
            ++frameBufferPtr;
        }
    }
}
void RSDK::Legacy::DrawScaledTintMask(int32 direction, int32 XPos, int32 YPos, int32 pivotX, int32 pivotY, int32 scaleX, int32 scaleY, int32 width,
                                      int32 height, int32 sprX, int32 sprY, int32 sheetID)
{
    int32 roundedYPos = 0;
    int32 roundedXPos = 0;
    int32 truescaleX  = 4 * scaleX;
    int32 truescaleY  = 4 * scaleY;
    int32 widthM1     = width - 1;
    int32 trueXPos    = XPos - (truescaleX * pivotX >> 11);
    int32 trueYPos    = YPos - (truescaleY * pivotY >> 11);
    width             = truescaleX * width >> 11;
    height            = truescaleY * height >> 11;
    int32 finalscaleX = (int32)(float)((float)(2048.0 / (float)truescaleX) * 2048.0);
    int32 finalscaleY = (int32)(float)((float)(2048.0 / (float)truescaleY) * 2048.0);

    if (width + trueXPos > GFX_LINESIZE) {
        width = GFX_LINESIZE - trueXPos;
    }

    if (direction) {
        if (trueXPos < 0) {
            widthM1 -= trueXPos * -finalscaleX >> 11;
            roundedXPos = (uint16)trueXPos * -(int16)finalscaleX & 0x7FF;
            width += trueXPos;
            trueXPos = 0;
        }
    }
    else if (trueXPos < 0) {
        sprX += trueXPos * -finalscaleX >> 11;
        roundedXPos = (uint16)trueXPos * -(int16)finalscaleX & 0x7FF;
        width += trueXPos;
        trueXPos = 0;
    }

    if (height + trueYPos > SCREEN_YSIZE) {
        height = SCREEN_YSIZE - trueYPos;
    }
    if (trueYPos < 0) {
        sprY += trueYPos * -finalscaleY >> 11;
        roundedYPos = (uint16)trueYPos * -(int16)finalscaleY & 0x7FF;
        height += trueYPos;
        trueYPos = 0;
    }

    if (width <= 0 || height <= 0)
        return;

    GFXSurface *surface = &gfxSurface[sheetID];
    int32 pitch         = GFX_LINESIZE - width;
    int32 gfxwidth      = surface->width;
    // uint8 *lineBuffer       = &gfxLineBuffer[trueYPos];
    uint8 *gfxData         = &graphicData[sprX + surface->width * sprY + surface->dataPosition];
    uint16 *frameBufferPtr = &currentScreen->frameBuffer[trueXPos + GFX_LINESIZE * trueYPos];
    if (direction == FLIP_X) {
        uint8 *gfxDataPtr = &gfxData[widthM1];
        int32 gfxPitch    = 0;
        while (height--) {
            int32 roundXPos = roundedXPos;
            int32 w         = width;
            while (w--) {
                if (*gfxDataPtr > 0)
                    *frameBufferPtr = tintLookupTable[*frameBufferPtr];
                int32 offsetX = finalscaleX + roundXPos;
                gfxDataPtr -= offsetX >> 11;
                gfxPitch += offsetX >> 11;
                roundXPos = offsetX & 0x7FF;
                ++frameBufferPtr;
            }
            frameBufferPtr += pitch;
            int32 offsetY = finalscaleY + roundedYPos;
            gfxDataPtr += gfxPitch + (offsetY >> 11) * gfxwidth;
            roundedYPos = offsetY & 0x7FF;
            gfxPitch    = 0;
        }
    }
    else {
        int32 gfxPitch = 0;
        int32 h        = height;
        while (h--) {
            int32 roundXPos = roundedXPos;
            int32 w         = width;
            while (w--) {
                if (*gfxData > 0)
                    *frameBufferPtr = tintLookupTable[*frameBufferPtr];
                int32 offsetX = finalscaleX + roundXPos;
                gfxData += offsetX >> 11;
                gfxPitch += offsetX >> 11;
                roundXPos = offsetX & 0x7FF;
                ++frameBufferPtr;
            }
            frameBufferPtr += pitch;
            int32 offsetY = finalscaleY + roundedYPos;
            gfxData += (offsetY >> 11) * gfxwidth - gfxPitch;
            roundedYPos = offsetY & 0x7FF;
            gfxPitch    = 0;
        }
    }
}

void RSDK::Legacy::DrawSprite(int32 XPos, int32 YPos, int32 width, int32 height, int32 sprX, int32 sprY, int32 sheetID)
{
    if (width + XPos > GFX_LINESIZE)
        width = GFX_LINESIZE - XPos;
    if (XPos < 0) {
        sprX -= XPos;
        width += XPos;
        XPos = 0;
    }
    if (height + YPos > SCREEN_YSIZE)
        height = SCREEN_YSIZE - YPos;
    if (YPos < 0) {
        sprY -= YPos;
        height += YPos;
        YPos = 0;
    }
    if (width <= 0 || height <= 0)
        return;

    GFXSurface *surface = &gfxSurface[sheetID];
    int32 pitch         = GFX_LINESIZE - width;
    int32 gfxPitch      = surface->width - width;
    uint8 *lineBuffer   = &gfxLineBuffer[YPos];
    uint8 *pixels       = &graphicData[sprX + surface->width * sprY + surface->dataPosition];
    uint16 *frameBuffer = &currentScreen->frameBuffer[XPos + GFX_LINESIZE * YPos];

    while (height--) {
        activePalette = fullPalette[*lineBuffer];
        lineBuffer++;

        int32 w = width;
        while (w--) {
            if (*pixels > 0)
                *frameBuffer = activePalette[*pixels];
            ++pixels;
            ++frameBuffer;
        }

        frameBuffer += pitch;
        pixels += gfxPitch;
    }
}

void RSDK::Legacy::DrawSpriteFlipped(int32 XPos, int32 YPos, int32 width, int32 height, int32 sprX, int32 sprY, int32 direction, int32 sheetID)
{
    int32 widthFlip  = width;
    int32 heightFlip = height;

    if (width + XPos > GFX_LINESIZE) {
        width = GFX_LINESIZE - XPos;
    }
    if (XPos < 0) {
        sprX -= XPos;
        width += XPos;
        widthFlip += XPos + XPos;
        XPos = 0;
    }
    if (height + YPos > SCREEN_YSIZE) {
        height = SCREEN_YSIZE - YPos;
    }
    if (YPos < 0) {
        sprY -= YPos;
        height += YPos;
        heightFlip += YPos + YPos;
        YPos = 0;
    }
    if (width <= 0 || height <= 0)
        return;

    GFXSurface *surface = &gfxSurface[sheetID];
    int32 pitch;
    int32 gfxPitch;
    uint8 *lineBuffer;
    uint8 *pixels;
    uint16 *frameBuffer;
    switch (direction) {
        case FLIP_NONE:
            pitch       = GFX_LINESIZE - width;
            gfxPitch    = surface->width - width;
            lineBuffer  = &gfxLineBuffer[YPos];
            pixels      = &graphicData[sprX + surface->width * sprY + surface->dataPosition];
            frameBuffer = &currentScreen->frameBuffer[XPos + GFX_LINESIZE * YPos];

            while (height--) {
                activePalette = fullPalette[*lineBuffer];
                lineBuffer++;

                int32 w = width;
                while (w--) {
                    if (*pixels > 0)
                        *frameBuffer = activePalette[*pixels];
                    ++pixels;
                    ++frameBuffer;
                }

                frameBuffer += pitch;
                pixels += gfxPitch;
            }
            break;

        case FLIP_X:
            pitch       = GFX_LINESIZE - width;
            gfxPitch    = width + surface->width;
            lineBuffer  = &gfxLineBuffer[YPos];
            pixels      = &graphicData[widthFlip - 1 + sprX + surface->width * sprY + surface->dataPosition];
            frameBuffer = &currentScreen->frameBuffer[XPos + GFX_LINESIZE * YPos];
            while (height--) {
                activePalette = fullPalette[*lineBuffer];

                lineBuffer++;
                int32 w = width;
                while (w--) {
                    if (*pixels > 0)
                        *frameBuffer = activePalette[*pixels];
                    --pixels;
                    ++frameBuffer;
                }

                frameBuffer += pitch;
                pixels += gfxPitch;
            }
            break;

        case FLIP_Y:
            pitch       = GFX_LINESIZE - width;
            gfxPitch    = width + surface->width;
            lineBuffer  = &gfxLineBuffer[YPos];
            pixels      = &graphicData[sprX + surface->width * (sprY + heightFlip - 1) + surface->dataPosition];
            frameBuffer = &currentScreen->frameBuffer[XPos + GFX_LINESIZE * YPos];
            while (height--) {
                activePalette = fullPalette[*lineBuffer];
                lineBuffer++;

                int32 w = width;
                while (w--) {
                    if (*pixels > 0)
                        *frameBuffer = activePalette[*pixels];
                    ++pixels;
                    ++frameBuffer;
                }

                frameBuffer += pitch;
                pixels -= gfxPitch;
            }
            break;

        case FLIP_XY:
            pitch       = GFX_LINESIZE - width;
            gfxPitch    = surface->width - width;
            lineBuffer  = &gfxLineBuffer[YPos];
            pixels      = &graphicData[widthFlip - 1 + sprX + surface->width * (sprY + heightFlip - 1) + surface->dataPosition];
            frameBuffer = &currentScreen->frameBuffer[XPos + GFX_LINESIZE * YPos];
            while (height--) {
                activePalette = fullPalette[*lineBuffer];
                lineBuffer++;

                int32 w = width;
                while (w--) {
                    if (*pixels > 0)
                        *frameBuffer = activePalette[*pixels];
                    --pixels;
                    ++frameBuffer;
                }

                frameBuffer += pitch;
                pixels -= gfxPitch;
            }
            break;

        default: break;
    }
}
void RSDK::Legacy::DrawSpriteScaled(int32 direction, int32 XPos, int32 YPos, int32 pivotX, int32 pivotY, int32 scaleX, int32 scaleY, int32 width,
                                    int32 height, int32 sprX, int32 sprY, int32 sheetID)
{
    int32 roundedYPos = 0;
    int32 roundedXPos = 0;
    int32 truescaleX  = 4 * scaleX;
    int32 truescaleY  = 4 * scaleY;
    int32 widthM1     = width - 1;
    int32 trueXPos    = XPos - (truescaleX * pivotX >> 11);
    int32 trueYPos    = YPos - (truescaleY * pivotY >> 11);
    width             = truescaleX * width >> 11;
    height            = truescaleY * height >> 11;
    int32 finalscaleX = (int32)(float)((float)(2048.0 / (float)truescaleX) * 2048.0);
    int32 finalscaleY = (int32)(float)((float)(2048.0 / (float)truescaleY) * 2048.0);
    if (width + trueXPos > GFX_LINESIZE) {
        width = GFX_LINESIZE - trueXPos;
    }

    if (direction) {
        if (trueXPos < 0) {
            widthM1 -= trueXPos * -finalscaleX >> 11;
            roundedXPos = (uint16)trueXPos * -(int16)finalscaleX & 0x7FF;
            width += trueXPos;
            trueXPos = 0;
        }
    }
    else if (trueXPos < 0) {
        sprX += trueXPos * -finalscaleX >> 11;
        roundedXPos = (uint16)trueXPos * -(int16)finalscaleX & 0x7FF;
        width += trueXPos;
        trueXPos = 0;
    }

    if (height + trueYPos > SCREEN_YSIZE) {
        height = SCREEN_YSIZE - trueYPos;
    }
    if (trueYPos < 0) {
        sprY += trueYPos * -finalscaleY >> 11;
        roundedYPos = (uint16)trueYPos * -(int16)finalscaleY & 0x7FF;
        height += trueYPos;
        trueYPos = 0;
    }

    if (width <= 0 || height <= 0)
        return;

    GFXSurface *surface = &gfxSurface[sheetID];
    int32 pitch         = GFX_LINESIZE - width;
    int32 gfxwidth      = surface->width;
    uint8 *lineBuffer   = &gfxLineBuffer[trueYPos];
    uint8 *pixels       = &graphicData[sprX + surface->width * sprY + surface->dataPosition];
    uint16 *frameBuffer = &currentScreen->frameBuffer[trueXPos + GFX_LINESIZE * trueYPos];

    if (direction == FLIP_X) {
        uint8 *pixelsPtr = &pixels[widthM1];
        int32 gfxPitch   = 0;

        while (height--) {
            activePalette = fullPalette[*lineBuffer];
            lineBuffer++;

            int32 roundXPos = roundedXPos;
            int32 w         = width;
            while (w--) {
                if (*pixelsPtr > 0)
                    *frameBuffer = activePalette[*pixelsPtr];
                int32 offsetX = finalscaleX + roundXPos;
                pixelsPtr -= offsetX >> 11;
                gfxPitch += offsetX >> 11;
                roundXPos = offsetX & 0x7FF;
                ++frameBuffer;
            }

            frameBuffer += pitch;
            int32 offsetY = finalscaleY + roundedYPos;
            pixelsPtr += gfxPitch + (offsetY >> 11) * gfxwidth;
            roundedYPos = offsetY & 0x7FF;
            gfxPitch    = 0;
        }
    }
    else {
        int32 gfxPitch = 0;
        int32 h        = height;
        while (h--) {
            activePalette = fullPalette[*lineBuffer];
            lineBuffer++;

            int32 roundXPos = roundedXPos;
            int32 w         = width;
            while (w--) {
                if (*pixels > 0)
                    *frameBuffer = activePalette[*pixels];
                int32 offsetX = finalscaleX + roundXPos;
                pixels += offsetX >> 11;
                gfxPitch += offsetX >> 11;
                roundXPos = offsetX & 0x7FF;
                ++frameBuffer;
            }

            frameBuffer += pitch;
            int32 offsetY = finalscaleY + roundedYPos;
            pixels += (offsetY >> 11) * gfxwidth - gfxPitch;
            roundedYPos = offsetY & 0x7FF;
            gfxPitch    = 0;
        }
    }
}
void RSDK::Legacy::DrawSpriteRotated(int32 direction, int32 XPos, int32 YPos, int32 pivotX, int32 pivotY, int32 sprX, int32 sprY, int32 width,
                                     int32 height, int32 rotation, int32 sheetID)
{
    int32 sprXPos    = (pivotX + sprX) << 9;
    int32 sprYPos    = (pivotY + sprY) << 9;
    int32 fullwidth  = width + sprX;
    int32 fullheight = height + sprY;
    int32 angle      = rotation & 0x1FF;
    if (angle < 0)
        angle += 0x200;
    if (angle)
        angle = 0x200 - angle;
    int32 sine   = sin512LookupTable[angle];
    int32 cosine = cos512LookupTable[angle];
    int32 xPositions[4];
    int32 yPositions[4];

    if (direction == FLIP_X) {
        xPositions[0] = XPos + ((sine * (-pivotY - 2) + cosine * (pivotX + 2)) >> 9);
        yPositions[0] = YPos + ((cosine * (-pivotY - 2) - sine * (pivotX + 2)) >> 9);
        xPositions[1] = XPos + ((sine * (-pivotY - 2) + cosine * (pivotX - width - 2)) >> 9);
        yPositions[1] = YPos + ((cosine * (-pivotY - 2) - sine * (pivotX - width - 2)) >> 9);
        xPositions[2] = XPos + ((sine * (height - pivotY + 2) + cosine * (pivotX + 2)) >> 9);
        yPositions[2] = YPos + ((cosine * (height - pivotY + 2) - sine * (pivotX + 2)) >> 9);
        int32 a       = pivotX - width - 2;
        int32 b       = height - pivotY + 2;
        xPositions[3] = XPos + ((sine * b + cosine * a) >> 9);
        yPositions[3] = YPos + ((cosine * b - sine * a) >> 9);
    }
    else {
        xPositions[0] = XPos + ((sine * (-pivotY - 2) + cosine * (-pivotX - 2)) >> 9);
        yPositions[0] = YPos + ((cosine * (-pivotY - 2) - sine * (-pivotX - 2)) >> 9);
        xPositions[1] = XPos + ((sine * (-pivotY - 2) + cosine * (width - pivotX + 2)) >> 9);
        yPositions[1] = YPos + ((cosine * (-pivotY - 2) - sine * (width - pivotX + 2)) >> 9);
        xPositions[2] = XPos + ((sine * (height - pivotY + 2) + cosine * (-pivotX - 2)) >> 9);
        yPositions[2] = YPos + ((cosine * (height - pivotY + 2) - sine * (-pivotX - 2)) >> 9);
        int32 a       = width - pivotX + 2;
        int32 b       = height - pivotY + 2;
        xPositions[3] = XPos + ((sine * b + cosine * a) >> 9);
        yPositions[3] = YPos + ((cosine * b - sine * a) >> 9);
    }

    int32 left = GFX_LINESIZE;
    for (int32 i = 0; i < 4; ++i) {
        if (xPositions[i] < left)
            left = xPositions[i];
    }
    if (left < 0)
        left = 0;

    int32 right = 0;
    for (int32 i = 0; i < 4; ++i) {
        if (xPositions[i] > right)
            right = xPositions[i];
    }
    if (right > GFX_LINESIZE)
        right = GFX_LINESIZE;
    int32 maxX = right - left;

    int32 top = SCREEN_YSIZE;
    for (int32 i = 0; i < 4; ++i) {
        if (yPositions[i] < top)
            top = yPositions[i];
    }
    if (top < 0)
        top = 0;

    int32 bottom = 0;
    for (int32 i = 0; i < 4; ++i) {
        if (yPositions[i] > bottom)
            bottom = yPositions[i];
    }
    if (bottom > SCREEN_YSIZE)
        bottom = SCREEN_YSIZE;
    int32 maxY = bottom - top;

    if (maxX <= 0 || maxY <= 0)
        return;

    GFXSurface *surface = &gfxSurface[sheetID];
    int32 pitch         = GFX_LINESIZE - maxX;
    int32 lineSize      = surface->widthShift;
    uint16 *frameBuffer = &currentScreen->frameBuffer[left + GFX_LINESIZE * top];
    uint8 *lineBuffer   = &gfxLineBuffer[top];
    int32 startX        = left - XPos;
    int32 startY        = top - YPos;
    int32 shiftPivot    = (sprX << 9) - 1;
    int32 shiftheight   = (sprY << 9) - 1;
    fullwidth <<= 9;
    fullheight <<= 9;
    uint8 *pixels = &graphicData[surface->dataPosition];
    if (cosine < 0 || sine < 0)
        sprYPos += sine + cosine;

    if (direction == FLIP_X) {
        int32 drawX = sprXPos - (cosine * startX - sine * startY) - 0x100;
        int32 drawY = cosine * startY + sprYPos + sine * startX;
        while (maxY--) {
            activePalette = fullPalette[*lineBuffer];
            lineBuffer++;

            int32 finalX = drawX;
            int32 finalY = drawY;
            int32 w      = maxX;
            while (w--) {
                if (finalX > shiftPivot && finalX < fullwidth && finalY > shiftheight && finalY < fullheight) {
                    uint8 index = pixels[(finalY >> 9 << lineSize) + (finalX >> 9)];
                    if (index > 0)
                        *frameBuffer = activePalette[index];
                }
                ++frameBuffer;
                finalX -= cosine;
                finalY += sine;
            }

            drawX += sine;
            drawY += cosine;
            frameBuffer += pitch;
        }
    }
    else {
        int32 drawX = sprXPos + cosine * startX - sine * startY;
        int32 drawY = cosine * startY + sprYPos + sine * startX;
        while (maxY--) {
            activePalette = fullPalette[*lineBuffer];
            lineBuffer++;

            int32 finalX = drawX;
            int32 finalY = drawY;
            int32 w      = maxX;
            while (w--) {
                if (finalX > shiftPivot && finalX < fullwidth && finalY > shiftheight && finalY < fullheight) {
                    uint8 index = pixels[(finalY >> 9 << lineSize) + (finalX >> 9)];
                    if (index > 0)
                        *frameBuffer = activePalette[index];
                }
                ++frameBuffer;
                finalX += cosine;
                finalY += sine;
            }

            drawX -= sine;
            drawY += cosine;
            frameBuffer += pitch;
        }
    }
}

void RSDK::Legacy::DrawSpriteRotozoom(int32 direction, int32 XPos, int32 YPos, int32 pivotX, int32 pivotY, int32 sprX, int32 sprY, int32 width,
                                      int32 height, int32 rotation, int32 scale, int32 sheetID)
{
    if (scale == 0)
        return;

    int32 sprXPos    = (pivotX + sprX) << 9;
    int32 sprYPos    = (pivotY + sprY) << 9;
    int32 fullwidth  = width + sprX;
    int32 fullheight = height + sprY;
    int32 angle      = rotation & 0x1FF;
    if (angle < 0)
        angle += 0x200;
    if (angle)
        angle = 0x200 - angle;
    int32 sine   = scale * sin512LookupTable[angle] >> 9;
    int32 cosine = scale * cos512LookupTable[angle] >> 9;
    int32 xPositions[4];
    int32 yPositions[4];

    if (direction == FLIP_X) {
        xPositions[0] = XPos + ((sine * (-pivotY - 2) + cosine * (pivotX + 2)) >> 9);
        yPositions[0] = YPos + ((cosine * (-pivotY - 2) - sine * (pivotX + 2)) >> 9);
        xPositions[1] = XPos + ((sine * (-pivotY - 2) + cosine * (pivotX - width - 2)) >> 9);
        yPositions[1] = YPos + ((cosine * (-pivotY - 2) - sine * (pivotX - width - 2)) >> 9);
        xPositions[2] = XPos + ((sine * (height - pivotY + 2) + cosine * (pivotX + 2)) >> 9);
        yPositions[2] = YPos + ((cosine * (height - pivotY + 2) - sine * (pivotX + 2)) >> 9);
        int32 a       = pivotX - width - 2;
        int32 b       = height - pivotY + 2;
        xPositions[3] = XPos + ((sine * b + cosine * a) >> 9);
        yPositions[3] = YPos + ((cosine * b - sine * a) >> 9);
    }
    else {
        xPositions[0] = XPos + ((sine * (-pivotY - 2) + cosine * (-pivotX - 2)) >> 9);
        yPositions[0] = YPos + ((cosine * (-pivotY - 2) - sine * (-pivotX - 2)) >> 9);
        xPositions[1] = XPos + ((sine * (-pivotY - 2) + cosine * (width - pivotX + 2)) >> 9);
        yPositions[1] = YPos + ((cosine * (-pivotY - 2) - sine * (width - pivotX + 2)) >> 9);
        xPositions[2] = XPos + ((sine * (height - pivotY + 2) + cosine * (-pivotX - 2)) >> 9);
        yPositions[2] = YPos + ((cosine * (height - pivotY + 2) - sine * (-pivotX - 2)) >> 9);
        int32 a       = width - pivotX + 2;
        int32 b       = height - pivotY + 2;
        xPositions[3] = XPos + ((sine * b + cosine * a) >> 9);
        yPositions[3] = YPos + ((cosine * b - sine * a) >> 9);
    }
    int32 truescale = (int32)(float)((float)(512.0 / (float)scale) * 512.0);
    sine            = truescale * sin512LookupTable[angle] >> 9;
    cosine          = truescale * cos512LookupTable[angle] >> 9;

    int32 left = GFX_LINESIZE;
    for (int32 i = 0; i < 4; ++i) {
        if (xPositions[i] < left)
            left = xPositions[i];
    }
    if (left < 0)
        left = 0;

    int32 right = 0;
    for (int32 i = 0; i < 4; ++i) {
        if (xPositions[i] > right)
            right = xPositions[i];
    }
    if (right > GFX_LINESIZE)
        right = GFX_LINESIZE;
    int32 maxX = right - left;

    int32 top = SCREEN_YSIZE;
    for (int32 i = 0; i < 4; ++i) {
        if (yPositions[i] < top)
            top = yPositions[i];
    }
    if (top < 0)
        top = 0;

    int32 bottom = 0;
    for (int32 i = 0; i < 4; ++i) {
        if (yPositions[i] > bottom)
            bottom = yPositions[i];
    }
    if (bottom > SCREEN_YSIZE)
        bottom = SCREEN_YSIZE;
    int32 maxY = bottom - top;

    if (maxX <= 0 || maxY <= 0)
        return;

    GFXSurface *surface = &gfxSurface[sheetID];
    int32 pitch         = GFX_LINESIZE - maxX;
    int32 lineSize      = surface->widthShift;
    uint16 *frameBuffer = &currentScreen->frameBuffer[left + GFX_LINESIZE * top];
    uint8 *lineBuffer   = &gfxLineBuffer[top];
    int32 startX        = left - XPos;
    int32 startY        = top - YPos;
    int32 shiftPivot    = (sprX << 9) - 1;
    int32 shiftheight   = (sprY << 9) - 1;
    fullwidth <<= 9;
    fullheight <<= 9;
    uint8 *pixels = &graphicData[surface->dataPosition];
    if (cosine < 0 || sine < 0)
        sprYPos += sine + cosine;

    if (direction == FLIP_X) {
        int32 drawX = sprXPos - (cosine * startX - sine * startY) - (truescale >> 1);
        int32 drawY = cosine * startY + sprYPos + sine * startX;
        while (maxY--) {
            activePalette = fullPalette[*lineBuffer];
            lineBuffer++;

            int32 finalX = drawX;
            int32 finalY = drawY;
            int32 w      = maxX;
            while (w--) {
                if (finalX > shiftPivot && finalX < fullwidth && finalY > shiftheight && finalY < fullheight) {
                    uint8 index = pixels[(finalY >> 9 << lineSize) + (finalX >> 9)];
                    if (index > 0)
                        *frameBuffer = activePalette[index];
                }

                ++frameBuffer;
                finalX -= cosine;
                finalY += sine;
            }

            drawX += sine;
            drawY += cosine;
            frameBuffer += pitch;
        }
    }
    else {
        int32 drawX = sprXPos + cosine * startX - sine * startY;
        int32 drawY = cosine * startY + sprYPos + sine * startX;
        while (maxY--) {
            activePalette = fullPalette[*lineBuffer];
            lineBuffer++;

            int32 finalX = drawX;
            int32 finalY = drawY;
            int32 w      = maxX;
            while (w--) {
                if (finalX > shiftPivot && finalX < fullwidth && finalY > shiftheight && finalY < fullheight) {
                    uint8 index = pixels[(finalY >> 9 << lineSize) + (finalX >> 9)];
                    if (index > 0)
                        *frameBuffer = activePalette[index];
                }

                ++frameBuffer;
                finalX += cosine;
                finalY += sine;
            }

            drawX -= sine;
            drawY += cosine;
            frameBuffer += pitch;
        }
    }
}

void RSDK::Legacy::DrawBlendedSprite(int32 XPos, int32 YPos, int32 width, int32 height, int32 sprX, int32 sprY, int32 sheetID)
{
    if (width + XPos > GFX_LINESIZE)
        width = GFX_LINESIZE - XPos;
    if (XPos < 0) {
        sprX -= XPos;
        width += XPos;
        XPos = 0;
    }
    if (height + YPos > SCREEN_YSIZE)
        height = SCREEN_YSIZE - YPos;
    if (YPos < 0) {
        sprY -= YPos;
        height += YPos;
        YPos = 0;
    }
    if (width <= 0 || height <= 0)
        return;

    GFXSurface *surface = &gfxSurface[sheetID];
    int32 pitch         = GFX_LINESIZE - width;
    int32 gfxPitch      = surface->width - width;
    uint8 *lineBuffer   = &gfxLineBuffer[YPos];
    uint8 *pixels       = &graphicData[sprX + surface->width * sprY + surface->dataPosition];
    uint16 *frameBuffer = &currentScreen->frameBuffer[XPos + GFX_LINESIZE * YPos];

    while (height--) {
        activePalette = fullPalette[*lineBuffer];
        lineBuffer++;

        int32 w = width;
        while (w--) {
            if (*pixels > 0)
                *frameBuffer = ((activePalette[*pixels] & 0xF7DE) >> 1) + ((*frameBuffer & 0xF7DE) >> 1);
            ++pixels;
            ++frameBuffer;
        }

        frameBuffer += pitch;
        pixels += gfxPitch;
    }
}
void RSDK::Legacy::DrawAlphaBlendedSprite(int32 XPos, int32 YPos, int32 width, int32 height, int32 sprX, int32 sprY, int32 alpha, int32 sheetID)
{
    if (alpha > 0xFF)
        alpha = 0xFF;

    if (width + XPos > GFX_LINESIZE)
        width = GFX_LINESIZE - XPos;
    if (XPos < 0) {
        sprX -= XPos;
        width += XPos;
        XPos = 0;
    }
    if (height + YPos > SCREEN_YSIZE)
        height = SCREEN_YSIZE - YPos;
    if (YPos < 0) {
        sprY -= YPos;
        height += YPos;
        YPos = 0;
    }
    if (width <= 0 || height <= 0 || alpha <= 0)
        return;

    GFXSurface *surface = &gfxSurface[sheetID];
    int32 pitch         = GFX_LINESIZE - width;
    int32 gfxPitch      = surface->width - width;
    uint8 *lineBuffer   = &gfxLineBuffer[YPos];
    uint8 *pixels       = &graphicData[sprX + surface->width * sprY + surface->dataPosition];
    uint16 *frameBuffer = &currentScreen->frameBuffer[XPos + GFX_LINESIZE * YPos];
    if (alpha == 0xFF) {
        while (height--) {
            activePalette = fullPalette[*lineBuffer];
            lineBuffer++;

            int32 w = width;
            while (w--) {
                if (*pixels > 0)
                    *frameBuffer = activePalette[*pixels];
                ++pixels;
                ++frameBuffer;
            }

            frameBuffer += pitch;
            pixels += gfxPitch;
        }
    }
    else {
        uint16 *fbufferBlend = &blendLookupTable[0x20 * (0xFF - alpha)];
        uint16 *pixelBlend   = &blendLookupTable[0x20 * alpha];

        while (height--) {
            activePalette = fullPalette[*lineBuffer];
            lineBuffer++;

            int32 w = width;
            while (w--) {
                if (*pixels > 0) {
                    uint16 color = activePalette[*pixels];

                    int32 R = (fbufferBlend[(*frameBuffer & 0xF800) >> 11] + pixelBlend[(color & 0xF800) >> 11]) << 11;
                    int32 G = (fbufferBlend[(*frameBuffer & 0x7E0) >> 6] + pixelBlend[(color & 0x7E0) >> 6]) << 6;
                    int32 B = fbufferBlend[*frameBuffer & 0x1F] + pixelBlend[color & 0x1F];

                    *frameBuffer = R | G | B;
                }
                ++pixels;
                ++frameBuffer;
            }

            frameBuffer += pitch;
            pixels += gfxPitch;
        }
    }
}
void RSDK::Legacy::DrawAdditiveBlendedSprite(int32 XPos, int32 YPos, int32 width, int32 height, int32 sprX, int32 sprY, int32 alpha, int32 sheetID)
{
    if (alpha > 0xFF)
        alpha = 0xFF;

    if (width + XPos > GFX_LINESIZE)
        width = GFX_LINESIZE - XPos;

    if (XPos < 0) {
        sprX -= XPos;
        width += XPos;
        XPos = 0;
    }

    if (height + YPos > SCREEN_YSIZE)
        height = SCREEN_YSIZE - YPos;

    if (YPos < 0) {
        sprY -= YPos;
        height += YPos;
        YPos = 0;
    }

    if (width <= 0 || height <= 0 || alpha <= 0)
        return;

    uint16 *blendTablePtr = &blendLookupTable[0x20 * alpha];
    GFXSurface *surface   = &gfxSurface[sheetID];
    int32 pitch           = GFX_LINESIZE - width;
    int32 gfxPitch        = surface->width - width;
    uint8 *lineBuffer     = &gfxLineBuffer[YPos];
    uint8 *pixels         = &graphicData[sprX + surface->width * sprY + surface->dataPosition];
    uint16 *frameBuffer   = &currentScreen->frameBuffer[XPos + GFX_LINESIZE * YPos];

    while (height--) {
        activePalette = fullPalette[*lineBuffer];
        lineBuffer++;

        int32 w = width;
        while (w--) {
            if (*pixels > 0) {
                uint16 color = activePalette[*pixels];

                int32 R = MIN((blendTablePtr[(color & 0xF800) >> 11] << 11) + (*frameBuffer & 0xF800), 0xF800);
                int32 G = MIN((blendTablePtr[(color & 0x7E0) >> 6] << 6) + (*frameBuffer & 0x7E0), 0x7E0);
                int32 B = MIN(blendTablePtr[color & 0x1F] + (*frameBuffer & 0x1F), 0x1F);

                *frameBuffer = R | G | B;
            }

            ++pixels;
            ++frameBuffer;
        }

        frameBuffer += pitch;
        pixels += gfxPitch;
    }
}
void RSDK::Legacy::DrawSubtractiveBlendedSprite(int32 XPos, int32 YPos, int32 width, int32 height, int32 sprX, int32 sprY, int32 alpha, int32 sheetID)
{
    if (alpha > 0xFF)
        alpha = 0xFF;

    if (width + XPos > GFX_LINESIZE)
        width = GFX_LINESIZE - XPos;
    if (XPos < 0) {
        sprX -= XPos;
        width += XPos;
        XPos = 0;
    }
    if (height + YPos > SCREEN_YSIZE)
        height = SCREEN_YSIZE - YPos;
    if (YPos < 0) {
        sprY -= YPos;
        height += YPos;
        YPos = 0;
    }
    if (width <= 0 || height <= 0 || alpha <= 0)
        return;

    uint16 *subBlendTable = &subtractLookupTable[0x20 * alpha];
    GFXSurface *surface   = &gfxSurface[sheetID];
    int32 pitch           = GFX_LINESIZE - width;
    int32 gfxPitch        = surface->width - width;
    uint8 *lineBuffer     = &gfxLineBuffer[YPos];
    uint8 *pixels         = &graphicData[sprX + surface->width * sprY + surface->dataPosition];
    uint16 *frameBuffer   = &currentScreen->frameBuffer[XPos + GFX_LINESIZE * YPos];

    while (height--) {
        activePalette = fullPalette[*lineBuffer];
        lineBuffer++;

        int32 w = width;
        while (w--) {
            if (*pixels > 0) {
                uint16 color = activePalette[*pixels];

                int32 R = MAX((*frameBuffer & 0xF800) - (subBlendTable[(color & 0xF800) >> 11] << 11), 0);
                int32 G = MAX((*frameBuffer & 0x7E0) - (subBlendTable[(color & 0x7E0) >> 6] << 6), 0);
                int32 B = MAX((*frameBuffer & 0x1F) - subBlendTable[color & 0x1F], 0);

                *frameBuffer = R | G | B;
            }
            ++pixels;
            ++frameBuffer;
        }
        frameBuffer += pitch;
        pixels += gfxPitch;
    }
}

void RSDK::Legacy::DrawFace(void *v, uint32 color)
{
    Vertex *verts = (Vertex *)v;
    int32 alpha   = (color & 0x7F000000) >> 23;
    if (alpha < 1)
        return;

    if (alpha > 0xFF)
        alpha = 0xFF;

    if (verts[0].x < 0 && verts[1].x < 0 && verts[2].x < 0 && verts[3].x < 0)
        return;

    if (verts[0].x > GFX_LINESIZE && verts[1].x > GFX_LINESIZE && verts[2].x > GFX_LINESIZE && verts[3].x > GFX_LINESIZE)
        return;

    if (verts[0].y < 0 && verts[1].y < 0 && verts[2].y < 0 && verts[3].y < 0)
        return;

    if (verts[0].y > SCREEN_YSIZE && verts[1].y > SCREEN_YSIZE && verts[2].y > SCREEN_YSIZE && verts[3].y > SCREEN_YSIZE)
        return;

    if (verts[0].x == verts[1].x && verts[1].x == verts[2].x && verts[2].x == verts[3].x)
        return;

    if (verts[0].y == verts[1].y && verts[1].y == verts[2].y && verts[2].y == verts[3].y)
        return;

    int32 vertexA = 0;
    int32 vertexB = 1;
    int32 vertexC = 2;
    int32 vertexD = 3;
    if (verts[1].y < verts[0].y) {
        vertexA = 1;
        vertexB = 0;
    }
    if (verts[2].y < verts[vertexA].y) {
        int32 temp = vertexA;
        vertexA    = 2;
        vertexC    = temp;
    }
    if (verts[3].y < verts[vertexA].y) {
        int32 temp = vertexA;
        vertexA    = 3;
        vertexD    = temp;
    }
    if (verts[vertexC].y < verts[vertexB].y) {
        int32 temp = vertexB;
        vertexB    = vertexC;
        vertexC    = temp;
    }
    if (verts[vertexD].y < verts[vertexB].y) {
        int32 temp = vertexB;
        vertexB    = vertexD;
        vertexD    = temp;
    }
    if (verts[vertexD].y < verts[vertexC].y) {
        int32 temp = vertexC;
        vertexC    = vertexD;
        vertexD    = temp;
    }

    int32 faceTop    = verts[vertexA].y;
    int32 faceBottom = verts[vertexD].y;
    if (faceTop < 0)
        faceTop = 0;
    if (faceBottom > SCREEN_YSIZE)
        faceBottom = SCREEN_YSIZE;
    for (int32 i = faceTop; i < faceBottom; ++i) {
        faceLineStart[i] = 100000;
        faceLineEnd[i]   = -100000;
    }

    ProcessScanEdge(&verts[vertexA], &verts[vertexB]);
    ProcessScanEdge(&verts[vertexA], &verts[vertexC]);
    ProcessScanEdge(&verts[vertexA], &verts[vertexD]);
    ProcessScanEdge(&verts[vertexB], &verts[vertexC]);
    ProcessScanEdge(&verts[vertexC], &verts[vertexD]);
    ProcessScanEdge(&verts[vertexB], &verts[vertexD]);

    uint16 color16      = PACK_RGB888(((color >> 16) & 0xFF), ((color >> 8) & 0xFF), ((color >> 0) & 0xFF));
    uint16 *frameBuffer = &currentScreen->frameBuffer[GFX_LINESIZE * faceTop];

    if (alpha == 0xFF) {
        while (faceTop < faceBottom) {
            int32 startX = faceLineStart[faceTop];
            int32 endX   = faceLineEnd[faceTop];
            if (startX >= GFX_LINESIZE || endX <= 0) {
                frameBuffer += GFX_LINESIZE;
            }
            else {
                if (startX < 0)
                    startX = 0;
                if (endX > GFX_LINESIZE_MINUSONE)
                    endX = GFX_LINESIZE_MINUSONE;
                uint16 *frameBufferPtr = &frameBuffer[startX];
                frameBuffer += GFX_LINESIZE;
                int32 vertexwidth = endX - startX + 1;
                while (vertexwidth--) {
                    *frameBufferPtr = color16;
                    ++frameBufferPtr;
                }
            }
            ++faceTop;
        }
    }
    else {
        uint16 *fbufferBlend = &blendLookupTable[0x20 * (0xFF - alpha)];
        uint16 *pixelBlend   = &blendLookupTable[0x20 * alpha];

        while (faceTop < faceBottom) {
            int32 startX = faceLineStart[faceTop];
            int32 endX   = faceLineEnd[faceTop];
            if (startX >= GFX_LINESIZE || endX <= 0) {
                frameBuffer += GFX_LINESIZE;
            }
            else {
                if (startX < 0)
                    startX = 0;
                if (endX > GFX_LINESIZE_MINUSONE)
                    endX = GFX_LINESIZE_MINUSONE;
                uint16 *framebufferPtr = &frameBuffer[startX];
                frameBuffer += GFX_LINESIZE;
                int32 vertexwidth = endX - startX + 1;
                while (vertexwidth--) {
                    int32 R = (fbufferBlend[(*framebufferPtr & 0xF800) >> 11] + pixelBlend[(color16 & 0xF800) >> 11]) << 11;
                    int32 G = (fbufferBlend[(*framebufferPtr & 0x7E0) >> 6] + pixelBlend[(color16 & 0x7E0) >> 6]) << 6;
                    int32 B = fbufferBlend[*framebufferPtr & 0x1F] + pixelBlend[color16 & 0x1F];

                    *framebufferPtr = R | G | B;
                    ++framebufferPtr;
                }
            }
            ++faceTop;
        }
    }
}
void RSDK::Legacy::DrawTexturedFace(void *v, uint8 sheetID)
{
    Vertex *verts = (Vertex *)v;

    if (verts[0].x < 0 && verts[1].x < 0 && verts[2].x < 0 && verts[3].x < 0)
        return;
    if (verts[0].x > GFX_LINESIZE && verts[1].x > GFX_LINESIZE && verts[2].x > GFX_LINESIZE && verts[3].x > GFX_LINESIZE)
        return;
    if (verts[0].y < 0 && verts[1].y < 0 && verts[2].y < 0 && verts[3].y < 0)
        return;
    if (verts[0].y > SCREEN_YSIZE && verts[1].y > SCREEN_YSIZE && verts[2].y > SCREEN_YSIZE && verts[3].y > SCREEN_YSIZE)
        return;
    if (verts[0].x == verts[1].x && verts[1].x == verts[2].x && verts[2].x == verts[3].x)
        return;
    if (verts[0].y == verts[1].y && verts[1].y == verts[2].y && verts[2].y == verts[3].y)
        return;

    int32 vertexA = 0;
    int32 vertexB = 1;
    int32 vertexC = 2;
    int32 vertexD = 3;
    if (verts[1].y < verts[0].y) {
        vertexA = 1;
        vertexB = 0;
    }
    if (verts[2].y < verts[vertexA].y) {
        int32 temp = vertexA;
        vertexA    = 2;
        vertexC    = temp;
    }
    if (verts[3].y < verts[vertexA].y) {
        int32 temp = vertexA;
        vertexA    = 3;
        vertexD    = temp;
    }
    if (verts[vertexC].y < verts[vertexB].y) {
        int32 temp = vertexB;
        vertexB    = vertexC;
        vertexC    = temp;
    }
    if (verts[vertexD].y < verts[vertexB].y) {
        int32 temp = vertexB;
        vertexB    = vertexD;
        vertexD    = temp;
    }
    if (verts[vertexD].y < verts[vertexC].y) {
        int32 temp = vertexC;
        vertexC    = vertexD;
        vertexD    = temp;
    }

    int32 faceTop    = verts[vertexA].y;
    int32 faceBottom = verts[vertexD].y;
    if (faceTop < 0)
        faceTop = 0;
    if (faceBottom > SCREEN_YSIZE)
        faceBottom = SCREEN_YSIZE;
    for (int32 i = faceTop; i < faceBottom; ++i) {
        faceLineStart[i] = 100000;
        faceLineEnd[i]   = -100000;
    }

    ProcessScanEdgeUV(&verts[vertexA], &verts[vertexB]);
    ProcessScanEdgeUV(&verts[vertexA], &verts[vertexC]);
    ProcessScanEdgeUV(&verts[vertexA], &verts[vertexD]);
    ProcessScanEdgeUV(&verts[vertexB], &verts[vertexC]);
    ProcessScanEdgeUV(&verts[vertexC], &verts[vertexD]);
    ProcessScanEdgeUV(&verts[vertexB], &verts[vertexD]);

    uint16 *frameBuffer = &currentScreen->frameBuffer[GFX_LINESIZE * faceTop];
    uint8 *pixels       = &graphicData[gfxSurface[sheetID].dataPosition];
    int32 shiftwidth    = gfxSurface[sheetID].widthShift;
    uint8 *lineBuffer   = &gfxLineBuffer[faceTop];

    while (faceTop < faceBottom) {
        activePalette = fullPalette[*lineBuffer];
        lineBuffer++;

        int32 startX = faceLineStart[faceTop];
        int32 endX   = faceLineEnd[faceTop];
        int32 UPos   = faceLineStartU[faceTop];
        int32 VPos   = faceLineStartV[faceTop];
        if (startX >= GFX_LINESIZE || endX <= 0) {
            frameBuffer += GFX_LINESIZE;
        }
        else {
            int32 posDifference = endX - startX;
            int32 bufferedUPos  = 0;
            int32 bufferedVPos  = 0;
            if (endX == startX) {
                bufferedUPos = 0;
                bufferedVPos = 0;
            }
            else {
                bufferedUPos = (faceLineEndU[faceTop] - UPos) / posDifference;
                bufferedVPos = (faceLineEndV[faceTop] - VPos) / posDifference;
            }
            if (endX > GFX_LINESIZE_MINUSONE)
                posDifference = GFX_LINESIZE_MINUSONE - startX;
            if (startX < 0) {
                posDifference += startX;
                UPos -= startX * bufferedUPos;
                VPos -= startX * bufferedVPos;
                startX = 0;
            }

            uint16 *framebufferPtr = &frameBuffer[startX];
            frameBuffer += GFX_LINESIZE;

            int32 counter = posDifference + 1;
            while (counter--) {
                if (UPos < 0)
                    UPos = 0;
                if (VPos < 0)
                    VPos = 0;
                uint16 index = pixels[(VPos >> 16 << shiftwidth) + (UPos >> 16)];
                if (index > 0)
                    *framebufferPtr = activePalette[index];
                framebufferPtr++;
                UPos += bufferedUPos;
                VPos += bufferedVPos;
            }
        }
        ++faceTop;
    }
}

void RSDK::Legacy::v4::DrawObjectAnimation(void *objScr, void *ent, int32 XPos, int32 YPos)
{
    ObjectScript *objectScript = (ObjectScript *)objScr;
    Entity *entity             = (Entity *)ent;
    SpriteAnimation *sprAnim   = &animationList[objectScript->animFile->aniListOffset + entity->animation];
    SpriteFrame *frame         = &animFrames[sprAnim->frameListOffset + entity->frame];
    int32 rotation             = 0;

    switch (sprAnim->rotationStyle) {
        case ROTSTYLE_NONE:
            switch (entity->direction) {
                case FLIP_NONE:
                    DrawSpriteFlipped(frame->pivotX + XPos, frame->pivotY + YPos, frame->width, frame->height, frame->sprX, frame->sprY, FLIP_NONE,
                                      frame->sheetID);
                    break;

                case FLIP_X:
                    DrawSpriteFlipped(XPos - frame->width - frame->pivotX, frame->pivotY + YPos, frame->width, frame->height, frame->sprX,
                                      frame->sprY, FLIP_X, frame->sheetID);
                    break;
                case FLIP_Y:

                    DrawSpriteFlipped(frame->pivotX + XPos, YPos - frame->height - frame->pivotY, frame->width, frame->height, frame->sprX,
                                      frame->sprY, FLIP_Y, frame->sheetID);
                    break;

                case FLIP_XY:
                    DrawSpriteFlipped(XPos - frame->width - frame->pivotX, YPos - frame->height - frame->pivotY, frame->width, frame->height,
                                      frame->sprX, frame->sprY, FLIP_XY, frame->sheetID);
                    break;

                default: break;
            }
            break;

        case ROTSTYLE_FULL:
            DrawSpriteRotated(entity->direction, XPos, YPos, -frame->pivotX, -frame->pivotY, frame->sprX, frame->sprY, frame->width, frame->height,
                              entity->rotation, frame->sheetID);
            break;

        case ROTSTYLE_45DEG:
            if (entity->rotation >= 0x100)
                DrawSpriteRotated(entity->direction, XPos, YPos, -frame->pivotX, -frame->pivotY, frame->sprX, frame->sprY, frame->width,
                                  frame->height, 0x200 - ((0x214 - entity->rotation) >> 6 << 6), frame->sheetID);
            else
                DrawSpriteRotated(entity->direction, XPos, YPos, -frame->pivotX, -frame->pivotY, frame->sprX, frame->sprY, frame->width,
                                  frame->height, (entity->rotation + 20) >> 6 << 6, frame->sheetID);
            break;

        case ROTSTYLE_STATICFRAMES: {
            if (entity->rotation >= 0x100)
                rotation = 8 - ((532 - entity->rotation) >> 6);
            else
                rotation = (entity->rotation + 20) >> 6;
            int32 frameID = entity->frame;
            switch (rotation) {
                case 0: // 0 deg
                case 8: // 360 deg
                    rotation = 0x00;
                    break;

                case 1: // 45 deg
                    frameID += sprAnim->frameCount;
                    if (entity->direction)
                        rotation = 0;
                    else
                        rotation = 0x80;
                    break;

                case 2: // 90 deg
                    rotation = 0x80;
                    break;

                case 3: // 135 deg
                    frameID += sprAnim->frameCount;
                    if (entity->direction)
                        rotation = 0x80;
                    else
                        rotation = 0x100;
                    break;

                case 4: // 180 deg
                    rotation = 0x100;
                    break;

                case 5: // 225 deg
                    frameID += sprAnim->frameCount;
                    if (entity->direction)
                        rotation = 0x100;
                    else
                        rotation = 384;
                    break;

                case 6: // 270 deg
                    rotation = 384;
                    break;

                case 7: // 315 deg
                    frameID += sprAnim->frameCount;
                    if (entity->direction)
                        rotation = 384;
                    else
                        rotation = 0;
                    break;

                default: break;
            }

            frame = &animFrames[sprAnim->frameListOffset + frameID];
            DrawSpriteRotated(entity->direction, XPos, YPos, -frame->pivotX, -frame->pivotY, frame->sprX, frame->sprY, frame->width, frame->height,
                              rotation, frame->sheetID);
            break;
        }

        default: break;
    }
}

void RSDK::Legacy::v4::DrawFadedFace(void *v, uint32 color, uint32 fogColor, int32 alpha)
{
    Vertex *verts = (Vertex *)v;
    if (alpha > 0xFF)
        alpha = 0xFF;

    if (alpha < 1)
        return;

    if (verts[0].x < 0 && verts[1].x < 0 && verts[2].x < 0 && verts[3].x < 0)
        return;

    if (verts[0].x > GFX_LINESIZE && verts[1].x > GFX_LINESIZE && verts[2].x > GFX_LINESIZE && verts[3].x > GFX_LINESIZE)
        return;

    if (verts[0].y < 0 && verts[1].y < 0 && verts[2].y < 0 && verts[3].y < 0)
        return;

    if (verts[0].y > SCREEN_YSIZE && verts[1].y > SCREEN_YSIZE && verts[2].y > SCREEN_YSIZE && verts[3].y > SCREEN_YSIZE)
        return;

    if (verts[0].x == verts[1].x && verts[1].x == verts[2].x && verts[2].x == verts[3].x)
        return;

    if (verts[0].y == verts[1].y && verts[1].y == verts[2].y && verts[2].y == verts[3].y)
        return;

    int32 vertexA = 0;
    int32 vertexB = 1;
    int32 vertexC = 2;
    int32 vertexD = 3;
    if (verts[1].y < verts[0].y) {
        vertexA = 1;
        vertexB = 0;
    }
    if (verts[2].y < verts[vertexA].y) {
        int32 temp = vertexA;
        vertexA    = 2;
        vertexC    = temp;
    }
    if (verts[3].y < verts[vertexA].y) {
        int32 temp = vertexA;
        vertexA    = 3;
        vertexD    = temp;
    }
    if (verts[vertexC].y < verts[vertexB].y) {
        int32 temp = vertexB;
        vertexB    = vertexC;
        vertexC    = temp;
    }
    if (verts[vertexD].y < verts[vertexB].y) {
        int32 temp = vertexB;
        vertexB    = vertexD;
        vertexD    = temp;
    }
    if (verts[vertexD].y < verts[vertexC].y) {
        int32 temp = vertexC;
        vertexC    = vertexD;
        vertexD    = temp;
    }

    int32 faceTop    = verts[vertexA].y;
    int32 faceBottom = verts[vertexD].y;
    if (faceTop < 0)
        faceTop = 0;
    if (faceBottom > SCREEN_YSIZE)
        faceBottom = SCREEN_YSIZE;
    for (int32 i = faceTop; i < faceBottom; ++i) {
        faceLineStart[i] = 100000;
        faceLineEnd[i]   = -100000;
    }

    ProcessScanEdge(&verts[vertexA], &verts[vertexB]);
    ProcessScanEdge(&verts[vertexA], &verts[vertexC]);
    ProcessScanEdge(&verts[vertexA], &verts[vertexD]);
    ProcessScanEdge(&verts[vertexB], &verts[vertexC]);
    ProcessScanEdge(&verts[vertexC], &verts[vertexD]);
    ProcessScanEdge(&verts[vertexB], &verts[vertexD]);

    uint16 color16    = PACK_RGB888(((color >> 16) & 0xFF), ((color >> 8) & 0xFF), ((color >> 0) & 0xFF));
    uint16 fogColor16 = PACK_RGB888(((fogColor >> 16) & 0xFF), ((fogColor >> 8) & 0xFF), ((fogColor >> 0) & 0xFF));

    uint16 *frameBuffer  = &currentScreen->frameBuffer[GFX_LINESIZE * faceTop];
    uint16 *fbufferBlend = &blendLookupTable[0x20 * (0xFF - alpha)];
    uint16 *pixelBlend   = &blendLookupTable[0x20 * alpha];

    while (faceTop < faceBottom) {
        int32 startX = faceLineStart[faceTop];
        int32 endX   = faceLineEnd[faceTop];
        if (startX >= GFX_LINESIZE || endX <= 0) {
            frameBuffer += GFX_LINESIZE;
        }
        else {
            if (startX < 0)
                startX = 0;
            if (endX > GFX_LINESIZE_MINUSONE)
                endX = GFX_LINESIZE_MINUSONE;

            uint16 *frameBufferPtr = &frameBuffer[startX];
            frameBuffer += GFX_LINESIZE;
            int32 vertexwidth = endX - startX + 1;
            while (vertexwidth--) {
                int32 R = (fbufferBlend[(fogColor16 & 0xF800) >> 11] + pixelBlend[(color16 & 0xF800) >> 11]) << 11;
                int32 G = (fbufferBlend[(fogColor16 & 0x7E0) >> 6] + pixelBlend[(color16 & 0x7E0) >> 6]) << 6;
                int32 B = fbufferBlend[fogColor16 & 0x1F] + pixelBlend[color16 & 0x1F];

                *frameBufferPtr = R | G | B;
                ++frameBufferPtr;
            }
        }

        ++faceTop;
    }
}
void RSDK::Legacy::v4::DrawTexturedFaceBlended(void *v, uint8 sheetID)
{
    Vertex *verts = (Vertex *)v;
    if (verts[0].x < 0 && verts[1].x < 0 && verts[2].x < 0 && verts[3].x < 0)
        return;

    if (verts[0].x > GFX_LINESIZE && verts[1].x > GFX_LINESIZE && verts[2].x > GFX_LINESIZE && verts[3].x > GFX_LINESIZE)
        return;

    if (verts[0].y < 0 && verts[1].y < 0 && verts[2].y < 0 && verts[3].y < 0)
        return;

    if (verts[0].y > SCREEN_YSIZE && verts[1].y > SCREEN_YSIZE && verts[2].y > SCREEN_YSIZE && verts[3].y > SCREEN_YSIZE)
        return;

    if (verts[0].x == verts[1].x && verts[1].x == verts[2].x && verts[2].x == verts[3].x)
        return;

    if (verts[0].y == verts[1].y && verts[1].y == verts[2].y && verts[2].y == verts[3].y)
        return;

    int32 vertexA = 0;
    int32 vertexB = 1;
    int32 vertexC = 2;
    int32 vertexD = 3;
    if (verts[1].y < verts[0].y) {
        vertexA = 1;
        vertexB = 0;
    }
    if (verts[2].y < verts[vertexA].y) {
        int32 temp = vertexA;
        vertexA    = 2;
        vertexC    = temp;
    }
    if (verts[3].y < verts[vertexA].y) {
        int32 temp = vertexA;
        vertexA    = 3;
        vertexD    = temp;
    }
    if (verts[vertexC].y < verts[vertexB].y) {
        int32 temp = vertexB;
        vertexB    = vertexC;
        vertexC    = temp;
    }
    if (verts[vertexD].y < verts[vertexB].y) {
        int32 temp = vertexB;
        vertexB    = vertexD;
        vertexD    = temp;
    }
    if (verts[vertexD].y < verts[vertexC].y) {
        int32 temp = vertexC;
        vertexC    = vertexD;
        vertexD    = temp;
    }

    int32 faceTop    = verts[vertexA].y;
    int32 faceBottom = verts[vertexD].y;
    if (faceTop < 0)
        faceTop = 0;
    if (faceBottom > SCREEN_YSIZE)
        faceBottom = SCREEN_YSIZE;
    for (int32 i = faceTop; i < faceBottom; ++i) {
        faceLineStart[i] = 100000;
        faceLineEnd[i]   = -100000;
    }

    ProcessScanEdgeUV(&verts[vertexA], &verts[vertexB]);
    ProcessScanEdgeUV(&verts[vertexA], &verts[vertexC]);
    ProcessScanEdgeUV(&verts[vertexA], &verts[vertexD]);
    ProcessScanEdgeUV(&verts[vertexB], &verts[vertexC]);
    ProcessScanEdgeUV(&verts[vertexC], &verts[vertexD]);
    ProcessScanEdgeUV(&verts[vertexB], &verts[vertexD]);

    uint16 *frameBuffer = &currentScreen->frameBuffer[GFX_LINESIZE * faceTop];
    uint8 *pixels       = &graphicData[gfxSurface[sheetID].dataPosition];
    int32 shiftwidth    = gfxSurface[sheetID].widthShift;
    uint8 *lineBuffer   = &gfxLineBuffer[faceTop];
    while (faceTop < faceBottom) {
        activePalette = fullPalette[*lineBuffer];
        lineBuffer++;

        int32 startX = faceLineStart[faceTop];
        int32 endX   = faceLineEnd[faceTop];
        int32 UPos   = faceLineStartU[faceTop];
        int32 VPos   = faceLineStartV[faceTop];

        if (startX >= GFX_LINESIZE || endX <= 0) {
            frameBuffer += GFX_LINESIZE;
        }
        else {
            int32 posDifference = endX - startX;
            int32 bufferedUPos  = 0;
            int32 bufferedVPos  = 0;
            if (endX == startX) {
                bufferedUPos = 0;
                bufferedVPos = 0;
            }
            else {
                bufferedUPos = (faceLineEndU[faceTop] - UPos) / posDifference;
                bufferedVPos = (faceLineEndV[faceTop] - VPos) / posDifference;
            }

            if (endX > GFX_LINESIZE_MINUSONE)
                posDifference = GFX_LINESIZE_MINUSONE - startX;

            if (startX < 0) {
                posDifference += startX;
                UPos -= startX * bufferedUPos;
                VPos -= startX * bufferedVPos;
                startX = 0;
            }

            uint16 *framebufferPtr = &frameBuffer[startX];
            frameBuffer += GFX_LINESIZE;
            int32 counter = posDifference + 1;
            while (counter--) {
                if (UPos < 0)
                    UPos = 0;
                if (VPos < 0)
                    VPos = 0;
                uint16 index = pixels[(VPos >> 16 << shiftwidth) + (UPos >> 16)];
                if (index > 0)
                    *framebufferPtr = ((activePalette[index] & 0xF7BC) >> 1) + ((*framebufferPtr & 0xF7BC) >> 1);
                framebufferPtr++;
                UPos += bufferedUPos;
                VPos += bufferedVPos;
            }
        }
        ++faceTop;
    }
}

void RSDK::Legacy::v3::DrawObjectAnimation(void *objScr, void *ent, int32 XPos, int32 YPos)
{
    ObjectScript *objectScript = (ObjectScript *)objScr;
    Entity *entity             = (Entity *)ent;
    SpriteAnimation *sprAnim   = &animationList[objectScript->animFile->aniListOffset + entity->animation];
    SpriteFrame *frame         = &animFrames[sprAnim->frameListOffset + entity->frame];
    int32 rotation             = 0;

    switch (sprAnim->rotationStyle) {
        case ROTSTYLE_NONE:
            switch (entity->direction) {
                case FLIP_NONE:
                    DrawSpriteFlipped(frame->pivotX + XPos, frame->pivotY + YPos, frame->width, frame->height, frame->sprX, frame->sprY, FLIP_NONE,
                                      frame->sheetID);
                    break;

                case FLIP_X:
                    DrawSpriteFlipped(XPos - frame->width - frame->pivotX, frame->pivotY + YPos, frame->width, frame->height, frame->sprX,
                                      frame->sprY, FLIP_X, frame->sheetID);
                    break;
                case FLIP_Y:

                    DrawSpriteFlipped(frame->pivotX + XPos, YPos - frame->height - frame->pivotY, frame->width, frame->height, frame->sprX,
                                      frame->sprY, FLIP_Y, frame->sheetID);
                    break;

                case FLIP_XY:
                    DrawSpriteFlipped(XPos - frame->width - frame->pivotX, YPos - frame->height - frame->pivotY, frame->width, frame->height,
                                      frame->sprX, frame->sprY, FLIP_XY, frame->sheetID);
                    break;

                default: break;
            }
            break;

        case ROTSTYLE_FULL:
            DrawSpriteRotated(entity->direction, XPos, YPos, -frame->pivotX, -frame->pivotY, frame->sprX, frame->sprY, frame->width, frame->height,
                              entity->rotation, frame->sheetID);
            break;

        case ROTSTYLE_45DEG:
            if (entity->rotation >= 0x100)
                DrawSpriteRotated(entity->direction, XPos, YPos, -frame->pivotX, -frame->pivotY, frame->sprX, frame->sprY, frame->width,
                                  frame->height, 0x200 - ((0x214 - entity->rotation) >> 6 << 6), frame->sheetID);
            else
                DrawSpriteRotated(entity->direction, XPos, YPos, -frame->pivotX, -frame->pivotY, frame->sprX, frame->sprY, frame->width,
                                  frame->height, (entity->rotation + 20) >> 6 << 6, frame->sheetID);
            break;

        case ROTSTYLE_STATICFRAMES: {
            if (entity->rotation >= 0x100)
                rotation = 8 - ((532 - entity->rotation) >> 6);
            else
                rotation = (entity->rotation + 20) >> 6;
            int32 frameID = entity->frame;
            switch (rotation) {
                case 0: // 0 deg
                case 8: // 360 deg
                    rotation = 0x00;
                    break;

                case 1: // 45 deg
                    frameID += sprAnim->frameCount;
                    if (entity->direction)
                        rotation = 0;
                    else
                        rotation = 0x80;
                    break;

                case 2: // 90 deg
                    rotation = 0x80;
                    break;

                case 3: // 135 deg
                    frameID += sprAnim->frameCount;
                    if (entity->direction)
                        rotation = 0x80;
                    else
                        rotation = 0x100;
                    break;

                case 4: // 180 deg
                    rotation = 0x100;
                    break;

                case 5: // 225 deg
                    frameID += sprAnim->frameCount;
                    if (entity->direction)
                        rotation = 0x100;
                    else
                        rotation = 384;
                    break;

                case 6: // 270 deg
                    rotation = 384;
                    break;

                case 7: // 315 deg
                    frameID += sprAnim->frameCount;
                    if (entity->direction)
                        rotation = 384;
                    else
                        rotation = 0;
                    break;

                default: break;
            }

            frame = &animFrames[sprAnim->frameListOffset + frameID];
            DrawSpriteRotated(entity->direction, XPos, YPos, -frame->pivotX, -frame->pivotY, frame->sprX, frame->sprY, frame->width, frame->height,
                              rotation, frame->sheetID);
            break;
        }

        default: break;
    }
}

void RSDK::Legacy::DrawTextMenuEntry(void *menu, int32 rowID, int32 XPos, int32 YPos, int32 textHighlight)
{
    TextMenu *tMenu = (TextMenu *)menu;
    int32 id        = tMenu->entryStart[rowID];
    for (int32 i = 0; i < tMenu->entrySize[rowID]; ++i) {
        DrawSprite(XPos + (i << 3) - (((tMenu->entrySize[rowID] % 2) & (tMenu->alignment == 2)) * 4), YPos, 8, 8, ((tMenu->textData[id] & 0xF) << 3),
                   ((tMenu->textData[id] >> 4) << 3) + textHighlight, textMenuSurfaceNo);
        id++;
    }
}
void RSDK::Legacy::DrawStageTextEntry(void *menu, int32 rowID, int32 XPos, int32 YPos, int32 textHighlight)
{
    TextMenu *tMenu = (TextMenu *)menu;
    int32 id        = tMenu->entryStart[rowID];
    for (int32 i = 0; i < tMenu->entrySize[rowID]; ++i) {
        if (i == tMenu->entrySize[rowID] - 1) {
            DrawSprite(XPos + (i << 3), YPos, 8, 8, ((tMenu->textData[id] & 0xF) << 3), ((tMenu->textData[id] >> 4) << 3), textMenuSurfaceNo);
        }
        else {
            DrawSprite(XPos + (i << 3), YPos, 8, 8, ((tMenu->textData[id] & 0xF) << 3), ((tMenu->textData[id] >> 4) << 3) + textHighlight,
                       textMenuSurfaceNo);
        }
        id++;
    }
}
void RSDK::Legacy::DrawBlendedTextMenuEntry(void *menu, int32 rowID, int32 XPos, int32 YPos, int32 textHighlight)
{
    TextMenu *tMenu = (TextMenu *)menu;
    int32 id        = tMenu->entryStart[rowID];
    for (int32 i = 0; i < tMenu->entrySize[rowID]; ++i) {
        DrawBlendedSprite(XPos + (i << 3), YPos, 8, 8, ((tMenu->textData[id] & 0xF) << 3), ((tMenu->textData[id] >> 4) << 3) + textHighlight,
                          textMenuSurfaceNo);
        id++;
    }
}
void RSDK::Legacy::DrawTextMenu(void *menu, int32 XPos, int32 YPos)
{
    TextMenu *tMenu = (TextMenu *)menu;
    int32 cnt       = 0;

    if (tMenu->visibleRowCount > 0) {
        cnt = (int32)(tMenu->visibleRowCount + tMenu->visibleRowOffset);
    }
    else {
        tMenu->visibleRowOffset = 0;
        cnt                     = (int32)tMenu->rowCount;
    }

    if (tMenu->selectionCount == 3) {
        tMenu->selection2 = -1;
        for (int32 i = 0; i < tMenu->selection1 + 1; ++i) {
            if (tMenu->entryHighlight[i]) {
                tMenu->selection2 = i;
            }
        }
    }

    switch (tMenu->alignment) {
        case 0:
            for (int32 i = (int32)tMenu->visibleRowOffset; i < cnt; ++i) {
                switch (tMenu->selectionCount) {
                    case 1:
                        if (i == tMenu->selection1)
                            DrawTextMenuEntry(tMenu, i, XPos, YPos, 128);
                        else
                            DrawTextMenuEntry(tMenu, i, XPos, YPos, 0);
                        break;

                    case 2:
                        if (i == tMenu->selection1 || i == tMenu->selection2)
                            DrawTextMenuEntry(tMenu, i, XPos, YPos, 128);
                        else
                            DrawTextMenuEntry(tMenu, i, XPos, YPos, 0);
                        break;

                    case 3:
                        if (i == tMenu->selection1)
                            DrawTextMenuEntry(tMenu, i, XPos, YPos, 128);
                        else
                            DrawTextMenuEntry(tMenu, i, XPos, YPos, 0);

                        if (i == tMenu->selection2 && i != tMenu->selection1)
                            DrawStageTextEntry(tMenu, i, XPos, YPos, 128);
                        break;
                }
                YPos += 8;
            }
            break;

        case 1:
            for (int32 i = (int32)tMenu->visibleRowOffset; i < cnt; ++i) {
                int32 entryX = XPos - (tMenu->entrySize[i] << 3);
                switch (tMenu->selectionCount) {
                    case 1:
                        if (i == tMenu->selection1)
                            DrawTextMenuEntry(tMenu, i, entryX, YPos, 128);
                        else
                            DrawTextMenuEntry(tMenu, i, entryX, YPos, 0);
                        break;

                    case 2:
                        if (i == tMenu->selection1 || i == tMenu->selection2)
                            DrawTextMenuEntry(tMenu, i, entryX, YPos, 128);
                        else
                            DrawTextMenuEntry(tMenu, i, entryX, YPos, 0);
                        break;

                    case 3:
                        if (i == tMenu->selection1)
                            DrawTextMenuEntry(tMenu, i, entryX, YPos, 128);
                        else
                            DrawTextMenuEntry(tMenu, i, entryX, YPos, 0);

                        if (i == tMenu->selection2 && i != tMenu->selection1)
                            DrawStageTextEntry(tMenu, i, entryX, YPos, 128);
                        break;
                }
                YPos += 8;
            }
            break;

        case 2:
            for (int32 i = (int32)tMenu->visibleRowOffset; i < cnt; ++i) {
                int32 entryX = XPos - (tMenu->entrySize[i] >> 1 << 3);
                switch (tMenu->selectionCount) {
                    case 1:
                        if (i == tMenu->selection1)
                            DrawTextMenuEntry(tMenu, i, entryX, YPos, 128);
                        else
                            DrawTextMenuEntry(tMenu, i, entryX, YPos, 0);
                        break;
                    case 2:
                        if (i == tMenu->selection1 || i == tMenu->selection2)
                            DrawTextMenuEntry(tMenu, i, entryX, YPos, 128);
                        else
                            DrawTextMenuEntry(tMenu, i, entryX, YPos, 0);
                        break;
                    case 3:
                        if (i == tMenu->selection1)
                            DrawTextMenuEntry(tMenu, i, entryX, YPos, 128);
                        else
                            DrawTextMenuEntry(tMenu, i, entryX, YPos, 0);

                        if (i == tMenu->selection2 && i != tMenu->selection1)
                            DrawStageTextEntry(tMenu, i, entryX, YPos, 128);
                        break;
                }
                YPos += 8;
            }
            break;

        default: break;
    }
}
