/**
 * OpenCVTimeTravel
 * @author Matt Norby
 * May 5, 2018
 * This program takes two images as input.
 * The first is a photo of two kayakers in Indiana, USA from 2012.
 * The second is a photo of a tourist in Koln, Germany from 1997.
 * The tourist is one of the kayakers.
 * The program allows the user to lasso-select a portion of the
 * kayak photo to seamlessly clone into the 1997 photo.
 * Some additional tricks (blurring, saving/restoring foreground pixels)
 * are used to make the photo a bit more realistic.
 */

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv2/photo/photo.hpp>
#include <iostream>
using namespace cv;
using namespace std;

vector<Point2i> pts;

#define RESIZE_FACTOR 8
//#define USE_HARDCODED_POINTS 1

void handleMouseEvent(int event, int x, int y, int flags, void* userdata) {
    // When the user left-clicks...
    if (event == EVENT_LBUTTONDOWN) {
        // Log the location of the click, and add it to a vector.
        cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
        pts.push_back(Point2i(x, y));
    }
}

int main(int argc, char *argv[]) {
    // Load the kayak image from disk
    Mat img = imread("kayak2012.jpg", CV_LOAD_IMAGE_COLOR);
    if (img.empty()) {
        cout << "Cannot open image: kayak2012.jpg" << endl;
        return -1;
    }

    // Resize the image, so that the kayak is a believable size.
    cout << "Resize the source image" << endl;
    resize(img, img, Size2i(img.cols / RESIZE_FACTOR, img.rows / RESIZE_FACTOR));

    // The photo of the kayak is in better focus than the target image.
    // Blur the kayak to compensate.
    cout << "Blur the source image" << endl;
    Mat blurImg = img.clone();
    blur(img, blurImg, Size(3, 3));

    // Seamless cloning will attempt to blend colors around the edges,
    // but not so much in the middle.  Because the water colors are a bit
    // different, we need to darken the water in the kayak photo.  To do that,
    // we use a lookup table.  OpenCV applies it to each channel (RGB).
    // The approach used here is based on observations of pixel values from
    // the kayak water (RGB 100..180) and the Koln water (RGB 100..140).
    cout << "Apply color lookup table to source image" << endl;
    Mat lookupTable(1, 256, CV_8U);
    uchar* ltPtr = lookupTable.ptr();
    for (int i = 0; i < 256; i++) {
        if (i <= 100 || i > 180) {
            // Values outside our range of interest are unchanged
            ltPtr[i] = i;
        } else {
            // Ex. 140 --> 120, 160 --> 130, 180 --> 140
            ltPtr[i] = ((i - 100) / 2) + 100;
        }
    }
    LUT(blurImg, lookupTable, img);

    cout << "Obtain source image mask" << endl;
#ifdef USE_HARDCODED_POINTS
    // These points were used to generate the clone.jpg in the repo.
    pts.push_back(Point2i(383, 196));
    pts.push_back(Point2i(239, 199));
    pts.push_back(Point2i(136, 211));
    pts.push_back(Point2i( 26, 103));
    pts.push_back(Point2i( 71,   1));
    pts.push_back(Point2i(124,   1));
    pts.push_back(Point2i(383, 104));
#else
    // Show the kayak, and let the user click points in the image.
    // When the user presses any key on the keyboard, cloning proceeds.
    // For best results, the user should loosely lasso the kayak,
    // paddlers, and the reflection in the water.
    namedWindow("Kayak", CV_WINDOW_AUTOSIZE);
    setMouseCallback("Kayak", handleMouseEvent, 0);
    imshow("Kayak", img);
    waitKey(0);
    setMouseCallback("Kayak", 0, 0);
#endif // USE_HARDCODED_POINTS

    // We need at least three points for a polygonal region.
    if (pts.size() >= 3) {
        // Create a mask from a filled polygon whose vertices are the selected points.
        // We will use the mask to limit the area affected by the seamless cloning routine.
        Mat mouseMask(img.size(), img.type(), Scalar(0, 0, 0));
        vector<vector<Point2i> > mouseContours;
        mouseContours.push_back(pts);
        drawContours(mouseMask, mouseContours, 0, Scalar(255, 255, 255), CV_FILLED);

        // Show the user the region of the kayak that was actually selected.
        cout << "Show source image mask" << endl;
        Mat selectedRegion(img.size(), img.type(), Scalar(0, 0, 0));
        bitwise_and(img, mouseMask, selectedRegion);
        namedWindow("Selected Region", CV_WINDOW_AUTOSIZE);
        imshow("Selected Region", selectedRegion);

        // Load the target image
        cout << "Load target image" << endl;
        Mat imgTarget = imread("koln1997.jpg", IMREAD_COLOR);
        if (imgTarget.empty()) {
            cout << "Cannot open image: koln1997.jpg" << endl;
            return -1;
        }

        // Because the front of the kayak is missing, we will place the
        // kayak in the 1997 image, where the front will be occluded
        // by the tourist in the foreground.  But that creates a problem
        // for seamless cloning, which wants to mix colors near the seam.
        // Create a mask for the foreground portion of the target image.
        // We will restore this area after cloning.
        // Because the tolerances are lower for these selections, the
        // pixels were selected via mouse, and hardcoded here.
        cout << "Create a mask for the foreground of the target image" << endl;
        vector<vector<Point2i> > fgContours;
        vector<Point2i> fgPts;
        fgPts.push_back(Point2i(860, 1184));
        fgPts.push_back(Point2i(848, 1112));
        fgPts.push_back(Point2i(844, 1060));
        fgPts.push_back(Point2i(832, 980));
        fgPts.push_back(Point2i(836, 916));
        fgPts.push_back(Point2i(828, 908));
        fgPts.push_back(Point2i(856, 796));
        fgPts.push_back(Point2i(912, 748));
        fgPts.push_back(Point2i(1084, 748));
        fgPts.push_back(Point2i(1084, 1180));
        fgContours.push_back(fgPts);
        Mat fgMask(imgTarget.size(), imgTarget.type(), Scalar(0, 0, 0));
        drawContours(fgMask, fgContours, 0, Scalar(255, 255, 255), CV_FILLED);

        cout << "Seamless cloning in progress" << endl;
        Mat imgFinal;
        Point2i centerPt(imgTarget.cols * 2 / 5, imgTarget.rows * 3 / 4);
        seamlessClone(img, imgTarget, mouseMask, centerPt, imgFinal, NORMAL_CLONE);

        // Restore the foreground portion, using the mask we created earlier.
        cout << "Restore foreground portion of target image" << endl;
        imgTarget.copyTo(imgFinal, fgMask);

        cout << "Write output image to file" << endl;
        imwrite("clone.jpg", imgFinal);

        cout << "Resize and show final image" << endl;
        resize(imgFinal, imgFinal, Size2i(imgFinal.cols / 2, imgFinal.rows / 2));
        namedWindow("Clone", CV_WINDOW_AUTOSIZE);
        imshow("Clone", imgFinal);
        waitKey(0);
    }

    return 0;
}
