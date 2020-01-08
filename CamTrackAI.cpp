////////////////////////////////////////////////////////////////////////////////////////////////////
//Include header files
////////////////////////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <string>
#include <ctime>
#include <ratio>
#include <chrono>
#include <stdlib.h>
#include <stdio.h>
#include <cmath>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include <string>
#include <thread>

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Wizard.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/fl_ask.H>

////////////////////////////////////////////////////////////////////////////////////////////////////
//Include libraries
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "dynamixel_sdk.h"
#include <highgui.hpp>
#include <opencv.hpp>
#include <tracking/tracker.hpp>
#include <core/core.hpp>
#include <imgproc.hpp>
#include <imgcodecs.hpp>

////////////////////////////////////////////////////////////////////////////////////////////////////
//Defining namespaces
////////////////////////////////////////////////////////////////////////////////////////////////////
using namespace std;
using namespace cv;
using namespace std::chrono;

////////////////////////////////////////////////////////////////////////////////////////////////////
//Define colors for GUI
////////////////////////////////////////////////////////////////////////////////////////////////////
Scalar Green = Scalar(179, 239, 194); //Green
Scalar Blue = Scalar(177, 171, 151); //Blue
Scalar Purple = Scalar(114, 111, 116); //Purple
Scalar Brown = Scalar(61, 95, 115); //Brown

////////////////////////////////////////////////////////////////////////////////////////////////////
//Global Variables & Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////
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
string ChosenTracker = "";

int centerOffset_X;
int centerOffset_Y;
int centerOffsetStrength;

Fl_Window *firstWindow = 0;
Fl_Window *secondWindow = 0;
Fl_Window *infoWindow = 0;

#define SCREEN_X                        1920
#define SCREEN_Y                        1080

////////////////////////////////////////////////////////////////////////////////////////////////////
//Defining motor properties
////////////////////////////////////////////////////////////////////////////////////////////////////
#define CW_ANGLE_LIMIT                  6
#define CCW_ANGLE_LIMIT                 8
#define MOVING_SPEED                    32
#define TORQUE                          24
#define TORQUE_LIMIT                    34
#define PROTOCOL_VERSION                1.0
#define DXL_ID                          4
#define BAUDRATE                        57600
#define DEVICENAME                      "/dev/ttyUSB0"

////////////////////////////////////////////////////////////////////////////////////////////////////
//Function which triggers when Mouse Movements or Clicks are detected (Used for selecting the Region of interest [ROI])
////////////////////////////////////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////////////////////////////////////
//Function for Drawing a smoothed out Rectangle (So Camera wont wiggle)
////////////////////////////////////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////////////////////////////////////
//Function for Calcualting a smooth follow
////////////////////////////////////////////////////////////////////////////////////////////////////
void SmoothPrediction (Mat frame, Point Middle, Point Point_1, Point Point_2, Point Point_3, Point Point_4, Point Point_5) {
  //Calculate the weighted mean
  Prediction_X = (((Point_1.x - Point_2.x) * 0.4) + ((Point_2.x - Point_3.x) * 0.3) + ((Point_3.x - Point_4.x) * 0.2) + ((Point_4.x - Point_5.x) * 0.1)) / 4;
  Prediction_Y = (((Point_1.y - Point_2.y) * 0.4) + ((Point_2.y - Point_3.y) * 0.3) + ((Point_3.y - Point_4.y) * 0.2) + ((Point_4.y - Point_5.y) * 0.1)) / 4;

  //Create the Point where the prediction will be
  PredictionPoint = Point(Point_1.x + Prediction_X, Point_1.y + Prediction_Y);

  //Draw Arrowed Line (For SMooth Follow Visuals)
  arrowedLine(frame, Middle, PredictionPoint, Blue, 2, 2, 0, 0.1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//Function for handling the whole GUI
////////////////////////////////////////////////////////////////////////////////////////////////////
void GUISetting (Fl_Output*center_X, Fl_Output*center_Y, Fl_Output*SmoothingRation_X, Fl_Output*SmoothingRation_Y, Fl_Output*framesPerSecond, Fl_Output*msecondsPerSecond, Fl_Output*ScreenSize_X, Fl_Output*ScreenSize_Y, Fl_Output*trackerDisplay, Point Middle, Point FrameInfo, double PictureTime, string Tracker) {
  double FPS = 1000/PictureTime;

  center_X->value(to_string(Middle.x).c_str());
  center_Y->value(to_string(Middle.y).c_str());
  SmoothingRation_X->value(to_string(Prediction_X).c_str());
  SmoothingRation_Y->value(to_string(Prediction_Y).c_str());
  framesPerSecond->value(to_string(FPS).c_str());
  msecondsPerSecond->value(to_string(PictureTime).c_str());
  ScreenSize_X->value(to_string(FrameInfo.x).c_str());
  ScreenSize_Y->value(to_string(FrameInfo.y).c_str());
  trackerDisplay->value(Tracker.c_str());

}

////////////////////////////////////////////////////////////////////////////////////////////////////
//Main function 2
////////////////////////////////////////////////////////////////////////////////////////////////////
int TrackerMain(Fl_Output*trackerInfo, Fl_Output*videoInfo, Fl_Output*center_X, Fl_Output*center_Y, Fl_Output*SmoothingRation_X, Fl_Output*SmoothingRation_Y, Fl_Output*framesPerSecond, Fl_Output*msecondsPerSecond, Fl_Output*ScreenSize_X, Fl_Output*ScreenSize_Y, Fl_Output*trackerDisplay) {

  //Variables Initializing
  Point Point_1 = Point(1, 9); //Newest Frame
  Point Point_2 = Point(2, 8);
  Point Point_3 = Point(3, 7);
  Point Point_4 = Point(4, 6);
  Point Point_5 = Point(5, 5);
  Point Middle = Point(1,1);
  int returnValue;

  printf("\033c"); //clear console

  dynamixel::PortHandler *portHandler = dynamixel::PortHandler::getPortHandler(DEVICENAME); //Initialize PortHandler instance & Set the port path
  dynamixel::PacketHandler *packetHandler = dynamixel::PacketHandler::getPacketHandler(PROTOCOL_VERSION); //Initialize PacketHandler instance & Set the protocol version

  // Open port
  returnValue = portHandler->openPort();
  cout << "OPEN PORTXXX " << returnValue << endl;

  // Set port baudrate
  returnValue = portHandler->setBaudRate(BAUDRATE);
  cout << "BAUDRATE CHANGE " << returnValue << endl;

  //Enable torque
  returnValue = packetHandler->write1ByteTxOnly(portHandler, DXL_ID, TORQUE, 1);
  cout << "TORQUE " << returnValue << endl;

  //Set torque limit
  returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID, TORQUE_LIMIT, 1023);
  cout << "TORQUE_LIMIT " << returnValue << endl;

  //CW angle limit
  returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID, CW_ANGLE_LIMIT, 0);
  cout << "CW_ANGLE_LIMIT " << returnValue << endl;

  //CCW angle limit
  returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID, CCW_ANGLE_LIMIT, 0);
  cout << "CCW_ANGLE_LIMIT " << returnValue << endl;

  //Ask if Internal or External Webcam
  /*
  char Input_1;
  cout << "Would you like to use the Internal, the External Camera or choose a file? [i/e/f]" << endl;
  do {
    cin >> Input_1;
  } while (!cin.fail() && Input_1 != 'i' && Input_1 != 'e' && Input_1 != 'f');
  */

  //Choose Tracker and Start It
  Ptr<Tracker> tracker;
  if (ChosenTracker == "KCF") {
    tracker = TrackerKCF::create();
  } else if (ChosenTracker == "CSRT") {
    tracker = TrackerCSRT::create();
  } else if (ChosenTracker == "TLD") {
    tracker = TrackerTLD::create();
  } else if (ChosenTracker == "MIL") {
    tracker = TrackerMIL::create();
  } else if (ChosenTracker == "MOSSE") {
    tracker = TrackerMOSSE::create();
  } else if (ChosenTracker == "Boosting") {
    tracker = TrackerBoosting::create();
  } else if (ChosenTracker == "MedianFlow") {
    tracker = TrackerMedianFlow::create();
  } else {
    cout << "Error chosing tracker" << endl;
    exit(0);
  }

  //Create Capturing Device
  cout << "\nStarting Video Capturing device\t(...)" << endl;
  VideoCapture video;
  video = VideoCapture(0);
  /*
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
  */

  //Check if Capturing Device is up
  if (video.isOpened()) {
    cout << "Starting Video Capturing device\t(...)\tSuccess!" << endl;
  }
  else {
    cout << "Starting Video Capturing device\t(...)\tError!" << endl;
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

  /*
  String imageName( "/home/theb3arybear/Desktop/Background.jpg" ); // by default
  Mat image;
  image = imread( samples::findFile( imageName ), IMREAD_COLOR ); // Read the file
  namedWindow( "Display window", WINDOW_NORMAL ); // Create a window for display.             // Show our image inside it.
  */

  duration<double, std::milli> time_span;
  double FPS = 0;
  high_resolution_clock::time_point t1 = high_resolution_clock::now();
  high_resolution_clock::time_point t2 = high_resolution_clock::now();

  float trackerShiftX;

  while (video.read(frame)) {
    //Timestamp 1
    t1 = high_resolution_clock::now();

    //Update Boundary Box
    if (tracker->update(frame, trackingBox)) {
      rectangle(frame, trackingBox, Purple, 2, 8);
    }

    //Calucalte Position of Bounding Boxpthread_exit(NULL);
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
    GUISetting(center_X, center_Y, SmoothingRation_X, SmoothingRation_Y, framesPerSecond, msecondsPerSecond, ScreenSize_X, ScreenSize_Y, trackerDisplay, Middle, FrameInfo, time_span.count(), ChosenTracker);

    Fl::check();

    imshow("Video feed", frame);

  /*
    //Calculate prediction
    if (Prediction_X < 0) {

      trackerShiftX = abs(Prediction_X) * 100;

      if (trackerShiftX >= 1023) {
        trackerShiftX = 1023;
      }
    } else {
      trackerShiftX = abs(Prediction_X) * 100;

      if (trackerShiftX >= 1023) {

        trackerShiftX = 2047;

      } else {

        trackerShiftX = trackerShiftX + 1024;

      }
    }
    */

    //MoveX


    trackerShiftX = abs(returnPoint.x - (FrameInfo.x/2));


    //Set Speed
    returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID, MOVING_SPEED, trackerShiftX);

    //For ending the video early
    if (waitKey(25) >= 0) {
      //Release video capture and writer
      video.release();
      //Destroy all windows
      destroyAllWindows();
    }

    //Timestamp 2 and FPS CalculationPrediction_X
    t2 = high_resolution_clock::now();
    time_span = t2 - t1;

  }

  uint8_t receivedPackage;
  uint16_t receivedError;
  do {
    packetHandler->read2ByteRx(portHandler, receivedPackage, &receivedError);
    //Set Speed
    returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID, MOVING_SPEED, 0);
  } while(receivedError != 0);

  // Close port
  portHandler->closePort();

  return 0;

}



////////////////////////////////////////////////////////////////////////////////////////////////////
//Buttons for GUI
////////////////////////////////////////////////////////////////////////////////////////////////////
void b(Fl_Widget *, void * v) {
  fl_beep();

  Fl_Output* slider = (Fl_Output*) v;
  slider->value("LEL");
}

void b_Exit(Fl_Widget *, void *) {
  fl_beep();
  exit(0);
}

void b_Next(Fl_Widget *, void *) {
  fl_beep();
  firstWindow->hide();
}

void b_KCF(Fl_Widget *, void *) {
  fl_beep();
  ChosenTracker = "KCF";
  firstWindow->hide();
}

void b_CSRT(Fl_Widget *, void *) {
  fl_beep();
  ChosenTracker = "CSRT";
  firstWindow->hide();
}

void b_TLD(Fl_Widget *, void *) {
  fl_beep();
  ChosenTracker = "TLD";
  firstWindow->hide();
}

void b_MIL(Fl_Widget *, void *) {
  fl_beep();
  ChosenTracker = "MIL";
  firstWindow->hide();
}

void b_MOSSE(Fl_Widget *, void *) {
  fl_beep();
  ChosenTracker = "MOSSE";
  firstWindow->hide();
}

void b_BOOSTING(Fl_Widget *, void *) {
  fl_beep();
  ChosenTracker = "Boosting";
  firstWindow->hide();
}

void b_MEDIANFLOW(Fl_Widget *, void *) {
  fl_beep();
  ChosenTracker = "MedianFlow";
  firstWindow->hide();
}

void b_UP(Fl_Widget *, void *) {
  fl_beep();
}

void b_DOWN(Fl_Widget *, void *) {
  fl_beep();
}

void b_LEFT(Fl_Widget *, void *) {
  fl_beep();
}

void b_RIGHT(Fl_Widget *, void *) {
  fl_beep();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//First Page. Here the user will choose which Tracker to use.
////////////////////////////////////////////////////////////////////////////////////////////////////
int FirstWindow(int argc, char **argv) {

  firstWindow = new Fl_Window(0,0,SCREEN_X/2,SCREEN_Y);
  firstWindow->resizable(firstWindow);

  Fl_Output *out1 = new Fl_Output(0, 1*(SCREEN_Y/10)-10, SCREEN_X/2, 1, "Chose a tracker");
  out1->labelsize(25);
  out1->align(FL_ALIGN_TOP|FL_ALIGN_LEFT);

  Fl_Button *kcf = new Fl_Button(0, 1*(SCREEN_Y/10), SCREEN_X/15, SCREEN_Y/15, "KCF");
  //b1->callback(b_CSRT,out1);
  kcf->callback(b_KCF,0);

  Fl_Button *csrt = new Fl_Button(1*(SCREEN_X/8), 1*(SCREEN_Y/10), SCREEN_X/15, SCREEN_Y/15, "CSRT");
  csrt->callback(b_CSRT,0);

  Fl_Button *tld = new Fl_Button(2*(SCREEN_X/8), 1*(SCREEN_Y/10), SCREEN_X/15, SCREEN_Y/15, "TLD");
  tld->callback(b_TLD,0);

  Fl_Button *mil = new Fl_Button(3*(SCREEN_X/8), 1*(SCREEN_Y/10), SCREEN_X/15, SCREEN_Y/15, "MIL");
  mil->callback(b_MIL,0);

  Fl_Button *boosting = new Fl_Button(0, 2*(SCREEN_Y/10), SCREEN_X/15, SCREEN_Y/15, "Boosting");
  boosting->callback(b_BOOSTING,0);

  Fl_Button *mosse = new Fl_Button(1*(SCREEN_X/8), 2*(SCREEN_Y/10), SCREEN_X/15, SCREEN_Y/15, "MOSSE");
  mosse->callback(b_MOSSE,0);

  Fl_Button *medianflow = new Fl_Button(2*(SCREEN_X/8), 2*(SCREEN_Y/10), SCREEN_X/15, SCREEN_Y/15, "Medianflow");
  medianflow->callback(b_MEDIANFLOW,0);

  Fl_Button *b3 = new Fl_Button(0,SCREEN_Y-SCREEN_Y/15, SCREEN_X/15, SCREEN_Y/15, "Quit");
  b3->callback(b_Exit,0);

  Fl_Button *next = new Fl_Button(SCREEN_X/2-SCREEN_X/15,SCREEN_Y-SCREEN_Y/15, SCREEN_X/15, SCREEN_Y/15, "Next");
  //next->label("@+92->");
  next->callback(b_Next,0);

  firstWindow->end();
  firstWindow->show(argc,argv);

  firstWindow->redraw();

  return Fl::run();

}

////////////////////////////////////////////////////////////////////////////////////////////////////
//Second Page. This is the main Tracker HUD/GUI
////////////////////////////////////////////////////////////////////////////////////////////////////
int SecondWindow(int argc, char **argv) {

  //Create the window
  secondWindow = new Fl_Window(0,0,SCREEN_X/2,SCREEN_Y);
  secondWindow->resizable(secondWindow);

  //Titel "Tracker Information"
  Fl_Output *trackerInfo = new Fl_Output(0, 1*(SCREEN_Y/10)-10, SCREEN_X/2, 1, "Tracker Information");
  trackerInfo->labelsize(25);
  trackerInfo->align(FL_ALIGN_TOP|FL_ALIGN_LEFT);

  //Titel "Video Information"
  Fl_Output *videoInfo = new Fl_Output(0, 3*(SCREEN_Y/10)-10, SCREEN_X/2, 1, "Video Information");
  videoInfo->labelsize(25);
  videoInfo->align(FL_ALIGN_TOP|FL_ALIGN_LEFT);

  //Titel "Move Center"
  Fl_Output *moveCenter = new Fl_Output(0, 6*(SCREEN_Y/10)-10, SCREEN_X/2, 1, "Move Center");
  moveCenter->labelsize(25);
  moveCenter->align(FL_ALIGN_TOP|FL_ALIGN_LEFT);


  Fl_Output *center_X = new Fl_Output(10, 1*(SCREEN_Y/10)+10, 100, 20, "Center (x) ");
  center_X->labelsize(15);
  center_X->align(FL_ALIGN_RIGHT);

  Fl_Output *center_Y = new Fl_Output(10, 1*(SCREEN_Y/10)+40, 100, 20, "Center (y) ");
  center_Y->labelsize(15);
  center_Y->align(FL_ALIGN_RIGHT);

  Fl_Output *SmoothingRation_X = new Fl_Output(10, 1*(SCREEN_Y/10)+70, 100, 20, "Smoothing Ration (x) ");
  SmoothingRation_X->labelsize(15);
  SmoothingRation_X->align(FL_ALIGN_RIGHT);

  Fl_Output *SmoothingRation_Y = new Fl_Output(10, 1*(SCREEN_Y/10)+100, 100, 20, "Smoothing Ration (y) ");
  SmoothingRation_Y->labelsize(15);
  SmoothingRation_Y->align(FL_ALIGN_RIGHT);

  Fl_Output *framesPerSecond = new Fl_Output(10, 3*(SCREEN_Y/10)+10, 100, 20, "FPS ");
  framesPerSecond->labelsize(15);
  framesPerSecond->align(FL_ALIGN_RIGHT);

  Fl_Output *msecondsPerSecond = new Fl_Output(10, 3*(SCREEN_Y/10)+40, 100, 20, "Milliseconds per frame ");
  msecondsPerSecond->labelsize(15);
  msecondsPerSecond->align(FL_ALIGN_RIGHT);

  Fl_Output *ScreenSize_X = new Fl_Output(10, 3*(SCREEN_Y/10)+100, 100, 20, "px Screensize (x) ");
  ScreenSize_X->labelsize(15);
  ScreenSize_X->align(FL_ALIGN_RIGHT);

  Fl_Output *ScreenSize_Y = new Fl_Output(10, 3*(SCREEN_Y/10)+130, 100, 20, "px Screensize (y) ");
  ScreenSize_Y->labelsize(15);
  ScreenSize_Y->align(FL_ALIGN_RIGHT);

  Fl_Output *trackerDisplay = new Fl_Output(10, 3*(SCREEN_Y/10)+160, 100, 20, "Chosen Tracker ");
  trackerDisplay->labelsize(15);
  trackerDisplay->align(FL_ALIGN_RIGHT);

  Fl_Button *b3 = new Fl_Button(0,SCREEN_Y-SCREEN_Y/15, SCREEN_X/15, SCREEN_Y/15, "Quit");
  b3->callback(b_Exit,0);

  Fl_Button *next = new Fl_Button(SCREEN_X/2-SCREEN_X/15,SCREEN_Y-SCREEN_Y/15, SCREEN_X/15, SCREEN_Y/15, "Next");
  next->callback(b_Next,0);

  secondWindow->end();
  secondWindow->show(argc,argv);

  thread threadObj(TrackerMain, trackerInfo, videoInfo, center_X, center_Y, SmoothingRation_X, SmoothingRation_Y, framesPerSecond, msecondsPerSecond, ScreenSize_X, ScreenSize_Y, trackerDisplay);

  return Fl::run();

}

int InfoWindow(int argc, char **argv) {

  infoWindow = new Fl_Window(0,0,1000,100);
  infoWindow->resizable(infoWindow);

  Fl_Output out2(20, 40, 80, 25, "");
  Fl_Output *out1 = &out2;


  Fl_Button *b3 = new Fl_Button(220,20, 80, 25, "Return");
  b3->callback(b_Exit,0);

  infoWindow->end();
  infoWindow->show(argc,argv);

  return Fl::run();

}

////////////////////////////////////////////////////////////////////////////////////////////////////
//Main function
////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv) {

  centerOffset_X = 0;
  centerOffset_Y = 0;
  centerOffsetStrength = 0;

  FirstWindow(argc,argv);

  SecondWindow(argc,argv);

}
