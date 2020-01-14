////////////////////////////////////////////////////////////////////////////////////////////////////
// **CAMTRACKAI**
//
// Coded by BE3dARt with <3
//
// Visit https://github.com/BE3dARt/CamTrackAI for the documentation/ installing instructions and legal notice
////////////////////////////////////////////////////////////////////////////////////////////////////

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

#include <opencv2/opencv.hpp>
#include <opencv2/tracking/tracker.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//Defining namespaces
////////////////////////////////////////////////////////////////////////////////////////////////////
using namespace std;
using namespace cv;
using namespace std::chrono;

////////////////////////////////////////////////////////////////////////////////////////////////////
//Define colors for GUI
////////////////////////////////////////////////////////////////////////////////////////////////////
Scalar colorGreen = Scalar(179, 239, 194); //Green
Scalar colorBlue = Scalar(177, 171, 151); //Blue
Scalar colorPurple = Scalar(114, 111, 116); //Purple

////////////////////////////////////////////////////////////////////////////////////////////////////
//Global Variables & Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////
//Defines which tracker to use
string ChosenTracker = "";

//FLTK windows
Fl_Window *firstWindow = 0;
Fl_Window *secondWindow = 0;

//Shutdown and reboot
bool shutdownActivated = false;
bool rebootActivated = false;

//Key and mouse events
bool MouseClicked = false;

Point LButtonDown = Point(0, 0);
Point LButtonHold = Point(0, 0);

////////////////////////////////////////////////////////////////////////////////////////////////////
//Defining Properties
////////////////////////////////////////////////////////////////////////////////////////////////////
#define SCREEN_X                        1920
#define SCREEN_Y                        1080

#define PORT                            4097
#define SHUTDOWN						3001
#define REBOOT							3002

#define KEY_START           113
#define KEY_END             119
#define KEY_REBOOT          114
#define KEY_SHUTDOWN        102

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function which triggers when Mouse Movements or Clicks are detected (Used for selecting the Region of interest [ROI])
// @event	Defines for which event the function looks out for.
// @x		x-coordinate of a point
// @y		y-coordinate of a point
// @flags
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
// Function to calculate a smoothed rectangle (Camera wont start to wiggle)
// @frame		Image (Window) on which to draw the recatngle
// @Point_1		Newest tracker center point
// @Point_2		-
// @Point_3		-
// @Point_4		-
// @Point_5		Oldest tracker center point
////////////////////////////////////////////////////////////////////////////////////////////////////
Point SmoothFrame (Mat frame, Point Point_1, Point Point_2, Point Point_3, Point Point_4, Point Point_5) {
  double tempX;
  double tempY;
  Point returnValue;

  //Calculate the weighted mean
  tempX = ((Point_1.x * 0.4) + (Point_2.x * 0.3) + (Point_3.x * 0.2) + (Point_4.x * 0.05) + (Point_5.x * 0.05));
  tempY = ((Point_1.y * 0.4) + (Point_2.y * 0.3) + (Point_3.y * 0.2) + (Point_4.y * 0.05) + (Point_5.y * 0.05));

  //Make Point to return
  returnValue = Point((tempX), (tempY));

  //Draw Rectangle (For Smooth Follow Visuals)
  rectangle(frame, Point((tempX - 50), (tempY - 50)), Point((tempX + 50), (tempY + 50)), colorGreen, 2, 8, 0);

  return returnValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function for handling the whole GUI
// @center_X				Pointer to GUI element which displays the tracker's center x value
// @center_Y				Pointer to GUI element which displays the tracker's center y value
// @framesPerSecond			Pointer to GUI element which displays the fps
// @msecondsPerSecond		Pointer to GUI element which displays the milliseconds used for calculating one frame
// @ScreenSize_X			Pointer to GUI element which displays the image's x size
// @ScreenSize_Y			Pointer to GUI element which displays the image's y size
// @trackerDisplay			Pointer to GUI element which displays which tracker is used
// @Middle					Point (x,y) of the tracker's center
// @FrameInfo				Point (x,y) of the size of the image
// @PictureTime				Time consumed for one while loop, aka calulating one frame
// @Tracker					Which tracker is used currently
// @smoothedCenter			Point (x,y) of the tracker's smoothed out center
// @errorDisplay			Pointer to GUI element which displays if there is an error with the tracker
// @trackerError			True if there is an error with the tracker
////////////////////////////////////////////////////////////////////////////////////////////////////
void GUISetting (Fl_Output*center_X, Fl_Output*center_Y, Fl_Output*framesPerSecond, Fl_Output*msecondsPerSecond, Fl_Output*ScreenSize_X, Fl_Output*ScreenSize_Y, Point Middle, Point FrameInfo, double PictureTime, Point smoothedCenter, Fl_Output *errorDisplay, bool trackerError) {
  int FPS = 1000/PictureTime;  //Frames per second for calculation of one frame

  //What to display if there is an error with the tracker
  string errorString;
  if(trackerError) {
    errorString = "SUCCESS";
  } else {
    errorString = "ERROR";
  }

  //Write values to GUI elements
  center_X->value(to_string(smoothedCenter.x).c_str());
  center_Y->value(to_string(smoothedCenter.y).c_str());
  framesPerSecond->value(to_string(FPS).c_str());
  msecondsPerSecond->value(to_string(PictureTime).c_str());
  ScreenSize_X->value(to_string(FrameInfo.x).c_str());
  ScreenSize_Y->value(to_string(FrameInfo.y).c_str());
  errorDisplay->value(errorString.c_str());
}

void Shutdown(int socket, int positionArray[2]) {
    if (rebootActivated) {
      positionArray[0] = REBOOT;
      cout << "\n\n=====================================================================================\nRebooting Raspberry Pi ...\n=====================================================================================\n\n" << endl;
    } else {
      positionArray[0] = SHUTDOWN; //Set movement instruction to 'REBOOT'. So the Raspberry Pi will receive it and reboot.
      cout << "\n\n=====================================================================================\nShutting down Raspberry Pi ...\n=====================================================================================\n\n" << endl;
    }
    positionArray[1] = 0; //Set the motor to 0 movement

    send(socket, positionArray , 8, 0); //Send movement instruction to Raspberry Pi

    close(socket); //Close socket

    exit(0); //Exit application
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Main function, where all the magic happens
// @center_X				Pointer to GUI element which displays the tracker's center x value
// @center_Y				Pointer to GUI element which displays the tracker's center y value
// @framesPerSecond			Pointer to GUI element which displays the fps
// @msecondsPerSecond		Pointer to GUI element which displays the milliseconds used for calculating one frame
// @ScreenSize_X			Pointer to GUI element which displays the image's x size
// @ScreenSize_Y			Pointer to GUI element which displays the image's y size
// @trackerDisplay			Pointer to GUI element which displays which tracker is used
// @serverIP				Pointer to IP Address
// @errorDisplay			Pointer to GUI element which displays if there is an error with the tracker
////////////////////////////////////////////////////////////////////////////////////////////////////
int TrackerMain(Fl_Output*center_X, Fl_Output*center_Y, Fl_Output*framesPerSecond, Fl_Output*msecondsPerSecond, Fl_Output*ScreenSize_X, Fl_Output*ScreenSize_Y, Fl_Output*errorDisplay, const char* serverIP) {

  Point smoothedCenter; //Point (x,y) to store the smoothed out center of tracker
  int moveMotorX; //How fast to move motors in x direction
  int moveMotorY; //How fast to move motors in y direction
  int stepX; //How big the steps are for motor movement in x direction (that it is 0 movement in the center and 100% movement at border of screen)
  int stepY; //How big the steps are for motor movement in y direction (that it is 0 movement in the center and 100% movement at border of screen)
  int differnceToCenterX; //Difference from tracker's x center to screen's x center
  int differnceToCenterY; //Difference from tracker's y center to screen's y center
  int positionArray[2] = {0, 0}; //Holds motors movement x and y value (Used for sending over TCP)
  int socket_fd; //Socket
  int serverPort = PORT; //Port over which connection goes

  bool runningMain = true; //For exiting the main while loop
  bool running = true; //For exiting the "tracker update" while loop

  duration<double, milli> time_span; //Timeintervall to calculate 'msecondsPerSecond' and 'fps'
  high_resolution_clock::time_point beginTime = high_resolution_clock::now(); //Start of counter
  high_resolution_clock::time_point endTime = high_resolution_clock::now(); //Start of counter

  Point Point_1 = Point(1, 9); //Oldest frame
  Point Point_2 = Point(2, 8); //-
  Point Point_3 = Point(3, 7); //-
  Point Point_4 = Point(4, 6); //-
  Point Point_5 = Point(5, 5); //-
  Point Middle = Point(1,1); //Newest frame
  Point FrameInfo = Point(480, 300); //Point (x,y) which holds image resolution

  struct sockaddr_in serverAddr; //Struct that holds IP Adress/Port/Family
  socklen_t addrLen = sizeof(struct sockaddr_in); //Calculate size which is reserved for 'serverAddr'

  Mat img; //holds image (frame)
  img = Mat::zeros(300 , 480, CV_8UC3); //Fill in image (frame) with 0
  int imgSize = img.total() * img.elemSize(); //Image (frame) size
  uchar *iptr = img.data; //Pointer to image (frame) for it to be used with TCP/IP

  namedWindow("CamTrackAI", WINDOW_NORMAL); //Create an OpenCV window

  setMouseCallback("CamTrackAI", CallBackFunc, NULL); //Set a callback function for mouse events

  Rect2d trackingBox = Rect2d(Point(100,100), Point(0,0)); //Box which is drawn by the user for selecting the ROI (Region of interest)
  int ExitWhile = 0; //Exit a while loop

  bool trackerError; //Returning value by tracker->update()

  //Initiate Socket
  if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    cout << "socket()\tERROR!" << endl;
  }

  //Fill in struct 'serverAddr'
  serverAddr.sin_family = PF_INET; //Family
  serverAddr.sin_addr.s_addr = inet_addr(serverIP); //IP Address
  serverAddr.sin_port = htons(serverPort); //Port

  //Connect to socket
  if (connect(socket_fd, (sockaddr*)&serverAddr, addrLen) < 0) {
    cout << "connect()\tERROR!" << endl;
  }

  while (runningMain) {
    //Fill with 0 so that motors won't move
    positionArray[0] = 0;
    positionArray[1] = 0;

    //Evaluate decision by user and define type of tracker to be used. If there was an error, Reboot instructions will be sent to the Raspberry Pi and the application will shut down.
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
      rebootActivated = true;
      Shutdown(socket_fd, positionArray);
    }

    //Choose what object to track (ROI). Exits if 'y' (Key Number 121) or the button 'b_COMMIT' has been pressed.
    while (ExitWhile != KEY_START) {
      recv(socket_fd, iptr, imgSize , MSG_WAITALL); //Receive image from Raspberyy Pi and store it into 'img' (iptr)

      trackingBox = Rect2d(LButtonDown, LButtonHold); //Update rectangle which is drawn with the user's mouse
      rectangle(img, trackingBox, colorPurple, 2, 8); //Make the drawn box visible

      imshow("CamTrackAI", img); //Show image with 'img' and the rectangle on a higher layer

      ExitWhile = waitKey(20); //Wait 25ms for 'e' key to be pressed. 'ExitWhile' will change to 121 if key was pressed and initiates exit of the loop.

      if (ExitWhile == KEY_REBOOT) {
        rebootActivated = true;
      }

      if (ExitWhile == KEY_SHUTDOWN) {
        shutdownActivated = true;
      }

      if (shutdownActivated == true || rebootActivated == true) {
        ExitWhile = KEY_START;
        Shutdown(socket_fd, positionArray);
      } else {
        send(socket_fd, positionArray , 8, 0); //Send movement instructions to motors
      }
    }

    ExitWhile = 0; //Reset for it to be used in another loop

    tracker->init(img, trackingBox); //Initalize tracker with the user's defined box

    running = true; //Reset for it to be used in another loop

    while (running) {

      beginTime = high_resolution_clock::now(); //Begin timer

      //Receive image from Raspberyy Pi and store it into 'img' (iptr). If there was an error, send Reboot instruction to Raspberry Pi and close the application
      if (recv(socket_fd, iptr, imgSize , MSG_WAITALL) == -1) {
        cout << "recv()\tERROR!" << endl;
        rebootActivated = true;
        Shutdown(socket_fd, positionArray);
      }

      //The tracker searches for the object in the current frame. If it fails, it will send 0 (zero movement for motors) to the Raspberry Pi until the object is found again.
      if ((trackerError = tracker->update(img, trackingBox)) == true) {
        rectangle(img, trackingBox, colorPurple, 2, 8); //Make trackerdata visible through a rectangle

        Middle = Point((trackingBox.x + trackingBox.width / 2),(trackingBox.y + trackingBox.height / 2));   //Calucalte center of tracker

        //Update Points (from oldest frame to newest frame)
        Point_5 = Point_4;
        Point_4 = Point_3;
        Point_3 = Point_2;
        Point_2 = Point_1;
        Point_1 = Middle;

        smoothedCenter = SmoothFrame(img, Point_1, Point_2, Point_3, Point_4, Point_5);

        Fl::check(); //Refresh GUI

        imshow("CamTrackAI", img); //Show image (frame) with the rectangle (tracker data) on a higher layer

        //Calculate the steps based on the image's resolution and maximal movement by the Dynamixel motors.
        stepX = 1023 / (FrameInfo.x/2);
        stepY = 1023 / (FrameInfo.y/2);

        //Calucalte difference from tracker's x center to screen's x center
        differnceToCenterX = smoothedCenter.x - (FrameInfo.x/2);
        differnceToCenterY = smoothedCenter.y - (FrameInfo.y/2);

        //Checks if the difference from the center is + or minus 0. If it is < 0, it will send a number ranging from 0-1023 to the Raspberry Pi. If false, it will send numbers ranging from 1024-2047. This is used to define the direction.
        if(differnceToCenterX < 0) {
          moveMotorX = abs(differnceToCenterX) * stepX;
        } else {
          moveMotorX = (abs(differnceToCenterX) * stepX) + 1024;
        }

        if(differnceToCenterY < 0) {
          moveMotorY = abs(differnceToCenterY) * stepY;
        } else {
          moveMotorY = (abs(differnceToCenterY) * stepY) + 1024;
        }

        //Set movement instruction for the motors
        positionArray[0] = moveMotorX;
        positionArray[1] = moveMotorY;
      } else {
        positionArray[0] = 0;
        positionArray[1] = 0;
      }

      // Exit the loop if the 'esc' key was pressed.
      int k = waitKey(1);
      if(k == KEY_END)
      {
        running = false;
      }

      if (k == KEY_REBOOT) {
        rebootActivated = true;
      }

      if (k == KEY_SHUTDOWN) {
        shutdownActivated = true;
      }

      //Write values into the GUI
      GUISetting(center_X, center_Y, framesPerSecond, msecondsPerSecond, ScreenSize_X, ScreenSize_Y, Middle, FrameInfo, time_span.count(), smoothedCenter, errorDisplay, trackerError);
      // Check if user requested a reboot or shutdown of the Raspberry Pi. Else just carry on with the loop.
      if (shutdownActivated == true || rebootActivated == true) {
        Shutdown(socket_fd, positionArray);
      } else {
        send(socket_fd, positionArray , 8, 0);
      }

      //Timestamp 2 and time span calulating
      endTime = high_resolution_clock::now();
      time_span = endTime - beginTime;
    }
  }
  close(socket_fd); //Close socket

  exit(0); //Exit application

  return 0;

}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Buttons for the GUI
// @FL_Widget	Target widget
// @*			Data that can be stored in a Widget with fltk::Widget::user_data(void*)
// -> 			Description taken from https://www.fltk.org/doc-2.0/html/group__example2.html
////////////////////////////////////////////////////////////////////////////////////////////////////
//Choose KCF
void b_KCF(Fl_Widget *, void *) {
  fl_beep();
  ChosenTracker = "KCF";
  firstWindow->hide();
}

//Choose CSRT
void b_CSRT(Fl_Widget *, void *) {
  fl_beep();
  ChosenTracker = "CSRT";
  firstWindow->hide();
}

//Choose TLD
void b_TLD(Fl_Widget *, void *) {
  fl_beep();
  ChosenTracker = "TLD";
  firstWindow->hide();
}

//Choose MIL
void b_MIL(Fl_Widget *, void *) {
  fl_beep();
  ChosenTracker = "MIL";
  firstWindow->hide();
}

//Choose MOSSE
void b_MOSSE(Fl_Widget *, void *) {
  fl_beep();
  ChosenTracker = "MOSSE";
  firstWindow->hide();
}

//Choose BOOSTING
void b_BOOSTING(Fl_Widget *, void *) {
  fl_beep();
  ChosenTracker = "Boosting";
  firstWindow->hide();
}

//Choose MedianFlow
void b_MEDIANFLOW(Fl_Widget *, void *) {
  fl_beep();
  ChosenTracker = "MedianFlow";
  firstWindow->hide();
}

//Activate shutdown
void b_SHUTDOWN(Fl_Widget *, void *) {
  fl_beep();
  firstWindow->hide();
  shutdownActivated = true;
}

//Activate reboot
void b_REBOOT(Fl_Widget *, void *) {
  fl_beep();
  rebootActivated = true;
  firstWindow->hide();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//First GUI page. The user will choose a tracker
////////////////////////////////////////////////////////////////////////////////////////////////////
int FirstWindow(int argc, char **argv) {

  firstWindow = new Fl_Window(0,0,SCREEN_X/2,SCREEN_Y);
  firstWindow->resizable(firstWindow);

  Fl_Output *out1 = new Fl_Output(0, 1*(SCREEN_Y/10)-10, SCREEN_X/2, 1, "Chose a tracker");
  out1->labelsize(25);
  out1->align(FL_ALIGN_TOP|FL_ALIGN_LEFT);

  Fl_Button *kcf = new Fl_Button(0, 1*(SCREEN_Y/10), SCREEN_X/15, SCREEN_Y/15, "KCF");
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

  Fl_Button *shutdown = new Fl_Button(0,SCREEN_Y-SCREEN_Y/15, SCREEN_X/15, SCREEN_Y/15, "Shut Down");
  shutdown->callback(b_SHUTDOWN,0);

  Fl_Button *reboot = new Fl_Button(SCREEN_X/15 + 20,SCREEN_Y-SCREEN_Y/15, SCREEN_X/15, SCREEN_Y/15, "Reboot");
  reboot->callback(b_REBOOT,0);

  firstWindow->end();
  firstWindow->show(argc,argv);

  firstWindow->redraw();

  return Fl::run();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//Second Page. This is the main Tracker HUD/GUI
////////////////////////////////////////////////////////////////////////////////////////////////////
int SecondWindow(int argc, char **argv, const char* serverIP) {

  //Create the window
  secondWindow = new Fl_Window(0,0,SCREEN_X/2,SCREEN_Y);
  secondWindow->resizable(secondWindow);

  //Group with Titel "Tracker Information"
  Fl_Output *trackerInfo = new Fl_Output(0, 1*(SCREEN_Y/10)-10, SCREEN_X/2, 2, "Tracker Information");
  trackerInfo->labelsize(25);
  trackerInfo->align(FL_ALIGN_TOP|FL_ALIGN_LEFT);

  Fl_Output *center_X = new Fl_Output(10, 1*(SCREEN_Y/10)+10, 150, 20, "Center (x) ");
  center_X->labelsize(15);
  center_X->align(FL_ALIGN_RIGHT);

  Fl_Output *center_Y = new Fl_Output(10, 1*(SCREEN_Y/10)+40, 150, 20, "Center (y) ");
  center_Y->labelsize(15);
  center_Y->align(FL_ALIGN_RIGHT);

  Fl_Output *framesPerSecond = new Fl_Output(10, 1*(SCREEN_Y/10)+70, 150, 20, "FPS ");
  framesPerSecond->labelsize(15);
  framesPerSecond->align(FL_ALIGN_RIGHT);

  Fl_Output *msecondsPerSecond = new Fl_Output(10, 1*(SCREEN_Y/10)+100, 150, 20, "Milliseconds per frame ");
  msecondsPerSecond->labelsize(15);
  msecondsPerSecond->align(FL_ALIGN_RIGHT);

  Fl_Output *trackerDisplay = new Fl_Output(10, 1*(SCREEN_Y/10)+130, 150, 20, "Chosen Tracker ");
  trackerDisplay->labelsize(15);
  trackerDisplay->align(FL_ALIGN_RIGHT);
  trackerDisplay->value(ChosenTracker.c_str());

  Fl_Output *errorDisplay = new Fl_Output(10, 1*(SCREEN_Y/10)+160, 150, 20, "Tracker Status ");
  errorDisplay->labelsize(15);
  errorDisplay->align(FL_ALIGN_RIGHT);


  //Group with Titel "Video Information"
  Fl_Output *videoInfo = new Fl_Output(0, 4*(SCREEN_Y/10)-10, SCREEN_X/2, 2, "Video Information");
  videoInfo->labelsize(25);
  videoInfo->align(FL_ALIGN_TOP|FL_ALIGN_LEFT);

  Fl_Output *ScreenSize_X = new Fl_Output(10, 4*(SCREEN_Y/10)+10, 150, 20, "px Screensize (x) ");
  ScreenSize_X->labelsize(15);
  ScreenSize_X->align(FL_ALIGN_RIGHT);

  Fl_Output *ScreenSize_Y = new Fl_Output(10, 4*(SCREEN_Y/10)+40, 150, 20, "px Screensize (y) ");
  ScreenSize_Y->labelsize(15);
  ScreenSize_Y->align(FL_ALIGN_RIGHT);

  //Group with Titel "More Information"
  Fl_Output *moreInfo = new Fl_Output(0, 6*(SCREEN_Y/10)-10, SCREEN_X/2, 2, "More Information");
  moreInfo->labelsize(25);
  moreInfo->align(FL_ALIGN_TOP|FL_ALIGN_LEFT);

  Fl_Output *ipDisplay = new Fl_Output(10, 6*(SCREEN_Y/10)+10, 150, 20, "Connected (IP Address) ");
  ipDisplay->labelsize(15);
  ipDisplay->align(FL_ALIGN_RIGHT);
  ipDisplay->value(serverIP);

  //Group with Titel "Controls"
  Fl_Output *controls = new Fl_Output(0, 7*(SCREEN_Y/10)-10, SCREEN_X/2, 2, "Controls");
  controls->labelsize(25);
  controls->align(FL_ALIGN_TOP|FL_ALIGN_LEFT);

  Fl_Output *message1 = new Fl_Output(10, 7*(SCREEN_Y/10)+10, 150, 20, "Start New Tracker (If there is a running tracker, you have to end it first)");
  message1->labelsize(15);
  message1->align(FL_ALIGN_RIGHT);
  char charMessage1 = KEY_START;
  const char *ptrCharMessage1 = &charMessage1;
  message1->value(ptrCharMessage1);

  Fl_Output *message2 = new Fl_Output(10, 7*(SCREEN_Y/10)+40, 150, 20, "End Current Tracker");
  message2->labelsize(15);
  message2->align(FL_ALIGN_RIGHT);
  char charMessage2 = KEY_END;
  const char *ptrCharMessage2 = &charMessage2;
  message2->value(ptrCharMessage2);

  Fl_Output *message3 = new Fl_Output(10, 7*(SCREEN_Y/10)+70, 150, 20, "Reboot Raspberry Pi/ Shut down application");
  message3->labelsize(15);
  message3->align(FL_ALIGN_RIGHT);
  char charMessage3 = KEY_REBOOT;
  const char *ptrCharMessage3 = &charMessage3;
  message3->value(ptrCharMessage3);

  Fl_Output *message4 = new Fl_Output(10, 7*(SCREEN_Y/10)+100, 150, 20, "Shut down Raspberry Pi / Shut down application");
  message4->labelsize(15);
  message4->align(FL_ALIGN_RIGHT);
  char charMessage4 = KEY_SHUTDOWN;
  const char *ptrCharMessage4 = &charMessage4;
  message4->value(ptrCharMessage4);

  Fl_Button *shutdown = new Fl_Button(0,SCREEN_Y-SCREEN_Y/15, SCREEN_X/15, SCREEN_Y/15, "Shut down");
  shutdown->callback(b_SHUTDOWN,0);

  Fl_Button *reboot = new Fl_Button(1*SCREEN_X/15 + 10,SCREEN_Y-SCREEN_Y/15, SCREEN_X/15, SCREEN_Y/15, "Reboot");
  reboot->callback(b_REBOOT,0);

  secondWindow->end();
  secondWindow->show(argc,argv);

  thread threadObj(TrackerMain, center_X, center_Y, framesPerSecond, msecondsPerSecond, ScreenSize_X, ScreenSize_Y, errorDisplay, serverIP);

  return Fl::run();

}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to read output from a console command
// @
////////////////////////////////////////////////////////////////////////////////////////////////////
string GetStdoutFromCommand(string cmd) {

  string data;
  FILE * stream;

  const int max_buffer = 256;
  char buffer[max_buffer];

  stream = popen(cmd.c_str(), "r");

  if (stream) {
    while (!feof(stream)) {
      if (fgets(buffer, max_buffer, stream) != NULL) {
        data.append(buffer);
      }
    }
    pclose(stream);
  }
  return data;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//Main function
////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv) {

  string ls = GetStdoutFromCommand("avahi-resolve -n raspberrypi.local -4 | grep \"raspberrypi.local\" | cut -f2");

  const char *serverIP = ls.c_str();

  FirstWindow(argc,argv);

  SecondWindow(argc,argv, serverIP);

}
