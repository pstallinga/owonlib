# owonlib
C library for Owon oscilloscopes USB Linux

Communicating through USB to Owon Oscilloscopes. Tested with two different SDS7102 models.

It needs the libusb-dev library installed on your system to have access to the usb library

I used CodeLite for writing and debugging. Make sure you add "-lusb" to your CodeLite project Linker Options. If the #define debug is set nonzero, it will output debugging information (see file 'debug.txt'). Otherwise it will only output what the main program requests.

Put this line in a file '70owon.rules' in either '/etc/udev/rules.d/' or '/lib/udev/rules.d/':<br>
SUBSYSTEMS=="usb", ATTRS{idVendor}=="5345", ATTRS{idProduct}=="1234", MODE="0666"

Peter Stallinga, peter.stallinga at gmail
