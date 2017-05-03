package com.example.app;

import android.content.res.AssetManager;

/**
 * Created by Android on 2017/5/3.
 */

public class RenderJNI {
    static {
        System.loadLibrary("renderJni");
    }
    public static native String sayHi();

    public static native void initAssetsManager(AssetManager assetManager);


    public static native void on_surface_created(String imagePath);

    public static native void on_surface_changed(int width, int height);

    public static native void on_draw_frame();
}
