cmake_minimum_required(VERSION 3.4.1)

# now build app's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")


#openCV config
set(OPEN_CV_ROOT D:/Android/CodeProject/OpenCV-android-sdk)
include_directories(${OPEN_CV_ROOT}/sdk/native/jni/include)

add_library( lib_opencv STATIC IMPORTED )
set_target_properties(lib_opencv
                    PROPERTIES IMPORTED_LOCATION
                    D:/Android/CodeProject/GPU_Android/NativeDisplayImage/app/src/main/jniLibs/${ANDROID_ABI}/libopencv_java3.so)




add_library(renderJni SHARED
            RenderJNI.cpp
            buffer.cpp
            texture.cpp

            SIFT/SIFT.cpp
            )



# add lib dependencies
target_link_libraries(renderJni
                      android
                      log 
                      EGL
                      GLESv2
                      lib_opencv
                      )

