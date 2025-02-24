#!/usr/bin/python
"""
Usage: validate.py partname [strict]
       strict: DIM service providing luminosity has to be available
Stdout: list of input detectors (1 line)
        ' '\n -no input detectors
        'Errors:\n... -error message (integrity of files broken)
RC: 0: ok
    8: error message printed to stdout
Output files:
/tmp/partname.log
/tmp/partname.pcfg 
"""
import os.path, sys
WORKDIR= "/tmp"   # "./"
def prterr(errlines):
  #print os.path.join( WORKDIR,partname+".pcfg")
  #f= open( os.path.join( WORKDIR,partname+".pcfg"), "w")
  #f.write("Errors:\n") ; f.write(errlines)  ; f.close()
  #print "Errors:"
  print errlines
  sys.exit(8)
def main():
  if hasattr(sys,'version_info'):
    if sys.version_info >= (2, 3):
      # sick off the new hex() warnings, and no time to digest what the
      # impact will be!
      import warnings
      warnings.filterwarnings("ignore", category=FutureWarning, append=1)
  #reload(parted)
  if len(sys.argv)<2:
    prterr("1 parameter missing: partition name")
  strict=None
  if len(sys.argv)>2:
    if sys.argv[2]== "strict":
      strict= "strict"
  partname= sys.argv[1]   #"beam" "erp"
  saveout= sys.stdout
  logfile= open(os.path.join( WORKDIR,"parted.log"), "w")
  sys.stdout= logfile
  import parted
  #if parted.TDSLTUS.loaderrors!=
  #print "parted.log..."
  #print parted.TDLTUS.loaderrors
  logfile.write(parted.TDLTUS.loaderrors)
  sys.stdout= saveout ; logfile.close()
  #
  saveout= sys.stdout
  logfile= open(os.path.join( WORKDIR,partname+".log"), "w")
  sys.stdout= logfile
  part= parted.TrgPartition(partname, strict)
  part.prt()
  if part.loaderrors=="":
    errs= part.savepcfg(wdir=WORKDIR)   # without 'rcfg '
    sys.stdout= saveout ; logfile.close()
    if errs!="":
      prterr(errs)
    else:
      part.prtInputDetectors()
      #print part.prtInputDetectors(hexs="yes")
  else:
    sys.stdout= saveout ; logfile.close()
    prterr(part.loaderrors)
 
if __name__ == "__main__":
    main()

