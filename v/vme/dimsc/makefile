# test makefile
include makefile_common
SDIR= .
LIBS = $(EXTRALIBS)
all: $(ODIR) $(ODIR)/server $(ODIR)/client \
 $(ODIR)/server2 $(ODIR)/client2 $(ODIR)/client_stamped
$(ODIR):
	mkdir -p $(ODIR)
$(ODIR)/server:	$(SDIR)/server.c $(DIMDIR)/$(ODIR)/libdim.a
	$(CC) $(CFLAGS) $(LDFLAGS) $(SDIR)/server.c -L${SMIDIR}/linux -lsmi -L$(DIMDIR)/linux -ldim -o $(ODIR)/server $(LIBS)
$(ODIR)/server2:	$(SDIR)/server2.c $(DIMDIR)/$(ODIR)/libdim.a
	$(CC) $(CFLAGS) $(LDFLAGS) $(SDIR)/server2.c -L${SMIDIR}/linux -lsmi -L$(DIMDIR)/linux -ldim -o $(ODIR)/server2 $(LIBS)
	
$(ODIR)/client:	$(SDIR)/client.c $(DIMDIR)/$(ODIR)/libdim.a
	$(CC) $(CFLAGS) $(LDFLAGS) $(SDIR)/client.c -L$(DIMDIR)/linux -ldim -o $@  $(LIBS)
$(ODIR)/client2: $(SDIR)/client2.c $(DIMDIR)/$(ODIR)/libdim.a
	$(CC) $(CFLAGS) $(LDFLAGS) $(SDIR)/client2.c -L$(DIMDIR)/linux -ldim -o $@  $(LIBS)
$(ODIR)/client_stamped:	client_stamped.c $(DIMDIR)/$(ODIR)/libdim.a
	$(CC) $(CFLAGS) -I$(VMEBDIR)/vmeblib $(LDFLAGS) $< \
 -L$(DIMDIR)/linux -ldim -L$(VMEBDIR)/vmeblib/linux_s -lvmeb \
 -o $@  $(LIBS)
clean:
	-rm $(ODIR)/*

