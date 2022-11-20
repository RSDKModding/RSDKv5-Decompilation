package org.rems.rsdkv5;

import static android.os.Build.VERSION.SDK_INT;

import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;

import com.google.androidgamesdk.GameActivity;


import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;

public class RSDK extends GameActivity {
    static {
        //https://developer.android.com/ndk/guides/cpp-support#shared_runtimes
        System.loadLibrary("c++_shared");
        System.loadLibrary("RetroEngine");
    }

    private static Uri basePath = null;
    private static OutputStream log = null;

    private void hideSystemUI() {

        if (SDK_INT >= Build.VERSION_CODES.P) {
            getWindow().getAttributes().layoutInDisplayCutoutMode
                    = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
        }

        View decorView = getWindow().getDecorView();
        WindowInsetsControllerCompat controller = new WindowInsetsControllerCompat(getWindow(),
                decorView);
        controller.hide(WindowInsetsCompat.Type.systemBars());
        controller.hide(WindowInsetsCompat.Type.displayCutout());
        controller.setSystemBarsBehavior(
                WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        if (basePath == null) {
            if (getIntent().getData() != null) {
                basePath = getIntent().getData();
            }
            // elif file exists read file
            else basePath = new Uri.Builder().path("/tree/primary:RSDK/v5").build();;
        }

        WindowCompat.setDecorFitsSystemWindows(getWindow(), false);
        hideSystemUI();
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS, WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        try {
            ParcelFileDescriptor.adoptFd(getFD("log.txt", "a")).close();
        } catch (IOException e) {
            e.printStackTrace();
        }

        Uri uri = basePath.buildUpon().appendEncodedPath(
                "document/" + Uri.encode(basePath.getLastPathSegment() + "/log.txt")).build();

        try {
            if (log == null)
                log = getContentResolver().openOutputStream(uri, "wa");
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        }

        super.onCreate(savedInstanceState);
    }

    @Override
    protected void onDestroy() {
        setResult(RESULT_OK);
        try {
            log.close();
        } catch (Exception e) {
        }
        finish();
        super.onDestroy();
    }

    @Override
    protected void onResume() {
        WindowCompat.setDecorFitsSystemWindows(getWindow(), false);
        hideSystemUI();
        super.onResume();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        // this is mostly stolen from sdl LOL
        // this func survives android v3 because it's just  easier to handle
        final int pointerCount = event.getPointerCount();
        int action = event.getActionMasked();
        int i = -1;

        switch(action) {
            case MotionEvent.ACTION_MOVE:
                for (i = 0; i < pointerCount; i++) {
                    int finger = event.getPointerId(i);
                    float x = event.getX(i) / mSurfaceView.getWidth();
                    float y = event.getY(i) / mSurfaceView.getHeight();

                    nativeOnTouch(finger, action, x, y);
                }
                break;

            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_DOWN:
                // Primary pointer up/down, the index is always zero
                i = 0;
                /* fallthrough */
            case MotionEvent.ACTION_POINTER_UP:
            case MotionEvent.ACTION_POINTER_DOWN: {
                // Non primary pointer up/down
                if (i == -1) {
                    i = event.getActionIndex();
                }

                int finger = event.getPointerId(i);
                float x = event.getX(i) / mSurfaceView.getWidth();
                float y = event.getY(i) / mSurfaceView.getHeight();

                nativeOnTouch(finger, action, x, y);
            }
            break;

            case MotionEvent.ACTION_CANCEL:
                for (i = 0; i < pointerCount; i++) {
                    int finger = event.getPointerId(i);
                    float x = event.getX(i) / mSurfaceView.getWidth();
                    float y = event.getY(i) / mSurfaceView.getHeight();

                    nativeOnTouch(finger, MotionEvent.ACTION_UP, x, y);
                }
                break;

            default:
                break;
        }

        return true;
    }

    public static native void nativeOnTouch(int fingerID, int action, float x, float y);

    public int getFD(String filepath, String mode) {
        String useMode = mode;
        try {
            // DocumentFile docfile = DocumentFile.fromTreeUri(this, basePath).findFile(filepath);
            // Uri uri = docfile.getUri();
            Uri uri = basePath.buildUpon().appendEncodedPath(
                    "document/" + Uri.encode(basePath.getLastPathSegment() + "/" + filepath)).build();
            useMode = mode;
            switch (mode.charAt(0)) {
                case 'a': useMode = "wa"; break;
                case 'r': useMode = "r"; break;
                default: useMode = "w"; break;
            }
            ParcelFileDescriptor parcel = getContentResolver().openFileDescriptor(uri, useMode); // i don't think mode is actually required
            int fd = parcel.dup().detachFd();
            parcel.close();
            return fd;
        } catch (Exception e) {
            try {
                if (!useMode.equals("r")) {
                    if (Launcher.instance == null) {
                        Log.w("RSDKv5-J", String.format("Cannot create file %s; game opened without launcher", filepath));
                        return 0;
                    }
                    Uri uri = Launcher.instance.createFile(filepath);
                    ParcelFileDescriptor parcel = getContentResolver().openFileDescriptor(uri, useMode); // i don't think mode is actually required
                    int fd = parcel.dup().detachFd();
                    parcel.close();
                    return fd;
                }
            }
            catch (Exception ex) {
                ex.printStackTrace();
            }
        }
        return 0;
    }

    public void writeLog(String string, int as) {
        Log.println(as, "RSDKv5", string);
        try {
            log.write(string.getBytes(StandardCharsets.UTF_8));
            log.flush();
        } catch (Exception e) {};
    }
}
