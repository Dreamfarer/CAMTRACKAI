/**
* OpenCV video streaming over TCP/IP
* Server: Captures video from a webcam and send it to a client
* by Isaac Maia
*
* modified by Sheriff Olaoye (sheriffolaoye.com)
*/

#include "opencv2/opencv.hpp"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <string.h>

#include "dynamixel_sdk.h"

using namespace cv;
using namespace std;

#define DXL_ID_1                          1
#define DXL_ID_2                          2

#define VIDEOSOURCE											0

#define CW_ANGLE_LIMIT                  6
#define CCW_ANGLE_LIMIT                 8
#define MOVING_SPEED                    32
#define TORQUE                          24
#define TORQUE_LIMIT                    34

#define PROTOCOL_VERSION                1.0
#define BAUDRATE                        57600
#define DEVICENAME                      "/dev/ttyUSB0"

#define PORT                            4097

void TCPServer () {
	//Inizializing Variables
	int returnValue;

	int localSocket;
	int remoteSocket;
	int port = PORT;

	bool status = true;

	VideoCapture cap(VIDEOSOURCE);

	int my_net_id;
	int client_id;

	//Networking
	struct sockaddr_in localAddr;
	struct sockaddr_in remoteAddr;

	int addrLen = sizeof(struct sockaddr_in);

	if ((localSocket = socket(AF_INET , SOCK_STREAM , 0)) < 0){
		cout << "socket()\tERROR!" << endl;
		return;
	}

	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = INADDR_ANY;
	localAddr.sin_port = htons( port );

	if (bind(localSocket,(struct sockaddr *)&localAddr , sizeof(localAddr)) < 0) {
		cout << "bind()\tERROR!" << endl;
		close(localSocket);
		return;
	}

	// Listening
	listen(localSocket , 3);
	std::cout <<  "Waiting for connections...\n" << endl;

	//accept connection from an incoming client
	while(status){
		if ((remoteSocket = accept(localSocket, (struct sockaddr *)&remoteAddr, (socklen_t*)&addrLen)) < 0) {
			cout << "accept()\tERROR" << endl;
			close(localSocket);
			close(remoteSocket);
			return;
		} else {
			status = false;
		}
	}

	//OpenCV
	Mat img, flippedFrame;

	//Check if Capturing Device is up
	if (cap.isOpened()) {
		cout << "Starting Video Capturing device\t(...)\tSuccess!" << endl;
	}
	else {
		cout << "Starting Video Capturing device\t(...)\tError!" << endl;
	}

	cap.set(CAP_PROP_FRAME_WIDTH, 480);
	cap.set(CAP_PROP_FRAME_HEIGHT, 300);

	int height = cap.get(CAP_PROP_FRAME_HEIGHT);
	int width = cap.get(CAP_PROP_FRAME_WIDTH);

	std::cout << "height: " << height << std::endl;
	std::cout << "width: " << width << std::endl;

	img = Mat::zeros(height, width, CV_8UC3);
	int imgSize = img.total() * img.elemSize();
	std::cout << "Image Size:" << imgSize << std::endl;

	dynamixel::PortHandler *portHandler = dynamixel::PortHandler::getPortHandler(DEVICENAME);
	dynamixel::PacketHandler *packetHandler = dynamixel::PacketHandler::getPacketHandler(PROTOCOL_VERSION);

	// Open port
	returnValue = portHandler->openPort();

	// Set port baudrate
	returnValue = portHandler->setBaudRate(BAUDRATE);

	//Enable torque
	returnValue = packetHandler->write1ByteTxOnly(portHandler, DXL_ID_1, TORQUE, 1);
	returnValue = packetHandler->write1ByteTxOnly(portHandler, DXL_ID_2, TORQUE, 1);

	//Set torque limit
	returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_1, TORQUE_LIMIT, 1023);
	returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_2, TORQUE_LIMIT, 1023);

	//CW angle limit
	returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_1, CW_ANGLE_LIMIT, 0);
	returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_2, CW_ANGLE_LIMIT, 0);

	//CCW angle limit
	returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_1, CCW_ANGLE_LIMIT, 0);
	returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_2, CCW_ANGLE_LIMIT, 0);

	status = true;

	int positionArray[2];

	while(status){
		// get a frame from the camera
		cap >> img;
		// flip the frame
		flip(img, flippedFrame, 1);

		// send the flipped frame over the network
		std::cout << "Server send" << std::endl;
		send(remoteSocket, flippedFrame.data, imgSize, 0);

		returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_1, MOVING_SPEED, positionArray[0]);

		if(recv(remoteSocket, positionArray, 8, 0) == -1) {
			std::cout << "FèèCK" << positionArray << std::endl;
		}

		positionArray[0] = positionArray[0];
		positionArray[1] = positionArray[1];

		std::cout << "Server receive: x:" << positionArray[0] << "y:" << positionArray[1] << std::endl;

		//Write bytes to Dynamixel Motors

		returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_2, MOVING_SPEED, positionArray[1]);


	}

	uint8_t receivedPackage;
	uint16_t receivedError;

	do {
		packetHandler->read2ByteRx(portHandler, receivedPackage, &receivedError);
		//Set Speed
		returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_1, MOVING_SPEED, 0);
		returnValue = packetHandler->write2ByteTxOnly(portHandler, DXL_ID_2, MOVING_SPEED, 0);
	} while(receivedError != 0);

	// Close port
	portHandler->closePort();

	// close the socket
	close(remoteSocket);

	return;
}

int main(int argc, char** argv)
{
	TCPServer();
}
