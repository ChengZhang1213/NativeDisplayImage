package com.example.app;

import android.os.Bundle;
import android.os.Environment;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;

import com.example.app.gl.GLView;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";
    GLView mView;
    String path = Environment.getExternalStorageDirectory().getAbsolutePath() + "/download/images/0.png";


    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mView = new GLView(getApplication(),path);

        setContentView(mView);
//        setContentView(R.layout.activity_main);
//        ImageView iv = (ImageView) findViewById(R.id.iv);
//        Bitmap bitmap = BitmapFactory.decodeFile(path);
//        iv.setImageBitmap(bitmap);
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (mView != null) {
            mView.onPause();

        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (mView != null) {

            mView.onResume();
        }
    }
}
