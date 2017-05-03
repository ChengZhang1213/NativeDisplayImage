//
// Created by Android on 2017/5/3.
//
#include "jni.h"
#include "string"
extern "C"{
JNIEXPORT jstring JNICALL
Java_com_example_app_RenderJNI_sayHi(JNIEnv *env, jobject obj);
}
JNIEXPORT jstring JNICALL
Java_com_example_app_RenderJNI_sayHi(JNIEnv *env, jobject obj){
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
};