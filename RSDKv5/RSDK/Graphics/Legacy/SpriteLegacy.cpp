
int32 RSDK::Legacy::AddGraphicsFile(const char *filePath)
{
    char sheetPath[0x100];

    StrCopy(sheetPath, "Data/Sprites/");
    StrAdd(sheetPath, filePath);

    int32 sheetID = 0;
    while (StrLength(gfxSurface[sheetID].fileName) > 0) {
        if (StrComp(gfxSurface[sheetID].fileName, sheetPath))
            return sheetID;
        if (++sheetID == LEGACY_SURFACE_COUNT)
            return 0;
    }

    ImageGIF image;
    if (image.Load(sheetPath, true)) {
        GFXSurface *surface = &gfxSurface[sheetID];

        StrCopy(surface->fileName, sheetPath);
        surface->width  = image.width;
        surface->height = image.height;

        surface->dataPosition = gfxDataPosition;
        gfxDataPosition += surface->width * surface->height;

        if (gfxDataPosition < LEGACY_GFXDATA_SIZE) {
            image.pixels = &graphicData[surface->dataPosition];
            image.Load(NULL, false);
        }
        else {
            gfxDataPosition = 0;
            PrintLog(PRINT_NORMAL, "WARNING: Exceeded max gfx size!");
        }

        surface->widthShift = 0;
        int32 w             = surface->width;
        while (w > 1) {
            w >>= 1;
            ++surface->widthShift;
        }
    }

    return sheetID;
}

void RSDK::Legacy::RemoveGraphicsFile(const char *filePath, int32 sheetID)
{
    if (sheetID < 0) {
        for (int32 i = 0; i < LEGACY_SURFACE_COUNT; ++i) {
            if (StrLength(gfxSurface[i].fileName) > 0 && StrComp(gfxSurface[i].fileName, filePath))
                sheetID = i;
        }
    }

    if (sheetID >= 0 && StrLength(gfxSurface[sheetID].fileName)) {
        StrCopy(gfxSurface[sheetID].fileName, "");
        int32 dataPosStart = gfxSurface[sheetID].dataPosition;
        int32 dataPosEnd   = gfxSurface[sheetID].dataPosition + gfxSurface[sheetID].height * gfxSurface[sheetID].width;

        for (int32 i = LEGACY_GFXDATA_SIZE - dataPosEnd; i > 0; --i) graphicData[dataPosStart++] = graphicData[dataPosEnd++];
        gfxDataPosition -= gfxSurface[sheetID].height * gfxSurface[sheetID].width;

        for (int32 i = 0; i < LEGACY_SURFACE_COUNT; ++i) {
            if (gfxSurface[i].dataPosition > gfxSurface[sheetID].dataPosition)
                gfxSurface[i].dataPosition -= gfxSurface[sheetID].height * gfxSurface[sheetID].width;
        }
    }
}