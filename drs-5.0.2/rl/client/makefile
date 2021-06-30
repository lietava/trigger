SDIR= .
ODIR= linux
DAQDIR = /opt/libDAQlogbook
DIMDIR=/home/pi/dim_v20r9
MYSQLLIBS=`/usr/bin/mysql_config --libs`
EXTRALIBS = -L$(DIMDIR)/linux -ldim
CC=g++
SFX=cxx
CFLAGS = -Wall -g3 -I$(DIMDIR)/dim 
#CFLAGS = -Wall -g3 -I$(DIMDIR)/dim -I$(VMEBDIR)/vmeblib  -I$(VMECFDIR)/ctp 
LDFLAGS = -lpthread
CFLAGS +=-DCPLUSPLUS

client: client.o
	$(CC) $(LDFLAGS) client.o $(EXTRALIBS) -o client
#	rm -f $(ODIR)/*.o


client.o:client.cxx  
	$(CC) $(CFLAGS) -c -o client.o client.cxx

clean:
	-rm -f $(ODIR)/*.o

