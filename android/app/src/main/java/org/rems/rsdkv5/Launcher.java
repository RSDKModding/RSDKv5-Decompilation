package org.rems.rsdkv5;

import static android.os.Build.VERSION.SDK_INT;

import android.Manifest;
import android.app.Activity;
import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.content.UriPermission;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.provider.DocumentsContract;
import android.util.Log;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.documentfile.provider.DocumentFile;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.concurrent.TimeUnit;

public class Launcher extends AppCompatActivity {

    private static final int RSDK_VER = 5;
    private static Uri basePath = null;
    private static final int REQUEST_CODE = 3000;

    public static Launcher instance = null;

    private File basePathStore;

    private static ActivityResultLauncher<Intent> folderLauncher = null;
    private static ActivityResultLauncher<Intent> gameLauncher = null;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        basePathStore = new File(getFilesDir(), "basePathStore");

        folderLauncher = registerForActivityResult(
                new ActivityResultContracts.StartActivityForResult(),
                result -> {
                    if (result.getResultCode() == Activity.RESULT_OK && result.getData() != null) {
                        basePath = result.getData().getData();
                    }
                    try {
                        Log.i("hi", String.format("%d", getContentResolver().openInputStream(
                                DocumentFile.fromTreeUri(this, basePath).findFile("Settings.ini").getUri()).read()));
                    } catch (FileNotFoundException e) {
                        e.printStackTrace();
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                    startGame(true);
                }
        );

        gameLauncher = registerForActivityResult(
                new ActivityResultContracts.StartActivityForResult(),
                result -> {
                    quit(0);
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
            else startGame(false);
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
                    basePath.getPath(),
                    TimeUnit.MILLISECONDS.toSeconds(l) + 1
            ));
        }

        @Override
        public void onFinish() {
            alert.getButton(AlertDialog.BUTTON_POSITIVE).callOnClick();
        }
    }

    private void refreshStore() {
        if (basePathStore.exists()) {
            try {
                BufferedReader reader = new BufferedReader(new FileReader(basePathStore));
                String uri = reader.readLine();
                if (uri != null) {
                    basePath = Uri.parse(uri);
                }
                reader.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }

        if (basePath != null) {
            try {
                FileWriter writer = new FileWriter(basePathStore);
                writer.write(basePath.toString() + "\n");
                writer.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    private void startGame(boolean fromPicker) {

        refreshStore();

        boolean found = false;
        if (basePath != null) {
            for (UriPermission uriPermission : getContentResolver().getPersistedUriPermissions()) {
                if (uriPermission.getUri().toString().matches(basePath.toString())) {
                    found = true;
                    break;
                }
            }
        }

        if (!found && !fromPicker) {
            new AlertDialog.Builder(this)
                    .setTitle("Path confirmation")
                    .setMessage(basePath != null ?
                            "Please reconfirm the path the game should run in." :
                            "Please set the path the game should run in.")
                    .setPositiveButton("OK", (dialog, i) -> {
                        folderPicker();
                    })
                    .setNegativeButton("Cancel", (dialog, i) -> {
                        dialog.cancel();
                        quit(3);
                    })
                    .setCancelable(false)
                    .show();
        }
        else {
            AlertDialog baseAlert = null;

            DialogTimer timer = new DialogTimer(5000, 100);

            baseAlert = new AlertDialog.Builder(this)
                    .setTitle("Game starting")
                    .setMessage("Game will start in...")
                    .setPositiveButton("Start", (dialog, i) -> {
                                //String p = Environment.getExternalStorageDirectory().getAbsolutePath() + "/" + basePath;
                                try {
                                    if (DocumentFile.fromTreeUri(this, basePath).findFile(".nomedia") == null)
                                        createFile(".nomedia");
                                } catch (Exception e) {}

                                if (!checkPermission()) {
                                    quit(1);
                                }

                                Intent intent = new Intent(this, RSDK.class);
                                intent.setData(basePath);
                                intent.setFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION |
                                        Intent.FLAG_GRANT_READ_URI_PERMISSION |
                                        Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
                                grantUriPermission(getApplicationContext().getPackageName() + ".RSDK", basePath,
                                        Intent.FLAG_GRANT_WRITE_URI_PERMISSION |
                                                Intent.FLAG_GRANT_READ_URI_PERMISSION |
                                                Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);

                                instance = this;

                                gameLauncher.launch(intent);
                            }
                    )
                    .setNeutralButton("Change Path", (dialog, i) -> {
                        timer.cancel();
                        folderPicker();
                    })
                    .create();

            timer.alert = baseAlert;
            baseAlert.setOnShowListener(dialog -> timer.start());

            baseAlert.show();
        }
    }

    // https://stackoverflow.com/questions/62782648/
    private boolean checkPermission() {
        if (SDK_INT >= Build.VERSION_CODES.R) {
            return true;
        } else {
            return ContextCompat.checkSelfPermission(this, android.Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED;
        }
    }

    private void folderPicker() {
        refreshStore();
        folderLauncher.launch(
                new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE)
                        .putExtra(DocumentsContract.EXTRA_INITIAL_URI, basePath)
                        .addFlags(Intent.FLAG_GRANT_WRITE_URI_PERMISSION |
                                Intent.FLAG_GRANT_READ_URI_PERMISSION |
                                Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION));
    }

    public Uri createFile(String filename) throws FileNotFoundException {
        return DocumentFile.fromTreeUri(this, basePath).createFile("application/octet-stream", filename).getUri();
    }

    private void requestPermission(Activity activity, Bundle bundle) {

        ActivityResultLauncher<Intent> launcher = registerForActivityResult(
                new ActivityResultContracts.StartActivityForResult(),
                    result -> startGame(false)
                );

        new AlertDialog.Builder(this)
                .setTitle("Permissions needed")
                .setMessage(
                        String.format("RSDK needs to be able to access external files.\nWould you like to grant read/write permissions?")
                )
                .setPositiveButton("Yes", (dialogInterface, i) -> {
                    ActivityCompat.requestPermissions(activity, new String[]{Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE}, REQUEST_CODE);
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
                        startGame(false);
                    } else {
                        quit(1);
                    }
                }
                break;
        }
    }

}
