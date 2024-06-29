#include "RSDK/Core/RetroEngine.hpp"

using namespace RSDK;

#if RETRO_REV0U
#include "Legacy/ObjectLegacy.cpp"
#endif

ObjectClass RSDK::objectClassList[OBJECT_COUNT];
int32 RSDK::objectClassCount = 0;

int32 RSDK::globalObjectCount = 0;
int32 RSDK::globalObjectIDs[OBJECT_COUNT];
int32 RSDK::stageObjectIDs[OBJECT_COUNT];

EntityBase RSDK::objectEntityList[ENTITY_COUNT];

EditableVarInfo *RSDK::editableVarList;
int32 RSDK::editableVarCount = 0;

TypeGroupList RSDK::typeGroups[TYPEGROUP_COUNT];

bool32 RSDK::validDraw = false;

ForeachStackInfo RSDK::foreachStackList[FOREACH_STACK_COUNT];
ForeachStackInfo *RSDK::foreachStackPtr = NULL;

#if RETRO_REV0U
#if RETRO_USE_MOD_LOADER
void RSDK::RegisterObject(Object **staticVars, const char *name, uint32 entityClassSize, uint32 staticClassSize, void (*update)(),
                          void (*lateUpdate)(), void (*staticUpdate)(), void (*draw)(), void (*create)(void *), void (*stageLoad)(),
                          void (*editorLoad)(), void (*editorDraw)(), void (*serialize)(), void (*staticLoad)(Object *))
{
    return RegisterObject_STD(staticVars, name, entityClassSize, staticClassSize, update, lateUpdate, staticUpdate, draw, create, stageLoad,
                              editorLoad, editorDraw, serialize, staticLoad);
}

void RSDK::RegisterObject_STD(Object **staticVars, const char *name, uint32 entityClassSize, uint32 staticClassSize, std::function<void()> update,
                              std::function<void()> lateUpdate, std::function<void()> staticUpdate, std::function<void()> draw,
                              std::function<void(void *)> create, std::function<void()> stageLoad, std::function<void()> editorLoad,
                              std::function<void()> editorDraw, std::function<void()> serialize, std::function<void(Object *)> staticLoad)
#else
void RSDK::RegisterObject(Object **staticVars, const char *name, uint32 entityClassSize, uint32 staticClassSize, void (*update)(),
                          void (*lateUpdate)(), void (*staticUpdate)(), void (*draw)(), void (*create)(void *), void (*stageLoad)(),
                          void (*editorLoad)(), void (*editorDraw)(), void (*serialize)(), void (*staticLoad)(Object *))
#endif
#else
#if RETRO_USE_MOD_LOADER
void RSDK::RegisterObject(Object **staticVars, const char *name, uint32 entityClassSize, uint32 staticClassSize, void (*update)(),
                          void (*lateUpdate)(), void (*staticUpdate)(), void (*draw)(), void (*create)(void *), void (*stageLoad)(),
                          void (*editorLoad)(), void (*editorDraw)(), void (*serialize)())
{
    return RegisterObject_STD(staticVars, name, entityClassSize, staticClassSize, update, lateUpdate, staticUpdate, draw, create, stageLoad,
                              editorLoad, editorDraw, serialize);
}

void RSDK::RegisterObject_STD(Object **staticVars, const char *name, uint32 entityClassSize, uint32 staticClassSize, std::function<void()> update,
                              std::function<void()> lateUpdate, std::function<void()> staticUpdate, std::function<void()> draw,
                              std::function<void(void *)> create, std::function<void()> stageLoad, std::function<void()> editorLoad,
                              std::function<void()> editorDraw, std::function<void()> serialize)
#else
void RSDK::RegisterObject(Object **staticVars, const char *name, uint32 entityClassSize, uint32 staticClassSize, void (*update)(),
                          void (*lateUpdate)(), void (*staticUpdate)(), void (*draw)(), void (*create)(void *), void (*stageLoad)(),
                          void (*editorLoad)(), void (*editorDraw)(), void (*serialize)())
#endif
#endif
{
    if (objectClassCount < OBJECT_COUNT) {
        if (entityClassSize > sizeof(EntityBase))
            PrintLog(PRINT_NORMAL, "Class exceeds max entity memory: %s", name);

        ObjectClass *classInfo = &objectClassList[objectClassCount];
        GEN_HASH_MD5(name, classInfo->hash);
        classInfo->staticVars      = staticVars;
        classInfo->entityClassSize = entityClassSize;
        classInfo->staticClassSize = staticClassSize;
        classInfo->update          = update;
        classInfo->lateUpdate      = lateUpdate;
        classInfo->staticUpdate    = staticUpdate;
        classInfo->draw            = draw;
        classInfo->create          = create;
        classInfo->stageLoad       = stageLoad;
        classInfo->editorLoad      = editorLoad;
        classInfo->editorDraw      = editorDraw;
        classInfo->serialize       = serialize;
#if RETRO_REV0U
        classInfo->staticLoad = staticLoad;
#endif

#if !RETRO_USE_ORIGINAL_CODE
        classInfo->name = name;
#endif

        ++objectClassCount;
    }
}

#if RETRO_REV02 || RETRO_USE_MOD_LOADER
void RSDK::RegisterStaticVariables(void **staticVars, const char *name, uint32 classSize)
{
    RETRO_HASH_MD5(hash);
    GEN_HASH_MD5(name, hash);
    AllocateStorage((void **)staticVars, classSize, DATASET_STG, true);
    LoadStaticVariables((uint8 *)*staticVars, hash, 0);
}
#endif

#define ALIGN_TO(type)                                                                                                                               \
    aligned = dataPos & -(int32)sizeof(type);                                                                                                        \
    if (aligned < dataPos)                                                                                                                           \
        dataPos = aligned + sizeof(type);

void RSDK::LoadStaticVariables(uint8 *classPtr, uint32 *hash, int32 readOffset)
{
    char fullFilePath[0x40];

    const char *hexChars = "0123456789ABCDEF";
    char classHash[]     = "00000000000000000000000000000000";

    int32 strPos = 0;
    for (int32 i = 0; i < 32; i += 4) classHash[strPos++] = hexChars[(hash[0] >> i) & 0xF];
    for (int32 i = 0; i < 32; i += 4) classHash[strPos++] = hexChars[(hash[1] >> i) & 0xF];
    for (int32 i = 0; i < 32; i += 4) classHash[strPos++] = hexChars[(hash[2] >> i) & 0xF];
    for (int32 i = 0; i < 32; i += 4) classHash[strPos++] = hexChars[(hash[3] >> i) & 0xF];

    sprintf_s(fullFilePath, sizeof(fullFilePath), "Data/Objects/Static/%s.bin", classHash);

    FileInfo info;
    InitFileInfo(&info);
    if (LoadFile(&info, fullFilePath, FMODE_RB)) {
        uint32 sig = ReadInt32(&info, false);

        if (sig != RSDK_SIGNATURE_OBJ) {
            CloseFile(&info);
            return;
        }

        int32 dataPos = readOffset;
        while (info.readPos < info.fileSize) {
            int32 type      = ReadInt8(&info);
            int32 arraySize = ReadInt32(&info, false);

            bool32 hasValues = (type & 0x80) != 0;
            type &= 0x7F;

            int32 aligned = 0;
            if (hasValues) {
                uint32 count = ReadInt32(&info, false);

                switch (type) {
                    default:
#if !RETRO_USE_ORIGINAL_CODE
                        PrintLog(PRINT_NORMAL, "Invalid static variable type: %d", type);
#endif
                        break;

                    case SVAR_UINT8:
                    case SVAR_INT8:
                        if (info.readPos + (count * sizeof(uint8)) <= info.fileSize && &classPtr[dataPos]) {
                            for (int32 i = 0; i < count * sizeof(uint8); i += sizeof(uint8)) ReadBytes(&info, &classPtr[dataPos + i], sizeof(uint8));
                        }
                        else {
                            info.readPos += count * sizeof(uint8);
                        }

                        dataPos += count * sizeof(uint8);
                        break;

                    case SVAR_UINT16:
                    case SVAR_INT16: {
                        ALIGN_TO(int16);

                        if (info.readPos + (count * sizeof(int16)) <= info.fileSize && &classPtr[dataPos]) {
                            for (int32 i = 0; i < count * sizeof(int16); i += sizeof(int16)) {
#if !RETRO_USE_ORIGINAL_CODE
                                *(int16 *)&classPtr[dataPos + i] = ReadInt16(&info);
#else
                                // This only works as intended on little-endian CPUs.
                                ReadBytes(&info, &classPtr[dataPos + i], sizeof(int16));
#endif
                            }
                        }
                        else {
                            info.readPos += count * sizeof(int16);
                        }

                        dataPos += sizeof(int16) * count;
                        break;
                    }

                    case SVAR_UINT32:
                    case SVAR_INT32: {
                        ALIGN_TO(int32);

                        if (info.readPos + (count * sizeof(int32)) <= info.fileSize && &classPtr[dataPos]) {
                            for (int32 i = 0; i < count * sizeof(int32); i += sizeof(int32)) {
#if !RETRO_USE_ORIGINAL_CODE
                                *(int32 *)&classPtr[dataPos + i] = ReadInt32(&info, false);
#else
                                // This only works as intended on little-endian CPUs.
                                ReadBytes(&info, &classPtr[dataPos + i], sizeof(int32));
#endif
                            }
                        }
                        else {
                            info.readPos += count * sizeof(int32);
                        }

                        dataPos += sizeof(int32) * count;
                        break;
                    }

                    case SVAR_BOOL: {
                        ALIGN_TO(bool32);

                        if (info.readPos + (count * sizeof(bool32)) <= info.fileSize && &classPtr[dataPos]) {
                            for (int32 i = 0; i < count * sizeof(bool32); i += sizeof(bool32)) {
#if !RETRO_USE_ORIGINAL_CODE
                                *(bool32 *)&classPtr[dataPos + i] = (bool32)ReadInt32(&info, false);
#else
                                // This only works as intended on little-endian CPUs.
                                ReadBytes(&info, &classPtr[dataPos + i], sizeof(bool32));
#endif
                            }
                        }
                        else {
                            info.readPos += count * sizeof(bool32);
                        }

                        dataPos += sizeof(bool32) * count;
                        break;
                    }
                }
            }
            else {
                switch (type) {
                    case SVAR_UINT8:
                    case SVAR_INT8: dataPos += sizeof(uint8) * arraySize; break;

                    case SVAR_UINT16:
                    case SVAR_INT16:
                        ALIGN_TO(int16);

                        dataPos += sizeof(int16) * arraySize;
                        break;

                    case SVAR_UINT32:
                    case SVAR_INT32:
                        ALIGN_TO(int32);

                        dataPos += sizeof(int32) * arraySize;
                        break;

                    case SVAR_BOOL:
                        ALIGN_TO(bool32);

                        dataPos += sizeof(bool32) * arraySize;
                        break;

                    case SVAR_POINTER:
                        ALIGN_TO(void *);

                        dataPos += sizeof(void *) * arraySize;
                        break;

                    case SVAR_VECTOR2:
                        ALIGN_TO(int32);

                        dataPos += sizeof(Vector2) * arraySize;
                        break;

                    case SVAR_STRING:
                        ALIGN_TO(void *);

                        dataPos += sizeof(String) * arraySize;
                        break;

                    case SVAR_ANIMATOR:
                        ALIGN_TO(void *);

                        dataPos += sizeof(Animator) * arraySize;
                        break;

                    case SVAR_HITBOX:
                        ALIGN_TO(int16);

                        dataPos += sizeof(Hitbox) * arraySize;
                        break;

                    case SVAR_SPRITEFRAME:
                        ALIGN_TO(int16);

                        dataPos += sizeof(GameSpriteFrame) * arraySize;
                        break;

                    default:
#if !RETRO_USE_ORIGINAL_CODE
                        PrintLog(PRINT_NORMAL, "Invalid data type: %d", type);
#endif
                        break;
                }
            }
        }

        CloseFile(&info);
    }
}

void RSDK::InitObjects()
{
    sceneInfo.entitySlot = 0;
    sceneInfo.createSlot = ENTITY_COUNT - 0x100;
    cameraCount          = 0;

    for (int32 o = 0; o < sceneInfo.classCount; ++o) {
#if RETRO_USE_MOD_LOADER
        currentObjectID = o;
#endif

        if (objectClassList[stageObjectIDs[o]].stageLoad)
            objectClassList[stageObjectIDs[o]].stageLoad();
    }

#if RETRO_USE_MOD_LOADER
    RunModCallbacks(MODCB_ONSTAGELOAD, NULL);
#endif

    for (int32 e = 0; e < ENTITY_COUNT; ++e) {
        sceneInfo.entitySlot = e;
        sceneInfo.entity     = &objectEntityList[e];

        if (sceneInfo.entity->classID) {
            if (objectClassList[stageObjectIDs[sceneInfo.entity->classID]].create) {
                sceneInfo.entity->interaction = true;
                objectClassList[stageObjectIDs[sceneInfo.entity->classID]].create(NULL);
            }
        }
    }

    sceneInfo.state = ENGINESTATE_REGULAR;

    if (!cameraCount)
        AddCamera(&screens[0].position, TO_FIXED(screens[0].center.x), TO_FIXED(screens[0].center.y), false);
}
void RSDK::ProcessObjects()
{
    for (int32 i = 0; i < DRAWGROUP_COUNT; ++i) drawGroups[i].entityCount = 0;

    for (int32 o = 0; o < sceneInfo.classCount; ++o) {
#if RETRO_USE_MOD_LOADER
        currentObjectID = o;
#endif

        ObjectClass *classInfo = &objectClassList[stageObjectIDs[o]];
        if ((*classInfo->staticVars)->active == ACTIVE_ALWAYS || (*classInfo->staticVars)->active == ACTIVE_NORMAL) {
            if (classInfo->staticUpdate)
                classInfo->staticUpdate();
        }
    }

#if RETRO_USE_MOD_LOADER
    RunModCallbacks(MODCB_ONSTATICUPDATE, INT_TO_VOID(ENGINESTATE_REGULAR));
#endif

    for (int32 s = 0; s < cameraCount; ++s) {
        CameraInfo *camera = &cameras[s];

        if (camera->targetPos) {
            if (camera->worldRelative) {
                camera->position.x = camera->targetPos->x;
                camera->position.y = camera->targetPos->y;
            }
            else {
                camera->position.x = TO_FIXED(camera->targetPos->x);
                camera->position.y = TO_FIXED(camera->targetPos->y);
            }
        }
    }

    sceneInfo.entitySlot = 0;
    for (int32 e = 0; e < ENTITY_COUNT; ++e) {
        sceneInfo.entity = &objectEntityList[e];
        if (sceneInfo.entity->classID) {
            switch (sceneInfo.entity->active) {
                default:
                case ACTIVE_DISABLED: break;

                case ACTIVE_NEVER:
                case ACTIVE_PAUSED: sceneInfo.entity->inRange = false; break;

                case ACTIVE_ALWAYS:
                case ACTIVE_NORMAL: sceneInfo.entity->inRange = true; break;

                case ACTIVE_BOUNDS:
                    sceneInfo.entity->inRange = false;

                    for (int32 s = 0; s < cameraCount; ++s) {
                        int32 sx = abs(sceneInfo.entity->position.x - cameras[s].position.x);
                        int32 sy = abs(sceneInfo.entity->position.y - cameras[s].position.y);

                        if (sx <= sceneInfo.entity->updateRange.x + cameras[s].offset.x
                            && sy <= sceneInfo.entity->updateRange.y + cameras[s].offset.y) {
                            sceneInfo.entity->inRange = true;
                            break;
                        }
                    }
                    break;

                case ACTIVE_XBOUNDS:
                    sceneInfo.entity->inRange = false;

                    for (int32 s = 0; s < cameraCount; ++s) {
                        int32 sx = abs(sceneInfo.entity->position.x - cameras[s].position.x);

                        if (sx <= sceneInfo.entity->updateRange.x + cameras[s].offset.x) {
                            sceneInfo.entity->inRange = true;
                            break;
                        }
                    }
                    break;

                case ACTIVE_YBOUNDS:
                    sceneInfo.entity->inRange = false;

                    for (int32 s = 0; s < cameraCount; ++s) {
                        int32 sy = abs(sceneInfo.entity->position.y - cameras[s].position.y);

                        if (sy <= sceneInfo.entity->updateRange.y + cameras[s].offset.y) {
                            sceneInfo.entity->inRange = true;
                            break;
                        }
                    }
                    break;

                case ACTIVE_RBOUNDS:
                    sceneInfo.entity->inRange = false;

                    for (int32 s = 0; s < cameraCount; ++s) {
                        int32 sx = FROM_FIXED(abs(sceneInfo.entity->position.x - cameras[s].position.x));
                        int32 sy = FROM_FIXED(abs(sceneInfo.entity->position.y - cameras[s].position.y));

                        if (sx * sx + sy * sy <= sceneInfo.entity->updateRange.x + cameras[s].offset.x) {
                            sceneInfo.entity->inRange = true;
                            break;
                        }
                    }
                    break;
            }

            if (sceneInfo.entity->inRange) {
                if (objectClassList[stageObjectIDs[sceneInfo.entity->classID]].update)
                    objectClassList[stageObjectIDs[sceneInfo.entity->classID]].update();

                if (sceneInfo.entity->drawGroup < DRAWGROUP_COUNT)
                    drawGroups[sceneInfo.entity->drawGroup].entries[drawGroups[sceneInfo.entity->drawGroup].entityCount++] = sceneInfo.entitySlot;
            }
        }
        else {
            sceneInfo.entity->inRange = false;
        }

        sceneInfo.entitySlot++;
    }

#if RETRO_USE_MOD_LOADER
    RunModCallbacks(MODCB_ONUPDATE, INT_TO_VOID(ENGINESTATE_REGULAR));
#endif

    for (int32 i = 0; i < TYPEGROUP_COUNT; ++i) typeGroups[i].entryCount = 0;

    sceneInfo.entitySlot = 0;
    for (int32 e = 0; e < ENTITY_COUNT; ++e) {
        sceneInfo.entity = &objectEntityList[e];

        if (sceneInfo.entity->inRange && sceneInfo.entity->interaction) {
            typeGroups[GROUP_ALL].entries[typeGroups[GROUP_ALL].entryCount++] = e; // All active objects

            typeGroups[sceneInfo.entity->classID].entries[typeGroups[sceneInfo.entity->classID].entryCount++] = e; // class-based groups

            if (sceneInfo.entity->group >= TYPE_COUNT)
                typeGroups[sceneInfo.entity->group].entries[typeGroups[sceneInfo.entity->group].entryCount++] = e; // extra groups
        }

        sceneInfo.entitySlot++;
    }

    sceneInfo.entitySlot = 0;
    for (int32 e = 0; e < ENTITY_COUNT; ++e) {
        sceneInfo.entity = &objectEntityList[e];

        if (sceneInfo.entity->inRange) {
            if (objectClassList[stageObjectIDs[sceneInfo.entity->classID]].lateUpdate)
                objectClassList[stageObjectIDs[sceneInfo.entity->classID]].lateUpdate();
        }

        sceneInfo.entity->onScreen = 0;
        sceneInfo.entitySlot++;
    }

#if RETRO_USE_MOD_LOADER
    RunModCallbacks(MODCB_ONLATEUPDATE, INT_TO_VOID(ENGINESTATE_REGULAR));
#endif
}
void RSDK::ProcessPausedObjects()
{
    for (int32 i = 0; i < DRAWGROUP_COUNT; ++i) drawGroups[i].entityCount = 0;

    for (int32 o = 0; o < sceneInfo.classCount; ++o) {
#if RETRO_USE_MOD_LOADER
        currentObjectID = o;
#endif

        ObjectClass *classInfo = &objectClassList[stageObjectIDs[o]];
        if ((*classInfo->staticVars)->active == ACTIVE_ALWAYS || (*classInfo->staticVars)->active == ACTIVE_PAUSED) {
            if (classInfo->staticUpdate)
                classInfo->staticUpdate();
        }
    }

#if RETRO_USE_MOD_LOADER
    RunModCallbacks(MODCB_ONSTATICUPDATE, INT_TO_VOID(ENGINESTATE_PAUSED));
#endif

    sceneInfo.entitySlot = 0;
    for (int32 e = 0; e < ENTITY_COUNT; ++e) {
        sceneInfo.entity = &objectEntityList[e];

        if (sceneInfo.entity->classID) {
            if (sceneInfo.entity->active == ACTIVE_ALWAYS || sceneInfo.entity->active == ACTIVE_PAUSED) {
                if (objectClassList[stageObjectIDs[sceneInfo.entity->classID]].update)
                    objectClassList[stageObjectIDs[sceneInfo.entity->classID]].update();

                if (sceneInfo.entity->drawGroup < DRAWGROUP_COUNT)
                    drawGroups[sceneInfo.entity->drawGroup].entries[drawGroups[sceneInfo.entity->drawGroup].entityCount++] = sceneInfo.entitySlot;
            }
        }
        else {
            sceneInfo.entity->inRange = false;
        }

        sceneInfo.entitySlot++;
    }

#if RETRO_USE_MOD_LOADER
    RunModCallbacks(MODCB_ONUPDATE, INT_TO_VOID(ENGINESTATE_PAUSED));
#endif

    sceneInfo.entitySlot = 0;
    for (int32 e = 0; e < ENTITY_COUNT; ++e) {
        sceneInfo.entity = &objectEntityList[e];

        if (sceneInfo.entity->active == ACTIVE_ALWAYS || sceneInfo.entity->active == ACTIVE_PAUSED) {
            if (objectClassList[stageObjectIDs[sceneInfo.entity->classID]].lateUpdate)
                objectClassList[stageObjectIDs[sceneInfo.entity->classID]].lateUpdate();
        }

        sceneInfo.entity->onScreen = 0;
        sceneInfo.entitySlot++;
    }

#if RETRO_USE_MOD_LOADER
    RunModCallbacks(MODCB_ONLATEUPDATE, INT_TO_VOID(ENGINESTATE_PAUSED));
#endif
}
void RSDK::ProcessFrozenObjects()
{
    for (int32 i = 0; i < DRAWGROUP_COUNT; ++i) drawGroups[i].entityCount = 0;

    for (int32 o = 0; o < sceneInfo.classCount; ++o) {
#if RETRO_USE_MOD_LOADER
        currentObjectID = o;
#endif

        ObjectClass *classInfo = &objectClassList[stageObjectIDs[o]];
        if ((*classInfo->staticVars)->active == ACTIVE_ALWAYS || (*classInfo->staticVars)->active == ACTIVE_PAUSED) {
            if (classInfo->staticUpdate)
                classInfo->staticUpdate();
        }
    }

#if RETRO_USE_MOD_LOADER
    RunModCallbacks(MODCB_ONSTATICUPDATE, INT_TO_VOID(ENGINESTATE_FROZEN));
#endif

    for (int32 s = 0; s < cameraCount; ++s) {
        CameraInfo *camera = &cameras[s];

        if (camera->targetPos) {
            if (camera->worldRelative) {
                camera->position.x = camera->targetPos->x;
                camera->position.y = camera->targetPos->y;
            }
            else {
                camera->position.x = TO_FIXED(camera->targetPos->x);
                camera->position.y = TO_FIXED(camera->targetPos->y);
            }
        }
    }

    sceneInfo.entitySlot = 0;
    for (int32 e = 0; e < ENTITY_COUNT; ++e) {
        sceneInfo.entity = &objectEntityList[e];

        if (sceneInfo.entity->classID) {
            switch (sceneInfo.entity->active) {
                default:
                case ACTIVE_DISABLED: break;

                case ACTIVE_NEVER:
                case ACTIVE_PAUSED: sceneInfo.entity->inRange = false; break;

                case ACTIVE_ALWAYS:
                case ACTIVE_NORMAL: sceneInfo.entity->inRange = true; break;

                case ACTIVE_BOUNDS:
                    sceneInfo.entity->inRange = false;

                    for (int32 s = 0; s < cameraCount; ++s) {
                        int32 sx = abs(sceneInfo.entity->position.x - cameras[s].position.x);
                        int32 sy = abs(sceneInfo.entity->position.y - cameras[s].position.y);

                        if (sx <= sceneInfo.entity->updateRange.x + cameras[s].offset.x
                            && sy <= sceneInfo.entity->updateRange.y + cameras[s].offset.y) {
                            sceneInfo.entity->inRange = true;
                            break;
                        }
                    }
                    break;

                case ACTIVE_XBOUNDS:
                    sceneInfo.entity->inRange = false;

                    for (int32 s = 0; s < cameraCount; ++s) {
                        int32 sx = abs(sceneInfo.entity->position.x - cameras[s].position.x);

                        if (sx <= sceneInfo.entity->updateRange.x + cameras[s].offset.x) {
                            sceneInfo.entity->inRange = true;
                            break;
                        }
                    }
                    break;

                case ACTIVE_YBOUNDS:
                    sceneInfo.entity->inRange = false;

                    for (int32 s = 0; s < cameraCount; ++s) {
                        int32 sy = abs(sceneInfo.entity->position.y - cameras[s].position.y);

                        if (sy <= sceneInfo.entity->updateRange.y + cameras[s].offset.y) {
                            sceneInfo.entity->inRange = true;
                            break;
                        }
                    }
                    break;

                case ACTIVE_RBOUNDS:
                    sceneInfo.entity->inRange = false;

                    for (int32 s = 0; s < cameraCount; ++s) {
                        int32 sx = FROM_FIXED(abs(sceneInfo.entity->position.x - cameras[s].position.x));
                        int32 sy = FROM_FIXED(abs(sceneInfo.entity->position.y - cameras[s].position.y));

                        if (sx * sx + sy * sy <= sceneInfo.entity->updateRange.x + cameras[s].offset.x) {
                            sceneInfo.entity->inRange = true;
                            break;
                        }
                    }
                    break;
            }

            if (sceneInfo.entity->inRange) {
                if (sceneInfo.entity->active == ACTIVE_ALWAYS || sceneInfo.entity->active == ACTIVE_PAUSED) {
                    if (objectClassList[stageObjectIDs[sceneInfo.entity->classID]].update)
                        objectClassList[stageObjectIDs[sceneInfo.entity->classID]].update();
                }

                if (sceneInfo.entity->drawGroup < DRAWGROUP_COUNT)
                    drawGroups[sceneInfo.entity->drawGroup].entries[drawGroups[sceneInfo.entity->drawGroup].entityCount++] = sceneInfo.entitySlot;
            }
        }
        else {
            sceneInfo.entity->inRange = false;
        }

        sceneInfo.entitySlot++;
    }

#if RETRO_USE_MOD_LOADER
    RunModCallbacks(MODCB_ONUPDATE, INT_TO_VOID(ENGINESTATE_FROZEN));
#endif

    for (int32 i = 0; i < TYPEGROUP_COUNT; ++i) typeGroups[i].entryCount = 0;

    sceneInfo.entitySlot = 0;
    for (int32 e = 0; e < ENTITY_COUNT; ++e) {
        sceneInfo.entity = &objectEntityList[e];

        if (sceneInfo.entity->inRange) {
            if (sceneInfo.entity->active == ACTIVE_ALWAYS || sceneInfo.entity->active == ACTIVE_PAUSED) {
                if (objectClassList[stageObjectIDs[sceneInfo.entity->classID]].lateUpdate)
                    objectClassList[stageObjectIDs[sceneInfo.entity->classID]].lateUpdate();
            }

            if (sceneInfo.entity->interaction) {
                typeGroups[GROUP_ALL].entries[typeGroups[GROUP_ALL].entryCount++] = e; // All active entities

                typeGroups[sceneInfo.entity->classID].entries[typeGroups[sceneInfo.entity->classID].entryCount++] = e; // type-based groups

                if (sceneInfo.entity->group >= TYPE_COUNT)
                    typeGroups[sceneInfo.entity->group].entries[typeGroups[sceneInfo.entity->group].entryCount++] = e; // extra groups
            }
        }

        sceneInfo.entity->onScreen = 0;
        sceneInfo.entitySlot++;
    }

#if RETRO_USE_MOD_LOADER
    RunModCallbacks(MODCB_ONLATEUPDATE, INT_TO_VOID(ENGINESTATE_FROZEN));
#endif
}
void RSDK::ProcessObjectDrawLists()
{
    if (sceneInfo.state != ENGINESTATE_LOAD && sceneInfo.state != (ENGINESTATE_LOAD | ENGINESTATE_STEPOVER)) {
        for (int32 s = 0; s < videoSettings.screenCount; ++s) {
            currentScreen             = &screens[s];
            sceneInfo.currentScreenID = s;

            for (int32 l = 0; l < DRAWGROUP_COUNT; ++l) drawGroups[l].layerCount = 0;

            for (int32 t = 0; t < LAYER_COUNT; ++t) {
                uint8 drawGroup = tileLayers[t].drawGroup[s];

                if (drawGroup < DRAWGROUP_COUNT)
                    drawGroups[drawGroup].layerDrawList[drawGroups[drawGroup].layerCount++] = t;
            }

            sceneInfo.currentDrawGroup = 0;
            for (int32 l = 0; l < DRAWGROUP_COUNT; ++l) {
                if (engine.drawGroupVisible[l]) {
                    DrawList *list = &drawGroups[l];

                    if (list->hookCB)
                        list->hookCB();

                    if (list->sorted) {
                        for (int32 e = 0; e < list->entityCount; ++e) {
                            for (int32 i = list->entityCount - 1; i > e; --i) {
                                int32 slot1 = list->entries[i - 1];
                                int32 slot2 = list->entries[i];
                                if (objectEntityList[slot2].zdepth > objectEntityList[slot1].zdepth) {
                                    list->entries[i - 1] = slot2;
                                    list->entries[i]     = slot1;
                                }
                            }
                        }
                    }

                    for (int32 i = 0; i < list->entityCount; ++i) {
                        sceneInfo.entitySlot = list->entries[i];
                        validDraw            = false;
                        sceneInfo.entity     = &objectEntityList[list->entries[i]];
                        if (sceneInfo.entity->visible) {
                            if (objectClassList[stageObjectIDs[sceneInfo.entity->classID]].draw)
                                objectClassList[stageObjectIDs[sceneInfo.entity->classID]].draw();

#if RETRO_VER_EGS || RETRO_USE_DUMMY_ACHIEVEMENTS
                            if (i == list->entityCount - 1)
                                SKU::DrawAchievements();
#endif

                            sceneInfo.entity->onScreen |= validDraw << sceneInfo.currentScreenID;
                        }
                    }

                    for (int32 i = 0; i < list->layerCount; ++i) {
                        TileLayer *layer = &tileLayers[list->layerDrawList[i]];

#if RETRO_USE_MOD_LOADER
                        RunModCallbacks(MODCB_ONSCANLINECB, (void *)layer->scanlineCallback);
#endif
                        if (layer->scanlineCallback)
                            layer->scanlineCallback(scanlines);
                        else
                            ProcessParallax(layer);

                        switch (layer->type) {
                            case LAYER_HSCROLL: DrawLayerHScroll(layer); break;
                            case LAYER_VSCROLL: DrawLayerVScroll(layer); break;
                            case LAYER_ROTOZOOM: DrawLayerRotozoom(layer); break;
                            case LAYER_BASIC: DrawLayerBasic(layer); break;
                            default: break;
                        }
                    }

#if RETRO_USE_MOD_LOADER
                    RunModCallbacks(MODCB_ONDRAW, INT_TO_VOID(l));
#endif

                    if (currentScreen->clipBound_X1 > 0)
                        currentScreen->clipBound_X1 = 0;

                    if (currentScreen->clipBound_Y1 > 0)
                        currentScreen->clipBound_Y1 = 0;

                    if (currentScreen->size.x >= 0) {
                        if (currentScreen->clipBound_X2 < currentScreen->size.x)
                            currentScreen->clipBound_X2 = currentScreen->size.x;
                    }
                    else {
                        currentScreen->clipBound_X2 = 0;
                    }

                    if (currentScreen->size.y >= 0) {
                        if (currentScreen->clipBound_Y2 < currentScreen->size.y)
                            currentScreen->clipBound_Y2 = currentScreen->size.y;
                    }
                    else {
                        currentScreen->clipBound_Y2 = 0;
                    }
                }

                sceneInfo.currentDrawGroup++;
            }

#if !RETRO_USE_ORIGINAL_CODE
            if (engine.showUpdateRanges) {
                for (int32 l = 0; l < DRAWGROUP_COUNT; ++l) {
                    if (engine.drawGroupVisible[l]) {
                        DrawList *list = &drawGroups[l];
                        for (int32 i = 0; i < list->entityCount; ++i) {
                            Entity *entity     = &objectEntityList[list->entries[i]];

                            if (entity->visible || (engine.showUpdateRanges & 2)) {
                                switch (entity->active) {
                                    default:
                                    case ACTIVE_DISABLED:
                                    case ACTIVE_NEVER: break;

                                    case ACTIVE_ALWAYS:
                                    case ACTIVE_NORMAL: 
                                    case ACTIVE_PAUSED:
                                        DrawRectangle(entity->position.x, entity->position.y, TO_FIXED(1), TO_FIXED(1), 0x0000FF, 0xFF, INK_NONE,
                                                      false);
                                        break;

                                    case ACTIVE_BOUNDS:
                                        DrawLine(entity->position.x - entity->updateRange.x, entity->position.y - entity->updateRange.y,
                                                 entity->position.x + entity->updateRange.x, entity->position.y - entity->updateRange.y, 0x0000FF,
                                                 0xFF, INK_NONE, false);

                                        DrawLine(entity->position.x - entity->updateRange.x, entity->position.y + entity->updateRange.y,
                                                 entity->position.x + entity->updateRange.x, entity->position.y + entity->updateRange.y, 0x0000FF,
                                                 0xFF, INK_NONE, false);

                                        DrawLine(entity->position.x - entity->updateRange.x, entity->position.y - entity->updateRange.y,
                                                 entity->position.x - entity->updateRange.x, entity->position.y + entity->updateRange.y, 0x0000FF,
                                                 0xFF, INK_NONE, false);

                                        DrawLine(entity->position.x + entity->updateRange.x, entity->position.y - entity->updateRange.y,
                                                 entity->position.x + entity->updateRange.x, entity->position.y + entity->updateRange.y, 0x0000FF,
                                                 0xFF, INK_NONE, false);
                                        break;

                                    case ACTIVE_XBOUNDS:
                                        DrawLine(entity->position.x - entity->updateRange.x, TO_FIXED(currentScreen->position.y),
                                                 entity->position.x - entity->updateRange.x,
                                                 TO_FIXED(currentScreen->position.y + currentScreen->size.y), 0x0000FF, 0xFF, INK_NONE, false);

                                        DrawLine(entity->position.x + entity->updateRange.x, TO_FIXED(currentScreen->position.y),
                                                 entity->position.x + entity->updateRange.x,
                                                 TO_FIXED(currentScreen->position.y + currentScreen->size.y), 0x0000FF, 0xFF, INK_NONE, false);
                                        break;

                                    case ACTIVE_YBOUNDS:
                                        DrawLine(TO_FIXED(currentScreen->position.x), entity->position.y - entity->updateRange.y,
                                                 TO_FIXED(currentScreen->position.x + currentScreen->size.x),
                                                 entity->position.y - entity->updateRange.y, 0x0000FF, 0xFF, INK_NONE, false);

                                        DrawLine(TO_FIXED(currentScreen->position.x), entity->position.y + entity->updateRange.y,
                                                 TO_FIXED(currentScreen->position.x + currentScreen->size.x),
                                                 entity->position.y + entity->updateRange.y, 0x0000FF, 0xFF, INK_NONE, false);
                                        break;

                                    case ACTIVE_RBOUNDS:
                                        DrawCircleOutline(entity->position.x, entity->position.y, FROM_FIXED(entity->updateRange.x),
                                                          FROM_FIXED(entity->updateRange.x) + 1, 0x0000FF, 0xFF, INK_NONE, false);
                                        break;
                                }
                            }
                        }
                    }
                }
            }

            if (engine.showEntityInfo) {
                for (int32 l = 0; l < DRAWGROUP_COUNT; ++l) {
                    if (engine.drawGroupVisible[l]) {
                        DrawList *list = &drawGroups[l];
                        for (int32 i = 0; i < list->entityCount; ++i) {
                            Entity *entity = &objectEntityList[list->entries[i]];

                            if (entity->visible || (engine.showEntityInfo & 2)) {
                                char buffer[0x100];
                                sprintf_s(buffer, sizeof(buffer), "%s\nx: %g\ny: %g", objectClassList[stageObjectIDs[entity->classID]].name,
                                          entity->position.x / 65536.0f, entity->position.y / 65536.0f);

                                DrawDevString(buffer, FROM_FIXED(entity->position.x) - currentScreen->position.x,
                                              FROM_FIXED(entity->position.y) - currentScreen->position.y, ALIGN_LEFT, 0xF0F0F0);
                            }
                        }
                    }
                }
            }

            if (showHitboxes) {
                for (int32 i = 0; i < debugHitboxCount; ++i) {
                    DebugHitboxInfo *info = &debugHitboxList[i];
                    int32 x               = info->pos.x + TO_FIXED(info->hitbox.left);
                    int32 y               = info->pos.y + TO_FIXED(info->hitbox.top);
                    int32 w               = abs((info->pos.x + TO_FIXED(info->hitbox.right)) - x);
                    int32 h               = abs((info->pos.y + TO_FIXED(info->hitbox.bottom)) - y);

                    switch (info->type) {
                        case H_TYPE_TOUCH: DrawRectangle(x, y, w, h, info->collision ? 0x808000 : 0xFF0000, 0x60, INK_ALPHA, false); break;

                        case H_TYPE_CIRCLE:
                            DrawCircle(info->pos.x, info->pos.y, info->hitbox.left, info->collision ? 0x808000 : 0xFF0000, 0x60, INK_ALPHA, false);
                            break;

                        case H_TYPE_BOX:
                            DrawRectangle(x, y, w, h, 0x0000FF, 0x60, INK_ALPHA, false);

                            if (info->collision & 1) // top
                                DrawRectangle(x, y, w, TO_FIXED(1), 0xFFFF00, 0xC0, INK_ALPHA, false);

                            if (info->collision & 8) // bottom
                                DrawRectangle(x, y + h, w, TO_FIXED(1), 0xFFFF00, 0xC0, INK_ALPHA, false);

                            if (info->collision & 2) { // left
                                int32 sy = y;
                                int32 sh = h;

                                if (info->collision & 1) {
                                    sy += TO_FIXED(1);
                                    sh -= TO_FIXED(1);
                                }

                                if (info->collision & 8)
                                    sh -= TO_FIXED(1);

                                DrawRectangle(x, sy, TO_FIXED(1), sh, 0xFFFF00, 0xC0, INK_ALPHA, false);
                            }

                            if (info->collision & 4) { // right
                                int32 sy = y;
                                int32 sh = h;

                                if (info->collision & 1) {
                                    sy += TO_FIXED(1);
                                    sh -= TO_FIXED(1);
                                }

                                if (info->collision & 8)
                                    sh -= TO_FIXED(1);

                                DrawRectangle(x + w, sy, TO_FIXED(1), sh, 0xFFFF00, 0xC0, INK_ALPHA, false);
                            }
                            break;

                        case H_TYPE_PLAT:
                            DrawRectangle(x, y, w, h, 0x00FF00, 0x60, INK_ALPHA, false);

                            if (info->collision & 1) // top
                                DrawRectangle(x, y, w, TO_FIXED(1), 0xFFFF00, 0xC0, INK_ALPHA, false);

                            if (info->collision & 8) // bottom
                                DrawRectangle(x, y + h, w, TO_FIXED(1), 0xFFFF00, 0xC0, INK_ALPHA, false);
                            break;
                    }
                }
            }

            if (engine.showPaletteOverlay) {
                for (int32 p = 0; p < PALETTE_BANK_COUNT; ++p) {
                    int32 x = (videoSettings.pixWidth - (0x10 << 3));
                    int32 y = (SCREEN_YSIZE - (0x10 << 2));

                    for (int32 c = 0; c < PALETTE_BANK_SIZE; ++c) {
                        uint32 clr = GetPaletteEntry(p, c);

                        DrawRectangle(x + ((c & 0xF) << 1) + ((p % (PALETTE_BANK_COUNT / 2)) * (2 * 16)),
                                      y + ((c >> 4) << 1) + ((p / (PALETTE_BANK_COUNT / 2)) * (2 * 16)), 2, 2, clr, 0xFF, INK_NONE, true);
                    }
                }
            }

#endif

            currentScreen++;
            sceneInfo.currentScreenID++;
        }
    }
}

uint16 RSDK::FindObject(const char *name)
{
    RETRO_HASH_MD5(hash);
    GEN_HASH_MD5(name, hash);

    for (int32 o = 0; o < sceneInfo.classCount; ++o) {
        if (HASH_MATCH_MD5(hash, objectClassList[stageObjectIDs[o]].hash))
            return o;
    }

    return TYPE_DEFAULTOBJECT;
}

int32 RSDK::GetEntityCount(uint16 classID, bool32 isActive)
{
    if (classID >= TYPE_COUNT)
        return 0;
    if (isActive)
        return typeGroups[classID].entryCount;

    int32 entityCount = 0;
    for (int32 i = 0; i < ENTITY_COUNT; ++i) {
        if (objectEntityList[i].classID == classID)
            entityCount++;
    }

    return entityCount;
}

void RSDK::ResetEntity(Entity *entity, uint16 classID, void *data)
{
    if (entity) {
        ObjectClass *info = &objectClassList[stageObjectIDs[classID]];
        memset(entity, 0, info->entityClassSize);

        if (info->create) {
            Entity *curEnt = sceneInfo.entity;

            sceneInfo.entity              = entity;
            sceneInfo.entity->interaction = true;
#if RETRO_USE_MOD_LOADER
            int32 superStore          = superLevels[inheritLevel];
            superLevels[inheritLevel] = 0;
#endif
            info->create(data);
#if RETRO_USE_MOD_LOADER
            superLevels[inheritLevel] = superStore;
#endif
            sceneInfo.entity->classID = classID;

            sceneInfo.entity = curEnt;
        }

        entity->classID = classID;
    }
}

void RSDK::ResetEntitySlot(uint16 slot, uint16 classID, void *data)
{
    ObjectClass *object = &objectClassList[stageObjectIDs[classID]];
    slot                = slot < ENTITY_COUNT ? slot : (ENTITY_COUNT - 1);

    Entity *entity = &objectEntityList[slot];
    memset(&objectEntityList[slot], 0, object->entityClassSize);

    if (object->create) {
        Entity *curEnt = sceneInfo.entity;

        sceneInfo.entity    = entity;
        entity->interaction = true;
#if RETRO_USE_MOD_LOADER
        int32 superStore          = superLevels[inheritLevel];
        superLevels[inheritLevel] = 0;
#endif
        object->create(data);
#if RETRO_USE_MOD_LOADER
        superLevels[inheritLevel] = superStore;
#endif
        entity->classID = classID;

        sceneInfo.entity = curEnt;
    }
    else {
        entity->classID = classID;
    }
}

Entity *RSDK::CreateEntity(uint16 classID, void *data, int32 x, int32 y)
{
    ObjectClass *object = &objectClassList[stageObjectIDs[classID]];
    Entity *entity      = &objectEntityList[sceneInfo.createSlot];

    int32 permCnt = 0, loopCnt = 0;
    while (entity->classID) {
        // after 16 loops, the game says fuck it and will start overwriting non-temp objects
        if (!entity->isPermanent && loopCnt >= 16)
            break;

        if (entity->isPermanent)
            ++permCnt;

        sceneInfo.createSlot++;
        if (sceneInfo.createSlot == ENTITY_COUNT) {
            sceneInfo.createSlot = TEMPENTITY_START;
            entity               = &objectEntityList[sceneInfo.createSlot];
        }
        else {
            entity = &objectEntityList[sceneInfo.createSlot];
        }

        if (permCnt >= TEMPENTITY_COUNT)
            break;

        ++loopCnt;
    }

    memset(entity, 0, object->entityClassSize);
    entity->position.x  = x;
    entity->position.y  = y;
    entity->interaction = true;

    if (object->create) {
        Entity *curEnt = sceneInfo.entity;

        sceneInfo.entity = entity;
#if RETRO_USE_MOD_LOADER
        int32 superStore          = superLevels[inheritLevel];
        superLevels[inheritLevel] = 0;
#endif
        object->create(data);
#if RETRO_USE_MOD_LOADER
        superLevels[inheritLevel] = superStore;
#endif
        entity->classID = classID;

        sceneInfo.entity = curEnt;
    }
    else {
        entity->classID = classID;
        entity->active  = ACTIVE_NORMAL;
        entity->visible = true;
    }

    return entity;
}

bool32 RSDK::GetActiveEntities(uint16 group, Entity **entity)
{
    if (group >= TYPEGROUP_COUNT)
        return false;

    if (!entity)
        return false;

    if (*entity) {
        ++foreachStackPtr->id;
    }
    else {
        foreachStackPtr++;
        foreachStackPtr->id = 0;
    }

    for (Entity *nextEntity = &objectEntityList[typeGroups[group].entries[foreachStackPtr->id]]; foreachStackPtr->id < typeGroups[group].entryCount;
         ++foreachStackPtr->id, nextEntity = &objectEntityList[typeGroups[group].entries[foreachStackPtr->id]]) {
        if (nextEntity->classID == group) {
            *entity = nextEntity;
            return true;
        }
    }

    foreachStackPtr--;

    return false;
}
bool32 RSDK::GetAllEntities(uint16 classID, Entity **entity)
{
    if (classID >= OBJECT_COUNT)
        return false;

    if (!entity)
        return false;

    if (*entity) {
        ++foreachStackPtr->id;
    }
    else {
        foreachStackPtr++;
        foreachStackPtr->id = 0;
    }

    for (; foreachStackPtr->id < ENTITY_COUNT; ++foreachStackPtr->id) {
        Entity *nextEntity = &objectEntityList[foreachStackPtr->id];
        if (nextEntity->classID == classID) {
            *entity = nextEntity;
            return true;
        }
    }

    foreachStackPtr--;

    return false;
}

bool32 RSDK::CheckOnScreen(Entity *entity, Vector2 *range)
{
    if (!entity)
        return false;

    if (!range)
        range = &entity->updateRange;

    return CheckPosOnScreen(&entity->position, range);
}
bool32 RSDK::CheckPosOnScreen(Vector2 *position, Vector2 *range)
{
    if (!position || !range)
        return false;

    for (int32 s = 0; s < cameraCount; ++s) {
        int32 sx = abs(position->x - cameras[s].position.x);
        int32 sy = abs(position->y - cameras[s].position.y);

        if (sx <= range->x + cameras[s].offset.x && sy <= range->y + cameras[s].offset.y)
            return true;
    }

    return false;
}

void RSDK::ClearStageObjects()
{
    // Unload static object classes
    for (int32 o = 0; o < sceneInfo.classCount; ++o) {
        if (objectClassList[stageObjectIDs[o]].staticVars) {
            *objectClassList[stageObjectIDs[o]].staticVars = NULL;
        }
    }
}
