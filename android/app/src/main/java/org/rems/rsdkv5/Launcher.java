package org.rems.rsdkv5;

import static android.os.Build.VERSION.SDK_INT;

import android.Manifest;
import android.app.Activity;
import android.app.ActivityManager;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.os.Environment;
import android.provider.DocumentsContract;
import android.provider.Settings;
import android.widget.EditText;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.io.File;
import java.util.concurrent.TimeUnit;

import kotlin.jvm.internal.Lambda;

public class Launcher extends AppCompatActivity {

    private static final int RSDK_VER = 5;
    private static String basePath = "RSDK/v" + RSDK_VER; // TODO: maybe do something cool and allow the user to set their own?
    private static final int REQUEST_CODE = 3000;

    private static ActivityResultLauncher<Intent> folderLauncher = null;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        folderLauncher = registerForActivityResult(
                new ActivityResultContracts.StartActivityForResult(),
                result -> {
                    if (result.getResultCode() == Activity.RESULT_OK && result.getData() != null) {
                        basePath = result.getData().getData().getPath();
                    }
                    startGame();
                }
        );

        boolean canRun = true;

        if (RSDK_VER == 5) {
            if (((ActivityManager)getSystemService(Context.ACTIVITY_SERVICE)).getDeviceConfigurationInfo().reqGlEsVersion < 0x20000) {
                canRun = false;
                new AlertDialog.Builder(this)
                        .setTitle("GLES 2.0 unsupported")
                        .setMessage("This device does not support GLES 2.0, which is required for running RSDKv5.")
                        .setNegativeButton("OK", (dialogInterface, i) -> {
                            dialogInterface.cancel();
                            quit(2);
                        })
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

    static class DialogTimer extends CountDownTimer {
        public AlertDialog alert;

        public DialogTimer(long millisInFuture, long countDownInterval) {
            super(millisInFuture, countDownInterval);
        }

        @Override
        public void onTick(long l) {
            alert.setMessage(String.format(
                    "Game will start in %s in %d seconds...",
                    basePath,
                    TimeUnit.MILLISECONDS.toSeconds(l) + 1
            ));
        }

        @Override
        public void onFinish() {
            alert.getButton(AlertDialog.BUTTON_POSITIVE).callOnClick();
        }
    }

    private void startGame() {

        AlertDialog baseAlert = null;

        DialogTimer timer = new DialogTimer(6000, 100);

        baseAlert = new AlertDialog.Builder(this)
                .setTitle("Game starting")
                .setPositiveButton("Start", (dialog, i) -> {
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
                )
                .setNeutralButton("Change Path", (dialog, i) -> {
                    timer.cancel();
                    folderPicker();
                })
                .create();

        baseAlert.setOnShowListener(dialog -> {
            timer.alert = (AlertDialog)dialog;
            timer.start();
        });

        baseAlert.show();
    }

    // https://stackoverflow.com/questions/62782648/
    private boolean checkPermission() {
        if (SDK_INT >= Build.VERSION_CODES.R) {
            return Environment.isExternalStorageManager();
        } else {
            return ContextCompat.checkSelfPermission(this, android.Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED;
        }
    }

    private void folderPicker() {
        if (SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            folderLauncher.launch(new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE).putExtra(DocumentsContract.EXTRA_INITIAL_URI,
                        Uri.fromFile(new File(Environment.getExternalStorageDirectory().getAbsolutePath() + "/" + basePath))));
        }
        else {
            EditText txt = new EditText(this);
            txt.setText(basePath);
            new AlertDialog.Builder(this)
                    .setTitle("Path to use")
                    .setMessage("Please type in the path to use, starting from the SD card root.")
                    .setView(txt)
                    .setPositiveButton("OK", (dialog, i) -> {
                        basePath = txt.getText().toString();
                        startGame();
                    })
                    .setNegativeButton("Cancel", (dialog, i) -> {
                        dialog.cancel();
                        startGame();
                    });
        }
    }

    private void requestPermission(Activity activity, Bundle bundle) {

        ActivityResultLauncher<Intent> launcher = registerForActivityResult(
                new ActivityResultContracts.StartActivityForResult(),
                    result -> startGame()
                );

        new AlertDialog.Builder(this)
                .setTitle("Permissions needed")
                .setMessage(
                        String.format("RSDK needs to be able to access %s.\nWould you like to grant read/write permissions?", basePath)
                )
                .setPositiveButton("Yes", (dialogInterface, i) -> {
                    if (SDK_INT >= Build.VERSION_CODES.R - 1) {
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
                    quit(1);
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
