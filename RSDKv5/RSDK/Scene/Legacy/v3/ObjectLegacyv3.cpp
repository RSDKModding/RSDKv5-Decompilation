
int32 RSDK::Legacy::v3::objectLoop    = 0;
int32 RSDK::Legacy::v3::curObjectType = 0;
RSDK::Legacy::v3::Entity RSDK::Legacy::v3::objectEntityList[LEGACY_v3_ENTITY_COUNT];

char RSDK::Legacy::v3::typeNames[LEGACY_v3_OBJECT_COUNT][0x40];

void RSDK::Legacy::v3::SetObjectTypeName(const char *objectName, int32 objectID)
{
    int32 objNameID  = 0;
    int32 typeNameID = 0;
    while (objectName[objNameID]) {
        if (objectName[objNameID] != ' ')
            typeNames[objectID][typeNameID++] = objectName[objNameID];
        ++objNameID;
    }
    typeNames[objectID][typeNameID] = 0;
    PrintLog(PRINT_NORMAL, "Set Object (%d) name to: %s", objectID, objectName);
}

void RSDK::Legacy::v3::ProcessStartupObjects()
{
    scriptFrameCount = 0;
    ClearAnimationData();
    activePlayer               = 0;
    activePlayerCount          = 1;
    scriptEng.arrayPosition[2] = LEGACY_v3_TEMPENTITY_START;

    Entity *entity = &objectEntityList[LEGACY_v3_TEMPENTITY_START];
    for (int32 i = 0; i < LEGACY_v3_OBJECT_COUNT; ++i) {
        ObjectScript *scriptInfo    = &objectScriptList[i];
        objectLoop                  = LEGACY_v3_TEMPENTITY_START;
        curObjectType               = i;
        scriptInfo->frameListOffset = scriptFrameCount;
        scriptInfo->spriteSheetID   = 0;
        entity->type                = i;
        if (scriptCode[scriptInfo->subStartup.scriptCodePtr] > 0)
            ProcessScript(scriptInfo->subStartup.scriptCodePtr, scriptInfo->subStartup.jumpTablePtr, SUB_SETUP);
        scriptInfo->frameCount = scriptFrameCount - scriptInfo->frameListOffset;
    }
    entity->type  = 0;
    curObjectType = 0;
}

void RSDK::Legacy::v3::ProcessObjects()
{
    for (int32 i = 0; i < LEGACY_DRAWLAYER_COUNT; ++i) drawListEntries[i].listSize = 0;

    for (objectLoop = 0; objectLoop < LEGACY_v3_ENTITY_COUNT; ++objectLoop) {
        bool active = false;
        int32 x = 0, y = 0;
        Entity *entity = &objectEntityList[objectLoop];
        switch (entity->priority) {
            case PRIORITY_ACTIVE_BOUNDS:
                x      = entity->XPos >> 16;
                y      = entity->YPos >> 16;
                active = x > xScrollOffset - OBJECT_BORDER_X1 && x < OBJECT_BORDER_X2 + xScrollOffset && y > yScrollOffset - OBJECT_BORDER_Y1
                         && y < yScrollOffset + OBJECT_BORDER_Y2;
                break;

            case PRIORITY_ACTIVE:
            case PRIORITY_ACTIVE_PAUSED: active = true; break;

            case PRIORITY_ACTIVE_XBOUNDS:
                x      = entity->XPos >> 16;
                active = x > xScrollOffset - OBJECT_BORDER_X1 && x < OBJECT_BORDER_X2 + xScrollOffset;
                break;

            case PRIORITY_ACTIVE_BOUNDS_REMOVE:
                x = entity->XPos >> 16;
                y = entity->YPos >> 16;
                if (x <= xScrollOffset - OBJECT_BORDER_X1 || x >= OBJECT_BORDER_X2 + xScrollOffset || y <= yScrollOffset - OBJECT_BORDER_Y1
                    || y >= yScrollOffset + OBJECT_BORDER_Y2) {
                    active       = false;
                    entity->type = OBJ_TYPE_BLANKOBJECT;
                }
                else {
                    active = true;
                }
                break;

            case PRIORITY_INACTIVE: active = false; break;

            default: break;
        }

        if (active && entity->type > OBJ_TYPE_BLANKOBJECT) {
            ObjectScript *scriptInfo = &objectScriptList[entity->type];
            activePlayer             = 0;
            if (scriptCode[scriptInfo->subMain.scriptCodePtr] > 0)
                ProcessScript(scriptInfo->subMain.scriptCodePtr, scriptInfo->subMain.jumpTablePtr, SUB_MAIN);
            if (scriptCode[scriptInfo->subPlayerInteraction.scriptCodePtr] > 0) {
                while (activePlayer < activePlayerCount) {
                    if (playerList[activePlayer].objectInteractions)
                        ProcessScript(scriptInfo->subPlayerInteraction.scriptCodePtr, scriptInfo->subPlayerInteraction.jumpTablePtr,
                                      SUB_PLAYERINTERACTION);
                    ++activePlayer;
                }
            }

            if (entity->drawOrder < LEGACY_DRAWLAYER_COUNT)
                drawListEntries[entity->drawOrder].entityRefs[drawListEntries[entity->drawOrder].listSize++] = objectLoop;
        }
    }
}

void RSDK::Legacy::v3::ProcessPausedObjects()
{
    for (int32 i = 0; i < LEGACY_DRAWLAYER_COUNT; ++i) drawListEntries[i].listSize = 0;

    for (objectLoop = 0; objectLoop < ENTITY_COUNT; ++objectLoop) {
        Entity *entity = &objectEntityList[objectLoop];

        if (entity->priority == PRIORITY_ACTIVE_PAUSED && entity->type > OBJ_TYPE_BLANKOBJECT) {
            ObjectScript *scriptInfo = &objectScriptList[entity->type];
            activePlayer             = 0;
            if (scriptCode[scriptInfo->subMain.scriptCodePtr] > 0)
                ProcessScript(scriptInfo->subMain.scriptCodePtr, scriptInfo->subMain.jumpTablePtr, SUB_MAIN);

            if (scriptCode[scriptInfo->subPlayerInteraction.scriptCodePtr] > 0) {
                while (activePlayer < LEGACY_v3_PLAYER_COUNT) {
                    if (playerList[activePlayer].objectInteractions)
                        ProcessScript(scriptInfo->subPlayerInteraction.scriptCodePtr, scriptInfo->subPlayerInteraction.jumpTablePtr,
                                      SUB_PLAYERINTERACTION);
                    ++activePlayer;
                }
            }

            if (entity->drawOrder < LEGACY_DRAWLAYER_COUNT)
                drawListEntries[entity->drawOrder].entityRefs[drawListEntries[entity->drawOrder].listSize++] = objectLoop;
        }
    }
}