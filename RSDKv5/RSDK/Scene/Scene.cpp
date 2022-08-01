#include "RSDK/Core/RetroEngine.hpp"

#if RETRO_REV0U
#include "Legacy/SceneLegacy.cpp"
#endif

using namespace RSDK;

uint8 RSDK::tilesetPixels[TILESET_SIZE * 4];

ScanlineInfo *RSDK::scanlines = NULL;
TileLayer RSDK::tileLayers[LAYER_COUNT];
CollisionMask RSDK::collisionMasks[CPATH_COUNT][TILE_COUNT * 4];
TileInfo RSDK::tileInfo[CPATH_COUNT][TILE_COUNT * 4];

#if RETRO_REV02
bool32 RSDK::forceHardReset = false;
#endif
char RSDK::currentSceneFolder[0x10];
char RSDK::currentSceneID[0x10];

SceneInfo RSDK::sceneInfo;

void RSDK::LoadSceneFolder()
{
#if RETRO_USE_MOD_LOADER
    // run this before the game actually unloads all the objects & scene assets
    RunModCallbacks(MODCB_STAGEUNLOAD, NULL);
#endif

    sceneInfo.timeCounter  = 0;
    sceneInfo.minutes      = 0;
    sceneInfo.seconds      = 0;
    sceneInfo.milliseconds = 0;

    // clear draw groups
    for (int32 i = 0; i < DRAWGROUP_COUNT; ++i) {
        drawGroups[i].entityCount = 0;
        drawGroups[i].layerCount  = 0;
    }

    // Clear Type groups
    for (int32 i = 0; i < TYPEGROUP_COUNT; ++i) {
        typeGroups[i].entryCount = 0;
    }

#if RETRO_REV02
    // Unload debug values
    ClearViewableVariables();

    // "unload" tint table
    tintLookupTable = NULL;
#endif

    // Unload TileLayers
    for (int32 l = 0; l < LAYER_COUNT; ++l) {
        MEM_ZERO(tileLayers[l]);
        for (int32 c = 0; c < CAMERA_COUNT; ++c) tileLayers[l].drawGroup[c] = -1;
    }

    SceneListInfo *list = &sceneInfo.listCategory[sceneInfo.activeCategory];
#if RETRO_REV02
    if (strcmp(currentSceneFolder, sceneInfo.listData[sceneInfo.listPos].folder) == 0 && !forceHardReset) {
        // Reload
        ClearUnusedStorage(DATASET_STG);
        sceneInfo.filter = sceneInfo.listData[sceneInfo.listPos].filter;
        PrintLog(PRINT_NORMAL, "Reloading Scene \"%s - %s\" with filter %d", list->name, sceneInfo.listData[sceneInfo.listPos].name,
                 sceneInfo.listData[sceneInfo.listPos].filter);
        return;
    }
#endif

#if !RETRO_REV02
    if (strcmp(currentSceneFolder, sceneInfo.listData[sceneInfo.listPos].folder) == 0) {
        // Reload
        ClearUnusedStorage(DATASET_STG);
        PrintLog(PRINT_NORMAL, "Reloading Scene \"%s - %s\"", list->name, sceneInfo.listData[sceneInfo.listPos].name);
        return;
    }
#endif

    // Unload stage 3DScenes & models
    Clear3DScenes();

    // Unload stage sprite animations
    ClearSpriteAnimations();

    // Unload stage surfaces
    ClearGfxSurfaces();

    // Unload stage sfx & audio channels
    ClearStageSfx();

    // Unload stage objects
    ClearStageObjects();

    // Clear draw groups
    for (int32 l = 0; l < DRAWGROUP_COUNT; ++l) {
        MEM_ZERO(drawGroups[l]);
        drawGroups[l].sorted = false;
    }

    // Clear stage storage
    ClearUnusedStorage(DATASET_STG);
    ClearUnusedStorage(DATASET_SFX);

    for (int32 s = 0; s < SCREEN_COUNT; ++s) {
        screens[s].position.x = 0;
        screens[s].position.y = 0;
    }

    SceneListEntry *sceneEntry = &sceneInfo.listData[sceneInfo.listPos];
    strcpy(currentSceneFolder, sceneEntry->folder);

#if RETRO_REV02
    forceHardReset   = false;
    sceneInfo.filter = sceneEntry->filter;
    PrintLog(PRINT_NORMAL, "Loading Scene \"%s - %s\" with filter %d", list->name, sceneEntry->name, sceneEntry->filter);
#endif

#if !RETRO_REV02
    PrintLog(PRINT_NORMAL, "Loading Scene \"%s - %s\"", list->name, sceneEntry->name);
#endif

    char fullFilePath[0x40];

    // Load TileConfig
    sprintf_s(fullFilePath, (int32)sizeof(fullFilePath), "Data/Stages/%s/TileConfig.bin", currentSceneFolder);
    LoadTileConfig(fullFilePath);

    // Load StageConfig
    sprintf_s(fullFilePath, (int32)sizeof(fullFilePath), "Data/Stages/%s/StageConfig.bin", currentSceneFolder);

    FileInfo info;
    InitFileInfo(&info);
    if (LoadFile(&info, fullFilePath, FMODE_RB)) {
        uint32 sig = ReadInt32(&info, false);

        if (sig != RSDK_SIGNATURE_CFG) {
            CloseFile(&info);
            return;
        }

        sceneInfo.useGlobalObjects = ReadInt8(&info);
        sceneInfo.classCount       = 0;

        if (sceneInfo.useGlobalObjects) {
            for (int32 o = 0; o < globalObjectCount; ++o) stageObjectIDs[o] = globalObjectIDs[o];
            sceneInfo.classCount = globalObjectCount;
        }
        else {
            for (int32 o = 0; o < TYPE_DEFAULT_COUNT; ++o) stageObjectIDs[o] = globalObjectIDs[o];

            sceneInfo.classCount = TYPE_DEFAULT_COUNT;
        }

        uint8 objectCount = ReadInt8(&info);
        for (int32 o = 0; o < objectCount; ++o) {
            ReadString(&info, textBuffer);

            RETRO_HASH_MD5(hash);
            GEN_HASH_MD5(textBuffer, hash);

            stageObjectIDs[sceneInfo.classCount] = 0;
            for (int32 id = 0; id < objectClassCount; ++id) {
                if (HASH_MATCH_MD5(hash, objectClassList[id].hash)) {
                    stageObjectIDs[sceneInfo.classCount] = id;
                    sceneInfo.classCount++;
                }
            }
        }

        for (int32 o = 0; o < sceneInfo.classCount; ++o) {
            ObjectClass *objClass = &objectClassList[stageObjectIDs[o]];
            if (objClass->staticVars && !*objClass->staticVars) {
                AllocateStorage((void **)objClass->staticVars, objClass->staticClassSize, DATASET_STG, true);

#if RETRO_REV0U
                if (objClass->staticLoad)
                    objClass->staticLoad(*objClass->staticVars);
                else
                    LoadStaticVariables((uint8 *)*objClass->staticVars, objClass->hash, sizeof(Object));
#else
                LoadStaticVariables((uint8 *)*objClass->staticVars, objClass->hash, sizeof(Object));
#endif


#if RETRO_USE_MOD_LOADER
                for (ModInfo &mod : modList) {
                    if (mod.staticVars.find(objClass->hash) != mod.staticVars.end()) {
                        auto sVars = mod.staticVars.at(objClass->hash);
                        RegisterStaticVariables((void **)sVars.staticVars, sVars.name.c_str(), sVars.size);
                    }
                }
#endif

                (*objClass->staticVars)->classID = o;
                if (o >= TYPE_DEFAULT_COUNT)
                    (*objClass->staticVars)->active = ACTIVE_NORMAL;
            }
        }

        for (int32 p = 0; p < PALETTE_BANK_COUNT; ++p) {
            activeStageRows[p] = ReadInt16(&info);

            for (int32 r = 0; r < 0x10; ++r) {
                if ((activeStageRows[p] >> r & 1)) {
                    for (int32 c = 0; c < 0x10; ++c) {
                        uint8 red                     = ReadInt8(&info);
                        uint8 green                   = ReadInt8(&info);
                        uint8 blue                    = ReadInt8(&info);
                        stagePalette[p][(r << 4) + c] = rgb32To16_B[blue] | rgb32To16_G[green] | rgb32To16_R[red];
                    }
                }
                else {
                    for (int32 c = 0; c < 0x10; ++c) stagePalette[p][(r << 4) + c] = 0;
                }
            }
        }

        uint8 sfxCount = ReadInt8(&info);
        char sfxPath[0x100];
        for (int32 i = 0; i < sfxCount; ++i) {
            ReadString(&info, sfxPath);
            uint8 maxConcurrentPlays = ReadInt8(&info);
            LoadSfx(sfxPath, maxConcurrentPlays, SCOPE_STAGE);
        }

        CloseFile(&info);
    }

    sprintf_s(fullFilePath, (int32)sizeof(fullFilePath), "Data/Stages/%s/16x16Tiles.gif", currentSceneFolder);
    LoadStageGIF(fullFilePath);

#if RETRO_USE_MOD_LOADER
    for (int32 h = 0; h < (int32)objectHookList.size(); ++h) {
        for (int32 i = 0; i < objectClassCount; ++i) {
            if (HASH_MATCH_MD5(objectClassList[i].hash, objectHookList[h].hash)) {
                if (objectHookList[h].staticVars && objectClassList[i].staticVars)
                    *objectHookList[h].staticVars = *objectClassList[i].staticVars;
                break;
            }
        }
    }
#endif
}
void RSDK::LoadSceneAssets()
{
    memset(objectEntityList, 0, ENTITY_COUNT * sizeof(EntityBase));

    SceneListEntry *sceneEntry = &sceneInfo.listData[sceneInfo.listPos];
    char fullFilePath[0x40];
    sprintf_s(fullFilePath, (int32)sizeof(fullFilePath), "Data/Stages/%s/Scene%s.bin", currentSceneFolder, sceneEntry->id);

    dataStorage[DATASET_TMP].usedStorage = 0;

    for (int32 s = 0; s < SCREEN_COUNT; ++s) screens[s].waterDrawPos = screens[s].size.y;

    if (screens[0].size.y > 0)
        memset(gfxLineBuffer, 0, screens[0].size.y * sizeof(uint8));

    memset(tileLayers, 0, LAYER_COUNT * sizeof(TileLayer));

    // Reload palette
    for (int32 b = 0; b < 8; ++b) {
        for (int32 r = 0; r < 0x10; ++r) {
            if ((activeGlobalRows[b] >> r & 1)) {
                for (int32 c = 0; c < 0x10; ++c) fullPalette[b][(r << 4) + c] = globalPalette[b][(r << 4) + c];
            }

            if ((activeStageRows[b] >> r & 1)) {
                for (int32 c = 0; c < 0x10; ++c) fullPalette[b][(r << 4) + c] = stagePalette[b][(r << 4) + c];
            }
        }
    }

    FileInfo info;
    InitFileInfo(&info);
    if (LoadFile(&info, fullFilePath, FMODE_RB)) {
        uint32 sig = ReadInt32(&info, false);

        if (sig != RSDK_SIGNATURE_SCN) {
            CloseFile(&info);
            return;
        }

        // Editor Metadata

        // I'm leaving this section here so that the "format" can be documented, since the official code is 3 lines and just skips it lol

        /*
        uint8 unknown1 = ReadInt8(&info); // usually 3, sometimes 4, LRZ1 (old) is 2

        uint8 b                = ReadInt8(&info);
        uint8 g                = ReadInt8(&info);
        uint8 r                = ReadInt8(&info);
        uint8 a                = ReadInt8(&info);
        color backgroundColor1 = (a << 24) | (r << 16) | (g << 8) | (b << 0);

        b                      = ReadInt8(&info);
        g                      = ReadInt8(&info);
        r                      = ReadInt8(&info);
        a                      = ReadInt8(&info);
        color backgroundColor2 = (a << 24) | (r << 16) | (g << 8) | (b << 0);

        uint8 unknown2 = ReadInt8(&info); // always 1 afaik
        uint8 unknown3 = ReadInt8(&info); // always 1 afaik
        uint8 unknown4 = ReadInt8(&info); // always 4 afaik
        uint8 unknown5 = ReadInt8(&info); // always 0 afaik
        uint8 unknown6 = ReadInt8(&info); // always 1 afaik
        uint8 unknown7 = ReadInt8(&info); // always 4 afaik
        uint8 unknown8 = ReadInt8(&info); // always 0 afaik

        char stampName[0x20];
        ReadString(&info, stampName);

        uint8 unknown9 = ReadInt8(&info); // usually 3, 4, or 5
        */

        // Skip over Metadata, since we won't be using it at all in-game
        Seek_Cur(&info, 0x10);
        uint8 strLen = ReadInt8(&info);
        Seek_Cur(&info, strLen + 1);

        // Tile Layers
        uint8 layerCount = ReadInt8(&info);
        for (int32 l = 0; l < layerCount; ++l) {
            TileLayer *layer = &tileLayers[l];

            // Tests in RetroED & comparing images of the RSDKv5 editor we have puts this as the most likely use for this (otherwise unused) variable
            bool32 visibleInEditor = ReadInt8(&info) != 0;
            (void)visibleInEditor; // unused

            ReadString(&info, textBuffer);
            GEN_HASH_MD5(textBuffer, layer->name);

            layer->type         = ReadInt8(&info);
            layer->drawGroup[0] = ReadInt8(&info);
            for (int32 s = 1; s < CAMERA_COUNT; ++s) layer->drawGroup[s] = layer->drawGroup[0];

            layer->xsize = ReadInt16(&info);
            int32 shift  = 1;
            int32 shift2 = 1;
            int32 val    = 0;
            do {
                shift = shift2;
                val   = 1 << shift2++;
            } while (val < layer->xsize);
            layer->widthShift = shift;

            layer->ysize = ReadInt16(&info);
            shift        = 1;
            shift2       = 1;
            val          = 0;
            do {
                shift = shift2;
                val   = 1 << shift2++;
            } while (val < layer->ysize);
            layer->heightShift = shift;

            layer->parallaxFactor = ReadInt16(&info);
            layer->scrollSpeed    = ReadInt16(&info) << 8;
            layer->scrollPos      = 0;

            layer->layout = NULL;
            if (layer->xsize || layer->ysize) {
                AllocateStorage((void **)&layer->layout, sizeof(uint16) * (1 << layer->widthShift) * (1 << layer->heightShift), DATASET_STG, true);
                memset(layer->layout, 0xFF, sizeof(uint16) * (1 << layer->widthShift) * (1 << layer->heightShift));
            }

            int32 size = layer->xsize;
            if (size <= layer->ysize)
                size = layer->ysize;
            AllocateStorage((void **)&layer->lineScroll, TILE_SIZE * size, DATASET_STG, true);

            layer->scrollInfoCount = ReadInt16(&info);
            for (int32 s = 0; s < layer->scrollInfoCount; ++s) {
                layer->scrollInfo[s].parallaxFactor = ReadInt16(&info);
                layer->scrollInfo[s].scrollSpeed    = ReadInt16(&info) << 8;
                layer->scrollInfo[s].scrollPos      = 0;
                layer->scrollInfo[s].tilePos        = 0;
                layer->scrollInfo[s].deform         = ReadInt8(&info);

                // this isn't used anywhere in-engine, and is never set in the files. so as you might expect, no one knows what it is for!
                layer->scrollInfo[s].unknown = ReadInt8(&info);
            }

            uint8 *scrollIndexes = NULL;
            ReadCompressed(&info, (uint8 **)&scrollIndexes);
            memcpy(layer->lineScroll, scrollIndexes, TILE_SIZE * size * sizeof(uint8));
            scrollIndexes = NULL;

            uint8 *tileLayout = NULL;
            ReadCompressed(&info, (uint8 **)&tileLayout);

            int32 id = 0;
            for (int32 y = 0; y < layer->ysize; ++y) {
                for (int32 x = 0; x < layer->xsize; ++x) {
                    layer->layout[x + (y << layer->widthShift)] = (tileLayout[id + 1] << 8) + tileLayout[id + 0];
                    id += 2;
                }
            }

            tileLayout = NULL;
        }

        // Objects
        uint8 objectCount = ReadInt8(&info);
        editableVarList   = NULL;
        AllocateStorage((void **)&editableVarList, sizeof(EditableVarInfo) * EDITABLEVAR_COUNT, DATASET_TMP, false);

#if RETRO_REV02
        EntityBase *tempEntityList = NULL;
        AllocateStorage((void **)&tempEntityList, SCENEENTITY_COUNT * sizeof(EntityBase), DATASET_TMP, true);
#endif

        for (int32 i = 0; i < objectCount; ++i) {
            RETRO_HASH_MD5(objHash);
            objHash[0] = ReadInt32(&info, false);
            objHash[1] = ReadInt32(&info, false);
            objHash[2] = ReadInt32(&info, false);
            objHash[3] = ReadInt32(&info, false);

            int32 classID = 0;
            for (int32 o = 0; o < sceneInfo.classCount; ++o) {
                if (HASH_MATCH_MD5(objHash, objectClassList[stageObjectIDs[o]].hash)) {
                    classID = o;
                    break;
                }
            }

#if !RETRO_USE_ORIGINAL_CODE
            if (!classID && i >= TYPE_DEFAULT_COUNT)
                PrintLog(PRINT_NORMAL, "Object Class %d is unimplimented!", i);
#endif

            ObjectClass *objectClass = &objectClassList[stageObjectIDs[classID]];

            uint8 varCount           = ReadInt8(&info);
            EditableVarInfo *varList = NULL;
            AllocateStorage((void **)&varList, sizeof(EditableVarInfo) * varCount, DATASET_TMP, false);
            editableVarCount = 0;
            if (classID) {
#if RETRO_REV02
                SetEditableVar(VAR_UINT8, "filter", classID, offsetof(Entity, filter));
#endif

#if RETRO_USE_MOD_LOADER
                currentObjectID = classID;
#endif

                if (objectClass->serialize)
                    objectClass->serialize();
            }

            for (int32 e = 1; e < varCount; ++e) {
                RETRO_HASH_MD5(varHash);
                varHash[0] = ReadInt32(&info, false);
                varHash[1] = ReadInt32(&info, false);
                varHash[2] = ReadInt32(&info, false);
                varHash[3] = ReadInt32(&info, false);

                int32 varID = 0;
                MEM_ZERO(varList[e]);
                for (int32 v = 0; v < editableVarCount; ++v) {
                    if (HASH_MATCH_MD5(varHash, editableVarList[v].hash)) {
                        varID = v;
                        HASH_COPY_MD5(varList[e].hash, editableVarList[v].hash);
                        varList[e].offset = editableVarList[v].offset;
                        varList[e].active = true;
                        break;
                    }
                }

                editableVarList[varID].type = varList[e].type = ReadInt8(&info);
            }

            uint16 entityCount = ReadInt16(&info);
            for (int32 e = 0; e < entityCount; ++e) {
                uint16 slotID = ReadInt16(&info);

                EntityBase *entity = NULL;

#if RETRO_REV02
                if (slotID < SCENEENTITY_COUNT)
                    entity = &objectEntityList[slotID + RESERVE_ENTITY_COUNT];
                else
                    entity = &tempEntityList[slotID - SCENEENTITY_COUNT];
#else
                entity = &objectEntityList[slotID + RESERVE_ENTITY_COUNT];
#endif

                entity->classID = classID;
#if RETRO_REV02
                entity->filter = 0xFF;
#endif
                entity->position.x = ReadInt32(&info, false);
                entity->position.y = ReadInt32(&info, false);

                uint8 *entityBuffer = (uint8 *)entity;

                uint8 tempBuffer[0x10];
                for (int32 v = 1; v < varCount; ++v) {
                    switch (varList[v].type) {
                        case VAR_UINT8:
                        case VAR_INT8:
                            if (varList[v].active)
                                ReadBytes(&info, &entityBuffer[varList[v].offset], sizeof(int8));
                            else
                                ReadBytes(&info, tempBuffer, sizeof(int8));
                            break;

                        case VAR_UINT16:
                        case VAR_INT16:
                            if (varList[v].active)
                                ReadBytes(&info, &entityBuffer[varList[v].offset], sizeof(int16));
                            else
                                ReadBytes(&info, tempBuffer, sizeof(int16));
                            break;

                        case VAR_UINT32:
                        case VAR_INT32:
                            if (varList[v].active)
                                ReadBytes(&info, &entityBuffer[varList[v].offset], sizeof(int32));
                            else
                                ReadBytes(&info, tempBuffer, sizeof(int32));
                            break;

                        // not entirely sure on specifics here, should always be sizeof(int32) but it having a unique type implies it isn't always
                        case VAR_ENUM:
                            if (varList[v].active)
                                ReadBytes(&info, &entityBuffer[varList[v].offset], sizeof(int32));
                            else
                                ReadBytes(&info, tempBuffer, sizeof(int32));
                            break;

                        case VAR_BOOL:
                            if (varList[v].active)
                                ReadBytes(&info, &entityBuffer[varList[v].offset], sizeof(bool32));
                            else
                                ReadBytes(&info, tempBuffer, sizeof(bool32));
                            break;

                        case VAR_STRING:
                            if (varList[v].active) {
                                String *string = (String *)&entityBuffer[varList[v].offset];
                                uint16 len     = ReadInt16(&info);

                                InitString(string, (char *)"", len);
                                for (string->length = 0; string->length < len; ++string->length) string->chars[string->length] = ReadInt16(&info);
                            }
                            else {
                                Seek_Cur(&info, ReadInt16(&info) * sizeof(uint16));
                            }
                            break;

                        case VAR_VECTOR2:
                            if (varList[v].active) {
                                ReadBytes(&info, &entityBuffer[varList[v].offset], sizeof(int32));
                                ReadBytes(&info, &entityBuffer[varList[v].offset + sizeof(int32)], sizeof(int32));
                            }
                            else {
                                ReadBytes(&info, tempBuffer, sizeof(int32)); // x
                                ReadBytes(&info, tempBuffer, sizeof(int32)); // y
                            }
                            break;

                        // Never used in mania so we don't know for sure, but it's our best guess!
                        case VAR_FLOAT:
                            if (varList[v].active)
                                ReadBytes(&info, &entityBuffer[varList[v].offset], sizeof(float));
                            else
                                ReadBytes(&info, tempBuffer, sizeof(float));
                            break;

                        case VAR_COLOR:
                            if (varList[v].active)
                                ReadBytes(&info, &entityBuffer[varList[v].offset], sizeof(color));
                            else
                                ReadBytes(&info, tempBuffer, sizeof(color));
                            break;
                    }
                }
            }
        }

#if RETRO_REV02
        // handle filter and stuff
        EntityBase *entity = &objectEntityList[RESERVE_ENTITY_COUNT];
        int32 activeSlot   = RESERVE_ENTITY_COUNT;
        for (int32 i = RESERVE_ENTITY_COUNT; i < SCENEENTITY_COUNT + RESERVE_ENTITY_COUNT; ++i) {
            if (sceneInfo.filter & entity->filter) {
                if (i != activeSlot) {
                    memcpy(&objectEntityList[activeSlot], entity, sizeof(EntityBase));
                    memset(entity, 0, sizeof(EntityBase));
                }

                ++activeSlot;
            }
            else {
                memset(entity, 0, sizeof(EntityBase));
            }

            entity++;
        }

        for (int32 i = 0; i < SCENEENTITY_COUNT; ++i) {
            if (sceneInfo.filter & tempEntityList[i].filter)
                memcpy(&objectEntityList[activeSlot++], &tempEntityList[i], sizeof(EntityBase));

            if (activeSlot >= SCENEENTITY_COUNT + RESERVE_ENTITY_COUNT)
                break;
        }

        tempEntityList = NULL;
#endif

        editableVarList = NULL;

        CloseFile(&info);
    }
}
void RSDK::LoadTileConfig(char *filepath)
{
    FileInfo info;
    InitFileInfo(&info);

    if (LoadFile(&info, filepath, FMODE_RB)) {
        uint32 sig = ReadInt32(&info, false);
        if (sig != RSDK_SIGNATURE_TIL) {
            CloseFile(&info);
            return;
        }

        uint8 *buffer = NULL;
        ReadCompressed(&info, &buffer);

        int32 bufPos = 0;
        for (int32 p = 0; p < CPATH_COUNT; ++p) {
            // No Flip/Stored in file
            for (int32 t = 0; t < TILE_COUNT; ++t) {
                uint8 maskHeights[0x10];
                uint8 maskActive[0x10];

                memcpy(maskHeights, buffer + bufPos, TILE_SIZE * sizeof(uint8));
                bufPos += TILE_SIZE;
                memcpy(maskActive, buffer + bufPos, TILE_SIZE * sizeof(uint8));
                bufPos += TILE_SIZE;

                bool32 yFlip                    = buffer[bufPos++];
                tileInfo[p][t].floorAngle = buffer[bufPos++];
                tileInfo[p][t].lWallAngle = buffer[bufPos++];
                tileInfo[p][t].rWallAngle = buffer[bufPos++];
                tileInfo[p][t].roofAngle  = buffer[bufPos++];
                tileInfo[p][t].flag       = buffer[bufPos++];

                if (yFlip) {
                    for (int32 c = 0; c < TILE_SIZE; c++) {
                        if (maskActive[c]) {
                            collisionMasks[p][t].floorMasks[c] = 0x00;
                            collisionMasks[p][t].roofMasks[c]  = maskHeights[c];
                        }
                        else {
                            collisionMasks[p][t].floorMasks[c] = 0xFF;
                            collisionMasks[p][t].roofMasks[c]  = 0xFF;
                        }
                    }

                    // LWall rotations
                    for (int32 c = 0; c < TILE_SIZE; ++c) {
                        int32 h = 0;
                        while (true) {
                            if (h == TILE_SIZE) {
                                collisionMasks[p][t].lWallMasks[c] = 0xFF;
                                break;
                            }

                            uint8 m = collisionMasks[p][t].roofMasks[h];
                            if (m != 0xFF && c <= m) {
                                collisionMasks[p][t].lWallMasks[c] = h;
                                break;
                            }
                            else {
                                ++h;
                                if (h <= -1)
                                    break;
                            }
                        }
                    }

                    // RWall rotations
                    for (int32 c = 0; c < TILE_SIZE; ++c) {
                        int32 h = TILE_SIZE - 1;
                        while (true) {
                            if (h == -1) {
                                collisionMasks[p][t].rWallMasks[c] = 0xFF;
                                break;
                            }

                            uint8 m = collisionMasks[p][t].roofMasks[h];
                            if (m != 0xFF && c <= m) {
                                collisionMasks[p][t].rWallMasks[c] = h;
                                break;
                            }
                            else {
                                --h;
                                if (h >= TILE_SIZE)
                                    break;
                            }
                        }
                    }
                }
                else // Regular Tile
                {
                    // Collision heights
                    for (int32 c = 0; c < TILE_SIZE; ++c) {
                        if (maskActive[c]) {
                            collisionMasks[p][t].floorMasks[c] = maskHeights[c];
                            collisionMasks[p][t].roofMasks[c]  = 0x0F;
                        }
                        else {
                            collisionMasks[p][t].floorMasks[c] = 0xFF;
                            collisionMasks[p][t].roofMasks[c]  = 0xFF;
                        }
                    }

                    // LWall rotations
                    for (int32 c = 0; c < TILE_SIZE; ++c) {
                        int32 h = 0;
                        while (true) {
                            if (h == TILE_SIZE) {
                                collisionMasks[p][t].lWallMasks[c] = 0xFF;
                                break;
                            }

                            uint8 m = collisionMasks[p][t].floorMasks[h];
                            if (m != 0xFF && c >= m) {
                                collisionMasks[p][t].lWallMasks[c] = h;
                                break;
                            }
                            else {
                                ++h;
                                if (h <= -1)
                                    break;
                            }
                        }
                    }

                    // RWall rotations
                    for (int32 c = 0; c < TILE_SIZE; ++c) {
                        int32 h = TILE_SIZE - 1;
                        while (true) {
                            if (h == -1) {
                                collisionMasks[p][t].rWallMasks[c] = 0xFF;
                                break;
                            }

                            uint8 m = collisionMasks[p][t].floorMasks[h];
                            if (m != 0xFF && c >= m) {
                                collisionMasks[p][t].rWallMasks[c] = h;
                                break;
                            }
                            else {
                                --h;
                                if (h >= TILE_SIZE)
                                    break;
                            }
                        }
                    }
                }
            }

            // FlipX
            for (int32 t = 0; t < TILE_COUNT; ++t) {
                int32 off                             = (FLIP_X * TILE_COUNT);
                tileInfo[p][t + off].flag       = tileInfo[p][t].flag;
                tileInfo[p][t + off].floorAngle = -tileInfo[p][t].floorAngle;
                tileInfo[p][t + off].lWallAngle = -tileInfo[p][t].rWallAngle;
                tileInfo[p][t + off].roofAngle  = -tileInfo[p][t].roofAngle;
                tileInfo[p][t + off].rWallAngle = -tileInfo[p][t].lWallAngle;

                for (int32 c = 0; c < TILE_SIZE; ++c) {
                    int32 h = collisionMasks[p][t].lWallMasks[c];
                    if (h == 0xFF)
                        collisionMasks[p][t + off].rWallMasks[c] = 0xFF;
                    else
                        collisionMasks[p][t + off].rWallMasks[c] = 0xF - h;

                    h = collisionMasks[p][t].rWallMasks[c];
                    if (h == 0xFF)
                        collisionMasks[p][t + off].lWallMasks[c] = 0xFF;
                    else
                        collisionMasks[p][t + off].lWallMasks[c] = 0xF - h;

                    collisionMasks[p][t + off].floorMasks[c] = collisionMasks[p][t].floorMasks[0xF - c];
                    collisionMasks[p][t + off].roofMasks[c]  = collisionMasks[p][t].roofMasks[0xF - c];
                }
            }

            // FlipY
            for (int32 t = 0; t < TILE_COUNT; ++t) {
                int32 off                             = (FLIP_Y * TILE_COUNT);
                tileInfo[p][t + off].flag       = tileInfo[p][t].flag;
                tileInfo[p][t + off].floorAngle = -0x80 - tileInfo[p][t].roofAngle;
                tileInfo[p][t + off].lWallAngle = -0x80 - tileInfo[p][t].lWallAngle;
                tileInfo[p][t + off].roofAngle  = -0x80 - tileInfo[p][t].floorAngle;
                tileInfo[p][t + off].rWallAngle = -0x80 - tileInfo[p][t].rWallAngle;

                for (int32 c = 0; c < TILE_SIZE; ++c) {
                    int32 h = collisionMasks[p][t].roofMasks[c];
                    if (h == 0xFF)
                        collisionMasks[p][t + off].floorMasks[c] = 0xFF;
                    else
                        collisionMasks[p][t + off].floorMasks[c] = 0xF - h;

                    h = collisionMasks[p][t].floorMasks[c];
                    if (h == 0xFF)
                        collisionMasks[p][t + off].roofMasks[c] = 0xFF;
                    else
                        collisionMasks[p][t + off].roofMasks[c] = 0xF - h;

                    collisionMasks[p][t + off].lWallMasks[c] = collisionMasks[p][t].lWallMasks[0xF - c];
                    collisionMasks[p][t + off].rWallMasks[c] = collisionMasks[p][t].rWallMasks[0xF - c];
                }
            }

            // FlipXY
            for (int32 t = 0; t < TILE_COUNT; ++t) {
                int32 off                             = (FLIP_XY * TILE_COUNT);
                int32 offY                            = (FLIP_Y * TILE_COUNT);
                tileInfo[p][t + off].flag             = tileInfo[p][t + offY].flag;
                tileInfo[p][t + off].floorAngle = -tileInfo[p][t + offY].floorAngle;
                tileInfo[p][t + off].lWallAngle = -tileInfo[p][t + offY].rWallAngle;
                tileInfo[p][t + off].roofAngle  = -tileInfo[p][t + offY].roofAngle;
                tileInfo[p][t + off].rWallAngle = -tileInfo[p][t + offY].lWallAngle;

                for (int32 c = 0; c < TILE_SIZE; ++c) {
                    int32 h = collisionMasks[p][t + offY].lWallMasks[c];
                    if (h == 0xFF)
                        collisionMasks[p][t + off].rWallMasks[c] = 0xFF;
                    else
                        collisionMasks[p][t + off].rWallMasks[c] = 0xF - h;

                    h = collisionMasks[p][t + offY].rWallMasks[c];
                    if (h == 0xFF)
                        collisionMasks[p][t + off].lWallMasks[c] = 0xFF;
                    else
                        collisionMasks[p][t + off].lWallMasks[c] = 0xF - h;

                    collisionMasks[p][t + off].floorMasks[c] = collisionMasks[p][t + offY].floorMasks[0xF - c];
                    collisionMasks[p][t + off].roofMasks[c]  = collisionMasks[p][t + offY].roofMasks[0xF - c];
                }
            }
        }

        CloseFile(&info);
    }
}
void RSDK::LoadStageGIF(char *filepath)
{
    ImageGIF tileset;

    if (tileset.Load(filepath, true) && tileset.width == TILE_SIZE && tileset.height <= TILE_COUNT * TILE_SIZE) {
        tileset.pixels = tilesetPixels;
        tileset.Load(NULL, false);

        for (int32 r = 0; r < 0x10; ++r) {
            // only overwrite inactive rows
            if (!(activeStageRows[0] >> r & 1) && !(activeGlobalRows[0] >> r & 1)) {
                for (int32 c = 0; c < 0x10; ++c) {
                    uint8 red                    = (tileset.palette[(r << 4) + c] >> 0x10);
                    uint8 green                  = (tileset.palette[(r << 4) + c] >> 0x08);
                    uint8 blue                   = (tileset.palette[(r << 4) + c] >> 0x00);
                    fullPalette[0][(r << 4) + c] = rgb32To16_B[blue] | rgb32To16_G[green] | rgb32To16_R[red];
                }
            }
        }

        // Flip X
        uint8 *srcPixels = tilesetPixels;
        uint8 *dstPixels = &tilesetPixels[(FLIP_X * TILESET_SIZE) + (TILE_SIZE - 1)];
        for (int32 t = 0; t < 0x400 * TILE_SIZE; ++t) {
            for (int32 r = 0; r < TILE_SIZE; ++r) {
                *dstPixels-- = *srcPixels++;
            }

            dstPixels += (TILE_SIZE * 2);
        }

        // Flip Y
        srcPixels = tilesetPixels;
        for (int32 t = 0; t < 0x400; ++t) {
            dstPixels = &tilesetPixels[(FLIP_Y * TILESET_SIZE) + (t * TILE_DATASIZE) + (TILE_DATASIZE - TILE_SIZE)];
            for (int32 y = 0; y < TILE_SIZE; ++y) {
                for (int32 x = 0; x < TILE_SIZE; ++x) {
                    *dstPixels++ = *srcPixels++;
                }

                dstPixels -= (TILE_SIZE * 2);
            }
        }

        // Flip XY
        srcPixels = &tilesetPixels[(FLIP_Y * TILESET_SIZE)];
        dstPixels = &tilesetPixels[(FLIP_XY * TILESET_SIZE) + (TILE_SIZE - 1)];
        for (int32 t = 0; t < 0x400 * TILE_SIZE; ++t) {
            for (int32 r = 0; r < TILE_SIZE; ++r) {
                *dstPixels-- = *srcPixels++;
            }

            dstPixels += (TILE_SIZE * 2);
        }

        tileset.palette = NULL;
        tileset.decoder = NULL;
        tileset.pixels  = NULL;
    }
}

void RSDK::ProcessParallaxAutoScroll()
{
    for (int32 l = 0; l < LAYER_COUNT; ++l) {
        TileLayer *layer = &tileLayers[l];

        if (layer->layout) {
            layer->scrollPos += layer->scrollSpeed;

            for (int32 s = 0; s < layer->scrollInfoCount; ++s) layer->scrollInfo[s].scrollPos += layer->scrollInfo[s].scrollSpeed;
        }
    }
}
void RSDK::ProcessParallax(TileLayer *layer)
{
    if (!layer->xsize || !layer->ysize)
        return;

    int32 pixelWidth          = TILE_SIZE * layer->xsize;
    int32 pixelHeight         = TILE_SIZE * layer->ysize;
    ScanlineInfo *scanlinePtr = scanlines;
    ScrollInfo *scrollInfo    = layer->scrollInfo;

    switch (layer->type) {
        default: break;

        case LAYER_HSCROLL: {
            for (int32 i = 0; i < layer->scrollInfoCount; ++i) {
                scrollInfo->tilePos = scrollInfo->scrollPos + (currentScreen->position.x * scrollInfo->parallaxFactor << 8);

                int16 tilePos = (scrollInfo->tilePos >> 16) % pixelWidth;
                if (tilePos < 0)
                    tilePos += pixelWidth;
                scrollInfo->tilePos = tilePos << 0x10;

                ++scrollInfo;
            }

            int16 scrollPos =
                ((int32)((layer->scrollPos + (layer->parallaxFactor * currentScreen->position.y << 8)) & 0xFFFF0000) >> 16) % pixelHeight;
            if (scrollPos < 0)
                scrollPos += pixelHeight;

            uint8 *lineScrollPtr = &layer->lineScroll[scrollPos];

            // Above water
            int32 *deformationData = &layer->deformationData[(scrollPos + (uint16)layer->deformationOffset) & 0x1FF];
            for (int32 i = 0; i < currentScreen->waterDrawPos; ++i) {
                scanlinePtr->position.x = layer->scrollInfo[*lineScrollPtr].tilePos;
                if (layer->scrollInfo[*lineScrollPtr].deform)
                    scanlinePtr->position.x += *deformationData << 0x10;

                scanlinePtr->position.y = scrollPos++ << 0x10;

                deformationData++;
                if (scrollPos == pixelHeight) {
                    lineScrollPtr = layer->lineScroll;
                    scrollPos     = 0;
                }
                else {
                    ++lineScrollPtr;
                }
                scanlinePtr++;
            }

            // Under water
            deformationData = &layer->deformationDataW[(scrollPos + (uint16)layer->deformationOffsetW) & 0x1FF];
            for (int32 i = currentScreen->waterDrawPos; i < currentScreen->size.y; ++i) {
                scanlinePtr->position.x = layer->scrollInfo[*lineScrollPtr].tilePos;
                if (layer->scrollInfo[*lineScrollPtr].deform)
                    scanlinePtr->position.x += *deformationData << 0x10;

                scanlinePtr->position.y = scrollPos++ << 0x10;

                scanlinePtr->deform.x = 1 << 16;
                scanlinePtr->deform.y = 0 << 16;

                deformationData++;
                if (scrollPos == pixelHeight) {
                    lineScrollPtr = layer->lineScroll;
                    scrollPos     = 0;
                }
                else {
                    ++lineScrollPtr;
                }
                scanlinePtr++;
            }
            break;
        }

        case LAYER_VSCROLL: {
            for (int32 i = 0; i < layer->scrollInfoCount; ++i) {
                scrollInfo->tilePos = scrollInfo->scrollPos + (currentScreen->position.y * scrollInfo->parallaxFactor << 8);
                scrollInfo->tilePos = ((scrollInfo->tilePos >> 16) % pixelHeight) << 0x10;

                ++scrollInfo;
            }

            int16 scrollPos =
                ((int32)((layer->scrollPos + (layer->parallaxFactor * currentScreen->position.x << 8)) & 0xFFFF0000) >> 16) % pixelWidth;
            if (scrollPos < 0)
                scrollPos += pixelWidth;

            uint8 *lineScrollPtr = &layer->lineScroll[scrollPos];

            // Above water
            for (int32 i = 0; i < currentScreen->size.x; ++i) {
                scanlinePtr->position.x = scrollPos++ << 0x10;
                scanlinePtr->position.y = layer->scrollInfo[*lineScrollPtr].tilePos;
                scanlinePtr->deform.x   = 1 << 16;
                scanlinePtr->deform.y   = 0 << 16;

                if (scrollPos == pixelWidth) {
                    lineScrollPtr = layer->lineScroll;
                    scrollPos     = 0;
                }
                else {
                    ++lineScrollPtr;
                }

                scanlinePtr++;
            }
            break;
        }

        case LAYER_ROTOZOOM: {
            int16 scrollPosX =
                ((int32)((layer->scrollPos + (layer->parallaxFactor * currentScreen->position.x << 8)) & 0xFFFF0000) >> 0x10) % pixelWidth;
            if (scrollPosX < 0)
                scrollPosX += pixelWidth;

            int16 scrollPosY =
                ((int32)((layer->scrollPos + (layer->parallaxFactor * currentScreen->position.y << 8)) & 0xFFFF0000) >> 0x10) % pixelHeight;
            if (scrollPosY < 0)
                scrollPosY += pixelHeight;

            for (int32 i = 0; i < currentScreen->size.y; ++i) {
                scanlinePtr->position.x = scrollPosX << 0x10;
                scanlinePtr->position.y = scrollPosY++ << 0x10;
                scanlinePtr->deform.x   = 1 << 16;
                scanlinePtr->deform.y   = 0 << 16;

                scanlinePtr++;
            }
            break;
        }

        case LAYER_BASIC: {
            for (int32 i = 0; i < layer->scrollInfoCount; ++i) {
                scrollInfo->tilePos = scrollInfo->scrollPos + (currentScreen->position.x * scrollInfo->parallaxFactor << 8);

                int16 tilePos = (scrollInfo->tilePos >> 16) % pixelWidth;
                if (tilePos < 0)
                    tilePos += pixelWidth;
                scrollInfo->tilePos = tilePos << 0x10;

                ++scrollInfo;
            }

            int16 scrollPos =
                ((int32)((layer->scrollPos + (layer->parallaxFactor * currentScreen->position.y << 8)) & 0xFFFF0000) >> 16) % pixelHeight;
            if (scrollPos < 0)
                scrollPos += pixelHeight;

            uint8 *lineScrollPtr = &layer->lineScroll[scrollPos];

            // Above water
            int32 *deformationData = &layer->deformationData[((scrollPos >> 16) + (uint16)layer->deformationOffset) & 0x1FF];
            for (int32 i = 0; i < currentScreen->waterDrawPos; ++i) {
                scanlinePtr->position.x = layer->scrollInfo[*lineScrollPtr].tilePos;
                if (layer->scrollInfo[*lineScrollPtr].deform)
                    scanlinePtr->position.x += *deformationData;
                scanlinePtr->position.y = scrollPos++ << 0x10;

                scanlinePtr->deform.x = 1 << 16;
                scanlinePtr->deform.y = 0 << 16;

                deformationData++;
                if (scrollPos == pixelHeight) {
                    lineScrollPtr = layer->lineScroll;
                    scrollPos     = 0;
                }
                else {
                    ++lineScrollPtr;
                }
                scanlinePtr++;
            }

            // Under water
            deformationData = &layer->deformationDataW[((scrollPos >> 16) + (uint16)layer->deformationOffsetW) & 0x1FF];
            for (int32 i = currentScreen->waterDrawPos; i < currentScreen->size.y; ++i) {
                scanlinePtr->position.x = layer->scrollInfo[*lineScrollPtr].tilePos;
                if (layer->scrollInfo[*lineScrollPtr].deform)
                    scanlinePtr->position.x += *deformationData;
                scanlinePtr->position.y = scrollPos++ << 0x10;

                deformationData++;
                if (scrollPos == pixelHeight) {
                    lineScrollPtr = layer->lineScroll;
                    scrollPos     = 0;
                }
                else {
                    ++lineScrollPtr;
                }

                scanlinePtr++;
            }
            break;
        }
    }
}

void RSDK::ProcessSceneTimer()
{
    if (sceneInfo.timeEnabled) {
        sceneInfo.timeCounter += 100;

        if (sceneInfo.timeCounter >= 6000) {
            sceneInfo.timeCounter -= 6025;

            if (++sceneInfo.seconds >= 60) {
                sceneInfo.seconds = 0;

                if (++sceneInfo.minutes >= 60)
                    sceneInfo.minutes = 0;
            }
        }

        sceneInfo.milliseconds = sceneInfo.timeCounter / videoSettings.refreshRate;
    }
}

void RSDK::SetScene(const char *categoryName, const char *sceneName)
{
    RETRO_HASH_MD5(catHash);
    GEN_HASH_MD5(categoryName, catHash);

    RETRO_HASH_MD5(scnHash);
    GEN_HASH_MD5(sceneName, scnHash);

    for (int32 i = 0; i < sceneInfo.categoryCount; ++i) {
        if (HASH_MATCH_MD5(sceneInfo.listCategory[i].hash, catHash)) {
            sceneInfo.activeCategory = i;
            sceneInfo.listPos        = sceneInfo.listCategory[i].sceneOffsetStart;

            for (int32 s = 0; s < sceneInfo.listCategory[i].sceneCount; ++s) {
                if (HASH_MATCH_MD5(sceneInfo.listData[sceneInfo.listCategory[i].sceneOffsetStart + s].hash, scnHash)) {
                    sceneInfo.listPos = sceneInfo.listCategory[i].sceneOffsetStart + s;
                    break;
                }
            }

            break;
        }
    }
}

void RSDK::CopyTileLayout(uint16 dstLayerID, int32 dstStartX, int32 dstStartY, uint16 srcLayerID, int32 srcStartX, int32 srcStartY, int32 countX,
                          int32 countY)
{
    if (dstLayerID < LAYER_COUNT && srcLayerID < LAYER_COUNT) {
        TileLayer *dstLayer = &tileLayers[dstLayerID];
        TileLayer *srcLayer = &tileLayers[srcLayerID];

        if (dstStartX >= 0 && dstStartX < dstLayer->xsize && dstStartY >= 0 && dstStartY < dstLayer->ysize) {
            if (srcStartX >= 0 && srcStartX < srcLayer->xsize && srcStartY >= 0 && srcStartY < srcLayer->ysize) {
                if (dstStartX + countX > dstLayer->xsize)
                    countX = dstLayer->xsize - dstStartX;

                if (dstStartY + countY > dstLayer->ysize)
                    countY = dstLayer->ysize - dstStartY;

                if (srcStartX + countX > srcLayer->xsize)
                    countX = srcLayer->xsize - srcStartX;

                if (srcStartY + countY > srcLayer->ysize)
                    countY = srcLayer->ysize - srcStartY;

                for (int32 y = 0; y < countY; ++y) {
                    for (int32 x = 0; x < countX; ++x) {
                        uint16 tile = srcLayer->layout[(x + srcStartX) + ((y + srcStartY) << srcLayer->widthShift)];
                        dstLayer->layout[(x + dstStartX) + ((y + dstStartY) << dstLayer->widthShift)] = tile;
                    }
                }
            }
        }
    }
}

void RSDK::DrawLayerHScroll(TileLayer *layer)
{
    if (!layer->xsize || !layer->ysize)
        return;

    int32 lineTileCount    = (currentScreen->pitch >> 4) - 1;
    uint8 *lineBuffer      = &gfxLineBuffer[currentScreen->clipBound_Y1];
    ScanlineInfo *scanline = &scanlines[currentScreen->clipBound_Y1];
    uint16 *frameBuffer    = &currentScreen->frameBuffer[currentScreen->pitch * currentScreen->clipBound_Y1];

    for (int32 cy = currentScreen->clipBound_Y1; cy < currentScreen->clipBound_Y2; ++cy) {
        int32 x               = scanline->position.x;
        int32 y               = scanline->position.y;
        int32 tileX           = x >> 0x10;
        uint16 *activePalette = fullPalette[*lineBuffer++];

        if (tileX >= TILE_SIZE * layer->xsize)
            x = (tileX - TILE_SIZE * layer->xsize) << 0x10;
        else if (tileX < 0)
            x = (tileX + TILE_SIZE * layer->xsize) << 0x10;

        int32 tileRemain = TILE_SIZE - ((x >> 0x10) & 0xF);
        int32 sheetX     = (x >> 16) & 0xF;
        int32 sheetY     = TILE_SIZE * ((y >> 0x10) & 0xF);
        int32 lineRemain = currentScreen->pitch;

        int32 tx       = x >> 20;
        uint16 *layout = &layer->layout[tx + ((y >> 20) << layer->widthShift)];
        lineRemain -= tileRemain;

        if (*layout >= 0xFFFF) {
            frameBuffer += tileRemain;
        }
        else {
            uint8 *pixels = &tilesetPixels[TILE_DATASIZE * (*layout & 0xFFF) + sheetY + sheetX];
            for (int32 x = 0; x < tileRemain; ++x) {
                if (*pixels)
                    *frameBuffer = activePalette[*pixels];
                ++pixels;
                ++frameBuffer;
            }
        }

        for (int32 l = 0; l < lineTileCount; ++l) {
            ++layout;

            if (++tx == layer->xsize) {
                tx = 0;
                layout -= layer->xsize;
            }

            if (*layout < 0xFFFF) {
                uint8 *pixels = &tilesetPixels[TILE_DATASIZE * (*layout & 0xFFF) + sheetY];

                uint8 index = *pixels;
                if (index)
                    *frameBuffer = activePalette[index];

                index = pixels[1];
                if (index)
                    frameBuffer[1] = activePalette[index];

                index = pixels[2];
                if (index)
                    frameBuffer[2] = activePalette[index];

                index = pixels[3];
                if (index)
                    frameBuffer[3] = activePalette[index];

                index = pixels[4];
                if (index)
                    frameBuffer[4] = activePalette[index];

                index = pixels[5];
                if (index)
                    frameBuffer[5] = activePalette[index];

                index = pixels[6];
                if (index)
                    frameBuffer[6] = activePalette[index];

                index = pixels[7];
                if (index)
                    frameBuffer[7] = activePalette[index];

                index = pixels[8];
                if (index)
                    frameBuffer[8] = activePalette[index];

                index = pixels[9];
                if (index)
                    frameBuffer[9] = activePalette[index];

                index = pixels[10];
                if (index)
                    frameBuffer[10] = activePalette[index];

                index = pixels[11];
                if (index)
                    frameBuffer[11] = activePalette[index];

                index = pixels[12];
                if (index)
                    frameBuffer[12] = activePalette[index];

                index = pixels[13];
                if (index)
                    frameBuffer[13] = activePalette[index];

                index = pixels[14];
                if (index)
                    frameBuffer[14] = activePalette[index];

                index = pixels[15];
                if (index)
                    frameBuffer[15] = activePalette[index];
            }

            frameBuffer += TILE_SIZE;
            lineRemain -= TILE_SIZE;
        }

        while (lineRemain > 0) {
            ++layout;

            if (++tx == layer->xsize) {
                tx = 0;
                layout -= layer->xsize;
            }

            tileRemain = lineRemain >= TILE_SIZE ? TILE_SIZE : lineRemain;

            if (*layout >= 0xFFFF) {
                frameBuffer += tileRemain;
            }
            else {
                uint8 *pixels = &tilesetPixels[TILE_DATASIZE * (*layout & 0xFFF) + sheetY];
                for (int32 x = 0; x < tileRemain; ++x) {
                    if (*pixels)
                        *frameBuffer = activePalette[*pixels];
                    ++pixels;
                    ++frameBuffer;
                }
            }

            lineRemain -= TILE_SIZE;
        }

        ++scanline;
    }
}
void RSDK::DrawLayerVScroll(TileLayer *layer)
{
    if (!layer->xsize || !layer->ysize)
        return;

    int32 lineTileCount    = (currentScreen->size.y >> 4) - 1;
    uint16 *frameBuffer    = &currentScreen->frameBuffer[currentScreen->clipBound_X1];
    ScanlineInfo *scanline = &scanlines[currentScreen->clipBound_X1];
    uint16 *activePalette  = fullPalette[gfxLineBuffer[0]];

    for (int32 cx = currentScreen->clipBound_X1; cx < currentScreen->clipBound_X2; ++cx) {
        int32 x  = scanline->position.x;
        int32 y  = scanline->position.y;
        int32 ty = y >> 0x10;

        if (ty >= TILE_SIZE * layer->ysize)
            y -= (TILE_SIZE * layer->ysize) << 0x10;
        else if (ty < 0)
            y += (TILE_SIZE * layer->ysize) << 0x10;

        int32 tileRemain = TILE_SIZE - ((y >> 16) & 0xF);
        int32 sheetX     = (x >> 16) & 0xF;
        int32 sheetY     = (y >> 16) & 0xF;
        int32 lineRemain = currentScreen->size.y;

        uint16 *layout = &layer->layout[(x >> 20) + ((y >> 20) << layer->widthShift)];
        lineRemain -= tileRemain;

        if (*layout >= 0xFFFF) {
            frameBuffer += currentScreen->pitch * tileRemain;
        }
        else {
            uint8 *pixels = &tilesetPixels[TILE_SIZE * (sheetY + TILE_SIZE * (*layout & 0xFFF)) + sheetX];
            for (int32 y = 0; y < tileRemain; ++y) {
                if (*pixels)
                    *frameBuffer = activePalette[*pixels];
                pixels += TILE_SIZE;
                frameBuffer += currentScreen->pitch;
            }
        }

        ty = y >> 20;
        for (int32 l = 0; l < lineTileCount; ++l) {
            layout += layer->xsize;

            if (++ty == layer->ysize) {
                ty = 0;
                layout -= layer->ysize << layer->widthShift;
            }

            if (*layout >= 0xFFFF) {
                frameBuffer += TILE_SIZE * currentScreen->pitch;
            }
            else {
                uint8 *pixels = &tilesetPixels[TILE_DATASIZE * (*layout & 0xFFF) + sheetX];

                if (*pixels)
                    *frameBuffer = activePalette[*pixels];

                if (pixels[0x10])
                    frameBuffer[currentScreen->pitch * 1] = activePalette[pixels[0x10]];

                if (pixels[0x20])
                    frameBuffer[currentScreen->pitch * 2] = activePalette[pixels[0x20]];

                if (pixels[0x30])
                    frameBuffer[currentScreen->pitch * 3] = activePalette[pixels[0x30]];

                if (pixels[0x40])
                    frameBuffer[currentScreen->pitch * 4] = activePalette[pixels[0x40]];

                if (pixels[0x50])
                    frameBuffer[currentScreen->pitch * 5] = activePalette[pixels[0x50]];

                if (pixels[0x60])
                    frameBuffer[currentScreen->pitch * 6] = activePalette[pixels[0x60]];

                if (pixels[0x70])
                    frameBuffer[currentScreen->pitch * 7] = activePalette[pixels[0x70]];

                if (pixels[0x80])
                    frameBuffer[currentScreen->pitch * 8] = activePalette[pixels[0x80]];

                if (pixels[0x90])
                    frameBuffer[currentScreen->pitch * 9] = activePalette[pixels[0x90]];

                if (pixels[0xA0])
                    frameBuffer[currentScreen->pitch * 10] = activePalette[pixels[0xA0]];

                if (pixels[0xB0])
                    frameBuffer[currentScreen->pitch * 11] = activePalette[pixels[0xB0]];

                if (pixels[0xC0])
                    frameBuffer[currentScreen->pitch * 12] = activePalette[pixels[0xC0]];

                if (pixels[0xD0])
                    frameBuffer[currentScreen->pitch * 13] = activePalette[pixels[0xD0]];

                if (pixels[0xE0])
                    frameBuffer[currentScreen->pitch * 14] = activePalette[pixels[0xE0]];

                if (pixels[0xF0])
                    frameBuffer[currentScreen->pitch * 15] = activePalette[pixels[0xF0]];

                frameBuffer += currentScreen->pitch * TILE_SIZE;
            }

            lineRemain -= TILE_SIZE;
        }

        while (lineRemain > 0) {
            layout += layer->xsize;

            if (++ty == layer->ysize) {
                ty = 0;
                layout -= layer->ysize << layer->widthShift;
            }

            tileRemain = lineRemain >= TILE_SIZE ? TILE_SIZE : lineRemain;
            if (*layout >= 0xFFFF) {
                frameBuffer += currentScreen->pitch * sheetY;
            }
            else {
                uint8 *pixels = &tilesetPixels[TILE_DATASIZE * (*layout & 0xFFF) + sheetX];
                for (int32 y = 0; y < tileRemain; ++y) {
                    if (*pixels)
                        *frameBuffer = activePalette[*pixels];

                    pixels += TILE_SIZE;
                    frameBuffer += currentScreen->pitch;
                }
            }

            lineRemain -= TILE_SIZE;
        }

        frameBuffer -= currentScreen->pitch * currentScreen->size.y;

        ++scanline;
        ++frameBuffer;
    }
}
void RSDK::DrawLayerRotozoom(TileLayer *layer)
{
    if (!layer->xsize || !layer->ysize)
        return;

    uint16 *layout         = layer->layout;
    uint8 *lineBuffer      = &gfxLineBuffer[currentScreen->clipBound_Y1];
    ScanlineInfo *scanline = &scanlines[currentScreen->clipBound_Y1];
    uint16 *frameBuffer    = &currentScreen->frameBuffer[currentScreen->clipBound_X1 + currentScreen->clipBound_Y1 * currentScreen->pitch];

    int32 width    = (TILE_SIZE << layer->widthShift) - 1;
    int32 height   = (TILE_SIZE << layer->heightShift) - 1;
    int32 lineSize = currentScreen->clipBound_X2 - currentScreen->clipBound_X1;

    for (int32 cy = currentScreen->clipBound_Y1; cy < currentScreen->clipBound_Y2; ++cy) {
        int32 posX = scanline->position.x;
        int32 posY = scanline->position.y;

        uint16 *activePalette = fullPalette[*lineBuffer];
        ++lineBuffer;
        int32 fbOffset = currentScreen->pitch - lineSize;

        for (int32 cx = 0; cx < lineSize; ++cx) {
            int32 tx = posX >> 20;
            int32 ty = posY >> 20;
            int32 x  = (posX >> 16) & 0xF;
            int32 y  = (posY >> 16) & 0xF;

            uint16 tile = layout[((width >> 4) & tx) + (((height >> 4) & ty) << layer->widthShift)] & 0xFFF;
            uint8 idx   = tilesetPixels[TILE_SIZE * (y + TILE_SIZE * tile) + x];

            if (idx)
                *frameBuffer = activePalette[idx];

            posX += scanline->deform.x;
            posY += scanline->deform.y;
            ++frameBuffer;
        }

        frameBuffer += fbOffset;
        ++scanline;
    }
}
void RSDK::DrawLayerBasic(TileLayer *layer)
{
    if (!layer->xsize || !layer->ysize)
        return;

    if (currentScreen->clipBound_X1 >= currentScreen->clipBound_X2 || currentScreen->clipBound_Y1 >= currentScreen->clipBound_Y2)
        return;

    uint16 *activePalette = fullPalette[0];
    if (currentScreen->clipBound_X1 < currentScreen->clipBound_X2 && currentScreen->clipBound_Y1 < currentScreen->clipBound_Y2) {
        int32 lineSize = (currentScreen->clipBound_X2 - currentScreen->clipBound_X1) >> 4;

        ScanlineInfo *scanline = &scanlines[currentScreen->clipBound_Y1];

        int32 tx          = (currentScreen->clipBound_X1 + (scanline->position.x >> 16)) >> 4;
        int32 ty          = (scanline->position.y >> 16) >> 4;
        int32 sheetY      = (scanline->position.y >> 16) & 0xF;
        int32 sheetX      = (currentScreen->clipBound_X1 + (scanline->position.x >> 16)) & 0xF;
        int32 tileRemainX = TILE_SIZE - sheetX;
        int32 tileRemainY = TILE_SIZE - sheetY;

        uint16 *frameBuffer = &currentScreen->frameBuffer[currentScreen->clipBound_X1 + currentScreen->clipBound_Y1 * currentScreen->pitch];
        uint16 *layout      = &layer->layout[tx + (ty << layer->widthShift)];

        // Remaining pixels on top
        {
            if (*layout == 0xFFFF) {
                frameBuffer += TILE_SIZE - sheetX;
            }
            else {
                uint8 *pixels = &tilesetPixels[TILE_DATASIZE * (*layout & 0xFFF) + TILE_SIZE * sheetY + sheetX];

                for (int32 y = 0; y < tileRemainY; ++y) {
                    for (int32 x = 0; x < tileRemainX; ++x) {
                        if (*pixels)
                            *frameBuffer = activePalette[*pixels];
                        ++pixels;
                        ++frameBuffer;
                    }

                    pixels += sheetX;
                    frameBuffer += currentScreen->pitch - tileRemainX;
                }

                frameBuffer += tileRemainX - currentScreen->pitch * tileRemainY;
            }

            ++layout;
            if (++tx == layer->xsize) {
                tx = 0;
                layout -= layer->xsize;
            }

            for (int32 x = 0; x < lineSize; ++x) {
                if (*layout == 0xFFFF) {
                    frameBuffer += TILE_SIZE;
                }
                else {
                    uint8 *pixels = &tilesetPixels[TILE_DATASIZE * (*layout & 0xFFF) + TILE_SIZE * sheetY];
                    for (int32 y = 0; y < tileRemainY; ++y) {
                        uint8 index = *pixels;
                        if (index)
                            *frameBuffer = activePalette[index];

                        index = pixels[1];
                        if (index)
                            frameBuffer[1] = activePalette[index];

                        index = pixels[2];
                        if (index)
                            frameBuffer[2] = activePalette[index];

                        index = pixels[3];
                        if (index)
                            frameBuffer[3] = activePalette[index];

                        index = pixels[4];
                        if (index)
                            frameBuffer[4] = activePalette[index];

                        index = pixels[5];
                        if (index)
                            frameBuffer[5] = activePalette[index];

                        index = pixels[6];
                        if (index)
                            frameBuffer[6] = activePalette[index];

                        index = pixels[7];
                        if (index)
                            frameBuffer[7] = activePalette[index];

                        index = pixels[8];
                        if (index)
                            frameBuffer[8] = activePalette[index];

                        index = pixels[9];
                        if (index)
                            frameBuffer[9] = activePalette[index];

                        index = pixels[10];
                        if (index)
                            frameBuffer[10] = activePalette[index];

                        index = pixels[11];
                        if (index)
                            frameBuffer[11] = activePalette[index];

                        index = pixels[12];
                        if (index)
                            frameBuffer[12] = activePalette[index];

                        index = pixels[13];
                        if (index)
                            frameBuffer[13] = activePalette[index];

                        index = pixels[14];
                        if (index)
                            frameBuffer[14] = activePalette[index];

                        index = pixels[15];
                        if (index)
                            frameBuffer[15] = activePalette[index];

                        frameBuffer += currentScreen->pitch;
                        pixels += TILE_SIZE;
                    }

                    frameBuffer += TILE_SIZE - currentScreen->pitch * tileRemainY;
                }

                ++layout;
                if (++tx == layer->xsize) {
                    tx = 0;
                    layout -= layer->xsize;
                }
            }

            if (*layout == 0xFFFF) {
                frameBuffer += currentScreen->pitch * tileRemainY;
            }
            else {
                uint8 *pixels = &tilesetPixels[TILE_DATASIZE * (*layout & 0xFFF) + TILE_SIZE * sheetY];

                for (int32 y = 0; y < tileRemainY; ++y) {
                    for (int32 x = 0; x < sheetX; ++x) {
                        if (*pixels)
                            *frameBuffer = activePalette[*pixels];
                        ++pixels;
                        ++frameBuffer;
                    }

                    pixels += tileRemainX;
                    frameBuffer += currentScreen->pitch - sheetX;
                }
            }
        }

        // We've drawn a single line of pixels, increase our variables
        frameBuffer += sheetX + -TILE_SIZE * lineSize - TILE_SIZE;
        scanline += tileRemainY;
        if (++ty == layer->ysize)
            ty = 0;

        // Draw the bulk of the tiles
        int32 lineTileCount = ((currentScreen->clipBound_Y2 - currentScreen->clipBound_Y1) >> 4) - 1;
        for (int32 l = 0; l < lineTileCount; ++l) {
            sheetX      = (currentScreen->clipBound_X1 + (scanline->position.x >> 16)) & 0xF;
            tx          = (currentScreen->clipBound_X1 + (scanline->position.x >> 16)) >> 4;
            tileRemainX = TILE_SIZE - sheetX;
            layout      = &layer->layout[tx + (ty << layer->widthShift)];

            // Draw any stray pixels on the left
            if (*layout == 0xFFFF) {
                frameBuffer += tileRemainX;
            }
            else {
                uint8 *pixels = &tilesetPixels[TILE_DATASIZE * (*layout & 0xFFF) + sheetX];

                for (int32 y = 0; y < TILE_SIZE; ++y) {
                    for (int32 x = 0; x < tileRemainX; ++x) {
                        if (*pixels)
                            *frameBuffer = activePalette[*pixels];
                        ++pixels;
                        ++frameBuffer;
                    }

                    pixels += sheetX;
                    frameBuffer += currentScreen->pitch - tileRemainX;
                }

                frameBuffer += tileRemainX - TILE_SIZE * currentScreen->pitch;
            }
            ++layout;
            if (++tx == layer->xsize) {
                tx = 0;
                layout -= layer->xsize;
            }

            // Draw the bulk of the tiles on this line
            for (int32 x = 0; x < lineSize; ++x) {
                if (*layout == 0xFFFF) {
                    frameBuffer += TILE_SIZE;
                }
                else {
                    uint8 *pixels = &tilesetPixels[TILE_DATASIZE * (*layout & 0xFFF)];

                    for (int32 y = 0; y < TILE_SIZE; ++y) {
                        uint8 index = *pixels;
                        if (index)
                            *frameBuffer = activePalette[index];

                        index = pixels[1];
                        if (index)
                            frameBuffer[1] = activePalette[index];

                        index = pixels[2];
                        if (index)
                            frameBuffer[2] = activePalette[index];

                        index = pixels[3];
                        if (index)
                            frameBuffer[3] = activePalette[index];

                        index = pixels[4];
                        if (index)
                            frameBuffer[4] = activePalette[index];

                        index = pixels[5];
                        if (index)
                            frameBuffer[5] = activePalette[index];

                        index = pixels[6];
                        if (index)
                            frameBuffer[6] = activePalette[index];

                        index = pixels[7];
                        if (index)
                            frameBuffer[7] = activePalette[index];

                        index = pixels[8];
                        if (index)
                            frameBuffer[8] = activePalette[index];

                        index = pixels[9];
                        if (index)
                            frameBuffer[9] = activePalette[index];

                        index = pixels[10];
                        if (index)
                            frameBuffer[10] = activePalette[index];

                        index = pixels[11];
                        if (index)
                            frameBuffer[11] = activePalette[index];

                        index = pixels[12];
                        if (index)
                            frameBuffer[12] = activePalette[index];

                        index = pixels[13];
                        if (index)
                            frameBuffer[13] = activePalette[index];

                        index = pixels[14];
                        if (index)
                            frameBuffer[14] = activePalette[index];

                        index = pixels[15];
                        if (index)
                            frameBuffer[15] = activePalette[index];

                        pixels += TILE_SIZE;
                        frameBuffer += currentScreen->pitch;
                    }

                    frameBuffer -= TILE_SIZE * currentScreen->pitch;
                    frameBuffer += TILE_SIZE;
                }

                ++layout;
                if (++tx == layer->xsize) {
                    tx = 0;
                    layout -= layer->xsize;
                }
            }

            // Draw any stray pixels on the right
            if (*layout == 0xFFFF) {
                frameBuffer += TILE_SIZE * currentScreen->pitch;
            }
            else {
                uint8 *pixels = &tilesetPixels[TILE_DATASIZE * (*layout & 0xFFF)];

                for (int32 y = 0; y < TILE_SIZE; ++y) {
                    for (int32 x = 0; x < sheetX; ++x) {
                        if (*pixels)
                            *frameBuffer = activePalette[*pixels];
                        ++pixels;
                        ++frameBuffer;
                    }

                    pixels += tileRemainX;
                    frameBuffer += currentScreen->pitch - sheetX;
                }
            }
            ++layout;
            if (++tx == layer->xsize) {
                tx = 0;
                layout -= layer->xsize;
            }

            // We've drawn a single line, increase our variables
            scanline += TILE_SIZE;
            frameBuffer += sheetX + -TILE_SIZE * lineSize - TILE_SIZE;
            if (++ty == layer->ysize)
                ty = 0;
        }

        // Remaining pixels on bottom
        {
            tx          = (currentScreen->clipBound_X1 + (scanline->position.x >> 16)) >> 4;
            sheetX      = (currentScreen->clipBound_X1 + (scanline->position.x >> 16)) & 0xF;
            tileRemainX = TILE_SIZE - sheetX;
            layout      = &layer->layout[tx + (ty << layer->widthShift)];

            if (*layout != 0xFFFF) {
                frameBuffer += tileRemainX;
            }
            else {
                uint8 *pixels = &tilesetPixels[TILE_DATASIZE * (*layout & 0xFFF) + sheetX];

                for (int32 y = 0; y < sheetY; ++y) {
                    for (int32 x = 0; x < tileRemainX; ++x) {
                        if (*pixels)
                            *frameBuffer = activePalette[*pixels];
                        ++pixels;
                        ++frameBuffer;
                    }

                    pixels += sheetX;
                    frameBuffer += currentScreen->pitch - tileRemainX;
                }

                frameBuffer += tileRemainX - currentScreen->pitch * sheetY;
            }
            ++layout;
            if (++tx == layer->xsize) {
                tx = 0;
                layout -= layer->xsize;
            }

            for (int32 x = 0; x < lineSize; ++x) {
                if (*layout == 0xFFFF) {
                    frameBuffer += TILE_SIZE;
                }
                else {
                    uint8 *pixels = &tilesetPixels[TILE_DATASIZE * (*layout & 0xFFF)];
                    for (int32 y = 0; y < sheetY; ++y) {
                        uint8 index = *pixels;
                        if (index)
                            *frameBuffer = activePalette[index];

                        index = pixels[1];
                        if (index)
                            frameBuffer[1] = activePalette[index];

                        index = pixels[2];
                        if (index)
                            frameBuffer[2] = activePalette[index];

                        index = pixels[3];
                        if (index)
                            frameBuffer[3] = activePalette[index];

                        index = pixels[4];
                        if (index)
                            frameBuffer[4] = activePalette[index];

                        index = pixels[5];
                        if (index)
                            frameBuffer[5] = activePalette[index];

                        index = pixels[6];
                        if (index)
                            frameBuffer[6] = activePalette[index];

                        index = pixels[7];
                        if (index)
                            frameBuffer[7] = activePalette[index];

                        index = pixels[8];
                        if (index)
                            frameBuffer[8] = activePalette[index];

                        index = pixels[9];
                        if (index)
                            frameBuffer[9] = activePalette[index];

                        index = pixels[10];
                        if (index)
                            frameBuffer[10] = activePalette[index];

                        index = pixels[11];
                        if (index)
                            frameBuffer[11] = activePalette[index];

                        index = pixels[12];
                        if (index)
                            frameBuffer[12] = activePalette[index];

                        index = pixels[13];
                        if (index)
                            frameBuffer[13] = activePalette[index];

                        index = pixels[14];
                        if (index)
                            frameBuffer[14] = activePalette[index];

                        index = pixels[15];
                        if (index)
                            frameBuffer[15] = activePalette[index];

                        pixels += TILE_SIZE;
                        frameBuffer += currentScreen->pitch;
                    }

                    frameBuffer += TILE_SIZE - currentScreen->pitch * sheetY;
                }

                ++layout;
                if (++tx == layer->xsize) {
                    tx = 0;
                    layout -= layer->xsize;
                }
            }

            if (*layout != 0xFFFF) {
                uint8 *pixels = &tilesetPixels[256 * (*layout & 0xFFF)];

                for (int32 y = 0; y < sheetY; ++y) {
                    for (int32 x = 0; x < sheetX; ++x) {
                        if (*pixels)
                            *frameBuffer = activePalette[*pixels];
                        ++pixels;
                        ++frameBuffer;
                    }

                    pixels += tileRemainX;
                }

                frameBuffer += currentScreen->pitch - sheetX;
            }
        }
    }
}
