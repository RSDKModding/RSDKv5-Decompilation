uint8 AudioDevice::contextInitialized;
oboe::Result AudioDevice::status = oboe::Result::OK;
oboe::AudioStream *AudioDevice::stream;
pthread_mutex_t AudioDevice::mutex;

// REAL LINE I WROTE
AudioDevice *AudioDevice::audioDevice;

bool AudioDevice::createStream() {
    oboe::AudioStreamBuilder builder;

    builder.setSampleRate(AUDIO_FREQUENCY)
            ->setFormat(oboe::AudioFormat::Float)
            ->setChannelCount(AUDIO_CHANNELS)
            ->setBufferCapacityInFrames(MIX_BUFFER_SIZE)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setDataCallback(audioDevice)
            ->setErrorCallback(audioDevice);

    oboe::Result result = builder.openStream(&stream);
    if (result != oboe::Result::OK) {
        PrintLog(PRINT_NORMAL, "Oboe: Failed to open stream: (%s)", convertToText(result));
        return false;
    }
    return true;
}

bool AudioDevice::shutdownStream() {
    if (stream->getState() != oboe::StreamState::Closed) {
        stream->stop();
        oboe::Result result = stream->close();
        if (result != oboe::Result::OK) {
            PrintLog(PRINT_NORMAL, "Oboe: Failed to shutdown stream: (%s)", convertToText(result));
            return false;
        }
    }
    return true;
}

bool32 AudioDevice::Init()
{
    if (!contextInitialized) {
        contextInitialized = true;
        InitAudioChannels();
        audioDevice = new AudioDevice();
    }

    if (!createStream())
        return false;

    stream->requestStart();
    return true;
}

void AudioDevice::InitAudioChannels()
{
    pthread_mutex_init(&mutex, NULL);

    AudioDeviceBase::InitAudioChannels();
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

void AudioDevice::onErrorAfterClose(oboe::AudioStream *s, oboe::Result error) {
    LockAudioDevice();
    if (error == oboe::Result::ErrorDisconnected) {
        // Clean up and create a new audio stream
        shutdownStream();
        createStream();
        stream->requestStart();
    }
    UnlockAudioDevice();
}

void AudioDevice::Release()
{
    LockAudioDevice();
    shutdownStream();
    UnlockAudioDevice();
    AudioDeviceBase::Release();
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