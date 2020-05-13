#include <opencv2/opencv.hpp>
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <Windows.h>
#include <iostream>
#include <string>
#include "TrackerMain.h"

using namespace std;
using namespace cv;



Mat hwnd2mat(HWND hwnd) {

    HDC hwindowDC, hwindowCompatibleDC;

    int height, width, srcheight, srcwidth;
    HBITMAP hbwindow;
    Mat src;
    BITMAPINFOHEADER  bi;

    hwindowDC = GetDC(hwnd);
    hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
    SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

    RECT windowsize;    // get the height and width of the screen
    //GetClientRect(hwnd, &windowsize); - does not capture the entirety of the screen, excludes taskbar
    GetWindowRect(hwnd, &windowsize);

    // TODO: Need to figure out why GetWindowRect is only capturing half of user window
    srcheight = 1075;   //1100
    srcwidth = 1920;    //1923

    height = windowsize.bottom / 1;  //change this to whatever size you want to resize to
    width = windowsize.right / 1;

    src.create(height, width, CV_8UC4);

    // create a bitmap
    hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
    bi.biSize = sizeof(BITMAPINFOHEADER);    //http://msdn.microsoft.com/en-us/library/windows/window/dd183402%28v=vs.85%29.aspx
    bi.biWidth = width;
    bi.biHeight = -height;  //this is the line that makes it draw upside down or not
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    // use the previously created device context with the bitmap
    SelectObject(hwindowCompatibleDC, hbwindow);
    // copy from the window device context to the bitmap device context
    StretchBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, 0, 0, srcwidth, srcheight, SRCCOPY); //change SRCCOPY to NOTSRCCOPY for wacky colors !
    GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, src.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);  //copy from hwindowCompatibleDC to hbwindow

    // avoid memory leak
    DeleteObject(hbwindow); DeleteDC(hwindowCompatibleDC); ReleaseDC(hwnd, hwindowDC);

    return src;
}

// returns thresholded image for contour identification
Mat findActiveBox(Mat greyScaled)
{
    Mat thresholded;

    threshold(greyScaled, thresholded, 46, 0, THRESH_TOZERO);   // 46
    threshold(thresholded, thresholded, 53, 0, THRESH_TOZERO_INV);  // 53

    return thresholded;
}

// Compares active histogram to pre-generated histograms to check for active program
void identifyProgram(Mat activeHist)
{
    static bool first = true;
    static Mat popupWindowHist;
    static Mat leagueHist;

    if (first)
    {
        // declaring global images/histograms for comparison
        Mat popupWindow = imread("PromptPopup.png");
        Mat leagueWindow = imread("LeagueOfLegends.png");
        vector<Mat> bgrPlanes;

        float range[] = { 0, 256 }; const float* histRange = { range }; int histSize = 256; int channels[] = { 0, 1 };
        //TODO - debugging histogram comparison
        split(popupWindow, bgrPlanes);
        calcHist(&bgrPlanes[0], 1, 0, Mat(), popupWindowHist, 1, &histSize, &histRange, true, false);
        normalize(popupWindowHist, popupWindowHist, 0, 1, NORM_MINMAX, -1, Mat());

        split(leagueWindow, bgrPlanes);
        calcHist(&bgrPlanes[0], 1, 0, Mat(), leagueHist, 1, &histSize, &histRange, true, false);
        normalize(leagueHist, leagueHist, 0, 1, NORM_MINMAX, -1, Mat());

        first = false;
    }

    vector<double> inter = { compareHist(leagueHist, activeHist, HISTCMP_INTERSECT), 
                             compareHist(popupWindowHist, activeHist, HISTCMP_INTERSECT) };

    int dist = distance(inter.begin(), max_element(inter.begin(), inter.end()));

    cout << dist;
}

int main()
{
    int kSize = 3;
    int scale = 1;
    int delta = 0;
    int ddepth = CV_16S;

    //window variables
    HWND window = GetDesktopWindow();
    Mat taskbar;
    
    //Cropping rect to capture taskbar
    Rect r = Rect(0, 837, 1000, 27);

    taskbar = hwnd2mat(window)(r);
    Mat popupWindow = imread("PromptPopup.png");
    Mat leagueWindow = imread("LeagueOfLegends.png");

    //taskbar = imread("TaskbarCapture.png");

    // Converting to greyscaled for easier contour capture
    Mat greyScaled;
    cvtColor(taskbar, greyScaled, COLOR_BGR2GRAY, 0);

    Mat thresholded = findActiveBox(greyScaled);

    // eroding to remove false contours from other program emblems
    erode(thresholded, thresholded, getStructuringElement(MORPH_ELLIPSE, Size(6, 6)));
   
    // calculating image moments for shape detection
    Moments thresholdMoments = moments(thresholded);
    int posX = thresholdMoments.m10; int posY = thresholdMoments.m01; int contourArea = thresholdMoments.m00;

    Point p = Point(thresholdMoments.m10/thresholdMoments.m00, thresholdMoments.m01/thresholdMoments.m00);

    //retrieving active program's emblem
    Rect activeBox = Rect((posX / contourArea) - 10, 5, 20, 20);
    Mat active = taskbar(activeBox); 

    vector<Mat> bgrPlanes;
    split(active, bgrPlanes);

    // configuring histogram for active window 
    Mat activeHist; float range[] = { 0, 256 }; const float* histRange = { range }; int histSize = 256; int channels[] = { 0, 1 };
    calcHist(&bgrPlanes[0], 1, 0, Mat(), activeHist, 1, &histSize, &histRange, true, false);

    /*calcHist(&active, 1, channels, Mat(), activeHist, 1, &histSize, &histRange, true, false);*/
    normalize(activeHist, activeHist, 0, 1, NORM_MINMAX, -1, Mat()); 

    identifyProgram(activeHist);

    imshow("HWND", active);
    waitKey(0);

    return 0;
}
