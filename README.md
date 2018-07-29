## Instructions

1. install <code>mpg123</code> on target by running <code>apt-get install mpg123</code>

2. copy <code>libmpg123.so.0</code> on target to directory <code>asound_lib_BBB</code>, then rename it to <code>libmpg123.so</code>

    - first <code>cd /usr/lib/arm-linux-gnueabihf/</code>

    - then <code>cp libmpg123.so.0 /mnt/remote/asound_lib_BBB/libmpg123.so</code>

    - if <code>libmpg123.so.0</code> is not in there, run <code>ldconfig -p | grep libmpg123.so.0</code> to find the file path

3. on target, navigate to project root directory, run <code>make</code>

    - if you get this error: <code>"fatal error: mpg123.h no such file or directory"</code>, run <code>sudo apt-get install libmpg123-dev</code>

## Issues:

1. you need to wait a long time to play <code>mp3</code> files, because the <code>mp3</code> needs to get converted to <code>pcm</code> first in order to load the bytes to memory. 

    - a solution would be to decode <code>mp3</code> files in the background while playing other songs.

## Note:

I am doing it this way because it plays nicely with the structure we have now; it can pause, run, switch songs (not when it's decoding tho), etc. If I were to use other libraries to play the song, then I need to implement the whole structure again just for <code>mp3</code>, and most importantly, I need to fight with <code>pcm</code> handler to get the permission of using the default sound output device.