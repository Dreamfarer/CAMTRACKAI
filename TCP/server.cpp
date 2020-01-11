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

using namespace cv;
using namespace std;

int capDev = 0;
VideoCapture cap(capDev);

#define PORT                            4097

void TCPServer () {
  //--------------------------------------------------------
  //networking stuff: socket, bind, listen
  //--------------------------------------------------------
  int localSocket;
  int remoteSocket;
  int port = PORT;

  bool status = true;

  struct  sockaddr_in localAddr;
  struct  sockaddr_in remoteAddr;

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

  Mat img, flippedFrame;

  int height = cap.get(CAP_PROP_FRAME_HEIGHT);
  int width = cap.get(CAP_PROP_FRAME_WIDTH);

  std::cout << "height: " << height << std::endl;
  std::cout << "width: " << width << std::endl;

  img = Mat::zeros(height, width, CV_8UC3);

  int imgSize = img.total() * img.elemSize();

  std::cout << "Image Size:" << imgSize << std::endl;

  status = true;

  int my_net_id;
  int client_id;

  while(status){
      // get a frame from the camera
      cap >> img;
      // flip the frame
      flip(img, flippedFrame, 1);

      // send the flipped frame over the network
      std::cout << "Server send" << std::endl;
      send(remoteSocket, flippedFrame.data, imgSize, 0);

      if(recv(remoteSocket, &my_net_id, 4, 0) == -1) {
        std::cout << "FèèCK" << my_net_id << std::endl;
      }
      client_id = ntohl(my_net_id);
      std::cout << "Server receive: " << client_id << std::endl;
  }

  // close the socket
  close(remoteSocket);

  return;
}

void TCPClient (char* serverIP) {
  //
  // Networking Setup
  //
  int socket_fd;
  int serverPort;

  serverPort = PORT;

  struct sockaddr_in serverAddr;
  socklen_t addrLen = sizeof(struct sockaddr_in);

  //Initiate Socket
  if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
      cout << "socket()\tERROR!" << endl;
  }

  //Specify socket propertirs (Port, IP Address)
  serverAddr.sin_family = PF_INET;
  serverAddr.sin_addr.s_addr = inet_addr(serverIP);
  serverAddr.sin_port = htons(serverPort);

  //Connect to socket
  if (connect(socket_fd, (sockaddr*)&serverAddr, addrLen) < 0) {
      cout << "connect()\tERROR!" << endl;
  }

  //
  //OpenCV Code
  //

  Mat img;
  img = Mat::zeros(480 , 640, CV_8UC3);
  int imgSize = img.total() * img.elemSize();
  uchar *iptr = img.data;
  int key;

  std::cout << "Image Size:" << imgSize << std::endl;

  namedWindow("CamTrackAI", 1);

  int my_id = 1234;
  int my_net_id = htonl(my_id);

  while (1) {
      std::cout << "Client receive" << std::endl;
      recv(socket_fd, iptr, imgSize , MSG_WAITALL);

      std::cout << "Client Send" << std::endl;
      send(socket_fd, (const char*)&my_net_id , 4, 0);

      cv::imshow("CamTrackAI", img);
      if ((key = cv::waitKey(10)) >= 0) break;
  }

  close(socket_fd);

  return;
}

int main(int argc, char** argv)
{
  if (argc == 1) {
    TCPServer();
  }

  if (argc == 2) {
    char* serverIP = argv[1];
    TCPClient(serverIP);

  }
}
