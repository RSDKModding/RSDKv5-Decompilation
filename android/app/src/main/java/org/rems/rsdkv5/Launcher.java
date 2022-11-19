package org.rems.rsdkv5;

import static android.os.Build.VERSION.SDK_INT;

import android.Manifest;
import android.app.Activity;
import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.io.File;

public class Launcher extends AppCompatActivity {

    private static final int RSDK_VER = 5;
    private static String basePath = "RSDK/v" + RSDK_VER; // TODO: maybe do something cool and allow the user to set their own?
    private static final int REQUEST_CODE = 3000;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        boolean canRun = true;

        if (RSDK_VER == 5) {
            if (((ActivityManager)getSystemService(Context.ACTIVITY_SERVICE)).getDeviceConfigurationInfo().reqGlEsVersion < 0x20000) {
                canRun = false;
                new AlertDialog.Builder(this)
                        .setTitle("GLES 2.0 unsupported")
                        .setMessage("This device does not support GLES 2.0, which is required for running RSDKv5.")
                        .setNegativeButton("OK", (dialogInterface, i) -> quit(2))
                        .setCancelable(false)
                        .show();
            }
        }

        if (canRun) {
            if (!checkPermission()) {
                requestPermission(this, savedInstanceState);
            }
            else startGame();
        }
    }

    private void quit(int code) {
        finishAffinity();
        System.exit(code);
    }

    private void startGame() {
        // TODO:
        // if (base path file not written) {
        //    initial base path flow
        // }
        // else wait 3 seconds and show path

        String p = Environment.getExternalStorageDirectory().getAbsolutePath() + "/" + basePath;
        try {
            new File(p).mkdirs();
            new File(p + "../.nomedia").createNewFile();
        }
        catch (Exception e) {};

        if (!checkPermission()) {
            finishAffinity();
            System.exit(1);
        }
        Intent intent = new Intent(this, RSDK.class);
        intent.putExtra("basePath", basePath);
        startActivity(intent);
        finish();
    }

    // https://stackoverflow.com/questions/62782648/
    private boolean checkPermission() {
        if (SDK_INT >= Build.VERSION_CODES.R) {
            return Environment.isExternalStorageManager();
        } else {
            return ContextCompat.checkSelfPermission(this, android.Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED;
        }
    }

    private void requestPermission(Activity activity, Bundle bundle) {

        ActivityResultLauncher<Intent> launcher = registerForActivityResult(
                new ActivityResultContracts.StartActivityForResult(),
                result -> {
                    if (result.getResultCode() == Activity.RESULT_CANCELED) {
                        startGame();
                    }
                    else {
                        finishAffinity();
                        System.exit(1);
                    }
                });

        new AlertDialog.Builder(this)
                .setTitle("Permissions needed")
                .setMessage(
                        String.format("RSDK needs to be able to access %s.\nWould you like to grant read/write permissions?", basePath)
                )
                .setPositiveButton("Yes", (dialogInterface, i) -> {
                    if (SDK_INT >= Build.VERSION_CODES.R) {
                        try {
                            Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
                            intent.addCategory("android.intent.category.DEFAULT");
                            intent.setData(Uri.parse(String.format("package:%s",getApplicationContext().getPackageName())));
                            launcher.launch(intent);
                        } catch (Exception e) {
                            Intent intent = new Intent();
                            intent.setAction(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION);
                            launcher.launch(intent);
                        }
                    } else {
                        ActivityCompat.requestPermissions(activity, new String[]{Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE}, REQUEST_CODE);
                    }
                })
                .setNegativeButton("No", (dialogInterface, i) -> {
                    finishAffinity();
                    System.exit(1);
                })
                .setCancelable(false)
                .show();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        switch (requestCode) {
            case REQUEST_CODE:
                if (grantResults.length > 0) {
                    boolean READ_EXTERNAL_STORAGE = grantResults[0] == PackageManager.PERMISSION_GRANTED;
                    boolean WRITE_EXTERNAL_STORAGE = grantResults[1] == PackageManager.PERMISSION_GRANTED;

                    if (READ_EXTERNAL_STORAGE && WRITE_EXTERNAL_STORAGE) {
                        startGame();
                    } else {
                        finishAffinity();
                        System.exit(1);
                    }
                }
                break;
        }
    }

}
