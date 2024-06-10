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
#include "RecordingDialog.hpp"
#include "dialogs.hpp"
#include "IDs.hpp"
#include "prefs.hpp"

// wxWidgets:
#include <wx/filename.h>

using namespace std;

// -----------------------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(RecordingDialog, wxDialog)
  EVT_COMBOBOX(ID::SourceCombo, RecordingDialog::OnSourceSelectionChange)
END_EVENT_TABLE()

// -----------------------------------------------------------------------------------------------

RecordingDialog::RecordingDialog(wxWindow *parent,
                                 bool is_2D_data_available,
                                 bool are_multiple_chemicals_available,
                                 bool default_is_2D_data,
                                 bool is_3D_surface_available)
    : wxDialog(parent,wxID_ANY,_("Recording settings"),wxDefaultPosition,wxDefaultSize,wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
    , source_current_view(_("current view"))
    , source_2D_data(_("2D data"))
    , source_2D_data_all_chemicals(_("2D data (all chemicals)"))
    , source_3D_surface(_("3D surface"))
{
    // create the controls
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    SetSizer(vbox);

    wxStaticText* source_label = new wxStaticText(this, wxID_STATIC, _("Image source:"));
    this->source_combo = new wxComboBox(this, ID::SourceCombo, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);
    this->source_combo->AppendString(this->source_current_view);
    if(is_2D_data_available)
        this->source_combo->AppendString(this->source_2D_data);
    if(are_multiple_chemicals_available)
        this->source_combo->AppendString(this->source_2D_data_all_chemicals);
    if (is_3D_surface_available)
        this->source_combo->AppendString(this->source_3D_surface);
    this->source_combo->SetSelection(default_is_2D_data ? 1 : 0);

    wxStaticText* folder_label = new wxStaticText(this, wxID_STATIC, _("Save frames here: (will overwrite)"));
    wxBoxSizer* hbox1 = new wxBoxSizer(wxHORIZONTAL);
    {
        this->folder_edit = new wxTextCtrl(this,wxID_ANY);
        this->folder_edit->SetValue(recordingdir); // from prefs
        this->folder_edit->SetMinSize(wxSize(400,-1));
        hbox1->Add(this->folder_edit,1, wxEXPAND | wxRIGHT,10);
        wxButton *change_folder_button = new wxButton(this,wxID_ANY,_("..."));
        change_folder_button->Bind(wxEVT_COMMAND_BUTTON_CLICKED,&RecordingDialog::OnChangeFolder,this);
        hbox1->Add(change_folder_button,0,wxLEFT | wxRIGHT,0);
    }

    wxStaticText* filenames_label = new wxStaticText(this, wxID_STATIC, _("Filenames:"));
    wxBoxSizer* hbox2 = new wxBoxSizer(wxHORIZONTAL);
    {
        this->filename_prefix_edit = new wxTextCtrl(this,wxID_ANY,_("frame_"));
        hbox2->Add(this->filename_prefix_edit,1,wxRIGHT,10);
        hbox2->Add(new wxStaticText(this, wxID_STATIC, _("000000")),0, wxRIGHT | wxALIGN_BOTTOM,10);
        this->extension_combo = new wxComboBox(this,wxID_ANY,wxEmptyString,wxDefaultPosition,wxDefaultSize,0,NULL,wxCB_READONLY);
        this->extension_combo->AppendString(_(".png"));
        this->extension_combo->AppendString(_(".jpg"));
        this->extension_combo->SetSelection(0);
        hbox2->Add(this->extension_combo,0,wxLEFT | wxRIGHT,0);
    }

    wxBoxSizer* hbox3 = new wxBoxSizer(wxHORIZONTAL);
    {
        this->should_decimate_check = new wxCheckBox(this, wxID_ANY, _("Reduce triangle count to"));
        this->should_decimate_check->SetValue(true);
        this->should_decimate_check->Enable(false);
        hbox3->Add(this->should_decimate_check, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 10);
        this->target_reduction_edit = new wxTextCtrl(this, wxID_ANY, _("20"));
        this->target_reduction_edit->Enable(false);
        hbox3->Add(this->target_reduction_edit, 0, wxRIGHT, 10);
        hbox3->Add(new wxStaticText(this, wxID_STATIC, _("%")), 0, wxALIGN_CENTER_VERTICAL, 10);
    }

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
    vbox->Add(source_label, 0, wxLEFT | wxRIGHT, 10);
    vbox->Add(this->source_combo, 0, wxLEFT | wxRIGHT, 10);
    vbox->AddSpacer(10);
    vbox->Add(folder_label, 0, wxLEFT | wxRIGHT, 10);
    vbox->Add(hbox1, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    vbox->AddSpacer(10);
    vbox->Add(filenames_label, 0, wxLEFT | wxRIGHT, 10);
    vbox->Add(hbox2, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    vbox->AddSpacer(12);
    vbox->Add(hbox3, 0, wxLEFT | wxRIGHT, 10);
    vbox->AddSpacer(12);
    vbox->Add(buttbox, 1, wxGROW | wxTOP | wxBOTTOM, 10);

    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
}

// -----------------------------------------------------------------------------------------------

bool RecordingDialog::Validate()
{
    if( !wxDirExists(this->folder_edit->GetValue()) )
    {
        int answer = wxMessageBox("Folder doesn't exist. Create?","Confirm",wxOK|wxCANCEL);
        if( answer == wxOK )
        {
            bool ret = wxFileName::Mkdir(this->folder_edit->GetValue(),wxS_DIR_DEFAULT,wxPATH_MKDIR_FULL);
            if( !ret )
            {
                wxMessageBox("Unable to create folder.","Error",wxOK|wxICON_ERROR);
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    if (!this->target_reduction_edit->GetValue().ToDouble(&this->target_reduction))
        return false;
    return wxDialog::Validate();
}

// -----------------------------------------------------------------------------------------------

bool RecordingDialog::TransferDataFromWindow()
{
    this->record_data_image = (this->source_combo->GetValue()==this->source_2D_data) || (this->source_combo->GetValue()==this->source_2D_data_all_chemicals);
    this->record_all_chemicals = (this->source_combo->GetValue()==this->source_2D_data_all_chemicals);
    this->recording_extension = string(this->extension_combo->GetValue().mb_str());
    this->record_3D_surface = (this->source_combo->GetValue() == this->source_3D_surface);
    recordingdir = this->folder_edit->GetValue(); // save folder in prefs
    this->recording_prefix = string(this->folder_edit->GetValue().mb_str()) + "/" + string(this->filename_prefix_edit->GetValue().mb_str());
    this->should_decimate = this->should_decimate_check->GetValue();
    this->target_reduction_edit->GetValue().ToDouble(&this->target_reduction);
    // TODO: save other settings in prefs too, if wanted
    return true;
}

// -----------------------------------------------------------------------------------------------

void RecordingDialog::OnChangeFolder(wxCommandEvent& event)
{
    wxDirDialog dlg(NULL, "Choose target folder", this->folder_edit->GetValue(), wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    if(dlg.ShowModal()!=wxID_OK) return;
    this->folder_edit->SetValue(dlg.GetPath());
}

// -----------------------------------------------------------------------------------------------

void RecordingDialog::OnSourceSelectionChange(wxCommandEvent& event)
{
    this->extension_combo->Clear();
    if (this->source_combo->GetValue() == this->source_3D_surface)
    {
        this->extension_combo->AppendString(_(".obj"));
        this->extension_combo->AppendString(_(".ply"));
        this->extension_combo->AppendString(_(".vtp"));
        this->should_decimate_check->Enable(true);
        this->target_reduction_edit->Enable(true);
    }
    else
    {
        this->extension_combo->AppendString(_(".png"));
        this->extension_combo->AppendString(_(".jpg"));
        this->should_decimate_check->Enable(false);
        this->target_reduction_edit->Enable(false);
    }
    this->extension_combo->SetSelection(0);
}

// -----------------------------------------------------------------------------------------------
