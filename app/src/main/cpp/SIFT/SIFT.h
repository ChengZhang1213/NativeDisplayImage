//
// Created by Android on 2017/5/4.
//

#ifndef GPUSIFTANDROID_SIFT_H
#define GPUSIFTANDROID_SIFT_H
#include <cstdint>
#include <vector>
#include <opencv2/core/mat.hpp>
#include "GLES2/gl2.h"
#include "KeyPoint.h"

void initWithHeight(int picWidth,int picHeight,int oct);
std::vector<MyKeyPoint> computeSiftImage(cv::Mat mat);

#endif //GPUSIFTANDROID_SIFT_H
