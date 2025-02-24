#include "libctp++.h"
#include <time.h>   
#include <sys/time.h>   /* gettimeofday */
/*----------------------------------------------*/
void GetMicSec(w32 *tsec, w32 *tusec) {
int rc;
struct timeval tv; struct timezone tz;
rc=gettimeofday(&tv,&tz);
*tsec=tv.tv_sec; *tusec=tv.tv_usec;
}
w32 DiffSecUsec(w32 tsec,w32 tusec,w32 prevtsec,w32 prevtusec) {
w32 usecdiff;
if( tusec >= prevtusec) {
  usecdiff= 1000000*(tsec-prevtsec) + tusec-prevtusec;
} else {
  usecdiff= 1000000*(tsec-prevtsec-1) + prevtusec-tusec;
};
return(usecdiff);   /* in microseconds */
}
/*---------------------------------------------------------------CountTime()
  Count time from the last function call.
*/
w32 CountTime(){
 static w32 seconds=0;
 static w32 micsec=0;
 w32 sec,mic,time;
 GetMicSec(&sec,&mic);
 time=(sec-seconds)*1000000+(mic-micsec);
 seconds=sec;
 micsec=mic;
 return time;
}
/*FGROUP DebCon
*/
void mysleep(w32 delta){
 w32 seconds,micsec,sec,mic,time;
 GetMicSec(&seconds,&micsec);
 time=0;
 while(time<delta){
   GetMicSec(&sec,&mic);
   time=(sec-seconds)*1000000+(mic-micsec);
 }
}
void getdatetime(char* dmyhms) {
/* format date/time for logs
   dmyhms:  string[20] dd.mm.yyyy hh:mm:ss */
/*int i; */
time_t T;
struct tm *LT;  /*LocalTime */
T=time(&T); LT = localtime(&T);
sprintf(dmyhms,"%2.2d.%2.2d.%4.0d %2.2d:%2.2d:%2.2d",
  LT->tm_mday,LT->tm_mon+1,LT->tm_year+1900,
  LT->tm_hour,LT->tm_min,LT->tm_sec);
  for(int i=0; i<28; i++) { if(dmyhms[i] == ' ') dmyhms[i] = '_'; }; 
}

