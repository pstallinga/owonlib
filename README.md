# owonlib
C library for Owon oscilloscopes USB Linux

Communicating through USB to Owon Oscilloscopes. Tested with two different SDS7102 models.

It needs the libusb-dev library installed on your system to have access to the usb library

I used CodeLite for writing and debugging. Make sure you add "-lusb" to your project Linker Options. If the #define debug is set nonzero, it will output debugging information (see fle debug.txt). Otherwise it will only output what the main program requests.

Peter Stallinga, peter.stallinga at gmail
