/**************************************************************\
 * PSOwon. A driver for Owon Oscilloscopes                    *
 *    Peter Stallinga, 2020.                                  *
 *                                                            *
 * Based on document OWON Oscilloscope PC Guidance Manual 1.3 *
\**************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <usb.h>
#include <time.h>
#include "owonlib.h"

char *owondefaultfilename = "psowon.txt";
char *owonfilename=NULL;
struct owonInfo oinfo;
usb_dev_handle *devhandle;
int numowondevices = 0;

// in case you need to convert:
void litte2BigEndian(char *p){
	char c;
	c = *p;
	*p = *(p+3);
	*(p+3) = c;
	c = *(p+1);
	*(p+1) = *(p+2);
	*(p+2) = c;
}

void printFileInfo(struct owonInfo xinf){
  int chin;
  printf("|-------------------- FILE DATA, %s (length:%4d) ------------------\n", xinf.idn, xinf.memorysize);
  printf("| start address: 0x%p\n", xinf.startaddress);
  printf("| file description: %s\n", xinf.idn);
  printf("| file length: %d bytes\n", xinf.memorysize);
  printf("| device name: %s\n", xinf.devicename);
  printf("| number of channels read: %d\n", oinfo.nchannels);
  for (chin=0; chin<oinfo.nchannels; chin++)
    printf("|   CH%d at address: 0x%p\n", chin+1, xinf.channels[chin].headeraddress);	
  printf("|-----------------------------------------------------------------------\n\n");
}

void printChannelInfo(struct channelInfo chinfo){
   	printf("\n|-------------------- CHANNEL DATA, %s (length:%4d) ------------------\n", chinfo.channelname,chinfo.blocklength+3);
    printf("| total memory size: %d bytes\n", chinfo.blocklength+3);
    printf("| total memory size: %d bytes\n", chinfo.memorysize);
    printf("| memory address: 0x%p\n", chinfo.memoryaddress);
    printf("| header address: 0x%p\n", chinfo.headeraddress);
    printf("| data address  : 0x%p\n", chinfo.dataaddress);
   	printf("|------ HEADER (length:%2d) ---------------------------------------------\n",
	   (int) ((char *)chinfo.dataaddress-((char *) chinfo.headeraddress)));
    printf("| channel name: %s\n", chinfo.channelname);
    printf("| data block length: %d bytes\n", (int) chinfo.blocklength);
    if(chinfo.extradatavalid){
      if ((chinfo.extendedflag&1)==1)
        { printf("|   deep memory wave\n");}
      else
        { printf("|   normal wave\n"); }
      if ((chinfo.extendedflag&2)==2)
        { printf("|   deep memory wave available\n"); }
      else
        { printf("|   deep memory wave not available\n"); }
    }
   	printf("| wholescreencollectingpoints: %d\n", (int) chinfo.wholescreencollectingpoints);
    printf("| samplecount2: %d\n", (int) chinfo.numberofcollectingpoints);
   	printf("| slowmovingnumber: %d\n", (int) chinfo.slowmovingnumber);
    printf("| timebase level: %d\n", (int) chinfo.timebaselevel);
   	printf("| zeropoint: %d\n", (int) chinfo.zeropoint);
   	printf("| vert scale level: %d\n", (int) chinfo.voltagelevel);
   	printf("| attenmultpowrindex: 0x%08x (%d)\n", (int) chinfo.attenmultpowrindex, (int) chinfo.attenmultpowrindex);
   	printf("| spacinginterval: 0x%08x (%d)\n", (int) chinfo.spacinginterval, (int) chinfo.spacinginterval);
   	printf("| frequency: %d Hz\n", (int) chinfo.frequency);
    printf("| cycle: 0x%08x (%d)\n", (int) chinfo.cycle, (int) chinfo.cycle);
    printf("| voltvalueperpoint: 0x%08x (%d)\n", (int) chinfo.voltvalueperpoint, (int) chinfo.voltvalueperpoint);
   	printf("|------ DATA (length:%4d) ---------------------------------------------\n",2*chinfo.numberofcollectingpoints);
    printf("| samples: %d. Total of 2*%d = %d bytes\n", chinfo.numberofcollectingpoints,
	   chinfo.numberofcollectingpoints, chinfo.numberofcollectingpoints*2);		
   	printf("|--------- CALCULATED VALUES -------------------------------------------\n");
   	printf("| frequency: %e GS/s\n", chinfo.frequency/1e9);
   	printf("| time base: %e s\n", chinfo.timeBase);
   	printf("| vertical scale: %e V\n", chinfo.vertScale);
   	printf("|-----------------------------------------------------------------------\n");
}

void decodeChannelHeader(char *xbuffer) {
	struct channelInfo chinfo;
	
	chinfo.memoryaddress=oinfo.channels[oinfo.nchannels].memoryaddress;
	chinfo.headeraddress=oinfo.channels[oinfo.nchannels].headeraddress;
	chinfo.memorysize=oinfo.channels[oinfo.nchannels].memorysize;
	memcpy(&chinfo.channelname,xbuffer,3);
	chinfo.channelname[3]='\0';
	xbuffer+=3;	
	memcpy(&chinfo.blocklength, xbuffer,4);
	if (chinfo.blocklength<0) // deep memory wave
	  {
		chinfo.blocklength=-chinfo.blocklength;
		chinfo.extradatavalid=1;
	  }
	else
		chinfo.extradatavalid=0;
    xbuffer+=4;
   	memcpy(&chinfo.extendedflag, xbuffer, 4);
    xbuffer+=4;
   	memcpy(&chinfo.offset, xbuffer, 4);
    xbuffer+=4;
	memcpy(&chinfo.wholescreencollectingpoints, xbuffer, 4);
    xbuffer+=4;	
	memcpy(&chinfo.numberofcollectingpoints, xbuffer, 4);
    xbuffer+=4;	
	memcpy(&chinfo.slowmovingnumber, xbuffer, 4);
    xbuffer+=4;	
	memcpy(&chinfo.timebaselevel, xbuffer, 4);
    xbuffer+=4;
	switch (chinfo.timebaselevel){
		case -2: chinfo.timeBase=1.0E-9; break;
		case -1: chinfo.timeBase=2.0E-9; break;
		case  0: chinfo.timeBase=5.0E-9; break;
		case  1: chinfo.timeBase=1.0E-8; break;
		case  2: chinfo.timeBase=2.5E-8; break;
		case  3: chinfo.timeBase=5.0E-8; break;
		case  4: chinfo.timeBase=1.0E-7; break;
		case  5: chinfo.timeBase=2.5E-7; break;
		case  6: chinfo.timeBase=5.0E-7; break;
		case  7: chinfo.timeBase=1.0E-6; break;
		case  8: chinfo.timeBase=2.5E-6; break;
		case  9: chinfo.timeBase=5.0E-6; break;
		case 10: chinfo.timeBase=1.0E-5; break;
		case 11: chinfo.timeBase=2.5E-5; break;
		case 12: chinfo.timeBase=5.0E-5; break;
		case 13: chinfo.timeBase=1.0E-4; break;
		case 14: chinfo.timeBase=2.5E-4; break;
		case 15: chinfo.timeBase=5.0E-4; break;
		case 16: chinfo.timeBase=1.0E-3; break;
		case 17: chinfo.timeBase=2.5E-3; break;
		case 18: chinfo.timeBase=5.0E-3; break;
		case 19: chinfo.timeBase=1.0E-2; break;
		case 20: chinfo.timeBase=2.5E-2; break;
		case 21: chinfo.timeBase=5.0E-2; break;
		case 22: chinfo.timeBase=1.0E-1; break;
		case 23: chinfo.timeBase=2.5E-1; break;
		case 24: chinfo.timeBase=5.0E-1; break;
		case 25: chinfo.timeBase=1.0; break;
		case 26: chinfo.timeBase=2.5; break;
		case 27: chinfo.timeBase=5.0; break;
		case 28: chinfo.timeBase=10.0; break;
		case 29: chinfo.timeBase=25.0; break;
		case 30: chinfo.timeBase=50.0; break;
		case 31: chinfo.timeBase=100.0; break;
	}		
	memcpy(&chinfo.zeropoint, xbuffer, 4);
    xbuffer+=4;	
	memcpy(&chinfo.voltagelevel, xbuffer, 4);
    xbuffer+=4;
	switch (chinfo.voltagelevel){
		case  0: chinfo.vertScale=2E-3; break;
		case  1: chinfo.vertScale=5E-3; break;
		case  2: chinfo.vertScale=10E-3; break;
		case  3: chinfo.vertScale=20E-3; break;
		case  4: chinfo.vertScale=50E-3; break;
		case  5: chinfo.vertScale=100E-3; break;
		case  6: chinfo.vertScale=200E-3; break;
		case  7: chinfo.vertScale=500E-3; break;
		case  8: chinfo.vertScale=1.0; break;
		case  9: chinfo.vertScale=2.0; break;
		case 10: chinfo.vertScale=5.0; break;
		case 11: chinfo.vertScale=10.0; break;
		case 12: chinfo.vertScale=20.0; break;
		case 13: chinfo.vertScale=50.0; break;
		case 14: chinfo.vertScale=100.0; break;
		case 15: chinfo.vertScale=200.0; break;
		case 16: chinfo.vertScale=500.0; break;
		case 17: chinfo.vertScale=1000.0; break;
		case 18: chinfo.vertScale=2000.0; break;
		case 19: chinfo.vertScale=5000.0; break;
		case 20: chinfo.vertScale=1E4; break;
	}	
	memcpy(&chinfo.attenmultpowrindex, xbuffer, 4);
    xbuffer+=4;	
	memcpy(&chinfo.spacinginterval, xbuffer, 4);
    xbuffer+=4;	
	memcpy(&chinfo.frequency, xbuffer, 4);
    xbuffer+=4;	
	memcpy(&chinfo.cycle, xbuffer, 4);
    xbuffer+=4;	
	memcpy(&chinfo.voltvalueperpoint, xbuffer, 4);
    xbuffer+=4;	
    chinfo.dataaddress = (short int*) xbuffer;	
	
	oinfo.channels[oinfo.nchannels] = chinfo;
}

int pscmp(char *s, char *t){
	int i=0;
	int n=strlen(t);
	
	while ((*s==*t) && (i<n)) {i++; s++; t++;}
	if (i<n) return (0);
	else return(1);
}

void decodeFileHeader(char *xbuffer, int sizebuf){
	// only at first channel data!
	time_t timestamp;
	
    //determine from the header whether this is bitmap data or vectorgram
    // is it a 'BM' (bitmap) ?
    if(*xbuffer=='B' &&  *(xbuffer+1)=='M'){
        printf("..Found 640x480 bitmap of %04xh (%d) bytes\n", (int) *(xbuffer+2), *(xbuffer+2));
	}
    // is it a vectorgram ('SPB') ?   If so, we decode the contents..
    else if(*xbuffer=='S' &&  *(xbuffer+1)=='P' && *(xbuffer+2)=='B') {
        if (debug) printf("..Found vector data:\n");
 		memcpy(&oinfo.idn, xbuffer, 6);
		oinfo.idn[6]='\0';
		if (debug) printf("    File description: %s\n", oinfo.idn);
		if (vgramheaderlength<20){
		  if (pscmp(xbuffer, "SPBW01")) strcpy(oinfo.devicename, "PDS6062x");
		  else if (pscmp(xbuffer, "SPBW11")) strcpy(oinfo.devicename, "HDS2062M");
		  else if (pscmp(xbuffer, "SPBW10")) strcpy(oinfo.devicename, "HDS2062N");
		  else if (pscmp(xbuffer, "SPBV01")) strcpy(oinfo.devicename, "PDS5022S");
		  else if (pscmp(xbuffer, "SPBV10")) strcpy(oinfo.devicename, "HDS1022N");
		  else if (pscmp(xbuffer, "SPBV11")) strcpy(oinfo.devicename, "HDS1022M");
		  else if (pscmp(xbuffer, "SPBV12")) strcpy(oinfo.devicename, "HDS1021M");
		  else if (pscmp(xbuffer, "SPBX01")) strcpy(oinfo.devicename, "MSO7102");
		  else if (pscmp(xbuffer, "SPBX10")) strcpy(oinfo.devicename, "HDS3102N");
		  else if (pscmp(xbuffer, "SPBM01")) strcpy(oinfo.devicename, "MSO8202");
		  else if (pscmp(xbuffer, "SPBS01")) strcpy(oinfo.devicename, "SDS6062");
		  else if (pscmp(xbuffer, "SPBS02")) strcpy(oinfo.devicename, "SDS7102");
		  else if (pscmp(xbuffer, "SPBS03")) strcpy(oinfo.devicename, "SDS8202");
		  else if (pscmp(xbuffer, "SPBS04")) strcpy(oinfo.devicename, "SDS9302");
		  else strcpy(oinfo.devicename, "unknown");
 		  if (debug) printf("    Device name: Owon %s\n", oinfo.devicename);
		}
		xbuffer+=6;
		oinfo.memorysize=sizebuf;
		if (debug) printf("    File length: %d bytes\n", oinfo.memorysize);
		xbuffer+=13;
		if (vgramheaderlength>19){
 		  memcpy(&oinfo.devicename, xbuffer, 7);
		  oinfo.devicename[7]='\0';
		  if (debug) printf("    Device name: Owon %s\n", oinfo.devicename);
		}
			
		//xinfo.channels[nchannels].memoryaddress = xbuffer;
    }
    // is it neither a BM (bitmap) nor a SPB (vectorgram) ?
    else {
    	printf("ERROR: Failed to determine data type.\n");
		printf("%c %c %c %c\n", *xbuffer, *(xbuffer+1), *(xbuffer+2), *(xbuffer+3));
    }
	timestamp = time(NULL);
    strftime(oinfo.timestring, 21, "%d/%h/%Y %H:%M:%S", localtime(&timestamp));
    if (debug){
		printf("..Time stamp. Seconds since 1 January 1970: %ld\n", timestamp);
		printf("..Time: %s\n", oinfo.timestring);
	}
}

// add the found Owon device to an array.
void found_usb_lock(struct usb_device *dev) {
  if(numowondevices < MAX_OWON_DEVICES) {
    owon_devices[numowondevices++] = dev;
  }
}

int findOwons() {
	  struct usb_bus *bus;
	  struct usb_dev_handle *dh;
   	  struct usb_device *dev;
	  int ret=0;
	  
	  usb_find_busses();
	  usb_find_devices();

	  for (bus = usb_busses; bus; bus = bus->next) {
	    for (dev = bus->devices; dev; dev = dev->next)
	      if(dev->descriptor.idVendor == USB_LOCK_VENDOR && dev->descriptor.idProduct == USB_LOCK_PRODUCT) {
	        found_usb_lock(dev);
	        if (debug) printf("....Found an Owon device %04x:%04x on bus %s\n", USB_LOCK_VENDOR,USB_LOCK_PRODUCT, bus->dirname);
	    	if (debug) printf("....Resetting device.");
	    	dh=usb_open(dev);
			usb_reset(dh);
	    	if (debug) printf("OK\n");
	    	ret++;
	      }
	  }
      if (debug) printf("..Found a total of %d Owon devices\n", ret);	  
	  return (ret); // retrun number of Owons found
}

void writeRawData(char *xbuffer, int count, char *fname) {
	FILE *fp;

    if (debug) printf("..Opening file \'%s\' for raw data output\n", fname);
	if ((fp=fopen(fname,"w")) == NULL) {
	  printf("ERROR: Failed to open file \'%s\'!\n", fname);
	  return;
	}
	else
      { if (debug) printf("....Successfully opened file \'%s\'\n", fname); }

	if (fwrite(xbuffer,sizeof(unsigned char), count, fp) != count)
	  printf("ERROR: Failed to write %d bytes to file %s\n", count, fname);
	else
	  { if (debug) printf("....Successfully written %d bytes to file %s\n", count, fname);}

	fclose(fp);
	return;
}

void cleanString(char *starts){	
	char *p;
	char *q;
	
	p = strstr(starts, "e");  // strip redundant 0 and . before e
	if (p) {
		q=p;
		while ( (q>starts+1) && ((*(q-1)=='0') || (*(q-1)=='.')))
		  q--;
		do 
		  *q++ = *p;
		while (*p++!=0);
	}
	p = strstr(starts, "e+00"); // can cut redundant exponent
	if ((p) && ((*(p+4)==0)||(*(p+4)=='0'))) *p=0; // should not cut e+005 but can cut e+000
redundantzero:	// e+05 can be written as e+5, e+005 as e+5, etc.
	p = strstr(starts, "e-0");
	if (!p) p = strstr(starts, "e+0");
	if (p) {
		p+=2;  // point to '0'
		q=p+1;
		do
		  *p++ = *q;
		while (*q++!=0);
		goto redundantzero;
	}
}

void saveData(FILE *ff){
	int ichan, j;
	char s[255];
	short int *ip[MAX_CHANNELS];
	
	for (ichan=0; ichan<oinfo.nchannels; ichan++)
	  ip[ichan] = oinfo.channels[ichan].dataaddress;
	for (j=0; j<oinfo.channels[0].numberofcollectingpoints; j++){
		sprintf(s, "%e", ((double) j)*oinfo.channels[0].timeBase/500.0);
		cleanString(s);
		fprintf(ff, "%s", s);
		for (ichan=0; ichan<oinfo.nchannels; ichan++){
		  sprintf(s, "%e", (*ip[ichan])*oinfo.channels[ichan].vertScale/25.0);
		  cleanString(s);
		  fprintf(ff, " %s", s);
		  ip[ichan]++;
		}  
		fprintf(ff, "\n");
	}	
}

void saveDataASCII(char *fname) {
	int ichan;
	FILE *fout;
	fout = fopen(fname, "w");
	fprintf(fout, "%% Owon %s data file. (Computer time: %s)\n", oinfo.devicename, oinfo.timestring);
	fprintf(fout, "%% Time (s)");
	for (ichan=0; ichan<oinfo.nchannels; ichan++)
	  fprintf(fout, ", %s (V)", oinfo.channels[ichan].channelname);	  
	fprintf(fout, "\n");
	saveData(fout);
	fclose(fout);	
}

void saveDataMatlab(char *fname){
	int ichan;
	FILE *fout;
	fout = fopen(fname, "w");
	fprintf(fout, "%% Owon %s data file. PjotrSoft v28-JUL-2020\n", oinfo.devicename);
	fprintf(fout, "%% Date and time: %s\n", oinfo.timestring);
	fprintf(fout, "%% Time (s)");
	for (ichan=0; ichan<oinfo.nchannels; ichan++)
	  fprintf(fout, ", %s (V)", oinfo.channels[ichan].channelname);
	fprintf(fout, "\n");
	fprintf(fout, "k=[\n");
	saveData(fout);
    fprintf(fout, "];\n");	  
	fprintf(fout, "t=k(:,[1]);\n");
	for (ichan=0; ichan<oinfo.nchannels; ichan++)
  	  fprintf(fout, "ch%d=k(:,[%d]);\n", ichan, ichan+2);
    fprintf(fout, "plot(t,ch0);\n");
    fprintf(fout, "xlabel('Time (s)');\n");
    fprintf(fout, "ylabel('Voltage (V)');\n");
	fclose(fout);	
}

int owonCommand(char *cmd){
    int ret=0;
	
	    // clear any halt status on the bulk OUT endpoint
	ret = usb_clear_halt(devhandle, BULK_WRITE_ENDPOINT);
	if (debug) printf("..Attempting to bulk write %s command to device...\n",cmd);
	ret = usb_bulk_write(devhandle, BULK_WRITE_ENDPOINT, cmd,
			strlen(cmd), DEFAULT_TIMEOUT);
	if(ret < 0) {
	  printf("ERROR: Failed to bulk write %04x '%s'\n", ret, strerror(-ret));
	  return(ret);
	}
	if (debug) printf("....Successful bulk write of 0x%04x bytes!\n", (unsigned int) strlen(cmd));	
    return(0);
}

int openCommunication(struct usb_device *dev){
  signed int ret=0;	// set to < 0 to indicate USB errors
  char owondescriptorbuffer[0x12];

	if(dev->descriptor.idVendor != USB_LOCK_VENDOR || dev->descriptor.idProduct != USB_LOCK_PRODUCT) {
	  printf("ERROR: Failed device lock attempt: not passed a USB device handle!\n");
	  return(-1);
	}

	if (debug) printf("..Attempting USB lock on device  %04x:%04x.",
			dev->descriptor.idVendor, dev->descriptor.idProduct);
	devhandle = usb_open(dev);
	if(devhandle > 0) {
      if (debug) printf("..Attempting to set device to default configuration.");
	  ret = usb_set_configuration(devhandle, DEFAULT_CONFIGURATION);
	  if(ret) {
		 if (debug) printf("\n"); 
		 printf("ERROR: Failed to set default configuration %d '%s'\n", ret, strerror(-ret));
		 return(ret);
	  }
	  else 
	     { if (debug) printf(" OK\n");}

      if (debug) printf("..Attempting to claim interface 0 of %04x:%04x and\n",
			dev->descriptor.idVendor, dev->descriptor.idProduct);
	  ret = usb_claim_interface(devhandle, DEFAULT_INTERFACE);
	  ret += usb_clear_halt(devhandle, BULK_READ_ENDPOINT);
	  ret += usb_set_altinterface(devhandle, DEFAULT_INTERFACE);
	  if(ret) {
        printf("ERROR: Failed to claim interface %d: %d : \'%s\'\n", DEFAULT_INTERFACE, ret, strerror(-ret));
	    return(ret);
      }
      else
    	{ if (debug) printf("....Successfully claimed interface 0 to %04x:%04x \n",
		dev->descriptor.idVendor, dev->descriptor.idProduct);}
	}
	else {
	  printf("ERROR: Failed to open device..\'%s\'", strerror(-ret));
	  return(-1);
	}

  	if (debug) printf("..Attempting to get the Device Descriptor\n");
  	ret = usb_get_descriptor(devhandle, USB_DT_DEVICE, 0x00, owondescriptorbuffer, 0x12);
  	if(ret < 0) {
	  printf("ERROR: Failed to get device descriptor %04x '%s'\n", ret, strerror(-ret));
	  return(ret);
	}
	else
	  { if (debug) printf("....Successfully obtained device descriptor!\n"); }

	return(0);  // no error
} 

int determineVectorgramHeaderLength(char *xbuf){
	char *searchstring = "CH";
	int i=0, m=0;
	
	while ((m<2)&&(i<100))
	  if (searchstring[m]==xbuf[i++]) m++;
	if (m==2) return(i-2); else return(0);
}

int closeCommunication(){
	int ret=0;
	
    if (debug) printf("..Attempting to release interface %d\n", DEFAULT_INTERFACE);
    ret = usb_release_interface(devhandle, DEFAULT_INTERFACE);
    if(ret) {
      printf("ERROR: Failed to release interface %d: '%s'\n", DEFAULT_INTERFACE, strerror(-ret));
	  return(ret);
    }
	if (debug) printf("....Successful release of interface %d!\n", DEFAULT_INTERFACE);

	usb_reset(devhandle);
	usb_close(devhandle);
	return(0);
}

void owonReadMemory(struct usb_device *dev) {
	signed int ret=0;	// set to < 0 to indicate USB errors
	int i=0, j=0;
	char *channelptr;	 // points to the start of a channel
	int owonflag;
    unsigned int owondatabuffersize=0;
    char responseheader[RESPONSE_START_LENGTH];  // 12-byte reply from Owon
    char *owondatabuffer;// malloc at runtime

    if (debug) printf("Entering readOwonMemory:\n");
    if (owonCommand(OWON_START_DATA_CMD)){
	  printf("ERROR: Failed write comamnd %s\n", OWON_START_DATA_CMD);
	  return;
	}
	
	// clear any halt status on the bulk IN endpoint
	ret = usb_clear_halt(devhandle, BULK_READ_ENDPOINT);
	
    oinfo.nchannels=0;
	
readnextchannel:
	if (debug) printf("..Attempting to read response header 0x%04x (%d) bytes from device...\n",
	  (unsigned int) RESPONSE_START_LENGTH, (unsigned int)  RESPONSE_START_LENGTH);
	ret = usb_bulk_read(devhandle, BULK_READ_ENDPOINT, responseheader,
			RESPONSE_START_LENGTH, DEFAULT_TIMEOUT);
	if(ret<0) {
		usb_resetep(devhandle,BULK_READ_ENDPOINT);
		printf("ERROR: Failed to read: 0x%04x (%d) bytes: '%s'\n",
		  (unsigned int) RESPONSE_START_LENGTH,(unsigned int) RESPONSE_START_LENGTH, strerror(-ret));
		return;
	}
	else
	  { if (debug) printf("....Successful read of %04x (%d) bytes:\n", ret, ret);}
	  
    if(debug) {
        // display the contents of the Owon Response Buffer
	    printf("..Owon Response Header:\t0x%08x: ",0);
	    for(i=0; i<ret; i++)
           printf("%02x ", (unsigned char)responseheader[i]);
        printf("\n");
    }
    // retrieve the bulk read byte count from the Owon command buffer
    // the count is held in little endian format in the first 4 bytes of that buffer
    memcpy(&owondatabuffersize,responseheader,4); //  responseheader, the source for memcpy, is little endian
    if (debug) printf("..dataBufSize = 0x%08x (%d) bytes\n", owondatabuffersize, owondatabuffersize);
	
    if (debug) printf("..Attempting to malloc read buffer space of 0x%08x (%d) bytes\n", owondatabuffersize, owondatabuffersize);
    oinfo.channels[oinfo.nchannels].memoryaddress = malloc(owondatabuffersize);
	oinfo.channels[oinfo.nchannels].memorysize = owondatabuffersize;
	owondatabuffer = oinfo.channels[oinfo.nchannels].memoryaddress;
    if(!owondatabuffer) {
      printf("ERROR: Failed to malloc(0x%08xh)!\n", owondatabuffersize);
      return;
    }
    else
      { if (debug) printf("....Successful malloc!\n"); }
	  
	memcpy(&owonflag,responseheader+8,4); 
    if (debug) printf("..Owon response buffer flag:%d\n", owonflag);

    if (debug) printf("..Owon ready to bulk transfer %08xh (%d) bytes\n", owondatabuffersize, owondatabuffersize);

	if (debug) printf("..Attempting to bulk read %08xh (%d) bytes from device...\n", owondatabuffersize, owondatabuffersize);
	ret = usb_bulk_read(devhandle, BULK_READ_ENDPOINT, owondatabuffer,
			owondatabuffersize, DEFAULT_BITMAP_READ_TIMEOUT);
	if(ret < 0) {
	  printf("ERROR: Failed to bulk read: %xh (%d) bytes: %d - '%s'\n", owondatabuffersize, owondatabuffersize, ret, strerror(-ret));
	  usb_reset(devhandle);
	  return;
	}
	else
	  { if (debug) printf("..Successful bulk read of %08xh (%d) bytes!:\n", ret, ret);}
	  
	// Now we have the information in the buffer starting at address owondatabuffer


    oinfo.channels[oinfo.nchannels].memoryaddress = owondatabuffer;
    if (oinfo.nchannels==0){  // File header only at the FIRST channel!!!
        if (debug) { // let's take a look in memory there:
            // hexdump the first 0x40 bytes of the Owon Data Buffer
	        printf("..Hexdump of first 0x40 bytes of the Owon data buffer :\n");
            for(i=0; i<=0x03; i++) {
              printf("\t%07x0: ",i);
              for(j=0;j<0x10;j++)
    	        printf("%02x ", (unsigned char) owondatabuffer[(i*0x10)+j]);
              printf("\n");
              printf("\t        : ");
              for(j=0;j<0x10;j++)
    	        printf(" %c ", (char) owondatabuffer[(i*0x10)+j]);
              printf("\n");
            }
        }
		// look for 'CH' in data buffer to determine header length
		if (debug) printf("..Determining file header length:\n");
		vgramheaderlength = determineVectorgramHeaderLength(owondatabuffer);
		if (vgramheaderlength==0) {
			printf("..Vectogram header end ('CH') not found\n");
			return;
		}
		else if (debug) printf("....File header length = %d bytes\n", vgramheaderlength);
        // extract information about the file:
        decodeFileHeader(owondatabuffer, owondatabuffersize);
	    oinfo.startaddress=owondatabuffer;
        printFileInfo(oinfo);
	    //finfo.channels[0].memoryaddress = owondatabuffer;
	    oinfo.channels[0].headeraddress = owondatabuffer+vgramheaderlength;	
    }
    else {  // data only contains channel, without Vectorgram description header
   	    //finfo.channels[finfo.nchannels].memoryaddress = owondatabuffer;
	    oinfo.channels[oinfo.nchannels].headeraddress = owondatabuffer;	
	}
	
    // initialize the header pointer to the first header in the data
	channelptr = oinfo.channels[oinfo.nchannels].headeraddress;
	
	if (debug){	
		printf("..Owon Data Buffer info:\n");
    	printf("    owondatabuffer pointer = 0x%p\n", owondatabuffer);
    	printf("    owondatabuffersize = 0x%x (%d)\n", owondatabuffersize, owondatabuffersize);
    	printf("    VECTORGRAM_FILE_HDR_LENGTH= 0x%02x\n", vgramheaderlength);
    	printf("    channelptr = 0x%p\n", channelptr);
    	printf("    owondatabuffer+vgramheaderlength = 0x%p\n", owondatabuffer+vgramheaderlength);
    }
		
    if (debug) writeRawData(owondatabuffer, owondatabuffersize, "output.bin");
		
    if (debug) {
     // hexdump the first 0x40 bytes of channel header
 	  printf("..Hexdump of channel header:\n");
      for(i=0; i<=0x04; i++) {
    	printf("\t%07x0: ",i);
    	for(j=0;j<0x10;j++)
    	 	printf("%02x ", (unsigned char) *(channelptr+(i*0x10)+j));
    	printf("\n");
    	printf("\t ASCII  : ");
    	for(j=0;j<0x10;j++)
    	  	printf(" %c ", (char) *(channelptr+(i*0x10)+j));
    	printf("\n");
      }
    }
	
	decodeChannelHeader(channelptr);
	
   	oinfo.nchannels++;

	if (owonflag>128){
		if (debug) printf("\nOwon 3rd-int flag>128: We're still not done yet!!\n");
		goto readnextchannel;
	}	

	return;
}

void initializeOwonLib(){
	int i;
	for (i=0; i<10; i++) oinfo.channels[i].memoryaddress = NULL;
}