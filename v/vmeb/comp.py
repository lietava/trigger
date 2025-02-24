#!/usr/bin/env python2
"""vmeb compiler
Usage: 
comp.py board
  -> looks for file board_cf in $VMECFDIR, if not found, new
  one is created from tho list of all *.[ch] files in $VMECFDIR/BOARD
  directory
History:
4.11.2002 VMEREGSTART32,16,8 added (instead of REGSTART)
          boardname.make not created if old one already exists
7.11.     FGROUP [TOP |groupname] [GUI fuifun_name]
29.12. /*FGROUP "name
                 name2" -FGROUP name on more lines possible
27.2. BoardBaseAddress can be CONST (hex, dec) or "VXI0::450"
ToDo:
Done (29.12.) /*FGROUP 'CSR1 CSR2' - doesn't take fgroup name as expected
- create dir in VMEBDIR dirextory

14.11. /*END -everything ignored after this label in .cf file
 1.12. OSVAR added to vmeb/vmeai.make, and correspondingly checked
       in comp.py
 3.12. space length added for /*BOARD (char BoardSpaceLength[]=)
26. 1. BOARD.cf file abandoned. Now /*FGROUP... statements are
       in any [ch] files in BOARD directory
19.4.  char * as a func. parameter allowed
23.2.  VMERCCINCS added (when makefile prepared with slave interface)
 9.3.  /*FGROUP "Set\npars" GUI func_name [button_name]
       - FGROUP name can be string now
       - optional button_name (can be string too, unfortunatelly
         the \n in Menu widget doesn't work as expected) 
10.3.  -function type now registered ( see FTYPES)
12.3.  - now, START of the comment inside commnet detected ( see incomm)
27.4.  - VMECC -set in BOARD.make to 'cc' or 'gcc'
24.5.  - /*HIDDEN added
28.5.  - BoardName added
23.6.  - from now OSVAR (in vmeai.make) changed to VMEDRVAR and
         $OS changed to $VMEDRIVER (environment variable)
 6.7.  - *_sim.c* files not scanned if VMEDRIVER != SIMVME
14.9.2005
       - VARARGS added -i.e. declaration of function with
         variable arguments 'int func(int n, ...)' now allowed
16.9.2005 bug /*FGROUP TOP GUI SSMbrowser */ fixed
       Before: closing '*/' had to be on next line, or help part
               had to be supplied
30.9.2005 {} added for all the initialiser-items in *_cf.c files
20.2.2006 bug fixed: par. type *char produced only char
18.10.2006 vmeb/vmeblib introduced (commmon library for ctp_proxy,ctp,ltu,ttcvi)
11.6. doFGROUP now better (first '/*FGROUP group' statement 
  was not taken correctly- it created TOP button) 
"""
import os.path, os, string, sys, glob
SYMNAME=1
AFPTYPE=2     # allowed function/parameter type (int, w32, void)
COMSTART=3
COMEND=4
OPERATOR=5
LEFTBRACKET1=6
RIGHTBRACKET1=7
COMSTARTSLASH=8
CONST=12
COMMA=13
NEWLINE=14
REGEND= 15
STRING= 16
VARARGS=17
UNKNOWN=88
#
#self.ifuncs[ix][0..4]:
NGUINAME=5      # the name shown as the button name
NPARDESC=4	# number of parameters for this function
NFNAME=3        # name of the function
NFTYPE=2        # return type
NFUSAGE=1       # help text
NFGROUP=0       # the name of the group the func belongs to

FTYPES={"w32":"0x100","w16":"0x100","w8":"0x100",
        "int":"0x200","float":"0x300","void":"0x400","char":"0x500"}
class aiup:
  """lexical parser for .c .h files
  name.cf -input file
  """
  # token types:
  def __init__(self, boardName):
    self.wordchars = '#abcdfeghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_'
    self.digits = '0123456789'
    self.hexachars= '0123456789abcdefABCDEF'
    self.whitespace=" \t\n"
    self.retchars= []
    self.board= ("", "0x0", "0x1000", "A24", "")  #name,base,len,modifier,usage
    self.vmeregs= []
    self.hiddenfuncs=""
    self.ifuncs= []
    self.line=""
    #self.vmecfdir= os.getenv('VMECFDIR')
    #self.vmebdir= os.getenv('VMEBDIR')
    self.os= os.environ['VMEDRIVER']
    self.vmecfdir= os.environ['VMECFDIR']
    self.vmebdir= os.environ['VMEBDIR']
    #Xself.cfname=cffile
    #Xself.name= os.path.splitext(cffile)[0]   # default (should follow boardname)
    #Xself.basename= os.path.basename(self.name)
    self.basename= boardName
    #working dir for comp is: ...vmecfdir/boardname/
    boarddir=self.vmecfdir+'/'+self.basename+ '/'
    if os.access(boarddir, os.W_OK) == 0:
      sys.exit(boarddir+" doesn't exist")
    os.chdir(boarddir)
    # remove it before searching for .[ch] files:
    if os.access(self.basename+"_cf.c", os.F_OK) != 0:
      os.system("rm "+self.basename+"_cf.c")
    if os.access(self.basename+"_cf", os.W_OK) == 0:
      print self.basename+"_cf doesn't exists, creating..."    
      self.do_cf();
    else:
      print 'Reading '+self.basename+"_cf file..."    
      c= open(self.basename+'_cf', "r")
      self.hnames= c.readlines()
      c.close()
      for i in range(len(self.hnames)):
        self.hnames[i]= string.strip(self.hnames[i])
      #for hn in self.hnames: print ':',hn,':'
      
    # BOARD_cf file if present used for searching /*FGROUP ...
    # statements. In addition, it is used for BOARD.make file creating
    # if it doesn't exist
    #print 'self.hnames:',self.hnames
    self.errors=0
    for hn in self.hnames:
      #if self.os != "SIMVME" and (hn == (self.basename+"_sim.c")):
      if self.os != "SIMVME" and (string.find(hn,'_sim.c')!=-1):
        #print self.basename+"_sim.c"+ " not scanned (",self.os,")"
        print hn + " not scanned (",self.os,")"
        continue
      fnc= self.findLabel(hn)
      #print "fnc=",fnc
      #####if fnc == None: break
      #fnc()
    #ol= "\n#define DefaultBaseAddress %s"%self.board[1]
    #ol= "\nw32 BoardBaseAddress=%s;"%self.board[1]
    # max 10 chars: 0x12345678 (unix) or VXI0::450 (visa)
    self.c= open(self.basename+'_cf.c', "w")
    self.c.write("/* generated by comp.py */\n")
    self.c.write('#include <stdio.h>\n') 
    self.c.write('#include "vmewrap.h"\n')
    self.c.write('#include "lexan.h"\n')
    self.c.write('#include "vmeaistd.h"\n')
    ol= "\nchar BoardName[]=\"%s\";"%self.basename
    self.c.write(ol)
    ol= "\nchar BoardBaseAddress[11]=\"%s\";"%self.board[1]
    self.c.write(ol)
    ol= "\nchar BoardSpaceLength[11]=\"%s\";"%self.board[2]
    self.c.write(ol)
    ol= "\nchar BoardSpaceAddmod[11]=\"%s\";"%self.board[3]
    self.c.write(ol)
    for x in self.ifuncs:
      npars= len(x[NPARDESC])
      if npars>0:
        ol= '\nTpardesc %s_parameters[%d]={'%(x[NFNAME], npars)
        self.c.write(ol)
        npar=0
        #print "Tpardesc-x:",x
        for par1 in x[NPARDESC]:
          #print "Tpardesc-par1:",par1
          npar= npar+1
          if par1[1] == 'int':
            prtyp='1'
          elif par1[1] == 'w32' or par1[1] == 'w16' or par1[1] == 'w8':
            prtyp='2'
          elif par1[1] == 'char':
            prtyp='3'
          elif par1[1] == 'float':
            prtyp='4'
          #elif par1[1] == 'VARARGS':
          #  prtyp='4'
          else:
            prtyp='bad par. type'
          #
          if par1[2] == '*':
            prindir= '| 0x80000000'
          elif par1[2] == 'VARARGS':
            prindir= '| 0x40000000'
          elif par1[2] == '':
            prindir=''
          else:
            prindir= 'Error: * or ... expected'
          if npar == npars:
            enofli= '};'
          else:
            enofli= ','
          ol= '\n{"%s", %s%s}%s' % (par1[0], prtyp, prindir, enofli)
          self.c.write(ol)
      if x[NFUSAGE]:
        ol='\nchar %s_usagehelp[]="%s";\n'%(x[NFNAME], x[NFUSAGE])
        self.c.write(ol)
    self.c.write("\n")
    # write out the function prototypes:
    for x in self.ifuncs:
      if x[NFTYPE]!="GUI":
        #print "fcProts.x:",x
        ol= '%s %s('% (x[NFTYPE],x[NFNAME])
        cornone=''
        if len(x[NPARDESC]) > 0:
          for xp in x[NPARDESC]:
            #print "fcprots.xp:",xp 
            if xp[2]=="VARARGS":
              ol= ol+ cornone + xp[1] + ' ' + xp[0] +', ...'
            else:
              ol= ol+ cornone + xp[1] + ' ' + xp[2]+xp[0]
            cornone=', '
        ol= ol+');\n'
        #print "funcprots:",ol
        self.c.write(ol)
    #write out table allnames:
    ol= "\nint nnames=%d;\n"%(len(self.vmeregs)+len(self.ifuncs)+1)
    self.c.write(ol)
    self.c.write("Tname allnames[MAXNAMES]={\n");
    #print "dbgsb:",self.board
    #ol= '"%s", tSYMNAME, NULL, 0, NULL, %s, NULL'%(self.board[0],self.board[1]) 
    #ol= '{"%s", tSYMNAME, NULL, {.strptr=BoardSpaceLength}, 0.0, NULL, {.bax=BoardBaseAddress}, NULL}'%(self.board[0]) 
    ol= '{"%s", tSYMNAME, NULL, {0}, 0.0, NULL, {0}, NULL}'%(self.board[0]) 
    self.c.write(ol)
    for x in self.vmeregs:
      if x[2]:
        ol= ',\n{"%s", tVMEADR|%s, NULL, {0}, 0.0, NULL, {%s}, NULL}'%(x[0],x[2],x[1])
      else:
        ol= ',\n{"%s", tVMEADR, NULL, {0}, 0.0, NULL, {%s}, NULL}'%(x[0],x[1])
      self.c.write(ol)
    for x in self.ifuncs:
      if len(x[NPARDESC]) > 0:
        pars=x[NFNAME]+'_parameters'
      else: pars= 'NULL'
      if x[NFUSAGE]:
        usghlp= x[NFNAME]+'_usagehelp'
      else:
        usghlp= 'NULL'
      #print "xinifuncs:",x
      if x[NFTYPE]=="GUI":
        ol= ',\n{"%s", tFUN, %s, {0xdead}, 0.0, %s, {%d}, %s}'% \
          (x[NFNAME],"NULL",pars,len(x[NPARDESC]),usghlp)
      else:
        ftx= FTYPES[x[NFTYPE]]
        #ol= ',\n{"%s", tFUN+%s, (w32 (*)())%s, 0xdead...
        ol= ',\n{"%s", tFUN+%s, (funcall)%s, {0xdead}, 0.0, %s, {%d}, %s}'% \
          (x[NFNAME],ftx,x[NFNAME],pars,len(x[NPARDESC]),usghlp)
      self.c.write(ol)
    self.c.write("};\n")
    self.c.close()
    if self.errors>0:
      print 'errors found, exiting...'
      return
    #sys.exit('debug exit')
    #------------------------------------------ prepare makefile
    # if it doesn't exist
    if self.os == "Windows_NT":
      os.system(self.vmebdir+"\wincomp.bat "+self.basename)
    else:
      if os.access(self.basename+'.make', os.F_OK)==0:
        # BOARD.make doesn't exist:
        self.domakefile()
      else:
        # BOARD.make exists:
        c= open(self.basename+'.make', "r")
        #print c.readline()
        newos=c.readline()
        newos=string.split(newos)[1]
        c.close()
        if newos != self.os:
          print 'removing '+boarddir+self.basename+ \
            '.make,*.o (old VMEDRIVER:'+newos+'!='+self.os+')'
          #os.system("touch "+self.vmebdir+'/'+"cmdbase.c")
          os.system("rm "+boarddir+"*.o")
          self.domakefile()
        else:
          print self.basename+'.make'+' already exists and is used'
      # make cmd line interface:
      os.system("make -f "+self.basename+".make")

  def do_cf(self):
    #----------------------------------------- prepare BOARD_cf file
    hnames= glob.glob('*.[hc]') #; hnames.sort()
    c= open(self.basename+'_cf', "w")
    for hn in hnames:
      c.write(hn+"\n")
    self.hnames= hnames
    c.close()

  def domakefile(self):
    neobfi=''   # to be included in makefile dependances:
    dependson={} # dependson['file.c']='file1.h file2.h'
    for n in self.hnames:
      if n[-1] == 'c':
        #if self.os != "SIMVME" and (n == (self.basename+"_sim.c")):continue
        if self.os != "SIMVME" and (string.find(n,'_sim.c')!=-1):continue
        hdeps=''
        c= open(n, "r")
        for line in c.readlines():
          if line[0:9] == '#include ':
            hfile= string.split(line)[1]
            #print 'domakefile:', type(hfile),hfile
            if hfile[0] == '"':
              # commented 18.10 (vmeblib introduced):
              if hfile == '"vmewrap.h"' or \
                 hfile == '"vmesim.h"':
                hfile='"$(VMEBDIR)/vmeblib/vmewrap.h"'
                vmebdirinc=1
              if hfile == '"vmeblib.h"':
                hfile='"$(VMEBDIR)/vmeblib/vmeblib.h"'
                vmebdirinc=1
              VMERCCINCS='/ATLAS/rcc_error/v1r0p1/src/'
              if hfile[1:-1]=='rcc_error/rcc_error.h' or \
                 hfile[1:-1]=='vme_rcc/vme_rcc.h' or \
                 hfile[1:-1]=='cmem_rcc/cmem_rcc.h' :
                hfile2=VMERCCINCS+hfile[1:-1]
              else:
                hfile2=hfile[1:-1]
              hdeps= hdeps+ ' ' +hfile2
        dependson[n]= hdeps
        neobfi= neobfi + n[:-1]+'o '
    print 'object files:',neobfi
    c= open(self.vmebdir+'/'+'vmeai.make', "r")
    ol= string.join(c.readlines(), '')
    c.close()
    ol= string.replace(ol, "BNAME", self.basename)
    ol= string.replace(ol, "NEXTOBJFILES", neobfi)
    ol= string.replace(ol, "VMEDRVAR", self.os)
    c= open(self.basename+'.make', "w")
    c.write(ol); 
    for fn in dependson.keys():
      ol= fn[:-1]+'o: '+ fn + dependson[fn] + '\n'
      c.write(ol)
      # commented 18.10 (vmeblib introduced):
      #if string.find(dependson[fn], "vmewrap.h")!= -1 or \
      #   string.find(dependson[fn], "vmesim.h")!= -1:
      #  ol= '\t $(VMECC) -g -c $(CFLAGS) -I$(VMEBDIR) '+fn +'\n'
      #else:
      ol= '\t $(VMECC) -g -c $(CFLAGS) $(INCDIRS) '+fn +'\n'
      c.write(ol)
    c.close()
    print self.basename+'.make'+' created'

  def printboard(self):
    #print "--------------\nBoardname and base addr:", self.board
    #print "Regs:",len(self.vmeregs),"\n",  self.vmeregs
    #print "Funcs:",len(self.ifuncs)
    #for x in self.ifuncs:
    #   print x
    #print self.ifuncs
    tkf= open(self.basename+'_cf.py', "w")
    #tkf.write('self.baseAddr="'+str(self.board[1])+'"\n')
    ##ba= '%s'%self.board[1]
    ##tkf.write('self.baseAddr="'+ba+'"\n')
    tkf.write('self.baseAddr="'+self.board[1]+'"\n')
    tkf.write('self.spaceLength="'+self.board[2]+'"\n')
    tkf.write('self.vmeregs='+str(self.vmeregs)+'\n')
    # .c file is already printed
    for ix in range(len(self.ifuncs)):
      #print "printboard.ix:",ix,self.ifuncs[ix]
      self.ifuncs[ix][NFUSAGE]= string.replace(self.ifuncs[ix][NFUSAGE],
        '\\n\\','')
      if self.ifuncs[ix][NFGROUP]:
        self.ifuncs[ix][NFGROUP]= string.replace(self.ifuncs[ix][NFGROUP],
        '\\n','\n')
      if len(self.ifuncs[ix])>=(NGUINAME+1):
        self.ifuncs[ix][NGUINAME]= string.replace(self.ifuncs[ix][NGUINAME],
        '\\n','\n')
    tkf.write('self.hiddenfuncs="'+self.hiddenfuncs+'"\n')
    tkf.write('self.funcs='+str(self.ifuncs)+'\n')
    tkf.close()

  def printout(self):
    self.c.write(self.line)
    #print self.line[0:-1]

  def err(self, msg,s=""):
    print 'ERROR in line:',self.linenumber,' line:',self.line[:-1]
    print '       ',msg
    if s == "exit": sys.exit(msg)
    self.errors= self.errors+1
    #raise msg

  def getline(self):
    #Xself.printout()
    self.line= self.cf.readline()
    self.linenumber= self.linenumber+1
    self.ixlex=0

  def getchar(self):
    if self.retchars != []:
      nc= self.retchars[-1]
      self.retchars= self.retchars[0:-1]
      return(nc)
    if self.line == '': return(None)
    nc= self.line[self.ixlex]
    self.ixlex= self.ixlex+1
    if nc == '\n':
      #if self.incomm2==None: self.getline()
      self.getline()
    return nc

  def retchar(self, c):
    self.retchars.append(c)
    #if self.ixlex <= 0:
    #  print "internal error - aiup.retchar()"
    #else:
    #  self.ixlex= self.ixlex-1
    
  def findLabel(self, hn):
    self.cf= open(hn, 'r')
    self.linenumber=1; 
    print "scanning: ",hn,"..."
    while 1:
      self.getline()
      self.incomm=None
      self.incomm2=None
      if self.line=='': break
      if self.line[0:5] ==    "/*END": break
      if self.line[0:7] ==    "/*BOARD":
        self.ixlex= 7
        #return(self.doBOARD)
        self.doBOARD()
      elif self.line[0:12] == "/*REGSTART16":
        self.ixlex= 12
        #return(self.doREGSTART("0x04000000"))
        self.doREGSTART("0x04000000")
      elif self.line[0:11] == "/*REGSTART8":
        self.ixlex= 11
        #return(self.doREGSTART("0x02000000"))
        self.doREGSTART("0x02000000")
      elif self.line[0:12] == "/*REGSTART32":
        self.ixlex= 12
        #return(self.doREGSTART(""))
        self.doREGSTART("")
      elif self.line[0:8] ==  "/*FGROUP":
        self.ixlex= 8
        #return(self.doFGROUP)
        self.doFGROUP()
      elif self.line[0:8] ==  "/*HIDDEN":
        self.ixlex= 8
        #return(self.doFGROUP)
        self.doHIDDEN()
    self.cf.close()
    return(None)

  def ignoreWhite(self, meaningfull=''):
    #nc= self.line[self.ixlex]
    nc= self.getchar()
    while 1:
      if nc == meaningfull: break
      if not(nc in self.whitespace): break
      nc= self.getchar()
    return(nc)
    
  def getToken(self, meaningfull=''):
    """
getToken is used inside the comment:
- after labels /*FGROUP /*REG32 ... until ') -right bracket
  enclosing subroutine parameters declarations in the input stream.
meaningfull: normally space,tab,NL are white-spaces, if meaningfull
             parameter is present the corresponding chars 
             given in it will not be regarded as whitespace
return: tktype,tkvalue
    """
    tktype=None
    tkvalue=''
    if self.line == '': return(None)
    nc= self.ignoreWhite(meaningfull)
    if nc in self.wordchars:
      tktype= SYMNAME
      tkvalue= nc
      while 1:
        nc= self.getchar()
        if nc in self.wordchars+self.digits:
          tkvalue= tkvalue+nc
        else:
          self.retchar(nc)
          break
    elif nc == '"':
      tktype=STRING
      tkvalue="" 
      while 1:
        nc= self.getchar()
        if nc != '"':
          tkvalue= tkvalue+nc
        else:
          break
    elif nc == '0':
      tktype= CONST
      nc= self.getchar()
      if nc == 'x':
        tkvalue= "0x"
        while 1:
          nc= self.getchar()
          if nc in self.hexachars:
            tkvalue= tkvalue+nc
          else:
            self.retchar(nc)
            break
      else:
        tkvalue= '0'
        while 1:
          if nc in self.digits:
            tkvalue= tkvalue+nc
            nc= self.getchar()
          else:
            self.retchar(nc)
            break
    elif nc in self.digits:
      tktype= CONST
      while 1:
        if nc in self.digits:
          tkvalue= tkvalue+nc
          nc= self.getchar()
        else:
          self.retchar(nc)
          break
    elif nc == '/':
      nc= self.getchar()
      if nc == '*':
        if self.ixlex == 2 and self.line[0:8] == "/*REGEND":
          tktype=REGEND; tkvalue= "/*REGEND"
        else:
          tktype= COMSTART
          tkvalue= '/*'
      elif nc =='/':
        tktype= COMSTARTSLASH
        tkvalue= '//'
      else:
        tktype= OPERATOR
        tkvalue= '/'
        self.retchar(nc) 
    elif nc == '*':
      nc= self.getchar()
      if nc == '/':
        tktype= COMEND
        tkvalue= '*/'
      else:
        tktype= OPERATOR
        tkvalue= '*'
        self.retchar(nc) 
    elif nc == '(':
      tktype= LEFTBRACKET1
      tkvalue= '('
    elif nc == ')':
      tktype= RIGHTBRACKET1
      tkvalue= ')'
    elif nc == ',':
      tktype= COMMA
      tkvalue= ','
    elif nc == '.':
      nc= self.getchar()
      if nc=='.':
        nc= self.getchar()
        if nc=='.':
          tktype= VARARGS
          tkvalue= '...'
        else:
          tktype= UNKNOWN
          tkvalue= '..'+nc
      else:
        tktype= UNKNOWN
        tkvalue= '.'+nc
    elif nc == '\n':
      tktype= NEWLINE
      tkvalue= '\n'
    elif nc == None: return None
    else:
      tktype= UNKNOWN
      tkvalue= nc
      #nc= self.getchar()
    #print "Token:",tktype,tkvalue
    return tktype,tkvalue

  def skipTokens(self, tk2skip):
    while 1:
      tk= self.getToken()
      if tk == None: break 
      if tk[0] == COMSTARTSLASH:
        self.getline()
      if tk[0] == COMSTART:
        if self.incomm:
          self.err("ERROR: /* found inside a comment")
        else:
          self.incomm= 1
          self.skipTokens([(COMEND, "")])
      if tk[0] in map(lambda x:x[0], tk2skip):
        self.incomm=None
        break 
    return tk

  def getUsage(self):
    usage=''
    while 1:
      nc= self.getchar()
      if nc == None: break
      if nc == '\n':
        usage=usage+'\\n\\'
      if nc == '*':
        nc= self.getchar()
        if nc == '/':
          break
        else:
          usage=usage+'*'
      usage=usage+nc
    return(usage)

  def doBOARD(self):
    #print "                      here doBOARD"
    tk= self.getToken()
    if tk==None: 
      self.err("Unexpected end of "+self.cfname)
    elif tk[0] == SYMNAME:
      #print tk
      bname= tk[1]
      tk= self.getToken()
      if tk[0] == CONST:   # hexa base address in case of SBC
        bbase= tk[1]
      elif tk[0] == STRING:
        bbase= tk[1]       # "VXI0::450" - NI-VXI interface
      else:
        self.err("Base address of the board expected")
      bspacelength="0x1000"
      tk= self.getToken()
      if tk[0] == CONST:   # hexa VME space length in case of SBC
        bspacelength= tk[1]
      else:
        self.err("Length of VME address space of the board expected")
      tk= self.getToken()
      bspacemodifier= 'A24'  # default. Other: A32, A16 (only for VMERCC/CCT)
      if tk[0] == SYMNAME:   # hexa VME space length in case of SBC
        if tk[1]=='A32' or tk[1]=='A24' or tk[1]=='A16':
          bspacemodifier= tk[1]
      self.board= bname,bbase,bspacelength,bspacemodifier,"description not yet done"
      #print "                           doBOARD:",bname,bbase
    else:
      self.err("Board name expected")

  def doREGSTART(self, v81632):
    #print "                      here doREGSTART",v81632
    self.skipTokens([(COMEND, "")])
    while 1:
      #tk= self.getToken()
      #while tk[0] == NEWLINE: tk= self.getToken()
      tk= self.skipTokens([(SYMNAME,"#define"), (REGEND,"")])
      #print "dbg:",tk
      if tk == None:
        self.err("/*REGEND mising")        
      elif tk[0]==REGEND:
        #print "OK REGEND found"
        break;
      elif tk[1] == '#define':
        tk= self.getToken()
        if tk[0] == SYMNAME:
          tk2= self.getToken()
          if tk2[0] == CONST:
            self.vmeregs.append( (tk[1], tk2[1], v81632) )
      else:
        print "got:", tk
        self.err("#define expected")        
    #print "regstart finished"

  def doFGROUP(self):
    """ /*FGROUP TOP [GUI] [[pydefname ["ButtonName"]]
        /*FGROUP group [GUI [[pydefname "ButtonName"]]
    """
    #print "                      here doFGROUP"
    f1=[]   #[ FUNgroupName, usage, ftype, fname, [ [p1, t1, i1],...] ]
    #             0      1      2      3
    # p1: par. name, t1: par. type, i1: '' or '*' for indirect
    # gui function:
    #        [ FUNgroupName, usage, GUI, fname, [] ]
    ftypename=None   # remains None for non GUI declarations
    tk= self.getToken('\n')
    if tk[0]== SYMNAME or tk[0]==STRING:
      if tk[1]== 'TOP':
        f1.append(None)
        tk= self.getToken('\n')
      elif tk[1]=='GUI':
        f1.append(None)   #group name missing, default: TOP
      else:
        f1.append(tk[1])
        tk= self.getToken('\n')
      #if tk[0]== SYMNAME and tk[1]== 'GUI':
      if tk[1]== 'GUI':
        tk= self.getToken('\n')
        #if tk[0]== SYMNAME or tk[0]==STRING:
        if tk[0]== SYMNAME:
          tk2= self.getToken('\n')
          #print"doFGROUP tk2:",tk2
          if tk2[0]== SYMNAME or tk2[0]==STRING:
            ftypename= 'GUI',tk[1],tk2[1]
            tk= self.getToken('\n')  # 1609 fn "fn show" rest->tk
          else:
            ftypename= 'GUI',tk[1],tk[1]
            tk= tk2                  # 1609 fn rest->tk
        elif tk[0]==STRING:
          self.err(" String ("+tk[1]+") not allowed as GUI function name",
            "exit")
        else:
          self.err(" GUI function name missing")
    else:
      f1.append(None)

    if tk[0] != COMEND:
      #tk= self.skipTokens([(COMEND,"*/")])
      f1.append(self.getUsage())
    else:
      f1.append("")
    if ftypename:
      f1.append(ftypename[0])    # GUI
      f1.append(ftypename[1])    # GUI name
      f1.append([])             # no pars with GUI
      f1.append(ftypename[2])    # GUI button name
      #print "doFGROUP f1:",f1
    else:
      tk= self.getToken()
      if tk[0] == SYMNAME:
        tk2= self.getToken()
        if tk2[0] == SYMNAME:
          f1.append(tk[1])    # func. type
          f1.append(tk2[1])   # name
          tk= self.getToken()
          #print "dbgg:",tk
          if tk[0] != LEFTBRACKET1:
            self.err("'(' missing in func. declaration")
        elif tk2[0] == LEFTBRACKET1:
          f1.append("int")   # default return func. type
          f1.append(tk[1])   # name
        else:
          self.err("'(' missing in function declaration", 'exit')
      else:
          print f1
          self.err("Func. declaration (for group " +   #f1[0]+
            ") expected instead of this line",'exit')
      npar=0; pars=[]
      lastType=''
      while 1:
        tk= self.getToken()
        #print "tk:",tk
        if tk[0] == SYMNAME:
          t1= tk[1]             # type ?
          if t1=='const':
            continue
          elif t1=='void' or t1=='int' or t1=='w8' or t1=='w16' or\
             t1=='w32' or t1=='char' or t1=='float':
            lastType= t1
            tk= self.getToken()
          else:
            if lastType:
              t1= lastType
            else:
              self.err('no type given for '+tk[1]+' func.  parameter')
        elif tk[0] == VARARGS:
          t1="VARARGS"
        elif tk[0] == RIGHTBRACKET1:
          break;
        else:
          self.err("int, char, float, w8, w16, w32 or void expected as parameter type")   
        #
        if tk[0] == OPERATOR and tk[1]=='*':
          i1= tk[1]
          tk= self.getToken()
        else:
          i1= ''
        if tk[0] == SYMNAME:
          p1= tk[1]
        elif tk[0]==VARARGS:
          p1= "VARARGS"
        else:
          self.err("Parameter name expected")
        if p1=="VARARGS":
          pars[-1][2]="VARARGS"
        else:
          pars.append( [p1,t1,i1])   #name,type,[*]
          #print "novarargs-adding par: [", p1,t1,i1,"]"
        #f1.append([p1, t1, i1])
        npar =+ 1
        tk= self.getToken()
        if tk[0] == RIGHTBRACKET1: break;
        if t1=="VARARGS":
          self.err(") expected after ... in list of parameters")
        if tk[0] == COMMA: continue
      f1.append(pars)
      #print "doFGROUP.f1:",f1
    self.ifuncs.append(f1)
    #print "doFGROUP.self.ifuncs:",self.ifuncs

  def doHIDDEN(self):
    #print "                      here doHIDDEN"
    while 1:
      tk= self.getToken('\n')
      if tk[0]== SYMNAME or tk[0]==STRING:
        self.hiddenfuncs= self.hiddenfuncs+" "+tk[1]
      elif tk[0]==COMEND: 
        #print "doHIDDEN:"+self.hiddenfuncs
        break;

def main():
  import sys
  if len(sys.argv) < 2:
    print """
Usage: comp.py bN1 bN2 ...
- input files: bNx_cf (expected in corresponding directories)
- output files: bNx_cf.c, bNx_cf.py, bNx.make, bNx.o, bNx.exe
-OLD output files: bNx.c, bNx.py, bNx.make, bNx.o, bNx.exe
- next step: 
  ./bNx.exe                                 -start line interface   or
  ./crate.py bN1[=baseadr] bN2[=baseadr]    -start GUI 

No input files given, trying v.cf file...
"""
    a= aiup("example")
    a.printboard()
  else:
    for bn in sys.argv[1:]:
      a= aiup(bn)
      a.printboard()

if __name__ == "__main__":
  main()


