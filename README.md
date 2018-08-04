## Instructions


Install mpg123 on target by running 

	# apt-get install mpg123

Copy libmpg123.so.0 on target to directory asound\_lib\_BBB, then rename it to libmpg123.so

	# cd /usr/lib/arm-linux-gnueabihf/
	# cp libmpg123.so.0 /mnt/remote/asound_lib_BBB/libmpg123.so

Troubleshooting: If libmpg123.so.0 is not there, run the following to find the file path.

	# ldconfig -p | grep libmpg123.so.0 

On host, navigate to project root directory, run 

	$ make

Troubleshooting: If you get this error: 

	$ fatal error: mpg123.h no such file or directory

Then run: 

	$ sudo apt-get install libmpg123-dev


Sources Used:
https://www.mpg123.de/api/id3dump_8c_source.shtml