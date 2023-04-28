uint8 AudioDevice::contextInitialized;
PaStream *AudioDevice::stream;

bool32 AudioDevice::Init()
{
    if (!contextInitialized) {
        contextInitialized = true;
        InitAudioChannels();
    }

    PaError result;
    result = Pa_Initialize();
    if (result != paNoError) {
        PrintLog(PRINT_NORMAL, "[PA] Initialization failed: %s", Pa_GetErrorText(result));
        return false;
    }
    result = Pa_OpenDefaultStream(&stream, 0, AUDIO_CHANNELS, paFloat32, AUDIO_FREQUENCY, MIX_BUFFER_SIZE, AudioCallback, nullptr);
    if (result != paNoError) {
        PrintLog(PRINT_NORMAL, "[PA] Opening stream failed: %s", Pa_GetErrorText(result));
        return false;
    }
    result = Pa_StartStream(stream);
    if (result != paNoError) {
        PrintLog(PRINT_NORMAL, "[PA] Starting stream failed: %s", Pa_GetErrorText(result));
        return false;
    }
    return true;
}

void AudioDevice::Release()
{
    Pa_StopStream(stream);
    Pa_Terminate();
}

void AudioDevice::InitAudioChannels() { AudioDeviceBase::InitAudioChannels(); }

int RSDK::AudioDevice::AudioCallback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo,
                                     PaStreamCallbackFlags statusFlags, void *userData)
{
    (void)input;
    (void)timeInfo;
    (void)statusFlags;
    (void)userData;

    AudioDevice::ProcessAudioMixing(output, frameCount * AUDIO_CHANNELS);
    return 0;
}
