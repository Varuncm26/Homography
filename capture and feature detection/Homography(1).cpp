#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/features2d.hpp>
#include <iostream>
#include <vector>

using namespace std;
using namespace cv;

bool loadCameraMatrix(Mat& cameraMatrix, Mat& distCoeffs) {
    // LOAD CAMERA MATRIX
    FileStorage fs("calib.yml", FileStorage::READ);
    if (!fs.isOpened()) {
        cerr << "Error: Could not open calib.yml" << endl;
        return false;
    }

    fs["camera_matrix"] >> cameraMatrix;
    fs["dist_coeffs"] >> distCoeffs;
    fs.release();

    cout << "Camera Matrix loaded." << endl;
    return true;
}

bool loadImages(Mat& queryImg, Mat& trainImg) {
    // LOAD IMAGES 
    queryImg = imread("capture_1.jpg");
    trainImg = imread("capture_2.jpg");

    if (queryImg.empty() || trainImg.empty()) {
        cerr << "Error: Could not load images. Run the capture program first." << endl;
        return false;
    }
    return true;
}

void undistortImages(Mat& queryImg, Mat& trainImg, const Mat& cameraMatrix, const Mat& distCoeffs) {
    Mat queryUndistorted, trainUndistorted;

    // Undistort the images
    undistort(queryImg, queryUndistorted, cameraMatrix, distCoeffs);
    undistort(trainImg, trainUndistorted, cameraMatrix, distCoeffs);

    queryImg = queryUndistorted;
    trainImg = trainUndistorted;
}

void detectFeatures(const Mat& queryImg, const Mat& trainImg, 
                    vector<KeyPoint>& queryKeypoints, vector<KeyPoint>& trainKeypoints, 
                    Mat& queryDescriptors, Mat& trainDescriptors) {
    
    // Grayscale conversion
    Mat queryGray, trainGray;
    cvtColor(queryImg, queryGray, COLOR_BGR2GRAY);
    cvtColor(trainImg, trainGray, COLOR_BGR2GRAY);

    // Initialize SIFT detector 
    Ptr<SIFT> detector = SIFT::create(400);

    // // Initialize ORB detector
    // Ptr<ORB> detector = ORB::create(5000);

    // Detect keypoints and compute descriptors
    detector->detectAndCompute(queryGray, noArray(), queryKeypoints, queryDescriptors);
    detector->detectAndCompute(trainGray, noArray(), trainKeypoints, trainDescriptors);

    printf("Found %d query keypoints and %d train keypoints.\n", 
           (int)queryKeypoints.size(), (int)trainKeypoints.size());
}

vector<DMatch> matchFeatures(const Mat& queryDescriptors, const Mat& trainDescriptors) {
    // MATCHING 
    BFMatcher matcher(NORM_L2);
    // BFMatcher matcher(NORM_HAMMING); // for ORB FEATURE DESCRIPTOR

    // Check for descriptors
    if (queryDescriptors.empty() || trainDescriptors.empty()) {
        cerr << "Error: No descriptors found in one of the images." << endl;
        return vector<DMatch>();
    }

    vector<vector<DMatch>> knnMatches;
    matcher.knnMatch(queryDescriptors, trainDescriptors, knnMatches, 2);

    // Ratio test to filter matches
    vector<DMatch> goodMatches;
    for (size_t i = 0; i < knnMatches.size(); i++) {
        if (knnMatches[i].size() >= 2) {
            if (knnMatches[i][0].distance < 0.75 * knnMatches[i][1].distance) {
                goodMatches.push_back(knnMatches[i][0]);
            }
        }
    }
    printf("Found %d good matches.\n", (int)goodMatches.size());
    return goodMatches;
}

Mat computeHomographyAndFilter(const vector<KeyPoint>& queryKeypoints, const vector<KeyPoint>& trainKeypoints, 
                               const vector<DMatch>& goodMatches, 
                               vector<DMatch>& inliers, vector<DMatch>& outliers, Mat& ransacMask) {
    
    // HOMOGRAPHY 
    // Extract match points
    vector<Point2f> pts_src;
    vector<Point2f> pts_dst;

    for (size_t i = 0; i < goodMatches.size(); i++) {
        pts_src.push_back(queryKeypoints[goodMatches[i].queryIdx].pt);
        pts_dst.push_back(trainKeypoints[goodMatches[i].trainIdx].pt);
    }

    // Calculate homography
    Mat H;
    if (pts_src.size() >= 4) {
        H = findHomography(pts_src, pts_dst, RANSAC, 3, ransacMask);
    } else {
        cerr << "Warning: Not enough matches to calculate homography." << endl;
        return Mat();
    }

    // SEPARATE MATCHES
    for (size_t i = 0; i < goodMatches.size(); i++) {
        if (ransacMask.at<uchar>(i) != 0) {
            inliers.push_back(goodMatches[i]);  // Match 
        }
        else {
            outliers.push_back(goodMatches[i]); // Error
        }
    }

    cout << "RANSAC Result: " << inliers.size() << " Inliers, " << outliers.size() << " Outliers." << endl;
    return H;
}

void performDecomposition(const Mat& H, const Mat& cameraMatrix) {
    if (H.empty()) return;

    // DECOMPOSE HOMOGRAPHY 
    vector<Mat> Rs, Ts, Ns;
    
    int solutions = decomposeHomographyMat(H, cameraMatrix, Rs, Ts, Ns);   // produces 4 solutions for R, T, N

    cout << "Decomposition found " << solutions << " possible solutions." << endl;
    cout << "---------------------------------------------------" << endl;

    // PRINT SOLUTIONS
    for (int i = 0; i < solutions; i++) {
        cout << "Solution #" << i + 1 << ":" << endl;
        
        cout << "  Rotation (R): " << endl << Rs[i] << endl;
        cout << "  Translation (T): " << endl << Ts[i] << endl;
        cout << "  Plane Normal (N): " << endl << Ns[i] << endl;
        
        // solution filtering 
        if (Ns[i].at<double>(2) > 0) {
             cout << "  (Note: This normal points towards the camera)" << endl;
        }
    }
}

void visualizeMatches(const Mat& queryImg, const Mat& trainImg, 
                      const vector<KeyPoint>& queryKeypoints, const vector<KeyPoint>& trainKeypoints,
                      const vector<DMatch>& inliers, const vector<DMatch>& outliers, 
                      const vector<DMatch>& goodMatches) {
    
    // inlier and outlier visualization
    Mat img_matches;

    drawMatches(queryImg, queryKeypoints, trainImg, trainKeypoints, outliers, img_matches,
                Scalar(0, 0, 255), Scalar::all(-1), vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

    drawMatches(queryImg, queryKeypoints, trainImg, trainKeypoints, inliers, img_matches,
                Scalar(0, 255, 0), Scalar::all(-1), vector<char>(), 
                DrawMatchesFlags::DRAW_OVER_OUTIMG | DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

    // Save the result
    imwrite("ransac_result.jpg", img_matches);
    cout << "Result saved as ransac_result.jpg (Green=Inliers, Red=Outliers)" << endl;

    // Show the result
    namedWindow("RANSAC Inliers/Outliers", WINDOW_AUTOSIZE);
    imshow("RANSAC Inliers/Outliers", img_matches);
    waitKey(0);

    // DISPLAY AND STORE RESULTS 
    drawMatches(queryImg, queryKeypoints, trainImg, trainKeypoints, goodMatches, img_matches);

    // Save the result
    imwrite("featuremap_result.jpg", img_matches);
    cout << "Result saved as featuremap_result.jpg" << endl;

    // Show the result
    namedWindow("Feature Matches", WINDOW_AUTOSIZE);
    imshow("Feature Matches", img_matches);
    
    cout << "Press any key to exit" << endl;
    waitKey(0);
}


int main()
{
    // Load Calibration 
    Mat cameraMatrix, distCoeffs;
    if (!loadCameraMatrix(cameraMatrix, distCoeffs)) return -1;

    //  Load Images
    Mat queryImg, trainImg;
    if (!loadImages(queryImg, trainImg)) return -1;

    //  Undistort
   // undistortImages(queryImg, trainImg, cameraMatrix, distCoeffs);

    //  Feature Detection (SIFT)
    vector<KeyPoint> queryKeypoints, trainKeypoints;
    Mat queryDescriptors, trainDescriptors;
    detectFeatures(queryImg, trainImg, queryKeypoints, trainKeypoints, queryDescriptors, trainDescriptors);

    //  Feature Matching
    vector<DMatch> goodMatches = matchFeatures(queryDescriptors, trainDescriptors);

    // Compute Homography & Filter Inliers/Outliers
    vector<DMatch> inliers, outliers;
    Mat ransacMask;
    Mat H = computeHomographyAndFilter(queryKeypoints, trainKeypoints, goodMatches, inliers, outliers, ransacMask);

    // Decompose Homography
    if (!H.empty()) {
        performDecomposition(H, cameraMatrix);
    

    // Visualization
    visualizeMatches(queryImg, trainImg, queryKeypoints, trainKeypoints, inliers, outliers, goodMatches);

    return 0;
}