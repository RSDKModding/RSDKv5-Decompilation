#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio/miniaudio.h>

uint8 AudioDevice::contextInitialized;
ma_device AudioDevice::device;

bool32 AudioDevice::Init()
{
    if (!contextInitialized) {
        contextInitialized = true;
        InitAudioChannels();
    }

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32;   // Set to ma_format_unknown to use the device's native format.
    config.playback.channels = 2;               // Set to 0 to use the device's native channel count.
    config.periodSizeInFrames = MIX_BUFFER_SIZE;
    config.sampleRate        = AUDIO_FREQUENCY;           // Set to 0 to use the device's native sample rate.
    config.dataCallback      = AudioCallback;   // This function will be called when miniaudio needs more data.

    ma_result result = ma_device_init(NULL, &config, &device);
    if (result != MA_SUCCESS) {
        PrintLog(PRINT_NORMAL, "[MA] Initializing device failed: %d", result);
        return false;
    }

    result = ma_device_start(&device);
    if (result != MA_SUCCESS) {
        PrintLog(PRINT_NORMAL, "[MA] Starting device failed: %d", result);
        return false;
    }

    return true;
}

void AudioDevice::Release()
{
    ma_device_uninit(&device);
}

void AudioDevice::InitAudioChannels() { AudioDeviceBase::InitAudioChannels(); }

void RSDK::AudioDevice::AudioCallback(ma_device* device, void *output, const void *input, ma_uint32 frameCount)
{
    (void)input;
    (void)device;

    AudioDevice::ProcessAudioMixing(output, frameCount * AUDIO_CHANNELS);
}
