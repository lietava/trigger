#include "L0BOARD.h"
L0BOARD::L0BOARD(int vsp)
:
	BOARD("l0",0x829000,vsp,4),
	L0_CONDITION(0x400),
	L0_INVERT(0x600)
{
  this->AddSSMmode("inmon",0); 
  this->AddSSMmode("outmon",1); 
  this->AddSSMmode("ingen",2); 
  this->AddSSMmode("outgen",3); 
}
//----------------------------------------------------------------------------
/* 
 * Check counters assuming classes are not configured
 */
int L0BOARD::CheckCountersNoTriggers()
{
 int ret=0;
 w32 time = countdiff[CL0TIME]; 

 if(countdiff[CL0STR] != 0){
   printf("L0 strobe != 0 %u \n",countdiff[CL0STR]);
   ret=1;
 }
 for(int i=0;i<100;i++){
  w32 tt=time*0.4/0.025;
  w32 diff;
  if(tt>countdiff[i+CL0CLSB]) diff  = tt-countdiff[i+CL0CLSB];
  else diff = countdiff[i+CL0CLSB]-tt;
  if( diff > 16){
    printf("L0classB%02i != time  %u %u\n",i,countdiff[i+CL0CLSB] ,tt);
    ret=1;
  }
  if(countdiff[i+CL0CLSA] != 0){
    printf("L0classA%02i != 0 %u \n",i,countdiff[i+CL0CLSA]);
    ret=1;
  }
 }
 for(int i=0;i<7;i++){
    if(countdiff[i+CL0CLST] != 0){
      printf("l0clst%1i != 0 %u \n",i, countdiff[i+CL0CLST]);
      ret=1;
    }
 }
 if(ret==0)printf("L0  CheckCountersNoTriggers: NO ERROR detected.\n");
 return ret;
} 
//----------------------------------------------------------------------------
/* 
 * set single class at index, with input mask, l0function and rnd and bc
 * index can be mask, that is bit pattern of chosen classes
*/
void L0BOARD::setClass(w32 index,w32 inputs,w32 l0f,w32 rn,w32 bc)
{
 w32 word=inputs+(l0f<<24) + (rn<<28)+(bc<<30);
 vmew(L0_CONDITION+4*index,word);
}
//----------------------------------------------------------------------------
/*
 *  set single class at index and  with input mask and default l0f,rnd and bc
 *  index can be class mask
*/
void L0BOARD::setClass(w32 index,w32 inputs)
{
 setClass(index,inputs,0xf,0x3,0x3);
}
//----------------------------------------------------------------------------
/* 
 * set single class at index and  with input mask and cluster
 * index can be class mask
*/ 
void L0BOARD::SetClass(w32 index,w32 inputs,w32 cluster)
{
 w32 invinputs = ~inputs & 0xffffff;
 setClass(index,invinputs);
 setClassVetoes(index,cluster);
}
//----------------------------------------------------------------------------
/* 
 * Set all classes to 0xfffff - dont care
*/
void L0BOARD::setClassesToZero()
{
 for(w32 i=0; i<kNClasses; i++){
    setClass(i+1,0xffffff,0xf,0x3,0x3);
    setClassVetoes(i+1,0,0xfff,0x1,0x1);
 }
}
//-----------------------------------------------------------------------------------
void L0BOARD::setClassInvert(w32 index,w32 invert)
{
 vmew(L0_INVERT+4*index,invert);
}
//-----------------------------------------------------------------------------------
//
int L0BOARD::AnalSSM()
{
 return 0;
}
