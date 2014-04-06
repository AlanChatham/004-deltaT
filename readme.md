004 - A Brief History of Arms in America

This project uses our usual KinectOSC skeleton tracking array 
and our Cinder 3D head tracking code. It makes colorful ribbons
that trace the path of the viewer's forearms as they move by the space.

It assumes two projectors, projecting images on screens that are
put at 90 degrees to each other. The spacing of the Kinects is 
also hard-coded for now, although it should be compatible with 
the latest of our versions of Kinect-OSC, which provides for 
dynamic position tweaking of the sensors, but that will be 
a little less 'out-of-the-box' than this.

In order to compile this, you'll need to download Cinder
http://libcinder.org/releases/cinder_0.8.5_vc2012.zip
and unzip that into the 'PutCinderHere' folder

You'll also need to be running Visual Studio 2012
(NOT VS 2013!!!), but the Express (free) version is fine,
so I'd go download that if I were you.