

int32 RSDK::Legacy::globalSFXCount = 0;
int32 RSDK::Legacy::stageSFXCount  = 0;

int32 RSDK::Legacy::musicVolume       = 0;
int32 RSDK::Legacy::sfxVolume         = 0;
int32 RSDK::Legacy::bgmVolume         = 0;
int32 RSDK::Legacy::musicCurrentTrack = 0;
int32 RSDK::Legacy::musicChannel      = 0;

float RSDK::Legacy::v4::musicRatio        = 0.0f;

RSDK::Legacy::TrackInfo RSDK::Legacy::musicTracks[LEGACY_TRACK_COUNT];

char RSDK::Legacy::v4::sfxNames[SFX_COUNT][0x40];

void RSDK::Legacy::SetMusicTrack(const char *filePath, uint8 trackID, bool32 loop, uint32 loopPoint)
{
    TrackInfo *track = &musicTracks[trackID & 0xF];

    // StrCopy(track->fileName, "Data/Music/");
    // StrAdd(track->fileName, filePath);
    StrCopy(track->fileName, filePath);
    track->trackLoop = loop;
    track->loopPoint = loopPoint;
}

int32 RSDK::Legacy::PlayMusic(int32 trackID)
{
    TrackInfo *track = &musicTracks[trackID & 0xF];

    if (!track->fileName[0]) {
        StopChannel(musicChannel);
    }
    else {
        uint32 startPos = 0;
        if (RSDK::Legacy::v4::musicRatio) {
            startPos                     = (uint32)(GetChannelPos(musicChannel) * RSDK::Legacy::v4::musicRatio);
            RSDK::Legacy::v4::musicRatio = 0.0;
        }

        musicCurrentTrack = trackID;
        musicChannel      = PlayStream(track->fileName, musicChannel, startPos, track->loopPoint, true);
        musicVolume       = 100;
    }

    return musicChannel;
}

void RSDK::Legacy::SetMusicVolume(uint32 volume)
{
    musicVolume = MIN(volume, 100);
    SetChannelAttributes(musicChannel, musicVolume * 0.01, 0.0, 1.0);
}

void RSDK::Legacy::v4::SwapMusicTrack(const char *filePath, uint8 trackID, uint32 loopPoint, uint32 ratio)
{
    TrackInfo *track = &musicTracks[trackID & 0xF];

    if (StrLength(filePath) <= 0) {
        StopChannel(musicChannel);
    }
    else {
        // StrCopy(track->fileName, "Data/Music/");
        // StrAdd(track->fileName, filePath);
        StrCopy(track->fileName, filePath);
        track->trackLoop = true;
        track->loopPoint = loopPoint;
        musicRatio       = ratio / 10000.0f;

        PlayMusic(trackID);
    }
}

void RSDK::Legacy::LoadSfx(char *filename, uint8 slot, uint8 scope)
{
    char fullFilePath[0x80];
    sprintf_s(fullFilePath, (int32)sizeof(fullFilePath), "Data/SoundFX/%s", filename);

    RETRO_HASH_MD5(hash);
    GEN_HASH_MD5(filename, hash);

    if (sfxList[slot].scope != SCOPE_NONE)
        return;

    uint8 type = fullFilePath[strlen(fullFilePath) - 3];
    if (type == 'w' || type == 'W') {
        GEN_HASH_MD5(filename, sfxList[slot].hash);
        sfxList[slot].scope              = scope;
        sfxList[slot].maxConcurrentPlays = 1;

        uint16 channels = 0;
        uint32 freq     = 0;
        uint32 format   = 0;
        uint32 size     = 0;
        ReadSfx(fullFilePath, slot, 1, scope, &size, &format, &channels, &freq);
    }
    else {
        // what the
        PrintLog(PRINT_ERROR, "Sfx format not supported!");
    }
}

void RSDK::Legacy::SetSfxAttributes(int32 sfxID, int32 loop, int8 pan)
{
    for (int32 c = 0; c < CHANNEL_COUNT; ++c) {
        if (channels[c].soundID == sfxID && channels[c].state == CHANNEL_SFX) {
            RSDK::SetChannelAttributes(c, 1.0, pan / 100.0f, 1.0);
            if (loop != -1)
                channels[c].loop = loop ? 0 : -1;
        }
    }
}

void RSDK::Legacy::v4::SetSfxName(const char *sfxName, int32 sfxID)
{
    int32 sfxNameID   = 0;
    int32 soundNameID = 0;
    while (sfxName[sfxNameID]) {
        if (sfxName[sfxNameID] != ' ')
            sfxNames[sfxID][soundNameID++] = sfxName[sfxNameID];
        ++sfxNameID;
    }
    sfxNames[sfxID][soundNameID] = 0;

    PrintLog(PRINT_NORMAL, "Set SFX (%d) name to: %s", sfxID, sfxName);
}

#if RETRO_USE_MOD_LOADER
char RSDK::Legacy::v3::globalSfxNames[SFX_COUNT][0x40];
char RSDK::Legacy::v3::stageSfxNames[SFX_COUNT][0x40];

void RSDK::Legacy::v3::SetSfxName(const char *sfxName, int32 sfxID, bool32 global)
{
    char *sfxNamePtr = global ? globalSfxNames[sfxID] : stageSfxNames[sfxID];

    int32 sfxNamePos = 0;
    int32 sfxPtrPos  = 0;
    uint8 mode      = 0;
    while (sfxName[sfxNamePos]) {
        if (sfxName[sfxNamePos] == '.' && mode == 1)
            mode = 2;
        else if ((sfxName[sfxNamePos] == '/' || sfxName[sfxNamePos] == '\\') && !mode)
            mode = 1;
        else if (sfxName[sfxNamePos] != ' ' && mode == 1)
            sfxNamePtr[sfxPtrPos++] = sfxName[sfxNamePos];
        ++sfxNamePos;
    }
    sfxNamePtr[sfxPtrPos] = 0;
    PrintLog(PRINT_NORMAL, "Set %s SFX (%d) name to: %s", (global ? "Global" : "Stage"), sfxID, sfxNamePtr);
}
#endif