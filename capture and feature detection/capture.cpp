#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp> 
#include <iostream>

using namespace std;
using namespace cv;

int main()
{
    
    // INITIALIZE CAMERA 
VideoCapture cap(0, cv::CAP_V4L2); 

if (!cap.isOpened()) {
    cerr << "Error: Could not open camera. Check connection." << endl;
    return -1;
}

// Set width and height same as used in calibration
cap.set(CAP_PROP_FRAME_WIDTH, 2328);
cap.set(CAP_PROP_FRAME_HEIGHT, 1748);
    // Set Video format MJPG (Motion JPEG) 
cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M','J','P','G'));
    
    // Set frames per second (FPS) 
    cap.set(cv::CAP_PROP_FPS, 30);


    Mat frame;
    int captureCount = 0;

    cout << "--- Camera Started ---" << endl;
    cout << "Press 'c' to capture an image." << endl;
    cout << "1st Capture = Query Image (Object)" << endl;
    cout << "2nd Capture = Train Image (Scene)" << endl;
    cout << "Press 'q' to quit." << endl;

    // CAPTURE LOOP 
    while (true) {
        cap >> frame;
        if (frame.empty()) {
            cerr << "Error: Blank frame grabbed" << endl;
            break;
        }

        imshow("ArduCam Feed", frame);

        char key = (char)waitKey(30);

        if (key == 'q') {
            break;
        }
        else if (key == 'c') {
            if (captureCount == 0) {
                // Save first image
                imwrite("capture_1.jpg", frame);
                cout << "Captured Query Image. Saved as capture_1.jpg" << endl;
                captureCount++;
                
                waitKey(500); 
            }
            else if (captureCount == 1) {
                // Save second image
                imwrite("capture_2.jpg", frame);
                cout << "Captured Train Image. Saved as capture_2.jpg" << endl;
                
                cout << "Both images captured. Exiting..." << endl;
                break; 
            }
        }
    }

    // Release camera
    cap.release();
    destroyWindow("ArduCam Feed");

    return 0;
}