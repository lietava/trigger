typedef struct Tltucfg1 {
  unsigned int run; 
  unsigned int LTUFineDelay1;
  unsigned int LTUFineDelay2;
  unsigned int LTUBCDelayAdd;
  char detector[20];
} Tltucfg1;
#ifndef ONLY_Tltucfg1
// pydim/server.c needs only definition above
#ifdef PROXY_MAIN
#define EXTRN
#else
#define EXTRN extern
#endif
EXTRN Tltucfg *templtucfg;
EXTRN char BoardBaseAddress[40]; /* LTU VME address */
EXTRN char BoardSpaceLength[40];

EXTRN char dimservernameCAP[36];
EXTRN char PARTITION_NAME[24];

/* functions from ltu_utils.c */
void infolog_trg(char level, char *msg);
int ltu_configure(int global);
int startemu();
void stopemu(int sodeod);
void resumeemu();
void pauseemu();
int eodemu(int sodeod);
int busystatus();
void busy12(int enable);
void setdelay(int);
int Setglobalmode();
void Setstdalonemode();
int ltucfg2daq(Tltucfg1 *lcp);
#endif
