#ifdef __linux__
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#elif defined(_WIN32) || defined(_WIN64)
#include <conio.h>
#endif

//Include header files
#include <iostream>
#include <string>
#include <ctime>
#include <ratio>
#include <chrono>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/tracking/tracker.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <stdlib.h>
#include <stdio.h>

#include "dynamixel_sdk.h"                                  // Uses Dynamixel SDK library

//Defining namespaces
using namespace std;
using namespace cv;
using namespace std::chrono;

//Color definitions for the applications color scheme
Scalar Green = Scalar(179, 239, 194); //Green
Scalar Blue = Scalar(177, 171, 151); //Blue
Scalar Purple = Scalar(114, 111, 116); //Purple
Scalar Brown = Scalar(61, 95, 115); //Brown

//Defining Global Variables
double Prediction_X;
double Prediction_Y;
double Smooth_X;
double Smooth_Y;
Point returnPoint;
Point PredictionPoint = Point(1, 1);
Point SmoothPoint = Point(1, 1);
Point LButtonDown = Point(0, 0);
Point LButtonHold = Point(0, 0);
Point CheckPoint;
bool MouseClicked = false;

// Control table address
#define ADDR_MX_TORQUE_ENABLE           24                  // Control table address is different in Dynamixel model
#define ADDR_MX_GOAL_POSITION           30
#define ADDR_MX_PRESENT_POSITION        36

// Protocol version
#define PROTOCOL_VERSION                1.0                 // See which protocol version is used in the Dynamixel

// Default setting
#define DXL_ID                          4                   // Dynamixel ID: 1
#define BAUDRATE                        57600
#define DEVICENAME                      "/dev/ttyUSB0"      // Check which port is being used on your controller
                                                            // ex) Windows: "COM1"   Linux: "/dev/ttyUSB0"
#define MOVING_SPEED                    32
#define TORQUE_ENABLE                   1                   // Value for enabling the torque
#define TORQUE_DISABLE                  0                   // Value for disabling the torque
#define DXL_MINIMUM_POSITION_VALUE      0                // Dynamixel will rotate between this value
#define DXL_MAXIMUM_POSITION_VALUE      1023             // and this value (note that the Dynamixel would not move when the position value is out of movable range. Check e-manual about the range of the Dynamixel you use.)

#define ESC_ASCII_VALUE                 0x1b

//Function which triggers when Mouse Movements or Clicks are detected.
//Used for selecting the Region of interest (ROI)
void CallBackFunc(int event, int x, int y, int flags, void* userdata) {
  if (event == EVENT_LBUTTONDOWN && MouseClicked == false) {
    LButtonDown = Point(x, y);
    MouseClicked = true;
  }
  if (event == EVENT_MOUSEMOVE && MouseClicked == true) {
    LButtonHold = Point(x, y);
  }
  if (event == EVENT_LBUTTONUP) {
    MouseClicked = false;
  }
  if (event == EVENT_MOUSEMOVE && flags == EVENT_FLAG_SHIFTKEY) {
    int TempLButtonDownX = LButtonDown.x - x;
    int TempLButtonDownY = LButtonDown.y - y;
    int TempLButtonHoldX = LButtonHold.x - TempLButtonDownX;
    int TempLButtonHoldY = LButtonHold.y - TempLButtonDownY;

    LButtonDown = Point(x, y);
    LButtonHold = Point(TempLButtonHoldX, TempLButtonHoldY);
  }
  if (event == EVENT_MOUSEMOVE && flags == EVENT_FLAG_CTRLKEY) {
    int TempLButtonDownX = LButtonDown.x - x;
    int TempLButtonDownY = LButtonDown.y - y;
    int TempLButtonHoldX = LButtonHold.x + TempLButtonDownX;
    int TempLButtonHoldY = LButtonHold.y + TempLButtonDownY;

    LButtonDown = Point(x, y);
    LButtonHold = Point(TempLButtonHoldX, TempLButtonHoldY);
  }
}

//Function for Drawing a smoothed out Rectangle (So Camera wont wiggle)
Point SmoothFrame (Mat frame, Point Point_1, Point Point_2, Point Point_3, Point Point_4, Point Point_5) {
  //Calculate the weighted mean
  Smooth_X = ((Point_1.x * 0.4) + (Point_2.x * 0.3) + (Point_3.x * 0.2) + (Point_4.x * 0.05) + (Point_5.x * 0.05));
  Smooth_Y = ((Point_1.y * 0.4) + (Point_2.y * 0.3) + (Point_3.y * 0.2) + (Point_4.y * 0.05) + (Point_5.y * 0.05));

  //Make Point to return
  returnPoint = Point((Smooth_X), (Smooth_Y));

  //Draw Rectangle (For Smooth Follow Visuals)
  rectangle(frame, Point((Smooth_X - 50), (Smooth_Y - 50)), Point((Smooth_X + 50), (Smooth_Y + 50)), Green, 2, 8, 0);

  //Return Value
  return returnPoint;
}

//Function for Calcualting a smooth follow
void SmoothPrediction (Mat frame, Point Middle, Point Point_1, Point Point_2, Point Point_3, Point Point_4, Point Point_5) {
  //Calculate the weighted mean
  Prediction_X = (((Point_1.x - Point_2.x) * 0.4) + ((Point_2.x - Point_3.x) * 0.3) + ((Point_3.x - Point_4.x) * 0.2) + ((Point_4.x - Point_5.x) * 0.1)) / 4;
  Prediction_Y = (((Point_1.y - Point_2.y) * 0.4) + ((Point_2.y - Point_3.y) * 0.3) + ((Point_3.y - Point_4.y) * 0.2) + ((Point_4.y - Point_5.y) * 0.1)) / 4;

  //Create the Point where the prediction will be
  PredictionPoint = Point(Point_1.x + Prediction_X, Point_1.y + Prediction_Y);

  //Draw Arrowed Line (For SMooth Follow Visuals)
  arrowedLine(frame, Middle, PredictionPoint, Blue, 2, 2, 0, 0.1);
}

//Function for handling the whole GUI
void GUISetting (Mat frame, Point Middle, Point FrameInfo, double PictureTime, string Tracker) {
  //Draw Background behind Text
  line(frame, Point(0, 400), Point(800, 400), Purple, 800, 8);

  //Put Coordinates on Screen
  putText(frame, "Middle:", Point(0, 30), FONT_HERSHEY_PLAIN, 1, Green,2, 8);
  putText(frame, "X: " + to_string(Middle.x) + ", Y: " + to_string(Middle.y), Point(0, 60), FONT_HERSHEY_PLAIN, 1,Green,1, 8);

  //Write Text on Screen
  putText(frame, "Smoothing Ratio (Higer Value means faster movement):", Point(0, 90), FONT_HERSHEY_PLAIN, 1,Green,2, 8);
  putText(frame, "X: " + to_string(Prediction_X) + ", Y: " + to_string(Prediction_Y), Point(0, 120), FONT_HERSHEY_PLAIN, 1,Green,1, 8);

  //Display Frame Info
  putText(frame, "Video & Tracker Information", Point(0, 180), FONT_HERSHEY_PLAIN, 1,Blue,2, 8);
  putText(frame, "Width: " + to_string(FrameInfo.x) + "px , Height: " + to_string(FrameInfo.y) + "px", Point(0, 210), FONT_HERSHEY_PLAIN, 1,Blue,1, 8);

  //Display FPS (+ Calculation)
  double FPS = 1000/PictureTime;
  putText(frame, "FPS: " + to_string(FPS) + ", Millseconds per frame: " + to_string(PictureTime), Point(0, 240), FONT_HERSHEY_PLAIN, 1,Blue,1, 8);

  //Display Tracker Name
  putText(frame, "Tracker: " + Tracker, Point(0, 270), FONT_HERSHEY_PLAIN, 1,Blue,1, 8);

  //Display the frame
  imshow("Display window", frame);
  resizeWindow("Display window", 800, 795);
}

int main() {

  //Clear Console and First Words
  printf("\033c");
  cout << "Welcome to TrackAIr! By Luca, Kay, Gianluca with <3" << endl;

  // Initialize PortHandler instance
  // Set the port path
  // Get methods and members of PortHandlerLinux or PortHandlerWindows
  dynamixel::PortHandler *portHandler = dynamixel::PortHandler::getPortHandler(DEVICENAME);

  // Initialize PacketHandler instance
  // Set the protocol version
  // Get methods and members of Protocol1PacketHandler or Protocol2PacketHandler
  dynamixel::PacketHandler *packetHandler = dynamixel::PacketHandler::getPacketHandler(PROTOCOL_VERSION);

  //Variables Initializing
  Point Point_1 = Point(1, 9); //Newest Frame
  Point Point_2 = Point(2, 8);
  Point Point_3 = Point(3, 7);
  Point Point_4 = Point(4, 6);
  Point Point_5 = Point(5, 5);
  Point Middle = Point(1,1);

  int dxl_comm_result = COMM_TX_FAIL;             // Communication result
  uint8_t dxl_error = 0;                          // Dynamixel error
  uint16_t dxl_present_position = 0;
  int dxl_goal_position = 0;

  // Open port
  if (portHandler->openPort())
  {
    printf("Succeeded to open the port!\n");
  }
  else
  {
    printf("Failed to open the port!\n");
    printf("Press any key to terminate...\n");
    return 0;
  }

  // Set port baudrate
  if (portHandler->setBaudRate(BAUDRATE))
  {
    printf("Succeeded to change the baudrate!\n");
  }
  else
  {
    printf("Failed to change the baudrate!\n");
    printf("Press any key to terminate...\n");
    return 0;
  }

  //Ask if Internal or External Webcam
  char Input_1;
  cout << "Would you like to use the Internal, the External Camera or choose a file? [i/e/f]" << endl;
  do {
    cin >> Input_1;
  } while (!cin.fail() && Input_1 != 'i' && Input_1 != 'e' && Input_1 != 'f');

  //Choose Tracker and Start It
  Ptr<Tracker> tracker;
  string ChosenTracker;
  char Input_2;
  cout << "\nWhich Tracking Algorithm would you like to load?\n1\tKCF\n2\tCSRT\n3\tTLD\n4\tMIL\n5\tMOSSE\n6\tBOOSTING\n7\tMEDIANFLOW" << endl;
  do {
    cin >> Input_2;
  } while (!cin.fail() && Input_2 != '1' && Input_2 != '2' && Input_2 != '3' && Input_2 != '4' && Input_2 != '5' && Input_2 != '6' && Input_2 != '7');

  if (Input_2 == '1') {
    tracker = TrackerKCF::create();
    ChosenTracker = "KCF";
  }
  if (Input_2 == '2') {
    tracker = TrackerCSRT::create();
    ChosenTracker = "CSRT";
  }
  if (Input_2 == '3') {
    tracker = TrackerTLD::create();
    ChosenTracker = "TLD";
  }
  if (Input_2 == '4') {
    tracker = TrackerMIL::create();
    ChosenTracker = "MIL";
  }
  if (Input_2 == '5') {
    tracker = TrackerMOSSE::create();
    ChosenTracker = "MOSSE";
  }
  if (Input_2 == '6') {
    tracker = TrackerBoosting::create();
    ChosenTracker = "Boosting";
  }
  if (Input_2 == '7') {
    tracker = TrackerMedianFlow::create();
    ChosenTracker = "MedianFlow";
  }

  //Create Capturing Device
  cout << "\nStarting Video Capturing device\t(...)" << endl;
  VideoCapture video;
  string Input_3;
  if (Input_1 == 'i') {
    video = VideoCapture(0);
  }
  if (Input_1 == 'e') {
    video = VideoCapture("/dev/video2");
  }
  if (Input_1 == 'f') {
    cout << "\nType in your filename with extensions" << endl;
    cin >> Input_3;
    video = VideoCapture(Input_3);
  }

  //Check if Capturing Device is up
  if (video.isOpened()) {
    cout << "Starting Video Capturing device\t(...)\tSuccess!" << endl;
  }
  else {
    cout << "Starting Video Capturing device\t(...)\tError!" << endl;
    return 1;
  }

  //Get video resolution
  Point FrameInfo = Point(video.get(CAP_PROP_FRAME_WIDTH), video.get(CAP_PROP_FRAME_HEIGHT));

  //For saving the frame
  Mat frame;

  namedWindow("Video feed", WINDOW_NORMAL);

  //set the callback function for any mouse event
  setMouseCallback("Video feed", CallBackFunc, NULL);

  //Choose what object to track
  Rect2d trackingBox = Rect2d(Point(100,100), Point(0,0));
  int ExitWhile = 0;
  while (ExitWhile != 121) {
    video.read(frame);
    trackingBox = Rect2d(LButtonDown, LButtonHold);
    rectangle(frame, trackingBox, Purple, 2, 8);
    imshow("Video feed", frame);
    ExitWhile = waitKey(25);
  }
  tracker->init(frame, trackingBox);

  String imageName( "Background.jpg" ); // by default
  Mat image;
  image = imread( samples::findFile( imageName ), IMREAD_COLOR ); // Read the file
  namedWindow( "Display window", WINDOW_NORMAL ); // Create a window for display.             // Show our image inside it.

  duration<double, std::milli> time_span;
  double FPS = 0;
  high_resolution_clock::time_point t1 = high_resolution_clock::now();
  high_resolution_clock::time_point t2 = high_resolution_clock::now();

  while (video.read(frame)) {
    //Timestamp 1
    t1 = high_resolution_clock::now();

    //Update Boundary Box
    if (tracker->update(frame, trackingBox)) {
      rectangle(frame, trackingBox, Purple, 2, 8);
    }

    //Calucalte Position of Bounding Box
    Middle = Point((trackingBox.x + trackingBox.width / 2),(trackingBox.y + trackingBox.height / 2));

    //Update Points
    Point_5 = Point_4;
    Point_4 = Point_3;
    Point_3 = Point_2;
    Point_2 = Point_1;
    Point_1 = Middle;

    //Smooth out rectangle
    Middle = SmoothFrame(frame, Point_1, Point_2, Point_3, Point_4, Point_5);

    //Calls SmoothFollow Function
    SmoothPrediction(frame, Middle, Point_1, Point_2, Point_3, Point_4, Point_5);

    //Handles whole GUI
    GUISetting(image, Middle, FrameInfo, time_span.count(), ChosenTracker);

    imshow("Video feed", frame);

    //For ending the video early
    if (waitKey(25) >= 0) {
      //Release video capture and writer
      video.release();
      //Destroy all windows
      destroyAllWindows();
      return 0;
    }

    //Timestamp 2 and FPS Calculation
    t2 = high_resolution_clock::now();
    time_span = t2 - t1;



    //Motors


    packetHandler->read2ByteTxRx(portHandler, DXL_ID,ADDR_MX_PRESENT_POSITION, &dxl_present_position, &dxl_error);

    printf("[ID:%03d] PresPos:%03d\n", DXL_ID, dxl_present_position);

    dxl_present_position = 0;

    // Write goal position
    dxl_comm_result = packetHandler->write2ByteTxOnly(portHandler, DXL_ID, ADDR_MX_GOAL_POSITION, dxl_goal_position);

    // Change goal position
    if (dxl_goal_position >= 1023) {
      dxl_goal_position = 0;
    } else {
      dxl_goal_position = dxl_goal_position + 10;
    }

  }


  // Close port
  portHandler->closePort();

  return 0;



}
