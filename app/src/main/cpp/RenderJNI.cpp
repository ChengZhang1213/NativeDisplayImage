//
// Created by Android on 2017/5/3.
//
#include <assert.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include "jni.h"
#include "string"
#include "GLES2/gl2.h"
#include "android_tools.h"
#include "texture.h"
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "buffer.h"

extern "C" {
JNIEXPORT jstring JNICALL
Java_com_example_app_RenderJNI_sayHi(JNIEnv *env, jobject obj);
JNIEXPORT void JNICALL
Java_com_example_app_RenderJNI_initAssetsManager(JNIEnv *env, jclass type, jobject assetManager);

JNIEXPORT void JNICALL
Java_com_example_app_RenderJNI_on_1surface_1changed(JNIEnv *env, jclass type, jint width,
                                                    jint height);
JNIEXPORT void JNICALL
Java_com_example_app_RenderJNI_on_1draw_1frame(JNIEnv *env, jclass type);

JNIEXPORT void JNICALL
Java_com_example_app_RenderJNI_on_1surface_1created(JNIEnv *env, jclass type, jstring imagePath_);
}

JNIEXPORT jstring JNICALL
Java_com_example_app_RenderJNI_sayHi(JNIEnv *env, jobject obj) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

JNIEXPORT void JNICALL
Java_com_example_app_RenderJNI_initAssetsManager(JNIEnv *env, jclass type,
                                                 jobject java_asset_manager) {

}

auto gVertexShader =
        "attribute vec4 vPosition;\n"
                "attribute vec2 a_TextureCoordinates;\n"
                "varying vec2 v_TextureCoordinates;\n"
                "void main() {\n"
                "v_TextureCoordinates = a_TextureCoordinates;\n"
                "gl_Position = vPosition;\n"
                "}\n";

auto gFragmentShader =
        "precision mediump float;\n"
                "uniform sampler2D u_TextureUnit;\n"
                "varying vec2 v_TextureCoordinates;\n"
                "void main() {\n"
                "  //gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
                "gl_FragColor = texture2D(u_TextureUnit, v_TextureCoordinates);\n"
                "}\n";

GLuint textureID;
GLuint buffer;
GLuint gProgram;
GLuint gvPositionHandle;
GLuint textureCoordinatesHandle;
GLuint u_texture_unit_location;

void onPreDraw(const char *string);

//const GLfloat gTriangleVertices[] = {
//        -0.5f, 0.5f, 0.0f,
//        0.5f, 0.5f, 0.0f,
//        -0.5f, -0.5f, 0.0f,
//        0.5f, -0.5f, 0.0f
////        -1.0f, -1.0f, 0.0f,//bottom left
////        1.0f, -1.0f, 0.0f,//bottom right
////        -1.0f, 1.0f, 0.0f,//top left
////        1.0f, 1.0f, 0.0f//top right
//};
static const float rect[] = {-1.0f, -1.0f, 0.0f, 0.0f,
                             -1.0f, 1.0f, 0.0f, 1.0f,
                             1.0f, -1.0f, 1.0f, 0.0f,
                             1.0f, 1.0f, 1.0f, 1.0f};

bool setupGraphics(int w, int h) {
    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    LOGI("setupGraphics(%d, %d)", w, h);
    gProgram = createProgram(gVertexShader, gFragmentShader);
    if (!gProgram) {
        LOGE("Could not create program.");
        return false;
    }
    gvPositionHandle = (GLuint) glGetAttribLocation(gProgram, "vPosition");
    textureCoordinatesHandle = (GLuint) glGetAttribLocation(gProgram, "a_TextureCoordinates");
    u_texture_unit_location = (GLuint) glGetUniformLocation(gProgram, "u_TextureUnit");
    checkGlError("glGetAttribLocation");
    LOGI("glGetAttribLocation(\"vPosition\") = %d\n",
         gvPositionHandle);

    glViewport(0, 0, w, h);
    checkGlError("glViewport");
    return true;
}


void onPreDraw(const char *imgPath) {
    const cv::Mat &mat = cv::imread(imgPath);
    textureID = load_texture(mat.cols, mat.rows, CV_8UC4, mat.data);
    buffer = create_vbo(sizeof(rect), rect, GL_STATIC_DRAW);

}

void renderFrame() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(gProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glUniform1i(u_texture_unit_location, 0);

    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GL_FLOAT),
                          BUFFER_OFFSET(0));
    glVertexAttribPointer(textureCoordinatesHandle, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GL_FLOAT),
                          BUFFER_OFFSET(2 * sizeof(GL_FLOAT)));
    glEnableVertexAttribArray(gvPositionHandle);
    glEnableVertexAttribArray(textureCoordinatesHandle);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


JNIEXPORT void JNICALL
Java_com_example_app_RenderJNI_on_1surface_1created(JNIEnv *env, jclass type, jstring imagePath_) {
    const char *imagePath = env->GetStringUTFChars(imagePath_, 0);
    onPreDraw(imagePath);

    env->ReleaseStringUTFChars(imagePath_, imagePath);
}


JNIEXPORT void JNICALL
Java_com_example_app_RenderJNI_on_1surface_1changed(JNIEnv *env, jclass type, jint width,
                                                    jint height) {
    setupGraphics(width, height);

}

JNIEXPORT void JNICALL
Java_com_example_app_RenderJNI_on_1draw_1frame(JNIEnv *env, jclass type) {
    renderFrame();
}
