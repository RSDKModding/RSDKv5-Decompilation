package org.rems.rsdkv5;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorFilter;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.AppCompatImageView;

import com.bumptech.glide.Glide;
import com.bumptech.glide.load.Option;
import com.bumptech.glide.request.target.CustomTarget;
import com.bumptech.glide.request.transition.Transition;

import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.CharBuffer;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;

public class LoadingIcon extends AppCompatImageView {
    private RSDK game;
    private Context context;

    private List<Bitmap> sheets = new ArrayList<>();

    BasicAnimator waitSpinner = null;

    private static final int RSDK_SIGNATURE_SPR = 0x525053;
    private int opacity = 0;

    boolean fetching = false;

    public final ScheduledExecutorService scheduler;
    private final AnimateTimer animator = new AnimateTimer(this);

    @Override
    @SuppressLint("DrawAllocation")
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (opacity != 0 && !fetching && game.pixHeight != 0) {
            SpriteFrame frame = waitSpinner.animation.frames[waitSpinner.frameID];
            Bitmap bitmap = sheets.get(frame.sheetID);

            Paint paint = new Paint();
            paint.setAlpha(Math.min(opacity, 255));

            int scale = Math.max(getHeight() / game.pixHeight, 0) + 1;

            Matrix m = new Matrix();
            m.setScale(scale, scale);

            Bitmap crop = Bitmap.createBitmap(bitmap, frame.sprX, frame.sprY, frame.width, frame.height, m, false);

            canvas.drawBitmap(crop, (24 - frame.pivotX) * scale, getHeight() - (24 - frame.pivotY) * scale, paint);
            //canvas.drawBitmap(bitmap, frame.getRect(), new Rect(0, 0, 1000, 1000), paint);
        }
    }

    public static class ImageInfo {
        public ImageInfo(int width, int height, byte[] pixels, int[] palette) {
            w = width;
            h = height;
            this.pixels = pixels;
            this.palette = palette;
        }

        int w;
        int h;
        byte[] pixels;
        int[] palette;
    }

    // trimmed & modded RSDK classes
    // we're gonna read it in-java, so restrictions don't matter
    static class SpriteFrame {
        public char sprX;
        public char sprY;
        public char width;
        public char height;
        public short pivotX;
        public short pivotY;
        public char duration;
        public char unicodeChar;
        public char sheetID;

        public Rect getRect() {
            return new Rect(sprX, sprY, width, height);
        }
    };

    static class SpriteAnimationEntry {
        public int frameListOffset;
        public char frameCount;
        public short animationSpeed;
        public char loopIndex;
        public char rotationStyle;
    };

    static class SpriteAnimation {
        public SpriteFrame[] frames;
        public SpriteAnimationEntry[] animations;
        public char animCount;
    };

    static class BasicAnimator {
        public SpriteAnimation animation;
        int animID = 0;
        int frameID = 0;
        int timer = 0;

        void processAnimation() {
            SpriteAnimationEntry anim = animation.animations[animID];
            SpriteFrame frame = animation.frames[frameID];
            timer += anim.animationSpeed;

            while (timer > frame.duration) {
                ++frameID;
                
                timer -= frame.duration;
                if (frameID >= anim.frameCount) {
                    frameID = anim.loopIndex;
                }
            }
        }
    }

    public LoadingIcon(Context context, RSDK game) {
        super(context);
        this.game = game;
        scheduler = Executors.newScheduledThreadPool(0);
    }

    private static String readString(ByteBuffer reader) throws IOException {
        byte[] array = new byte[Byte.toUnsignedInt(reader.get()) - 1];
        reader.get(array);
        reader.get(); // null byte
        return new String(array);
    }

    public SpriteAnimation loadSpriteAnimation(byte[] animationFile) throws IOException {
        ByteBuffer reader = ByteBuffer.wrap(animationFile);
        reader.order(ByteOrder.LITTLE_ENDIAN);
        SpriteAnimation spr = new SpriteAnimation();
        if (reader.getInt() != RSDK_SIGNATURE_SPR) {
            return null;
        }
        spr.frames = new SpriteFrame[reader.getInt()];

        int sheetCount = Byte.toUnsignedInt(reader.get());
        int start = sheets.size();

        for (int i = 0; i < sheetCount; ++i) {

            byte[] data = RSDK.nativeLoadFile("Data/Sprites/" + readString(reader));
            if (data == null) {
                return null;
            }

            Glide.with(game)
                    .asBitmap()
                    .load(data)
                    .into(new CustomTarget<Bitmap>() {
                        @Override
                        public void onResourceReady(@NonNull Bitmap resource, @Nullable Transition<? super Bitmap> transition) {
                            resource.setHasAlpha(true);
                            for (int y = 0; y < resource.getHeight(); ++y) {
                                for (int x = 0; x < resource.getWidth(); ++x) {
                                    if ((resource.getPixel(x, y) & 0x00FFFFFF) == 0x00FF00FF) {
                                        resource.setPixel(x, y, Color.TRANSPARENT);
                                    }
                                }
                            }//*/
                            sheets.add(resource);

                        }

                        @Override
                        public void onLoadCleared(@Nullable Drawable placeholder) {
                        }
                    });
        }

        int hitboxCount = Byte.toUnsignedInt(reader.get()); // hitboxCount
        for (int i = 0; i < hitboxCount; ++i) readString(reader);

        spr.animCount = reader.getChar();
        spr.animations = new SpriteAnimationEntry[spr.animCount];

        int frameID = 0;
        for (int i = 0; i < spr.animCount; ++i) {
            readString(reader);

            spr.animations[i] = new SpriteAnimationEntry();
            SpriteAnimationEntry animation = spr.animations[i];

            animation.frameCount = reader.getChar();
            animation.frameListOffset = frameID;
            animation.animationSpeed = reader.getShort();
            animation.loopIndex = (char)Byte.toUnsignedInt(reader.get());
            reader.get();

            for (int f = 0; f < animation.frameCount; ++f) {
                spr.frames[frameID] = new SpriteFrame();
                SpriteFrame frame = spr.frames[frameID++];

                frame.sheetID     = (char)(start + Byte.toUnsignedInt(reader.get()));
                frame.duration    = reader.getChar();
                frame.unicodeChar = reader.getChar();
                frame.sprX        = reader.getChar();
                frame.sprY        = reader.getChar();
                frame.width       = reader.getChar();
                frame.height      = reader.getChar();
                frame.pivotX      = reader.getShort();
                frame.pivotY      = reader.getShort();
            }

            // hitboxes, we don't need em
        }
        return spr;
    }


    public void loadAnimationFiles(byte[] spinnerFile) throws IOException {
        fetching = true;
        sheets.clear();

        if (spinnerFile != null) {
            waitSpinner = new BasicAnimator();
            waitSpinner.animation = loadSpriteAnimation(spinnerFile);
        }
        fetching = false;
    }

    class AnimateTimer implements Runnable {
        public boolean showing = false;
        public boolean running = false;
        LoadingIcon icon;
        ScheduledFuture<?> t;

        public AnimateTimer(LoadingIcon icon) {
            this.icon = icon;
        }

        public void run() {
            if (!fetching)
                waitSpinner.processAnimation();

            if (showing) {
                if (opacity < 255)
                    opacity += 16;
            }
            else {
                opacity -= 16;
            }
            // for some reason if i get rid of this it's gone??
            setImageAlpha(Math.min(opacity, 255));

            invalidate(); // force a redraw

            if (opacity <= 0 || fetching) {
                running = false;
                t.cancel(false);
            }
        }   
    }

    public void show() {
        Log.d("RSDKv5-J", "START LOAD");
        if (waitSpinner != null && !fetching) {
            animator.showing = true;
            if (!animator.running) {
                animator.running = true;
                animator.t = scheduler.scheduleAtFixedRate(animator, 0, 1000 / 60, TimeUnit.MILLISECONDS);
            }
        }
    }

    public void hide() {
        if (animator.showing) {
            Log.d("RSDKv5-J", "END LOAD");
            animator.showing = false;
        }
    }
}
