#include <RSDK/Core/RetroEngine.hpp>
#include <main.hpp>
#include <pthread.h>

bool32 launched = false;
pthread_t mainthread;
int returnCode;

void *threadCallback(void *a)
{
    while (!RSDK::RenderDevice::window)
        ;
    return (void *)RSDK_main(0, NULL, (void *)RSDK::LinkGameLogic);
}

JNIEXPORT void jnifuncN(nativeOnPause, RSDKv5)
{
#if RETRO_REV02
    if (RSDK::SKU::userCore)
        RSDK::SKU::userCore->focusState = 1;
#endif
    RSDK::videoSettings.windowState = RSDK::WINDOWSTATE_UNINITIALIZED;
    RSDK::RenderDevice::Release(true);
}
JNIEXPORT void jnifuncN(nativeOnResume, RSDKv5) {
#if RETRO_REV02
    if (RSDK::SKU::userCore)
        RSDK::SKU::userCore->focusState = 0;
#endif
    if (RSDK::RenderDevice::isInitialized)
        RSDK::RenderDevice::Release(true);
    if (RSDK::RenderDevice::isRunning)
        RSDK::RenderDevice::Init();
}
JNIEXPORT void jnifunc(nativeOnStart, RSDKv5, jstring basepath)
{
    if (!launched) {
        char buffer[0x200];
        strcpy(buffer, env->GetStringUTFChars((jstring)basepath, NULL));
        RSDK::SKU::SetUserFileCallbacks(buffer, NULL, NULL);
        RSDK::RenderDeviceBase::isRunning = false;
        pthread_create(&mainthread, NULL, &threadCallback, NULL);
        launched = true;
    }
}
JNIEXPORT void jnifuncN(nativeOnStop, RSDKv5)
{
    RSDK::RenderDevice::isRunning = false;
    pthread_join(mainthread, NULL);
}

JNIEXPORT void jnifunc(nativeSetSurface, RSDKSurface, jobject surface)
{
    RSDK::videoSettings.windowState = RSDK::WINDOWSTATE_UNINITIALIZED;
    if (RSDK::RenderDevice::isInitialized)
        RSDK::RenderDevice::Release(true);

    if (surface) {
        RSDK::RenderDevice::window = ANativeWindow_fromSurface(env, surface);
        if (RSDK::RenderDevice::isRunning)
            RSDK::RenderDevice::Init();
    }
    else {
        ANativeWindow_release(RSDK::RenderDevice::window);
    }
}