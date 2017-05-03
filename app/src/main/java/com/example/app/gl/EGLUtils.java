package com.example.app.gl;

import android.util.Log;

import javax.microedition.khronos.egl.EGL10;

/**
 * Created by Android on 2017/5/2.
 */

public class EGLUtils {
    public static final String TAG= "EGLUtils";
    public static final boolean DEBUG = false;

    public static void checkEglError(String prompt, EGL10 egl) {
        int error;
        while ((error = egl.eglGetError()) != EGL10.EGL_SUCCESS) {
            Log.e(TAG, String.format("%s: EGL error: 0x%x", prompt, error));
        }
    }

}
