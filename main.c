#include "owonlib.h"

int main(int argc, char *argv[]) {
  int iowon=0;
  int i;
  char fn[20];
  initializeOwonLib();
  
  if (argc>1) {
    owonfilename = malloc(strlen(argv[1]));
    memcpy(owonfilename, argv[1], strlen(argv[1]));
  }
  else owonfilename = owondefaultfilename;

  if(findOwons()>0) {
    for(iowon=0; iowon < numowondevices; iowon++){
      if (!openCommunication(owon_devices[iowon])){ 
        owonReadMemory(owon_devices[iowon]);
        printFileInfo(oinfo);
        for (i=0; i<oinfo.nchannels; i++)
          printChannelInfo(oinfo.channels[i]);
        sprintf(fn, "psowon%d.asc", iowon);
        saveDataASCII(fn);
        sprintf(fn, "psowon%d.m", iowon);
        saveDataMatlab(fn);
        if (debug) printf("Freeing memory:\n");
        for (i=oinfo.nchannels-1; i>=0; i--){
          if (debug) printf("--block %d, size %d bytes at %p.\n", i,
                oinfo.channels[i].memorysize, oinfo.channels[i].memoryaddress);
          free(oinfo.channels[i].memoryaddress);		
        }
        closeCommunication();
      }
    }
  }	
  
  printf("\nHave a nice day!\n");
  if (!owonfilename) free(owonfilename);
  
  return 0;
}
