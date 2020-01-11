CC = g++ -std=c++11 -pthread
CFLAGS = -g -Wall
OPENCV = `pkg-config opencv4 --cflags --libs`
LIBS = $(OPENCV)

all:
	echo "compiling srv..."
	$(CC) $(CFLAGS) -o server server.cpp $(LIBS) -lpthread

server:srv.cpp
	$(CC) $(CFLAGS) -o server server.cpp $(LIBS) -lpthread
