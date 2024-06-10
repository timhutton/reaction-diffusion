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
#include "ImportImageDialog.hpp"
#include "dialogs.hpp"
#include "IDs.hpp"
#include "prefs.hpp"
#include "wxutils.hpp"

// readybase:
#include <utils.hpp>

// wxWidgets:
#include <wx/filename.h>

using namespace std;

// -----------------------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(ImportImageDialog, wxDialog)
END_EVENT_TABLE()

// -----------------------------------------------------------------------------------------------

ImportImageDialog::ImportImageDialog(wxWindow *parent, wxString filename, int num_chemicals, int i_target_chemical,
        float in_low, float in_high, float out_low, float out_high)
    : wxDialog(parent,wxID_ANY,_("Import an image"),wxDefaultPosition,wxDefaultSize,wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
    , in_low(in_low)
    , in_high(in_high)
    , out_low(out_low)
    , out_high(out_high)
{
    // create the controls
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    SetSizer(vbox);

    wxStaticText* source_label = new wxStaticText(this, wxID_STATIC, _("Image source:"));

    wxBoxSizer* hbox1 = new wxBoxSizer(wxHORIZONTAL);
    {
        this->filename_edit = new wxTextCtrl(this, wxID_ANY, filename, wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
        this->filename_edit->SetMinSize(wxSize(400, -1));
        hbox1->Add(this->filename_edit, 1, wxEXPAND | wxRIGHT, 10);
        wxButton *change_filename_button = new wxButton(this, wxID_ANY, _("..."));
        change_filename_button->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &ImportImageDialog::OnChangeFilename, this);
        hbox1->Add(change_filename_button, 0, wxLEFT | wxRIGHT, 0);
    }

    wxStaticText* convert_label = new wxStaticText(this, wxID_STATIC, _("(will be converted to grayscale if not already)"));

    wxBoxSizer* hbox2 = new wxBoxSizer(wxHORIZONTAL);
    {
        wxStaticText* chemical_label = new wxStaticText(this, wxID_STATIC, _("Apply to chemical:"));
        hbox2->Add(chemical_label, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, STDHGAP);
        this->chemical_combo = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);
        for (int i = 0; i < num_chemicals; ++i)
            this->chemical_combo->AppendString(GetChemicalName(i));
        this->chemical_combo->SetSelection(i_target_chemical);
        hbox2->Add(chemical_combo, 1, wxLEFT | wxRIGHT, STDHGAP);
    }

    wxBoxSizer* hbox3 = new wxBoxSizer(wxHORIZONTAL);
    {
        wxStaticText* map_label1 = new wxStaticText(this, wxID_STATIC, _("Map color range:"));
        hbox3->Add(map_label1, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, STDHGAP);
        this->in_low_edit = new wxTextCtrl(this, wxID_ANY, wxString::Format("%.1f", in_low), wxDefaultPosition, wxSize(50,-1), wxTE_RIGHT);
        hbox3->Add(this->in_low_edit, 1, wxGROW | wxLEFT | wxRIGHT, STDHGAP);
        wxStaticText* map_label2 = new wxStaticText(this, wxID_STATIC, _("-"));
        hbox3->Add(map_label2, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, STDHGAP);
        this->in_high_edit = new wxTextCtrl(this, wxID_ANY, wxString::Format("%.1f", in_high), wxDefaultPosition, wxSize(50, -1));
        hbox3->Add(this->in_high_edit, 1, wxGROW | wxLEFT | wxRIGHT, STDHGAP);
        wxStaticText* map_label3 = new wxStaticText(this, wxID_STATIC, _("onto"));
        hbox3->Add(map_label3, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, STDHGAP);
        this->out_low_edit = new wxTextCtrl(this, wxID_ANY, wxString::Format("%.1f", out_low), wxDefaultPosition, wxSize(50, -1), wxTE_RIGHT);
        hbox3->Add(this->out_low_edit, 1, wxGROW | wxLEFT | wxRIGHT, STDHGAP);
        wxStaticText* map_label4 = new wxStaticText(this, wxID_STATIC, _("-"));
        hbox3->Add(map_label4, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, STDHGAP);
        this->out_high_edit = new wxTextCtrl(this, wxID_ANY, wxString::Format("%.1f", out_high), wxDefaultPosition, wxSize(50, -1));
        hbox3->Add(this->out_high_edit, 1, wxGROW | wxLEFT | wxRIGHT, STDHGAP);
    }

    wxBoxSizer* buttbox = new wxBoxSizer(wxHORIZONTAL);
    wxSizer* stdbutts = CreateButtonSizer(wxOK | wxCANCEL);
    buttbox->Add(stdbutts, 1, wxGROW | wxRIGHT, STDHGAP);
    wxSize minsize = buttbox->GetMinSize();
    if (minsize.GetWidth() < 250) {
        minsize.SetWidth(250);
        buttbox->SetMinSize(minsize);
    }

    vbox->AddSpacer(12);
    vbox->Add(source_label, 0, wxLEFT | wxRIGHT, 10);
    vbox->Add(hbox1, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    vbox->Add(convert_label, 0, wxLEFT | wxRIGHT, 10);
    vbox->AddSpacer(10);
    vbox->Add(hbox2, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    vbox->AddSpacer(10);
    vbox->Add(hbox3, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    vbox->AddSpacer(12);
    vbox->Add(buttbox, 1, wxGROW | wxTOP | wxBOTTOM, 10);

    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
}

// -----------------------------------------------------------------------------------------------

void ImportImageDialog::OnChangeFilename(wxCommandEvent& event)
{
    wxFileDialog opendlg(this,
                         _("Choose an image file to import"),
                         wxEmptyString,
                         wxEmptyString,
                         _("Image files (*.jpg;*.png;*.bmp)|*.jpg;*.jpeg;*.png;*.bmp"),
                         wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (opendlg.ShowModal() != wxID_OK)
        return;

    this->filename_edit->SetValue(opendlg.GetPath());
    double scalar_range[2];
    GetScalarRangeFromImage(opendlg.GetPath(), scalar_range);
    this->in_low_edit->SetValue(wxString::Format("%.1f", scalar_range[0])); // (overwrites any previous changes)
    this->in_high_edit->SetValue(wxString::Format("%.1f", scalar_range[1]));
}

// -----------------------------------------------------------------------------------------------

bool ImportImageDialog::TransferDataFromWindow()
{
    const wxString errorCaption = _("Error in dialog");
    const int errorStyle = wxOK | wxCENTRE | wxICON_ERROR;
    this->image_filename = this->filename_edit->GetValue();
    if (this->image_filename.IsEmpty()) {
        wxMessageBox(_("No filename entered."), errorCaption, errorStyle);
        return false;
    }
    this->iTargetChemical = this->chemical_combo->GetSelection();
    if (!this->in_low_edit->GetValue().ToDouble(&this->in_low)) {
        wxMessageBox(_("Cannot convert value to number."), errorCaption, errorStyle);
        return false;
    }
    if (!this->in_high_edit->GetValue().ToDouble(&this->in_high)) {
        wxMessageBox(_("Cannot convert value to number."), errorCaption, errorStyle);
        return false;
    }
    if (!this->out_low_edit->GetValue().ToDouble(&this->out_low)) {
        wxMessageBox(_("Cannot convert value to number."), errorCaption, errorStyle);
        return false;
    }
    if (!this->out_high_edit->GetValue().ToDouble(&this->out_high)) {
        wxMessageBox(_("Cannot convert value to number."), errorCaption, errorStyle);
        return false;
    }
    return true;
}

// -----------------------------------------------------------------------------------------------
