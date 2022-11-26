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

#include <swappy/swappyGL.h>
#include <swappy/swappyGL_extra.h>

struct JNISetup {
    jclass clazz; // activity class
    jobject thiz; // activity object
    JNIEnv *env;  // jni env
};
struct JNISetup *GetJNISetup();

typedef struct android_app android_app;
extern android_app *app;
extern jmethodID getFD;
extern jmethodID writeLog;

extern jmethodID showLoading;
extern jmethodID hideLoading;
extern jmethodID setLoading;
extern jmethodID setPixSize;

class GameActivity;
class GameActivityKeyEvent;

#define jniname(name, class) Java_org_rems_rsdkv5_##class##_##name
#define jnifunc(name, class, ...) jniname(name, class)(JNIEnv *env, jclass cls, __VA_ARGS__)
#define jnifuncN(name, class) jniname(name, class)(JNIEnv *env, jclass cls)

extern "C" JNIEXPORT void JNICALL jnifunc(nativeOnTouch, RSDK, jint finger, jint action, jfloat x, jfloat y);
extern "C" JNIEXPORT jbyteArray JNICALL jnifunc(nativeLoadFile, RSDK, jstring file);

void AndroidCommandCallback(android_app *app, int32 cmd);
bool AndroidKeyDownCallback(GameActivity *activity, const GameActivityKeyEvent *event);
bool AndroidKeyUpCallback(GameActivity *activity, const GameActivityKeyEvent *event);

void ShowLoadingIcon();
void HideLoadingIcon();
void SetLoadingIcon();

#endif // ANDROID_ANDROIDHELPERS_H
