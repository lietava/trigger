/* 9.5.2017 write_df: commentd out line writing in gcalib.log file
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "vmewrap.h"
#include "ctp.h"
#include "ctplib.h"
#include "vmeblib.h"
#define DBMAIN
#include "Tpartition.h"
#include "lexan.h"

#ifdef CPLUSPLUS
#include <dis.hxx>
#else
#include <dis.h>
#endif

#define DBGCMDS 1
// following should be taken from VALID.LTUS
#define SDD 1
#define TOF 5
#define MTR 11
#define T00 13
#define ZDC 15
#define EMC 18
#define DAQ 19
// only for testing in lab:
#define HMPID 6
#define PHOS 7
#define CPV 8

//int GenSwtrg2(int n,char trigtype, int roc, w32 BC,w32 detectors, int customer);

int TAGcalthread=11;
int quit=0; 
int threadactive=0;
w32 heartbeat=0;
w32 last_heartbeat=0xffffffff;
/* 32bit unsigned int:
 (2**32-1)/1000000./60
71.582788250000007 -i.e. >1hour if time is kept in micsecs... */

w32 beammode=0x12345677;  // used only with T0: they do not get trigs in STABLE BEAMS

typedef struct t {
  w32 secs; w32 usecs;
} Ttime;
typedef struct ad {
  int deta;   //-1:not active, 1:active i.e. triggered/runn valid
  int periodms;      // in ms (the planning of next c. trig.). 0:NO GLOB. CALIBRATON
  int calbc;       // 3556->3011 from 31.3.2011 (or from ltu_proxy)
  int logroc;         // Log: bit 4 (0x10) Readout COntrol: (4 low bits, 0xf)
  int fileix;      // pointer to DATAFILES
  w32 runn;        // !=0 for detector not choosen for pp_N.data file
  char name[12];
  Ttime caltime;   //time of next cal. trigger. secs=0: not initialised
  Ttime lasttime;  //time of last cal. trigger. secs=0: not initialised
  w32 sent;   //number of cal. trigers sent. Cleared at the SOR
  w32 attempts; //number of cal. trigers attempts
} Tacde;

int NACTIVE;
Tacde ACTIVEDETS[NDETEC];

// DataFiles referenced solely from delDet() addDET() and calthread()
class DataFiles {
private:
typedef struct opened {
  w32 runn;
  FILE *dataf;   // opened if runn!=0
} Topened;
Topened DATAFILES[6];
int ffala;   // first free item after the last allocated one (i.e. 0..6)
int find_df(w32 runn);   // rc: 0..5, -1 if not available
public:
DataFiles();
~DataFiles();
//int open_df(w32 runn);   // rc: -1 if not available
int findoralloc_df(w32 runn);  // rc:0..5, -1: full table
void close_df(int ixdet);   // ix: 0..NDET-1. Also delete in DATAFILES table
void write_df(int ixdet, char *line);
int getffala();
};
//DataFiles::DataFiles(Tacde *det_table) {
DataFiles::DataFiles() {
for(int ix=0; ix<6; ix++) {
  DATAFILES[ix].runn=0; DATAFILES[ix].dataf=0;
};
ffala=0;
}
DataFiles::~DataFiles() {
for(int ix=0; ix<6; ix++) {
  FILE *fd;
  fd= DATAFILES[ix].dataf;
  if(fd != 0) {
    printf("WARN: closing DATAFILES[%d]\n", ix);
    fclose(fd);
  };
};
}
int DataFiles::getffala() {
return(ffala);
}

int DataFiles::find_df(w32 runn) {
int ix=0;
while(1) {
  if(ix>=ffala) return(-1);
  if(DATAFILES[ix].runn==runn) return(ix);
  ix++;
};
}
/* 
 * */
int DataFiles::findoralloc_df(w32 runn) {
int ff=-1; int ix=0;
while(1) {
  if(ix>=ffala) {   // not found, going to allocate...
    char fname[24];
    if(ff == -1) {
      if(ffala==6) return(-1);   // internal error (all entries allocated)
      ff= ffala; ffala++;
    };
    sprintf(fname,"WORK/pp_%d.data", runn);
    DATAFILES[ff].dataf= fopen(fname, "a"); // open file
    DATAFILES[ff].runn=runn;
    return(ff); 
  } else {
    if(DATAFILES[ix].runn==runn) return(ix);   // ok already opened
    if((DATAFILES[ix].runn==0) && (ff== -1)) ff= ix;   // first free (in case alloc needed)
    ix++;
  };
};
}
/*int DataFiles::open_df(w32 runn) {
}*/
void DataFiles::close_df(int ixdet) {
FILE *fd; int ix,ixin; char msg[80];
ixin= ACTIVEDETS[ixdet].fileix;
if(ixin== -1) {
  printf("close_df: det:%d pointing to closed file (-1).\n", ixdet);
  return;
};
fd= DATAFILES[ixin].dataf;
if(DATAFILES[ixin].runn == 0) {
  printf("close_df: det:%d already closed.\n", ixdet);
  return;
};
sprintf(msg, "close_df: closing det:%d", ixdet); prtLog(msg);
fclose(fd);
DATAFILES[ixin].dataf= 0;
DATAFILES[ixin].runn= 0;
ix= ffala-1;
while(1) {
  if(DATAFILES[ix].runn==0) ffala= ix;
  if(ix==0) break;
  ix--;
};
}
void DataFiles::write_df(int ixdet, char *line) {
FILE *fd; int ix;
ix= ACTIVEDETS[ixdet].fileix;
if( DATAFILES[ix].runn!=0) {
  fd= DATAFILES[ix].dataf;
  fprintf(fd, line);
} else {   // in case 'n' in gcalib.cfg file
  ; //printf("ERROR: runn=0 in write_df(%d, %s", ix, line);
};
}

DataFiles *DFS;

void gotsignal(int signum) {
char msg[100];
switch(signum) {
case SIGUSR1:  // kill -s USR1 pid
  signal(SIGUSR1, gotsignal); siginterrupt(SIGUSR1, 0);
  sprintf(msg, "got SIGUSR1 signal:%d\n", signum);
  break;
case SIGINT:
  signal(SIGINT, gotsignal); siginterrupt(SIGINT, 0);
  sprintf(msg, "got SIGINT signal, quitting...:%d\n", signum);
  quit=signum; 
  break;
case SIGQUIT:
  signal(SIGQUIT, gotsignal); siginterrupt(SIGQUIT, 0);
  sprintf(msg, "got SIGQUIT signal, quitting...:%d\n", signum);
  quit=signum; 
  break;
case SIGBUS: 
  //vmeish(); not to be called (if called, it kills dims process)
  sprintf(msg, "got SIGBUS signal:%d\n", signum);
  break; 
default:
  sprintf(msg, "got unknown signal:%d\n", signum);
};
prtLog(msg);
}

/* Operation:
- read gcalib.cfg
gcalib.cfg:
#ltuname period[ms] roc [log]
HMPID 2000 0 y
DAQ 3000 6 n
i.e. y: log all cal. request in pp_N.data file
     n: (or nothing) - do not log cal. requests
----------------------------------------*/ void read_gcalibcfg() {
FILE* gcalcfg;
enum Ttokentype token;
char line[MAXLINELENGTH], value[MAXCTPINPUTLENGTH];
char em1[200]="";
gcalcfg= openFile("gcalib.cfg","r");
if(gcalcfg==NULL) {
  prtLog("gcalib cannot be read. Using defaults");
  return;
};
while(fgets(line, MAXLINELENGTH, gcalcfg)){
  int ix,det,milsec, roc,log;
  roc=0; log=0;
  //printf("Decoding line:%s ",line);
  if(line[0]=='#') continue;
  if(line[0]=='\n') continue;
  ix=0; token= nxtoken(line, value, &ix);
  if(token==tSYMNAME) {
    char ltuname[20];
    strcpy(ltuname, value);
    det= findLTUdetnum(ltuname);
    if(det<0) {
      strcpy(em1,"bad LTU name in gcalib.cfg"); goto ERR; 
    };
    token=nxtoken(line, value, &ix);
    if(token==tINTNUM) {         // period in ms
      milsec= str2int(value);
    } else {strcpy(em1,"bad period (integer expected ms) in gcalib.cfg"); goto ERR; };
    ACTIVEDETS[det].periodms= milsec;
    token=nxtoken(line, value, &ix);
    if(token==tINTNUM) {         // roc (decimal)
      roc= str2int(value);
    } else if(token != tEOCMD) {
      strcpy(em1,"bad ROC (0-7 expected) in gcalib.cfg"); goto ERR;
    };
    token=nxtoken(line, value, &ix);
    if(token==tSYMNAME) {
      if(value[0]=='y') {
        log= 1;
      } else if(value[0]=='n') {
        log= 0;
      } else {
        sprintf(em1,"bad LOG option %s (y or n expected) in gcalib.cfg", value); goto ERR;
      }
    } else if((token != tEOCMD) && (token!=tCROSS)) {
      sprintf(em1,"bad LOG option  %s(y or n expected) in gcalib.cfg", value); goto ERR;
    };
    ACTIVEDETS[det].logroc= (log<<4) | roc;
    sprintf(em1,"gcalib.cfg:%s %dms 0x%x", ACTIVEDETS[det].name, ACTIVEDETS[det].periodms,
      ACTIVEDETS[det].logroc);
    prtLog(em1); em1[0]='\0';
  } else {strcpy(em1,"LTU name expected"); goto ERR; };
};
ERR: 
fclose(gcalcfg); if(em1[0]!='\0') prtLog(em1); return;
};

/*------------------------*/ void getcurtime(Ttime *ct) {
GetMicSec(&ct->secs, &ct->usecs); 
}
void addSecUsec(w32 *tsec,w32 *tusec,w32 plussec,w32 plususec) {
w32 usecs, tsecs; int ix;
usecs= *tusec + plususec; tsecs= *tsec + plussec;
for(ix=0; ix<4294 ; ix++) {   // (2**32-1)/1000000
  if(usecs>=1000000) {
    usecs= usecs-1000000; tsecs++;
  } else goto OK;
}; printf("Error 4294 in addSecUsec\n");
OK:*tsec= tsecs; *tusec= usecs;
}
/*------------------------*/ w32 diffnowbefore(Ttime *now, Ttime *before) {
w32 dif;
dif= DiffSecUsec(now->secs, now->usecs, before->secs, before->usecs);
return(dif);
}
/*------------------------*/ void addtime(Ttime *result, Ttime *delta) {
addSecUsec(&result->secs, &result->usecs, delta->secs, delta->usecs);
}
void printDET1(int ix) {
char active[20]; char msg[200];
if(ACTIVEDETS[ix].deta!=-1) {
  strcpy(active,"ACTIVE");
} else {
  strcpy(active,"NOT ACTIVE");
};
sprintf(msg, "%2d: %s %s. rate:%d ms. logroc:0x%x fileix:%d attempts/sent:%d/%d", 
  ix,ACTIVEDETS[ix].name, active, ACTIVEDETS[ix].periodms, 
  ACTIVEDETS[ix].logroc, ACTIVEDETS[ix].fileix,
  ACTIVEDETS[ix].attempts, ACTIVEDETS[ix].sent); 
prtLog(msg);
}
/*--------------------*/int addDET(int det, w32 runn) {
int rc=0; //OK:added or was added already. 1:not added
printf("addDET:%d %s\n",det, ACTIVEDETS[det].name);
if( ACTIVEDETS[det].periodms==0) {
  printf("addDET:WARN %d not configured for global calibration\n",det); rc=1;
} else if(ACTIVEDETS[det].deta==1) {
  printf("addDET:WARN %d already active\n",det);
} else {
  //int bc;
  ACTIVEDETS[det].deta= 1; 
  ACTIVEDETS[det].runn= runn; 
  ACTIVEDETS[det].sent= 0; ACTIVEDETS[det].attempts= 0; 
  ACTIVEDETS[det].caltime.secs= 0;   // unset
  ACTIVEDETS[det].caltime.usecs= 0;
  ACTIVEDETS[det].lasttime.secs= 0;   // unset
  if((ACTIVEDETS[det].logroc & 0x10) != 0) {
    ACTIVEDETS[det].fileix= DFS->findoralloc_df(runn);
  };
  NACTIVE++;
  if(NACTIVE>NDETEC) {
    printf("addDET:ERROR NACTIVE:%d\n",NACTIVE); rc=1;
  };
};
return(rc);
}
/*--------------------*/int delDET(int det) {
int rc=0;  //OK: deleted or was deleted
if(ACTIVEDETS[det].deta==-1) {
  if(ACTIVEDETS[det].periodms!=0) {   // do not print if not in global
    printf("delDET:WARN %d already not active\n",det);
  };
} else {
  DFS->close_df(det);
  ACTIVEDETS[det].deta=-1; NACTIVE--;
  printDET1(det);
  // check if another detector still in the same runn, if not: close pp_N.data file
  if(NACTIVE<0) {
    printf("delDET:ERROR NACTIVE:%d\n",NACTIVE); rc=1;
  };
};
return(rc);
}
/* go through SHM and add all global detectors, not yet included,i.e.
if detector is already choosen for calibration, do not replace
info in ACTIVEDETS for this detector (det's ltuproxy was contacted 
only at start!)
rc: number of active detectors
--------------------*/int shmupdateDETs() {
int ix,gdets; int actdets=0;
w32 det2runn[NDETEC];
gdets= cshmGlobalDets(det2runn);
for(ix=0; ix<NDETEC; ix++) {
  if(ACTIVEDETS[ix].periodms==0) continue;  // not to be calibrated (according to .cfg)
  if(ACTIVEDETS[ix].deta==-1) {   //not active, check if in global run
    if((gdets & (1<<ix))==(1<<ix)) {
      if(ix==T00) {   // T0
        //w32 beammode;
        //beammode= get_DIMW32("CTPDIM/BEAMMODE");cannot be used inside callback
        if(beammode== 11) { // STABLE BEAMS
          printf("updateDETS:T0 not calibrated (STABLE BEAMS)\n");
          continue;       // we do not want to calibrate T0 during  STABLE EBAMS
        };
      }
      if(addDET(ix, det2runn[ix])==0) actdets++;
    };
  } else {   // ix is active
    printf("updateDETS:active det:%d\n", ix);
    if((gdets & (1<<ix))==0) {  //but not in global partition
      int rc;
      printf("Warning: %s active but not in global, deactivating...\n",
        ACTIVEDETS[ix].name);
      rc= delDET(ix);
    } else {
      /* ix is active (in gcalib) and is in global (info from shm)
         i.e. we do not need to add it to list of active dets! */
      //if(addDET(ix)==0) actdets++;   -removed 11.4.2012
      actdets++;  // we want to return number of active (calibrated) dets
    };
  };
};
return(actdets);
}
/*---------------------------------------------*/ void emptyDET() {
int ix;
for(ix=0; ix<NDETEC; ix++) {
  ACTIVEDETS[ix].deta=-1;
};NACTIVE=0;
}
/*---------------------------------------------*/ void initDET() {
int ix,bc,det;
// 1st phase -very first defaults
for(ix=0; ix<NDETEC; ix++) {
  ACTIVEDETS[ix].deta=-1;
  ACTIVEDETS[ix].runn= 0;
  ACTIVEDETS[ix].periodms=0;   // no cal. triggers
  ACTIVEDETS[ix].logroc=0;     // no log, roc+0
  ACTIVEDETS[ix].fileix= -1;
  ACTIVEDETS[ix].calbc=3011;     // was 3556 till 31.3.2011
  if(ctpshmbase->validLTUs[ix].name[0] != '\0'){
    strcpy(ACTIVEDETS[ix].name, ctpshmbase->validLTUs[ix].name);
  } else {
    strcpy(ACTIVEDETS[ix].name,"nocalib");
  };
};NACTIVE=0;
ACTIVEDETS[SDD].periodms=0;    // todo: exception: 1 means: 3 triggs (3x50ms) spaced 15*60000ms
ACTIVEDETS[TOF].periodms=0;  // 200;
ACTIVEDETS[MTR].periodms=0; // 33000
ACTIVEDETS[T00].periodms=0;  // 1000;
ACTIVEDETS[ZDC].periodms=0;    // 10000;
ACTIVEDETS[EMC].periodms=0;  // 2000 (1000 .. 5000)
ACTIVEDETS[DAQ].periodms=0;
/*strcpy(ACTIVEDETS[SDD].name, "SDD");
strcpy(ACTIVEDETS[TOF].name, "TOF");
strcpy(ACTIVEDETS[MTR].name, "MTR");
strcpy(ACTIVEDETS[T00].name, "T0");
strcpy(ACTIVEDETS[ZDC].name, "ZDC");
strcpy(ACTIVEDETS[EMC].name, "EMC");
strcpy(ACTIVEDETS[DAQ].name, "DAQ"); */
// 2nd phase: calbc retrieved from ltuproxies (only T0 now)
det=T00;
bc= getCALIBBC(ACTIVEDETS[det].name);
if(bc>0) {
  if( ACTIVEDETS[det].calbc!= bc ) {
    printf("Detector:%s CALIBBC:%d->%d\n",ACTIVEDETS[det].name,
      ACTIVEDETS[det].calbc, bc );
      ACTIVEDETS[det].calbc= bc; 
  };
} else {
  printf("ERROR: cannot contact %s(%d) ltu proxy.CALIBBC set to:%d\n",
    ACTIVEDETS[det].name, det, ACTIVEDETS[det].calbc);
};
// 3rd phase: periodms (and roc?) from $dbctp/gcalib.cfg
read_gcalibcfg();
}  
/*---------------------------------------------*/ void printDETS() {
int ix;
printf("Calibration active for detectors:");
for(ix=0; ix<NDETEC; ix++) {
  if(ACTIVEDETS[ix].deta==1) {
    printf("%d ", ix);
  };
}; printf("\n");
for(ix=0; ix<NDETEC; ix++) {
  //if(ACTIVEDETS[ix].deta!=-1) {
  if(strcmp(ACTIVEDETS[ix].name,"nocalib")!=0) {
    printDET1(ix);
    continue;
  };
}; printf("\n"); fflush(stdout);
}
/* Find next detector (nearest in time) waiting for cal. trigger
rc: -1 (no active dets) or index to ACTIVEDETS
-----------------------------------------------*/ int findnextcDET() {
Ttime mint={0xffffffff,0xffffffff}; int ix,ixmin=-1;
for(ix=0; ix<NDETEC; ix++) {
  if(ACTIVEDETS[ix].deta==-1) continue;   //not active
  if(ACTIVEDETS[ix].caltime.secs < mint.secs) {
    mint.secs= ACTIVEDETS[ix].caltime.secs;
    mint.usecs= ACTIVEDETS[ix].caltime.usecs;
    ixmin=ix;
  } else if(ACTIVEDETS[ix].caltime.secs == mint.secs) {
    if(ACTIVEDETS[ix].caltime.usecs < mint.usecs) {
      mint.secs= ACTIVEDETS[ix].caltime.secs;
      mint.usecs= ACTIVEDETS[ix].caltime.usecs;
      ixmin=ix;
    };
  };
}; return(ixmin);
}
/*--------------------*/ int globalcalDET(int det){ 
int gdets; int rc=0;
w32 det2runn[NDETEC];
gdets= cshmGlobalDets(det2runn);
if((gdets & (1<<det))==(1<<det)) rc=1;
return(rc);
}
/*--------------------*/ void calthread(void *tag) {
Ttime ct,deltaTtime; w32 delta; int sddburst=0;
//printf("calthread:\n");
threadactive=1;
if(DBGCMDS) {
  char msg[100];
  sprintf(msg, "calthread start. DFS.ffala:%d", DFS->getffala());
  prtLog(msg);
};
while(1) {
  int ndit;
  heartbeat++; if(heartbeat>0xfffffff0) heartbeat=0;
  ndit= findnextcDET();   // find closest one in time. ndit: index into ACTIVEDETS
  //printf("det:%s %d %d\n", ACTIVEDETS[ndit].name, ACTIVEDETS[ndit].caltime.secs, ACTIVEDETS[ndit].caltime.usecs);
  if(ndit==-1) goto STP;  // cal. triggers not needed, stop thread
  if(globalcalDET(ndit)==0) {
    delDET(ndit); if(NACTIVE==0) goto STP;
    continue;
  };
  getcurtime(&ct); 
  if( ACTIVEDETS[ndit].caltime.secs>0) {
    w32 milisecs; int rcgt;
    delta=diffnowbefore(&ACTIVEDETS[ndit].caltime, &ct);
    if(delta>100) {
      usleep(delta);
    };
    if(globalcalDET(ndit)==0) {
      delDET(ndit); if(NACTIVE==0) goto STP;
      continue;
    };
    // generate calib. trigger:
    if(cshmGlobFlag(FLGignoreGCALIB)==0) {
      w32 orbitn[3];   // orbitn tsec tusec
      rcgt= GenSwtrg(1, 'c', ACTIVEDETS[ndit].logroc&0xf, ACTIVEDETS[ndit].calbc,1<<ndit, 
        swtriggers_gcalib, orbitn);
      ct.secs= orbitn[1]; ct.usecs= orbitn[2];
      if(rcgt==12345678) {
        delDET(ndit); if(NACTIVE==0) goto STP;
        continue;
      } else {
        w32 rcgtold;
        rcgtold= rcgt;
        if((rcgt & 0xffffff00)== 0xffffff00) {   // spec. case: one cal. trigger
          char line[80];
          rcgt= rcgt & 0xff;
          // update pp_N.data file: det tsec tusec orbitid rc
          sprintf(line,"%x %x %x %x %x\n", ndit, orbitn[1], orbitn[2],orbitn[0], rcgt);
          DFS->write_df(ndit, line);
        } else {
          char msg[100];
         sprintf(msg,"ERROR: got 0x%x instead of 0xffffff.., deactivating det. %d...",
            rcgtold, ndit);
          prtLog(msg);
          delDET(ndit); if(NACTIVE==0) goto STP;
          continue;
        };
      };
      ACTIVEDETS[ndit].attempts++;
    } else {
      rcgt=0; // cal. trigger not generated (disabled during start/stop part)
    };
    // getcurtime(&ct); // time of cal. trigger just attempted to be generated. Commented out 8.1.2017
    // movd up ACTIVEDETS[ndit].attempts++;
    if(rcgt==4) {    // 4:l2a OK, generated
      if(ACTIVEDETS[ndit].lasttime.secs==0) {
        milisecs=0;
      } else {
        milisecs= diffnowbefore(&ct, &ACTIVEDETS[ndit].lasttime)/1000;
      };
      ACTIVEDETS[ndit].sent++;
      //printf("CT:%s \t %d\t%d ms:%d\n", ACTIVEDETS[ndit].name, ct.secs, ct.usecs, milisecs);
      //printf("CT:%s ms:%d\n", ACTIVEDETS[ndit].name, milisecs);
      ACTIVEDETS[ndit].lasttime.secs= ct.secs;
      ACTIVEDETS[ndit].lasttime.usecs= ct.usecs;
    };
    // usleep(100);  // do we need this (waiting for the completion)?
    /* if(ACTIVEDETS[ndit].attempts > 200 ) {
      emptyDET(); goto STP;
    }; */
  };
  // the planning of next cal. trigger for ndit detector:
  // NextTime= CurrentTime + periodms[ndit] i.e.
  // IS THE SAME IN EITHER CASE (successful or unsuccsessful)
  if( ACTIVEDETS[ndit].caltime.secs==0) {   // wait 2 secs before 1st cal. event
    char msg[200];
    sprintf(msg, "1st planning:%s \t %d\t%d", ACTIVEDETS[ndit].name, ct.secs, ct.usecs);
    prtLog(msg);
    deltaTtime.secs=2; deltaTtime.usecs=0;
  } else {
    deltaTtime.secs=0; deltaTtime.usecs=1000*ACTIVEDETS[ndit].periodms;
  };
  if(ndit==SDD) {   // SDD 3x50ms every 15minutes
    sddburst++;
    if(sddburst==3) {
      sddburst=0;
      deltaTtime.usecs= 15*60*1000000;
    };
  };
  addtime(&ct, &deltaTtime);
  ACTIVEDETS[ndit].caltime.secs= ct.secs;
  ACTIVEDETS[ndit].caltime.usecs= ct.usecs;
  //printf("det:%s next:%d %d\n", ACTIVEDETS[ndit].name, ct.secs,ct.usecs); 
};
STP:
threadactive=0;
if(DBGCMDS) {
  prtLog("calthread stop.");
};
}
void startThread() {
dim_start_thread(calthread, (void *)&TAGcalthread);
usleep(100000);
if(threadactive==0) {
  char msg[200];
  sprintf(msg,"thread not started! exiting..."); prtLog(msg);
  //exit(8);
  quit=8;
};
/*
if(detectfile("gcalrestart", 0) >=0) {   //debug: simulate error by file presence
  // i.e.: echo blabla >$VMEWORKDIR/gcalrestart
  char msg[200];
  //system("gcalibrestart_at.sh >/tmp/gcalibresatrt_at.log");
  system("rm gcalrestart");
  sprintf(msg,"gcalrestart removed, registered, exiting..."); prtLog(msg);
  //exit(8);
  quit=8; 
}else {
  char msg[200];
  //system("gcalibrestart_at.sh");
  sprintf(msg,"WORK/../gcalrestart not present (i.e. real crash, not simulated)"); prtLog(msg);
};
*/
}

/*--------------------*/ void DOcmd(void *tag, void *msgv, int *size)  {
/* msg: if string finished by "\n\0" remove \n */
char *mymsg= (char *)msgv; int msglen=100;
char msg[101];
enum Ttokentype token; int ix; char value[100]; char em1[200];

if(DBGCMDS) {
  char logmsg[200];
  sprintf(logmsg, "DOcmd1: tag:%d size:%d mymsg:%s<-endofmsg", *(int *)tag, *size,mymsg);
  //prtLog(logmsg);
};
if(*size <msglen) msglen=*size;
strncpy(msg, mymsg, msglen); msg[msglen]='\0';
/*if(msg[*size-2]=='\n') {
  msg[*size-2]='\0';
} else {
  msg[*size-1]='\0';
}; */
/*msg: 
u         -update from SHM (this cmd is issued by ctp_proxy at
           Start, Resume
          Following (a,d) cmnds used only from cmdline by admin when debugging
a 0 2 5   -add detectors for calibration (valid only for active run)
d 0 5     -delete detectors from calibration 
*/
ix=0; token= nxtoken(msg, value, &ix);
if(token==tSYMNAME) {
  if((strcmp(value,"u")==0) ) {   // update from global runs (in shm)
    int ads;
    ads= shmupdateDETs();
    if(ads>0){
      printf("Docmd: starting thread, # of dets:%d threadactive:%d\n", 
        ads, threadactive);
      fflush(stdout);
      if(threadactive==0) {
        startThread();
      } else { // 1: 2nd global run (ok) or what?
        // perhaps, here we should stop/start active thread.
        sprintf(em1,"u:ads:%d, threadactive, i.e. 2nd global?", ads); 
        printf("DOcmd warning: %s/n", em1); //goto ERR;
        ;
      };
    };
  } else if((strcmp(value,"a")==0) || (strcmp(value,"d")==0)) {
    int det,rc; char adddel;
    adddel=value[0];
    while(1) {
      token= nxtoken(msg, value, &ix);
      if(token == tINTNUM) {
        det= str2int(value);
        if(adddel=='a') {
          rc= addDET(det, 0);
        } else {
          rc= delDET(det);
        };
        if(rc!=0) { sprintf(em1,"add/del:%c rc:%d", adddel, rc); goto ERR; };
        if(threadactive==0) {
          startThread();
        } else {
          sprintf(em1,"a:det:%d but threadactive is 1", det); goto ERR;
        };
      } else if(token == tINTNUM) {
        break;
      } else {
        strcpy(em1,"int expected: 'a/d det1 det2 ...' "); goto ERR;
      };
    };
  } else if((strcmp(value,"p")==0)) {
    printDETS();
  } else {
    strcpy(em1,"a,d or p expected as first item in message"); goto ERR; 
  };
} else {
  strcpy(em1,"a,d or p expcted as first item in message"); goto ERR;
};
return;
ERR:
printf("DOcmd ERROR:%s msg:%s\n", em1, msg);
}

/*---------------------------------------------*/ void registerDIM() {
#define MYNAME "CTPCALIB"
char command[100];
printf("DIM server:%s\n",MYNAME);
/*dis_add_error_handler(error_handler);
dis_add_exit_handler(exit_handler);
dis_add_client_exit_handler (client_exit_handler); */
printf("Commands:\n");
strcpy(command, MYNAME); strcat(command, "/DO");
dis_add_cmnd(command,"C", DOcmd, 88);  printf("%s\n", command);
printf("serving...\n");
dis_start_serving(MYNAME);  
}
void stopDIM() {
dis_stop_serving();
}

/*------------------------------------*/ int main(int argc, char **argv)  {
int rc,ads,ix; char msg[100];
/*
if(argc<3) {
  printf("Usage: ltuserver LTU_name base\n\
where LTU_name is detector name\n\
      base is the base address of LTU (e.g. 0x811000)\n\
"); exit(8);
}; 
if(argc>1) {
  if(strcmp(argv[1],"no1min")==0) {
    dimsflags=dimsflags | NO1MINFLAG;
  };
};*/
setlinebuf(stdout);
signal(SIGUSR1, gotsignal); siginterrupt(SIGUSR1, 0);
signal(SIGQUIT, gotsignal); siginterrupt(SIGQUIT, 0);
signal(SIGINT, gotsignal); siginterrupt(SIGINT, 0);
signal(SIGBUS, gotsignal); siginterrupt(SIGBUS, 0);
sprintf(msg, "gcalib starting, ver 2 %s %s...", __DATE__, __TIME__);
prtLog(msg);
rc= vmeopen("0x820000", "0xd000");
if(rc!=0) {
  printf("vmeopen CTP vme:%d\n", rc); exit(8);
};
cshmInit();
unlockBakery(&ctpshmbase->swtriggers,swtriggers_gcalib);

initDET(); // has to be after cshmInit()
DFS= new DataFiles();
checkCTP(); 
printf("No initCTP. initCTP left to be done by main ctp-proxy when started\n"); 
//initCTP();
registerDIM();
beammode= get_DIMW32("CTPDIM/BEAMMODE");  //cannot be used inside callback
ads= shmupdateDETs();  // added from 18.11.2010
if(ads>0){
  if(threadactive==0) {
    startThread();
  } else {
    printf("ads:%d but threadactive is 1 at the start", ads);   //cannot happen
  };
};
#ifdef SIMVME
printf("srand(73), (SIMVME)...\n");
srand(73);
#endif
printDETS();
while(1)  {  
  /* the activity of calthread is checked here:
    if threadactive==1 & heartbeat did not change, the calthread
    is not active in spite of threadactive is claiming it is active!
  */
  if(threadactive==1) {
    if(heartbeat == last_heartbeat) {
      prtLog("ERROR: heartbeat is quiet, setting threadactive to 0.");
      threadactive=0;
    };
  };
  //printf("sleeping 40secs...\n");
  last_heartbeat= heartbeat;
  /*dtq_sleep(2); */
  sleep(40);  // should be more than max. cal.trig period (33s for muon_trg)
  /*if(detectfile("gcalrestart", 0) >=0) { 
    char msg[200];
    sprintf(msg,"gcalrestart exists"); prtLog(msg);
    system("rm gcalrestart");
    sprintf(msg,"main: gcalrestart removed, exiting..."); prtLog(msg);
    quit=8; 
  }; */
  if(quit>0) break;
  beammode= get_DIMW32("CTPDIM/BEAMMODE");  //cannot be used inside callback
  //ds_update();
};  
sprintf(msg, "Exiting gcalib. quit:%d...\n",quit); prtLog(msg);
stopDIM(); 
// stop all active dets:
for(ix=0; ix<NDETEC; ix++) {
  rc= delDET(ix);
  if(rc!=0) { printf("delDET(%d) rc:%d", ix, rc); };
};
delete DFS;
cshmDetach();
vmeclose();
exit(0);
}   

