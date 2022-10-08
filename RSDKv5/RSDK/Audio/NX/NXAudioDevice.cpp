
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
    AudioDeviceBase::Release();
    UnlockAudioDevice();
}

void AudioDevice::InitAudioChannels()
{
    AudioDeviceBase::InitAudioChannels();
}
