package org.rems.rsdkv5;

import static android.os.Build.VERSION.SDK_INT;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

import com.google.androidgamesdk.GameActivity;

import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.provider.DocumentsContract.Document;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;

import androidx.annotation.NonNull;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;
import androidx.documentfile.provider.DocumentFile;

public class RSDK extends GameActivity {
    static {
        // https://developer.android.com/ndk/guides/cpp-support#shared_runtimes
        System.loadLibrary("c++_shared");
        System.loadLibrary("RetroEngine");
    }

    private static Uri basePath = null;
    private static OutputStream log = null;
    private static String pathString = null;

    private void hideSystemUI() {

        if (SDK_INT >= Build.VERSION_CODES.P) {
            getWindow()
                    .getAttributes().layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
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
            } else
                basePath = Launcher.refreshStore();

            if (basePath == null) {
                Log.e("RSDKv5-J", "Base path file not found; game cannot be started standalone.");
                setResult(RESULT_CANCELED);
                finishActivity(RESULT_CANCELED);
            }
        }

        pathString = DocumentFile.fromTreeUri(getApplicationContext(), basePath).getUri().getEncodedPath()
                + Uri.encode("/");


        WindowCompat.setDecorFitsSystemWindows(getWindow(), false);
        hideSystemUI();
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS,
                WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        try {
            ParcelFileDescriptor.adoptFd(getFD("log.txt".getBytes(), (byte)'a')).close();
        } catch (IOException e) {
            e.printStackTrace();
        }

        try {
            DocumentFile docfile = DocumentFile.fromTreeUri(this, basePath).findFile("log.txt");
            Uri uri = docfile.getUri();

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
        finishActivity(RESULT_OK);
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
        // this func survives android v3 because it's just easier to handle
        final int pointerCount = event.getPointerCount();
        int action = event.getActionMasked();
        int i = -1;

        switch (action) {
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

    public int getFD(byte[] file, byte mode) {
        String useMode, filepath = new String(file);
        switch (mode) {
            case 'a':
                useMode = "wa";
                break;
            case 'r':
                useMode = "r";
                break;
            default:
                useMode = "w";
                break;
        }
        try {
            Uri uri = basePath.buildUpon().encodedPath(pathString + Uri.encode(filepath)).build();
            ParcelFileDescriptor parcel = getContentResolver().openFileDescriptor(uri, useMode);
            int fd = parcel.dup().detachFd();
            parcel.close();
            return fd;
        } catch (Exception e) {
            try {
                if (!useMode.equals("r")) {
                    if (Launcher.instance == null) {
                        Log.w("RSDKv5-J",
                                String.format("Cannot create file %s; game opened without launcher", filepath));
                        return 0;
                    }
                    Uri uri = Launcher.instance.createFile(filepath);
                    ParcelFileDescriptor parcel = getContentResolver().openFileDescriptor(uri, useMode);
                    int fd = parcel.dup().detachFd();
                    parcel.close();
                    return fd;
                }
            } catch (Exception ex) {
                ex.printStackTrace();
            }
        }
        return 0;
    }

    public void writeLog(byte[] array, int as) {
        Log.println(as, "RSDKv5", new String(array));
        try {
            log.write(array);
            // log.write('\n');
            log.flush();
        } catch (Exception e) {
        }
    }

    public boolean fsExists(byte[] path) {
        Uri uri = basePath.buildUpon().encodedPath(pathString + Uri.encode(new String(path))).build();
        return DocumentFile.fromSingleUri(getApplicationContext(), uri).exists();
    }

    public boolean fsIsDir(byte[] path) {
        Uri uri = basePath.buildUpon().encodedPath(pathString + Uri.encode(new String(path))).build();
        return DocumentFile.fromSingleUri(getApplicationContext(), uri).isDirectory();
    }

    public String[] fsDirIter(byte[] path) {
        Uri uri = basePath.buildUpon().encodedPath(pathString + Uri.encode(new String(path))).build();
        DocumentFile dir = DocumentFile.fromTreeUri(getApplicationContext(), uri);
        if (dir.isFile())
            return new String[0];
        List<String> out = new ArrayList<String>();
        for (DocumentFile file : dir.listFiles()) {
            if (file.isDirectory())
                out.add(path + "/" + file.getName());
        }
        return out.toArray(new String[0]);
    }


    static class RecursiveIterator {
        static HashMap<byte[], RecursiveIterator> iterators = new HashMap<>();

        static class DocPos {
            public DocumentFile[] docs;
            int pos = -1;

            public DocPos(@NonNull DocumentFile doc) {
                this.docs = doc.listFiles();
            }

            public DocumentFile next() {
                try {
                    return docs[++pos];
                }
                catch (Exception e) {
                    return null;
                }
            }
        }

        String path;
        List<DocPos> docs = new ArrayList<>();

        public RecursiveIterator(String path, DocumentFile doc) {
            this.path = path;
            this.docs.add(new DocPos(doc));
        }

        public static RecursiveIterator get(byte[] path, DocumentFile doc) {
            if (iterators.get(path) != null)
                return iterators.get(path);
            RecursiveIterator iter = new RecursiveIterator(new String(path), doc);
            iterators.put(path, iter);
            return iter;
        }

        public String next() {
            while (docs.size() != 0) {
                int last = docs.size() - 1;
                DocumentFile doc = docs.get(last).next();
                try {
                    if (doc.isDirectory()) {
                        docs.add(new DocPos(doc));
                        continue;
                    }
                    String seg = doc.getUri().getLastPathSegment();
                    return seg.substring(seg.indexOf(path));
                }
                catch (NullPointerException e) {
                    docs.remove(last);
                    continue;
                }
            }
            iterators.remove(path);
            return null;
        }
    }

    public String fsRecurseIter(byte[] path) {
        Uri uri = basePath.buildUpon().encodedPath(pathString + Uri.encode(new String(path))).build();
        DocumentFile dir = DocumentFile.fromTreeUri(getApplicationContext(), uri);
        RecursiveIterator iter = RecursiveIterator.get(path, dir);
        return iter.next();
    }

}
