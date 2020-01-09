# CamTrackAI

This is our project work for school. It is a device that rotates a camera in x and y direction according to object tracker data by OpenCV.

## Prerequisites

You will need these following utilities/devices:

* [Dynamixel Rx-64](http://www.robotis.us/dynamixel-rx-64-hn05-n101/) (x2) - These are the motors we used. But you can use any Robots Dynamixel motor. They are used to turn the camera.
* [Robotis USB2Dynamixel Adapter](https://www.trossenrobotics.com/robotis-bioloid-usb2dynamixel.aspx) - It is used to communicate with the motors.

* [Raspberry Pi Camera Module](https://projects.raspberrypi.org/en/projects/getting-started-with-picamera) - Our camera, the Pi Cam. But any webcam will work fine.
* [Raspberry Pi 4 Model B](https://www.pishop.us/product/raspberry-pi-4-model-b-4gb/) - The computer we used. But you can use any computer with any Linux distro.

You will need the following libraries to compile this code:

* [OpenCV](https://opencv.org/releases/) (Not included, installing instructions further down) - For the tracking algorithm
* [Dynamixel SDK](https://github.com/ROBOTIS-GIT/DynamixelSDK) (Necessary parts included) - For controlling the dynamxiel motors
* [fltk 1.3.5](https://www.fltk.org/software.php) (Necessary parts included) - For the GUI


## Installing

### OpenCV installation

Follow the tutorial found [here](https://linuxize.com/post/how-to-install-opencv-on-ubuntu-18-04/). There are two ways presented to install OpenCV. Choose the second way further down.

After you finished the setup, type a final command into the console.

```
sudo apt-get install libopencv-dev
```

### Dynamixel SDK installation

We need to load the appropriate Dynamixel library. In the makefile, search for:

```
dxl_sbc_cpp
```

and change it to the appropriate library found in '/YOUR/PATH/TO/CAMTRACKAI/DynamixelSDK/c++/build'. If you have a x64 system, change the makefile to:

```
dxl_x64_cpp
```

Note that the suffix '.so' and the prefix 'lib' are missing. Also note that you have to install the library. You can do this by the following:

```
cd /YOUR/PATH/TO/CAMTRACKAI/DynamixelSDK/c++/build/linux_sbc
make
sudo make install
```

### Change path

Open the makefile and find this variable:

```
CAMTRACKAI_PATH = /home/theb3arybear/Desktop/CamTrackAI/GitHub/CamTrackAI
```

Change it to your location.

```
CAMTRACKAI_PATH = /YOUR/PATH/TO/CAMTRACKAI
```

### Change motor ID

Navigate to CamTrackAI.cpp and find this line.

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

## Further information

To completely remove OpenCV type into commandline:

```
find . -type d -name "*opencv*" -prune -exec rm -rf {} \;
```

## Authors

* **BE3dARt (Gianluca Imbiscuso)** - *Coding/Setup* - [BE3dARt.ch](https://be3dart.ch/)
* **Kay** - *Planning and building of tower*

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details
