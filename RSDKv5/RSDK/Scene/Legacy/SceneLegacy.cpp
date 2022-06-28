
#include "v3/SceneLegacyv3.cpp"
#include "v4/SceneLegacyv4.cpp"

int32 RSDK::Legacy::stageMode = 0;

RSDK::Legacy::Camera RSDK::Legacy::cameras[2];
RSDK::Legacy::Camera *RSDK::Legacy::currentCamera = NULL;

int32 RSDK::Legacy::xScrollOffset  = 0;
int32 RSDK::Legacy::yScrollOffset  = 0;
int32 RSDK::Legacy::cameraShift    = 0;
int32 RSDK::Legacy::cameraLockedY  = 0;
int32 RSDK::Legacy::cameraShakeX   = 0;
int32 RSDK::Legacy::cameraShakeY   = 0;
int32 RSDK::Legacy::cameraLag      = 0;
int32 RSDK::Legacy::cameraLagStyle = 0;

int32 RSDK::Legacy::curXBoundary1 = 0;
int32 RSDK::Legacy::newXBoundary1 = 0;
int32 RSDK::Legacy::curYBoundary1 = 0;
int32 RSDK::Legacy::newYBoundary1 = 0;
int32 RSDK::Legacy::curXBoundary2 = 0;
int32 RSDK::Legacy::curYBoundary2 = 0;
int32 RSDK::Legacy::waterLevel    = 0;
int32 RSDK::Legacy::waterDrawPos  = 0;
int32 RSDK::Legacy::newXBoundary2 = 0;
int32 RSDK::Legacy::newYBoundary2 = 0;

int32 RSDK::Legacy::SCREEN_SCROLL_LEFT  = SCREEN_CENTERX - 8;
int32 RSDK::Legacy::SCREEN_SCROLL_RIGHT = SCREEN_CENTERX + 8;

int32 RSDK::Legacy::lastYSize = -1;
int32 RSDK::Legacy::lastXSize = -1;

bool32 RSDK::Legacy::pauseEnabled     = true;
bool32 RSDK::Legacy::timeEnabled      = true;
bool32 RSDK::Legacy::debugMode        = false;
int32 RSDK::Legacy::frameCounter      = 0;
int32 RSDK::Legacy::stageMilliseconds = 0;
int32 RSDK::Legacy::stageSeconds      = 0;
int32 RSDK::Legacy::stageMinutes      = 0;

// Category and Scene IDs
char RSDK::Legacy::currentStageFolder[0x100];
int32 RSDK::Legacy::actID = 0;

char RSDK::Legacy::titleCardText[0x100];
uint8 RSDK::Legacy::titleCardWord2 = 0;

uint8 RSDK::Legacy::activeTileLayers[4];
uint8 RSDK::Legacy::tLayerMidPoint;
RSDK::Legacy::TileLayer RSDK::Legacy::stageLayouts[LEGACY_LAYER_COUNT];

int32 RSDK::Legacy::bgDeformationData0[LEGACY_DEFORM_COUNT];
int32 RSDK::Legacy::bgDeformationData1[LEGACY_DEFORM_COUNT];
int32 RSDK::Legacy::bgDeformationData2[LEGACY_DEFORM_COUNT];
int32 RSDK::Legacy::bgDeformationData3[LEGACY_DEFORM_COUNT];

RSDK::Legacy::LineScroll RSDK::Legacy::hParallax;
RSDK::Legacy::LineScroll RSDK::Legacy::vParallax;

uint16 RSDK::Legacy::tile3DFloorBuffer[0x100 * 0x100];

RSDK::Legacy::Tiles128x128 RSDK::Legacy::tiles128x128;
RSDK::Legacy::CollisionMasks RSDK::Legacy::collisionMasks[2];

uint8 RSDK::Legacy::tilesetGFXData[LEGACY_TILESET_SIZE];

void RSDK::Legacy::GetStageFilepath(char* dest, const char *filePath)
{
    StrCopy(dest, "Data/Stages/");
    StrAdd(dest, sceneInfo.listData[sceneInfo.listPos].folder);
    StrAdd(dest, "/");
    StrAdd(dest, filePath);
}

int32 RSDK::Legacy::LoadStageFile(const char *filePath, FileInfo *info)
{
    char dest[0x40];
    GetStageFilepath(dest, filePath);
    return LoadFile(info, dest, FMODE_RB);
}

int32 RSDK::Legacy::LoadActFile(const char *ext, FileInfo *info)
{
    char dest[0x40];

    StrCopy(dest, "Data/Stages/");
    StrAdd(dest, sceneInfo.listData[sceneInfo.listPos].folder);
    StrAdd(dest, "/Act");
    StrAdd(dest, sceneInfo.listData[sceneInfo.listPos].id);
    StrAdd(dest, ext);

    ConvertStringToInteger(sceneInfo.listData[sceneInfo.listPos].id, &actID);

    return LoadFile(info, dest, FMODE_RB);
}

void RSDK::Legacy::LoadStageChunks()
{
    FileInfo info;
    InitFileInfo(&info);

    if (LoadStageFile("128x128Tiles.bin", &info)) {
        uint8 entry[3];
        for (int32 i = 0; i < LEGACY_CHUNKTILE_COUNT; ++i) {
            ReadBytes(&info, entry, 3);
            entry[0] -= (uint8)((entry[0] >> 6) << 6);

            tiles128x128.visualPlane[i] = (uint8)(entry[0] >> 4);
            entry[0] -= 16 * (entry[0] >> 4);

            tiles128x128.direction[i] = (uint8)(entry[0] >> 2);
            entry[0] -= 4 * (entry[0] >> 2);

            tiles128x128.tileIndex[i]  = entry[1] + (entry[0] << 8);
            tiles128x128.gfxDataPos[i] = tiles128x128.tileIndex[i] << 8;

            tiles128x128.collisionFlags[0][i] = entry[2] >> 4;
            tiles128x128.collisionFlags[1][i] = entry[2] - ((entry[2] >> 4) << 4);
        }
        CloseFile(&info);
    }
}

void RSDK::Legacy::LoadStageCollisions()
{
    FileInfo info;
    InitFileInfo(&info);

    if (LoadStageFile("CollisionMasks.bin", &info)) {
        int32 tileIndex = 0;

        for (int32 t = 0; t < TILE_COUNT; ++t) {
            for (int32 p = 0; p < CPATH_COUNT; ++p) {
                uint8 flags                = ReadInt8(&info);
                bool32 isCeiling           = flags >> 4;
                collisionMasks[p].flags[t] = flags & 0xF;

                collisionMasks[p].angles[t] = ReadInt8(&info);
                collisionMasks[p].angles[t] |= ReadInt8(&info) << 8;
                collisionMasks[p].angles[t] |= ReadInt8(&info) << 16;
                collisionMasks[p].angles[t] |= ReadInt8(&info) << 24;

                if (isCeiling) // Ceiling Tile
                {
                    for (int32 c = 0; c < TILE_SIZE; c += 2) {
                        uint8 buffer                                   = ReadInt8(&info);
                        collisionMasks[p].roofMasks[c + tileIndex]     = buffer >> 4;
                        collisionMasks[p].roofMasks[c + tileIndex + 1] = buffer & 0xF;
                    }

                    // Has Collision (Pt 1)
                    uint8 solid = ReadInt8(&info);
                    int32 id = 1;
                    for (int32 c = 0; c < TILE_SIZE / 2; ++c) {
                        if (solid & id) {
                            collisionMasks[p].floorMasks[c + tileIndex + 8] = 0;
                        }
                        else {
                            collisionMasks[p].floorMasks[c + tileIndex + 8] = 0x40;
                            collisionMasks[p].roofMasks[c + tileIndex + 8]  = -0x40;
                        }
                        id <<= 1;
                    }

                    // Has Collision (Pt 2)
                    solid = ReadInt8(&info);
                    id = 1;
                    for (int32 c = 0; c < TILE_SIZE / 2; ++c) {
                        if (solid & id) {
                            collisionMasks[p].floorMasks[c + tileIndex] = 0;
                        }
                        else {
                            collisionMasks[p].floorMasks[c + tileIndex] = 0x40;
                            collisionMasks[p].roofMasks[c + tileIndex]  = -0x40;
                        }
                        id <<= 1;
                    }

                    // LWall rotations
                    for (int32 c = 0; c < TILE_SIZE; ++c) {
                        int32 h = 0;
                        while (h > -1) {
                            if (h == TILE_SIZE) {
                                collisionMasks[p].lWallMasks[c + tileIndex] = 0x40;
                                h                                           = -1;
                            }
                            else if (c > collisionMasks[p].roofMasks[h + tileIndex]) {
                                ++h;
                            }
                            else {
                                collisionMasks[p].lWallMasks[c + tileIndex] = h;
                                h                                           = -1;
                            }
                        }
                    }

                    // RWall rotations
                    for (int32 c = 0; c < TILE_SIZE; ++c) {
                        int32 h = TILE_SIZE - 1;
                        while (h < TILE_SIZE) {
                            if (h == -1) {
                                collisionMasks[p].rWallMasks[c + tileIndex] = -0x40;
                                h                                           = TILE_SIZE;
                            }
                            else if (c > collisionMasks[p].roofMasks[h + tileIndex]) {
                                --h;
                            }
                            else {
                                collisionMasks[p].rWallMasks[c + tileIndex] = h;
                                h                                           = TILE_SIZE;
                            }
                        }
                    }
                }
                else // Regular Tile
                {
                    for (int32 c = 0; c < TILE_SIZE; c += 2) {
                        uint8 buffer                                    = ReadInt8(&info);
                        collisionMasks[p].floorMasks[c + tileIndex]     = buffer >> 4;
                        collisionMasks[p].floorMasks[c + tileIndex + 1] = buffer & 0xF;
                    }

                    uint8 solid = ReadInt8(&info);
                    int32 id = 1;
                    for (int32 c = 0; c < TILE_SIZE / 2; ++c) // HasCollision
                    {
                        if (solid & id) {
                            collisionMasks[p].roofMasks[c + tileIndex + 8] = 0xF;
                        }
                        else {
                            collisionMasks[p].floorMasks[c + tileIndex + 8] = 0x40;
                            collisionMasks[p].roofMasks[c + tileIndex + 8]  = -0x40;
                        }
                        id <<= 1;
                    }

                    solid = ReadInt8(&info);
                    id = 1;
                    for (int32 c = 0; c < TILE_SIZE / 2; ++c) // HasCollision (pt 2)
                    {
                        if (solid & id) {
                            collisionMasks[p].roofMasks[c + tileIndex] = 0xF;
                        }
                        else {
                            collisionMasks[p].floorMasks[c + tileIndex] = 0x40;
                            collisionMasks[p].roofMasks[c + tileIndex]  = -0x40;
                        }
                        id <<= 1;
                    }

                    // LWall rotations
                    for (int32 c = 0; c < TILE_SIZE; ++c) {
                        int32 h = 0;
                        while (h > -1) {
                            if (h == TILE_SIZE) {
                                collisionMasks[p].lWallMasks[c + tileIndex] = 0x40;
                                h                                           = -1;
                            }
                            else if (c < collisionMasks[p].floorMasks[h + tileIndex]) {
                                ++h;
                            }
                            else {
                                collisionMasks[p].lWallMasks[c + tileIndex] = h;
                                h                                           = -1;
                            }
                        }
                    }

                    // RWall rotations
                    for (int32 c = 0; c < TILE_SIZE; ++c) {
                        int32 h = TILE_SIZE - 1;
                        while (h < TILE_SIZE) {
                            if (h == -1) {
                                collisionMasks[p].rWallMasks[c + tileIndex] = -0x40;
                                h                                           = TILE_SIZE;
                            }
                            else if (c < collisionMasks[p].floorMasks[h + tileIndex]) {
                                --h;
                            }
                            else {
                                collisionMasks[p].rWallMasks[c + tileIndex] = h;
                                h                                           = TILE_SIZE;
                            }
                        }
                    }
                }
            }
            tileIndex += 16;
        }

        CloseFile(&info);
    }
}
void RSDK::Legacy::LoadStageGIFFile()
{
    ImageGIF tileset;

    char dest[0x40];
    GetStageFilepath(dest, "16x16Tiles.gif");

    if (tileset.Load(dest, true) && tileset.width == TILE_SIZE && tileset.height <= TILE_COUNT * TILE_SIZE) {
        tileset.pixels = tilesetGFXData;
        tileset.Load(NULL, false);

        for (int32 c = 128; c < 256; ++c) {
            uint8 red   = (tileset.palette[c] >> 0x10);
            uint8 green = (tileset.palette[c] >> 0x08);
            uint8 blue  = (tileset.palette[c] >> 0x00);
            SetPaletteEntry(-1, c, red, green, blue);
        }

        tileset.palette = NULL;
        tileset.decoder = NULL;
        tileset.pixels  = NULL;

        tileset.Close();
    }
}

void RSDK::Legacy::ProcessInput() {
    RSDK::ProcessInput();

    bool32 anyPress = false;
    for (int32 i = 0; i < InputDeviceCount; ++i) {
        if (InputDevices[i])
            anyPress |= InputDevices[i]->anyPress;
    }
    SetGlobalVariableByName("input.pressButton", anyPress);
}

void RSDK::Legacy::ProcessSceneTimer()
{
    if (timeEnabled) {
        if (++frameCounter == 60) {
            frameCounter = 0;
            if (++stageSeconds > 59) {
                stageSeconds = 0;
                if (++stageMinutes > 59)
                    stageMinutes = 0;
            }
        }
        stageMilliseconds = 100 * frameCounter / 60;
    }
    else {
        frameCounter = 60 * stageMilliseconds / 100;
    }
}

void RSDK::Legacy::ResetBackgroundSettings()
{
    for (int32 i = 0; i < LEGACY_LAYER_COUNT; ++i) {
        stageLayouts[i].deformationOffset  = 0;
        stageLayouts[i].deformationOffsetW = 0;
        stageLayouts[i].scrollPos          = 0;
    }

    for (int32 i = 0; i < LEGACY_PARALLAX_COUNT; ++i) {
        hParallax.scrollPos[i] = 0;
        vParallax.scrollPos[i] = 0;
    }

    for (int32 i = 0; i < LEGACY_DEFORM_COUNT; ++i) {
        bgDeformationData0[i] = 0;
        bgDeformationData1[i] = 0;
        bgDeformationData2[i] = 0;
        bgDeformationData3[i] = 0;
    }
}

void RSDK::Legacy::SetLayerDeformation(int32 selectedDef, int32 waveLength, int32 waveWidth, int32 waveType, int32 YPos, int32 waveSize)
{
    int32 *deformPtr = nullptr;
    switch (selectedDef) {
        case DEFORM_FG: deformPtr = bgDeformationData0; break;
        case DEFORM_FG_WATER: deformPtr = bgDeformationData1; break;
        case DEFORM_BG: deformPtr = bgDeformationData2; break;
        case DEFORM_BG_WATER: deformPtr = bgDeformationData3; break;
        default: break;
    }

    int32 shift = 9;

    int32 id = 0;
    if (waveType == 1) {
        id = YPos;
        for (int32 i = 0; i < waveSize; ++i) {
            deformPtr[id] = waveWidth * sin512LookupTable[(i << 9) / waveLength & 0x1FF] >> shift;
            ++id;
        }
    }
    else {
        for (int32 i = 0; i < 0x200 * 0x100; i += 0x200) {
            int32 sine    = waveWidth * sin512LookupTable[i / waveLength & 0x1FF] >> shift;
            deformPtr[id] = sine;
            if (deformPtr[id] >= waveWidth)
                deformPtr[id] = waveWidth - 1;
            ++id;
        }
    }

    switch (selectedDef) {
        case DEFORM_FG:
            for (int32 i = LEGACY_DEFORM_STORE; i < LEGACY_DEFORM_COUNT; ++i)
                bgDeformationData0[i] = bgDeformationData0[i - LEGACY_DEFORM_STORE];
            break;

        case DEFORM_FG_WATER:
            for (int32 i = LEGACY_DEFORM_STORE; i < LEGACY_DEFORM_COUNT; ++i)
                bgDeformationData1[i] = bgDeformationData1[i - LEGACY_DEFORM_STORE];
            break;

        case DEFORM_BG:
            for (int32 i = LEGACY_DEFORM_STORE; i < LEGACY_DEFORM_COUNT; ++i)
                bgDeformationData2[i] = bgDeformationData2[i - LEGACY_DEFORM_STORE];
            break;

        case DEFORM_BG_WATER:
            for (int32 i = LEGACY_DEFORM_STORE; i < LEGACY_DEFORM_COUNT; ++i)
                bgDeformationData3[i] = bgDeformationData3[i - LEGACY_DEFORM_STORE];
            break;

        default: break;
    }
}

void RSDK::Legacy::InitCameras()
{
    memset(cameras, 0, sizeof(cameras));
    cameras[0].enabled = true;
    cameras[1].enabled = true;
    cameras[0].target  = -1;
    cameras[1].target  = -1;

    currentCamera = &cameras[0];
}