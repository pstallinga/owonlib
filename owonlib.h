/**************************************************************\
 * PSOwon. A driver library for Owon Oscilloscopes            *
 *    Peter Stallinga, 2020.                                  *
 *                                                            *
 * Based on document OWON Oscilloscope PC Guidance Manual 1.3 *
\**************************************************************/

#include <usb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define debug 1

#define USB_LOCK_VENDOR 0x5345        // (5345) Owon Technologies
#define USB_LOCK_PRODUCT 0x1234       // (1234) PDS Digital Oscilloscope
#define RESPONSE_START_LENGTH 12      // minimum reply 'header'
#define OWON_START_DATA_CMD "STARTBIN"
#define BULK_WRITE_ENDPOINT 0x03
#define BULK_READ_ENDPOINT 0x81
#define DEFAULT_INTERFACE 0x00
#define DEFAULT_CONFIGURATION 0x01
#define DEFAULT_TIMEOUT	500           // ms USB timeout
#define DEFAULT_BITMAP_READ_TIMEOUT 3000 // ms USB timeout for BMP
#define MAX_OWON_DEVICES 10           // max number of scopes connected
#define VECTORGRAM_BLOCK_HEADER_CHNAMELEN 3	// "CH1", "CH2", "CHA", etc.
#define MAX_CHANNELS 10               // every scope can have up to 10 channels

struct usb_device *owon_devices[MAX_OWON_DEVICES];
// usb_dev_handle *devhandle;

int vgramheaderlength;

// header of every channel of data:
struct channelInfo {	
  char channelname[4];// 3 bytes+\0
  int blocklength;
  int extendedflag;
  int offset;
  int wholescreencollectingpoints;
  int numberofcollectingpoints;
  int slowmovingnumber;
  int timebaselevel;
  int zeropoint;
  int voltagelevel;
  int attenmultpowrindex;
  int spacinginterval;
  int frequency;
  int cycle;
  int voltvalueperpoint;
// derived info (not directly in header):
  double vertScale;        // in volts
  double timeBase;         // in seconds
  int extradatavalid;
  void *memoryaddress;     // pointer to start of memory
  void *headeraddress;     //  start of header (for channel>0 same as above)
  short int *dataaddress;  //  start of data (2 bytes per data point)
  int memorysize;
};

// set of Owon data:
struct owonInfo {
  char *startaddress;  // header address
  char idn[7];         // type of file
  char devicename[9];  // model of oscilloscope
  int memorysize;
  int nchannels;
  char timestring[21];
  struct channelInfo channels[MAX_CHANNELS];
};

// externally visible variables:
extern char *owonfilename;
extern char *owondefaultfilename;
extern struct owonInfo oinfo;
extern usb_dev_handle *devhandle;
extern int numowondevices;

// externally visible functions:
extern void printFileInfo(struct owonInfo xinf);
extern void printChannelInfo(struct channelInfo chinfo);
extern int findOwons();
extern void saveDataASCII(char *fname);
extern void saveDataMatlab(char *fname);
extern int owonCommand(char *cmd);
extern int openCommunication(struct usb_device *dev);
extern int closeCommunication();
extern void owonReadMemory(struct usb_device *dev);
extern void initializeOwonLib();