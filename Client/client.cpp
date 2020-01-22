////////////////////////////////////////////////////////////////////////////////////////////////////
// **CAMTRACKAI**
//
// Coded by BE3dARt (Gianluca Imbiscuso) with <3
//
// Visit https://github.com/BE3dARt/CamTrackAI for the documentation/ installing instructions
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
string chosenTracker = ""; //Which tracker to use

//FLTK windows
Fl_Window *firstWindow = 0;
Fl_Window *secondWindow = 0;

//Shutdown and reboot
bool shutDownActivated = false;
bool rebootActivated = false;

//Mouse events
bool mouseClicked = false; 
Point leftButtonDown = Point(0, 0);
Point leftButtonHold = Point(0, 0);

////////////////////////////////////////////////////////////////////////////////////////////////////
//Defining Properties
////////////////////////////////////////////////////////////////////////////////////////////////////
#define SCREEN_X                        1920 //X-coordinate of screen (Used for GUI)
#define SCREEN_Y                        1080 //Y-coordinate of screen (Used for GUI)
#define SMALLER_STEP_RATIO				8 //Make motor speed smaller (Required because of problem with overturing of motors combined with input lag)	

#define PORT                            4097 //Network port for communication with server (Raspberry Pi)
#define SHUTDOWN						3001 //Shut down instruction for server (Raspberry Pi)
#define REBOOT							3002 //Reboot instruction for server (Raspberry Pi)

#define KEY_START           			113	//'q' key
#define KEY_END             			119 //'w' key
#define KEY_REBOOT          			114 //'r' key
#define KEY_SHUTDOWN        			102 //'f' key

////////////////////////////////////////////////////////////////////////////////////////////////////
// callBackFunc function: It triggers when mouse movements or clicks are detected (Used for selecting the Region of interest [ROI])
// @event	Defines for which mouse event the function does what
// @x		x-coordinate of mouse
// @y		y-coordinate of mouse
// @flags	Set additional condition for mouse event (Shift key or Ctrl pressed)
// -->		https://docs.opencv.org/2.4/modules/highgui/doc/user_interface.html?highlight=setmousecallback
////////////////////////////////////////////////////////////////////////////////////////////////////
void callBackFunc(int event, int x, int y, int flags) {
	//Triggers when left mouse button is pressed
  if (event == EVENT_LBUTTONDOWN && mouseClicked == false) {
    leftButtonDown = Point(x, y);
    mouseClicked = true;
  }
  
  //Triggers when mouse moves but only if previously left mouse button was pressed (Now holding)
  if (event == EVENT_MOUSEMOVE && mouseClicked == true) {
    leftButtonHold = Point(x, y);
  }
  
  //Triggers when left mouse button is released
  if (event == EVENT_LBUTTONUP) {
    mouseClicked = false;
  }
  
  //Triggers when mouse moves but also shift is pressed
  if (event == EVENT_MOUSEMOVE && flags == EVENT_FLAG_SHIFTKEY) {
    int tempLeftButtonDownX = leftButtonDown.x - x;
    int tempLeftButtonDownY = leftButtonDown.y - y;
    int tempLeftButtonHoldX = leftButtonHold.x - tempLeftButtonDownX;
    int tempLeftButtonHoldY = leftButtonHold.y - tempLeftButtonDownY;

    leftButtonDown = Point(x, y);
    leftButtonHold = Point(tempLeftButtonHoldX, tempLeftButtonHoldY);
  }
  
  //Triggers when mouse moves but also ctrl is pressed
  if (event == EVENT_MOUSEMOVE && flags == EVENT_FLAG_CTRLKEY) {
    int tempLeftButtonDownX = leftButtonDown.x - x;
    int tempLeftButtonDownY = leftButtonDown.y - y;
    int tempLeftButtonHoldX = leftButtonHold.x + tempLeftButtonDownX;
    int tempLeftButtonHoldY = leftButtonHold.y + tempLeftButtonDownY;

    leftButtonDown = Point(x, y);
    leftButtonHold = Point(tempLeftButtonHoldX, tempLeftButtonHoldY);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// smoothFrameFunc Function: Used to calculate a smoothed rectangle (So camera will not start to wiggle)
// @frame		Image (Window) on which to draw the recatngle
// @point_1		Newest tracker center point
// @point_2		-
// @point_3		-
// @point_4		-
// @point_5		Oldest tracker center point
////////////////////////////////////////////////////////////////////////////////////////////////////
Point smoothFrameFunc(Mat frame, Point point_1, Point point_2, Point point_3, Point point_4, Point point_5) {
  double tempX;
  double tempY;
  Point returnValue;

  //Calculate the weighted mean
  tempX = ((point_1.x * 0.4) + (point_2.x * 0.3) + (point_3.x * 0.2) + (point_4.x * 0.05) + (point_5.x * 0.05));
  tempY = ((point_1.y * 0.4) + (point_2.y * 0.3) + (point_3.y * 0.2) + (point_4.y * 0.05) + (point_5.y * 0.05));

  //Make Point to return
  returnValue = Point((tempX), (tempY));

  //Draw Rectangle (smooth follow)
  rectangle(frame, Point((tempX - 50), (tempY - 50)), Point((tempX + 50), (tempY + 50)), colorGreen, 2, 8, 0);

  return returnValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// guiHandlerFunc Function: Updates all GUI values
// @center_X				Pointer to GUI element which displays the tracker's center x value
// @center_Y				Pointer to GUI element which displays the tracker's center y value
// @framesPerSecond			Pointer to GUI element which displays the fps
// @msecondsPerSecond		Pointer to GUI element which displays the milliseconds used for calculating one frame
// @screenSize_X			Pointer to GUI element which displays the image's x size
// @screenSize_Y			Pointer to GUI element which displays the image's y size
// @middle					Point (x,y) of the tracker's center
// @frameInfo				Point (x,y) of the size of the image
// @pictureTime				Time consumed for one while loop, aka calulating one frame
// @smoothedCenter			Point (x,y) of the tracker's smoothed out center
// @errorDisplay			Pointer to GUI element which displays if there is an error with the tracker
// @trackerError			True if there is an error with the tracker
////////////////////////////////////////////////////////////////////////////////////////////////////
void guiHandlerFunc(Fl_Output*center_X, Fl_Output*center_Y, Fl_Output*framesPerSecond, Fl_Output*msecondsPerSecond, Fl_Output*screenSize_X, Fl_Output*screenSize_Y, Point middle, Point frameInfo, double pictureTime, Point smoothedCenter, Fl_Output *errorDisplay, bool trackerError) {
  int fps = 1000/pictureTime;  //Frames per second for calculation

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
  framesPerSecond->value(to_string(fps).c_str());
  msecondsPerSecond->value(to_string(pictureTime).c_str());
  screenSize_X->value(to_string(frameInfo.x).c_str());
  screenSize_Y->value(to_string(frameInfo.y).c_str());
  errorDisplay->value(errorString.c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// shutDownFunc Function: Used to shot down or reboot server (Raspberry Pi)
// @socket	The network socket that will be closed
////////////////////////////////////////////////////////////////////////////////////////////////////
void shutDownFunc(int socket) {
	int movementInstruction[2];

    if (rebootActivated) {
      movementInstruction[0] = REBOOT;
      cout << "\n\n=====================================================================================\nRebooting Raspberry Pi ...\n=====================================================================================\n\n" << endl;
    } else {
      movementInstruction[0] = SHUTDOWN; //Set movement instruction to 'REBOOT'. So the Raspberry Pi will receive it and reboot.
      cout << "\n\n=====================================================================================\nShutting down Raspberry Pi ...\n=====================================================================================\n\n" << endl;
    }
    movementInstruction[1] = 0; //Set the motor to 0 movement

    send(socket, movementInstruction , 8, 0); //Send movement instruction to Raspberry Pi

    close(socket); //Close socket

    exit(0); //Exit application
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// trackerMainFunc: Basically the main function. Everything will be called, calculated and executed from here.
// @center_X				Pointer to GUI element which displays the tracker's center x value
// @center_Y				Pointer to GUI element which displays the tracker's center y value
// @framesPerSecond			Pointer to GUI element which displays the fps
// @msecondsPerSecond		Pointer to GUI element which displays the milliseconds used for calculating one frame
// @screenSize_X			Pointer to GUI element which displays the image's x size
// @screenSize_Y			Pointer to GUI element which displays the image's y size
// @trackerDisplay			Pointer to GUI element which displays which tracker is used
// @serverIP				Pointer to IP Address
// @errorDisplay			Pointer to GUI element which displays if there is an error with the tracker
////////////////////////////////////////////////////////////////////////////////////////////////////
int trackerMainFunc(Fl_Output*center_X, Fl_Output*center_Y, Fl_Output*framesPerSecond, Fl_Output*msecondsPerSecond, Fl_Output*screenSize_X, Fl_Output*screenSize_Y, Fl_Output*errorDisplay, const char* serverIP) {

  Point smoothedCenter; //Point (x,y) to store the smoothed out center of tracker
  int moveMotorX; //How fast to move motors in x direction
  int moveMotorY; //How fast to move motors in y direction
  int stepX; //How big the steps are for motor movement in x direction (that it is 0 movement in the center and 100% movement at border of screen)
  int stepY; //How big the steps are for motor movement in y direction (that it is 0 movement in the center and 100% movement at border of screen)
  int differnceToCenterX; //Difference from tracker's x center to screen's x center
  int differnceToCenterY; //Difference from tracker's y center to screen's y center
  int positionArray[2] = {0, 0}; //Holds motors movement x and y value (Used for sending over TCP)
  int localSocket; //Socket
  int serverPort = PORT; //Port over which connection goes

  bool running = true; //For exiting the while loop which tracks the object

  duration<double, milli> time_span; //Timeintervall to calculate 'msecondsPerSecond' and 'fps'
  high_resolution_clock::time_point beginTime = high_resolution_clock::now(); //Start of counter
  high_resolution_clock::time_point endTime = high_resolution_clock::now(); //Start of counter

  Point point_1 = Point(1, 9); //Oldest frame
  Point point_2 = Point(2, 8); //-
  Point point_3 = Point(3, 7); //-
  Point point_4 = Point(4, 6); //-
  Point point_5 = Point(5, 5); //-
  Point middle = Point(1,1); //Newest frame
  Point frameInfo = Point(480, 300); //Point (x,y) which holds image resolution

  struct sockaddr_in serverAddr; //Struct that holds IP Adress/Port/Family
  socklen_t addrLen = sizeof(struct sockaddr_in); //Calculate size which is reserved for 'serverAddr'

  Mat img; //holds image (frame)
  img = Mat::zeros(300 , 480, CV_8UC3); //Fill in image (frame) with 0
  int imgSize = img.total() * img.elemSize(); //Image (frame) size
  uchar *iptr = img.data; //Pointer to image (frame) for it to be used with TCP/IP

  namedWindow("CamTrackAI", WINDOW_NORMAL); //Create an OpenCV window

  setMouseCallback("CamTrackAI", callBackFunc); //Set a callback function for mouse events

  Rect2d trackingBox = Rect2d(Point(100,100), Point(0,0)); //Box which is drawn by the user for selecting the ROI (Region of interest)
  int exitWhile = 0; //Exit a while loop
  
  bool trackerError; //Returning value by tracker->update()

  //Initiate Socket
  if ((localSocket = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    cout << "socket()\tERROR!" << endl;
  }

  //Fill in struct 'serverAddr'
  serverAddr.sin_family = PF_INET; //Family
  serverAddr.sin_addr.s_addr = inet_addr(serverIP); //IP Address
  serverAddr.sin_port = htons(serverPort); //Port

  //Connect to socket
  if (connect(localSocket, (sockaddr*)&serverAddr, addrLen) < 0) {
    cout << "connect()\tERROR!" << endl;
	exit(0);
  }

  while (true) {
    //Fill with 0 so that motors won't move
    positionArray[0] = 0;
    positionArray[1] = 0;

    //Evaluate decision by user and define type of tracker to be used. If there was an error, Reboot instructions will be sent to the server (Raspberry Pi).
    Ptr<Tracker> tracker;
    if (chosenTracker == "KCF") {
      tracker = TrackerKCF::create();
    } else if (chosenTracker == "CSRT") {
      tracker = TrackerCSRT::create();
    } else if (chosenTracker == "TLD") {
      tracker = TrackerTLD::create();
    } else if (chosenTracker == "MIL") {
      tracker = TrackerMIL::create();
    } else if (chosenTracker == "MOSSE") {
      tracker = TrackerMOSSE::create();
    } else if (chosenTracker == "Boosting") {
      tracker = TrackerBoosting::create();
    } else if (chosenTracker == "MedianFlow") {
      tracker = TrackerMedianFlow::create();
    } else {
      rebootActivated = true;
      shutDownFunc(localSocket);
    }

    //Choose what object to track (ROI).
    while (exitWhile != KEY_START) {
      recv(localSocket, iptr, imgSize , MSG_WAITALL); //Receive image from server (Raspberry Pi) and store it into 'img' (iptr)

      trackingBox = Rect2d(leftButtonDown, leftButtonHold); //Update rectangle which is drawn with the user's mouse
      rectangle(img, trackingBox, colorPurple, 2, 8); //Make the drawn box visible

      imshow("CamTrackAI", img); //Show image with 'img' and the rectangle in it

      exitWhile = waitKey(20); //Wait 25 milliseconds for a key to be pressed. 'exitWhile' will change to 'KEY_START' if this buttons has been pressed, thus exiting the loop.
		
      if (exitWhile == KEY_REBOOT) {
        rebootActivated = true;
      }

      if (exitWhile == KEY_SHUTDOWN) {
        shutDownActivated = true;
      }
		
		//Check if user requested a reboot or shut down of server (Raspberry Pi). Else just send movement instructions to server.
      if (shutDownActivated == true || rebootActivated == true) {
        exitWhile = KEY_START;
        shutDownFunc(localSocket);
      } else {
        send(localSocket, positionArray , 8, 0); //Send movement instructions to motors
      }
    }

    exitWhile = 0; //Reset for it to be used in another loop

    tracker->init(img, trackingBox); //Initalize tracker with the user's defined box

    running = true; //Reset for it to be used in another loop

    while (running) {

      beginTime = high_resolution_clock::now(); //Begin timer

      //Receive image from server (Raspberry Pi) and store it into 'img' (iptr). If there was an error, send reboot instruction to server (Raspberry Pi) and close the application
      if (recv(localSocket, iptr, imgSize , MSG_WAITALL) == -1) {
        cout << "recv()\tERROR!" << endl;
        rebootActivated = true;
        shutDownFunc(localSocket);
      }

      //The tracker searches for the object in the current frame. If it fails, it will send 0 (motor zero movement) to the server (Raspberry Pi) until the object is found again.
      if ((trackerError = tracker->update(img, trackingBox)) == true) {
        rectangle(img, trackingBox, colorPurple, 2, 8); //Make trackerdata visible through a rectangle

        middle = Point((trackingBox.x + trackingBox.width / 2),(trackingBox.y + trackingBox.height / 2)); //Calucalte center of tracker

        //Update Points (from oldest frame to newest frame)
        point_5 = point_4;
        point_4 = point_3;
        point_3 = point_2;
        point_2 = point_1;
        point_1 = middle;

        smoothedCenter = smoothFrameFunc(img, point_1, point_2, point_3, point_4, point_5);

        Fl::check(); //Refresh GUI

        imshow("CamTrackAI", img); //Show image (frame) with the rectangle (tracker data) in it

        //Calculate the steps based on the image's resolution and maximal movement by the Dynamixel motors.
        stepX = 1023 / (frameInfo.x/2);
        stepY = 1023 / (frameInfo.y/2);

        //Calucalte difference from tracker's x center to screen's x center
        differnceToCenterX = smoothedCenter.x - (frameInfo.x/2);
        differnceToCenterY = smoothedCenter.y - (frameInfo.y/2);

        //Checks if the difference from the center is + or minus 0. If it is < 0, it will send a number ranging from 0-1023 to server (Raspberry Pi). If it is > 0, it will send numbers ranging from 1024-2047. This is used to define the direction of spinning.
        if(differnceToCenterX < 0) {
          moveMotorX = (abs(differnceToCenterX) * stepX)/4;
        } else {
          moveMotorX = ((abs(differnceToCenterX) * stepX)/4) + 1024;
        }

        if(differnceToCenterY < 0) {
          moveMotorY = (abs(differnceToCenterY) * stepY)/SMALLER_STEP_RATIO;
        } else {
          moveMotorY = ((abs(differnceToCenterY) * stepY)/SMALLER_STEP_RATIO) + 1024;
        }

        //Set movement instruction for the motors
        positionArray[0] = moveMotorX;
        positionArray[1] = moveMotorY;
      } else {
        positionArray[0] = 0;
        positionArray[1] = 0;
      }

      int k = waitKey(1); //Wait 1 millisecond for key to be pressed (Else it would not detect key pressed)
	  
	   // Exit the loop if 'KEY_END' was pressed
      if(k == KEY_END)
      {
        running = false;
      }
		
      if (k == KEY_REBOOT) {
        rebootActivated = true;
      }
	  
      if (k == KEY_SHUTDOWN) {
        shutDownActivated = true;
      }

      guiHandlerFunc(center_X, center_Y, framesPerSecond, msecondsPerSecond, screenSize_X, screenSize_Y, middle, frameInfo, time_span.count(), smoothedCenter, errorDisplay, trackerError); //Send values to GUI

	  //Check if user requested a reboot or shut down of server (Raspberry Pi). Else just send movement instructions to server.
      if (shutDownActivated == true || rebootActivated == true) {
        shutDownFunc(localSocket);
      } else {
        send(localSocket, positionArray , 8, 0);
      }

      //Timestamp 2 and time span calulating
      endTime = high_resolution_clock::now();
      time_span = endTime - beginTime;
    }
  }
  close(localSocket); //Close socket

  exit(0); //Exit application

  return 0;

}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Buttons for the GUI
// @FL_Widget	Target widget
// @*			Data that can be stored in a Widget with fltk::Widget::user_data(void*)
// --> 			Description taken from https://www.fltk.org/doc-2.0/html/group__example2.html
////////////////////////////////////////////////////////////////////////////////////////////////////
//Choose KCF
void b_KCF(Fl_Widget *, void *) {
  fl_beep();
  chosenTracker = "KCF";
  firstWindow->hide();
}

//Choose CSRT
void b_CSRT(Fl_Widget *, void *) {
  fl_beep();
  chosenTracker = "CSRT";
  firstWindow->hide();
}

//Choose TLD
void b_TLD(Fl_Widget *, void *) {
  fl_beep();
  chosenTracker = "TLD";
  firstWindow->hide();
}

//Choose MIL
void b_MIL(Fl_Widget *, void *) {
  fl_beep();
  chosenTracker = "MIL";
  firstWindow->hide();
}

//Choose MOSSE
void b_MOSSE(Fl_Widget *, void *) {
  fl_beep();
  chosenTracker = "MOSSE";
  firstWindow->hide();
}

//Choose BOOSTING
void b_BOOSTING(Fl_Widget *, void *) {
  fl_beep();
  chosenTracker = "Boosting";
  firstWindow->hide();
}

//Choose MedianFlow
void b_MEDIANFLOW(Fl_Widget *, void *) {
  fl_beep();
  chosenTracker = "MedianFlow";
  firstWindow->hide();
}

//Activate shut down of server (Raspberry Pi)
void b_SHUTDOWN(Fl_Widget *, void *) {
  fl_beep();
  firstWindow->hide();
  shutDownActivated = true;
}

//Activate reboot of server (Raspberry Pi)
void b_REBOOT(Fl_Widget *, void *) {
  fl_beep();
  rebootActivated = true;
  firstWindow->hide();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// firstWindowFunc Function: Used to display the first window and start choose the tracker algorithm
// @argc		How many arguments are passed to the function	
// @argv		Char value of passed arguments
////////////////////////////////////////////////////////////////////////////////////////////////////
int firstWindowFunc(int argc, char **argv) {

  firstWindow = new Fl_Window(0,0,SCREEN_X/2,SCREEN_Y);
  firstWindow->resizable(firstWindow);

	//Buttons for choosing tracker algorithm
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

	//Button for shutting down server (Raspberry Pi)
  Fl_Button *shutDown = new Fl_Button(0,SCREEN_Y-SCREEN_Y/15, SCREEN_X/15, SCREEN_Y/15, "Shut Down");
  shutDown->callback(b_SHUTDOWN,0);

	//Button for rebooting server (Raspberry Pi)
  Fl_Button *reboot = new Fl_Button(SCREEN_X/15 + 20,SCREEN_Y-SCREEN_Y/15, SCREEN_X/15, SCREEN_Y/15, "Reboot");
  reboot->callback(b_REBOOT,0);

  firstWindow->end(); //End setting up elements for window 'firstWindow'
  firstWindow->show(argc,argv); //Show GUI

  return Fl::run(); //Run (Needed to keep GUI up)
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// secondWindowFunc Function: Used to display the second window and start the actual tracking process
// @argc		How many arguments are passed to the function	
// @argv		Char value of passed arguments
// @serverIP	IP of server (Raspberry Pi)
////////////////////////////////////////////////////////////////////////////////////////////////////
int secondWindowFunc(int argc, char **argv, const char* serverIP) {
	
  //Create a new window
  secondWindow = new Fl_Window(0,0,SCREEN_X/2,SCREEN_Y);
  secondWindow->resizable(secondWindow);

  //Group of GUI elements with Titel "Tracker Information"
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
  trackerDisplay->value(chosenTracker.c_str());

  Fl_Output *errorDisplay = new Fl_Output(10, 1*(SCREEN_Y/10)+160, 150, 20, "Tracker Status ");
  errorDisplay->labelsize(15);
  errorDisplay->align(FL_ALIGN_RIGHT);

  //Group of GUI elements with Titel "Video Information"
  Fl_Output *videoInfo = new Fl_Output(0, 4*(SCREEN_Y/10)-10, SCREEN_X/2, 2, "Video Information");
  videoInfo->labelsize(25);
  videoInfo->align(FL_ALIGN_TOP|FL_ALIGN_LEFT);

  Fl_Output *screenSize_X = new Fl_Output(10, 4*(SCREEN_Y/10)+10, 150, 20, "px Screensize (x) ");
  screenSize_X->labelsize(15);
  screenSize_X->align(FL_ALIGN_RIGHT);

  Fl_Output *screenSize_Y = new Fl_Output(10, 4*(SCREEN_Y/10)+40, 150, 20, "px Screensize (y) ");
  screenSize_Y->labelsize(15);
  screenSize_Y->align(FL_ALIGN_RIGHT);

  //Group of GUI elements with Titel "More Information"
  Fl_Output *moreInfo = new Fl_Output(0, 6*(SCREEN_Y/10)-10, SCREEN_X/2, 2, "More Information");
  moreInfo->labelsize(25);
  moreInfo->align(FL_ALIGN_TOP|FL_ALIGN_LEFT);

  Fl_Output *ipDisplay = new Fl_Output(10, 6*(SCREEN_Y/10)+10, 150, 20, "Connected (IP Address) ");
  ipDisplay->labelsize(15);
  ipDisplay->align(FL_ALIGN_RIGHT);
  ipDisplay->value(serverIP);

  //Group of GUI elements with Titel "Controls"
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

	//Button for shutting down server (Raspberry Pi)
  Fl_Button *shutDown = new Fl_Button(0,SCREEN_Y-SCREEN_Y/15, SCREEN_X/15, SCREEN_Y/15, "Shut down");
  shutDown->callback(b_SHUTDOWN,0);
	
	//Button for rebooting server (Raspberry Pi)
  Fl_Button *reboot = new Fl_Button(1*SCREEN_X/15 + 10,SCREEN_Y-SCREEN_Y/15, SCREEN_X/15, SCREEN_Y/15, "Reboot");
  reboot->callback(b_REBOOT,0);

  secondWindow->end(); //End setting up elements for window 'secondWindow'
  secondWindow->show(argc,argv); //Show GUI

  thread firstThread(trackerMainFunc, center_X, center_Y, framesPerSecond, msecondsPerSecond, screenSize_X, screenSize_Y, errorDisplay, serverIP); //Start thread

  return Fl::run(); //Run (Needed to keep GUI up)
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// getStdoutFromCommandFunc Function: This function reads the output of a console command
// @cmd		Which function/script to call in console
// -->		Author of this function is Jeremy Morgan. https://www.jeremymorgan.com/tutorials/c-programming/how-to-capture-the-output-of-a-linux-command-in-c/ . I edited some small parts.
////////////////////////////////////////////////////////////////////////////////////////////////////
string getStdoutFromCommandFunc(string cmd) {
	
  string data;
  FILE * stream;

	//Buffer for string
  const int max_buffer = 256;
  char buffer[max_buffer];

  stream = popen(cmd.c_str(), "r");

	//Read ouput until there are no more characters to read ('fgets' returns Null).
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
// Main Function: This function gets called when the program is being executed
// @argc	How many arguments are passed to the function	
// @argv	Char value of passed arguments
////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv) {
	
	//Resolve IP-address to hostname and store it a string
	string ls = getStdoutFromCommandFunc("avahi-resolve -n raspberrypi.local -4 | grep \"raspberrypi.local\" | cut -f2");
	const char *serverIP = ls.c_str();

	firstWindowFunc(argc,argv); //Show first GUI window

	secondWindowFunc(argc,argv, serverIP); //Show second GUI window
}
