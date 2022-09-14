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

void AudioDevice::ProcessAudioMixing(void *stream, int32 length)
{
    SAMPLE_FORMAT *streamF    = (SAMPLE_FORMAT *)stream;
    SAMPLE_FORMAT *streamEndF = ((SAMPLE_FORMAT *)stream) + length;

    memset(stream, 0, length * sizeof(SAMPLE_FORMAT));

    LockAudioDevice();

    for (int32 c = 0; c < CHANNEL_COUNT; ++c) {
        ChannelInfo *channel = &channels[c];

        switch (channel->state) {
            default:
            case CHANNEL_IDLE: break;

            case CHANNEL_SFX: {
                SAMPLE_FORMAT *sfxBuffer = &channel->samplePtr[channel->bufferPos];

                // somehow it can get here and not have any data to play, causing a crash. This should fix that
                if (!sfxBuffer)
                    continue;

                float volL = channel->volume, volR = channel->volume;
                if (channel->pan < 0.0)
                    volL = (1.0 + channel->pan) * channel->volume;
                else
                    volR = (1.0 - channel->pan) * channel->volume;

                float panL = volL * engine.soundFXVolume;
                float panR = volR * engine.soundFXVolume;

                uint32 speedPercent       = 0;
                SAMPLE_FORMAT *curStreamF = streamF;
                while (curStreamF < streamEndF && streamF < streamEndF) {
                    // Perform linear interpolation.
                    SAMPLE_FORMAT sample = (sfxBuffer[1] - sfxBuffer[0]) * speedMixAmounts[speedPercent >> 6] + sfxBuffer[0];

                    speedPercent += channel->speed;
                    sfxBuffer += speedPercent >> 16;
                    channel->bufferPos += speedPercent >> 16;
                    speedPercent &= 0xFFFF;

                    curStreamF[0] += sample * panL;
                    curStreamF[1] += sample * panR;
                    curStreamF += 2;

                    if (channel->bufferPos >= channel->sampleLength) {
                        if (channel->loop == 0xFFFFFFFF) {
                            channel->state   = CHANNEL_IDLE;
                            channel->soundID = -1;
                            break;
                        }
                        else {
                            channel->bufferPos -= channel->sampleLength;
                            channel->bufferPos += channel->loop;

                            sfxBuffer = &channel->samplePtr[channel->bufferPos];
                        }
                    }
                }

                break;
            }

            case CHANNEL_STREAM: {
                SAMPLE_FORMAT *streamBuffer = &channel->samplePtr[channel->bufferPos];

                // somehow it can get here and not have any data to play, causing a crash. This should fix that
                if (!streamBuffer)
                    continue;

                float volL = channel->volume, volR = channel->volume;
                if (channel->pan < 0.0)
                    volL = (1.0 + channel->pan) * channel->volume;
                else
                    volR = (1.0 - channel->pan) * channel->volume;

                float panL = volL * engine.streamVolume;
                float panR = volR * engine.streamVolume;

                uint32 speedPercent       = 0;
                SAMPLE_FORMAT *curStreamF = streamF;
                while (curStreamF < streamEndF && streamF < streamEndF) {
                    speedPercent += channel->speed;
                    int32 next = speedPercent >> 16;
                    speedPercent &= 0xFFFF;

                    curStreamF[0] += panL * streamBuffer[0];
                    curStreamF[1] += panR * streamBuffer[1];
                    curStreamF += 2;

                    streamBuffer += next * 2;
                    channel->bufferPos += next * 2;

                    if (channel->bufferPos >= channel->sampleLength) {
                        channel->bufferPos -= channel->sampleLength;

                        streamBuffer = &channel->samplePtr[channel->bufferPos];

                        UpdateStreamBuffer(channel);
                    }
                }
                break;
            }

            case CHANNEL_LOADING_STREAM: break;
        }
    }

    UnlockAudioDevice();
}

oboe::DataCallbackResult AudioDevice::onAudioReady(oboe::AudioStream *s, void *data, int32 len)
{
    if (s != stream)
        return oboe::DataCallbackResult::Stop;
    AudioDevice::ProcessAudioMixing(data, len * AUDIO_CHANNELS);
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