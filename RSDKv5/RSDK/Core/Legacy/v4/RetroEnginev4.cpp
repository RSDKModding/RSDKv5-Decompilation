
bool32 RSDK::Legacy::v4::LoadGameConfig(const char *filepath)
{
    char strBuffer[0x40];
    StrCopy(gameVerInfo.gameTitle, "Retro-Engine"); // this is the default window name

    globalVariablesCount = 0;
#if RETRO_USE_MOD_LOADER
    modSettings.playerCount = 0;
#endif

    int32 gcSceneCount = 0;

    FileInfo info;
    InitFileInfo(&info);

    bool32 loaded = LoadFile(&info, filepath, FMODE_RB);
    if (loaded) {
        ReadString(&info, gameVerInfo.gameTitle);
        ReadString(&info, gameVerInfo.gameSubtitle);

        uint8 buf[3];
        for (int32 c = 0; c < 0x60; ++c) {
            ReadBytes(&info, buf, 3);
            SetPaletteEntry(-1, c, buf[0], buf[1], buf[2]);
        }

        // Read Obect Names
        uint8 objectCount = ReadInt8(&info);
        for (int32 o = 0; o < objectCount; ++o) {
            ReadString(&info, strBuffer);
        }

        // Read Script Paths
        for (int32 s = 0; s < objectCount; ++s) {
            ReadString(&info, strBuffer);
        }

        globalVariablesCount = ReadInt8(&info);
        for (int32 v = 0; v < globalVariablesCount; ++v) {
            // Read Variable Name
            ReadString(&info, globalVariables[v].name);

            // Read Variable Value
            globalVariables[v].value = ReadInt8(&info) << 0;
            globalVariables[v].value |= ReadInt8(&info) << 8;
            globalVariables[v].value |= ReadInt8(&info) << 16;
            globalVariables[v].value |= ReadInt8(&info) << 24;
        }

        // Read SFX
        globalSFXCount = ReadInt8(&info);
        for (int32 s = 0; s < globalSFXCount; ++s) { // Sfx Names
            ReadString(&info, strBuffer);
            SetSfxName(strBuffer, s);
        }

        for (int32 s = 0; s < globalSFXCount; ++s) { // Sfx Paths
            ReadString(&info, strBuffer);
            RSDK::Legacy::LoadSfx(strBuffer, s, SCOPE_GLOBAL);
        }

        // Read Player Names
        uint8 plrCount = ReadInt8(&info);
        for (uint8 p = 0; p < plrCount; ++p) {
            ReadString(&info, strBuffer);

            // needed for PlayerName[] stuff in scripts
#if RETRO_USE_MOD_LOADER
            StrCopy(modSettings.playerNames[p], strBuffer);
            modSettings.playerCount++;
#endif
        }

        // Read ahead and get scene count
        int32 storedPos = info.readPos;

        int32 totalSceneCount   = 0;
        sceneInfo.categoryCount = STAGELIST_MAX;
        for (uint8 c = 0; c < sceneInfo.categoryCount; ++c) {
            int32 count = ReadInt8(&info);
            for (int32 s = 0; s < count; ++s) {
                ReadString(&info, strBuffer);
                ReadString(&info, strBuffer);
                ReadString(&info, strBuffer);
                ReadInt8(&info);
            }

            totalSceneCount += count;
        }

#if RETRO_USE_MOD_LOADER
        gcSceneCount = totalSceneCount;
        totalSceneCount += RSDK::Legacy::v4::LoadXMLStages(2, 0);
#endif
        if (!totalSceneCount)
            totalSceneCount = 1;

        int32 startScene = 0;
#if !RETRO_USE_ORIGINAL_CODE
        if (strlen(currentSceneFolder) && strlen(currentSceneID)) {
            AllocateStorage((void **)&sceneInfo.listData, sizeof(SceneListEntry) * (totalSceneCount + 1), DATASET_STG, false);
            SceneListEntry *scene = &sceneInfo.listData[totalSceneCount];
            strcpy(scene->name, "_RSDK_SCENE");
            strcpy(scene->folder, currentSceneFolder);
            strcpy(scene->id, currentSceneID);
#if RETRO_REV02
            scene->filter = sceneInfo.filter;
#endif
            GEN_HASH_MD5(scene->name, scene->hash);

            // Override existing values
            sceneInfo.activeCategory = 0;
            startScene               = totalSceneCount;
            currentSceneFolder[0]    = 0;
            currentSceneID[0]        = 0;
        }
        else {
            AllocateStorage((void **)&sceneInfo.listData, sizeof(SceneListEntry) * totalSceneCount, DATASET_STG, false);
        }
#else
        AllocateStorage((void **)&sceneInfo.listData, sizeof(SceneListEntry) * totalSceneCount, DATASET_STG, true);
#endif

        AllocateStorage((void **)&sceneInfo.listCategory, sizeof(SceneListInfo) * sceneInfo.categoryCount, DATASET_STG, true);

        char categoryNames[STAGELIST_MAX][0x20] = {
            "Presentation",
            "Regular",
            "Special",
            "Bonus",
        };

        // Jump back and properly load scene info
        Seek_Set(&info, storedPos);

        int32 sceneID = 0;
        for (uint8 c = 0; c < sceneInfo.categoryCount; ++c) {
            SceneListInfo *category = &sceneInfo.listCategory[c];
            StrCopy(category->name, categoryNames[c]);
            GEN_HASH_MD5(category->name, category->hash);

            category->sceneOffsetStart = sceneID;
            category->sceneCount       = ReadInt8(&info);
            for (int32 s = 0; s < category->sceneCount; ++s) {
                SceneListEntry *scene = &sceneInfo.listData[sceneID + s];

                ReadString(&info, scene->folder);

                ReadString(&info, scene->id);

                ReadString(&info, scene->name);
                GEN_HASH_MD5(scene->name, scene->hash);

                // highlighted (doesn't care)
                ReadInt8(&info);
            }

            category->sceneOffsetEnd = category->sceneOffsetStart + category->sceneCount;
            sceneID += category->sceneCount;
        }

        CloseFile(&info);

        sceneInfo.listPos = startScene;
#if RETRO_USE_MOD_LOADER
        RSDK::Legacy::v4::LoadXMLVariables();
        RSDK::Legacy::v4::LoadXMLPalettes();
        RSDK::Legacy::v4::LoadXMLObjects();
        RSDK::Legacy::v4::LoadXMLPlayers();
        RSDK::Legacy::v4::LoadXMLStages(0, gcSceneCount);
        
        SetGlobalVariableByName("options.devMenuFlag", false);
        if (engine.devMenu)
            SetGlobalVariableByName("options.devMenuFlag", true);
#endif

        usingBytecode = false;
        InitFileInfo(&info);
        if (useDataPack && LoadFile(&info, "Bytecode/GlobalCode.bin", FMODE_RB)) {
            usingBytecode = true;
            CloseFile(&info);
        }
    }

    // These need to be set every time its reloaded
    nativeFunctionCount = 0;

    nativeFunctionCount = 0;
    AddNativeFunction("SetAchievement", SetAchievement);
    AddNativeFunction("SetLeaderboard", SetLeaderboard);
    AddNativeFunction("HapticEffect", HapticEffect);

    // AddNativeFunction("Connect2PVS", Connect2PVS);
    // AddNativeFunction("Disconnect2PVS", Disconnect2PVS);
    // AddNativeFunction("SendEntity", SendEntity);
    // AddNativeFunction("SendValue", SendValue);
    // AddNativeFunction("ReceiveEntity", ReceiveEntity);
    // AddNativeFunction("ReceiveValue", ReceiveValue);
    // AddNativeFunction("TransmitGlobal", TransmitGlobal);
    // AddNativeFunction("ShowPromoPopup", ShowPromoPopup);

    AddNativeFunction("NotifyCallback", NotifyCallback);

#if RETRO_USE_MOD_LOADER
    AddNativeFunction("ExitGame", ExitGame);
    AddNativeFunction("FileExists", FileExists);
    AddNativeFunction("OpenModMenu", OpenModMenu); // Opens the dev menu-based mod menu incase you cant be bothered or smth
    AddNativeFunction("AddAchievement", AddGameAchievement);
    AddNativeFunction("SetAchievementDescription", SetAchievementDescription);
    AddNativeFunction("ClearAchievements", ClearAchievements);
    AddNativeFunction("GetAchievementCount", GetAchievementCount);
    AddNativeFunction("GetAchievement", GetAchievement);
    AddNativeFunction("GetAchievementName", GetAchievementName);
    AddNativeFunction("GetAchievementDescription", GetAchievementDescription);
    AddNativeFunction("GetScreenWidth", GetScreenWidth);
    AddNativeFunction("SetScreenWidth", SetScreenWidth);
    AddNativeFunction("GetWindowScale", GetWindowScale);
    AddNativeFunction("SetWindowScale", SetWindowScale);
    AddNativeFunction("GetWindowScaleMode", GetWindowScaleMode);
    AddNativeFunction("SetWindowScaleMode", SetWindowScaleMode);
    AddNativeFunction("GetWindowFullScreen", GetWindowFullScreen);
    AddNativeFunction("SetWindowFullScreen", SetWindowFullScreen);
    AddNativeFunction("GetWindowBorderless", GetWindowBorderless);
    AddNativeFunction("SetWindowBorderless", SetWindowBorderless);
    AddNativeFunction("GetWindowVSync", GetWindowVSync);
    AddNativeFunction("SetWindowVSync", SetWindowVSync);
    AddNativeFunction("ApplyWindowChanges", ApplyWindowChanges); // Refresh window after changing window options
    AddNativeFunction("GetModCount", GetModCount);
    AddNativeFunction("GetModName", GetModName);
    AddNativeFunction("GetModDescription", GetModDescription);
    AddNativeFunction("GetModAuthor", GetModAuthor);
    AddNativeFunction("GetModVersion", GetModVersion);
    AddNativeFunction("GetModActive", GetModActive);
    AddNativeFunction("SetModActive", SetModActive);
    AddNativeFunction("MoveMod", MoveMod);
    AddNativeFunction("RefreshEngine", RefreshEngine); // Reload engine after changing mod status
#endif

    return loaded;
}

void RSDK::Legacy::v4::ProcessEngine()
{
    switch (gameMode) {
        case ENGINE_DEVMENU:
            ProcessInput();
            currentScreen = &screens[0];

            if (devMenu.state)
                devMenu.state();
            break;

        case ENGINE_MAINGAME: ProcessStage(); break;

        case ENGINE_INITDEVMENU:
            LoadGameConfig("Data/Game/GameConfig.bin");
            InitFirstStage();
            ResetCurrentStageFolder();
            break;

        case ENGINE_WAIT:
            break;
        // case ENGINE_RESETGAME: break;

        case ENGINE_SCRIPTERROR: {
            currentScreen = screens;
            FillScreen(0x000000, 0x10, 0x10, 0x10);

            int32 yOff = DevOutput_GetStringYSize(scriptErrorMessage);
            DrawDevString(scriptErrorMessage, 8, currentScreen->center.y - (yOff >> 1) + 8, 0, 0xF0F0F0);
            break;
        }

        case ENGINE_INITPAUSE:
        case ENGINE_EXITPAUSE: gameMode = ENGINE_MAINGAME; break;

        case ENGINE_ENDGAME:
        case ENGINE_RESETGAME:
            sceneInfo.activeCategory = 0;
            sceneInfo.listPos        = 0;
            gameMode                 = ENGINE_MAINGAME;
            stageMode                = STAGEMODE_LOAD;
            break;

        default: break;
    }
}

#if RETRO_USE_MOD_LOADER
void RSDK::Legacy::v4::LoadXMLVariables()
{
    FileInfo info;
    for (int32 m = 0; m < (int32)ActiveMods().size(); ++m) {
        SetActiveMod(m);
        if (LoadFile(&info, "Data/Game/Game.xml", FMODE_RB)) {
            tinyxml2::XMLDocument *doc = new tinyxml2::XMLDocument;

            char *xmlData = new char[info.fileSize + 1];
            ReadBytes(&info, xmlData, info.fileSize);
            xmlData[info.fileSize] = 0;

            bool success = doc->Parse(xmlData) == tinyxml2::XML_SUCCESS;

            if (success) {
                const tinyxml2::XMLElement *gameElement      = (const tinyxml2::XMLElement *)FirstXMLChildElement(doc, nullptr, "game");
                const tinyxml2::XMLElement *variablesElement = (const tinyxml2::XMLElement *)FirstXMLChildElement(doc, gameElement, "variables");
                if (variablesElement) {
                    const tinyxml2::XMLElement *varElement = (const tinyxml2::XMLElement *)FirstXMLChildElement(doc, variablesElement, "variable");
                    if (varElement) {
                        do {
                            const tinyxml2::XMLAttribute *nameAttr = (const tinyxml2::XMLAttribute *)FindXMLAttribute(varElement, "name");
                            const char *varName                    = "unknownVariable";
                            if (nameAttr)
                                varName = GetXMLAttributeValueString(nameAttr);

                            const tinyxml2::XMLAttribute *valAttr = (const tinyxml2::XMLAttribute *)FindXMLAttribute(varElement, "value");
                            int32 varValue                        = 0;
                            if (valAttr)
                                varValue = GetXMLAttributeValueInt(valAttr);

                            StrCopy(globalVariables[globalVariablesCount].name, varName);
                            globalVariables[globalVariablesCount].value = varValue;
                            globalVariablesCount++;

                        } while ((varElement = (const tinyxml2::XMLElement *)NextXMLSiblingElement(doc, varElement, "variable")));
                    }
                }
            }

            delete[] xmlData;
            delete doc;

            CloseFile(&info);
        }
    }
    SetActiveMod(-1);
}
void RSDK::Legacy::v4::LoadXMLPalettes()
{
    FileInfo info;
    for (int32 m = 0; m < (int32)ActiveMods().size(); ++m) {
        SetActiveMod(m);
        if (LoadFile(&info, "Data/Game/Game.xml", FMODE_RB)) {
            tinyxml2::XMLDocument *doc = new tinyxml2::XMLDocument;

            char *xmlData = new char[info.fileSize + 1];
            ReadBytes(&info, xmlData, info.fileSize);
            xmlData[info.fileSize] = 0;

            bool success = doc->Parse(xmlData) == tinyxml2::XML_SUCCESS;

            if (success) {
                const tinyxml2::XMLElement *gameElement    = (const tinyxml2::XMLElement *)FirstXMLChildElement(doc, nullptr, "game");
                const tinyxml2::XMLElement *paletteElement = (const tinyxml2::XMLElement *)FirstXMLChildElement(doc, gameElement, "palette");
                if (paletteElement) {
                    const tinyxml2::XMLElement *clrElement = (const tinyxml2::XMLElement *)FirstXMLChildElement(doc, paletteElement, "color");
                    if (clrElement) {
                        do {
                            const tinyxml2::XMLAttribute *bankAttr = (const tinyxml2::XMLAttribute *)FindXMLAttribute(clrElement, "bank");
                            int32 clrBank                          = 0;
                            if (bankAttr)
                                clrBank = GetXMLAttributeValueInt(bankAttr);

                            const tinyxml2::XMLAttribute *indAttr = (const tinyxml2::XMLAttribute *)FindXMLAttribute(clrElement, "index");
                            int32 clrInd                          = 0;
                            if (indAttr)
                                clrInd = GetXMLAttributeValueInt(indAttr);

                            const tinyxml2::XMLAttribute *rAttr = (const tinyxml2::XMLAttribute *)FindXMLAttribute(clrElement, "r");
                            int32 clrR                          = 0;
                            if (rAttr)
                                clrR = GetXMLAttributeValueInt(rAttr);

                            const tinyxml2::XMLAttribute *gAttr = (const tinyxml2::XMLAttribute *)FindXMLAttribute(clrElement, "g");
                            int32 clrG                          = 0;
                            if (gAttr)
                                clrG = GetXMLAttributeValueInt(gAttr);

                            const tinyxml2::XMLAttribute *bAttr = (const tinyxml2::XMLAttribute *)FindXMLAttribute(clrElement, "b");
                            int32 clrB                          = 0;
                            if (bAttr)
                                clrB = GetXMLAttributeValueInt(bAttr);

                            SetPaletteEntry(clrBank, clrInd, clrR, clrG, clrB);

                        } while ((clrElement = (const tinyxml2::XMLElement *)NextXMLSiblingElement(doc, clrElement, "color")));
                    }
                }
            }

            delete[] xmlData;
            delete doc;

            CloseFile(&info);
        }
    }
    SetActiveMod(-1);
}
void RSDK::Legacy::v4::LoadXMLObjects()
{
    FileInfo info;
    modObjCount = 0;

    for (int32 m = 0; m < (int32)ActiveMods().size(); ++m) {
        SetActiveMod(m);
        if (LoadFile(&info, "Data/Game/Game.xml", FMODE_RB)) {
            tinyxml2::XMLDocument *doc = new tinyxml2::XMLDocument;

            char *xmlData = new char[info.fileSize + 1];
            ReadBytes(&info, xmlData, info.fileSize);
            xmlData[info.fileSize] = 0;

            bool success = doc->Parse(xmlData) == tinyxml2::XML_SUCCESS;

            if (success) {
                const tinyxml2::XMLElement *gameElement    = (const tinyxml2::XMLElement *)FirstXMLChildElement(doc, nullptr, "game");
                const tinyxml2::XMLElement *objectsElement = (const tinyxml2::XMLElement *)FirstXMLChildElement(doc, gameElement, "objects");
                if (objectsElement) {
                    const tinyxml2::XMLElement *objElement = (const tinyxml2::XMLElement *)FirstXMLChildElement(doc, objectsElement, "object");
                    if (objElement) {
                        do {
                            const tinyxml2::XMLAttribute *nameAttr = (const tinyxml2::XMLAttribute *)FindXMLAttribute(objElement, "name");
                            const char *objName                    = "unknownObject";
                            if (nameAttr)
                                objName = GetXMLAttributeValueString(nameAttr);

                            const tinyxml2::XMLAttribute *scrAttr = (const tinyxml2::XMLAttribute *)FindXMLAttribute(objElement, "script");
                            const char *objScript                 = "unknownObject.txt";
                            if (scrAttr)
                                objScript = GetXMLAttributeValueString(scrAttr);

                            uint8 flags = 0;

                            // forces the object to be loaded, this means the object doesn't have to be and *SHOULD NOT* be in the stage object list
                            // if it is, it'll cause issues!!!!
                            const tinyxml2::XMLAttribute *loadAttr = (const tinyxml2::XMLAttribute *)FindXMLAttribute(objElement, "forceLoad");
                            int32 objForceLoad                     = false;
                            if (loadAttr)
                                objForceLoad = GetXMLAttributeValueBool(loadAttr);

                            flags |= (objForceLoad & 1);

                            StrCopy(modTypeNames[modObjCount], objName);
                            StrCopy(modScriptPaths[modObjCount], objScript);
                            modScriptFlags[modObjCount] = flags;
                            modObjCount++;

                        } while ((objElement = (const tinyxml2::XMLElement *)NextXMLSiblingElement(doc, objElement, "object")));
                    }
                }
            }
            else {
                PrintLog(PRINT_NORMAL, "Failed to parse Game.xml File!");
            }

            delete[] xmlData;
            delete doc;

            CloseFile(&info);
        }
    }
    SetActiveMod(-1);
}
void RSDK::Legacy::v4::LoadXMLSoundFX()
{
    FileInfo info;
    FileInfo infoStore;
    for (int32 m = 0; m < (int32)ActiveMods().size(); ++m) {
        SetActiveMod(m);
        if (LoadFile(&info, "Data/Game/Game.xml", FMODE_RB)) {
            tinyxml2::XMLDocument *doc = new tinyxml2::XMLDocument;

            char *xmlData = new char[info.fileSize + 1];
            ReadBytes(&info, xmlData, info.fileSize);
            xmlData[info.fileSize] = 0;

            bool success = doc->Parse(xmlData) == tinyxml2::XML_SUCCESS;

            if (success) {
                const tinyxml2::XMLElement *gameElement   = (const tinyxml2::XMLElement *)FirstXMLChildElement(doc, nullptr, "game");
                const tinyxml2::XMLElement *soundsElement = (const tinyxml2::XMLElement *)FirstXMLChildElement(doc, gameElement, "sounds");
                if (soundsElement) {
                    const tinyxml2::XMLElement *sfxElement = (const tinyxml2::XMLElement *)FirstXMLChildElement(doc, soundsElement, "soundfx");
                    if (sfxElement) {
                        do {
                            const tinyxml2::XMLAttribute *nameAttr = (const tinyxml2::XMLAttribute *)FindXMLAttribute(sfxElement, "name");
                            const char *sfxName                    = "unknownSFX";
                            if (nameAttr)
                                sfxName = GetXMLAttributeValueString(nameAttr);

                            const tinyxml2::XMLAttribute *valAttr = (const tinyxml2::XMLAttribute *)FindXMLAttribute(sfxElement, "path");
                            const char *sfxPath                   = "unknownSFX.wav";
                            if (valAttr)
                                sfxPath = GetXMLAttributeValueString(valAttr);

                            SetSfxName(sfxName, globalSFXCount);

                            RSDK::Legacy::LoadSfx((char *)sfxPath, globalSFXCount, SCOPE_GLOBAL);
                            globalSFXCount++;

                        } while ((sfxElement = (const tinyxml2::XMLElement *)NextXMLSiblingElement(doc, sfxElement, "soundfx")));
                    }
                }
            }
            else {
                PrintLog(PRINT_NORMAL, "Failed to parse Game.xml File!");
            }

            delete[] xmlData;
            delete doc;

            CloseFile(&info);
        }
    }
    SetActiveMod(-1);
}
void RSDK::Legacy::v4::LoadXMLPlayers()
{
    FileInfo info;

    for (int32 m = 0; m < (int32)ActiveMods().size(); ++m) {
        SetActiveMod(m);
        if (LoadFile(&info, "Data/Game/Game.xml", FMODE_RB)) {
            tinyxml2::XMLDocument *doc = new tinyxml2::XMLDocument;

            char *xmlData = new char[info.fileSize + 1];
            ReadBytes(&info, xmlData, info.fileSize);
            xmlData[info.fileSize] = 0;

            bool success = doc->Parse(xmlData) == tinyxml2::XML_SUCCESS;

            if (success) {
                const tinyxml2::XMLElement *gameElement    = (const tinyxml2::XMLElement *)FirstXMLChildElement(doc, nullptr, "game");
                const tinyxml2::XMLElement *playersElement = (const tinyxml2::XMLElement *)FirstXMLChildElement(doc, gameElement, "players");
                if (playersElement) {
                    const tinyxml2::XMLElement *plrElement = (const tinyxml2::XMLElement *)FirstXMLChildElement(doc, playersElement, "player");
                    if (plrElement) {
                        do {
                            const tinyxml2::XMLAttribute *nameAttr = (const tinyxml2::XMLAttribute *)FindXMLAttribute(plrElement, "name");
                            const char *plrName                    = "unknownPlayer";
                            if (nameAttr)
                                plrName = GetXMLAttributeValueString(nameAttr);

                            StrCopy(modSettings.playerNames[modSettings.playerCount++], plrName);

                        } while ((plrElement = (const tinyxml2::XMLElement *)NextXMLSiblingElement(doc, plrElement, "player")));
                    }
                }
            }
            else {
                PrintLog(PRINT_NORMAL, "Failed to parse Game.xml File!");
            }

            delete[] xmlData;
            delete doc;

            CloseFile(&info);
        }
    }
    SetActiveMod(-1);
}
int32 RSDK::Legacy::v4::LoadXMLStages(int32 mode, int32 gcStageCount)
{
    FileInfo info;
    int32 listCount  = 0;
    int32 stageCount = 0;

    for (int32 m = 0; m < (int32)ActiveMods().size(); ++m) {
        SetActiveMod(m);
        if (LoadFile(&info, "Data/Game/Game.xml", FMODE_RB)) {
            tinyxml2::XMLDocument *doc = new tinyxml2::XMLDocument;

            char *xmlData = new char[info.fileSize + 1];
            ReadBytes(&info, xmlData, info.fileSize);
            xmlData[info.fileSize] = 0;

            bool success = doc->Parse(xmlData) == tinyxml2::XML_SUCCESS;

            if (success) {
                const tinyxml2::XMLElement *gameElement = (const tinyxml2::XMLElement *)FirstXMLChildElement(doc, nullptr, "game");
                const char *elementNames[]              = { "presentationStages", "regularStages", "specialStages", "bonusStages" };
                const char *categoryNames[]             = {
                    "Presentation",
                    "Regular",
                    "Special"
                    "Bonus",
                };

                for (int32 l = 0; l < sceneInfo.categoryCount; ++l) {
                    const tinyxml2::XMLElement *listElement = (const tinyxml2::XMLElement *)FirstXMLChildElement(doc, gameElement, elementNames[l]);
                    if (listElement) {
                        const tinyxml2::XMLElement *stgElement = (const tinyxml2::XMLElement *)FirstXMLChildElement(doc, listElement, "stage");

                        SceneListInfo *list = &sceneInfo.listCategory[l];
                        if (!mode) {
                            sprintf_s(list->name, (int32)sizeof(list->name), "%s", categoryNames[l]);
                            GEN_HASH_MD5(list->name, list->hash);

                            list->sceneOffsetStart = gcStageCount;
                            list->sceneOffsetEnd   = gcStageCount;
                            list->sceneCount       = 0;
                        }

                        if (stgElement) {
                            do {
                                if (!mode) {
                                    const tinyxml2::XMLAttribute *nameAttr = (const tinyxml2::XMLAttribute *)FindXMLAttribute(stgElement, "name");
                                    const char *stgName                    = "unknownStage";
                                    if (nameAttr)
                                        stgName = GetXMLAttributeValueString(nameAttr);

                                    const tinyxml2::XMLAttribute *folderAttr = (const tinyxml2::XMLAttribute *)FindXMLAttribute(stgElement, "folder");
                                    const char *stgFolder                    = "unknownStageFolder";
                                    if (nameAttr)
                                        stgFolder = GetXMLAttributeValueString(folderAttr);

                                    const tinyxml2::XMLAttribute *idAttr = (const tinyxml2::XMLAttribute *)FindXMLAttribute(stgElement, "id");
                                    const char *stgID                    = "unknownStageID";
                                    if (idAttr)
                                        stgID = GetXMLAttributeValueString(idAttr);

                                    SceneListEntry *scene = &sceneInfo.listData[gcStageCount];

                                    sprintf_s(scene->name, (int32)sizeof(scene->name), "%s", stgName);
                                    GEN_HASH_MD5(scene->name, scene->hash);
                                    sprintf_s(scene->folder, (int32)sizeof(scene->folder), "%s", stgFolder);
                                    sprintf_s(scene->id, (int32)sizeof(scene->id), "%s", stgID);

#if RETRO_REV02
                                    scene->filter = 0xFF;
#endif

                                    gcStageCount++;
                                    sceneInfo.listCategory[l].sceneCount++;
                                }
                                ++stageCount;
                            } while ((stgElement = (const tinyxml2::XMLElement *)NextXMLSiblingElement(doc, stgElement, "stage")));
                        }

                        if (!mode) {
                            list->sceneOffsetEnd += list->sceneCount;
                            sceneInfo.categoryCount++;
                        }
                    }
                }
            }
            else {
                PrintLog(PRINT_NORMAL, "Failed to parse Game.xml File!");
            }

            delete[] xmlData;
            delete doc;

            CloseFile(&info);
        }
    }

    SetActiveMod(-1);

    return stageCount;
}
#endif