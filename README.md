# CamTrackAI

This project is a work we have to do for school.

## Prerequisites

You will need these following utilities/devices:

* [Dynamixel Rx-64](http://www.robotis.us/dynamixel-rx-64-hn05-n101/)x2 - These are the motors we used. But you can use any Robots Dynamixel motor. They are used to turn the camera.
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

## Authors

* **BE3dARt (Gianluca Imbiscuso)** - *Coding/Setup* - [BE3dARt.ch](https://be3dart.ch/)
* **Kay O.** - *Planning and building of tower*

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details
