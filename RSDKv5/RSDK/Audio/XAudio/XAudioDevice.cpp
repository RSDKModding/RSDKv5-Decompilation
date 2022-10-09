

// allows generic string literal stuff
#include <tchar.h>

uint8 AudioDevice::contextInitialized;

IXAudio2 *AudioDevice::audioContext;
IXAudio2MasteringVoice *AudioDevice::masteringVoice;
IXAudio2SourceVoice *AudioDevice::sourceVoice;
AudioDeviceCallback AudioDevice::voiceCallback;
AudioEngineCallback AudioDevice::engineCallback;

RTL_CRITICAL_SECTION AudioDevice::criticalSection;

int32 AudioDevice::mixBufferID = 0;
float AudioDevice::mixBuffer[3][MIX_BUFFER_SIZE];

bool32 AudioDevice::Init()
{
    if (!contextInitialized) {
        contextInitialized = true;
        InitAudioChannels();

        CoInitializeEx(NULL, 0);
    }

    audioState = false;
    if (!audioContext) {
        if (SUCCEEDED(InitContext())) {
            if (SUCCEEDED(audioContext->CreateMasteringVoice(&masteringVoice, 0, 0, 0, NULL, NULL))) {
                WAVEFORMATEX format;
                format.cbSize          = 0;
                format.wBitsPerSample  = sizeof(SAMPLE_FORMAT) * 8;
                format.nChannels       = AUDIO_CHANNELS;
                format.nBlockAlign     = (format.nChannels * format.wBitsPerSample) / 8;
                format.wFormatTag      = WAVE_FORMAT_IEEE_FLOAT;
                format.nSamplesPerSec  = AUDIO_FREQUENCY;
                format.nAvgBytesPerSec = AUDIO_FREQUENCY * AUDIO_CHANNELS * sizeof(SAMPLE_FORMAT);

                if (SUCCEEDED(audioContext->CreateSourceVoice(&sourceVoice, &format, 0, 2.0, &voiceCallback, NULL, NULL))) {
                    sourceVoice->Start(0, XAUDIO2_COMMIT_NOW);
                    InitMixBuffer();
                    audioState = true;
                }
                else {
#if RETRO_RENDERDEVICE_DIRECTX9 || RETRO_RENDERDEVICE_DIRECTX11
                    if (videoSettings.windowed) {
                        MessageBox(RenderDevice::windowHandle, _T("Unable to create source voice. Sound will not be available during this session."),
                                   _T("AudioDevice::Init"), MB_OK);
                    }
#endif
                }
            }
            else {
#if RETRO_RENDERDEVICE_DIRECTX9 || RETRO_RENDERDEVICE_DIRECTX11
                if (videoSettings.windowed) {
                    MessageBox(RenderDevice::windowHandle, _T("Unable to create mastering voice. Sound will not be available during this session."),
                               _T("AudioDevice::Init"), MB_OK);
                }
#endif

                if (audioContext) {
                    audioContext->Release();
                    audioContext = NULL;
                }
            }
        }
        else {
#if RETRO_RENDERDEVICE_DIRECTX9 || RETRO_RENDERDEVICE_DIRECTX11
            if (videoSettings.windowed) {
                MessageBox(RenderDevice::windowHandle,
                           _T("Error Initializing XAudio2.\n"
                              "You may need to install the Jun 2010 DirectX update to have the XAudio Drivers.\n"
                              "Sound will not be available during this session."),
                           _T("AudioDevice::Init"), MB_OK);
            }
#endif
        }
    }

    return true;
}

void AudioDevice::Release()
{
    if (sourceVoice)
        sourceVoice->DestroyVoice();
    if (masteringVoice)
        masteringVoice->DestroyVoice();
    if (audioContext)
        audioContext->Release();

    AudioDeviceBase::Release();

    CoUninitialize();
    DeleteCriticalSection(&AudioDevice::criticalSection);
}

void AudioDevice::FrameInit()
{
    if (audioState > 2 && --audioState == 2) {
        if (sourceVoice) {
            sourceVoice->DestroyVoice();
            sourceVoice = NULL;
        }

        if (masteringVoice) {
            masteringVoice->DestroyVoice();
            masteringVoice = NULL;
        }

        audioContext->StopEngine();

        Sleep(1000);

        if (audioContext)
            audioContext->Release();
        audioContext = NULL;

        Init();
    }
}

void AudioDevice::HandleStreamLoad(ChannelInfo *channel, bool32 async)
{
    if (async) {
        DWORD threadId;
        HANDLE thread = CreateThread(NULL, 0, LoadStreamASync, channel, 0, &threadId);
        CloseHandle(thread);
    }
    else {
        LoadStream(channel);
    }
}

HRESULT AudioDevice::InitContext()
{
    IXAudio2 *context = nullptr;

    HRESULT res = XAudio2Create(&context, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (SUCCEEDED(res)) {
        res = context->StartEngine();
        if (FAILED(res))
            context->Release();
        else
            audioContext = context;
    }

    return res;
}

void AudioDevice::InitAudioChannels()
{
    InitializeCriticalSection(&AudioDevice::criticalSection);

    AudioDeviceBase::InitAudioChannels();
}

void AudioDevice::InitMixBuffer()
{
    void *mixBuffer = AudioDevice::mixBuffer[AudioDevice::mixBufferID];

    XAUDIO2_BUFFER buffer;
    memset(&buffer, 0, sizeof(buffer));
    buffer.pAudioData = (BYTE *)mixBuffer;
    buffer.pContext   = 0;
    buffer.AudioBytes = MIX_BUFFER_SIZE * sizeof(SAMPLE_FORMAT);

    AudioDevice::mixBufferID = ((AudioDevice::mixBufferID) + 1) % 3;

    AudioDevice::sourceVoice->SubmitSourceBuffer(&buffer, NULL);
    AudioDevice::audioContext->RegisterForCallbacks(&engineCallback);
}

void AudioEngineCallback::OnCriticalError(HRESULT Error) { AudioDevice::audioState = 30; }

void AudioDeviceCallback::OnBufferEnd(void *pBufferContext)
{
    XAUDIO2_VOICE_STATE state;
    AudioDevice::sourceVoice->GetState(&state);

    if (state.BuffersQueued < 2) {
        void *mixBuffer = AudioDevice::mixBuffer[AudioDevice::mixBufferID];

        LockAudioDevice();
        AudioDevice::ProcessAudioMixing(mixBuffer, MIX_BUFFER_SIZE);
        UnlockAudioDevice();

        XAUDIO2_BUFFER buffer;
        memset(&buffer, 0, sizeof(buffer));
        buffer.pAudioData = (BYTE *)mixBuffer;
        buffer.pContext   = 0;
        buffer.AudioBytes = MIX_BUFFER_SIZE * sizeof(SAMPLE_FORMAT);

        AudioDevice::mixBufferID = ((AudioDevice::mixBufferID) + 1) % 3;

        AudioDevice::sourceVoice->SubmitSourceBuffer(&buffer, NULL);
    }
}