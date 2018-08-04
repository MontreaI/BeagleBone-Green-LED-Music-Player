## Installing mpg123

Prerequisites: complete guides up to the Audio Guide

Install mpg123 on target by running 

	# apt-get install mpg123

Copy libmpg123.so.0 on target to directory asound\_lib\_BBB, then rename it to libmpg123.so

	# cd /usr/lib/arm-linux-gnueabihf/
	# cp libmpg123.so.0 /mnt/remote/asound_lib_BBB/libmpg123.so

Troubleshooting: If libmpg123.so.0 is not there, run the following to find the file path.

	# ldconfig -p | grep libmpg123.so.0 
	

## Building the project

On the host, navigate to the project root directory. The wave-files folder is for storing music files. 

Ensure that at least one wav or mp3 file is inside wave-files and run make.

Troubleshooting: If you get this error: 

	$ fatal error: mpg123.h no such file or directory

Then run: 

	$ sudo apt-get install libmpg123-dev

## Running the project

Connect the LED matrix as described in our BBG LED matrix guide. Mount the ~/cmpt433/public folder and run the executable file: 

	# mnt/remote/myApps/cmpt433_proj/wave_player


