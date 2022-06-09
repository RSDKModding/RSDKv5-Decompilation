#include <RSDK/Core/RetroEngine.hpp>
#include <main.hpp>
#include <pthread.h>

bool32 firstlaunch = true;
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
    // set focus stuff here
}
JNIEXPORT void jnifuncN(nativeOnResume, RSDKv5) {}
JNIEXPORT void jnifunc(nativeOnStart, RSDKv5, jstring basepath)
{
    char buffer[0x200];
    strcpy(buffer, env->GetStringUTFChars((jstring)basepath, NULL));
    RSDK::SKU::SetUserFileCallbacks(buffer, NULL, NULL);
    pthread_create(&mainthread, NULL, &threadCallback, NULL);
}
JNIEXPORT void jnifuncN(nativeOnStop, RSDKv5)
{
    // set some sort of exit flag in renderdev
}

JNIEXPORT void jnifunc(nativeSetSurface, RSDKSurface, jobject surface)
{
    if (surface) {
        RSDK::RenderDevice::window = ANativeWindow_fromSurface(env, surface);
        if (firstlaunch) {
            firstlaunch = false;
        }
    }
    else {
        ANativeWindow_release(RSDK::RenderDevice::window);
    }
}