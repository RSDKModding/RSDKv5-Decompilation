#define LockAudioDevice()
#define UnlockAudioDevice()

#include <thread>

namespace RSDK
{
class AudioDevice : public AudioDeviceBase
{
public:
    static ma_device device;

    static bool32 Init();
    static void Release();

    static void FrameInit() {}

    inline static void HandleStreamLoad(ChannelInfo *channel, bool32 async)
    {
        if (async) {
            std::thread thread(LoadStream, channel);
            thread.detach();
        }
        else
            LoadStream(channel);
    }

private:
    static uint8 contextInitialized;

    static void InitAudioChannels();

    static void AudioCallback(ma_device* device, void *output, const void *input, ma_uint32 frameCount);
};
} // namespace RSDK