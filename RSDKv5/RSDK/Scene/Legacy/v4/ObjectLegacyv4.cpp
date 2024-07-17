
// Game Objects
int32 RSDK::Legacy::v4::objectEntityPos = 0;
int32 RSDK::Legacy::v4::curObjectType   = 0;
RSDK::Legacy::v4::Entity RSDK::Legacy::v4::objectEntityList[LEGACY_v4_ENTITY_COUNT * 2]; //"regular" list & "storage" list
int32 RSDK::Legacy::v4::processObjectFlag[LEGACY_v4_ENTITY_COUNT];
RSDK::Legacy::v4::TypeGroupList RSDK::Legacy::v4::objectTypeGroupList[LEGACY_v4_TYPEGROUP_COUNT];

int32 RSDK::Legacy::v4::playerListPos = 0;

char RSDK::Legacy::v4::typeNames[LEGACY_v4_OBJECT_COUNT][0x40];

void RSDK::Legacy::v4::ProcessStartupObjects()
{
    scriptFrameCount = 0;
    ClearAnimationData();
    scriptEng.arrayPosition[8] = LEGACY_v4_TEMPENTITY_START;
    OBJECT_BORDER_X1           = 0x80;
    OBJECT_BORDER_X3           = 0x20;
    OBJECT_BORDER_X2           = SCREEN_XSIZE + 0x80;
    OBJECT_BORDER_X4           = SCREEN_XSIZE + 0x20;
    Entity *entity             = &objectEntityList[LEGACY_v4_TEMPENTITY_START];
    // Dunno what this is meant for, but it's here in the original code so...
    objectEntityList[TEMPENTITY_START + 1].type = objectEntityList[0].type;

    memset(foreachStack, -1, sizeof(foreachStack));
    memset(jumpTableStack, 0, sizeof(jumpTableStack));

    for (int32 i = 0; i < LEGACY_v4_OBJECT_COUNT; ++i) {
        ObjectScript *scriptInfo    = &objectScriptList[i];
        objectEntityPos             = LEGACY_v4_TEMPENTITY_START;
        curObjectType               = i;
        scriptInfo->frameListOffset = scriptFrameCount;
        scriptInfo->spriteSheetID   = 0;
        entity->type                = i;

        if (scriptCode[scriptInfo->eventStartup.scriptCodePtr] > 0)
            ProcessScript(scriptInfo->eventStartup.scriptCodePtr, scriptInfo->eventStartup.jumpTablePtr, EVENT_SETUP);
        scriptInfo->frameCount = scriptFrameCount - scriptInfo->frameListOffset;
    }
    entity->type  = 0;
    curObjectType = 0;
}

void RSDK::Legacy::v4::ProcessObjects()
{
    for (int32 i = 0; i < LEGACY_DRAWLAYER_COUNT; ++i) drawListEntries[i].listSize = 0;

    for (objectEntityPos = 0; objectEntityPos < LEGACY_v4_ENTITY_COUNT; ++objectEntityPos) {
        processObjectFlag[objectEntityPos] = false;
        int32 x = 0, y = 0;
        Entity *entity = &objectEntityList[objectEntityPos];
        x              = entity->xpos >> 16;
        y              = entity->ypos >> 16;

        switch (entity->priority) {
            case PRIORITY_BOUNDS:
                processObjectFlag[objectEntityPos] = x > xScrollOffset - OBJECT_BORDER_X1 && x < xScrollOffset + OBJECT_BORDER_X2
                                                     && y > yScrollOffset - OBJECT_BORDER_Y1 && y < yScrollOffset + OBJECT_BORDER_Y2;
                break;

            case PRIORITY_ACTIVE:
            case PRIORITY_ALWAYS:
            case PRIORITY_ACTIVE_SMALL: processObjectFlag[objectEntityPos] = true; break;

            case PRIORITY_XBOUNDS:
                processObjectFlag[objectEntityPos] = x > xScrollOffset - OBJECT_BORDER_X1 && x < OBJECT_BORDER_X2 + xScrollOffset;
                break;

            case PRIORITY_XBOUNDS_DESTROY:
                processObjectFlag[objectEntityPos] = x > xScrollOffset - OBJECT_BORDER_X1 && x < xScrollOffset + OBJECT_BORDER_X2;
                if (!processObjectFlag[objectEntityPos]) {
                    processObjectFlag[objectEntityPos] = false;
                    entity->type                       = OBJ_TYPE_BLANKOBJECT;
                }
                break;

            case PRIORITY_INACTIVE: processObjectFlag[objectEntityPos] = false; break;
            case PRIORITY_BOUNDS_SMALL:
                processObjectFlag[objectEntityPos] = x > xScrollOffset - OBJECT_BORDER_X3 && x < OBJECT_BORDER_X4 + xScrollOffset
                                                     && y > yScrollOffset - OBJECT_BORDER_Y3 && y < yScrollOffset + OBJECT_BORDER_Y4;
                break;

            default: break;
        }

        if (processObjectFlag[objectEntityPos] && entity->type > OBJ_TYPE_BLANKOBJECT) {
            ObjectScript *scriptInfo = &objectScriptList[entity->type];
            if (scriptCode[scriptInfo->eventUpdate.scriptCodePtr] > 0)
                ProcessScript(scriptInfo->eventUpdate.scriptCodePtr, scriptInfo->eventUpdate.jumpTablePtr, EVENT_MAIN);

            if (entity->drawOrder < LEGACY_DRAWLAYER_COUNT)
                drawListEntries[entity->drawOrder].entityRefs[drawListEntries[entity->drawOrder].listSize++] = objectEntityPos;
        }
    }

    for (int32 g = 0; g < LEGACY_v4_TYPEGROUP_COUNT; ++g) objectTypeGroupList[g].listSize = 0;

    for (objectEntityPos = 0; objectEntityPos < LEGACY_v4_ENTITY_COUNT; ++objectEntityPos) {
        Entity *entity = &objectEntityList[objectEntityPos];
        if (processObjectFlag[objectEntityPos] && entity->objectInteractions) {
            // Custom Group
            if (entity->groupID >= LEGACY_v4_OBJECT_COUNT) {
                TypeGroupList *listCustom                      = &objectTypeGroupList[objectEntityList[objectEntityPos].groupID];
                listCustom->entityRefs[listCustom->listSize++] = objectEntityPos;
            }

            // Type-Specific list
            TypeGroupList *listType                    = &objectTypeGroupList[objectEntityList[objectEntityPos].type];
            listType->entityRefs[listType->listSize++] = objectEntityPos;

            // All Entities list
            TypeGroupList *listAll                   = &objectTypeGroupList[GROUP_ALL];
            listAll->entityRefs[listAll->listSize++] = objectEntityPos;
        }
    }
}
void RSDK::Legacy::v4::ProcessPausedObjects()
{
    for (int32 i = 0; i < LEGACY_DRAWLAYER_COUNT; ++i) drawListEntries[i].listSize = 0;

    for (objectEntityPos = 0; objectEntityPos < LEGACY_v4_ENTITY_COUNT; ++objectEntityPos) {
        Entity *entity = &objectEntityList[objectEntityPos];

        if (entity->priority == PRIORITY_ALWAYS && entity->type > OBJ_TYPE_BLANKOBJECT) {
            ObjectScript *scriptInfo = &objectScriptList[entity->type];
            if (scriptCode[scriptInfo->eventUpdate.scriptCodePtr] > 0)
                ProcessScript(scriptInfo->eventUpdate.scriptCodePtr, scriptInfo->eventUpdate.jumpTablePtr, EVENT_MAIN);

            if (entity->drawOrder < LEGACY_DRAWLAYER_COUNT)
                drawListEntries[entity->drawOrder].entityRefs[drawListEntries[entity->drawOrder].listSize++] = objectEntityPos;
        }
    }
}
void RSDK::Legacy::v4::ProcessFrozenObjects()
{
    for (int32 g = 0; g < LEGACY_DRAWLAYER_COUNT; ++g) drawListEntries[g].listSize = 0;

    for (objectEntityPos = 0; objectEntityPos < LEGACY_v4_ENTITY_COUNT; ++objectEntityPos) {
        processObjectFlag[objectEntityPos] = false;
        int32 x = 0, y = 0;
        Entity *entity = &objectEntityList[objectEntityPos];
        x              = entity->xpos >> 16;
        y              = entity->ypos >> 16;

        switch (entity->priority) {
            case PRIORITY_BOUNDS:
                processObjectFlag[objectEntityPos] = x > xScrollOffset - OBJECT_BORDER_X1 && x < xScrollOffset + OBJECT_BORDER_X2
                                                     && y > yScrollOffset - OBJECT_BORDER_Y1 && y < yScrollOffset + OBJECT_BORDER_Y2;
                break;

            case PRIORITY_ACTIVE:
            case PRIORITY_ALWAYS:
            case PRIORITY_ACTIVE_SMALL: processObjectFlag[objectEntityPos] = true; break;

            case PRIORITY_XBOUNDS:
                processObjectFlag[objectEntityPos] = x > xScrollOffset - OBJECT_BORDER_X1 && x < OBJECT_BORDER_X2 + xScrollOffset;
                break;

            case PRIORITY_XBOUNDS_DESTROY:
                processObjectFlag[objectEntityPos] = x > xScrollOffset - OBJECT_BORDER_X1 && x < xScrollOffset + OBJECT_BORDER_X2;
                if (!processObjectFlag[objectEntityPos]) {
                    processObjectFlag[objectEntityPos] = false;
                    entity->type                       = OBJ_TYPE_BLANKOBJECT;
                }
                break;

            case PRIORITY_INACTIVE: processObjectFlag[objectEntityPos] = false; break;

            case PRIORITY_BOUNDS_SMALL:
                processObjectFlag[objectEntityPos] = x > xScrollOffset - OBJECT_BORDER_X3 && x < OBJECT_BORDER_X4 + xScrollOffset
                                                     && y > yScrollOffset - OBJECT_BORDER_Y3 && y < yScrollOffset + OBJECT_BORDER_Y4;
                break;

            default: break;
        }

        if (processObjectFlag[objectEntityPos] && entity->type > OBJ_TYPE_BLANKOBJECT) {
            ObjectScript *scriptInfo = &objectScriptList[entity->type];
            if (scriptCode[scriptInfo->eventUpdate.scriptCodePtr] > 0 && entity->priority == PRIORITY_ALWAYS)
                ProcessScript(scriptInfo->eventUpdate.scriptCodePtr, scriptInfo->eventUpdate.jumpTablePtr, EVENT_MAIN);

            if (entity->drawOrder < LEGACY_DRAWLAYER_COUNT)
                drawListEntries[entity->drawOrder].entityRefs[drawListEntries[entity->drawOrder].listSize++] = objectEntityPos;
        }
    }

    for (int32 g = 0; g < LEGACY_v4_TYPEGROUP_COUNT; ++g) objectTypeGroupList[g].listSize = 0;

    for (objectEntityPos = 0; objectEntityPos < LEGACY_v4_ENTITY_COUNT; ++objectEntityPos) {
        Entity *entity = &objectEntityList[objectEntityPos];
        if (processObjectFlag[objectEntityPos] && entity->objectInteractions) {
            // Custom Group
            if (entity->groupID >= LEGACY_v4_OBJECT_COUNT) {
                TypeGroupList *listCustom                      = &objectTypeGroupList[objectEntityList[objectEntityPos].groupID];
                listCustom->entityRefs[listCustom->listSize++] = objectEntityPos;
            }

            // Type-Specific list
            TypeGroupList *listType                    = &objectTypeGroupList[objectEntityList[objectEntityPos].type];
            listType->entityRefs[listType->listSize++] = objectEntityPos;

            // All Entities list
            TypeGroupList *listAll                   = &objectTypeGroupList[GROUP_ALL];
            listAll->entityRefs[listAll->listSize++] = objectEntityPos;
        }
    }
}
void RSDK::Legacy::v4::Process2PObjects()
{
    for (int32 i = 0; i < LEGACY_DRAWLAYER_COUNT; ++i) drawListEntries[i].listSize = 0;

    int32 boundX1 = -(0x200 << 16);
    int32 boundX2 = (0x200 << 16);
    int32 boundX3 = -(0x180 << 16);
    int32 boundX4 = (0x180 << 16);

    int32 boundY1 = -(0x180 << 16);
    int32 boundY2 = (0x180 << 16);
    int32 boundY3 = -(0x100 << 16);
    int32 boundY4 = (0x100 << 16);

    for (objectEntityPos = 0; objectEntityPos < LEGACY_v4_ENTITY_COUNT; ++objectEntityPos) {
        processObjectFlag[objectEntityPos] = false;
        int32 x = 0, y = 0;

        Entity *entity = &objectEntityList[objectEntityPos];
        x              = entity->xpos;
        y              = entity->ypos;

        // Set these here, they could (and prolly are) updated after objects
        Entity *entityP1 = &objectEntityList[0];
        int32 XPosP1     = entityP1->xpos;
        int32 YPosP1     = entityP1->ypos;

        Entity *entityP2 = &objectEntityList[1];
        int32 XPosP2     = entityP2->xpos;
        int32 YPosP2     = entityP2->ypos;

        switch (entity->priority) {
            case PRIORITY_BOUNDS:
                processObjectFlag[objectEntityPos] = x > XPosP1 + boundX1 && x < XPosP1 + boundX2 && y > YPosP1 + boundY1 && y < YPosP1 + boundY2;
                if (!processObjectFlag[objectEntityPos]) {
                    processObjectFlag[objectEntityPos] = x > XPosP2 + boundX1 && x < XPosP2 + boundX2 && y > YPosP2 + boundY1 && y < YPosP2 + boundY2;
                }
                break;

            case PRIORITY_ACTIVE:
            case PRIORITY_ALWAYS:
            case PRIORITY_ACTIVE_SMALL: processObjectFlag[objectEntityPos] = true; break;

            case PRIORITY_XBOUNDS:
                processObjectFlag[objectEntityPos] = x > XPosP1 + boundX1 && x < XPosP1 + boundX2;
                if (!processObjectFlag[objectEntityPos]) {
                    processObjectFlag[objectEntityPos] = x > XPosP2 + boundX1 && x < XPosP2 + boundX2;
                }
                break;

            case PRIORITY_XBOUNDS_DESTROY:
                processObjectFlag[objectEntityPos] = x > XPosP1 + boundX1 && x < XPosP1 + boundX2;
                if (!processObjectFlag[objectEntityPos]) {
                    processObjectFlag[objectEntityPos] = x > XPosP2 + boundX1 && x < XPosP2 + boundX2;
                }

                if (!processObjectFlag[objectEntityPos])
                    entity->type = OBJ_TYPE_BLANKOBJECT;
                break;

            case PRIORITY_INACTIVE: processObjectFlag[objectEntityPos] = false; break;

            case PRIORITY_BOUNDS_SMALL:
                processObjectFlag[objectEntityPos] = x > XPosP1 + boundX3 && x < XPosP1 + boundX4 && y > YPosP1 + boundY3 && y < YPosP1 + boundY4;
                if (!processObjectFlag[objectEntityPos]) {
                    processObjectFlag[objectEntityPos] = x > XPosP2 + boundX3 && x < XPosP2 + boundX4 && y > YPosP2 + boundY3 && y < YPosP2 + boundY4;
                }
                break;

            default: break;
        }

        if (processObjectFlag[objectEntityPos] && entity->type > OBJ_TYPE_BLANKOBJECT) {
            ObjectScript *scriptInfo = &objectScriptList[entity->type];
            if (scriptCode[scriptInfo->eventUpdate.scriptCodePtr] > 0)
                ProcessScript(scriptInfo->eventUpdate.scriptCodePtr, scriptInfo->eventUpdate.jumpTablePtr, EVENT_MAIN);

            if (entity->drawOrder < LEGACY_DRAWLAYER_COUNT)
                drawListEntries[entity->drawOrder].entityRefs[drawListEntries[entity->drawOrder].listSize++] = objectEntityPos;
        }
    }

    for (int32 i = 0; i < LEGACY_v4_TYPEGROUP_COUNT; ++i) objectTypeGroupList[i].listSize = 0;

    for (objectEntityPos = 0; objectEntityPos < LEGACY_v4_ENTITY_COUNT; ++objectEntityPos) {
        Entity *entity = &objectEntityList[objectEntityPos];
        if (processObjectFlag[objectEntityPos] && entity->objectInteractions) {
            // Custom Group
            if (entity->groupID >= LEGACY_v4_OBJECT_COUNT) {
                TypeGroupList *listCustom                      = &objectTypeGroupList[objectEntityList[objectEntityPos].groupID];
                listCustom->entityRefs[listCustom->listSize++] = objectEntityPos;
            }

            // Type-Specific list
            TypeGroupList *listType                    = &objectTypeGroupList[objectEntityList[objectEntityPos].type];
            listType->entityRefs[listType->listSize++] = objectEntityPos;

            // All Entities list
            TypeGroupList *listAll                   = &objectTypeGroupList[GROUP_ALL];
            listAll->entityRefs[listAll->listSize++] = objectEntityPos;
        }
    }
}

void RSDK::Legacy::v4::SetObjectTypeName(const char *objectName, int32 objectID)
{
    int32 objPos  = 0;
    int32 typePos = 0;
    while (objectName[objPos]) {
        if (objectName[objPos] != ' ')
            typeNames[objectID][typePos++] = objectName[objPos];
        ++objPos;
    }
    typeNames[objectID][typePos] = 0;
    PrintLog(PRINT_NORMAL, "Set Object (%d) name to: %s", objectID, objectName);
}

void RSDK::Legacy::v4::ProcessObjectControl(RSDK::Legacy::v4::Entity *entity)
{
    if ((uint8)entity->controlMode < CONT_P4) {
        int32 controllerID = entity->controlMode + 1;

        RSDK::ControllerState *controller = &RSDK::controller[controllerID];
        RSDK::AnalogState *stickL         = &RSDK::stickL[controllerID];

        entity->up    = stickL->keyUp.down || controller->keyUp.down;
        entity->down  = stickL->keyDown.down || controller->keyDown.down;
        entity->left  = stickL->keyLeft.down || controller->keyLeft.down;
        entity->right = stickL->keyRight.down | controller->keyRight.down;

        if (entity->left && entity->right) {
            entity->left  = false;
            entity->right = false;
        }

        entity->jumpPress = controller->keyB.press || controller->keyA.press || controller->keyC.press;
        entity->jumpHold  = controller->keyB.down || controller->keyA.down || controller->keyC.down;
    }
}