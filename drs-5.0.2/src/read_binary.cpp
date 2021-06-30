//
//  read_binary.cpp
//
//  Example file to read binary data saved by DRSOsc.
//
//  This program assumes that a pulse from a signal generator is split
//  and fed into channels #1 and #2. It then calculates the time difference
//  between these two pulses to show the performance of the DRS board
//  for time measurements.
//
//  $Id: read_binary.cpp 21284 2014-03-06 15:07:13Z ritt $
//

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#define FREQ_SAMP 5.12
#define FREQ_CAL  0.100   // GHz

typedef struct {
   char time_header[4];
   char bn[2];
   unsigned short board_number;
} THEADER;

typedef struct {
   char channel[4];
   float tcal[1024];
} TCHEADER;

typedef struct {
   char event_header[4];
   unsigned int ev_serial_number;
   unsigned short year;
   unsigned short month;
   unsigned short day;
   unsigned short hour;
   unsigned short minute;
   unsigned short second;
   unsigned short millisec;
   unsigned short reserved;
   char bn[2];
   unsigned short board_number;
   char tc[2];
   unsigned short trigger_cell;
} EHEADER;

typedef struct {
   char channel[4];
   unsigned short data[1024];
} CHANNEL;

/*-----------------------------------------------------------------------------*/

#define N_CHN 2 // number of saved channels

int main(int argc, const char * argv[])
{
   int ch, i, j, n, fh, ndt;
   double threshold, t1, t2, dt, sumdt, sumdt2;
   THEADER th;
   TCHEADER tch[N_CHN];
   EHEADER eh;
   CHANNEL channel[N_CHN];
   double waveform[N_CHN][1024], time[N_CHN][1024];
   
   // open file
   fh = open("test.dat", O_RDONLY, 0666);
   if (fh < 0) {
      printf("Cannot find data file\n");
      return 0;
   }

   // read time bin widths
   read(fh, &th, sizeof(th));
   for (ch=0 ; ch<N_CHN ; ch++)
      read(fh, &tch[ch], sizeof(tch[ch]));
   
   // initialize statistics
   ndt = 0;
   sumdt = sumdt2 = 0;
   
   for (n= 0 ; ; n++) {
      // read event header
      i = (int)read(fh, &eh, sizeof(eh));
      
      // check for valid event header
      if (memcmp(eh.event_header, "EHDR", 4) != 0) {
         printf("Invalid event header (probably number of saved channels not equal %d)\n", N_CHN);
         return 0;
      }
      
      // print notification every 100 events
      if (n % 100 == 0 || i != sizeof(eh))
         printf("Analyzing event #%d\n", n);
      
      // stop if end-of-file
      if (i != sizeof(eh))
         break;
      
      // read channel data
      for (ch=0 ; ch<N_CHN ; ch++) {
         read(fh, &channel[ch], sizeof(channel[ch]));
         
         for (i=0 ; i<1024 ; i++) {
            // convert to volts
            waveform[ch][i] = channel[ch].data[i]/65536.0-0.5;
            
            // calculate time for this cell
            for (j=0,time[ch][i]=0 ; j<i ; j++)
               time[ch][i] += tch[ch].tcal[(j+eh.trigger_cell) % 1024];
         }
      }
      
      // align cell #0 of all channels
      t1 = time[0][(1024-eh.trigger_cell) % 1024];
      for (ch=1 ; ch<N_CHN ; ch++) {
         t2 = time[ch][(1024-eh.trigger_cell) % 1024];
         dt = t1 - t2;
         for (i=0 ; i<1024 ; i++)
            time[ch][i] += dt;
      }
      
      t1 = t2 = 0;
      threshold = 0.3;
      
      // find peak in channel 1 above threshold
      for (i=0 ; i<1022 ; i++)
         if (waveform[0][i] < threshold && waveform[0][i+1] >= threshold) {
            t1 = (threshold-waveform[0][i])/(waveform[0][i+1]-waveform[0][i])*(time[0][i+1]-time[0][i])+time[0][i];
            break;
         }
      
      // find peak in channel 2 above threshold
      for (i=0 ; i<1022 ; i++)
         if (waveform[1][i] < threshold && waveform[1][i+1] >= threshold) {
            t2 = (threshold-waveform[1][i])/(waveform[1][i+1]-waveform[1][i])*(time[1][i+1]-time[1][i])+time[1][i];
            break;
         }
      
      // calculate distance of peaks with statistics
      if (t1 > 0 && t2 > 0) {
         ndt++;
         dt = t2 - t1;
         sumdt += dt;
         sumdt2 += dt*dt;
      }
   }
   
   // print statistics
   printf("dT = %1.3lfns +- %1.1lfps\n", sumdt/ndt, 1000*sqrt(1.0/(ndt-1)*(sumdt2-1.0/ndt*sumdt*sumdt)));
   
   return 1;
}

