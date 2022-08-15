

void RSDK::Legacy::v4::DrawObjectList(int32 group)
{
    for (int32 i = 0; i < drawListEntries[group].listSize; ++i) {
        objectEntityPos = drawListEntries[group].entityRefs[i];

        int32 type = objectEntityList[objectEntityPos].type;
        if (type) {
            if (scriptCode[objectScriptList[type].eventDraw.scriptCodePtr] > 0)
                ProcessScript(objectScriptList[type].eventDraw.scriptCodePtr, objectScriptList[type].eventDraw.jumpTablePtr, EVENT_DRAW);
        }
    }
}
void RSDK::Legacy::v4::DrawStageGFX()
{
    waterDrawPos = waterLevel - yScrollOffset;

    if (waterDrawPos < 0)
        waterDrawPos = 0;

    if (waterDrawPos > SCREEN_YSIZE)
        waterDrawPos = SCREEN_YSIZE;

    if (tLayerMidPoint < 3) {
        DrawObjectList(0);

        if (activeTileLayers[0] < LEGACY_LAYER_COUNT) {
            switch (stageLayouts[activeTileLayers[0]].type) {
                case LAYER_HSCROLL: DrawHLineScrollLayer(0); break;
                case LAYER_VSCROLL: DrawVLineScrollLayer(0); break;
                case LAYER_3DFLOOR: Draw3DFloorLayer(0); break;
                case LAYER_3DSKY: Draw3DSkyLayer(0); break;

                default: break;
            }
        }

        DrawObjectList(1);

        if (activeTileLayers[1] < LEGACY_LAYER_COUNT) {
            switch (stageLayouts[activeTileLayers[1]].type) {
                case LAYER_HSCROLL: DrawHLineScrollLayer(1); break;
                case LAYER_VSCROLL: DrawVLineScrollLayer(1); break;
                case LAYER_3DFLOOR: Draw3DFloorLayer(1); break;
                case LAYER_3DSKY: Draw3DSkyLayer(1); break;
                default: break;
            }
        }

        DrawObjectList(2);
        DrawObjectList(3);
        DrawObjectList(4);

        if (activeTileLayers[2] < LEGACY_LAYER_COUNT) {
            switch (stageLayouts[activeTileLayers[2]].type) {
                case LAYER_HSCROLL: DrawHLineScrollLayer(2); break;
                case LAYER_VSCROLL: DrawVLineScrollLayer(2); break;
                case LAYER_3DFLOOR: Draw3DFloorLayer(2); break;
                case LAYER_3DSKY: Draw3DSkyLayer(2); break;
                default: break;
            }
        }
    }
    else if (tLayerMidPoint < 6) {
        DrawObjectList(0);

        if (activeTileLayers[0] < LEGACY_LAYER_COUNT) {
            switch (stageLayouts[activeTileLayers[0]].type) {
                case LAYER_HSCROLL: DrawHLineScrollLayer(0); break;
                case LAYER_VSCROLL: DrawVLineScrollLayer(0); break;
                case LAYER_3DFLOOR: Draw3DFloorLayer(0); break;
                case LAYER_3DSKY: Draw3DSkyLayer(0); break;
                default: break;
            }
        }

        DrawObjectList(1);

        if (activeTileLayers[1] < LEGACY_LAYER_COUNT) {
            switch (stageLayouts[activeTileLayers[1]].type) {
                case LAYER_HSCROLL: DrawHLineScrollLayer(1); break;
                case LAYER_VSCROLL: DrawVLineScrollLayer(1); break;
                case LAYER_3DFLOOR: Draw3DFloorLayer(1); break;
                case LAYER_3DSKY: Draw3DSkyLayer(1); break;
                default: break;
            }
        }

        DrawObjectList(2);

        if (activeTileLayers[2] < LEGACY_LAYER_COUNT) {
            switch (stageLayouts[activeTileLayers[2]].type) {
                case LAYER_HSCROLL: DrawHLineScrollLayer(2); break;
                case LAYER_VSCROLL: DrawVLineScrollLayer(2); break;
                case LAYER_3DFLOOR: Draw3DFloorLayer(2); break;
                case LAYER_3DSKY: Draw3DSkyLayer(2); break;
                default: break;
            }
        }

        DrawObjectList(3);
        DrawObjectList(4);
    }

    if (tLayerMidPoint < 6) {
        if (activeTileLayers[3] < LEGACY_LAYER_COUNT) {
            switch (stageLayouts[activeTileLayers[3]].type) {
                case LAYER_HSCROLL: DrawHLineScrollLayer(3); break;
                case LAYER_VSCROLL: DrawVLineScrollLayer(3); break;
                case LAYER_3DFLOOR: Draw3DFloorLayer(3); break;
                case LAYER_3DSKY: Draw3DSkyLayer(3); break;
                default: break;
            }
        }

        DrawObjectList(5);
        DrawObjectList(7); // ???
        DrawObjectList(6);
    }

    if (fadeMode > 0)
        DrawRectangle(0, 0, SCREEN_XSIZE, SCREEN_YSIZE, fadeR, fadeG, fadeB, fadeA);

#if !RETRO_USE_ORIGINAL_CODE
    DrawDebugOverlays();
#endif
}

#if !RETRO_USE_ORIGINAL_CODE
void RSDK::Legacy::v4::DrawDebugOverlays()
{
    if (showHitboxes) {
        for (int32 i = 0; i < debugHitboxCount; ++i) {
            DebugHitboxInfo *info = &debugHitboxList[i];
            int32 x               = info->pos.x + (info->hitbox.left << 16);
            int32 y               = info->pos.y + (info->hitbox.top << 16);
            int32 w               = abs((info->pos.x + (info->hitbox.right << 16)) - x) >> 16;
            int32 h               = abs((info->pos.y + (info->hitbox.bottom << 16)) - y) >> 16;
            x                     = (x >> 16) - xScrollOffset;
            y                     = (y >> 16) - yScrollOffset;

            switch (info->type) {
                case H_TYPE_TOUCH:
                    if (showHitboxes & 1)
                        DrawRectangle(x, y, w, h, info->collision ? 0x80 : 0xFF, info->collision ? 0x80 : 0x00, 0x00, 0x60);
                    break;

                case H_TYPE_BOX:
                    if (showHitboxes & 1) {
                        DrawRectangle(x, y, w, h, 0x00, 0x00, 0xFF, 0x60);
                        if (info->collision & 1) // top
                            DrawRectangle(x, y, w, 1, 0xFF, 0xFF, 0x00, 0xC0);
                        if (info->collision & 8) // bottom
                            DrawRectangle(x, y + h, w, 1, 0xFF, 0xFF, 0x00, 0xC0);
                        if (info->collision & 2) { // left
                            int32 sy = y;
                            int32 sh = h;
                            if (info->collision & 1) {
                                sy++;
                                sh--;
                            }
                            if (info->collision & 8)
                                sh--;
                            DrawRectangle(x, sy, 1, sh, 0xFF, 0xFF, 0x00, 0xC0);
                        }
                        if (info->collision & 4) { // right
                            int32 sy = y;
                            int32 sh = h;
                            if (info->collision & 1) {
                                sy++;
                                sh--;
                            }
                            if (info->collision & 8)
                                sh--;
                            DrawRectangle(x + w, sy, 1, sh, 0xFF, 0xFF, 0x00, 0xC0);
                        }
                    }
                    break;

                case H_TYPE_PLAT:
                    if (showHitboxes & 1) {
                        DrawRectangle(x, y, w, h, 0x00, 0xFF, 0x00, 0x60);
                        if (info->collision & 1) // top
                            DrawRectangle(x, y, w, 1, 0xFF, 0xFF, 0x00, 0xC0);
                        if (info->collision & 8) // bottom
                            DrawRectangle(x, y + h, w, 1, 0xFF, 0xFF, 0x00, 0xC0);
                    }
                    break;
            }
        }
    }

    if (engine.showPaletteOverlay) {
        for (int32 p = 0; p < LEGACY_PALETTE_COUNT; ++p) {
            int32 x = (SCREEN_XSIZE - (0x10 << 3));
            int32 y = (SCREEN_YSIZE - (0x10 << 2));

            for (int32 c = 0; c < PALETTE_BANK_SIZE; ++c) {
                uint32 clr = GetPaletteEntryPacked(p, c);

                DrawRectangle(x + ((c & 0xF) << 1) + ((p % (LEGACY_PALETTE_COUNT / 2)) * (2 * 16)),
                              y + ((c >> 4) << 1) + ((p / (LEGACY_PALETTE_COUNT / 2)) * (2 * 16)), 2, 2, (clr >> 16) & 0xFF, (clr >> 8) & 0xFF,
                              (clr >> 0) & 0xFF, 0xFF);
            }
        }
    }
}
#endif