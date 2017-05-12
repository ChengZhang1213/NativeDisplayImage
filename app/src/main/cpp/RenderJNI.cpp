//
// Created by Android on 2017/5/3.
//
#include <assert.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include "string"
#include "GLES2/gl2.h"
#include "android_tools.h"
#include "texture.h"
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/features2d.hpp"
#include "buffer.h"

using namespace cv;

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
                "uniform sampler2D texture;\n"
                "varying vec2 v_TextureCoordinates;\n"
                "void main() {\n"
                "  //gl_FragColor = vec4(0.0, 1.0, 1.0, 1.0);\n"
                "gl_FragColor = texture2D(texture, v_TextureCoordinates);\n"
                "}\n";

GLuint textureID;
GLuint buffer;
GLuint gProgram;
GLuint gvPositionHandle;
GLuint textureCoordinatesHandle;
GLuint textureLocation;

void onPreDraw(const char *string);


void drawFeature(cv::Mat grayMat);

static const float rect[] = {-1.0f, -1.0f, 0.0f, 1.0f,
                             1.0f, -1.0f, 1.0f, 1.0f,
                             -1.0f, 1.0f, 0.0f, 0.0f,
                             1.0f, 1.0f, 1.0f, 0.0f
};

int width;
int height;

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
    textureLocation = (GLuint) glGetUniformLocation(gProgram, "texture");
    checkGlError("glGetAttribLocation");
    LOGI("glGetAttribLocation(\"vPosition\") = %d\n",
         gvPositionHandle);

    glViewport(0, 0, w, h);
    checkGlError("glViewport");
    width = w;
    height = h;
    return true;
}

void neon_convert(uint8_t *__restrict dest, uint8_t *__restrict src, int width, int height) {
    int k = (height - 1) * width * 4;
    int l = 0;
    for (int i = height - 1; i >= 0; --i) {
        for (int j = 0; j < width; ++j) {
            //uint8_t color = 0;
            uint8_t color = (77 * src[k] + 151 * src[k + 1] + 28 * src[k + 2]) / 256;
            dest[l] = color;
            dest[l + 1] = color;
            dest[l + 2] = color;
            dest[l + 3] = color;

            k += 4;
            l += 4;
        }
        k -= 8 * width;
    }
}

void onPreDraw(const char *imgPath) {
//    IplImage *src = cvLoadImage(imgPath);
//    IplImage *dst = cvCreateImage(cvGetSize(src),src->depth,src->nChannels);
//    //you should confirm the image type
//    cvCvtColor(src,dst,CV_BGR2GRAY);
    const cv::Mat &src = cv::imread(imgPath);

    width = src.rows;
    height = src.cols;
//    cv::Mat rgbMat;
//    cv::cvtColor(src,rgbMat,cv::COLOR_BGR2RGB);
// uint8_t *grayData;
//    grayData = (uint8_t *) calloc((size_t) (width * height * 4), sizeof(uint8_t));
//    neon_convert(grayData, rgbMat.data, width, height);

    cv::Mat grayMat;
    cv::cvtColor(src, grayMat, cv::COLOR_BGR2GRAY);


    drawFeature(grayMat);


    textureID = load_texture(src.cols, src.rows, GL_LUMINANCE, grayMat.data);
    buffer = create_vbo(sizeof(rect), rect, GL_STATIC_DRAW);
}

void drawFeature(cv::Mat grayMat) {
    std::vector<KeyPoint> keypoint;
    Mat descriptorMat;

    Ptr<FeatureDetector> fd = ORB::create();
    Ptr<DescriptorExtractor> de = fd;
    fd->detect(grayMat, keypoint); //检测特征点
    de->compute(grayMat, keypoint, descriptorMat);//生成特征点描述子


    for (unsigned int i = 0; i < keypoint.size(); ++i) {
        cv::circle(grayMat,
                   cv::Point((int) keypoint.at(i).pt.x, (int) keypoint.at(i).pt.y),
                   5,
                   cv::Scalar(0, 0, 255),
                   1,
                   cv::LINE_AA);
    }

}

void renderFrame() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(gProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glUniform1i(textureLocation, 0);

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
