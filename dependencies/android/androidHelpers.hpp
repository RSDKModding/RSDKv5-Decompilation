#ifndef ANDROIDHELPERS_H
#define ANDROIDHELPERS_H
// god i need to sort these out soon
#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/window.h>
#include <pthread.h>
#include <paddleboat/paddleboat.h>

#include <game-activity/native_app_glue/android_native_app_glue.h>

struct JNISetup {
    jclass clazz; // activity class
    jobject thiz; // activity object
    JNIEnv *env;  // jni env
};
struct JNISetup *GetJNISetup();

typedef struct android_app android_app;
extern android_app *app;

class GameActivity;
class GameActivityKeyEvent;

#define jnifunc(name, class, ...) JNICALL Java_com_decomp_rsdkv5_##class##_##name(JNIEnv *env, jclass cls, __VA_ARGS__)
#define jnifuncN(name, class) JNICALL Java_com_decomp_rsdkv5_##class##_##name(JNIEnv *env, jclass cls)
// the lone JNI func
extern "C" JNIEXPORT void jnifunc(nativeOnTouch, RSDKv5, jint finger, jint action, jfloat x, jfloat y);

void AndroidCommandCallback(android_app *app, int32 cmd);
bool AndroidKeyDownCallback(GameActivity *activity, const GameActivityKeyEvent *event);
bool AndroidKeyUpCallback(GameActivity *activity, const GameActivityKeyEvent *event);

#endif // ANDROID_ANDROIDHELPERS_H
