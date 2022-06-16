
uint8 AudioDevice::contextInitialized;
aaudio_result_t AudioDevice::status = AAUDIO_OK;
AAudioStream *AudioDevice::stream;
pthread_mutex_t AudioDevice::mutex;

bool32 AudioDevice::Init()
{
    if (!contextInitialized) {
        contextInitialized = true;
        InitAudioChannels();
    }

    AAudioStreamBuilder *builder;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK)
        return false;

    AAudioStreamBuilder_setSampleRate(builder, AUDIO_FREQUENCY);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setChannelCount(builder, AUDIO_CHANNELS);
    AAudioStreamBuilder_setBufferCapacityInFrames(builder, MIX_BUFFER_SIZE);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setDataCallback(builder, AudioCallback, NULL);
    AAudioStreamBuilder_setErrorCallback(builder, ErrorCallback, NULL);

    result = AAudioStreamBuilder_openStream(builder, &stream);
    if (result != AAUDIO_OK)
        return false;

    AAudioStreamBuilder_delete(builder);
    AAudioStream_requestStart(stream);

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
                    SAMPLE_FORMAT sample = (sfxBuffer[1] - *sfxBuffer) * speedMixAmounts[speedPercent >> 6] + *sfxBuffer;

                    speedPercent += channel->speed;
                    sfxBuffer += speedPercent >> 16;
                    channel->bufferPos += speedPercent >> 16;
                    speedPercent &= 0xFFFF;

                    curStreamF[0] += sample * panR;
                    curStreamF[1] += sample * panL;
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

                    curStreamF[0] += panR * *streamBuffer;
                    curStreamF[1] += panL * streamBuffer[next];
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

aaudio_data_callback_result_t AudioDevice::AudioCallback(AAudioStream *s, void *d, void *data, int32 len)
{
    (void)d; // Unused

    if (s != stream)
        return AAUDIO_CALLBACK_RESULT_STOP;
    AudioDevice::ProcessAudioMixing(data, len * AUDIO_CHANNELS);
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void AudioDevice::ErrorCallback(AAudioStream *s, void *d, aaudio_result_t error)
{
    (void)s;
    (void)d;
    status = error;
}

void AudioDevice::Release()
{
    AAudioStream_close(stream);
    if (vorbisInfo) {
        vorbis_deinit(vorbisInfo);
        if (!vorbisInfo->alloc.alloc_buffer)
            free(vorbisInfo);
    }
    vorbisInfo = NULL;
    pthread_mutex_destroy(&mutex);
};

void AudioDevice::FrameInit()
{
    if (status != AAUDIO_OK) {
        AAudioStream_requestStop(stream);
        AAudioStream_close(stream);
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