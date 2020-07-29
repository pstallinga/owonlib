# owonlib
C library for Owon oscilloscopes USB Linux

Communicating through USB to Owon Oscilloscopes. Tested with two different SDS7102 models.

It needs the libusb-dev library installed on your system to have access to the usb library

I used CodeLite for writing and debugging. Make sure you add "-lusb" to your CodeLite project Linker Options. If the #define debug is set nonzero, it will output debugging information (see file 'debug.txt'). Otherwise it will only output what the main program requests.

If it cannot communicate with the device because of permissions, put these two lines in /etc/udev/rules.d/10owon.rules:<br>
SUBSYSTEMS=="usb", ATTRS{idVendor}=="5345", MODE="0666"<br>
SUBSYSTEMS=="usb_device", ATTRS{idVendor}=="5345", MODE="0666"

Peter Stallinga, peter.stallinga at gmail
