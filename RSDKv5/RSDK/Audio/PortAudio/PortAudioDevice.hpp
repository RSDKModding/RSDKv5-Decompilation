#define LockAudioDevice()
#define UnlockAudioDevice()

#include <thread>

namespace RSDK
{
class AudioDevice : public AudioDeviceBase
{
public:
    static PaStream *stream;

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

    static int AudioCallback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo,
                              PaStreamCallbackFlags statusFlags, void *userData);
};
} // namespace RSDK