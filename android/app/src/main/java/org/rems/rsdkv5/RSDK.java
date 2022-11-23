package org.rems.rsdkv5;

import static android.os.Build.VERSION.SDK_INT;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.EmptyStackException;
import java.util.HashMap;
import java.util.List;
import java.util.Stack;

import com.google.androidgamesdk.GameActivity;

import android.content.ContentResolver;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.provider.DocumentsContract;
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

        String path;
        Stack<Cursor> docs = new Stack<>();
        Uri base;
        ContentResolver resolver;

        public RecursiveIterator(@NonNull ContentResolver resolver, String path, Uri uri) {
            this.path = path;
            base = uri;
            this.resolver = resolver;
            this.docs.add(resolver.query(
                    DocumentsContract.buildChildDocumentsUriUsingTree(base,
                            DocumentsContract.getDocumentId(base)
                    ), new String[]{
                            Document.COLUMN_DOCUMENT_ID,
                            Document.COLUMN_MIME_TYPE
                    }, null, null, null));
        }

        public static RecursiveIterator get(ContentResolver resolver, byte[] path, Uri uri) {
            if (iterators.get(path) != null)
                return iterators.get(path);
            RecursiveIterator iter = new RecursiveIterator(resolver, new String(path), uri);
            iterators.put(path, iter);
            return iter;
        }

        public String next() {
            Cursor c = null;
            try {
                c = docs.peek();
                c.moveToNext();
                Uri uri = DocumentsContract.buildDocumentUriUsingTree(base, c.getString(0));
                if (c.getString(1).equals(Document.MIME_TYPE_DIR)) {
                    docs.push(resolver.query(
                            DocumentsContract.buildChildDocumentsUriUsingTree(
                                    uri, DocumentsContract.getDocumentId(uri)
                            ), new String[]{
                                    Document.COLUMN_DOCUMENT_ID,
                                    Document.COLUMN_MIME_TYPE
                            }, null, null, null));
                    return next();
                }
                String seg = uri.getLastPathSegment();
                return seg.substring(seg.indexOf(path));
            } catch (EmptyStackException e) {
                iterators.remove(path);
                return null;
            } catch (Exception e) {
                c.close();
                docs.pop();
                return next();
            }
        }
    }

    public String fsRecurseIter(byte[] path) {
        Uri uri = basePath.buildUpon().encodedPath(pathString + Uri.encode(new String(path))).build();
        RecursiveIterator iter = RecursiveIterator.get(getContentResolver(), path, uri);
        return iter.next();
    }

}
