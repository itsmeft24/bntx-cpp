// bntx-thingy-for-csk.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <opencv2/opencv.hpp>
#include "BNTX.hpp"

int main(int argc, char* argv[])
{
#ifdef _DEBUG
    cv::Mat surf = cv::imread("0001.jpg", cv::IMREAD_UNCHANGED);
    if (surf.channels() == 3) {
        cv::cvtColor(surf, surf, cv::ColorConversionCodes::COLOR_RGB2RGBA);
    }
    BNTX bntx(surf, "chara_7_metaknight_00");
    bntx.Write("test.bntx");
#else
    cv::Mat surf = cv::imread(argv[1], cv::IMREAD_UNCHANGED);
    if (surf.channels() == 3) {
        cv::cvtColor(surf, surf, cv::ColorConversionCodes::COLOR_RGB2RGBA);
    }
    BNTX bntx(surf, argv[2]);
    bntx.Write(argv[3]);
#endif
}
