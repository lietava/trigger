/*
 * TriggerDialog.cpp
 * Modal Trigger Configuration Dialog class
 * $Id: TriggerDialog.cpp 21282 2014-03-06 10:13:43Z ritt $
 */

#include "DRSOscInc.h"

TriggerDialog::TriggerDialog( wxWindow* parent )
:
TriggerDialog_fb( parent )
{
   m_frame = (DOFrame *)parent;

   int tc = m_frame->GetTriggerConfig();
   m_cbOR1->SetValue((tc & (1<<0))>0);
   m_cbOR2->SetValue((tc & (1<<1))>0);
   m_cbOR3->SetValue((tc & (1<<2))>0);
   m_cbOR4->SetValue((tc & (1<<3))>0);
   m_cbOREXT->SetValue((tc & (1<<4))>0);

   m_cbAND1->SetValue((tc & (1<<8))>0);
   m_cbAND2->SetValue((tc & (1<<9))>0);
   m_cbAND3->SetValue((tc & (1<<10))>0);
   m_cbAND4->SetValue((tc & (1<<11))>0);
   m_cbANDEXT->SetValue((tc & (1<<12))>0);
   
   wxString s;
   s.Printf(wxT("%1.3lf"), m_frame->GetTrgLevel(0));
   m_tbLevel1->SetValue(s);
   s.Printf(wxT("%1.3lf"), m_frame->GetTrgLevel(1));
   m_tbLevel2->SetValue(s);
   s.Printf(wxT("%1.3lf"), m_frame->GetTrgLevel(2));
   m_tbLevel3->SetValue(s);
   s.Printf(wxT("%1.3lf"), m_frame->GetTrgLevel(3));
   m_tbLevel4->SetValue(s);
}

void TriggerDialog::OnClose( wxCommandEvent& event )
{
   this->Hide();
}

void TriggerDialog::OnButton( wxCommandEvent& event )
{
   m_frame->SetTriggerConfig(event.GetId(), event.IsChecked()); 
}

void TriggerDialog::OnTriggerLevel( wxCommandEvent& event )
{
   if (event.GetId() == ID_LEVEL1)
      m_frame->SetTrgLevel(0, atof(m_tbLevel1->GetValue().mb_str()));
   if (event.GetId() == ID_LEVEL2)
      m_frame->SetTrgLevel(1, atof(m_tbLevel2->GetValue().mb_str()));
   if (event.GetId() == ID_LEVEL3)
      m_frame->SetTrgLevel(2, atof(m_tbLevel3->GetValue().mb_str()));
   if (event.GetId() == ID_LEVEL4)
      m_frame->SetTrgLevel(3, atof(m_tbLevel4->GetValue().mb_str()));
}

void TriggerDialog::SetTriggerLevel(double level)
{
   wxString s;
   s.Printf(wxT("%1.3lf"), level);
   m_tbLevel1->SetValue(s);
   m_tbLevel2->SetValue(s);
   m_tbLevel3->SetValue(s);
   m_tbLevel4->SetValue(s);
}
