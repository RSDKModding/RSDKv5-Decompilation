
uint8 AudioDevice::contextInitialized;

SDL_AudioDeviceID AudioDevice::device;
SDL_AudioSpec AudioDevice::deviceSpec;

bool32 AudioDevice::Init()
{
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    if (!contextInitialized) {
        contextInitialized = true;
        InitAudioChannels();
    }

    SDL_AudioSpec want;
    want.freq     = AUDIO_FREQUENCY;
    want.format   = AUDIO_F32SYS;
    want.samples  = MIX_BUFFER_SIZE / AUDIO_CHANNELS;
    want.channels = AUDIO_CHANNELS;
    want.callback = AudioCallback;

    audioState = false;
    if ((device = SDL_OpenAudioDevice(nullptr, 0, &want, &deviceSpec, SDL_AUDIO_ALLOW_SAMPLES_CHANGE)) > 0) {
        SDL_PauseAudioDevice(device, SDL_FALSE);
        audioState = true;
    }
    else {
        PrintLog(PRINT_NORMAL, "ERROR: Unable to open audio device!");
        PrintLog(PRINT_NORMAL, "ERROR: %s", SDL_GetError());
    }

    return true;
}

void AudioDevice::Release()
{
    LockAudioDevice();
    AudioDeviceBase::Release();
    UnlockAudioDevice();

    SDL_CloseAudioDevice(AudioDevice::device);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void AudioDevice::InitAudioChannels()
{
    AudioDeviceBase::InitAudioChannels();
}

void AudioDevice::AudioCallback(void *data, uint8 *stream, int32 len)
{
    (void)data; // Unused

    AudioDevice::ProcessAudioMixing(stream, len / sizeof(SAMPLE_FORMAT));
}
