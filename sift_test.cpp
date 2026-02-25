#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/features2d.hpp>
#include <iostream>
#include <vector>

using namespace std;
using namespace cv;

int main(int argc, char** argv)
{
    // INITIALIZE CAMERA 
   // GStreamer pipeline 
string pipeline =
  "libcamerasrc ! "
	"video/x-raw,width=2328,height=1748,format=BGRx !"
	"videoconvert !"
	"video/x-raw,format=BGR !"
	"queue ! appsink drop =true";
    
VideoCapture cap(pipeline, cv::CAP_GSTREAMER);
    
    if (!cap.isOpened()) {
        cerr << "Error: Could not open camera. Check connection or index." << endl;
        return -1;
    }

    Mat frame, queryImg, trainImg;
    int captureCount = 0;

    cout << "--- Camera Started ---" << endl;
    cout << "Press 'c' to capture an image." << endl;
    cout << "1st Capture = First Image (Object to find)" << endl;
    cout << "2nd Capture = Second Image (Scene to search in)" << endl;
    cout << "Press 'q' to quit early." << endl;

    //  CAPTURE LOOP
    while (true) {
        cap >> frame; 
        if (frame.empty()) {
            cerr << "Error: Blank frame grabbed" << endl;
            break;
        }

        
        imshow("ArduCam Feed", frame);

        char key = (char)waitKey(30); 

        if (key == 'q') {
            return 0;
        }
        else if (key == 'c') {
            if (captureCount == 0) {
                queryImg = frame.clone();
                imwrite("capture_1_query.jpg", queryImg); 
                cout << "Captured Query Image. Saved as capture_1_query.jpg" << endl;
                captureCount++;
                
                waitKey(500); 
            }
            else if (captureCount == 1) {
                trainImg = frame.clone();
                imwrite("capture_2_train.jpg", trainImg); 
                cout << "Captured Train Image. Saved as capture_2_train.jpg" << endl;
                break; 
            }
        }
    }

    // Release camera
    cap.release();
    destroyWindow("ArduCam Feed");

    // SIFT PROCESSING
    cout << "Processing SIFT matching..." << endl;

    //grayscale convertion for feature matching
    Mat queryGray, trainGray;
    cvtColor(queryImg, queryGray, COLOR_BGR2GRAY);
    cvtColor(trainImg, trainGray, COLOR_BGR2GRAY);

    // Initialize SIFT detector 
   
    Ptr<SIFT> detector = SIFT::create(400);

    vector<KeyPoint> queryKeypoints, trainKeypoints;
    Mat queryDescriptors, trainDescriptors;

    // Detect keypoints and compute descriptors
    detector->detectAndCompute(queryGray, noArray(), queryKeypoints, queryDescriptors);
    detector->detectAndCompute(trainGray, noArray(), trainKeypoints, trainDescriptors);

    printf("Found %d query keypoints and %d train keypoints.\n", 
           (int)queryKeypoints.size(), (int)trainKeypoints.size());

    //MATCHING

    BFMatcher matcher(NORM_L2);

    
    //descriptors check
    if (queryDescriptors.empty() || trainDescriptors.empty()) {
        cerr << "Error: No descriptors found in one of the images." << endl;
        return -1;
    }

    vector<vector<DMatch>> knnMatches;
matcher.knnMatch(queryDescriptors, trainDescriptors, knnMatches, 2);

//ratio test to filter matches
vector<DMatch> goodMatches;
for (size_t i = 0; i < knnMatches.size(); i++) {
       
        if (knnMatches[i].size() >= 2) {
            if (knnMatches[i][0].distance < 0.75 * knnMatches[i][1].distance) {
                goodMatches.push_back(knnMatches[i][0]);
            }
        }
    }
    printf("Found %d good matches.\n", (int)goodMatches.size());

//extrct match points for homography
    vector<Point2f> pts_src;
    vector<Point2f> pts_dst;

for (size_t i = 0; i < goodMatches.size(); i++) {
    pts_src.push_back(queryKeypoints[goodMatches[i].queryIdx].pt);
    pts_dst.push_back(trainKeypoints[goodMatches[i].trainIdx].pt);
}
// Calculate homography
    Mat H;
    if (pts_src.size() >= 4) {
    H = findHomography(pts_src, pts_dst, RANSAC);
}
// warp two images 
    Mat warped;

    if (!H.empty()) {
    warpPerspective(queryImg, warped, H, trainImg.size());

imwrite("warp_result.jpg", warped);
    imshow("Warped Query Image", warped);
}

    // DISPLAY AND STORE RESULTS
    Mat img_matches;
    // Draw matches
    drawMatches(queryImg, queryKeypoints, trainImg, trainKeypoints, goodMatches, img_matches);

    // Save the result
    imwrite("sift_result.jpg", img_matches);
    cout << "Result saved as sift_result.jpg" << endl;

    // Show the result
    namedWindow("SIFT Matches", WINDOW_AUTOSIZE);
    imshow("SIFT Matches", img_matches);
    
    cout << "Press any key to exit..." << endl;
    waitKey(0);

    return 0;

}
