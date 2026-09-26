#pragma once
#include "hikrobot/include/MvCameraControl.h"
#include <opencv2/opencv.hpp>
#include <iostream>

class Camera {
    public:
    Camera();

    ~Camera();

    bool read(cv::Mat& img);

    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;

    private:

    void* handle_;
    bool is_grabbing_;

    bool setDefaultParams_();
    cv::Mat convertFrame_(MV_FRAME_OUT& raw);
};