/*  Copyright 2011-2024 The Ready Bunch

    This file is part of Ready.

    Ready is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ready is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Ready. If not, see <http://www.gnu.org/licenses/>.         */

// local:
#include "dialogs.hpp"
#include "wxutils.hpp"
#include "IDs.hpp"

// readybase:
#include "ImageRD.hpp"

// -----------------------------------------------------------------------------

MonospaceMessageBox::MonospaceMessageBox(const wxString& message, const wxString& title, const wxArtID& icon)
     : wxDialog(NULL,wxID_ANY,title,wxDefaultPosition,wxDefaultSize,wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    #ifdef __WXMAC__
        // need bigger font on Mac, and need to specify facename to get Monaco instead of Courier
        wxFont font(12, wxFONTFAMILY_MODERN,   wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Monaco"));
    #else
        wxFont font(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, _T("Monospace"), wxFONTENCODING_DEFAULT);
    #endif
    wxTextCtrl *text = new wxTextCtrl(this,wxID_ANY,message,wxDefaultPosition,wxDefaultSize,
        wxTE_MULTILINE|wxTE_READONLY|wxTE_DONTWRAP);
    text->SetFont(font);
    text->SetMinSize(wxSize(800,500));
    #ifdef __WXMAC__
        // nicer to use white bg on Mac
        text->SetBackgroundColour(*wxWHITE);
    #else
        text->SetBackgroundColour(this->GetBackgroundColour());
    #endif
    text->SetForegroundColour(*wxBLACK);
    this->SetIcon(wxArtProvider::GetIcon(icon));
    vbox->Add(text,wxSizerFlags(1).Expand().DoubleBorder());
    vbox->Add(this->CreateButtonSizer(wxOK),wxSizerFlags(0).Expand().DoubleBorder());
    this->SetSizerAndFit(vbox);
    this->ShowModal();
}

// =============================================================================

XYZIntDialog::XYZIntDialog(wxWindow* parent, const wxString& title,
                     int inx, int iny, int inz,
                     const wxPoint& pos, const wxSize& size)
{
    Create(parent, wxID_ANY, title, pos, size);

    // create the controls
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    SetSizer(vbox);

    wxStaticText* promptlabel = new wxStaticText(this, wxID_STATIC, _("Enter new X, Y, Z values:"));

    xbox = new wxTextCtrl(this, wxID_ANY, wxString::Format(wxT("%d"),inx), wxDefaultPosition, wxSize(50,wxDefaultCoord));
    ybox = new wxTextCtrl(this, wxID_ANY, wxString::Format(wxT("%d"),iny), wxDefaultPosition, wxSize(50,wxDefaultCoord));
    zbox = new wxTextCtrl(this, wxID_ANY, wxString::Format(wxT("%d"),inz), wxDefaultPosition, wxSize(50,wxDefaultCoord));

    wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

    hbox->AddStretchSpacer(1);

    hbox->Add(new wxStaticText(this, wxID_STATIC, wxT("X = ")), 0, wxALIGN_CENTER_VERTICAL, 0);
    hbox->Add(xbox, 0, wxALIGN_CENTER_VERTICAL, 0);

    hbox->AddStretchSpacer(1);

    hbox->Add(new wxStaticText(this, wxID_STATIC, wxT("Y = ")), 0, wxALIGN_CENTER_VERTICAL, 0);
    hbox->Add(ybox, 0, wxALIGN_CENTER_VERTICAL, 0);

    hbox->AddStretchSpacer(1);

    hbox->Add(new wxStaticText(this, wxID_STATIC, wxT("Z = ")), 0, wxALIGN_CENTER_VERTICAL, 0);
    hbox->Add(zbox, 0, wxALIGN_CENTER_VERTICAL, 0);

    hbox->AddStretchSpacer(1);

    wxSizer* stdbutts = CreateButtonSizer(wxOK | wxCANCEL);

    // position the controls
    wxBoxSizer* buttbox = new wxBoxSizer(wxHORIZONTAL);
    buttbox->Add(stdbutts, 1, wxGROW | wxRIGHT, STDHGAP);
    wxSize minsize = buttbox->GetMinSize();
    if (minsize.GetWidth() < 250) {
        minsize.SetWidth(250);
        buttbox->SetMinSize(minsize);
    }

    vbox->AddSpacer(12);
    vbox->Add(promptlabel, 0, wxLEFT | wxRIGHT, 10);
    vbox->AddSpacer(10);
    vbox->Add(hbox, 0, wxALL | wxEXPAND | wxALIGN_TOP, 0);
    vbox->AddSpacer(12);
    vbox->Add(buttbox, 1, wxGROW | wxTOP | wxBOTTOM, 10);

    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);

    if (pos == wxDefaultPosition) Centre();
    if (size != wxDefaultSize) SetSize(size);

    // select X value (must do this last on Windows)
    xbox->SetFocus();
    xbox->SetSelection(-1,-1);
}

// -----------------------------------------------------------------------------

bool XYZIntDialog::ValidNumber(wxTextCtrl* box, int* val)
{
    // validate given X/Y/Z value
    wxString str = box->GetValue();
    long i;
    if ( str.ToLong(&i) ) {
        *val = (int)i;
        return true;
    } else {
        Warning(_("Error converting number "));
        box->SetFocus();
        box->SetSelection(-1,-1);
        return false;
    }
}

// -----------------------------------------------------------------------------

bool XYZIntDialog::TransferDataFromWindow()
{
    return ValidNumber(xbox, &xval) &&
           ValidNumber(ybox, &yval) &&
           ValidNumber(zbox, &zval);
}

// =============================================================================

XYZFloatDialog::XYZFloatDialog(wxWindow* parent, const wxString& title,
                     float inx, float iny, float inz,
                     const wxPoint& pos, const wxSize& size)
{
    Create(parent, wxID_ANY, title, pos, size);

    // create the controls
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    SetSizer(vbox);

    wxStaticText* promptlabel = new wxStaticText(this, wxID_STATIC, _("Enter new X, Y, Z values:"));

    xbox = new wxTextCtrl(this, wxID_ANY, FormatFloat(inx), wxDefaultPosition, wxSize(50,wxDefaultCoord));
    ybox = new wxTextCtrl(this, wxID_ANY, FormatFloat(iny), wxDefaultPosition, wxSize(50,wxDefaultCoord));
    zbox = new wxTextCtrl(this, wxID_ANY, FormatFloat(inz), wxDefaultPosition, wxSize(50,wxDefaultCoord));

    wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

    hbox->AddStretchSpacer(1);

    hbox->Add(new wxStaticText(this, wxID_STATIC, wxT("X = ")), 0, wxALIGN_CENTER_VERTICAL, 0);
    hbox->Add(xbox, 0, wxALIGN_CENTER_VERTICAL, 0);

    hbox->AddStretchSpacer(1);

    hbox->Add(new wxStaticText(this, wxID_STATIC, wxT("Y = ")), 0, wxALIGN_CENTER_VERTICAL, 0);
    hbox->Add(ybox, 0, wxALIGN_CENTER_VERTICAL, 0);

    hbox->AddStretchSpacer(1);

    hbox->Add(new wxStaticText(this, wxID_STATIC, wxT("Z = ")), 0, wxALIGN_CENTER_VERTICAL, 0);
    hbox->Add(zbox, 0, wxALIGN_CENTER_VERTICAL, 0);

    hbox->AddStretchSpacer(1);

    wxSizer* stdbutts = CreateButtonSizer(wxOK | wxCANCEL);

    // position the controls
    wxBoxSizer* buttbox = new wxBoxSizer(wxHORIZONTAL);
    buttbox->Add(stdbutts, 1, wxGROW | wxRIGHT, STDHGAP);
    wxSize minsize = buttbox->GetMinSize();
    if (minsize.GetWidth() < 250) {
        minsize.SetWidth(250);
        buttbox->SetMinSize(minsize);
    }

    vbox->AddSpacer(12);
    vbox->Add(promptlabel, 0, wxLEFT | wxRIGHT, 10);
    vbox->AddSpacer(10);
    vbox->Add(hbox, 0, wxALL | wxEXPAND | wxALIGN_TOP, 0);
    vbox->AddSpacer(12);
    vbox->Add(buttbox, 1, wxGROW | wxTOP | wxBOTTOM, 10);

    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);

    if (pos == wxDefaultPosition) Centre();
    if (size != wxDefaultSize) SetSize(size);

    // select X value (must do this last on Windows)
    xbox->SetFocus();
    xbox->SetSelection(-1,-1);
}

// -----------------------------------------------------------------------------

bool XYZFloatDialog::TransferDataFromWindow()
{
    double x,y,z;
    bool ok = this->xbox->GetValue().ToDouble(&x) && this->ybox->GetValue().ToDouble(&y) && this->zbox->GetValue().ToDouble(&z);
    if (ok)
    {
        this->xval = x;
        this->yval = y;
        this->zval = z;
    }
    return ok;
}

// =============================================================================

BEGIN_EVENT_TABLE(MultiLineDialog, wxDialog)
    EVT_BUTTON (wxID_OK, MultiLineDialog::OnOK)
END_EVENT_TABLE()

MultiLineDialog::MultiLineDialog(wxWindow *parent,
                                 const wxString& caption,
                                 const wxString& message,
                                 const wxString& value)
     : wxDialog(GetParentForModalDialog(parent, wxOK | wxCANCEL),
                wxID_ANY, caption, wxDefaultPosition, wxDefaultSize,
                wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    wxBeginBusyCursor();

    wxBoxSizer* topsizer = new wxBoxSizer( wxVERTICAL );

    wxSizerFlags flagsBorder2;
    flagsBorder2.DoubleBorder();

    topsizer->Add(CreateTextSizer(message), flagsBorder2);

    m_textctrl = new wxTextCtrl(this, wxID_ANY, value,
                                wxDefaultPosition, wxSize(100,50), wxTE_MULTILINE | wxTE_PROCESS_TAB);
    #ifdef __WXMAC__
        // need bigger font on Mac, and need to specify facename to get Monaco instead of Courier
        wxFont font(12, wxFONTFAMILY_MODERN,   wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Monaco"));
    #else
        wxFont font(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, _T("Monospace"), wxFONTENCODING_DEFAULT);
    #endif
    m_textctrl->SetFont(font);

    // allow people to use small dialog if they wish
    // m_textctrl->SetMinSize(wxSize(800,500));

    topsizer->Add(m_textctrl, wxSizerFlags(1).Expand().TripleBorder(wxLEFT | wxRIGHT));

    wxSizer* buttonSizer = CreateSeparatedButtonSizer(wxOK | wxCANCEL);
    topsizer->Add(buttonSizer, wxSizerFlags(flagsBorder2).Expand());

    SetAutoLayout(true);
    SetSizer(topsizer);

    topsizer->SetSizeHints(this);
    topsizer->Fit(this);

    m_textctrl->SetFocus();
    m_textctrl->SetSelection(0,0);      // probably nicer not to select all text

    wxEndBusyCursor();
}

// -----------------------------------------------------------------------------

void MultiLineDialog::OnOK(wxCommandEvent& WXUNUSED(event))
{
    m_value = m_textctrl->GetValue();
    EndModal(wxID_OK);
}

// =============================================================================

#ifdef __WXOSX__

BEGIN_EVENT_TABLE(ParameterDialog, wxDialog)
    EVT_TIMER (wxID_ANY, ParameterDialog::OnOneTimer)
END_EVENT_TABLE()

void ParameterDialog::OnOneTimer(wxTimerEvent& WXUNUSED(event))
{
    if (namebox) {
        namebox->SetFocus();
        valuebox->SetFocus();
        valuebox->SetSelection(-1,-1);
    }
}

#endif

// -----------------------------------------------------------------------------

ParameterDialog::ParameterDialog(wxWindow* parent, bool can_edit_name,
                                 const wxString& inname, float inval,
                                 const wxPoint& pos, const wxSize& size)
{
    Create(parent, wxID_ANY, _("Change parameter"), pos, size);

    // create the controls
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    SetSizer(vbox);

    wxString prompt = can_edit_name ? _("Enter a new name and/or a new value:")
                                    : _("Enter a new value:");
    wxStaticText* promptlabel = new wxStaticText(this, wxID_STATIC, prompt);

    wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
    hbox->AddStretchSpacer(1);

    if (can_edit_name) {
        namebox = new wxTextCtrl(this, wxID_ANY, inname);
        hbox->Add(namebox, 0, wxALIGN_CENTER_VERTICAL, 0);
    } else {
        namebox = NULL;
        hbox->Add(new wxStaticText(this, wxID_STATIC, inname), 0, wxALIGN_CENTER_VERTICAL, 0);
    }

    hbox->Add(new wxStaticText(this, wxID_STATIC, wxT(" = ")), 0, wxALIGN_CENTER_VERTICAL, 0);

    valuebox = new wxTextCtrl(this, wxID_ANY, FormatFloat(inval));
    hbox->Add(valuebox, 0, wxALIGN_CENTER_VERTICAL, 0);
    hbox->AddStretchSpacer(1);

    wxSizer* stdbutts = CreateButtonSizer(wxOK | wxCANCEL);

    // position the controls
    wxBoxSizer* buttbox = new wxBoxSizer(wxHORIZONTAL);
    buttbox->Add(stdbutts, 1, wxGROW | wxRIGHT, STDHGAP);
    wxSize minsize = buttbox->GetMinSize();
    if (minsize.GetWidth() < 250) {
        minsize.SetWidth(250);
        buttbox->SetMinSize(minsize);
    }

    vbox->AddSpacer(12);
    vbox->Add(promptlabel, 0, wxLEFT | wxRIGHT, 10);
    vbox->AddSpacer(10);
    vbox->Add(hbox, 0, wxALL | wxEXPAND | wxALIGN_TOP, 0);
    vbox->AddSpacer(12);
    vbox->Add(buttbox, 1, wxGROW | wxTOP | wxBOTTOM, 10);

    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);

    if (pos == wxDefaultPosition) Centre();
    if (size != wxDefaultSize) SetSize(size);

    #ifdef __WXOSX__
        // due to wxOSX bug we have to set focus after dialog creation (see OnOneTimer)
        onetimer = new wxTimer(this, wxID_ANY);
        if (onetimer) onetimer->Start(10, wxTIMER_ONE_SHOT);
    #else
        // select value (must do this last on Windows)
        valuebox->SetFocus();
        valuebox->SetSelection(-1,-1);
    #endif
}

// -----------------------------------------------------------------------------

bool ParameterDialog::TransferDataFromWindow()
{
    if (namebox) name = namebox->GetValue();
    wxString valstr = valuebox->GetValue();
    double dbl;
    if (valstr.ToDouble(&dbl)) {
        value = (float)dbl;
        return true;
    } else {
        Warning(_("Error converting value to float"));
        return false;
    }
}

// =============================================================================

StringDialog::StringDialog(wxWindow* parent, const wxString& title,
                           const wxString& prompt, const wxString& instring,
                           const wxPoint& pos, const wxSize& size)
{
    Create(parent, wxID_ANY, title, pos, size);

    // create the controls
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topSizer);

    textbox = new wxTextCtrl(this, wxID_ANY, instring);

    wxStaticText* promptlabel = new wxStaticText(this, wxID_STATIC, prompt);

    wxSizer* stdbutts = CreateButtonSizer(wxOK | wxCANCEL);

    // position the controls
    wxBoxSizer* stdhbox = new wxBoxSizer(wxHORIZONTAL);
    stdhbox->Add(stdbutts, 1, wxGROW | wxRIGHT, STDHGAP);
    wxSize minsize = stdhbox->GetMinSize();
    if (minsize.GetWidth() < 250) {
        minsize.SetWidth(250);
        stdhbox->SetMinSize(minsize);
    }

    topSizer->AddSpacer(12);
    topSizer->Add(promptlabel, 0, wxLEFT | wxRIGHT, 10);
    topSizer->AddSpacer(10);
    topSizer->Add(textbox, 0, wxGROW | wxLEFT | wxRIGHT, 10);
    topSizer->AddSpacer(12);
    topSizer->Add(stdhbox, 1, wxGROW | wxTOP | wxBOTTOM, 10);

    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);

    if (pos == wxDefaultPosition) Centre();
    if (size != wxDefaultSize) SetSize(size);

    // select initial string (must do this last on Windows)
    textbox->SetFocus();
    textbox->SetSelection(-1,-1);
}

// -----------------------------------------------------------------------------

bool StringDialog::TransferDataFromWindow()
{
    result = textbox->GetValue();
    return true;
}

// =============================================================================

#ifdef __WXOSX__

BEGIN_EVENT_TABLE(IntegerDialog, wxDialog)
    EVT_TIMER (wxID_ANY, IntegerDialog::OnOneTimer)
END_EVENT_TABLE()

void IntegerDialog::OnOneTimer(wxTimerEvent& WXUNUSED(event))
{
    wxSpinCtrl* s1 = (wxSpinCtrl*) FindWindowById(ID_SPIN_CTRL);
    wxSpinCtrl* s2 = (wxSpinCtrl*) FindWindowById(ID_SPIN_CTRL+1);
    if (s1 && s2) {
        // first need to change focus to hidden control
        s2->SetFocus();
        s1->SetFocus();
        s1->SetSelection(-1,-1);
    }
}

#endif

// -----------------------------------------------------------------------------

#ifdef __WXMAC__

// override key event handler for wxSpinCtrl to allow key checking
class MySpinCtrl : public wxSpinCtrl
{
    public:
        MySpinCtrl(wxWindow* parent, wxWindowID id, const wxString& val = wxEmptyString,
                   const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize) :
            wxSpinCtrl(parent, id, val, pos, size)
        {
            // create a dynamic event handler for the underlying wxTextCtrl
            wxTextCtrl* textctrl = GetText();
            if (textctrl) {
                textctrl->Connect(wxID_ANY, wxEVT_CHAR,
                                  wxKeyEventHandler(IntegerDialog::OnSpinCtrlChar));
            }
        }
};

void IntegerDialog::OnSpinCtrlChar(wxKeyEvent& event)
{
    int key = event.GetKeyCode();

    if (event.CmdDown()) {
        // allow handling of cmd-x/v/etc
        event.Skip();

    } else if ( key == WXK_TAB ) {
        wxSpinCtrl* sc = (wxSpinCtrl*) FindWindowById(ID_SPIN_CTRL);
        if ( sc ) {
            sc->SetFocus();
            sc->SetSelection(-1,-1);
        }

    } else if ( key >= ' ' && key <= '~' ) {
        if ( (key >= '0' && key <= '9') || key == '+' || key == '-' ) {
            // allow digits and + or -
            event.Skip();
        } else {
            // disallow any other displayable ascii char
            Beep();
        }

    } else {
        event.Skip();
    }
}

#else

#define MySpinCtrl wxSpinCtrl

#endif // __WXMAC__

// -----------------------------------------------------------------------------

IntegerDialog::IntegerDialog(wxWindow* parent,
                             const wxString& title,
                             const wxString& prompt,
                             int inval, int minval, int maxval,
                             const wxPoint& pos, const wxSize& size)
{
    Create(parent, wxID_ANY, title, pos, size);

    minint = minval;
    maxint = maxval;

    // create the controls
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topSizer);

    spinctrl = new MySpinCtrl(this, ID_SPIN_CTRL, wxEmptyString,
                              wxDefaultPosition, wxSize(100,wxDefaultCoord));
    spinctrl->SetRange(minval, maxval);
    spinctrl->SetValue(inval);

    #ifdef __WXOSX__
        // create hidden spin ctrl so we can SetFocus below
        wxSpinCtrl* hidden = new wxSpinCtrl(this, ID_SPIN_CTRL+1, wxEmptyString,
                                            wxPoint(-1000,-1000), wxDefaultSize);
        hidden->SetValue(666);
    #endif

    wxStaticText* promptlabel = new wxStaticText(this, wxID_STATIC, prompt);

    wxSizer* stdbutts = CreateButtonSizer(wxOK | wxCANCEL);

    // position the controls
    wxBoxSizer* stdhbox = new wxBoxSizer(wxHORIZONTAL);
    stdhbox->Add(stdbutts, 1, wxGROW | wxRIGHT, STDHGAP);
    wxSize minsize = stdhbox->GetMinSize();
    if (minsize.GetWidth() < 250) {
        minsize.SetWidth(250);
        stdhbox->SetMinSize(minsize);
    }

    topSizer->AddSpacer(12);
    topSizer->Add(promptlabel, 0, wxLEFT | wxRIGHT, 10);
    topSizer->AddSpacer(10);
    topSizer->Add(spinctrl, 0, wxALIGN_CENTER, 0);
    topSizer->AddSpacer(12);
    topSizer->Add(stdhbox, 1, wxGROW | wxTOP | wxBOTTOM, 10);

    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);

    if (pos == wxDefaultPosition) Centre();
    if (size != wxDefaultSize) SetSize(size);

    #ifdef __WXOSX__
        // due to wxOSX bug we have to set focus after dialog creation
        onetimer = new wxTimer(this, wxID_ANY);
        if (onetimer) onetimer->Start(10, wxTIMER_ONE_SHOT);
    #else
        // select initial value (must do this last on Windows)
        spinctrl->SetFocus();
        spinctrl->SetSelection(-1,-1);
    #endif
}

// -----------------------------------------------------------------------------

bool IntegerDialog::TransferDataFromWindow()
{
    // spinctrl->GetValue() always returns a value within range even if
    // the text ctrl doesn't contain a valid number -- yuk!
    result = spinctrl->GetValue();
    if (result < minint || result > maxint) {
        wxString msg;
        msg.Printf(_("Value must be from %d to %d."), minint, maxint);
        Warning(msg);
        spinctrl->SetFocus();
        spinctrl->SetSelection(-1,-1);
        return false;
    } else {
        return true;
    }
}

// =============================================================================

FloatDialog::FloatDialog(wxWindow* parent, const wxString& title,
                         const wxString& prompt, float inval,
                         const wxPoint& pos, const wxSize& size)
{
    Create(parent, wxID_ANY, title, pos, size);

    // create the controls
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topSizer);

    textbox = new wxTextCtrl(this, wxID_ANY, wxString::Format(_T("%f"),inval));

    wxStaticText* promptlabel = new wxStaticText(this, wxID_STATIC, prompt);

    wxSizer* stdbutts = CreateButtonSizer(wxOK | wxCANCEL);

    // position the controls
    wxBoxSizer* stdhbox = new wxBoxSizer(wxHORIZONTAL);
    stdhbox->Add(stdbutts, 1, wxGROW | wxRIGHT, STDHGAP);
    wxSize minsize = stdhbox->GetMinSize();
    if (minsize.GetWidth() < 250) {
        minsize.SetWidth(250);
        stdhbox->SetMinSize(minsize);
    }

    topSizer->AddSpacer(12);
    topSizer->Add(promptlabel, 0, wxLEFT | wxRIGHT, 10);
    topSizer->AddSpacer(10);
    topSizer->Add(textbox, 0, wxGROW | wxLEFT | wxRIGHT, 10);
    topSizer->AddSpacer(12);
    topSizer->Add(stdhbox, 1, wxGROW | wxTOP | wxBOTTOM, 10);

    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);

    if (pos == wxDefaultPosition) Centre();
    if (size != wxDefaultSize) SetSize(size);

    // select initial string (must do this last on Windows)
    textbox->SetFocus();
    textbox->SetSelection(-1,-1);
}

// -----------------------------------------------------------------------------

bool FloatDialog::TransferDataFromWindow()
{
    double dr;
    if(!this->textbox->GetValue().ToDouble(&dr))
    {
        Warning(_("Failed to parse float"));
        return false;
    } else {
        this->result = dr;
        return true;
    }
}

// =============================================================================
