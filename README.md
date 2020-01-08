# CamTrackAI

This project is a work we have to do for school.

### Prerequisites

You will need these following utilities/devices:

* [Dynamixel Rx-64](http://www.robotis.us/dynamixel-rx-64-hn05-n101/)x2 - These are the motors we used. But you can use any Robots Dynamixel motor. They are used to turn the camera.
* [Robotis USB2Dynamixel Adapter](https://www.trossenrobotics.com/robotis-bioloid-usb2dynamixel.aspx) - It is used to communicate with the motors.

* [Raspberry Pi Camera Module](https://projects.raspberrypi.org/en/projects/getting-started-with-picamera) - Our camera, the Pi Cam. But any webcam will work fine.
* [Raspberry Pi 4 Model B](https://www.pishop.us/product/raspberry-pi-4-model-b-4gb/) - The computer we used. But you can use any computer with any Linux distro.


You will need the following libraries to compile this code:

* [OpenCV](https://opencv.org/releases/) - For the tracking algorithm
* [Dynamixel SDK](https://github.com/ROBOTIS-GIT/DynamixelSDK) - For controlling the dynamxiel motors
* [fltk 1.3.5](https://www.fltk.org/software.php) - For the GUI

### Installing

Open the makefile and find this line.

```
-L/home/theb3arybear/Desktop/CamTrackAI/GitHub/CamTrackAI/Libraries/fltk
```

Change it to your location.

```
-L/home/USERNAME/YOURPATH/CamTrackAI/Libraries/fltk
```

Navigate to CamTrackAI.cpp and find this line.

```
#define DXL_ID 4
```

Change the ID of the motors to match yours. Look it up how to change IDs of motors on the internet if you encounter problems.

## Compile and Run

Open the console and move to the makefile's location. First:

```
make
```

Then run the application with:

```
./CamTrackAI
```

## Built With

* [Dropwizard](http://www.dropwizard.io/1.0.2/docs/) - The web framework used
* [Maven](https://maven.apache.org/) - Dependency Management
* [ROME](https://rometools.github.io/rome/) - Used to generate RSS Feeds

## Contributing

Please read [CONTRIBUTING.md](https://gist.github.com/PurpleBooth/b24679402957c63ec426) for details on our code of conduct, and the process for submitting pull requests to us.

## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/your/project/tags).

## Authors

* **BE3dARt (Gianluca Imbiscuso)** - *Coding/Setup* - [BE3dARt.ch](https://be3dart.ch/)

See also the list of [contributors](https://github.com/your/project/contributors) who participated in this project.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details
