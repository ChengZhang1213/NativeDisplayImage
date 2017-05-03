package com.example.app;

/**
 * Created by Android on 2017/5/3.
 */

public class RenderJNI {
    static {
        System.loadLibrary("renderJni");
    }
    public static native String sayHi();
}
