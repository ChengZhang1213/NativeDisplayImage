//
// Created by Android on 2017/5/12.
//

#ifndef NATIVEDISPLAYIMAGE_KEYPOINT_H
#define NATIVEDISPLAYIMAGE_KEYPOINT_H
#include <cstdint>
typedef struct key_point{
    int x;
    int y;
    int level;
    float s;
    int t;
    uint8_t * d;
}MyKeyPoint;
#endif //NATIVEDISPLAYIMAGE_KEYPOINT_H
