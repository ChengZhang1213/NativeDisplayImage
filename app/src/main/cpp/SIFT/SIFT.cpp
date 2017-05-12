//
// Created by Android on 2017/5/4.
//


#include <malloc.h>
#include "math.h"
#include "SIFT.h"
#include "../common/android_tools.h"
#include "shaders.h"
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

int width;
int height;

// OpenGL framebuffer pointers:
GLuint detBuf[4], dogBuf[4][2], renderBuf, gxBuf[4], gyBuf[4], dispBuf, regBuf, winBuf, rotBuf, desBuf,
        desBuf2, edgeBuf, smoothBuf[4][4];

// OpenGL texture pointers:
GLuint pic, detTex[4], dogTex[4][2], gxTex[4], gyTex[4], gauss, gauss2, regTex, winTex, rotTex, desTex, desTex2,
        edgeTex, blankTex, smoothTex[4][4], smooth0, smooth1;

// OpenGL program pointers:
GLuint printer, grad, smoothDouble, smooth, dog, nms, edgeSuppression, orientation, mainOrientation, descriptor;
// Program parameters location pointers:
GLuint printerWritingPosition, printerReadingPosition, printerPic0;
GLuint gradWritingPosition, gradReadingPosition, gradPic0, gradPic1, gradSigma, gradDirection;
GLuint smoothDoubleWritingPosition, smoothDoubleReadingPosition, smoothDoubleGaussianCoeff, smoothDoubleDirection, smoothDoublePic0, smoothDoublePic1;
GLuint smoothWritingPosition, smoothReadingPosition, smoothGaussianCoeff, smoothDirection, smoothPic0;
GLuint dogWritingPosition, dogReadingPosition, dogPic0;
GLuint nmsWritingPosition, nmsReadingPosition, nmsWidth, nmsHeight, nmsPic0, nmsPic1;
GLuint edgeSuppressionWritingPosition, edgeSuppressionReadingPosition, edgeSuppressionPic0, edgeSuppressionPic1, edgeSuppressionWidth, edgeSuppressionHeight, edgeSuppressionScale, edgeSuppressionTheta;
GLuint orientationWritingPosition, orientationReadingPosition0, orientationReadingPosition1, orientationPicGradx, orientationPicGrady, orientationPicGauss, orientationScale, orientationTheta;
GLuint mainOrientationWritingPosition, mainOrientationReadingPosition, mainOrientationSize, mainOrientationPic0;
GLuint descriptorWritingPosition, descriptorReadingPosition, descriptorSize, descriptorPic0;

//A few other useful variables to initialize now:
uint8_t **nmsOut;

//constants:
int NB_OCT;
float coeffDown0[60];
float coeffDown1[60];
float coeffUp0[60];
float coeffUp1[60];
float coeffDoG[8];
float sigma[4];
GLshort writingPosition[8];
GLshort readingPosition[8];
GLfloat gaussCoord[8];


void initWithHeight(int picWidth, int picHeight, int oct) {
    width = picWidth;
    height = picHeight;
    NB_OCT = oct;

    // Full screen writing coordinates
    writingPosition[0] = (GLshort) -1.0f;
    writingPosition[1] = (GLshort) -1.0f;
    writingPosition[2] = (GLshort) 1.0f;
    writingPosition[3] = (GLshort) -1.0f;
    writingPosition[4] = (GLshort) -1.0f;
    writingPosition[5] = (GLshort) 1.0f;
    writingPosition[6] = (GLshort) 1.0f;
    writingPosition[7] = (GLshort) 1.0f;

    // Fulle screen reading coordinates
    readingPosition[0] = (GLshort) 0.0f;
    readingPosition[1] = (GLshort) 0.0f;
    readingPosition[2] = (GLshort) 1.0f;
    readingPosition[3] = (GLshort) 0.0f;
    readingPosition[4] = (GLshort) 0.0f;
    readingPosition[5] = (GLshort) 1.0f;
    readingPosition[6] = (GLshort) 1.0f;
    readingPosition[7] = (GLshort) 1.0f;


    // kernel values
    const float sigmas[7] = {1.34543, 1.6, 1.90273, 2.26274, 2.69087, 3.2, 3.80546};
    const float sigmaDown[4] = {sigmas[0], sigmas[1], sigmas[2], sigmas[3]};
    const float sigmaUp[4] = {sigmas[3], sigmas[4], sigmas[5], sigmas[6]};
    sigma[0] = sigmas[1];
    sigma[1] = sigmas[2];
    sigma[2] = sigmas[3];
    sigma[3] = sigmas[4];

    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 4; j++) {
            coeffDown0[4 * i + j] = (float) (
                    exp(-(float) (i) * (float) (i) / (2.0f * sigmaDown[j] * sigmaDown[j])) /
                    sqrt(2.0f * 3.14159 * sigmaDown[j] * sigmaDown[j]));
            coeffDown1[4 * i + j] = (float) (i == 0 ? 1.0f : exp(-(float) (i) * (float) (i) /
                                                                 (2.0f * sigmaDown[j] *
                                                                  sigmaDown[j])) /
                                                             sqrt(2.0f * 3.14159 * sigmaDown[j] *
                                                                  sigmaDown[j]));
        }
    }
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 4; j++) {
            coeffUp0[4 * i + j] = (float) (
                    exp(-(float) (i) * (float) (i) / (2.0f * sigmaUp[j] * sigmaUp[j])) /
                    sqrt(2.0f * 3.14159 * sigmaUp[j] * sigmaUp[j]));
            coeffUp1[4 * i + j] = (float) (i == 0 ? 1.0f : exp(-(float) (i) * (float) (i) /
                                                               (2.0f * sigmaUp[j] * sigmaUp[j])) /
                                                           sqrt(2.0f * 3.14159 * sigmaUp[j] *
                                                                sigmaUp[j]));
        }
    }

    float sigmaDoG = 1.8;
    for (int i = 0; i < 8; i++) {
        coeffDoG[i] = (float) (exp(-(float) (i) * (float) (i) / (2.0f * sigmaDoG * sigmaDoG)) /
                               sqrt(2.0f * 3.14159 * sigmaDoG * sigmaDoG));
    }


    //gaussian for orientation computation
    glGenTextures(1, &gauss);
    glBindTexture(GL_TEXTURE_2D, gauss);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    uint8_t *gaussData = (uint8_t *) calloc(16 * 16, sizeof(uint8_t));
    for (int i = -8; i < 8; i++) {
        for (int j = -8; j < 8; j++) {
            gaussData[16 * (i + 8) + j + 8] = (uint8_t) (255.0f *
                                                         exp(-(float) ((i + 0.5) * (i + 0.5) +
                                                                       (j + 0.5) * (j + 0.5)) /
                                                             (2.0f * 2.25)));
        }
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 16, 16, 0, GL_ALPHA, GL_UNSIGNED_BYTE, gaussData);
    free(gaussData);

    //gaussian for descriptor weighting
    glGenTextures(1, &gauss2);
    glBindTexture(GL_TEXTURE_2D, gauss2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    uint8_t *gaussData2 = (uint8_t *) calloc(32 * 32, sizeof(uint8_t));
    for (int i = -16; i < 16; i++) {
        for (int j = -16; j < 16; j++) {
            gaussData2[32 * (i + 16) + j + 16] = (uint8_t) (255.0f *
                                                            exp(-(float) ((i + 0.5) * (i + 0.5) +
                                                                          (j + 0.5) * (j + 0.5)) /
                                                                (2.0f * 64.0f)));
        }
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 32, 32, 0, GL_ALPHA, GL_UNSIGNED_BYTE, gaussData2);
    free(gaussData2);

    // Special texture coordinates for this gaussian:

    gaussCoord[0] = 5.0f / 32.0f;
    gaussCoord[1] = 5.0f / 32.0f;
    gaussCoord[2] = 27.0f / 32.0f;
    gaussCoord[3] = 5.0f / 32.0f;
    gaussCoord[4] = 5.0f / 32.0f;
    gaussCoord[5] = 27.0f / 32.0f;
    gaussCoord[6] = 27.0f / 32.0f;
    gaussCoord[7] = 27.0f / 32.0f;



    // ------------------- SHADERS INITIALIZATION PART ----------------------
    /* Builds shaders and sends them data, or at least locates the
     shader variable position so data can be sent to it later */

    //printer init
    printer = createProgram(vertex, printerFragmentShader);
    glUseProgram(printer);
    printerWritingPosition = (GLuint) glGetAttribLocation(printer, "writingPosition");
    glVertexAttribPointer(printerWritingPosition, 2, GL_SHORT, GL_FALSE, 0, writingPosition);
    glEnableVertexAttribArray(printerWritingPosition);
    printerReadingPosition = (GLuint) glGetAttribLocation(printer, "readingPosition");
    glVertexAttribPointer(printerReadingPosition, 2, GL_SHORT, GL_FALSE, 0, readingPosition);
    glEnableVertexAttribArray(printerReadingPosition);
    printerPic0 = (GLuint) glGetUniformLocation(printer, "pic");


    //gradient init
    grad = createProgram(vertex, gradientFragmentShader);
    glUseProgram(grad);
    gradWritingPosition = (GLuint) glGetAttribLocation(grad, "writingPosition");
    glVertexAttribPointer(gradWritingPosition, 2, GL_SHORT, GL_FALSE, 0, writingPosition);
    glEnableVertexAttribArray(gradWritingPosition);
    gradReadingPosition = (GLuint) glGetAttribLocation(grad, "readingPosition");
    glVertexAttribPointer(gradReadingPosition, 2, GL_SHORT, GL_FALSE, 0, readingPosition);
    glEnableVertexAttribArray(gradReadingPosition);
    gradPic0 = (GLuint) glGetUniformLocation(grad, "pic0");
    gradPic1 = (GLuint) glGetUniformLocation(grad, "pic1");
    gradSigma = (GLuint) glGetUniformLocation(grad, "sigma");
    glUniform1fv(gradSigma, 4, sigma);
    gradDirection = (GLuint) glGetUniformLocation(grad, "direction");

    //smoothDouble init, computes high precision smoothing in 2 pass
    smoothDouble = createProgram(vertex, smoothDoubleFragmentShader);
    glUseProgram(smoothDouble);
    smoothDoubleWritingPosition = (GLuint) glGetAttribLocation(smoothDouble, "writingPosition");
    checkGlError("glGetAttribLocation writingPosition");
    glVertexAttribPointer(smoothDoubleWritingPosition, 2, GL_SHORT, GL_FALSE, 0, writingPosition);
    glEnableVertexAttribArray(smoothDoubleWritingPosition);
    smoothDoubleReadingPosition = (GLuint) glGetAttribLocation(smoothDouble, "readingPosition");
    glVertexAttribPointer(smoothDoubleReadingPosition, 2, GL_SHORT, GL_FALSE, 0, readingPosition);
    glEnableVertexAttribArray(smoothDoubleReadingPosition);
    smoothDoubleGaussianCoeff = (GLuint) glGetUniformLocation(smoothDouble, "gaussianCoeff");
    smoothDoubleDirection = (GLuint) glGetUniformLocation(smoothDouble, "direction");
    smoothDoublePic1 = (GLuint) (GLuint) glGetUniformLocation(smoothDouble, "pic1");
    smoothDoublePic0 = (GLuint) glGetUniformLocation(smoothDouble, "pic0");

    //smooth init, more approximate version used to smooth DoG results
    smooth = createProgram(vertex, smoothFragmentShader);
    glUseProgram(smooth);
    smoothWritingPosition = (GLuint) glGetAttribLocation(smooth, "writingPosition");
    glVertexAttribPointer(smoothWritingPosition, 2, GL_SHORT, GL_FALSE, 0, writingPosition);
    glEnableVertexAttribArray(smoothWritingPosition);
    smoothReadingPosition = (GLuint) glGetAttribLocation(smooth, "readingPosition");
    glVertexAttribPointer(smoothReadingPosition, 2, GL_SHORT, GL_FALSE, 0, readingPosition);
    glEnableVertexAttribArray(smoothReadingPosition);
    smoothGaussianCoeff = (GLuint) glGetUniformLocation(smooth, "gaussianCoeff");
    smoothDirection = (GLuint) glGetUniformLocation(smooth, "direction");
    smoothPic0 = (GLuint) glGetUniformLocation(smooth, "pic");

    //dog init
    dog = createProgram(vertex, dogFragmentShader);
    glUseProgram(dog);
    dogWritingPosition = (GLuint) glGetAttribLocation(dog, "writingPosition");
    glVertexAttribPointer(dogWritingPosition, 2, GL_SHORT, GL_FALSE, 0, writingPosition);
    glEnableVertexAttribArray(dogWritingPosition);
    dogReadingPosition = (GLuint) glGetAttribLocation(dog, "readingPosition");
    glVertexAttribPointer(dogReadingPosition, 2, GL_SHORT, GL_FALSE, 0, readingPosition);
    glEnableVertexAttribArray(dogReadingPosition);
    dogPic0 = (GLuint) glGetUniformLocation(dog, "pic");


    //NMS init
    nms = createProgram(vertex, nmsFragmentShader);
    glUseProgram(nms);
    nmsWritingPosition = (GLuint) glGetAttribLocation(nms, "writingPosition");
    glVertexAttribPointer(nmsWritingPosition, 2, GL_SHORT, GL_FALSE, 0, writingPosition);
    glEnableVertexAttribArray(nmsWritingPosition);
    nmsReadingPosition = (GLuint) glGetAttribLocation(nms, "readingPosition");
    glVertexAttribPointer(nmsReadingPosition, 2, GL_SHORT, GL_FALSE, 0, readingPosition);
    glEnableVertexAttribArray(nmsReadingPosition);
    nmsWidth = (GLuint) glGetUniformLocation(nms, "width");
    nmsHeight = (GLuint) glGetUniformLocation(nms, "height");
    nmsPic0 = (GLuint) glGetUniformLocation(nms, "pic0");
    nmsPic1 = (GLuint) glGetUniformLocation(nms, "pic1");


    //Edge Response Suppression init
    edgeSuppression = createProgram(vertex0, edgeSuppressionFragmentShader);
    glUseProgram(edgeSuppression);
    edgeSuppressionWritingPosition = (GLuint) glGetAttribLocation(edgeSuppression,
                                                                  "writingPosition");
    glEnableVertexAttribArray(edgeSuppressionWritingPosition);
    edgeSuppressionPic0 = (GLuint) glGetUniformLocation(edgeSuppression, "pic0");
    edgeSuppressionPic1 = (GLuint) glGetUniformLocation(edgeSuppression, "pic1");
    edgeSuppressionWidth = (GLuint) glGetUniformLocation(edgeSuppression, "width");
    edgeSuppressionHeight = (GLuint) glGetUniformLocation(edgeSuppression, "height");
    edgeSuppressionScale = (GLuint) glGetUniformLocation(edgeSuppression, "scale");
    edgeSuppressionReadingPosition = (GLuint) glGetUniformLocation(edgeSuppression,
                                                                   "readingPosition");


    //orientation init
    orientation = createProgram(vertex2, orientationFragmentShader);
    glUseProgram(orientation);
    orientationWritingPosition = (GLuint) glGetAttribLocation(orientation, "writingPosition");
    glEnableVertexAttribArray(orientationWritingPosition);
    orientationReadingPosition0 = (GLuint) glGetAttribLocation(orientation, "readingPositionGrad");
    glEnableVertexAttribArray(orientationReadingPosition0);
    orientationReadingPosition1 = (GLuint) glGetAttribLocation(orientation, "readingPositionGauss");
    glVertexAttribPointer(orientationReadingPosition1, 2, GL_SHORT, GL_FALSE, 0, readingPosition);
    glEnableVertexAttribArray(orientationReadingPosition1);
    orientationPicGradx = (GLuint) glGetUniformLocation(orientation, "gradx");
    orientationPicGrady = (GLuint) glGetUniformLocation(orientation, "grady");
    orientationPicGauss = (GLuint) glGetUniformLocation(orientation, "gauss");
    orientationScale = (GLuint) glGetUniformLocation(orientation, "scale");
    orientationTheta = (GLuint) glGetUniformLocation(orientation, "theta");


    //main orientation init
    mainOrientation = createProgram(vertex, mainOrientationFragmentShader);
    glUseProgram(mainOrientation);
    mainOrientationWritingPosition = (GLuint) glGetAttribLocation(mainOrientation,
                                                                  "writingPosition");
    glVertexAttribPointer(mainOrientationWritingPosition, 2, GL_SHORT, GL_FALSE, 0,
                          writingPosition);
    glEnableVertexAttribArray(mainOrientationWritingPosition);
    mainOrientationReadingPosition = (GLuint) glGetAttribLocation(mainOrientation,
                                                                  "readingPosition");
    glVertexAttribPointer(mainOrientationReadingPosition, 2, GL_SHORT, GL_FALSE, 0,
                          readingPosition);
    glEnableVertexAttribArray(mainOrientationReadingPosition);
    mainOrientationSize = (GLuint) glGetUniformLocation(mainOrientation, "sqSize");
    mainOrientationPic0 = (GLuint) glGetUniformLocation(mainOrientation, "pic0");

    //descriptor init
    descriptor = createProgram(vertex, desciptorFragmentShader);
    glUseProgram(descriptor);
    descriptorWritingPosition = (GLuint) glGetAttribLocation(descriptor, "writingPosition");
    glVertexAttribPointer(descriptorWritingPosition, 2, GL_SHORT, GL_FALSE, 0, writingPosition);
    glEnableVertexAttribArray(descriptorWritingPosition);
    descriptorReadingPosition = (GLuint) glGetAttribLocation(descriptor, "readingPosition");
    glVertexAttribPointer(descriptorReadingPosition, 2, GL_SHORT, GL_FALSE, 0, readingPosition);
    glEnableVertexAttribArray(descriptorReadingPosition);
    descriptorSize = (GLuint) glGetUniformLocation(descriptor, "sqSize");
    descriptorPic0 = (GLuint) glGetUniformLocation(descriptor, "pic0");

    // ---------------------- BUFFERS AND TEXTURES INITIALIZATION --------------------


    glGenTextures(1, &regTex);
    glBindTexture(GL_TEXTURE_2D, regTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenFramebuffers(1, &regBuf);
    glBindFramebuffer(GL_FRAMEBUFFER, regBuf);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, regTex, 0);

    glGenTextures(1, &edgeTex);
    glBindTexture(GL_TEXTURE_2D, edgeTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenFramebuffers(1, &edgeBuf);
    glBindFramebuffer(GL_FRAMEBUFFER, edgeBuf);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, edgeTex, 0);

    glGenTextures(1, &winTex);
    glBindTexture(GL_TEXTURE_2D, winTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenFramebuffers(1, &winBuf);
    glBindFramebuffer(GL_FRAMEBUFFER, winBuf);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, winTex, 0);

    glGenTextures(1, &rotTex);
    glBindTexture(GL_TEXTURE_2D, rotTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenFramebuffers(1, &rotBuf);
    glBindFramebuffer(GL_FRAMEBUFFER, rotBuf);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rotTex, 0);

    glGenTextures(1, &desTex);
    glBindTexture(GL_TEXTURE_2D, desTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenFramebuffers(1, &desBuf);
    glBindFramebuffer(GL_FRAMEBUFFER, desBuf);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, desTex, 0);

    glGenTextures(1, &pic);
    glBindTexture(GL_TEXTURE_2D, pic);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);


    uint8_t *blankData = (uint8_t *) calloc((size_t) (width * height * 4), sizeof(uint8_t));
    for (int i = 0; i < width * height * 4; i++) {
        blankData[i] = 128;
    }
    glGenTextures(1, &blankTex);
    glBindTexture(GL_TEXTURE_2D, blankTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, blankData);


    // "classical" buffer, not framebuffer
    //uint8_t *nmsOut[NB_OCT];
    nmsOut = (uint8_t **) calloc((size_t) NB_OCT, sizeof(uint8_t *));

    for (int i = 0; i < NB_OCT; i++) {

        glGenFramebuffers(1, &dogBuf[i][0]);
        glBindFramebuffer(GL_FRAMEBUFFER, dogBuf[i][0]);
        glGenTextures(1, &dogTex[i][0]);
        glBindTexture(GL_TEXTURE_2D, dogTex[i][0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width >> i, height >> i, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dogTex[i][0],
                               0);

        glGenFramebuffers(1, &dogBuf[i][1]);
        glBindFramebuffer(GL_FRAMEBUFFER, dogBuf[i][1]);
        glGenTextures(1, &dogTex[i][1]);
        glBindTexture(GL_TEXTURE_2D, dogTex[i][1]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width >> i, height >> i, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dogTex[i][1],
                               0);

        glGenFramebuffers(1, &detBuf[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, detBuf[i]);
        glGenTextures(1, &detTex[i]);
        glBindTexture(GL_TEXTURE_2D, detTex[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width >> i, height >> i, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, detTex[i], 0);

        glGenFramebuffers(1, &gxBuf[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, gxBuf[i]);
        glGenTextures(1, &gxTex[i]);
        glBindTexture(GL_TEXTURE_2D, gxTex[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width >> i, height >> i, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gxTex[i], 0);

        glGenFramebuffers(1, &gyBuf[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, gyBuf[i]);
        glGenTextures(1, &gyTex[i]);
        glBindTexture(GL_TEXTURE_2D, gyTex[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width >> i, height >> i, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gyTex[i], 0);

        glGenFramebuffers(1, &smoothBuf[i][0]);
        glBindFramebuffer(GL_FRAMEBUFFER, smoothBuf[i][0]);
        glGenTextures(1, &smoothTex[i][0]);
        glBindTexture(GL_TEXTURE_2D, smoothTex[i][0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width >> i, height >> i, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, smoothTex[i][0],
                               0);

        glGenFramebuffers(1, &smoothBuf[i][1]);
        glBindFramebuffer(GL_FRAMEBUFFER, smoothBuf[i][1]);
        glGenTextures(1, &smoothTex[i][1]);
        glBindTexture(GL_TEXTURE_2D, smoothTex[i][1]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width >> i, height >> i, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, smoothTex[i][1],
                               0);

        glGenFramebuffers(1, &smoothBuf[i][2]);
        glBindFramebuffer(GL_FRAMEBUFFER, smoothBuf[i][2]);
        glGenTextures(1, &smoothTex[i][2]);
        glBindTexture(GL_TEXTURE_2D, smoothTex[i][2]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width >> i, height >> i, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, smoothTex[i][2],
                               0);

        glGenFramebuffers(1, &smoothBuf[i][3]);
        glBindFramebuffer(GL_FRAMEBUFFER, smoothBuf[i][3]);
        glGenTextures(1, &smoothTex[i][3]);
        glBindTexture(GL_TEXTURE_2D, smoothTex[i][3]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width >> i, height >> i, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, smoothTex[i][3],
                               0);

        nmsOut[i] = (uint8_t *) calloc((size_t) ((width >> i) * (height >> i) * 4),
                                       sizeof(uint8_t));

    }

    glGenFramebuffers(1, &dispBuf);
    glBindFramebuffer(GL_FRAMEBUFFER, dispBuf);
    glGenRenderbuffers(1, &renderBuf);
    glBindRenderbuffer(GL_RENDERBUFFER, renderBuf);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderBuf);

//    [context renderbufferStorage:GL_RENDERBUFFER fromDrawable:(id<EAGLDrawable>)self.layer];
}

//descriptor normalization and writing to structure
void reorganize(uint8_t *reorgOut, uint8_t *desOut, int nb, int sqSize) {
    for (int i = 0; i < nb; i++) {
        int firstIndex = 16 * (i % sqSize + (i / sqSize) * 4 * sqSize);
        for (int j = 0; j < 4; j++) {
            reorgOut[128 * i + 32 * j + 0] = desOut[firstIndex + j * 16 * sqSize + 0] / 16;
            reorgOut[128 * i + 32 * j + 1] = desOut[firstIndex + j * 16 * sqSize + 0];
            reorgOut[128 * i + 32 * j + 2] = desOut[firstIndex + j * 16 * sqSize + 1] / 16;
            reorgOut[128 * i + 32 * j + 3] = desOut[firstIndex + j * 16 * sqSize + 1];
            reorgOut[128 * i + 32 * j + 4] = desOut[firstIndex + j * 16 * sqSize + 2] / 16;
            reorgOut[128 * i + 32 * j + 5] = desOut[firstIndex + j * 16 * sqSize + 2];
            reorgOut[128 * i + 32 * j + 6] = desOut[firstIndex + j * 16 * sqSize + 3] / 16;
            reorgOut[128 * i + 32 * j + 7] = desOut[firstIndex + j * 16 * sqSize + 3];
            reorgOut[128 * i + 32 * j + 8] = desOut[firstIndex + j * 16 * sqSize + 4] / 16;
            reorgOut[128 * i + 32 * j + 9] = desOut[firstIndex + j * 16 * sqSize + 4];
            reorgOut[128 * i + 32 * j + 10] = desOut[firstIndex + j * 16 * sqSize + 5] / 16;
            reorgOut[128 * i + 32 * j + 11] = desOut[firstIndex + j * 16 * sqSize + 5];
            reorgOut[128 * i + 32 * j + 12] = desOut[firstIndex + j * 16 * sqSize + 6] / 16;
            reorgOut[128 * i + 32 * j + 13] = desOut[firstIndex + j * 16 * sqSize + 6];
            reorgOut[128 * i + 32 * j + 14] = desOut[firstIndex + j * 16 * sqSize + 7] / 16;
            reorgOut[128 * i + 32 * j + 15] = desOut[firstIndex + j * 16 * sqSize + 7];
            reorgOut[128 * i + 32 * j + 16] = desOut[firstIndex + j * 16 * sqSize + 8] / 16;
            reorgOut[128 * i + 32 * j + 17] = desOut[firstIndex + j * 16 * sqSize + 8];
            reorgOut[128 * i + 32 * j + 18] = desOut[firstIndex + j * 16 * sqSize + 9] / 16;
            reorgOut[128 * i + 32 * j + 19] = desOut[firstIndex + j * 16 * sqSize + 9];
            reorgOut[128 * i + 32 * j + 20] = desOut[firstIndex + j * 16 * sqSize + 10] / 16;
            reorgOut[128 * i + 32 * j + 21] = desOut[firstIndex + j * 16 * sqSize + 10];
            reorgOut[128 * i + 32 * j + 22] = desOut[firstIndex + j * 16 * sqSize + 11] / 16;
            reorgOut[128 * i + 32 * j + 23] = desOut[firstIndex + j * 16 * sqSize + 11];
            reorgOut[128 * i + 32 * j + 24] = desOut[firstIndex + j * 16 * sqSize + 12] / 16;
            reorgOut[128 * i + 32 * j + 25] = desOut[firstIndex + j * 16 * sqSize + 12];
            reorgOut[128 * i + 32 * j + 26] = desOut[firstIndex + j * 16 * sqSize + 13] / 16;
            reorgOut[128 * i + 32 * j + 27] = desOut[firstIndex + j * 16 * sqSize + 13];
            reorgOut[128 * i + 32 * j + 28] = desOut[firstIndex + j * 16 * sqSize + 14] / 16;
            reorgOut[128 * i + 32 * j + 29] = desOut[firstIndex + j * 16 * sqSize + 14];
            reorgOut[128 * i + 32 * j + 30] = desOut[firstIndex + j * 16 * sqSize + 15] / 16;
            reorgOut[128 * i + 32 * j + 31] = desOut[firstIndex + j * 16 * sqSize + 15];
        }
    }
}

//normalization as described in SIFT paper: normalize, clamp to [0 , 0.2], normalize again, quantize on [0, 0.5]
void normalize(std::vector<MyKeyPoint> tab, uint8_t *r, int nb) {
    for (int i = 0; i < nb; i++) {
        //norm
        float s = 0.0;
        for (int j = 0; j < 128; j++) {
            s += (float) (r[128 * i + j] * r[128 * i + j]);
        }
        s = (float) sqrt(s);
        //clamp
        float *clamp = (float *) calloc(128, sizeof(float));
        for (int j = 0; j < 128; j++) {
            float v = (float) r[128 * i + j] / s;
            clamp[j] = v > 0.2 ? 0.2 : v;
        }
        //norm
        s = 0.0;
        for (int j = 0; j < 128; j++) {
            s += clamp[j] * clamp[j];
        }
        s = sqrt(s) / 255.0;
        //output
        uint8_t *values = (uint8_t *) calloc(128, sizeof(uint8_t));
        for (int j = 0; j < 128; j++) {
            values[j] = (int) (2.0 * clamp[j] / s);
        }
//        [[tab objectAtIndex:i] setDesc:values];
        tab.at((unsigned int) i).d = values;
    }
}

std::vector<MyKeyPoint> computeSiftImage(cv::Mat grayMat) {
//    const cv::Mat &src = cv::imread(imgPath);
//
//

    glBindTexture(GL_TEXTURE_2D, pic);
    GLuint type = GL_LUMINANCE;
    glTexImage2D(GL_TEXTURE_2D, 0, type, width, height, 0, type,
                 GL_UNSIGNED_BYTE, grayMat.data);
    glGenerateMipmap(GL_TEXTURE_2D);
    // --------------------- DETECTION ----------------
    //Loop on octaves


    int w = width;
    int h = height;
    for (int i = 0; i < NB_OCT; i++) {
        glViewport(0, 0, w, h);

        // First 4 "Half levels"
        //horizontal smoothing
        glUseProgram(smoothDouble);
        glVertexAttribPointer(smoothDoubleWritingPosition, 2, GL_SHORT, GL_FALSE, 0,
                              writingPosition);
        glVertexAttribPointer(smoothDoubleReadingPosition, 2, GL_SHORT, GL_FALSE, 0,
                              readingPosition);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pic);
        glUniform1i(smoothDoublePic1, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pic);
        glUniform1i(smoothDoublePic0, 1);
        glActiveTexture(GL_TEXTURE0);
        glUniform2f(smoothDoubleDirection, 1.0f / (float) w, 0.0);
        glUniform4fv(smoothDoubleGaussianCoeff, 15, coeffDown0);
        glBindFramebuffer(GL_FRAMEBUFFER, dogBuf[i][0]);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        //second pass
        glUseProgram(smoothDouble);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pic);
        glUniform1i(smoothDoublePic1, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, dogTex[i][0]);
        glUniform1i(smoothDoublePic0, 1);
        glActiveTexture(GL_TEXTURE0);
        glUniform2f(smoothDoubleDirection, -1.0f / (float) w, 0.0);
        glUniform4fv(smoothDoubleGaussianCoeff, 15, coeffDown1);
        glBindFramebuffer(GL_FRAMEBUFFER, dogBuf[i][1]);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        //vertical smoothing
        glUseProgram(smoothDouble);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, dogTex[i][1]);
        glUniform1i(smoothDoublePic1, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, dogTex[i][1]);
        glUniform1i(smoothDoublePic0, 1);
        glActiveTexture(GL_TEXTURE0);
        glUniform2f(smoothDoubleDirection, 0.0, 1.0f / (float) h);
        glUniform4fv(smoothDoubleGaussianCoeff, 15, coeffDown0);
        glBindFramebuffer(GL_FRAMEBUFFER, dogBuf[i][0]);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        //second pass
        glUseProgram(smoothDouble);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, dogTex[i][1]);
        glUniform1i(smoothDoublePic1, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, dogTex[i][0]);
        glUniform1i(smoothDoublePic0, 1);
        glActiveTexture(GL_TEXTURE0);
        glUniform2f(smoothDoubleDirection, 0.0, -1.0f / (float) h);
        glUniform4fv(smoothDoubleGaussianCoeff, 15, coeffDown1);
        glBindFramebuffer(GL_FRAMEBUFFER, smoothBuf[i][0]);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


        // Last 4 "Half levels"
        //horizontal smoothing
        glUseProgram(smoothDouble);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pic);
        glUniform1i(smoothDoublePic1, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pic);
        glUniform1i(smoothDoublePic0, 1);
        glActiveTexture(GL_TEXTURE0);
        glUniform2f(smoothDoubleDirection, 1.0f / (float) w, 0.0);
        glUniform4fv(smoothDoubleGaussianCoeff, 15, coeffUp0);
        glBindFramebuffer(GL_FRAMEBUFFER, dogBuf[i][0]);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        //second pass
        glUseProgram(smoothDouble);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pic);
        glUniform1i(smoothDoublePic1, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, dogTex[i][0]);
        glUniform1i(smoothDoublePic0, 1);
        glActiveTexture(GL_TEXTURE0);
        glUniform2f(smoothDoubleDirection, -1.0f / (float) w, 0.0);
        glUniform4fv(smoothDoubleGaussianCoeff, 15, coeffUp1);
        glBindFramebuffer(GL_FRAMEBUFFER, dogBuf[i][1]);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        //vertical smoothing
        glUseProgram(smoothDouble);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, dogTex[i][1]);
        glUniform1i(smoothDoublePic1, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, dogTex[i][1]);
        glUniform1i(smoothDoublePic0, 1);
        glActiveTexture(GL_TEXTURE0);
        glUniform2f(smoothDoubleDirection, 0.0, 1.0f / (float) h);
        glUniform4fv(smoothDoubleGaussianCoeff, 15, coeffUp0);
        glBindFramebuffer(GL_FRAMEBUFFER, dogBuf[i][0]);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        //second pass
        glUseProgram(smoothDouble);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, dogTex[i][1]);
        glUniform1i(smoothDoublePic1, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, dogTex[i][0]);
        glUniform1i(smoothDoublePic0, 1);
        glActiveTexture(GL_TEXTURE0);
        glUniform2f(smoothDoubleDirection, 0.0, -1.0f / (float) h);
        glUniform4fv(smoothDoubleGaussianCoeff, 15, coeffUp1);
        glBindFramebuffer(GL_FRAMEBUFFER, smoothBuf[i][1]);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


        //While looping on octaves, we compute the gradients
        //horizontal grad
        glUseProgram(grad);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, smoothTex[i][0]);
        glUniform1i(gradPic0, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, smoothTex[i][1]);
        glUniform1i(gradPic1, 1);
        glActiveTexture(GL_TEXTURE0);
        glUniform2f(gradDirection, 1.0f / (float) w, 0.0);
        glBindFramebuffer(GL_FRAMEBUFFER, gxBuf[i]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        //vertical grad
        glUseProgram(grad);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, smoothTex[i][0]);
        glUniform1i(gradPic0, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, smoothTex[i][1]);
        glUniform1i(gradPic1, 1);
        glActiveTexture(GL_TEXTURE0);
        glUniform2f(gradDirection, 0.0, 1.0f / (float) h);
        glBindFramebuffer(GL_FRAMEBUFFER, gyBuf[i]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


        //DoG computation
        //First 3 levels
        glUseProgram(dog);
        glBindTexture(GL_TEXTURE_2D, smoothTex[i][0]);
        glUniform1i(dogPic0, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, smoothBuf[i][2]);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glUseProgram(smooth);
        glBindTexture(GL_TEXTURE_2D, smoothTex[i][2]);
        glUniform1i(smoothPic0, 0);
        glUniform1fv(smoothGaussianCoeff, 8, coeffDoG);
        glUniform2f(smoothDirection, 1.0f / (float) w, 0.0);
        glBindFramebuffer(GL_FRAMEBUFFER, smoothBuf[i][0]);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glUseProgram(smooth);
        glBindTexture(GL_TEXTURE_2D, smoothTex[i][0]);
        glUniform1i(smoothPic0, 0);
        glUniform1fv(smoothGaussianCoeff, 8, coeffDoG);
        glUniform2f(smoothDirection, 0.0, 1.0f / (float) h);
        glBindFramebuffer(GL_FRAMEBUFFER, dogBuf[i][0]);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);



        //Next and last 3 levels
        glUseProgram(dog);
        glBindTexture(GL_TEXTURE_2D, smoothTex[i][1]);
        glUniform1i(dogPic0, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, smoothBuf[i][3]);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glUseProgram(smooth);
        glBindTexture(GL_TEXTURE_2D, smoothTex[i][3]);
        glUniform1i(smoothPic0, 0);
        glUniform1fv(smoothGaussianCoeff, 8, coeffDoG);
        glUniform2f(smoothDirection, 1.0f / (float) w, 0.0);
        glBindFramebuffer(GL_FRAMEBUFFER, smoothBuf[i][1]);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glUseProgram(smooth);
        glBindTexture(GL_TEXTURE_2D, smoothTex[i][1]);
        glUniform1i(smoothPic0, 0);
        glUniform1fv(smoothGaussianCoeff, 8, coeffDoG);
        glUniform2f(smoothDirection, 0.0, 1.0f / (float) h);
        glBindFramebuffer(GL_FRAMEBUFFER, dogBuf[i][1]);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        //NMS
        glUseProgram(nms);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, dogTex[i][0]);
        glUniform1i(nmsPic0, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, dogTex[i][1]);
        glUniform1i(nmsPic1, 1);
        glActiveTexture(GL_TEXTURE0);
        glUniform1f(nmsWidth, (float) w);
        glUniform1f(nmsHeight, (float) h);
        glBindFramebuffer(GL_FRAMEBUFFER, detBuf[i]);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        //readback to CPU
        glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, nmsOut[i]);


        //Next octave
        w >>= 1;
        h >>= 1;

    }



    //CPU Processing part:
    // we must explore the image and find the coordinates and scales of keypoints,
    // then store them in an array

    w = width;
    h = height;

//    NSMutableArray *tab = [NSMutableArray arrayWithCapacity:0];


    std::vector<MyKeyPoint> srcMyKeyPointList;
    for (int i = 0; i < NB_OCT; i++) {
        for (int j = 0; j < h; j++) {
            for (int k = 0; k < w; k++) {
                for (int l = 0; l < 4; l++) {
                    if (nmsOut[i][4 * (w * j + k) + l] > 0) {
                        MyKeyPoint *key = new MyKeyPoint();
                        key->x = k << i;
                        key->y = j << i;
                        key->level = l + 4 * i;
                        key->s = (float) (1.6 * pow(sqrt(sqrt(2)), (float)(l + 4 * i)));

                        int regSize = (int) (1.0 * sigma[l] * exp2(i));
                        if (key->x > regSize && key->x < (width - regSize)
                            && key->y > regSize
                            && key->y < (height - regSize)) {
                            //todo
//                            LOGI("put keyPoint %d %d %d",key->x,key->y,key->level);
                            srcMyKeyPointList.push_back(*key);
                        }

                    }
                }
            }
        }
        w >>= 1;
        h >>= 1;
    }
//    for (auto val : tab) {
//        LOGI("get keyPoint %d %d %d",val.x,val.y,val.level);
//    }
    //sqSize = size of the square we will use to display the tiled regions of interest
    int sqSize = (int) ceil(sqrt((float) srcMyKeyPointList.size()));
    int nb = srcMyKeyPointList.size();
    LOGI("Src Tab size %d ---------------------", srcMyKeyPointList.size());


    uint8_t *edgeOut = (uint8_t *) calloc((size_t) (sqSize * sqSize * 4), sizeof(uint8_t));
    // back to GPU for edge response and low contrast response suppression
    // first we extract the 3 x 3 pixels (at octave size) region around each keypoint
    // and store them in one single texture.
    glViewport(0, 0, sqSize, sqSize);
    glBindFramebuffer(GL_FRAMEBUFFER, edgeBuf);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_2D, edgeTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sqSize, sqSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    for (int i = 0; i < nb; i++) {
        MyKeyPoint &key = srcMyKeyPointList[i];
        int o = key.level / 4;
        int s = key.level % 4;
        float w = (float) width / (float) exp2(o);
        float h = (float) height / (float) exp2(o);
        float x = (float) key.x / (float) width;
        float y = (float) key.y / (float) height;

        glUseProgram(edgeSuppression);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, dogTex[o][0]);
        glUniform1i(edgeSuppressionPic0, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, dogTex[o][1]);
        glUniform1i(edgeSuppressionPic1, 1);
        glActiveTexture(GL_TEXTURE0);
        glUniform1f(edgeSuppressionWidth, (float) w);
        glUniform1f(edgeSuppressionHeight, (float) h);
        glUniform1i(edgeSuppressionScale, s);
        glUniform2f(edgeSuppressionReadingPosition, x, y);

        GLfloat vCoords[] = {
                (GLfloat) ((float) (i % sqSize) * 2.0 / (float) sqSize - 1.0),
                (GLfloat) ((float) (i / sqSize) * 2.0 / (float) sqSize - 1.0),
                (GLfloat) ((float) (i % sqSize + 1) * 2.0 / (float) sqSize - 1.0),
                (GLfloat) ((float) (i / sqSize) * 2.0 / (float) sqSize - 1.0),
                (GLfloat) ((float) (i % sqSize) * 2.0 / (float) sqSize - 1.0),
                (GLfloat) ((float) (i / sqSize + 1) * 2.0 / (float) sqSize - 1.0),
                (GLfloat) ((float) (i % sqSize + 1) * 2.0 / (float) sqSize - 1.0),
                (GLfloat) ((float) (i / sqSize + 1) * 2.0 / (float) sqSize - 1.0),
        };
        glVertexAttribPointer(edgeSuppressionWritingPosition, 2, GL_FLOAT, GL_FALSE, 0, vCoords);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    //readback to CPU for processing
    glReadPixels(0, 0, sqSize, sqSize, GL_RGBA, GL_UNSIGNED_BYTE, edgeOut);

    // discard the pixels that don't pass the test.
//    for (auto val : tab) {
//        LOGI("before keyPoint  %d %d %d",val.x,val.y,val.level);
//    }
    std::vector<MyKeyPoint> discardResultMyKeyPointList;
    for (int i = 0; i < nb; i++) {
        int discarded = edgeOut[4 * i];
//        LOGI("discard info %d", discarded);
        if (discarded <= 200) {
            discardResultMyKeyPointList.push_back(srcMyKeyPointList[i]);
        }
    }


    LOGI("discard Tab size %d ---------------------", discardResultMyKeyPointList.size());

//    for (auto val : discardResultMyKeyPointList) {
//        LOGI("Discard Result keyPoint  %d %d %d", val.x, val.y, val.level);
//    }
    sqSize = (int) ceil(sqrt((float) discardResultMyKeyPointList.size()));
    nb = discardResultMyKeyPointList.size();

    //back to GPU for orientation computation
    glViewport(0, 0, 16 * sqSize, 16 * sqSize);
    glBindFramebuffer(GL_FRAMEBUFFER, regBuf);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_2D, regTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16 * sqSize, 16 * sqSize, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 0);
    //we tile the regions of interest in one single texture, and compute for each pixel
    //its orientation and weighted magnitude.
//    for (auto val : discardTab) {
//        LOGI("after discarded  keyPoint %d %d %d",val.x,val.y,val.level);
//    }

    for (int i = 0; i < nb; i++) {
        MyKeyPoint &key = discardResultMyKeyPointList.at((unsigned int) i);
        int o = key.level / 4;
        int s = key.level % 4;
        float sig = sigma[s];
        float w = (float) (width >> o);
        float h = (float) (height >> o);
        GLfloat x = (float) key.x / (float) width;
        GLfloat y = (float) key.y / (float) height;
        glUseProgram(orientation);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gxTex[o]);
        glUniform1i(orientationPicGradx, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gyTex[o]);
        glUniform1i(orientationPicGrady, 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gauss);
        glUniform1i(orientationPicGauss, 2);
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(orientationScale, s);
        glUniform1i(orientationTheta, 0);
        GLfloat regCoord[] =
                {
                        (GLfloat) (x - (8.0 * sig) / (float) w),
                        (GLfloat) (y - (8.0 * sig) / (float) h),
                        (GLfloat) (x + (8.0 * sig) / (float) w),
                        (GLfloat) (y - (8.0 * sig) / (float) h),
                        (GLfloat) (x - (8.0 * sig) / (float) w),
                        (GLfloat) (y + (8.0 * sig) / (float) h),
                        (GLfloat) (x + (8.0 * sig) / (float) w),
                        (GLfloat) (y + (8.0 * sig) / (float) h),
                };
        double minX = (double) (i % sqSize) / (double) sqSize * 2.0 - 1.0;
        double maxX = minX + 2.0 / (double) sqSize;
        double minY = (double) (i / sqSize) / (double) sqSize * 2.0 - 1.0;
        double maxY = minY + 2.0 / (double) sqSize;
        GLfloat regVertexCoord[] =
                {
                        (GLfloat) minX, (GLfloat) minY,
                        (GLfloat) maxX, (GLfloat) minY,
                        (GLfloat) minX, (GLfloat) maxY,
                        (GLfloat) maxX, (GLfloat) maxY,
                };
        glVertexAttribPointer(orientationReadingPosition0, 2, GL_FLOAT, GL_FALSE, 0, regCoord);
        glVertexAttribPointer(orientationWritingPosition, 2, GL_FLOAT, GL_FALSE, 0, regVertexCoord);
        glVertexAttribPointer(orientationReadingPosition1, 2, GL_SHORT, GL_FALSE, 0,
                              readingPosition);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    //we compute the main orientation from the previous texture.
    glViewport(0, 0, sqSize, sqSize);
    glBindTexture(GL_TEXTURE_2D, winTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sqSize, sqSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glUseProgram(mainOrientation);
    glBindTexture(GL_TEXTURE_2D, regTex);
    glUniform1i(mainOrientationPic0, 0);
    glVertexAttribPointer(mainOrientationWritingPosition, 2, GL_SHORT, GL_FALSE, 0,
                          writingPosition);
    glVertexAttribPointer(mainOrientationReadingPosition, 2, GL_SHORT, GL_FALSE, 0,
                          readingPosition);
    glUniform1i(mainOrientationSize, sqSize);
    glBindFramebuffer(GL_FRAMEBUFFER, winBuf);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


    //back to CPU to save theta in the array
    uint8_t *winOut = (uint8_t *) calloc((size_t) (sqSize * sqSize * 4), sizeof(uint8_t));
    glReadPixels(0, 0, sqSize, sqSize, GL_RGBA, GL_UNSIGNED_BYTE, winOut);




    for (int i = 0; i < nb; i++) {
        discardResultMyKeyPointList.at((unsigned int) i).t = winOut[4 * i];
        int j = 1;
        while (j < 4 && winOut[4 * i + j] < 100) { //multi-orientation part
            MyKeyPoint *key = new MyKeyPoint();
            MyKeyPoint &point = discardResultMyKeyPointList.at(i);
            key->x = point.x;
            key->y = point.y;
            key->level = point.level;
            //            key->initParamsX(point.getX(),point.getY(),point.getLevel());
            key->s = point.s;
            key->t = winOut[4 * i + j];
            discardResultMyKeyPointList.push_back(*key);
            j++;
        }
    }
    sqSize = (int) ceil(sqrt((float) discardResultMyKeyPointList.size()));
    nb = discardResultMyKeyPointList.size();

//    double tori = [[NSDate date] timeIntervalSince1970] - tori0;
//
//    double tdesc0 = [[NSDate date] timeIntervalSince1970];

    //description: we store the rotated regions of interest in one single texture
    // and compute their new orientation and weighted magnitude.
    // tiled in 30x30 while only the centered 16x16 are useful to allow 45° rotations.
    glViewport(0, 0, 30 * sqSize, 30 * sqSize);
    glBindFramebuffer(GL_FRAMEBUFFER, rotBuf);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_2D, rotTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 30 * sqSize, 30 * sqSize, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 0);
    glBindTexture(GL_TEXTURE_2D, desTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4 * sqSize, 4 * sqSize, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 0);

    for (int i = 0; i < nb; i++) {
        MyKeyPoint &key = discardResultMyKeyPointList.at((unsigned int) i);
//        MyKeyPoint* key = [tab objectAtIndex:i];
        int o = key.level / 4;
        int s = key.level % 4;
        int t = key.t;
        float sig = (float) (M_SQRT2 * sigma[s]);
        float w = (float) (width >> o);
        float h = (float) (height >> o);
        GLfloat x = (float) key.x / (float) width;
        GLfloat y = (float) key.y / (float) height;
        GLfloat posX = (GLfloat) (((float) (i % sqSize) + 0.5) / (float) sqSize * 2.0 - 1.0);
        GLfloat posY = (GLfloat) (((float) (i / sqSize) + 0.5) / (float) sqSize * 2.0 - 1.0);
        GLfloat RotPos[] = { //had to adjustate so there is the right number of pixels:
                (GLfloat) (posX + 1.01 * cos(-(3 + t) * M_PI_4) / (float) sqSize),
                (GLfloat) (posY + sin(-(3 + t) * M_PI_4) / (float) sqSize),
                (GLfloat) (posX + 1.01 * cos(-(1 + t) * M_PI_4) / (float) sqSize),
                (GLfloat) (posY + sin(-(1 + t) * M_PI_4) / (float) sqSize),
                (GLfloat) (posX + 1.01 * cos((3 - t) * M_PI_4) / (float) sqSize),
                (GLfloat) (posY + sin((3 - t) * M_PI_4) / (float) sqSize),
                (GLfloat) (posX + 1.01 * cos((1 - t) * M_PI_4) / (float) sqSize),
                (GLfloat) (posY + sin((1 - t) * M_PI_4) / (float) sqSize),
        };
        glUseProgram(orientation);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gxTex[o]);
        glUniform1i(orientationPicGradx, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gyTex[o]);
        glUniform1i(orientationPicGrady, 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gauss2);
        glUniform1i(orientationPicGauss, 2);
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(orientationScale, s);
        glUniform1i(orientationTheta, t);
        GLfloat regCoord[] =
                {
                        (GLfloat) (x - (8.0 * sig) / (float) w),
                        (GLfloat) (y - (8.0 * sig) / (float) h),
                        (GLfloat) (x + (8.0 * sig) / (float) w),
                        (GLfloat) (y - (8.0 * sig) / (float) h),
                        (GLfloat) (x - (8.0 * sig) / (float) w),
                        (GLfloat) (y + (8.0 * sig) / (float) h),
                        (GLfloat) (x + (8.0 * sig) / (float) w),
                        (GLfloat) (y + (8.0 * sig) / (float) h),
                };
        glVertexAttribPointer(orientationReadingPosition0, 2, GL_FLOAT, GL_FALSE, 0, regCoord);
        glVertexAttribPointer(orientationWritingPosition, 2, GL_FLOAT, GL_FALSE, 0, RotPos);
        glVertexAttribPointer(orientationReadingPosition1, 2, GL_FLOAT, GL_FALSE, 0, gaussCoord);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    }


    //finally we compute the descriptor from the previous texture.
    glViewport(0, 0, 4 * sqSize, 4 * sqSize);
    glUseProgram(descriptor);
    glVertexAttribPointer(descriptorReadingPosition, 2, GL_SHORT, GL_FALSE, 0, readingPosition);
    glVertexAttribPointer(descriptorWritingPosition, 2, GL_SHORT, GL_FALSE, 0, writingPosition);
    glBindTexture(GL_TEXTURE_2D, rotTex);
    glUniform1i(descriptorPic0, 0);
    glUniform1i(descriptorSize, sqSize);
    glBindFramebuffer(GL_FRAMEBUFFER, desBuf);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    //readback descriptor to CPU
    uint8_t *desOut = (uint8_t *) calloc((size_t) (16.0 * sqSize * sqSize * 4), sizeof(uint8_t));
    uint8_t *reorgOut = (uint8_t *) calloc((size_t) (32.0 * 4 * nb), sizeof(uint8_t));
    glReadPixels(0, 0, 4 * sqSize, 4 * sqSize, GL_RGBA, GL_UNSIGNED_BYTE, desOut);
    reorganize(reorgOut, desOut, nb, sqSize);
    normalize(discardResultMyKeyPointList, reorgOut, nb);


    LOGI("discardResultMyKeyPointList size %d ", discardResultMyKeyPointList.size());
//    for (auto val :discardResultMyKeyPointList) {
//
//        cv::circle(src,
//                   cv::Point(val.x, val.y),
//                   3,
//                   cv::Scalar(0, 0, 255),
//                   1,
//                   cv::LINE_AA);
//
////        LOGI("discardResultMyKeyPointList  keyPoint x=%d y=%d level=%d ", val.x, val.y, val.level);
//    }

//    LOGI("SAVE");
//    cv::imwrite("/storage/emulated/0/download/images/001.jpg", src);
    /*
    //writing to text file
    NSMutableString *desString = [NSMutableString stringWithCapacity:0];
    NSMutableString *frameString = [NSMutableString stringWithCapacity:0];
    for (int i=0; i<[tab count]; i++) {
        MyKeyPoint *key=[tab objectAtIndex:i];
        [frameString appendString:[NSString stringWithFormat:@"%u\t%u\t%1.3f\t%1.3f\t",[key getX], height-[key getY]-1, [key getS], M_PI-([key getT])*M_PI_4 ]];
        [frameString appendString:@"\n"];
        uint8_t * values = [key getD];
        for (int j=0; j<128; j++) {
            [desString appendString:[NSString stringWithFormat:@"%u ", values[j]]];
        }
        [desString appendString:@"\n"];
    }
    [desString writeToFile:[NSHomeDirectory() stringByAppendingPathComponent:@"Documents/result.des"] atomically:YES encoding:NSUTF8StringEncoding error:NULL];
    [frameString writeToFile:[NSHomeDirectory() stringByAppendingPathComponent:@"Documents/result.frame"] atomically:YES encoding:NSUTF8StringEncoding error:NULL];
    /* */

    //Gives the number of keypoints per scale

    int count[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    for (int i=0; i<nb; i++) {
        count[discardResultMyKeyPointList.at(i).level]++;
    }

    for (int i = 0; i < 16; ++i) {
        LOGI("Level %d :%d",i,count[i]);
    }

    for (int i = 0; i < discardResultMyKeyPointList.size(); ++i) {
        key_point &point = 
                discardResultMyKeyPointList.at(i);
        uint8_t *values = point.d;
//        for (int j=0; j<128; j++) {
//            LOGI("Describe %u",values[j]);
//        }

    }


//    NSMutableString * display = [NSMutableString stringWithCapacity:0];
//    [display appendString:@"\n"];
//    [display appendString:[NSString stringWithFormat:@"Total Keypoints: %u \n",nb]];
//    for (int i=0; i<16
//            ; i++) {
//        [display appendString:[NSString stringWith Discard keyPoint
//    NSLog(display,nil);
//
//
//    NSTimeInterval total = [[NSDate date] timeIntervalSince1970]-tinit;
//
//    NSString *result = [NSString stringWithFormat:@"Done in %1.3f s \nDetection: %1.3f s\nOrientation: %1.3f s\nDescription: %1.3f s\n",total, tdetect, tori, tdesc];
//    [[[[UIAlertView alloc] initWithTitle:@"Results"
//    message:result
//    delegate:nil
//    cancelButtonTitle:@"OK"
//    otherButtonTitles:nil] autorelease] show];




    return discardResultMyKeyPointList;


}
