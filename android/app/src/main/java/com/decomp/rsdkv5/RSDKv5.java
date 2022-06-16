package com.decomp.rsdkv5;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.os.Environment;
import android.view.WindowManager;
import android.widget.RelativeLayout;

import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;

import java.io.File;

public class RSDKv5 extends Activity {
    RSDKSurface surface;
    RelativeLayout layout;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        surface = new RSDKSurface(getApplication());

        layout = new RelativeLayout(this);
        layout.addView(surface);

        hideSystemBars();
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS, WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        setContentView(layout);
        nativeOnStart(getBasePath());
    }

    private void hideSystemBars() {
        WindowInsetsControllerCompat windowInsetsController =
                ViewCompat.getWindowInsetsController(getWindow().getDecorView());
        if (windowInsetsController == null) {
            return;
        }

        windowInsetsController.setSystemBarsBehavior(WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);

        windowInsetsController.hide(WindowInsetsCompat.Type.systemBars());
    }


    @Override
    protected void onPause() {
        super.onPause();
        nativeOnPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        surface.handleResume();
        hideSystemBars();
        nativeOnResume();
    }

    protected void onDestroy() {
        nativeOnStop();
        super.onDestroy();
    }

    public String getBasePath() {
        Context c = getApplicationContext();
        requestPermissions(new String[]{Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE}, 0);
        String p = Environment.getExternalStorageDirectory().getAbsolutePath() + "/RSDK/v5";
        //getExternalStorageDirectory is deprecated. I do not care.
        //rmg 20220610 i'm a changed woman EDIT: nvm not yet
        new File(p).mkdirs();
        return p + "/";
    }

    public static native void nativeOnStart(String basepath);
    public static native void nativeOnResume();
    public static native void nativeOnPause();
    public static native void nativeOnStop();

}
