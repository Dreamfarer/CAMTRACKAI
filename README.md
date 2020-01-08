# CamTrackAI

This is our project work for school. It is a device that rotates a camera in x and y direction according to object tracker data by OpenCV.

## Prerequisites

You will need these following utilities/devices:

* [Dynamixel Rx-64](http://www.robotis.us/dynamixel-rx-64-hn05-n101/) (x2) - These are the motors we used. But you can use any Robots Dynamixel motor. They are used to turn the camera.
* [Robotis USB2Dynamixel Adapter](https://www.trossenrobotics.com/robotis-bioloid-usb2dynamixel.aspx) - It is used to communicate with the motors.

* [Raspberry Pi Camera Module](https://projects.raspberrypi.org/en/projects/getting-started-with-picamera) - Our camera, the Pi Cam. But any webcam will work fine.
* [Raspberry Pi 4 Model B](https://www.pishop.us/product/raspberry-pi-4-model-b-4gb/) - The computer we used. But you can use any computer with any Linux distro.

You will need the following libraries to compile this code:

* [OpenCV](https://opencv.org/releases/) - For the tracking algorithm
* [Dynamixel SDK](https://github.com/ROBOTIS-GIT/DynamixelSDK) - For controlling the dynamxiel motors
* [fltk 1.3.5](https://www.fltk.org/software.php) - For the GUI

Because OpenCV is a big and complex framework, you will need to install it on your own machine until I find a solution for this. You can find the tutorial [here](https://linuxize.com/post/how-to-install-opencv-on-ubuntu-18-04/). You have to chose the second presented way to install it.

After you finished the setup, type a final command into the console.

```
sudo apt-get install libopencv-dev
```

## Installing

Open the makefile and find this line.

```
-L/home/theb3arybear/Desktop/CamTrackAI/GitHub/CamTrackAI/Libraries/fltk
```

Change it to your location.

```
-L/home/USERNAME/YOURPATH/CamTrackAI/Libraries/fltk
```

Now navigate to CamTrackAI.cpp and find this line.

```
#define DXL_ID 4
```

Change the ID of the motors to match yours. Look it up how to change IDs of motors on the internet if you encounter problems.

```
#define DXL_ID X
```

## Compile and Run

Open the console and move to the makefile's location. First:

```
make
```

Then run the application with:

```
./CamTrackAI
```

## Know Issues
This project is optimised for the Raspberry PI which runs a ARM 32 bit processor. If you have another architecture, follow the provided steps:

Object files used by the application are needed in the appropriate format. Go to the makefile and search for:

```
Objx32Arm
```

You have to chose the object file that suits your architecture. In the folder 'objects' you will find a bunch. If you have a x64 system, change the makefile to:

```
Objx64
```

Next we need to load the appropriate Dynamixel library. In the makefile, search for:

```
dxl_x64_cpp
```

and change it to the appropriate library found in 'CamTrackAI/DynamixelSDK/c++/build/linux64'. If you have a x64 system, change the makefile to:

```
dxl_x64_cpp
```

## Authors

* **BE3dARt (Gianluca Imbiscuso)** - *Coding/Setup* - [BE3dARt.ch](https://be3dart.ch/)
* **Kay** - *Planning and building of tower*

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details
