
uint8 AudioDevice::contextInitialized;

bool32 AudioDevice::Init()
{
    if (!contextInitialized) {
        contextInitialized = true;
        InitAudioChannels();
    }

    return true;
}

void AudioDevice::Release()
{
    LockAudioDevice();

    if (vorbisInfo) {
        vorbis_deinit(vorbisInfo);
        if (!vorbisInfo->alloc.alloc_buffer)
            free(vorbisInfo);
    }
    vorbisInfo = NULL;

    UnlockAudioDevice();
}

void AudioDevice::InitAudioChannels()
{
    for (int32 i = 0; i < CHANNEL_COUNT; ++i) {
        channels[i].soundID = -1;
        channels[i].state   = CHANNEL_IDLE;
    }

    for (int32 i = 0; i < 0x400; i += 2) {
        speedMixAmounts[i]     = (i + 0) * (1.0f / 1024.0f);
        speedMixAmounts[i + 1] = (i + 1) * (1.0f / 1024.0f);
    }

    GEN_HASH_MD5("Stream Channel 0", sfxList[SFX_COUNT - 1].hash);
    sfxList[SFX_COUNT - 1].scope              = SCOPE_GLOBAL;
    sfxList[SFX_COUNT - 1].maxConcurrentPlays = 1;
    sfxList[SFX_COUNT - 1].length             = MIX_BUFFER_SIZE;
    AllocateStorage((void **)&sfxList[SFX_COUNT - 1].buffer, MIX_BUFFER_SIZE * sizeof(SAMPLE_FORMAT), DATASET_MUS, false);

    initializedAudioChannels = true;
}
