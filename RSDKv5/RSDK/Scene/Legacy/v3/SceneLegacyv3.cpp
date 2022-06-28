
void RSDK::Legacy::v3::InitFirstStage()
{
    xScrollOffset = 0;
    yScrollOffset = 0;
    StopMusic();
    fadeMode     = 0;
    ClearGraphicsData();
    ClearAnimationData();
    activePalette = fullPalette[0];
    LoadPalette("MasterPalette.act", 0, 0, 0, 256);
#if RETRO_USE_MOD_LOADER
    // LoadXMLPalettes();
#endif

    stageMode = STAGEMODE_LOAD;
    gameMode  = 1;
}


void RSDK::Legacy::v3::ProcessStage() {

#if !RETRO_USE_ORIGINAL_CODE
    debugHitboxCount = 0;
#endif

    switch (stageMode) {
        case STAGEMODE_LOAD: // Startup
            fadeMode = 0;
            SetActivePalette(0, 0, 256);
            InitCameras();
            videoSettings.dimLimit = (5 * 60) * videoSettings.refreshRate;

            xScrollOffset = 0;
            yScrollOffset = 0;
            // yScrollA      = 0;
            // yScrollB      = SCREEN_YSIZE;
            // xScrollA      = 0;
            // xScrollB      = SCREEN_XSIZE;
            // yScrollMove   = 0;
            cameraShakeX  = 0;
            cameraShakeY  = 0;

            vertexCount = 0;
            faceCount   = 0;

            for (int32 p = 0; p < PLAYER_COUNT; ++p) {
                // memset(&playerList[p], 0, sizeof(playerList[p]));
                // playerList[p].visible            = true;
                // playerList[p].gravity            = 1; // Air
                // playerList[p].tileCollisions     = true;
                // playerList[p].objectInteractions = true;
            }

            pauseEnabled      = false;
            timeEnabled       = false;
            frameCounter      = 0;
            stageMilliseconds = 0;
            stageSeconds      = 0;
            stageMinutes      = 0;
            stageMode         = STAGEMODE_NORMAL;
#if RETRO_USE_MOD_LOADER
            // RefreshModFolders();
#endif
            ResetBackgroundSettings();
            // LoadStageFiles();
            break;

        case STAGEMODE_NORMAL:
            if (fadeMode > 0)
                fadeMode--;

            if (paletteMode > 0) {
                paletteMode = 0;
                SetActivePalette(0, 0, 256);
            }

            lastXSize = -1;
            lastYSize = -1;

            // if (pauseEnabled && controller[CONT_ANY].keyStart.press) {
            //     stageMode = STAGEMODE_PAUSED;
            //     PauseSound();
            // }

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

            // Update
            ProcessObjects();
            // HandleCameras();
            ProcessParallaxAutoScroll();

            // if (cameraTarget > -1) {
            //     if (cameraEnabled == 1) {
            //         switch (cameraStyle) {
            //             case CAMERASTYLE_FOLLOW: SetPlayerScreenPosition(&playerList[cameraTarget]); break;
            //             case CAMERASTYLE_EXTENDED:
            //             case CAMERASTYLE_EXTENDED_OFFSET_L:
            //             case CAMERASTYLE_EXTENDED_OFFSET_R: SetPlayerScreenPositionCDStyle(&playerList[cameraTarget]); break;
            //             case CAMERASTYLE_HLOCKED: SetPlayerHLockedScreenPosition(&playerList[cameraTarget]); break;
            //             default: break;
            //         }
            //     }
            //     else {
            //         SetPlayerLockedScreenPosition(&playerList[cameraTarget]);
            //     }
            // }
            // 
            // DrawStageGFX();
            break;

        case STAGEMODE_PAUSED:
            if (fadeMode > 0)
                fadeMode--;

            lastXSize = -1;
            lastYSize = -1;

            if (controller[CONT_ANY].keyC.press) {
                controller[CONT_ANY].keyC.press = false;

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

                ProcessObjects();
                // HandleCameras();
                // 
                // DrawStageGFX();

                if (fadeMode)
                    DrawRectangle(0, 0, SCREEN_XSIZE, SCREEN_YSIZE, fadeR, fadeG, fadeB, fadeA);
            }
            else {
                // Update
                ProcessPausedObjects();

                // DrawObjectList(0);
                // DrawObjectList(1);
                // DrawObjectList(2);
                // DrawObjectList(3);
                // DrawObjectList(4);
                // DrawObjectList(5);
                // DrawObjectList(7); // ???
                // DrawObjectList(6);

            }

#if !RETRO_USE_ORIGINAL_CODE
            // DrawDebugOverlays();
#endif

            if (pauseEnabled && controller[CONT_ANY].keyStart.press) {
                stageMode = STAGEMODE_NORMAL;
                ResumeSound();
            }
            break;
    }
}