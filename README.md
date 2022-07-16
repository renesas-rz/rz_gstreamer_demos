# RZV and RZG GStreamer Demo

These demos show how to implement the GStreamer C API on the RZV and RZG boards. For more detailed reference to the GStreamer API [here](https://gstreamer.freedesktop.org/documentation/tutorials/?gi-language=c).

## PlayVideoMP3

This demo is c code that shows how to implement the below CLI on C Code. The Example takes advantage the the RZ Hardware peripherals ( H.264, and VSPMFilere). 

###### Plays the MP3 in original format

`gst-launch-1.0 filesrc location=./Road.mp4 ! qtdemux ! queue ! h264parse ! queue ! omxh264dec ! queue ! vspmfilter dmabuf-use=true ! waylandsink`

##### Down Scales MP3 Video to VGA

`gst-launch-1.0 filesrc location=./Road.mp4 ! qtdemux ! queue ! h264parse ! queue ! omxh264dec ! queue ! vspmfilter dmabuf-use=true ! video/x-raw, width=640, height=480 ! waylandsink`

##### Build

1- Enable the Yocto SDK 

2- Run make

3- Two binaries are created PlayMP3video and PlayMP3video_vga. The first plays the video in it;s original format. The second does the downscale of the video.

# eAIDemo

TODO

 