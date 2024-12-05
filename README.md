# CamTrackAI

This is our project work for school. It is an autonomous device powered by a Raspberry Pi 4 that rotates a camera according to object tracker data by OpenCV.

It uses the TCP/IP to send a video source from the Raspberry Pi to a more powerful computer to calculates the object tracking. It will return the calculated data to the Raspberry Pi which then communicates with our Dynamixel Rx-64 motors.

## Prerequisites

You will need these following utilities/devices:

* [Dynamixel Rx-64](http://www.robotis.us/dynamixel-rx-64-hn05-n101/) (x2) - These are the motors we used. But you can use any Robotis Dynamixel motor.
* [Robotis USB2Dynamixel Adapter](https://www.trossenrobotics.com/robotis-bioloid-usb2dynamixel.aspx) - It is used to communicate with the Dynamixel motors.
* [Raspberry Pi Camera Module](https://projects.raspberrypi.org/en/projects/getting-started-with-picamera) - Our camera, the Pi Cam. But any webcam will work fine.
* [Raspberry Pi 4 Model B](https://www.pishop.us/product/raspberry-pi-4-model-b-4gb/) - The computer we used as server. But you can use any computer with any Linux distro.
* [Gens Ace 5000mah 14.8V](https://www.gensace.de/gens-ace-5000mah-14-8v-45c-4s1p-flat-lipo-battery.html) - You will need a 4S1P Battery to power the motors. Additionally add a 3A step down to 5V to power the Raspberry Pi.

You will need the following libraries/packages to compile this code:

* [OpenCV](https://opencv.org/releases/) (Not included, installing instructions further down) - For the tracking algorithm
* [Dynamixel SDK](https://github.com/ROBOTIS-GIT/DynamixelSDK) (Necessary parts included) - For controlling the Dynamxiel motors
* [FLTK](https://www.fltk.org/software.php) (Not included, installing instructions further down) - GUI
* [Avahi-Daemon](https://linux.die.net/man/8/avahi-daemon) (Not included, installing instructions further down) - Used for mDNS to resolve an IP address by a hostname.


## Installing

### OpenCV installation

Follow the tutorial found [here](https://linuxize.com/post/how-to-install-opencv-on-ubuntu-18-04/). There are two ways presented to install OpenCV. Choose the second way found further down.

After you finished the setup, type a final command into the console.

```
$sudo apt-get install libopencv-dev
```

### FLTK (The Fast Light Tool Kit ) installation

Follow these steps to a successful installation of FLTK. The complete tutorial is to be found [here](https://courses.cs.washington.edu/courses/cse557/14au/tools/fltk_install.html)

Firstly, you will need to download FLTK and extract it. Then follow these steps:

```
$./configure
$make
$sudo make install
```

### Dynamixel SDK installation

We need to load the appropriate Dynamixel library. In the server's makefile, search for:

```
dxl_sbc_cpp
```

and change it to the appropriate library found in '/YOUR/PATH/TO/CAMTRACKAI/DynamixelSDK/build'. If you have a x64 system, change the makefile to:

```
dxl_x64_cpp
```

Note that the suffix '.so' and the prefix 'lib' are missing. Also note that you have to install the library. You can do this by the following:

```
$cd /YOUR/PATH/TO/CAMTRACKAI/DynamixelSDK/build/linux64
$make
sudo make install
```
### Avahi Daemon installation

```
$sudo apt-get install avahi-daemon
```

### Change path

Open both makefiles and find this variable:

```
CAMTRACKAI_PATH = /home/theb3arybear/Desktop/CamTrackAI/GitHub/CamTrackAI
```

Change the path to your location.

```
CAMTRACKAI_PATH = /YOUR/PATH/TO/CAMTRACKAI
```

### Change motor ID

Navigate to in CamTrackAIServer.cpp in Server and find these line:

```
#define DXL_ID_1        1
#define DXL_ID_2        2
```

Change the ID of the motors to match yours. 

```
#define DXL_ID          X
```

### Change resolution

Navigate to CamTrackAIClient.cpp in Client and find these lines:

```
#define SCREEN_X        1920
#define SCREEN_Y        1080
```

Change them to match your monitor. You can run 'xdpyinfo | grep dimensions' if you need to know them.

### Networking Setup

Because this application uses mDNS, you can set up your network how would like it to be. The easiest setup would be connecting the server and the client with an ethernet cable. It is important to match the subnet mask and the IP addresses of both computers. To change the IP address and the subnet mask, type the following to the command line (This is an example, you can choose the addresses freely):

```
$sudo ifconfig eth0 192.168.1.1 netmask 255.255.255.0
```

## Compile and Run

Open the console and move to the makefile's location. First:

```
$make
```

Run the following command (on Raspberry Pi) to start the server:

```
$./CamTrackAIServer
```
Then run the next command (on external Computer) to start the client:

```
$./CamTrackAICLient
```

## Further information

To completely remove OpenCV, type the following line into console:

```
$find . -type d -name "*opencv*" -prune -exec rm -rf {} \;
```

## Known Issues (Unsolved)

* There are issues communicating with more than one Dynamixel motor. For unknown reason you cannot talk to two motors shortly after each other. There must be a short pause.
* After some time it may well be that the program stops for unknown reason.

## Authors

* **Dreamfarer** - *Project Planning/Coding/GitHub/Documentation*
* **Kay** - *Project Planning/Planning and building of construction/Documentation*
* **Luca** - *Project Planning/Documentation*

* **Jeremy Morgan** - *Provided code for reading output of a console command (getStdoutFromCommandFunc function in client.cpp)* - [Found Here](https://www.jeremymorgan.com/tutorials/c-programming/how-to-capture-the-output-of-a-linux-command-in-c/)
* **Steve Tuenkam** - *Provided idea behind sending video over TCP/IP using OpenCV* - [Found Here](https://gist.github.com/Tryptich/2a15909e384b582c51b5)

## Thanks to

Linus Torvalds for creating the best kernel, the guys over Parrot OS for the best Linux distro, the whole stackoverflow community for providing ideas and saving programmers' lives.
