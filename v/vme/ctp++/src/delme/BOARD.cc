#include "BOARD.h"
BOARD::BOARD(string const name,w32 const boardbase,int vsp,int nofssmmodes)
:
        BOARDBASIC(name,boardbase,vsp),
        numofmodes(nofssmmodes),
	SSMcommand(0x19c),
	SSMstart(0x1a0),
	SSMstop(0x1a4),
	SSMaddress(0x1a8),
	SSMdata(0x1ac),
	SSMstatus(0x1b0),
	SSMenable(0x1b4),
	SSMomvmer(0),
	SSMomvmew(1)
{
 if(d_name == "ltu")SSMbusybit=0x04; else SSMbusybit=0x100;
 //cout << d_name << " BOARD done"<<endl;
 ssm = new w32[Mega];
 //SSMModes =  new SSMmode[numofmodes];
 ssmtools.setssm(ssm);
}
//---------------------------------------------------------------------------
string *BOARD::GetChannels(string const &mode) const
{
 for(int i=0; i<numofmodes; i++){
   if(SSMModes[i].GetName() == mode) return SSMModes[i].GetChannels();
 }
 return 0;
}
//---------------------------------------------------------------------------
void BOARD::PrintChannels(string const &mode) const
{
 cout << "Board " << d_name << "mode " << mode ;
 for(int i=0; i<numofmodes; i++){
   if(SSMModes[i].GetName() == mode){
      cout << endl;
      for(int j=0;j<32;j++)cout << SSMModes[i].GetChannel(j) << endl;
      return;
   }
 }
 cout << "mode not found." << endl; 
}
//--------------------------------------------------------------------------
// mode  = ingen,inmon,outgen,outmon as in the file names CFG/ctp/ssmsigs
int BOARD::AddSSMmode(string const modename,int const imode) 
{
 //SSMModes[imode].Init(d_name,modename,imode);
 SSMMode mode(d_name,modename,imode);
 SSMModes.push_back(mode);
 return 0;
}
//-----------------------------------------------------------------------------
void BOARD::WriteSSM(w32 const word) const 
{
 for(int i=0;i<Mega;i++)ssm[i]=word;
}
//-----------------------------------------------------------------------------
void BOARD::WriteSSM(w32 const word,int const start,int const last) const
{
 for(int i=start;i<last;i++){
	 ssm[i%Mega]=word;
 cout << Mega <<" WriteSSM: "<< (i%Mega) << endl;
 }
}
//-----------------------------------------------------------------------------
int BOARD::ReadSSM() const
{
 w32 status;
 status=vmer(SSMstatus); 
 //cout << "status= " << hex << status << endl;
 w32 enable=(status&0xc0)<<2;
 w32 mod=SSMomvmer | (status&0x38) | enable;
 w32 ssma=vmer(SSMaddress);
 int rc=setomvspSSM(mod);
 if(rc) return rc;
 w32 nwords=Mega;
 if((ssma & 0x100000)==0){ //overflow bit is 0
    
   //vmew(SSMaddress,Mega-1);  
   vmew(SSMaddress,(w32)-1);
   nwords=ssma;
   printf("Warning: readins SSM without overflow flag: %i read \n",nwords);
 }  
 w32 d= vmer(SSMdata);
 d= vmer(SSMdata);
 for(w32 i=0; i<nwords; i++) {
   ssm[i]= vmer(SSMdata);
 };
 return 0;
}
//-----------------------------------------------------------------------------
int BOARD::WritehwSSM() const
{
  w32 status= vmer(SSMstatus);
  w32 enable= (status&0xc0)<<2;
  w32 mod= SSMomvmew | (status&0x38) | enable;
  int rc= setomvspSSM(mod);
  if(rc) return rc;
  w32 address=0;
  vmew(SSMaddress, address-1);
  for(int ix=0; ix<Mega; ix++)vmew(SSMdata,ssm[ix]);
  /* Final write addr. should be == Mega-1 */
  w32 data= vmer(SSMaddress);
  if( data != (Mega-1)) {
  cout << "ERROR: final write SSMaddress: " << hex << data <<
	  " expected: " << hex << Mega-1 << dec <<endl;
  rc=-1;
};
 return(rc);
}
//-----------------------------------------------------------------------------
void BOARD::PrintSSM(int const start,int const n) const
{
 for(int i=start;i<n;i++) cout << hex << i << " " << hex << ssm[i] << endl;
}
//----------------------------------------------------------------------------
// version from ssm.c
int BOARD::setomvspSSM(w32 const mod) const
{
 w32 bcstatus = getBCstatus();
 if(((0x7&mod)!=SSMomvmer && (0x7&mod)!=SSMomvmew)&&((bcstatus&0x3)!=0x2)) {
  cout << "Warning: BC signal not connected"<< endl;
  /* BC not necessary for vme R/W*/
  //return(1);
 }
 w32 status=vmer(SSMstatus);
 if( status & SSMbusybit) {
   cout << "SSM busy, stopping recording before setting new/op. mode..."<< endl;
   /*  vmew32(SSMstop+BSP*ctpboards[board].dial,DUMMYVAL); */
   vmew(SSMstop,0x0);
   /*  status=vmer32(SSMstatus+BSP*ctpboards[board].dial); */
   status=vmer(SSMstatus);
   if( status & SSMbusybit) {
     cout<<"ERROR: Cannot stop recording, operation or mode wasn't set!"<<endl;
     return(1);
   }
 }
 vmew(SSMcommand,mod&0x3f);
 if(d_name != "ltu"){
  w32  ssmen= (mod>>8)&3;
  vmew(SSMenable, ssmen);    
 }
 return 0;
}
//----------------------------------------------------------------------------
// cont  = 'c' :continous
//      != 'c': 1 pass
//      - assuming that modes in files are always 1 pass
int BOARD::SetMode(string const &mode,char const cont) const
{
 for(int i=0;i<numofmodes;i++){
  if(SSMModes[i].GetName() == mode){
    w32 modecode=SSMModes[i].GetModecode();
    if((cont == 'c'))modecode=modecode+1;
    return SetMode(modecode);
  }
 }
 cout << "SetMode: " << mode << " not found." << endl;
 return 1;
}
//----------------------------------------------------------------------------
int BOARD::SetMode(w32 const modecode) const
{
 int rc;
 ///printf("Setting mode for %s: %x \n",d_name.c_str(), modecode);
 if(d_name == "ltu"){
   if( (modecode&7) >3) {
     cout << "ERROR: setomSSM: " << modecode << ">3 for ltu board" << endl;
     return(2);
   }
   if( (modecode&0x10) != 0x10) {
    cout << "WARNING: setomSSM: "<< modecode <<" bit 0x10 not set for ltu board" <<endl;
   }
   rc= setomvspSSM(modecode&3);
 }else
   rc= setomvspSSM(modecode);   //CTP Board
 return rc;
}
//---------------------------------------------------------------------------
int BOARD::StartSSM() const{
   vmew(SSMaddress,0x0);
   vmew(SSMstart,0xffffffff);
   return 0;
}
//----------------------------------------------------------------------------
void BOARD::StopSSM() const
{
 w32 status = vmer(SSMstatus);
 if((status&SSMbusybit) == 0) {} // warn;
 vmew(SSMstop,0x0);
}
