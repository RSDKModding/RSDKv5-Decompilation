#include <oboe/AudioStreamBuilder.h>

uint8 AudioDevice::contextInitialized;
oboe::Result AudioDevice::status = oboe::Result::OK;
oboe::AudioStream *AudioDevice::stream;
pthread_mutex_t AudioDevice::mutex;

// REAL LINE I WROTE
AudioDevice *AudioDevice::audioDevice;

bool32 AudioDevice::Init()
{
    if (!contextInitialized) {
        contextInitialized = true;
        InitAudioChannels();
        audioDevice = new AudioDevice();
    }

    oboe::AudioStreamBuilder builder;

    builder.setSampleRate(AUDIO_FREQUENCY)
        ->setFormat(oboe::AudioFormat::Float)
        ->setChannelCount(AUDIO_CHANNELS)
        ->setBufferCapacityInFrames(MIX_BUFFER_SIZE)
        ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
        ->setDataCallback(audioDevice)
        ->setErrorCallback(audioDevice);

    oboe::Result result = builder.openStream(&stream);
    if (result != oboe::Result::OK)
        return false;

    stream->requestStart();
    return true;
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

    pthread_mutex_init(&mutex, NULL);
    initializedAudioChannels = true;
}

oboe::DataCallbackResult AudioDevice::onAudioReady(oboe::AudioStream *s, void *data, int32 len)
{
    if (s != stream)
        return oboe::DataCallbackResult::Stop;

    LockAudioDevice();
    AudioDevice::ProcessAudioMixing(data, len * AUDIO_CHANNELS);
    UnlockAudioDevice();

    return oboe::DataCallbackResult::Continue;
}

bool AudioDevice::onError(oboe::AudioStream *s, oboe::Result error)
{
    (void)s;
    status = error;
    return false;
}

void AudioDevice::Release()
{
    stream->close();
    if (vorbisInfo) {
        vorbis_deinit(vorbisInfo);
        if (!vorbisInfo->alloc.alloc_buffer)
            free(vorbisInfo);
    }
    vorbisInfo = NULL;
    pthread_mutex_destroy(&mutex);
    delete audioDevice;
};

void AudioDevice::FrameInit()
{
    if (status != oboe::Result::OK) {
        stream->requestStop();
        stream->close();
        Init();
    }
};

void AudioDevice::HandleStreamLoad(ChannelInfo *channel, bool32 async)
{
    if (async) {
        pthread_t loadThread;
        pthread_create(&loadThread, NULL, LoadStreamASync, channel);
    }
    else
        LoadStream(channel);
};