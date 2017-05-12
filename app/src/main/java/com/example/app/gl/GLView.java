package com.example.app.gl;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;

import com.example.app.RenderJNI;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Created by Android on 2017/5/2.
 */

public class GLView extends GLSurfaceView {
    private Context context;

    public GLView(Context context, String path) {
        super(context);
        init(context, path);
    }

    public GLView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context, null);

    }

    private void init(Context context, String path) {
        this.context = context;
        setEGLContextFactory(new ContextFactory());

        setRenderer(new Renderer(context, path));
    }

    private static class Renderer implements GLSurfaceView.Renderer {
        private Context context;
        private String path;

        public Renderer(Context context, String path) {
            this.context = context;
            this.path = path;
        }

        public void onDrawFrame(GL10 gl) {
            RenderJNI.on_draw_frame();
        }

        public void onSurfaceChanged(GL10 gl, int width, int height) {
            Bitmap bitmap = BitmapFactory.decodeFile(path);
            RenderJNI.on_surface_changed(width
                    , height);
        }

        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            RenderJNI.on_surface_created(path);
        }
    }

}
