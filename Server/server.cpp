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
#include "opencv2/opencv.hpp"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <string.h>

#include "dynamixel_sdk.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
//Defining namespaces
////////////////////////////////////////////////////////////////////////////////////////////////////
using namespace cv;
using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////
//Defining Properties
////////////////////////////////////////////////////////////////////////////////////////////////////
//Dynamixel
//--Byte positions in array (May differ from motor to motor)
#define CW_ANGLE_LIMIT                  6
#define CCW_ANGLE_LIMIT                 8
#define MOVING_SPEED                    32
#define TORQUE                          24
#define TORQUE_LIMIT                    34

//--Values
#define PROTOCOL_VERSION                1.0
#define BAUDRATE                        57600
#define DEVICENAME                      "/dev/ttyUSB0"
#define DXL_ID_1                        2 //ID of first motor
#define DXL_ID_2                        1 //ID of second motor

//OpenCV
#define VIDEOSOURCE						0
#define RESOLUTIONHEIGHT				300
#define RESOLUTIONWIDTH					480

//Networking
#define PORT                            4097
#define SHUTDOWN						3001
#define REBOOT							3002

////////////////////////////////////////////////////////////////////////////////////////////////////
// shutDownFunc Function: When called shuts down or reboots server (Raspberry Pi)
// @argument			Determines if function shuts down or reboots server (Raspberry Pi)
// @portHandler			Pointer to the porthandler of Dynamixel (Used for port handling)
// @packetHandler		Pointer to the packethandler of Dynamixel (Used for protocol version handling)
// @localSocket			Variable used to modify the local socket
// @remoteSocket		Variable used to modify the remote socket
////////////////////////////////////////////////////////////////////////////////////////////////////
void shutDownFunc(int argument, dynamixel::PortHandler *portHandler, dynamixel::PacketHandler *packetHandler, int localSocket, int remoteSocket) {

	close(localSocket); //Close local socket
	close(remoteSocket); //Close remote socket

	//Set movement of both motors to zero (There needs to be a short break in between the calls because of the adapters speed)
	packetHandler->write2ByteTxOnly(portHandler, DXL_ID_1, MOVING_SPEED, 0);
	cout << "2 seconds remaining..." << endl;
	sleep(1);
	packetHandler->write2ByteTxOnly(portHandler, DXL_ID_2, MOVING_SPEED, 0);
	cout << "1 second remaining..." << endl;
	sleep(1);

	portHandler->closePort(); //Close USB port

	//Shuts down or reboots server (Raspberry Pi) dependent on the argument pass to this function
	if(argument == SHUTDOWN) {
		system("shutdown -P now");
	} else {
		system("shutdown -r now");
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Main Function: This function gets called when the program is being executed
// @argc	How many arguments are passed to the function
// @argv	Char value of passed arguments
////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
	//Inizializing variables and structs
	int returnValue; //Stores return values of Dynamixel functions
	int localSocket; //Local socket
	int remoteSocket; //Remote socket
	bool status = true; //Used to run while loops until status turns false
	int positionArray[2]; //Array that stores motor instructions received by client
	Mat img; //OpenCV image that will get sent to client
	struct sockaddr_in localAddr; //struct that stores network information for local socket
	struct sockaddr_in remoteAddr; //struct that stores network information for remote socket
	int addrLen = sizeof(struct sockaddr_in); //Size of sockaddr_in struct

	//Define port and protocol version for communicating with Dynamixel motors
	dynamixel::PortHandler *portHandler = dynamixel::PortHandler::getPortHandler(DEVICENAME);
	dynamixel::PacketHandler *packetHandler = dynamixel::PacketHandler::getPacketHandler(PROTOCOL_VERSION);

	//Populating struct for socket
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = INADDR_ANY;
	localAddr.sin_port = htons(PORT);

	VideoCapture cap(VIDEOSOURCE); //Set video device

	//Setup / prepare socket and check if it an error occurs.
	if ((localSocket = socket(AF_INET , SOCK_STREAM , 0)) < 0){
		cout << "socket()\tERROR!" << endl;
		shutDownFunc(REBOOT, portHandler, packetHandler, localSocket, remoteSocket);
	}

	//Bind struct with socket and check if an error occurs.
	if (bind(localSocket,(struct sockaddr *)&localAddr , sizeof(localAddr)) < 0) {
		cout << "bind()\tERROR!" << endl;
		shutDownFunc(REBOOT, portHandler, packetHandler, localSocket, remoteSocket);
	}

	listen(localSocket , 3); // Listening for an incoming connection

	//Loop through function until a client connects to the socket
	while(status){
		if ((remoteSocket = accept(localSocket, (struct sockaddr *)&remoteAddr, (socklen_t*)&addrLen)) < 0) {
			cout << "accept()\tERROR" << endl;
			shutDownFunc(REBOOT, portHandler, packetHandler, localSocket, remoteSocket);
		} else {
			status = false;
		}
	}

	//Check if the capturing devics is up
	if (cap.isOpened() == false) {
		cout << "Opening capturing device\tERROR!" << endl;
		shutDownFunc(REBOOT, portHandler, packetHandler, localSocket, remoteSocket);
	}

	//Adjust the resolution of capture device
	cap.set(CAP_PROP_FRAME_WIDTH, RESOLUTIONWIDTH);
	cap.set(CAP_PROP_FRAME_HEIGHT, RESOLUTIONHEIGHT);

	img = Mat::zeros(RESOLUTIONHEIGHT, RESOLUTIONWIDTH, CV_8UC3); //Populate matrix with zeros

	int imgSize = img.total() * img.elemSize(); //Calucalte image size for sending over TCP_IP

	returnValue = portHandler->openPort(); //Open port for communication with Dynamixel motors

	returnValue = portHandler->setBaudRate(BAUDRATE); //Set port baudrate (Communication rate)

	//Enable torque
	returnValue = packetHandler->write1ByteTxOnly(portHandler, DXL_ID_1, TORQUE, 1);
	returnValue = packetHandler->write1ByteTxOnly(portHandler, DXL_ID_2, TORQUE, 1);

	//Set torque limit
	returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_1, TORQUE_LIMIT, 1023);
	returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_2, TORQUE_LIMIT, 1023);

	//CW angle limit (Used to set motors from joint mode to wheel mode)
	returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_1, CW_ANGLE_LIMIT, 0);
	returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_2, CW_ANGLE_LIMIT, 0);

	//CCW angle limit (Used to set motors from joint mode to wheel mode)
	returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_1, CCW_ANGLE_LIMIT, 0);
	returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_2, CCW_ANGLE_LIMIT, 0);

	status = true;
	while(status){
		cap >> img; //Get a frame from the camera

		//Send image to client / If there is an error, reboot server (Raspberry Pi)
		if(send(remoteSocket, img.data, imgSize, 0) == -1) {
			std::cout << "send()\tERROR!" << std::endl;
			shutDownFunc(REBOOT, portHandler, packetHandler, localSocket, remoteSocket); //Reboot server (Raspberry Pi)
		}

		returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_1, MOVING_SPEED, positionArray[0]); //Write movement instructions to one Dynamixel Motor

		//Receive movement instructions for Dynamixel motors / If there is an error, reboot server (Raspberry Pi)
		if(recv(remoteSocket, positionArray, 8, 0) == -1) {
			std::cout << "Recv()\tERROR!" << std::endl;
			shutDownFunc(REBOOT, portHandler, packetHandler, localSocket, remoteSocket); //Reboot server (Raspberry Pi)
		}

		//Shut down or reboot server (Raspberry Pi) if requested by client
		if(positionArray[0] == SHUTDOWN || positionArray[0] == REBOOT) {
			status = false; //Exit loop
			shutDownFunc(positionArray[0], portHandler, packetHandler, localSocket, remoteSocket); //Shut down or reboot server (Raspberry Pi) based on request of client
		}

		returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_2, MOVING_SPEED, positionArray[1]); //Write movement instructions to one Dynamixel Motor
	}

	shutDownFunc(REBOOT, portHandler, packetHandler, localSocket, remoteSocket); //Reboot server (Raspberry Pi)

	return 0;
}
