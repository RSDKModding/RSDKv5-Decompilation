#ifndef ANDROIDHELPERS_H
#define ANDROIDHELPERS_H
#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#define jnifunc(name, class, ...) JNICALL Java_com_decomp_rsdkv5_##class##_##name(JNIEnv *env, jclass cls, __VA_ARGS__)
#define jnifuncN(name, class) JNICALL Java_com_decomp_rsdkv5_##class##_##name(JNIEnv *env, jclass cls)


extern "C" JNIEXPORT void jnifuncN(nativeOnPause, RSDKv5);
extern "C" JNIEXPORT void jnifuncN(nativeOnResume, RSDKv5);
extern "C" JNIEXPORT void jnifunc(nativeOnStart, RSDKv5, jstring basepath);
extern "C" JNIEXPORT void jnifuncN(nativeOnStop, RSDKv5);
extern "C" JNIEXPORT void jnifunc(nativeSetSurface, RSDKSurface, jobject surface);


#endif // ANDROID_ANDROIDHELPERS_H
