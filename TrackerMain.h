#pragma once
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <Windows.h>
#include <iostream>
#include <string>

// function headers
cv::Mat hwnd2mat(HWND hwnd);
cv::Mat findActiveBox(cv::Mat greyScaled);
void identifyProgram(cv::Mat activeHist);
